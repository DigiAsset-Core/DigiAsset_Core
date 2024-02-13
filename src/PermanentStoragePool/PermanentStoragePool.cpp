//
// Created by mctrivia on 02/11/23.
//

#include "PermanentStoragePool.h"
#include "AppMain.h"
#include "Config.h"
#include "static_block.hpp"


using namespace std;
void PermanentStoragePool::setPoolIndexAndInitialize(unsigned int index, const Config& config) {
    //save pool index
    _poolIndex = index;

    //save config data
    const string prefix = "psp" + to_string(index);
    _subscribed = config.getBool(prefix + "subscribe", true);
    _autoRemoveBad = config.getBool(prefix + "pspautoremovebad", true);
    if (!_subscribed) return;

    //get payout address
    _payoutAddress = config.getString(prefix + "psppayout", "_psppayout");

    //handle condition payout address is a label
    if (!_payoutAddress.empty() && _payoutAddress[0] == '_') {
        DigiByteCore* dgb = AppMain::GetInstance()->getDigiByteCore();
        try {
            vector<string> addresses = dgb->getaddressesbylabel(_payoutAddress);
            _payoutAddress = addresses[0];
        } catch (const DigiByteException& e) {
            _payoutAddress = dgb->getnewaddress(_payoutAddress);
        }
    }

    //at start make sure all data that should be pinned is pinned
    Database* db = AppMain::GetInstance()->getDatabase();
    db->repinPermanent(_poolIndex);

    //let the pool know the config data in case they have extra config info
    _setConfig(config);
    start();
}
unsigned int PermanentStoragePool::getPoolIndex() const {
    return _poolIndex;
}
void PermanentStoragePool::markAssetAsPartOfPool(unsigned int assetIndex) {
    Database* db = AppMain::GetInstance()->getDatabase();
    db->addAssetToPool(_poolIndex, assetIndex);
}
bool PermanentStoragePool::isAssetPartOfPool(unsigned int assetIndex) const {
    Database* db = AppMain::GetInstance()->getDatabase();
    return db->isAssetInPool(_poolIndex, assetIndex);
}
bool PermanentStoragePool::isAssetBad(const std::string& assetId) {
    return false;
}
void PermanentStoragePool::repinAllFiles() const {
    Database* db = AppMain::GetInstance()->getDatabase();
    db->repinPermanent(_poolIndex);
}
bool PermanentStoragePool::subscribed() const {
    return _subscribed;
}
string PermanentStoragePool::getPayoutAddress() const {
    return _payoutAddress;
}

/**
 * Reports an asset as being in bad faith
 * @param assetId
 * @param internalOnly
 *   if true means the PSP called this function files will be unpinned if _autoRemoveBad is on
 *   if false means user called so files will be unpined and the PSP will receive warning that they may want to add to there list
 */
void PermanentStoragePool::reportAssetBad(const std::string& assetId, bool internalOnly) {
    //if calling as
    bool unpin = (!internalOnly || _autoRemoveBad);

    //remove the asset from the pool
    Database* db = AppMain::GetInstance()->getDatabase();
    db->removeAssetFromPool(_poolIndex, assetId, unpin);

    if (internalOnly) return;
    _reportAssetBad(assetId);
}

//override if you have a method of reporting bad assets
void PermanentStoragePool::_reportAssetBad(const std::string& assetId) {
    throw exceptionCouldntReport();
}

/**
 * Reports a file as being in bad faith
 * @param cid
 * @param internalOnly
 *   if true means the PSP called this function files will be unpinned if _autoRemoveBad is on
 *   if false means user called so files will be unpined and the PSP will receive warning that they may want to add to there list
 */
void PermanentStoragePool::reportFileBad(const string& cid, bool internalOnly) {
    //if calling as
    bool unpin = (!internalOnly || _autoRemoveBad);

    //remove the asset from the pool
    Database* db = AppMain::GetInstance()->getDatabase();
    db->removeFromPermanent(_poolIndex, cid, unpin);

    if (internalOnly) return;
    _reportAssetBad(cid);
}
void PermanentStoragePool::_reportFileBad(const string& cid) {
    throw exceptionCouldntReport();
}
void PermanentStoragePool::_setConfig(const Config& config) {
}
