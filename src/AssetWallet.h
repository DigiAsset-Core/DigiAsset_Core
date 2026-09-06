//
// Created by DigiAsset Core on 14/07/26.
//
// Helper functions for building wallet backed asset transactions.
// Used by the sendasset, issueasset and getwalletbalances RPC methods.
//

#ifndef DIGIASSET_CORE_ASSETWALLET_H
#define DIGIASSET_CORE_ASSETWALLET_H

#include "DigiAssetTypes.h"
#include "DigiByteTransaction.h"
#include <jsoncpp/json/value.h>
#include <string>
#include <vector>

namespace AssetWallet {

    /**
     * Returns all spendable wallet UTXOs with their asset data populated from the local database.
     * @param minconf - minimum number of confirmations(assets should use at least 1 since the
     *                  local database only knows about confirmed asset UTXOs)
     */
    std::vector<AssetUTXO> getWalletUTXOs(int minconf = 1);

    /**
     * Throws DigiByteException if the asset carries a rule that a wallet built transfer can
     * never satisfy, so the caller refuses the send instead of destroying the holding.
     *
     * addRuleOutputs() runs on issuance only - no part of the transfer path adds rule outputs.
     * A transfer of an asset whose rules require them therefore fails DigiAsset::checkRulesPass
     * when it is decoded, and the handler for that failure clears the asset from *every* output
     * in the transaction, the change output included.  Sending 1 of 5 units destroys all 5,
     * silently.  Refusing up front costs nothing: such a transfer could never have succeeded.
     *
     * Only rules no transfer can satisfy are refused.  KYC and vote restrictions are not, because
     * a send to an allowed address is legal and must keep working.
     */
    void assertTransferableAsset(const DigiAsset& asset);

    /**
     * As above, but with the chain state passed in rather than looked up, so the rule logic can
     * be exercised without a database.  chainHeight and nowSeconds are only read by the expiry
     * check; note getIfExpired() wants seconds, while the expiry is stored in ms.
     */
    void assertTransferableAsset(const DigiAsset& asset, unsigned int chainHeight, uint64_t nowSeconds);

    /**
     * Selects wallet UTXOs holding at least `amount` of the given asset.
     * Prefers UTXOs that hold only the requested asset so other assets don't need to be moved.
     * Throws DigiByteTransaction::exceptionNotEnoughFunds if the wallet lacks the requested amount.
     * Throws DigiByteException if the asset can not be transferred at all - see
     * assertTransferableAsset(), which this calls before choosing any input.
     */
    std::vector<AssetUTXO> selectAssetInputs(uint64_t assetIndex, uint64_t amount);

    /**
     * Converts a user supplied amount(display units - so 1.5 of a 2 decimal asset is 150 sats)
     * in to the smallest divisible units.  Accepts integers, doubles and decimal strings.
     */
    uint64_t parseAssetAmount(const Json::Value& amount, uint8_t decimals);

    /**
     * Formats a DigiByte sat count as a decimal DGB string for wallet RPC calls.
     */
    std::string satsToDecimal(uint64_t sats);

    /**
     * Rough miner fee estimate in sats for a not yet funded asset transaction: the node's
     * estimatesmartfee rate(min relay rate fallback) over an approximated funded size
     * (base + op_return + outputs + one funding input + change).  DigiByte fees are tiny
     * so rough is fine - used by the dryrun option of the asset RPC methods.
     */
    uint64_t estimateMinerFee(const DigiByteTransaction& tx);

    /**
     * Takes a fully described asset transaction(inputs, outputs and issuance data set),
     * then funds(pays the fee), signs and broadcasts it via the DigiByte wallet.
     *
     * While funding, all wallet UTXOs that carry assets or are unconfirmed get temporarily
     * locked so the wallet can not accidentally spend assets as transaction fees.
     *
     * @param tx - transaction to send(must be writable and pass encodeAssetOpReturn)
     * @param signedHex - optional output of the raw signed transaction hex
     * @return - txid of the broadcast transaction
     */
    std::string fundSignSend(const DigiByteTransaction& tx, std::string* signedHex = nullptr);

} // namespace AssetWallet

#endif //DIGIASSET_CORE_ASSETWALLET_H
