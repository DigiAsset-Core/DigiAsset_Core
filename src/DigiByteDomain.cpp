//
// Created by mctrivia on 31/07/23.
//
#include "DigiByteDomain.h"
#include "IPFS.h"
#include "static_block.hpp"
#include <iostream>

using namespace std;


// Static block to register our callback function with IPFS Controller
static_block {
    IPFS::registerCallback(DIGIBYTEDOMAIN_CALLBACK_NEWMETADATA_ID, DigiByteDomain::_callbackNewMetadata);
}

/**
 * this function is executed every time an asset is issued so DigiByte Domain can process changes
 * @param asset
 */
void DigiByteDomain::processAssetIssuance(const DigiAsset& asset) {
    //check if an asset we care about and stop processing if not
    Database* db = Database::GetInstance();
    if (!db->isMasterDomainAssetId(asset.getAssetId())) return;

    //a request to download the metadata will have already been made
    //however metadata may not already be downloaded so lets tell the IPFS controller to run onNewMeta function when its
    //downloaded.  This function will get called in order of issuance even though its asynchronous
    IPFS* ipfs = IPFS::GetInstance();
    ipfs->callOnDownload(asset.getCID(), "DIGIBYTEDOMAIN", "", DIGIBYTEDOMAIN_CALLBACK_NEWMETADATA_ID);
}

void
DigiByteDomain::_callbackNewMetadata(const std::string& cid, const std::string& extra, const std::string& content,
                                     bool failed) {
    //failed will always be false since no maxSleep ever set

    Database* db = Database::GetInstance();
    Json::CharReaderBuilder rbuilder;
    Json::Value metadata;
    istringstream s(content);
    string errs;
    Json::parseFromStream(rbuilder, s, &metadata, &errs);


    //todo Renzo Code Needed Here

}

std::string DigiByteDomain::getAddress(const string& domain) {
    Database* db = Database::GetInstance();
    return db->getDomainAddress(domain);
}

std::string DigiByteDomain::getAssetId(const string& domain) {
    Database* db = Database::GetInstance();
    return db->getDomainAssetId(domain);
}

bool DigiByteDomain::isDomain(const string& domain) {
    if (domain.size() < 4) {
        return false;
    }
    return domain.substr(domain.size() - 4) == ".dgb";
}
