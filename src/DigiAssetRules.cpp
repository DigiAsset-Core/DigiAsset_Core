//
// Created by mctrivia on 14/06/23.
//

#include "DigiAsset.h"
#include "DigiAssetRules.h"
#include <algorithm>
#include "DigiAssetTypes.h"
#include "serialize.h"

using namespace std;



/**
 * Rules data always takes up a multiple of 8bits but the commands withing it are not bytewise aligned.  After the last
 * rule is added four 1s are added to signal the end of the rules and than the remainder of the byte is padded with ones.
 *
 * Each rule starts with a 4 bit rule header and has variable length rules.  The rule headers are
 * 0x0 - Approval - This rule allows you to define 1 or more addresses that must approve a transaction for this asset
 * 0x1 - Royalties - This rule allows you to define 1 or more addresses that must be paid for a transaction to be valid
 * 0x2 - Geofence Allowed - Allows defining a list of countries that are allowed to receive the asset
 * 0x3 - Geofence Denied - Allows defining a list of countries that are not allowed to receive the asset
 * 0x4 - Define a vote asset or an asset that has an expiry
 * 0x5 - Define an asset that requires assets be burned with every transaction
 * 0x9 - Define what units the royalty price is defined in
 * 0xf - Rules done
 *
 * @param txData - transaction data
 * @param dataStream - data encoded in the chain with the pointer set to the beginning of the rules
 * @param cid - cid of meta data(needed to get vote labels only.  rules are on chain)
 * @param opCode - used to set if rules are rewritable or not
 */
DigiAssetRules::DigiAssetRules(const getrawtransaction_t& txData, BitIO& dataStream, const string& cid,
                               unsigned char opCode) {
    //check if any rules
    if ((opCode != 3) && (opCode != 4)) return;
    _noRules = false;

    //check if rewritable
    _rewritable = (opCode == 3);
    unsigned char ruleCode;
    do {
        ruleCode = dataStream.getBits(4);
        switch (ruleCode) {
            case RULE_END:
                break;
            case RULE_APPROVAL:
                decodeApproval(txData, dataStream);
                break;
            case RULE_ROYALTY_UNITS:
                decodeRoyaltyUnits(txData, dataStream);
                //intentionally no break
            case RULE_ROYALTIES:
                decodeRoyalties(txData, dataStream);
                break;
            case RULE_GEOFENCE_ALLOWED:
                _countryListIsBan = false;
                decodeGeofence(txData, dataStream);
                break;
            case RULE_GEOFENCE_DENIED:
                _countryListIsBan = true;
                decodeGeofence(txData, dataStream);
                break;
            case RULE_VOTE://RULE_EXPIRY also
                decodeVoteAndExpiry(txData, dataStream, cid);
                break;
            case RULE_DEFLATION:
                decodeDeflation(txData, dataStream);
                break;
            default:
                throw out_of_range("Invalid Rule Code");
        }
    } while (ruleCode != RULE_END);

    //if not byte aligned move to start of next byte
    size_t pointer = dataStream.getPosition();
    size_t extraBits = pointer % 8;
    if (extraBits != 0) dataStream.movePositionBy(8 - extraBits);
}

/**
 * This function decodes the approval data rule
 *
 * header nibble of 0 already been processed
 * 1 to 7 bytes - fixed precision encoded - required weight for the transaction to be valid
 * 2 to 12 bytes per approver or group of approvers
 *    if first bit is 0:
 *      1-4 bytes: fixed precision output number+1      (+1) so first byte is never 0x00
 *      1-7 bytes: fixed precision weight
 *
 *   if first bit is 1 then it is a range of outputs that are non asset outputs
 *      1-4 bytes: fixed precision start output number(with MSB forced to 1)
 *      1-7 bytes: fixed precision number of outputs included
 *      weight is value sent to the output-600 sats
 *      using range base immediately ends rule.
 *
 * 1 byte ending equal to 0x00(only if not range values.  There will only ever be 1 range so that is end if present)
 */
void DigiAssetRules::decodeApproval(const getrawtransaction_t& txData, BitIO& dataStream) {
    //get number of signers required
    _signersRequired = dataStream.getFixedPrecision();

    //get list of valid signers
    bool notDone = true;
    while (notDone) {
        //see if command is range or specific
        bool isRange = (dataStream.getBits(1) == 1);

        if (isRange) {


            //range
            dataStream.insertBits(0, 1);
            dataStream.movePositionBy(
                    -1);                            //move back 1 bit since this 0 is part of output number
            unsigned int start = dataStream.getFixedPrecision();
            unsigned int length = dataStream.getFixedPrecision();
            for (unsigned int outputNum = start; outputNum < start + length; outputNum++) {
                _signers.emplace_back(Signer{
                        .address = txData.vout[outputNum].scriptPubKey.addresses[0],
                        .weight =  txData.vout[outputNum].valueS - 600
                });
            }
            notDone = false;


        } else {


            //per output
            dataStream.movePositionBy(
                    -1);                         //move back 1 bit since this 0 is part of output number
            uint64_t temp = dataStream.getFixedPrecision();
            if (temp == 0) {
                notDone = false;
            } else {
                unsigned int outputNum = temp - 1;
                _signers.emplace_back(Signer{
                        .address = txData.vout[outputNum].scriptPubKey.addresses[0],
                        .weight =  dataStream.getFixedPrecision()
                });
            }
        }
    }
}

/**
 * This function decodes the royalty units
 *
 * header nibble of 9 already been processed
 * If first bit is 1:
 *      than next 7 bits indicate standard exchange rate index number
 *
 * If first bit is 0:
 *      1-4 bytes - fixed precision value indicating that you should use the address of that output number.
 *                  The index is the value sent to it -600
 */
void DigiAssetRules::decodeRoyaltyUnits(const getrawtransaction_t& txData, BitIO& dataStream) {
    if (dataStream.getBits(1) == 1) {

        //standard currency
        _exchangeRate = DigiAsset::standardExchangeRates[dataStream.getBits(7)];

    } else {

        //non standard
        dataStream.movePositionBy(-1);
        unsigned int output = dataStream.getFixedPrecision();
        string address = txData.vout[output].scriptPubKey.addresses[0];
        _exchangeRate = {
                .address = address,
                .index =  static_cast<unsigned char>(txData.vout[output].valueS - 600),
                .name =   "?"
        };

        //make sure exchange address is being watched
        Database* db = Database::GetInstance();
        db->addWatchAddress(address);

    }
}

/**
 * This function decodes the royalties
 *
 * header nibble of 1 already been processed
 * 1-7 bytes: fixed precision output number start
 * 1-7 bytes: fixed precision number of outputs included
 * cost is value sent to the output(can not be less then 600sats)
 */
void DigiAssetRules::decodeRoyalties(const getrawtransaction_t& txData, BitIO& dataStream) {
    unsigned int start = dataStream.getFixedPrecision();
    unsigned int length = dataStream.getFixedPrecision();
    for (unsigned int outputNum = start; outputNum < start + length; outputNum++) {
        _royalties.emplace_back(Royalty{
                .address = txData.vout[outputNum].scriptPubKey.addresses[0],
                .amount =  txData.vout[outputNum].valueS
        });
    }
}

/**
 * This function decodes the geofence
 * You can create either a list of countries that are allowed to receive or a list of countries that can not receive.
 * Both cant be defined(because that would be pointless)
 *
 * header nibble of 2 or 3 already been processed
 * 2 bytes per country code( ISO 3166-1 alpha-3  encoded in 3B40String format)
 * 2 bytes - 0xf9ff to mark end
 *
 * ISO 3166-1 alpha-3 Information: https://en.wikipedia.org/wiki/ISO_3166-1_alpha-3
 * 3B40String format:  BitIO::get3B40String
 */
void DigiAssetRules::decodeGeofence(const getrawtransaction_t& txData, BitIO& dataStream) {
    //get list of countries
    string value;
    while (value != "...") {
        value = dataStream.get3B40String(3);
        std::transform(value.begin(), value.end(), value.begin(), ::toupper); //make value upper case
        if (value != "...") _countryList.push_back(value);
    }
}

/**
 * Vote rules allow restricting sending to only specific addresses.  It also will also let core track the vote counts
 *
 *  header nibble of 4 already been processed
 *  1 bit marking if movable
 *  7 bit number of vote options
 *  1-7 byte fixed precision - cutoff.
 *      0 means no cutoff.
 *      1 to MIN_EPOCH_VALUE-1 - block height
 *      MIN_EPOCH_VALUE and up - means time in ms since January 1st, 1970 values under are block count
 *                              This encoding will not cause any problems for next 750,000 years.  Some time before then
 *                              A DigiAsset V4 must be made that moves this value up to a new minimum time based on the
 *                              epoch of that development
 * 1-7 byte fixed precision - address list
 *     0 use standard list
 *     >0 - start at output x-1 and use the address funds where sent to
 */
void DigiAssetRules::decodeVoteAndExpiry(const getrawtransaction_t& txData, BitIO& dataStream, const std::string& cid) {
    if (_rewritable) throw out_of_range("Invalid Rule Detected: Votes can not be part of rewritable rule asset");
    _movable = (dataStream.getBits(1) == 1);
    unsigned char voteLength = dataStream.getBits(7);
    uint64_t cutoff = dataStream.getFixedPrecision();
    int voteStart = dataStream.getFixedPrecision() - 1;
    if (cutoff != 0) _expiry = cutoff;
    if (voteLength == 0) return;

    //get address list
    if (voteStart == -1) {
        //default list(recommended as counts are tracked and garbage auto collected)
        for (unsigned char i = 0; i < voteLength; i++) {
            _voteOptions.emplace_back(VoteOption{
                    .address = DigiAsset::standardVoteAddresses[i],
                    .label = ""
            });
        }

    } else {
        //from outputs
        for (unsigned int outputNum = voteStart; outputNum < (unsigned) voteStart + voteLength; outputNum++) {
            _voteOptions.emplace_back(VoteOption{
                    .address = txData.vout[outputNum].scriptPubKey.addresses[0],
                    .label = ""
            });
        }
    }
    _voteLabelsCID = cid;
}

/**
 * Deflationary rule defines that every transaction must burn ar least this many assets
 *
 *  header nibble of 5 already been processed
 *  1-7 byte fixed precision - number of assets that must be burned(in assets base units - so if value is 5 and decimals
 *    are 2 than this means 0.05 assets must be burned per output)
 */
void DigiAssetRules::decodeDeflation(const getrawtransaction_t& txData, BitIO& dataStream) {
    _deflate = dataStream.getFixedPrecision();
}

/**
 * Forces the rules in to a locked state(gets executed for all locked assets)
 */
void DigiAssetRules::lock() {
    _rewritable = false;
}

bool DigiAssetRules::isRewritable() const {
    return _rewritable;
}

bool DigiAssetRules::empty() const {
    return _noRules;
}

uint64_t DigiAssetRules::getExpiry() const {
    return _expiry;
}

/**
 * returns if the current expiry is a block height(false means ms from epoch)
 * @return
 */
bool DigiAssetRules::isExpiryHeight() const {
    return (_expiry < MIN_EPOCH_VALUE);
}

/**
 * returns if the asset ever expires
 * @return
 */
bool DigiAssetRules::expires() const {
    return (_expiry != EXPIRE_NEVER);
}

/**
 * Function that serializes a DigiAssetRule and appends data to serializedData
 * @param serializedData
 * @param input
 */
void serialize(vector<uint8_t>& serializedData, const DigiAssetRules& input) {
    const uint8_t versionCode = 0;    //version code to allow detection of old serialization protocols.  max value is 15
    if (input.empty()) return;

    //get booleans and version code
    serialize(serializedData, (uint8_t) (
            input._rewritable * 128 +
            input._movable * 64 +
            input._countryListIsBan * 32 +
            versionCode
    ));

    //get data that will be included
    serialize(serializedData, (uint8_t) (
            (input._signersRequired > 0) * 128 +
            (!input._royalties.empty()) * 64 +
            (!input._countryList.empty()) * 32 +
            (!input._voteOptions.empty()) * 16 +
            (input._expiry != DigiAssetRules::EXPIRE_NEVER) * 8 +
            (input._deflate != 0) * 4 +
            (input._exchangeRate.enabled()) * 2 +
            (!input._voteLabelsCID.empty())
    ));


    //add signers
    if (input._signersRequired > 0) {
        serialize(serializedData, input._signersRequired);
        serialize(serializedData, input._signers);
    }

    //add royalties
    if (!input._royalties.empty()) {
        serialize(serializedData, input._royalties);
    }

    //add kyc country list
    if (!input._countryList.empty()) {
        serialize(serializedData, input._countryList);
    }

    //add vote options
    if (!input._voteOptions.empty()) {
        serialize(serializedData, input._voteOptions);
    }

    //add expiry
    if (input._expiry != DigiAssetRules::EXPIRE_NEVER) {
        serialize(serializedData, input._expiry);
    }

    //add deflate
    if (input._deflate != 0) {
        serialize(serializedData, input._deflate);
    }

    //add exchange rate
    if (input._exchangeRate.enabled()) {
        serialize(serializedData, input._exchangeRate);
    }

    //add vote label cid if not processed
    if (!input._voteLabelsCID.empty()) {
        serialize(serializedData, input._voteLabelsCID);
    }
}

/**
 * Function that serializes a DigiAssetRule and appends data to result
 * @param result
 * @param input
 */
void deserialize(const vector<uint8_t>& serializedData, size_t& i, DigiAssetRules& output) {
    //check if no rules
    if (serializedData.empty()) {
        output = DigiAssetRules();
        return;
    }

    //there are rules
    output._noRules = false;

    //get booleans and version code
    uint8_t flags;
    deserialize(serializedData, i, flags);
    if ((flags & 0x1f) != 0) throw out_of_range("Unknown serialization");
    output._rewritable = flags & 128;
    output._movable = flags & 64;
    output._countryListIsBan = flags & 32;

    //get data that will be included
    deserialize(serializedData, i, flags);

    //add signers
    if (flags & 128) {
        deserialize(serializedData, i, output._signersRequired);
        deserialize(serializedData, i, output._signers);
    }

    //add royalties
    if (flags & 64) {
        deserialize(serializedData, i, output._royalties);
    }

    //add kyc country list
    if (flags & 32) {
        deserialize(serializedData, i, output._countryList);
    }

    //add vote options
    if (flags & 16) {
        deserialize(serializedData, i, output._voteOptions);
    }

    //add expiry
    if (flags & 8) {
        deserialize(serializedData, i, output._expiry);
    }

    //add deflate
    if (flags & 4) {
        deserialize(serializedData, i, output._deflate);
    }

    //add exchange rate
    if (flags & 2) {
        deserialize(serializedData, i, output._exchangeRate);
    }

    //add vote label cid if not processed
    if (flags & 1) {
        deserialize(serializedData, i, output._voteLabelsCID);
    }
}

bool DigiAssetRules::getIfExpired(unsigned int height, uint64_t time) const {
    if (_expiry == EXPIRE_NEVER) return false;
    if (_expiry >= MIN_EPOCH_VALUE) {
        //time based expiry
        return (time >= _expiry / 1000);
    } else {
        //block height based expiry
        return (height > _expiry);
    }
}

uint64_t DigiAssetRules::getRequiredSignerWeight() const {
    return _signersRequired;
}

std::vector<Signer> DigiAssetRules::getSigners() const {
    return _signers;
}

bool DigiAssetRules::getIfRequiresRoyalty() const {
    return (!_royalties.empty());
}

ExchangeRate DigiAssetRules::getRoyaltyCurrency() const {
    return _exchangeRate;
}

std::vector<Royalty> DigiAssetRules::getRoyalties() const {
    return _royalties;
}

bool DigiAssetRules::getIfGeoFenced() const {
    return (_countryListIsBan || (!_countryList.empty()));
}

bool DigiAssetRules::getIfCountryAllowedToReceive(const string& country) const {
    //easy case kyc not required
    if (!getIfGeoFenced()) return true;

    //find if country in list
    bool inList = false;
    for (const string& countryInList: _countryList) {
        if (countryInList == country) {
            inList = true;
            break;
        }
    }

    //if white list then should be on list.  If black list should not be on list(XOR gate)
    return (((!_countryListIsBan) && inList) || (_countryListIsBan && (!inList)));
}

bool DigiAssetRules::getIfVoteRestricted() const {
    return !_movable;
}

bool DigiAssetRules::getIfValidVoteAddress(const string& address) const {
    for (const VoteOption& option: _voteOptions) {
        if (option.address == address) return true;
    }
    return false;
}

uint64_t DigiAssetRules::getRequiredBurn() const {
    return _deflate;
}

void DigiAssetRules::setRewritable(bool state) {
    _noRules = false;
    _rewritable = state;
}

void DigiAssetRules::setRequireSigners(uint64_t requiredWeight, const vector<Signer>& signers) {
    _noRules = false;
    _signersRequired = requiredWeight;
    _signers = signers;
}

void DigiAssetRules::setRequireKYC() {
    _noRules = false;
    _countryListIsBan = true;
    _countryList.clear();
}

void DigiAssetRules::setDoesNotRequireKYC() {
    _noRules = false;
    _countryListIsBan = false;
    _countryList.clear();
}

void DigiAssetRules::setRequireKYC(const vector<std::string>& countries, bool banList) {
    if ((countries.empty()) && (!banList)) throw exceptionInvalidRule();
    _noRules = false;
    _countryListIsBan = banList;
    _countryList = countries;
}

void DigiAssetRules::setRoyalties(const vector<Royalty>& royalties, const ExchangeRate& rate) {
    _noRules = false;
    _royalties = royalties;
    _exchangeRate = rate;
}

void DigiAssetRules::setVote(const vector<VoteOption>& voteOptions, uint64_t expiry) {
    _noRules = false;
    _voteOptions = voteOptions;
    _expiry = expiry;
}

void DigiAssetRules::setExpiry(uint64_t expiry) {
    _noRules = false;
    _expiry = expiry;
}

void DigiAssetRules::setDeflationary(uint64_t deflateRate) {
    _noRules = false;
    _deflate = deflateRate;
}

bool DigiAssetRules::operator==(const DigiAssetRules& b) {
    if (b._noRules != _noRules) return false;
    if (b._rewritable != _rewritable) return false;
    if (b._movable != _movable) return false;
    if (b._signersRequired != _signersRequired) return false;
    if (b._signers != _signers) return false;
    if (b._exchangeRate != _exchangeRate) return false;
    if (b._royalties != _royalties) return false;
    if (b._countryList != _countryList) return false;
    if (b._countryListIsBan != _countryListIsBan) return false;
    if (b._expiry != _expiry) return false;
    if (b._voteOptions != _voteOptions) return false;
    if (b._deflate != _deflate) return false;
    return true;
}

bool DigiAssetRules::getIfVote() const {
    return (!_voteOptions.empty());
}

/**
 * Converts DigiAssetRules Object into JSON for outputting by API.
 *
 * @return Json::Value - Returns a Json::Value object that represents the DigiAssetRules in JSON format.
 *
 * - changeable (bool): Indicates if the asset rules are changeable.
 * - deflation (unsigned int, optional): Number of assets that must be burned to make a valid transaction.
 * - expiry (unsigned int, optional): Block height or timestamp when asset can no longer be transferred.
 *   - values less than MIN_EPOCH_VALUE are block height.
 *   - values greater are epoch time in ms.
 * - royalty (Json::Value): Information about royalty payments, may contain:
 *   - units (Json::Value): Exchange rate units, may contain:
 *     - address (string): Conversion rate tracking address.
 *     - index (unsigned int): Conversion rate trackers can track up to 10 rates so this is which of the 10 (0 - 9).
 *     - name (string, optional): If it is one of the default exchange rates, there will be a currency code.
 *   - addresses (Json::Value): Royalty recipient addresses (key) and their amounts (value).
 *     - value is in sats. So if not units, then 10000000 = 1 DGB. If units point to USD, then 100000000 = 1 USD.
 * - geofence (Json::Value): Assets with this rule can only be sent to KYC verified addresses and will contain one of the following:
 *   - denied (array of strings): Everyone can hold the asset except those countries listed here.
 *   - allowed (array of strings): Only countries listed here can hold.
 * - voting (Json::Value): Voting options, may contain:
 *   - restricted (bool): If true, the asset can only be sent to one of the voting addresses.
 *   - options (Json::Value): Voting options addresses (key) and labels (value).
 * - approval (Json::Value): Rarely used rule but allows for an asset that requires approval by the creator for all trades.
 *   - required (unsigned int): Number of votes required for a transaction to be valid.
 *   - approvers (Json::Value): List of addresses (key) and their weights (value) that can cast their approval to the transaction.
 */
Json::Value DigiAssetRules::toJSON() {
    Json::Value result(Json::objectValue);

    // Handle simple results
    if (_noRules) return result;
    result["changeable"] = Json::Value(_rewritable);
    if (_deflate != 0) {
        result["deflation"] = Json::Value(_deflate);
    }
    if (expires()) {
        result["expiry"] = Json::Value(_expiry);
    }

    // Handle royalties
    if (getIfRequiresRoyalty()) {
        Json::Value royalty(Json::objectValue);

        if (_exchangeRate.enabled()) {
            Json::Value exchange(Json::objectValue);
            exchange["address"] = Json::Value(_exchangeRate.address);
            exchange["index"] = Json::Value(_exchangeRate.index);
            if (_exchangeRate.name != "?") {
                exchange["name"] = Json::Value(_exchangeRate.name);
            }
            royalty["units"] = exchange;
        }

        Json::Value addresses(Json::objectValue);
        for (const Royalty& recipient: _royalties) {
            addresses[recipient.address] = Json::Value(recipient.amount);
        }
        royalty["addresses"] = addresses;
        result["royalty"] = royalty;
    }

    // Handle geofence
    if (getIfGeoFenced()) {
        Json::Value geofence(Json::objectValue);
        Json::Value countries(Json::arrayValue);
        for (const string& country: _countryList) {
            countries.append(Json::Value(country));
        }

        if (_countryListIsBan) {
            geofence["denied"] = countries;
        } else {
            geofence["allowed"] = countries;
        }
        result["geofence"] = geofence;
    }

    // Handle voting
    if (!_voteOptions.empty()) {
        Json::Value vote(Json::objectValue);
        vote["restricted"] = Json::Value(!_movable);
        try {
            //download vote meta data and include
            Json::Value options(Json::objectValue);
            vector<VoteOption> voteOptions = getVoteOptions();
            for (const VoteOption& option: voteOptions) {
                options[option.address] = Json::Value(option.label);
            }
            vote["options"] = options;
        } catch (const exceptionVoteOptionsCorrupt& e) {
            //download failed so include list of addresses but flag that labels are corrupt
            Json::Value options(Json::objectValue);
            vector<VoteOption> voteOptions = _voteOptions;
            for (const VoteOption& option: voteOptions) {
                options[option.address] = "Label corrupt";
            }
            vote["options"] = options;
        }
        result["voting"] = vote;
    } else if (!_movable) {
        result["unmovable"] = Json::Value(true);
    }

    // Handle approval
    if (_signersRequired > 0) {
        Json::Value approval(Json::objectValue);
        approval["required"] = Json::Value(!_signersRequired);
        Json::Value signers(Json::objectValue);
        for (const Signer& option: _signers) {
            signers[option.address] = Json::Value(option.weight);
        }
        approval["approvers"] = signers;
        result["approval"] = approval;
    }

    return result;
}

/**
 * @brief Retrieves the voting options associated with the DigiAsset.
 *
 * This function fetches the voting options from the IPFS CID if they haven't been processed yet.
 * It validates the format and addresses of the voting options against what was stored in the blockchain.
 *
 * @throws exceptionVoteOptionsCorrupt If the voting options fetched from IPFS are corrupt or don't match the blockchain data.
 *
 * @return A vector of VoteOption structs containing the addresses and labels for each voting option.
 */
std::vector<VoteOption> DigiAssetRules::getVoteOptions() {
    if (_voteLabelsCID.empty()) {         //labels already processed so skip that step
        IPFS* ipfs = IPFS::GetInstance();
        string content = ipfs->callOnDownloadSync(_voteLabelsCID);

        //parse returned data
        Json::Reader reader;
        Json::Value metadata;
        if (!reader.parse(content, metadata)) {
            throw exceptionVoteOptionsCorrupt();
        }

        //make sure in correct format for vote labels and get vote options
        vector<VoteOption> options;
        if (!metadata.isMember("votes")) throw exceptionVoteOptionsCorrupt();
        Json::Value votes = metadata["votes"];
        if (!votes.isArray()) throw exceptionVoteOptionsCorrupt();
        bool usingString = (votes.empty() ||
                            votes[0].isString()); //allowed either array of vote labels or array of strings
        if ((usingString) && (votes.size() > DigiAsset::standardVoteCount)) {
            throw exceptionVoteOptionsCorrupt();
        }

        //process voting options
        for (size_t i = 0; i < votes.size(); ++i) {
            Json::Value vote = votes[static_cast<Json::ArrayIndex>(i)];

            if (usingString) {

                //array of string processing
                if (!vote.isString()) throw exceptionVoteOptionsCorrupt();   //invalid format
                options.emplace_back(VoteOption{
                        .address = DigiAsset::standardVoteAddresses[i],
                        .label = vote.asString()
                });

            } else {

                //array of object processing
                if (
                        !vote.isMember("address") || !vote["address"].isString() ||
                        !vote.isMember("label") || !vote["label"].isString()
                        ) {
                    throw exceptionVoteOptionsCorrupt();
                }
                options.emplace_back(VoteOption{
                        .address = vote["address"].asString(),
                        .label = vote["label"].asString()
                });

            }
        }

        //check all addresses match what was stored in the chain
        if (options != _voteOptions) {
            throw exceptionVoteOptionsCorrupt();
        }

        _voteOptions = options;
        _voteLabelsCID.clear();
    }
    return _voteOptions;
}