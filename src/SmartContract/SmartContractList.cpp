//
// Created by mctrivia on 19/05/24.
//

#include "SmartContractList.h"
#include "AppMain.h"
#include "CurlHandler.h"
#include "Log.h"
#include "SmartContractMetadata.h"
#include "static_block.hpp"
#include "utils.h"
#include <boost/filesystem.hpp>
#include <iostream>
#include <regex>

#ifdef _WIN32
#include <windows.h>
#define DYNLIB_HANDLE HMODULE
#define DYNLIB_LOAD(a) LoadLibraryA(a)
#define DYNLIB_GETSYM(a, b) GetProcAddress(a, b)
#define DYNLIB_CLOSE(a) FreeLibrary(a)
#define DYNLIB_ERROR() GetLastError()
#else
#include <dlfcn.h>
#define DYNLIB_HANDLE void*
#define DYNLIB_LOAD(a) dlopen(a, RTLD_LAZY)
#define DYNLIB_GETSYM(a, b) dlsym(a, b)
#define DYNLIB_CLOSE(a) dlclose(a)
#define DYNLIB_ERROR() dlerror()
#endif

namespace fs = boost::filesystem;


// Static block to register our callback function with IPFS Controller
static_block {
    IPFS::registerCallback(SMARTCONTRACT_CALLBACK_NEWMETADATA_ID, SmartContractList::_callbackNewMetadata);
}

//define what function alias lookup que executes
std::function<void(const std::string&)> SmartContractList::_smartContractAliasProcessor = [](const std::string& contractAddress) -> void {
    Database* db= AppMain::GetInstance()->getDatabase();
    auto contract = db->getSmartContract(contractAddress);
    contract.getAlias();    //will cause it to try and verify the alias
};
QueueThread<std::string> SmartContractList::_aliasLookUpQueue{_smartContractAliasProcessor};

SmartContractList::SmartContractList() {
    //load all plugins
    updateAvailableContracts();

    //load the alias lookup from database
    auto contractsToDo=AppMain::GetInstance()->getDatabase()->getSmartContractsThatNeedAlias();
    for (const auto& contract: contractsToDo) {
        _aliasLookUpQueue.add(contract);
    }
}

void SmartContractList::updateAvailableContracts() {
    _contracts.clear();

    fs::path contractsDir("SmartContractList");
    if (fs::exists(contractsDir) && fs::is_directory(contractsDir)) {
        for (const auto& entry: fs::directory_iterator(contractsDir)) {
            if (fs::is_directory(entry.path())) {
                fs::path libPath = entry.path() / "libcontract";
#ifdef _WIN32
                libPath.replace_extension(".dll");
#elif __APPLE__
                libPath.replace_extension(".dylib");
#else
                libPath.replace_extension(".so");
#endif
                loadPlugin(libPath.string());
            }
        }
    }
}

void SmartContractList::loadPlugin(const std::string& path) {
    //get the name
    size_t start = path.find_last_of("/\\") + 1;
    size_t end = path.find_first_of("/", start);
    string name = (end != string::npos) ? path.substr(start, end - start) : path.substr(start);

    // Get the instance of the logging system and add a log message indicating the plugin is being loaded
    Log* log = log->GetInstance();
    log->addMessage("Loading Smart Contract: " + name);

    // Load the shared library (dynamic library) specified by the path
    DYNLIB_HANDLE handle = DYNLIB_LOAD(path.c_str());
    if (!handle) {
        std::cerr << "Cannot open library: " << DYNLIB_ERROR() << '\n';
        return;
    }

    // Define a type alias for the function pointer to the createHandler function with the required signature
    typedef SmartContractHandler* (*CreateHandler)(const std::function<std::string(const std::string&, const std::string&)>&);

    // Get the createHandler function from the loaded library
    CreateHandler createHandler = (CreateHandler) DYNLIB_GETSYM(handle, "createHandler");
    if (!createHandler) {
        std::cerr << "Cannot load symbol createHandler: " << DYNLIB_ERROR() << '\n';
        DYNLIB_CLOSE(handle);
        return;
    }

    // Create an instance of SmartContractHandler using the createHandler function
    std::shared_ptr<SmartContractHandler> handler(createHandler(rpcInterface));

    // Store the handler in the _contracts map with the path as the key
    _contracts[name] = handler;
}

std::shared_ptr<SmartContractHandler> SmartContractList::getContract(const std::string& name) {
    auto it = _contracts.find(name);
    if (it != _contracts.end()) {
        return it->second;
    }
    throw std::out_of_range("invalid plugin name");
}
bool SmartContractList::empty() {
    return _contracts.empty();
}

ContractChainData SmartContractList::processTX(const getrawtransaction_t& txData, unsigned int height,
                                            const std::function<std::string(const std::string&, unsigned int)>& addressGetterFunction) {
    //check there are 2 or 3 outputs
    if ((txData.vout.size() < 2) || (txData.vout.size() > 3)) return {};

    //check output 0 is 700 sats
    if (txData.vout[0].valueS != 700) return {};

    //check output 1 is 0 sat
    if (txData.vout[1].valueS != 0) return {};

    //check output 0 and 2 have only 1 address
    if (txData.vout[0].scriptPubKey.addresses.size() != 1) return {};
    if ((txData.vout.size() == 3) && (txData.vout[2].scriptPubKey.addresses.size() != 1)) return {};

    //check there are 1 or 2 inputs all from the same address
    if (txData.vin.size() > 2) return {};

    //check encoded data on output 1 has correct header
    if (txData.vout[1].scriptPubKey.type != "nulldata") return {};
    BitIO dataStream = BitIO::makeHexString(txData.vout[1].scriptPubKey.hex);
    if (!dataStream.checkIsBitcoinOpReturn()) return {};                         //not an OP_RETURN
    if (dataStream.getBitcoinDataHeader() != BITIO_BITCOIN_TYPE_DATA) return {}; //not data
    dataStream = dataStream.copyBitcoinData();                                   //strip the header out
    if (dataStream.getNumberOfBitLeft() < 48) return {};
    if (dataStream.getPlainOldAsciiString(2) != "DC") return {};
    uint32_t version = dataStream.getBits(32);
    string cid;
    if (version >= 2) {
        cid = dataStream.getPlainOldAsciiString();
        if (!IPFS::isValidCID(cid)) return {};
    }
    if (dataStream.getNumberOfBitLeft() != 0) return {};

    //check all inputs and if exist output 2 have same address
    string publishingAddress = addressGetterFunction(txData.vin[0].txid, txData.vin[0].n);
    if ((txData.vin.size() == 2) && (addressGetterFunction(txData.vin[0].txid, txData.vin[0].n) != publishingAddress)) return {};
    if ((txData.vout.size() == 3) && (addressGetterFunction(txData.txid, 3) != publishingAddress)) return {};

    //get the output0address
    string contractAddress=addressGetterFunction(txData.txid, 0);

    //pin the ipfs data and add callback to store the deposit addresses
    if (version<=MAX_SMART_CONTRACT_VERSION_SUPPORTED) {    //don't download or process if its a version we don't know
        IPFS* ipfs = AppMain::GetInstance()->getIPFS();
        ipfs->pin(cid, MAX_SMART_CONTRACT_SIZE);
        ipfs->callOnDownload(cid, "SMARTCONTRACTS", contractAddress, SMARTCONTRACT_CALLBACK_NEWMETADATA_ID);
    } //we still want to save so we can process after we upgrade

    //return results
    return {
            .publisherAddress = publishingAddress,
            .contractAddress = contractAddress,
            .version = version,
            .cid = cid};
}

// Iterator support
std::map<std::string, std::shared_ptr<SmartContractHandler>>::iterator SmartContractList::begin() { return _contracts.begin(); }
std::map<std::string, std::shared_ptr<SmartContractHandler>>::iterator SmartContractList::end() { return _contracts.end(); }
std::map<std::string, std::shared_ptr<SmartContractHandler>>::const_iterator SmartContractList::begin() const { return _contracts.begin(); }
std::map<std::string, std::shared_ptr<SmartContractHandler>>::const_iterator SmartContractList::end() const { return _contracts.end(); }


std::function<std::string(const std::string&, const std::string&)> SmartContractList::rpcInterface=[](const std::string& methodName, const std::string& params) -> std::string {
    cout << "rpcInterface\n";
    return AppMain::GetInstance()->getRpcServer()->executeCallByPlugin(methodName, params);
};

void SmartContractList::_callbackNewMetadata(const std::string& cid, const std::string& contractAddress, const std::string& content, bool failed) {
    Database* db=AppMain::GetInstance()->getDatabase();

    //check size is less than max if greater then delete
    try {
        //process metadata and make sure it is valid
        auto contractData=db->getSmartContractChainData(contractAddress);
        SmartContractMetadata contractMetadata=SmartContractMetadata(content, contractData.version, contractAddress, contractData.publisher);


        //get sources and add to database
        for (const auto& source: contractMetadata.getSources()) {
            db->addSmartContractSource(contractAddress,source,true);
        }

        //find any addresses that are referenced other than sources(won't cause issues if add again)
        auto neededResources = contractMetadata.getReferences();
        for (const string& addressOrAsset: neededResources) {
            db->addSmartContractSource(contractAddress,addressOrAsset,false);
        }

        //mark that we have finished loading all the critical data
        db->setSmartContractReturnData(contractAddress,contractMetadata.getReturnJSON());

        //validate the alias
        _aliasLookUpQueue.add(contractAddress);

    } catch (const SmartContractMetadata::exception& e) {
        db->deleteSmartContract(contractAddress);
        return;
    }
}



