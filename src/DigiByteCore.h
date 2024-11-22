//
// Created by mctrivia on 30/01/23.
// Krzysztof Okupski's libbitcoin-api-cpp for reference
//

#ifndef DIGIBYTECORE_CONFIGDIGIBYTECORE_H
#define DIGIBYTECORE_CONFIGDIGIBYTECORE_H


#include "DigiByteCore_Exception.h"
#include "DigiByteCore_Types.h"
#include <iomanip>
#include <jsonrpccpp/client.h>
#include <jsonrpccpp/client/connectors/httpclient.h>
#include <mutex>
#include <random>
#include <string>


namespace jsonrpc {
    class HttpClient;

    class Client;
} // namespace jsonrpc

class DigiByteCore {
    std::unique_ptr<jsonrpc::HttpClient> httpClient = nullptr;
    std::unique_ptr<jsonrpc::Client> client = nullptr;
    uint64_t _dgbToSat(std::string value);
    static std::mutex _mutex;
    bool _useAssetPort = false;


    std::string _configFileName = "config.cfg";

    template<typename fn_t>
    auto errorCheckAPI(fn_t fn) -> decltype(fn());

    long long _runTime = 0;
    unsigned int _runCount = 0;

public:
    enum AddressTypes {
        LEGACY,
        SEGWIT,
        BECH32
    };


    std::string printProfilingInfo() {
        long long totalDuration = _runTime;
        int transactions = _runCount;
        long long avgDuration = transactions > 0 ? totalDuration / transactions : 0;

        std::ostringstream oss;
        oss << std::right << std::setw(30) << "DigiByte Core"
            << std::setw(20) << totalDuration
            << std::setw(20) << avgDuration
            << std::setw(20) << transactions << std::endl;
        return oss.str();
    }

    //constructor/destructor
    DigiByteCore() = default;
    ~DigiByteCore();

    //functions that drop connection
    void setFileName(const std::string& fileName, bool useAssetPort = false);
    void setConfig(const std::string& username, const std::string& password, const std::string& address = "127.0.0.1",
                   uint port = 14022);
    void dropConnection();

    //functions that create connection
    void makeConnection(); //will throw an error if we can't connect

    //config based getter
    std::string getFileName();

    //functions that require a connection.  all will throw a not connected error if connection lost
    uint getBlockCount();
    std::string getBlockHash(uint height);
    blockinfo_t getBlock(const std::string& hash);
    getrawtransaction_t getRawTransaction(const std::string& txid);
    std::vector<unspenttxout_t> listUnspent(int minconf = 1, int maxconf = 99999999, const std::vector<std::string>& addresses = {});
    getaddressinfo_t getAddressInfo(const std::string& address);


    /* === Auxiliary functions === */
    Json::Value sendcommand(const std::string& command, const Json::Value& params);

    std::string IntegerToString(int num);
    std::string RoundDouble(double num);


    /* === Node functions === */
    void addnode(const std::string& node, const std::string& command);
    std::vector<nodeinfo_t> getaddednodeinfo(bool dns);
    std::vector<nodeinfo_t> getaddednodeinfo(bool dns, const std::string& node);
    std::vector<peerinfo_t> getpeerinfo();
    int getconnectioncount();


    /* === Wallet functions === */
    void backupwallet(const std::string& destination);
    std::string encryptwallet(const std::string& passphrase);
    void walletlock();
    void walletpassphrase(const std::string& passphrase, int timeout);
    void walletpassphrasechange(const std::string& oldpassphrase, const std::string& newpassphrase);

    std::string dumpprivkey(const std::string& digibyteaddress);
    void importprivkey(const std::string& digibyteprivkey);
    void importprivkey(const std::string& digibyteprivkey, const std::string& label, bool rescan = true);
    void importaddress(const std::string& address, const std::string& account, bool rescan = true);

    std::string addmultisigaddress(int nrequired, const std::vector<std::string>& keys);
    std::string addmultisigaddress(int nrequired, const std::vector<std::string>& keys, const std::string& account);
    multisig_t createmultisig(int nrequired, const std::vector<std::string>& keys);
    std::string getnewaddress(const std::string& label = "", AddressTypes type = BECH32);
    validateaddress_t validateaddress(const std::string& digibyteaddress);

    void keypoolrefill();
    bool settxfee(double amount);
    double estimatefee(int blocks);

    std::string signmessage(const std::string& digibyteaddress, const std::string& message);
    bool verifymessage(const std::string& digibyteaddress, const std::string& signature, const std::string& message);

    getinfo_t getinfo();
    void stop();


    /* === Accounting === */
    double getbalance();
    double getbalance(const std::string& account, int minconf = 1, bool includewatchonly = false);
    double getunconfirmedbalance();

    double getreceivedbyaccount(const std::string& account, int minconf = 1);
    double getreceivedbyaddress(const std::string& digibyteaddress, int minconf = 1);

    std::vector<accountinfo_t> listreceivedbyaccount(int minconf = 1, bool includeempty = false);
    std::vector<addressinfo_t> listreceivedbyaddress(int minconf = 1, bool includeempty = false);

    gettransaction_t gettransaction(const std::string& tx, bool watch);
    std::vector<transactioninfo_t> listtransactions();
    std::vector<transactioninfo_t> listtransactions(const std::string& account, int count = 10, int from = 0);


    std::vector<std::string> getaddressesbylabel(const std::string& label, const std::string& type = "");
    std::vector<std::string> listlabels(const std::string& purpose = "");
    std::vector<std::vector<addressgrouping_t>> listaddressgroupings();

    std::string sendtoaddress(const std::string& digibyteaddress, double amount);
    std::string sendtoaddress(const std::string& digibyteaddress, double amount, const std::string& comment,
                              const std::string& comment_to);

    std::string sendfrom(const std::string& fromaccount, const std::string& todigibyteaddress, double amount);
    std::string sendfrom(const std::string& fromaccount, const std::string& todigibyteaddress, double amount,
                         const std::string& comment, const std::string& comment_to, int minconf = 1);

    std::string sendmany(const std::string& fromaccount, const std::map<std::string, double>& amounts);
    std::string
    sendmany(const std::string& fromaccount, const std::map<std::string, double>& amounts, const std::string comment,
             int minconf = 1);

    utxoinfo_t gettxout(const std::string& txid, int n, bool includemempool = true);
    utxosetinfo_t gettxoutsetinfo();

    std::vector<unspenttxout_t> listunspent(int minconf = 1, int maxconf = 99999999, const std::vector<std::string>& addresses = {});
    std::vector<txout_t> listlockunspent();
    bool lockunspent(bool unlock, const std::vector<txout_t>& outputs);


    /* === Mining functions === */
    std::string getbestblockhash();
    std::string getblockhash(int blocknumber);
    blockinfo_t getblock(const std::string& blockhash);
    int getblockcount();

    void setgenerate(bool generate, int genproclimit = -1);
    bool getgenerate();
    double getdifficulty();
    int gethashespersec();

    mininginfo_t getmininginfo();
    workdata_t getwork();
    bool getwork(const std::string& data);

    txsinceblock_t listsinceblock(const std::string& blockhash = "", int target_confirmations = 1);


    /* === Low level calls === */
    getrawtransaction_t getrawtransaction(const std::string& txid, bool verbose = false);
    decodescript_t decodescript(const std::string& hexString);
    decoderawtransaction_t decoderawtransaction(const std::string& hexString);
    std::string sendrawtransaction(const std::string& hexString, bool highFee);

    std::string createrawtransaction(const std::vector<txout_t>& inputs, const std::map<std::string, double>& amounts);
    std::string
    createrawtransaction(const std::vector<txout_t>& inputs, const std::map<std::string, std::string>& amounts);

    signrawtransaction_t signrawtransaction(const std::string& rawTx,
                                            const std::vector<signrawtxin_t> inputs = std::vector<signrawtxin_t>());
    signrawtransaction_t signrawtransaction(const std::string& rawTx, const std::vector<signrawtxin_t> inputs,
                                            const std::vector<std::string>& privkeys,
                                            const std::string& sighashtype = "ALL");

    std::vector<std::string> getrawmempool();
    std::string getrawchangeaddress();


    /*
    ███████╗██████╗ ██████╗  ██████╗ ██████╗ ███████╗
    ██╔════╝██╔══██╗██╔══██╗██╔═══██╗██╔══██╗██╔════╝
    █████╗  ██████╔╝██████╔╝██║   ██║██████╔╝███████╗
    ██╔══╝  ██╔══██╗██╔══██╗██║   ██║██╔══██╗╚════██║
    ███████╗██║  ██║██║  ██║╚██████╔╝██║  ██║███████║
    ╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝ ╚═════╝ ╚═╝  ╚═╝╚══════╝
     */
    class exception : public std::exception {
    protected:
        std::string _lastErrorMessage;
        mutable std::string _fullErrorMessage;

    public:
        explicit exception(const std::string& message = "Unknown") : _lastErrorMessage(message) {}

        virtual const char* what() const noexcept override {
            _fullErrorMessage = "DigiByte Core Exception: " + _lastErrorMessage;
            return _fullErrorMessage.c_str();
        }
    };

    class exceptionCoreOffline : public exception {
    public:
        explicit exceptionCoreOffline()
            : exception("Core Offline") {}
    };

    class exceptionDigiByteCoreNotConnected : public exception {
    public:
        explicit exceptionDigiByteCoreNotConnected()
            : exception("Core not connected") {} //Run makeConnection()
    };
};


#endif //DIGIBYTECORE_CONFIGDIGIBYTECORE_H
