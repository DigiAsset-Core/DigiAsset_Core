//
// Created by mctrivia on 01/04/24.
//

#include "AppMain.h"
#include "utils.h"
#include "BitcoinRpcServerMethods.h"
#include "gtest/gtest.h"
#include "../tests/RPCMethods.h"

using namespace std;


TEST_F(RPCMethodsTest, getrawtransaction) {
    ///rpc method we will be testing(if using as reference make sure you change value above and bellow this line)
    const std::string METHOD="getrawtransaction";
    bool result;

    //test invalid parameters throws an exception of type DigiByteException
    //0 parameters
    try {
        Json::Value params=Json::arrayValue;
        rpcMethods[METHOD](params);
        result=false;
    } catch (const DigiByteException& e) {
        result=true;
    } catch (...) {
        result=false;
    }
    EXPECT_TRUE(result);

    //test invalid parameters throws an exception of type DigiByteException
    //4 parameters
    try {
        Json::Value params=Json::arrayValue;
        params.append("ded11eb298b73da7a9f0da750840537285495ee6347cfd7ad278c6a2defacf0e");
        params.append(true);
        params.append(true);
        params.append(true);
        rpcMethods[METHOD](params);
        result=false;
    } catch (const DigiByteException& e) {
        result=true;
    } catch (...) {
        result=false;
    }
    EXPECT_TRUE(result);

    //test invalid parameters throws an exception of type DigiByteException
    //try to get a block not a transaction
    try {
        Json::Value params=Json::arrayValue;
        params.append("7c475da839f1a795d474f43480a6c101dbbfd3c4c4fda8dbfe96a36224fa5956");
        rpcMethods[METHOD](params);
        result=false;
    } catch (const DigiByteException& e) {
        result=true;
    } catch (...) {
        result=false;
    }
    EXPECT_TRUE(result);

    //test invalid parameters throws an exception of type DigiByteException
    //wrong length of txid
    try {
        Json::Value params=Json::arrayValue;
        params.append("test");
        rpcMethods[METHOD](params);
        result=false;
    } catch (const DigiByteException& e) {
        result=true;
    } catch (...) {
        result=false;
    }
    EXPECT_TRUE(result);

    //test getting non verbose output with a transaction not in database
    try {
        Json::Value params=Json::arrayValue;
        params.append("63beff1347087215be8992caf4f79448291c656235618c95c8bee6cc7d4cb4d3");
        auto results=rpcMethods[METHOD](params);
        EXPECT_EQ(results.asString(),"010000000101ae21be5438a995991f8e958bb3917d8d3a80f3229583819f932ca0b541da2f000000006a473044022007d03dbb3acadb18bc3a2663e9db005e69a530a329482736c3f26fd2958f5930022051f944149291bbdf10805ecd81253f3876e4938a6cffe8de6e1ba143d1cce5c4012102efe415534320dcb2bb139da7a7e52aeab0d145a2ae3caea4906a5ddf997cbba5feffffff0280c3c901000000001976a9147d2243ca8cf8c4bd1ee49aa4096d4e62ecfc6c3388ac14752a9a250000001976a9144720d96cae6ea1be8978d17abc54938f6225aef388ac9d23f400");
    } catch (...) {
        EXPECT_TRUE(false);
    }

    //test getting verbose output with a transaction not in database
    try {
        Json::Value params=Json::arrayValue;
        params.append("63beff1347087215be8992caf4f79448291c656235618c95c8bee6cc7d4cb4d3");
        params.append(true);
        auto results=rpcMethods[METHOD](params);
        EXPECT_TRUE(results.isObject());
        EXPECT_EQ(results["blockhash"].asString(),"5c0c7d7baecc5e13c69f06eb90534ab2db8ff0cc98a1ac8d8797f11dc16ea0ad");
        EXPECT_EQ(results["blocktime"].asUInt(),1666719839);
        EXPECT_TRUE(results["confirmations"].isUInt());;
        EXPECT_EQ(results["hash"].asString(),"63beff1347087215be8992caf4f79448291c656235618c95c8bee6cc7d4cb4d3");
        EXPECT_EQ(results["height"].asUInt(),16000000);
        EXPECT_EQ(results["hex"].asString(),"010000000101ae21be5438a995991f8e958bb3917d8d3a80f3229583819f932ca0b541da2f000000006a473044022007d03dbb3acadb18bc3a2663e9db005e69a530a329482736c3f26fd2958f5930022051f944149291bbdf10805ecd81253f3876e4938a6cffe8de6e1ba143d1cce5c4012102efe415534320dcb2bb139da7a7e52aeab0d145a2ae3caea4906a5ddf997cbba5feffffff0280c3c901000000001976a9147d2243ca8cf8c4bd1ee49aa4096d4e62ecfc6c3388ac14752a9a250000001976a9144720d96cae6ea1be8978d17abc54938f6225aef388ac9d23f400");
        EXPECT_EQ(results["locktime"].asUInt(),15999901);
        EXPECT_EQ(results["size"].asUInt(),225);
        EXPECT_EQ(results["time"].asUInt(),1666719839);
        EXPECT_EQ(results["txid"].asString(),"63beff1347087215be8992caf4f79448291c656235618c95c8bee6cc7d4cb4d3");
        EXPECT_EQ(results["version"].asUInt(),1);
        EXPECT_EQ(results["vin"].size(),1);
        EXPECT_EQ(results["vin"][0]["address"].asString(),"D7fkDoYQBCnvXYqzaFTowqrsCCaHy69wi4");
        EXPECT_TRUE(results["vin"][0]["assets"].isArray());
        EXPECT_TRUE(results["vin"][0]["assets"].empty());
        EXPECT_EQ(results["vin"][0]["scriptSig"]["hex"].asString(),"473044022007d03dbb3acadb18bc3a2663e9db005e69a530a329482736c3f26fd2958f5930022051f944149291bbdf10805ecd81253f3876e4938a6cffe8de6e1ba143d1cce5c4012102efe415534320dcb2bb139da7a7e52aeab0d145a2ae3caea4906a5ddf997cbba5");
        EXPECT_EQ(results["vin"][0]["sequence"].asUInt(),4294967294);
        EXPECT_EQ(results["vin"][0]["txid"].asString(),"2fda41b5a02c939f81839522f3803a8d7d91b38b958e1f9995a93854be21ae01");
        EXPECT_EQ(results["vin"][0]["vout"].asUInt(),0);
        EXPECT_EQ(results["vin"][0]["valueS"].asUInt64(),161530286200);
        EXPECT_EQ(results["vout"].size(),2);
        EXPECT_EQ(results["vout"][0]["address"].asString(),"DGYk3zqKSRwhisadwxqp6ncMkKBuYYKrRB");
        EXPECT_TRUE(results["vout"][0]["assets"].isArray());
        EXPECT_TRUE(results["vout"][0]["assets"].empty());
        EXPECT_EQ(results["vout"][0]["n"].asUInt(),0);
        EXPECT_EQ(results["vout"][0]["scriptPubKey"]["addresses"].size(),1);
        EXPECT_EQ(results["vout"][0]["scriptPubKey"]["addresses"][0].asString(),"DGYk3zqKSRwhisadwxqp6ncMkKBuYYKrRB");
        EXPECT_EQ(results["vout"][0]["scriptPubKey"]["asm"].asString(),"OP_DUP OP_HASH160 7d2243ca8cf8c4bd1ee49aa4096d4e62ecfc6c33 OP_EQUALVERIFY OP_CHECKSIG");
        EXPECT_EQ(results["vout"][0]["scriptPubKey"]["hex"].asString(),"76a9147d2243ca8cf8c4bd1ee49aa4096d4e62ecfc6c3388ac");
        EXPECT_EQ(results["vout"][0]["scriptPubKey"]["reqSigs"].asUInt(),1);
        EXPECT_EQ(results["vout"][0]["scriptPubKey"]["type"].asString(),"pubkeyhash");
        EXPECT_EQ(results["vout"][0]["valueS"].asUInt64(),30000000);
        EXPECT_EQ(results["vout"][1]["address"].asString(),"DBdBsHhkCYN7AmCtbeMAaTbdstWZxw1Bqg");
        EXPECT_TRUE(results["vout"][1]["assets"].isArray());
        EXPECT_TRUE(results["vout"][1]["assets"].empty());
        EXPECT_EQ(results["vout"][1]["n"].asUInt(),1);
        EXPECT_EQ(results["vout"][1]["scriptPubKey"]["addresses"].size(),1);
        EXPECT_EQ(results["vout"][1]["scriptPubKey"]["addresses"][0].asString(),"DBdBsHhkCYN7AmCtbeMAaTbdstWZxw1Bqg");
        EXPECT_EQ(results["vout"][1]["scriptPubKey"]["asm"].asString(),"OP_DUP OP_HASH160 4720d96cae6ea1be8978d17abc54938f6225aef3 OP_EQUALVERIFY OP_CHECKSIG");
        EXPECT_EQ(results["vout"][1]["scriptPubKey"]["hex"].asString(),"76a9144720d96cae6ea1be8978d17abc54938f6225aef388ac");
        EXPECT_EQ(results["vout"][1]["scriptPubKey"]["reqSigs"].asUInt(),1);
        EXPECT_EQ(results["vout"][1]["scriptPubKey"]["type"].asString(),"pubkeyhash");
        EXPECT_EQ(results["vout"][1]["valueS"].asUInt64(),161500263700);
        EXPECT_EQ(results["vsize"].asUInt(),225);
        EXPECT_EQ(results["weight"].asUInt(),900);
    } catch (...) {
        EXPECT_TRUE(false);
    }

    //test getting verbose output with a transaction that has assets
    try {
        Json::Value params=Json::arrayValue;
        params.append("d254074d0e558e4ff027b576d7fc7074ab5e7ef092c29cb665c4d45712f16b4b");
        params.append(true);
        auto results=rpcMethods[METHOD](params);
        EXPECT_TRUE(results.isObject());
        EXPECT_EQ(results["blockhash"].asString(),"000000000000000084ac88668e938437c91d3eef206bfc995bdc48a689607950");
        EXPECT_EQ(results["blocktime"].asUInt(),1650864019);
        EXPECT_TRUE(results["confirmations"].isUInt());;
        EXPECT_EQ(results["hash"].asString(),"d254074d0e558e4ff027b576d7fc7074ab5e7ef092c29cb665c4d45712f16b4b");
        EXPECT_EQ(results["height"].asUInt(),14939182);
        EXPECT_EQ(results["hex"].asString(),"0200000002da7b29a249cd7f633702cf27976ca10c7bfae770127a14e068ffc5cf9d616f57000000006b483045022100d299cb66884c46895b9e3eacefdc900793d80b5661de7d8c47f86e4d7257cc6c02200f0490ac879d22bd714e9adf731db1d0c33c9df86fc82bf840ad791138eacd94012103b9145dbe7efa83b380b39b54017253bd4ec33fc4969096caf27f4705f730f48fffffffffda7b29a249cd7f633702cf27976ca10c7bfae770127a14e068ffc5cf9d616f57030000006a47304402201d82725db84b0383725f5e58792bd2cf9d456b089814431e3442fc372895b6aa02203dba1719a52be34539364b302a27f6a5555a054e709b67451f842bedb8ea2c88012103b9145dbe7efa83b380b39b54017253bd4ec33fc4969096caf27f4705f730f48fffffffff0458020000000000001976a914ffdaae7f69029fe7a7ab2d8656fd587da443c88e88ac580200000000000016001468afdbd9081d4519bd56b0fbf73e92e7e50755a300000000000000000c6a0a44410315004204d00101389ecb1d000000001976a914ffdaae7f69029fe7a7ab2d8656fd587da443c88e88ac00000000");
        EXPECT_EQ(results["locktime"].asUInt(),0);
        EXPECT_EQ(results["size"].asUInt(),425);
        EXPECT_EQ(results["time"].asUInt(),1650864019);
        EXPECT_EQ(results["txid"].asString(),"d254074d0e558e4ff027b576d7fc7074ab5e7ef092c29cb665c4d45712f16b4b");
        EXPECT_EQ(results["version"].asUInt(),2);
        EXPECT_EQ(results["vin"].size(),2);
        EXPECT_EQ(results["vin"][0]["address"].asString(),"DUTvpSYuT1YumocWCG6UD2dq2V4jasSy4S");
        EXPECT_TRUE(results["vin"][0]["assets"].isArray());
        EXPECT_EQ(results["vin"][0]["assets"][0]["assetId"].asString(),"LaABthaRA9RvcTZPgrvrepP1Q5gxoe8uajptec");
        EXPECT_EQ(results["vin"][0]["assets"][0]["assetIndex"].asUInt(),1401);
        EXPECT_EQ(results["vin"][0]["assets"][0]["cid"].asString(),"bafkreifzwbp76esd56srq3vjhx4kfkakql3d75dfk4ffknyqgagutlvu3q");
        EXPECT_EQ(results["vin"][0]["assets"][0]["count"].asUInt64(),8270);
        EXPECT_EQ(results["vin"][0]["assets"][0]["decimals"].asUInt(),0);
        EXPECT_EQ(results["vin"][0]["scriptSig"]["hex"].asString(),"483045022100d299cb66884c46895b9e3eacefdc900793d80b5661de7d8c47f86e4d7257cc6c02200f0490ac879d22bd714e9adf731db1d0c33c9df86fc82bf840ad791138eacd94012103b9145dbe7efa83b380b39b54017253bd4ec33fc4969096caf27f4705f730f48f");
        EXPECT_EQ(results["vin"][0]["sequence"].asUInt(),4294967295);
        EXPECT_EQ(results["vin"][0]["txid"].asString(),"576f619dcfc5ff68e0147a1270e7fa7b0ca16c9727cf0237637fcd49a2297bda");
        EXPECT_EQ(results["vin"][0]["vout"].asUInt(),0);
        EXPECT_EQ(results["vin"][0]["valueS"].asUInt64(),600);
        EXPECT_EQ(results["vin"][1]["address"].asString(),"DUTvpSYuT1YumocWCG6UD2dq2V4jasSy4S");
        EXPECT_TRUE(results["vin"][1]["assets"].isArray());
        EXPECT_TRUE(results["vin"][1]["assets"].empty());
        EXPECT_EQ(results["vin"][1]["scriptSig"]["hex"].asString(),"47304402201d82725db84b0383725f5e58792bd2cf9d456b089814431e3442fc372895b6aa02203dba1719a52be34539364b302a27f6a5555a054e709b67451f842bedb8ea2c88012103b9145dbe7efa83b380b39b54017253bd4ec33fc4969096caf27f4705f730f48f");
        EXPECT_EQ(results["vin"][1]["sequence"].asUInt(),4294967295);
        EXPECT_EQ(results["vin"][1]["txid"].asString(),"576f619dcfc5ff68e0147a1270e7fa7b0ca16c9727cf0237637fcd49a2297bda");
        EXPECT_EQ(results["vin"][1]["vout"].asUInt(),3);
        EXPECT_EQ(results["vin"][1]["valueS"].asUInt64(),499884764);
        EXPECT_EQ(results["vout"].size(),4);
        EXPECT_EQ(results["vout"][0]["address"].asString(),"DUTvpSYuT1YumocWCG6UD2dq2V4jasSy4S");
        EXPECT_TRUE(results["vout"][0]["assets"].isArray());
        EXPECT_EQ(results["vout"][0]["assets"].size(),1);
        EXPECT_EQ(results["vout"][0]["assets"][0]["assetId"].asString(),"LaABthaRA9RvcTZPgrvrepP1Q5gxoe8uajptec");
        EXPECT_EQ(results["vout"][0]["assets"][0]["assetIndex"].asUInt(),1401);
        EXPECT_EQ(results["vout"][0]["assets"][0]["cid"].asString(),"bafkreifzwbp76esd56srq3vjhx4kfkakql3d75dfk4ffknyqgagutlvu3q");
        EXPECT_EQ(results["vout"][0]["assets"][0]["count"].asUInt64(),8269);
        EXPECT_EQ(results["vout"][0]["assets"][0]["decimals"].asUInt(),0);
        EXPECT_EQ(results["vout"][0]["n"].asUInt(),0);
        EXPECT_EQ(results["vout"][0]["scriptPubKey"]["addresses"].size(),1);
        EXPECT_EQ(results["vout"][0]["scriptPubKey"]["addresses"][0].asString(),"DUTvpSYuT1YumocWCG6UD2dq2V4jasSy4S");
        EXPECT_EQ(results["vout"][0]["scriptPubKey"]["asm"].asString(),"OP_DUP OP_HASH160 ffdaae7f69029fe7a7ab2d8656fd587da443c88e OP_EQUALVERIFY OP_CHECKSIG");
        EXPECT_EQ(results["vout"][0]["scriptPubKey"]["hex"].asString(),"76a914ffdaae7f69029fe7a7ab2d8656fd587da443c88e88ac");
        EXPECT_EQ(results["vout"][0]["scriptPubKey"]["reqSigs"].asUInt(),1);
        EXPECT_EQ(results["vout"][0]["scriptPubKey"]["type"].asString(),"pubkeyhash");
        EXPECT_EQ(results["vout"][0]["valueS"].asUInt64(),600);
        EXPECT_EQ(results["vout"][1]["address"].asString(),"dgb1qdzhahkggr4z3n02kkralw05juljsw4drjw7uu4");
        EXPECT_TRUE(results["vout"][1]["assets"].isArray());
        EXPECT_EQ(results["vout"][1]["assets"].size(),1);
        EXPECT_EQ(results["vout"][1]["assets"][0]["assetId"].asString(),"LaABthaRA9RvcTZPgrvrepP1Q5gxoe8uajptec");
        EXPECT_EQ(results["vout"][1]["assets"][0]["assetIndex"].asUInt(),1401);
        EXPECT_EQ(results["vout"][1]["assets"][0]["cid"].asString(),"bafkreifzwbp76esd56srq3vjhx4kfkakql3d75dfk4ffknyqgagutlvu3q");
        EXPECT_EQ(results["vout"][1]["assets"][0]["count"].asUInt64(),1);
        EXPECT_EQ(results["vout"][1]["assets"][0]["decimals"].asUInt(),0);
        EXPECT_EQ(results["vout"][1]["n"].asUInt(),1);
        EXPECT_EQ(results["vout"][1]["scriptPubKey"]["addresses"].size(),1);
        EXPECT_EQ(results["vout"][1]["scriptPubKey"]["addresses"][0].asString(),"dgb1qdzhahkggr4z3n02kkralw05juljsw4drjw7uu4");
        EXPECT_EQ(results["vout"][1]["scriptPubKey"]["asm"].asString(),"0 68afdbd9081d4519bd56b0fbf73e92e7e50755a3");
        EXPECT_EQ(results["vout"][1]["scriptPubKey"]["hex"].asString(),"001468afdbd9081d4519bd56b0fbf73e92e7e50755a3");
        EXPECT_EQ(results["vout"][1]["scriptPubKey"]["reqSigs"].asUInt(),1);
        EXPECT_EQ(results["vout"][1]["scriptPubKey"]["type"].asString(),"witness_v0_keyhash");
        EXPECT_EQ(results["vout"][1]["valueS"].asUInt64(),600);
        EXPECT_EQ(results["vout"][2]["address"].asString(),"");
        EXPECT_TRUE(results["vout"][2]["assets"].isArray());
        EXPECT_TRUE(results["vout"][2]["assets"].empty());
        EXPECT_EQ(results["vout"][2]["n"].asUInt(),2);
        EXPECT_EQ(results["vout"][2]["scriptPubKey"]["asm"].asString(),"OP_RETURN 44410315004204d00101");
        EXPECT_EQ(results["vout"][2]["scriptPubKey"]["hex"].asString(),"6a0a44410315004204d00101");
        EXPECT_EQ(results["vout"][2]["scriptPubKey"]["type"].asString(),"nulldata");
        EXPECT_EQ(results["vout"][2]["valueS"].asUInt64(),0);
        EXPECT_EQ(results["vout"][3]["address"].asString(),"DUTvpSYuT1YumocWCG6UD2dq2V4jasSy4S");
        EXPECT_TRUE(results["vout"][3]["assets"].isArray());
        EXPECT_TRUE(results["vout"][3]["assets"].empty());
        EXPECT_EQ(results["vout"][3]["n"].asUInt(),3);
        EXPECT_EQ(results["vout"][3]["scriptPubKey"]["addresses"].size(),1);
        EXPECT_EQ(results["vout"][3]["scriptPubKey"]["addresses"][0].asString(),"DUTvpSYuT1YumocWCG6UD2dq2V4jasSy4S");
        EXPECT_EQ(results["vout"][3]["scriptPubKey"]["asm"].asString(),"OP_DUP OP_HASH160 ffdaae7f69029fe7a7ab2d8656fd587da443c88e OP_EQUALVERIFY OP_CHECKSIG");
        EXPECT_EQ(results["vout"][3]["scriptPubKey"]["hex"].asString(),"76a914ffdaae7f69029fe7a7ab2d8656fd587da443c88e88ac");
        EXPECT_EQ(results["vout"][3]["scriptPubKey"]["reqSigs"].asUInt(),1);
        EXPECT_EQ(results["vout"][3]["scriptPubKey"]["type"].asString(),"pubkeyhash");
        EXPECT_EQ(results["vout"][3]["valueS"].asUInt64(),499883576);
        EXPECT_EQ(results["vsize"].asUInt(),425);
        EXPECT_EQ(results["weight"].asUInt(),1700);
    } catch (...) {
        EXPECT_TRUE(false);
    }
}
