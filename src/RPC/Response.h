//
// Created by mctrivia on 04/04/24.
//

#ifndef DIGIASSET_CORE_RESPONSE_H
#define DIGIASSET_CORE_RESPONSE_H


#include <jsoncpp/json/value.h>
namespace RPC {
    class Response {
    private:
        Json::Value _result;
        bool _error=false;
        size_t _size=sizeof(Response);

        std::vector<std::string> _invalidateOnAddressChange; //invalidates if one of these addresses is modified
        std::vector<std::string> _invalidateOnAssetChange;   //invalidates if one of these assets is modified
        int _blocksGoodFor=0;  //invalidates when drops bellow 0
        bool _invalidateOnNewAsset=false;  //if true will get deleted if a new asset is issued

    public:
        Response()=default;

        //functions for setting response value
        void setResult(const Json::Value& result);
        void setError(const Json::Value& error);

        //functions for setting how long cache is good for
        void addInvalidateOnAddressChange(const std::string& address);
        void addInvalidateOnAssetChange(const std::string& assetId);
        void setBlocksGoodFor(int blocks);
        void setInvalidateOnNewAsset();

        //function to detect if there was no response
        bool empty() const;
        size_t size() const;

        //functions to check if the cache should get deleted
        size_t addressChanged(const std::string& address) const;    //returns size if should delete, 0 if shouldn't
        size_t assetChanged(const std::string& assetId) const;      //returns size if should delete, 0 if shouldn't
        size_t newBlockAdded();                                     //returns size if should delete, 0 if shouldn't
        size_t newAssetIssued();                                    //returns size if should delete, 0 if shouldn't

        //functions to convert to Json
        Json::Value toJSON(const Json::Value& id) const;
    };
}



#endif //DIGIASSET_CORE_RESPONSE_H
