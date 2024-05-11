//
// Created by mctrivia on 01/04/24.
//

#include "AppMain.h"
#include "utils.h"
#include "RPC/MethodList.h"
#include "gtest/gtest.h"
#include "../tests/RPCMethods.h"

using namespace std;


TEST_F(RPCMethodsTest, listlastassets) {
    ///rpc method we will be testing(if using as reference make sure you change value above and bellow this line)
    const std::string METHOD="listlastassets";
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
        EXPECT_EQ(results.size(),4634);
        EXPECT_EQ(results[0]["assetId"].asString(),"La7ARUhZUvNgviS1DrYpkS9hWhqrxGyP2jxnyv");
        EXPECT_EQ(results[0]["assetIndex"].asUInt(),4634);
        EXPECT_EQ(results[0]["cid"].asString(),"bafkreifxfrog2jjldhxujds6nxebfds2r6rbggehheskgxrpm7looxu24m");
        EXPECT_EQ(results[0]["height"].asUInt(),17451065);
        EXPECT_EQ(results[1]["assetId"].asString(),"Uh8u7WcjtGmDdZaUhSmspUw5oPkzCch1ALrJzh");
        EXPECT_EQ(results[1]["assetIndex"].asUInt(),4633);
        EXPECT_EQ(results[1]["cid"].asString(),"bafkreibwvxn2ud63o2zuijiughydmetmbk5pqtwsgdsyip3tetzcgb23vy");
        EXPECT_EQ(results[1]["height"].asUInt(),17440347);
    } catch (...) {
        EXPECT_TRUE(false);
    }

    //get latest 100 assets
    try {
        Json::Value params=Json::arrayValue;
        params.append(100);
        auto results=RPC::methods[METHOD](params).toJSON(1)["result"];
        EXPECT_TRUE(results.isArray());
        EXPECT_EQ(results.size(),100);
        EXPECT_EQ(results[0]["assetId"].asString(),"La7ARUhZUvNgviS1DrYpkS9hWhqrxGyP2jxnyv");
        EXPECT_EQ(results[0]["assetIndex"].asUInt(),4634);
        EXPECT_EQ(results[0]["cid"].asString(),"bafkreifxfrog2jjldhxujds6nxebfds2r6rbggehheskgxrpm7looxu24m");
        EXPECT_EQ(results[0]["height"].asUInt(),17451065);
        EXPECT_EQ(results[1]["assetId"].asString(),"Uh8u7WcjtGmDdZaUhSmspUw5oPkzCch1ALrJzh");
        EXPECT_EQ(results[1]["assetIndex"].asUInt(),4633);
        EXPECT_EQ(results[1]["cid"].asString(),"bafkreibwvxn2ud63o2zuijiughydmetmbk5pqtwsgdsyip3tetzcgb23vy");
        EXPECT_EQ(results[1]["height"].asUInt(),17440347);
        EXPECT_EQ(results[2]["assetId"].asString(),"Uh8u7WcjtGmDdZaUhSmspUw5oPkzCch1ALrJzh");
        EXPECT_EQ(results[2]["assetIndex"].asUInt(),4632);
        EXPECT_EQ(results[2]["cid"].asString(),"bafkreibwvxn2ud63o2zuijiughydmetmbk5pqtwsgdsyip3tetzcgb23vy");
        EXPECT_EQ(results[2]["height"].asUInt(),17434069);
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
        EXPECT_EQ(results[0]["assetId"].asString(),"Lh8iWemic4x8cMxWzLQFDq3hRrVhvXV9XkEwJ6");
        EXPECT_EQ(results[0]["assetIndex"].asUInt(),100);
        EXPECT_EQ(results[0]["cid"].asString(),"bafkreibfvquhxz4yrahs2z3dwb6kcwyp3dmmusmymg5loqxlqpb56muyou");
        EXPECT_EQ(results[0]["height"].asUInt(),9236763);
        EXPECT_EQ(results[99]["assetId"].asString(),"DigiByte");
        EXPECT_EQ(results[99]["assetIndex"].asUInt(),1);
        EXPECT_EQ(results[99]["cid"].asString(),"QmfSVLAntanDUKrEHUnTXRh53GLUBHFfxk5x6LH4zz9PM4");
        EXPECT_EQ(results[99]["height"].asUInt(),1);
    } catch (...) {
        EXPECT_TRUE(false);
    }

    //get first 5 assets including DigiByte(verbose)
    try {
        Json::Value params=Json::arrayValue;
        params.append(5);
        params.append(5);
        params.append(false);
        auto results=RPC::methods[METHOD](params).toJSON(1)["result"];
        EXPECT_TRUE(results.isArray());
        EXPECT_EQ(results.size(),5);
        EXPECT_EQ(results[4]["assetId"].asString(),"DigiByte");
        EXPECT_EQ(results[4]["assetIndex"].asUInt(),1);
        EXPECT_EQ(results[4]["cid"].asString(),"QmfSVLAntanDUKrEHUnTXRh53GLUBHFfxk5x6LH4zz9PM4");
        EXPECT_EQ(results[4]["height"].asUInt(),1);
        EXPECT_EQ(results[4]["decimals"].asUInt(),8);
        EXPECT_EQ(results[4]["ipfs"]["data"]["assetId"].asString(),"DigiByte");
        EXPECT_EQ(results[3]["assetId"].asString(),"Ua94nEKabzhJeDJtxGFXdviT185tYeHqyHKeWC");
        EXPECT_EQ(results[3]["assetIndex"].asUInt(),2);
        EXPECT_EQ(results[3]["cid"].asString(),"bafkreifkzom442xtxj2vll3pgbsnw4t4tgh5w54unh2j3kbkeal2nqp6uy");
        EXPECT_EQ(results[3]["height"].asUInt(),8432316);
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
        EXPECT_EQ(results[0]["assetId"].asString(),"La7ARUhZUvNgviS1DrYpkS9hWhqrxGyP2jxnyv");
        EXPECT_EQ(results[0]["assetIndex"].asUInt(),4634);
        EXPECT_EQ(results[0]["cid"].asString(),"bafkreifxfrog2jjldhxujds6nxebfds2r6rbggehheskgxrpm7looxu24m");
        EXPECT_EQ(results[0]["height"].asUInt(),17451065);
        EXPECT_EQ(results[1]["assetId"].asString(),"La7ge55Q7oeg9MPCf3jpnMu3e1B3nT7k6C339m");
        EXPECT_EQ(results[1]["assetIndex"].asUInt(),4631);
        EXPECT_EQ(results[1]["cid"].asString(),"bafkreicjfex2owbtgluazikaqf5oyd3nnmivfvao7xpb44bkqnfm7zwlaa");
        EXPECT_EQ(results[1]["height"].asUInt(),17432086);
        EXPECT_EQ(results[2]["assetId"].asString(),"LaAPTcBLzPh2Ji7xQaF6us9ba9dB7DyqBsSeCw");
        EXPECT_EQ(results[2]["assetIndex"].asUInt(),4630);
        EXPECT_EQ(results[2]["cid"].asString(),"bafkreigraom4n2xxj6cvqein7tgvnjctutwfpcprgm7odjwe3oyrzbo5g4");
        EXPECT_EQ(results[2]["height"].asUInt(),17425665);
    } catch (...) {
        EXPECT_TRUE(false);
    }

    //filter assets in psp only and limit to most recent 50
    try {
        Json::Value params=Json::arrayValue;
        params.append(50);
        params.append(Json::nullValue);
        params.append(true);
        Json::Value filter=Json::objectValue;
        filter["psp"]=true;
        params.append(filter);
        auto results=RPC::methods[METHOD](params).toJSON(1)["result"];
        EXPECT_TRUE(results.isArray());
        EXPECT_EQ(results.size(),50);
        EXPECT_EQ(results[0]["assetId"].asString(),"La7ARUhZUvNgviS1DrYpkS9hWhqrxGyP2jxnyv");
        EXPECT_EQ(results[0]["assetIndex"].asUInt(),4634);
        EXPECT_EQ(results[0]["cid"].asString(),"bafkreifxfrog2jjldhxujds6nxebfds2r6rbggehheskgxrpm7looxu24m");
        EXPECT_EQ(results[0]["height"].asUInt(),17451065);
        EXPECT_EQ(results[1]["assetId"].asString(),"La7ge55Q7oeg9MPCf3jpnMu3e1B3nT7k6C339m");
        EXPECT_EQ(results[1]["assetIndex"].asUInt(),4631);
        EXPECT_EQ(results[1]["cid"].asString(),"bafkreicjfex2owbtgluazikaqf5oyd3nnmivfvao7xpb44bkqnfm7zwlaa");
        EXPECT_EQ(results[1]["height"].asUInt(),17432086);
        EXPECT_EQ(results[2]["assetId"].asString(),"LaAPTcBLzPh2Ji7xQaF6us9ba9dB7DyqBsSeCw");
        EXPECT_EQ(results[2]["assetIndex"].asUInt(),4630);
        EXPECT_EQ(results[2]["cid"].asString(),"bafkreigraom4n2xxj6cvqein7tgvnjctutwfpcprgm7odjwe3oyrzbo5g4");
        EXPECT_EQ(results[2]["height"].asUInt(),17425665);
    } catch (...) {
        EXPECT_TRUE(false);
    }

    //filter assets in psp only
    try {
        Json::Value params=Json::arrayValue;
        params.append(50);
        params.append(1375);
        params.append(true);
        Json::Value filter=Json::objectValue;
        filter["psp"]=true;
        params.append(filter);
        auto results=RPC::methods[METHOD](params).toJSON(1)["result"];
        EXPECT_TRUE(results.isArray());
        EXPECT_EQ(results.size(),50);
        EXPECT_EQ(results[0]["assetId"].asString(),"La6QVkyzjWNDGjGVBfkX1QTKzazxW5nEV73kJ5");
        EXPECT_EQ(results[0]["assetIndex"].asUInt(),1375);
        EXPECT_EQ(results[0]["cid"].asString(),"bafkreibd2opifdfti2ycedvo525ay2cxdnok5cvfkkfo4yjfuauvptj2iq");
        EXPECT_EQ(results[0]["height"].asUInt(),14272692);
        EXPECT_EQ(results[1]["assetId"].asString(),"La7JTKw7tFGUazvcafAzH7k55oPjrZEPhkXKds");
        EXPECT_EQ(results[1]["assetIndex"].asUInt(),1374);
        EXPECT_EQ(results[1]["cid"].asString(),"bafkreibotss54xrdjl3xoad7nf4hz56vchifzsyhaw4mysy5nq7rtzuolu");
        EXPECT_EQ(results[1]["height"].asUInt(),14264771);
        EXPECT_EQ(results[49]["assetId"].asString(),"Uh3zdD1KtLyjinZdSBi28sjLzVXDQBv2anVnZw");
        EXPECT_EQ(results[49]["assetIndex"].asUInt(),1316);
        EXPECT_EQ(results[49]["cid"].asString(),"bafkreiaefltngem3z67lktnfkq5s3pehdfbb6eh7ppov7zddpqwfwgc6ry");
        EXPECT_EQ(results[49]["height"].asUInt(),13938282);
    } catch (...) {
        EXPECT_TRUE(false);
    }
}
