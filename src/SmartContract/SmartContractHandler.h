//
// Created by mctrivia on 19/05/24.
//

#ifndef DIGIASSET_CORE_SMARTCONTRACTHANDLER_H
#define DIGIASSET_CORE_SMARTCONTRACTHANDLER_H


#include <functional>
#include <string>
#include <vector>

//All plugins should extend this class
class SmartContractHandler {
public:
    /**
     * Executed on planned shutdown.  Optional to implement
     */
    virtual ~SmartContractHandler() = default;


    /**
     * Used by chain analyzer only
     * Every time funds are sent to the smart contract address this function will be executed
     * @param trigger - string with JSON encoded vector<AssetUTXO> data for the funds that triggered it
     */
    virtual void executeContract(const std::string& trigger) = 0;




    /**
     * Function to return the publishing address.
     * This address ideally should get KYC verified and funds will need to be added to it to create contracts
     * @param optionalAccount - string leave blank if not using multiple accounts mode
     * @return
     */
    virtual std::string getPublishingAddress(std::string optionalAccount="") = 0;


    /**
     * Function to create a new smart contract
     * Should create and send a transaction with the following parameters:
     * Input:
     *  1 or 2 inputs from publishing address only
     *
     * Output:
     *  output 0 is 700 sats to contract address
     *  output 1 is op_return of DC{protocolVersion* as 4 byte big-endian binary}{cid of data}
     *  output 2 is optional sending funds back to publishing address only
     *  *please note the lowest protocol version is 2.  0 and 1 are not allowed as they are used to signal contract status
     *  cid should point to a file under 1 MB in size that follows one of the ContractDataFormat.v*.md formats
     *
     * @param params - string with JSON encoded data.
     * @param optionalAccount - string leave blank if not using multiple accounts mode
     * @return - returns contract address or {"error":"message"}
     */
    virtual std::string createContract(const std::string& params, std::string optionalAccount="") = 0;


    /**
     * Gets a list of all contract addresses that this plugin is handling
     * @param optionalAccount - if left blank returns all contract addresses, if not blank returns the accounts specified
     * @return
     */
    virtual std::vector<std::string> getContractAddresses(std::string optionalAccount="") = 0;


    /**
     * Provides details about what data createContract is expecting
     * @return - string with JSON encoded parameters.  Must output in one of the standard formats see ConstructorParameters.v*.md
     * If format is insufficient for your needs add needed features and submit a new version for consideration.  Please note it may take time for new formats to
     * be approved and implemented by all parties.  Until implemented new versions may not be usable by all tools.
     */
    virtual std::string getContractConstructorParameters() = 0;


    /**
     * Disables a contract
     * Should create and send a transaction with the following parameters:
     * Input:
     *  1 or 2 inputs from publishing address only
     *
     * Output:
     *  output 0 is 700 sats to contract address
     *  output 1 is op_return of DC{binary 0x00000000}
     *  output 2 is optional sending funds back to publishing address only
     *
     * @param contractAddress
     * @return - if successful at disabling.  Only reason it should fail is not enough funds in Publishing Address
     */
    virtual bool disableContract(const std::string& contractAddress) = 0;

    /**
     * Enables a contract
     * Should create and send a transaction with the following parameters:
     * Input:
     *  1 or 2 inputs from publishing address only
     *
     * Output:
     *  output 0 is 700 sats to contract address
     *  output 1 is op_return of DC{binary 0x00000001 - big endian}
     *  output 2 is optional sending funds back to publishing address only
     *
     * @param contractAddress
     * @return - if successful at disabling.  Only reason it should fail is not enough funds in Publishing Address
     */
    virtual bool enableContract(const std::string& contractAddress) = 0;
};



#endif //DIGIASSET_CORE_SMARTCONTRACTHANDLER_H
