//
// Created by mctrivia on 01/04/24.
//

#include "AppMain.h"
#include "utils.h"
#include "RPC/MethodList.h"
#include "gtest/gtest.h"
#include "../tests/RPCMethods.h"

using namespace std;


TEST_F(RPCMethodsTest, listassets) {
    ///rpc method we will be testing(if using as reference make sure you change value above and bellow this line)
    const std::string METHOD="listassets";
    bool result;

    //test invalid parameters throws an exception of type DigiByteException
    //5 parameters
    try {
        Json::Value params=Json::arrayValue;
        params.append(5);
        params.append(1);
        params.append(true);
        params.append(Json::objectValue);
        params.append(50);
        RPC::methods[METHOD](params);
        result=false;
    } catch (const DigiByteException& e) {
        result=true;
    } catch (...) {
        result=false;
    }
    EXPECT_TRUE(result);

    //test invalid parameters throws an exception of type DigiByteException
    //wrong type
    try {
        Json::Value params=Json::arrayValue;
        params.append("bad");
        RPC::methods[METHOD](params);
        result=false;
    } catch (const DigiByteException& e) {
        result=true;
    } catch (...) {
        result=false;
    }
    EXPECT_TRUE(result);

    //test invalid parameters throws an exception of type DigiByteException
    //wrong type
    try {
        Json::Value params=Json::arrayValue;
        params.append(100);
        params.append("bad");
        RPC::methods[METHOD](params);
        result=false;
    } catch (const DigiByteException& e) {
        result=true;
    } catch (...) {
        result=false;
    }
    EXPECT_TRUE(result);

    //test invalid parameters throws an exception of type DigiByteException
    //wrong type
    try {
        Json::Value params=Json::arrayValue;
        params.append(100);
        params.append(5);
        params.append("bad");
        RPC::methods[METHOD](params);
        result=false;
    } catch (const DigiByteException& e) {
        result=true;
    } catch (...) {
        result=false;
    }
    EXPECT_TRUE(result);

    //test invalid parameters throws an exception of type DigiByteException
    //wrong type
    try {
        Json::Value params=Json::arrayValue;
        params.append(100);
        params.append(5);
        params.append(true);
        params.append("bad");
        RPC::methods[METHOD](params);
        result=false;
    } catch (const DigiByteException& e) {
        result=true;
    } catch (...) {
        result=false;
    }
    EXPECT_TRUE(result);

    //default value
    try {
        Json::Value params=Json::arrayValue;
        auto results=RPC::methods[METHOD](params).toJSON(1)["result"];
        EXPECT_TRUE(results.isArray());
        EXPECT_EQ(results.size(),4633);
        EXPECT_EQ(results[0]["assetId"].asString(),"Ua94nEKabzhJeDJtxGFXdviT185tYeHqyHKeWC");
        EXPECT_EQ(results[0]["assetIndex"].asUInt(),2);
        EXPECT_EQ(results[0]["cid"].asString(),"bafkreifkzom442xtxj2vll3pgbsnw4t4tgh5w54unh2j3kbkeal2nqp6uy");
        EXPECT_EQ(results[0]["height"].asUInt(),8432316);
        EXPECT_EQ(results[1]["assetId"].asString(),"LaAKU55bbtgL9RSpNvuMrXhhASHgepmKbd247S");
        EXPECT_EQ(results[1]["assetIndex"].asUInt(),3);
        EXPECT_EQ(results[1]["cid"].asString(),"bafkreic4etklteiiikqrk4yfpofhv2vj6n3m2nqyucwpp2hbupwm3tioxy");
        EXPECT_EQ(results[1]["height"].asUInt(),8432958);
    } catch (...) {
        EXPECT_TRUE(false);
    }

    //get first 100 assets including DigiByte
    try {
        Json::Value params=Json::arrayValue;
        params.append(100);
        params.append(0);
        auto results=RPC::methods[METHOD](params).toJSON(1)["result"];
        EXPECT_TRUE(results.isArray());
        EXPECT_EQ(results.size(),100);
        EXPECT_EQ(results[0]["assetId"].asString(),"DigiByte");
        EXPECT_EQ(results[0]["assetIndex"].asUInt(),1);
        EXPECT_EQ(results[0]["cid"].asString(),"QmfSVLAntanDUKrEHUnTXRh53GLUBHFfxk5x6LH4zz9PM4");
        EXPECT_EQ(results[0]["height"].asUInt(),1);
        EXPECT_EQ(results[1]["assetId"].asString(),"Ua94nEKabzhJeDJtxGFXdviT185tYeHqyHKeWC");
        EXPECT_EQ(results[1]["assetIndex"].asUInt(),2);
        EXPECT_EQ(results[1]["cid"].asString(),"bafkreifkzom442xtxj2vll3pgbsnw4t4tgh5w54unh2j3kbkeal2nqp6uy");
        EXPECT_EQ(results[1]["height"].asUInt(),8432316);
        EXPECT_EQ(results[2]["assetId"].asString(),"LaAKU55bbtgL9RSpNvuMrXhhASHgepmKbd247S");
        EXPECT_EQ(results[2]["assetIndex"].asUInt(),3);
        EXPECT_EQ(results[2]["cid"].asString(),"bafkreic4etklteiiikqrk4yfpofhv2vj6n3m2nqyucwpp2hbupwm3tioxy");
        EXPECT_EQ(results[2]["height"].asUInt(),8432958);
    } catch (...) {
        EXPECT_TRUE(false);
    }

    //get second 100 assets including DigiByte
    try {
        Json::Value params=Json::arrayValue;
        params.append(100);
        params.append(100);
        auto results=RPC::methods[METHOD](params).toJSON(1)["result"];
        EXPECT_TRUE(results.isArray());
        EXPECT_EQ(results.size(),100);
        EXPECT_EQ(results[0]["assetId"].asString(),"Lh5YyPKjEUPqqoLkL7REHfp5JhNaJ32EkFiNmW");
        EXPECT_EQ(results[0]["assetIndex"].asUInt(),101);
        EXPECT_EQ(results[0]["cid"].asString(),"bafkreics3mg2nspzs22eyjnigkfj2duns5ebcf5j63leimj3u6h3lwl4vy");
        EXPECT_EQ(results[0]["height"].asUInt(),9236888);
        EXPECT_EQ(results[1]["assetId"].asString(),"Lh6GkJqXNKdUtDEwVAHNYe8meSfdi3EvbHphE5");
        EXPECT_EQ(results[1]["assetIndex"].asUInt(),102);
        EXPECT_EQ(results[1]["cid"].asString(),"bafkreihy2gtratv66vc4u7gp5hut2fbmqqdbjuzbqwuuoaltfxgjlgycjy");
        EXPECT_EQ(results[1]["height"].asUInt(),9237303);
    } catch (...) {
        EXPECT_TRUE(false);
    }

    //get first 5 assets including DigiByte(verbose)
    try {
        Json::Value params=Json::arrayValue;
        params.append(5);
        params.append(0);
        params.append(false);
        auto results=RPC::methods[METHOD](params).toJSON(1)["result"];
        EXPECT_TRUE(results.isArray());
        EXPECT_EQ(results.size(),5);
        EXPECT_EQ(results[0]["assetId"].asString(),"DigiByte");
        EXPECT_EQ(results[0]["assetIndex"].asUInt(),1);
        EXPECT_EQ(results[0]["cid"].asString(),"QmfSVLAntanDUKrEHUnTXRh53GLUBHFfxk5x6LH4zz9PM4");
        EXPECT_EQ(results[0]["height"].asUInt(),1);
        EXPECT_EQ(results[0]["decimals"].asUInt(),8);
        EXPECT_EQ(results[0]["ipfs"]["data"]["assetId"].asString(),"DigiByte");
        EXPECT_EQ(results[1]["assetId"].asString(),"Ua94nEKabzhJeDJtxGFXdviT185tYeHqyHKeWC");
        EXPECT_EQ(results[1]["assetIndex"].asUInt(),2);
        EXPECT_EQ(results[1]["cid"].asString(),"bafkreifkzom442xtxj2vll3pgbsnw4t4tgh5w54unh2j3kbkeal2nqp6uy");
        EXPECT_EQ(results[1]["height"].asUInt(),8432316);
        EXPECT_EQ(results[2]["assetId"].asString(),"LaAKU55bbtgL9RSpNvuMrXhhASHgepmKbd247S");
        EXPECT_EQ(results[2]["assetIndex"].asUInt(),3);
        EXPECT_EQ(results[2]["cid"].asString(),"bafkreic4etklteiiikqrk4yfpofhv2vj6n3m2nqyucwpp2hbupwm3tioxy");
        EXPECT_EQ(results[2]["height"].asUInt(),8432958);
    } catch (...) {
        EXPECT_TRUE(false);
    }

    //filter assets in psp only
    try {
        Json::Value params=Json::arrayValue;
        params.append(Json::nullValue);
        params.append(Json::nullValue);
        params.append(true);
        Json::Value filter=Json::objectValue;
        filter["psp"]=true;
        params.append(filter);
        auto results=RPC::methods[METHOD](params).toJSON(1)["result"];
        EXPECT_TRUE(results.isArray());
        EXPECT_EQ(results.size(),3209);
        EXPECT_EQ(results[0]["assetId"].asString(),"La9oiVh9bLhXtxB2GtHQCwtztoSR8EDJgFPUEP");
        EXPECT_EQ(results[0]["assetIndex"].asUInt(),1177);
        EXPECT_EQ(results[0]["cid"].asString(),"bafkreiaei3qo4wbilonq6rnqrmtt23gxtpu4xwvgcs2gockbl7ujraokce");
        EXPECT_EQ(results[0]["height"].asUInt(),12875575);
        EXPECT_EQ(results[1]["assetId"].asString(),"Ua3AKnsS7cmEF3prst8RTUXDuDhGKHacgHsDLv");
        EXPECT_EQ(results[1]["assetIndex"].asUInt(),1265);
        EXPECT_EQ(results[1]["cid"].asString(),"bafkreigcvf5soslzgik3kkzvx7xm6wzgmx4i3fgiyzk5ppteypj2k663qy");
        EXPECT_EQ(results[1]["height"].asUInt(),13387627);
        EXPECT_EQ(results[2]["assetId"].asString(),"Ua5YbTJs8Sy4trVvs554sTHb3hz1ZgHTWVLcHJ");
        EXPECT_EQ(results[2]["assetIndex"].asUInt(),1271);
        EXPECT_EQ(results[2]["cid"].asString(),"bafkreigg2neo7wzxxf6zfer7x4pzfvxsuzctatp7euwn2vsawja6b3n2iy");
        EXPECT_EQ(results[2]["height"].asUInt(),13433787);
    } catch (...) {
        EXPECT_TRUE(false);
    }
}
