//
// Created by mctrivia on 07/04/24.
//

#include "AppMain.h"
#include "utils.h"
#include "RPC/MethodList.h"
#include "gtest/gtest.h"
#include "../tests/RPCMethods.h"

using namespace std;


TEST_F(RPCMethodsTest, listlastassetspageindexes) {
    ///rpc method we will be testing(if using as reference make sure you change value above and bellow this line)
    const std::string METHOD="listlastassetspageindexes";
    bool result;

    //test invalid parameters throws an exception of type DigiByteException
    //0 parameters
    try {
        Json::Value params=Json::arrayValue;
        RPC::methods[METHOD](params);
        result=false;
    } catch (const DigiByteException& e) {
        result=true;
    } catch (...) {
        result=false;
    }
    EXPECT_TRUE(result);

    //test invalid parameters throws an exception of type DigiByteException
    //3 parameters
    try {
        Json::Value params=Json::arrayValue;
        params.append(5);
        params.append(1);
        params.append(true);
        RPC::methods[METHOD](params);
        result=false;
    } catch (const DigiByteException& e) {
        result=true;
    } catch (...) {
        result=false;
    }
    EXPECT_TRUE(result);

    //test invalid parameters throws an exception of type DigiByteException
    //wrong parameter types
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
    //wrong parameter types
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

    //Pages of 100
    try {
        Json::Value params=Json::arrayValue;
        params.append(100);
        auto results=RPC::methods[METHOD](params).toJSON(1)["result"];
        EXPECT_TRUE(results.isArray());
        EXPECT_EQ(results.size(),47);
        unsigned int i=0;
        for (int assetIndex=4634;assetIndex>0;assetIndex-=100) {
            EXPECT_TRUE(results[i]["skips"].isArray());
            EXPECT_TRUE(results[i]["skips"].empty());
            EXPECT_EQ(results[i]["start"].asUInt(), assetIndex);
            i++;
        }
    } catch (...) {
        EXPECT_TRUE(false);
    }

    //Pages of 100
    try {
        Json::Value params=Json::arrayValue;
        params.append(100);
        Json::Value filter=Json::objectValue;
        filter["psp"]=true;
        params.append(filter);
        auto results=RPC::methods[METHOD](params).toJSON(1)["result"];
        EXPECT_TRUE(results.isArray());
        EXPECT_EQ(results.size(),33);
        EXPECT_EQ(results[0]["start"].asUInt(),4634);
        EXPECT_EQ(results[0]["skips"].size(),81);
        EXPECT_EQ(results[0]["skips"][0].asUInt(),4633);
        EXPECT_EQ(results[0]["skips"][1].asUInt(),4632);
        EXPECT_EQ(results[0]["skips"][2].asUInt(),4629);
        EXPECT_EQ(results[0]["skips"][3].asUInt(),4624);
        EXPECT_EQ(results[0]["skips"][4].asUInt(),4622);
        EXPECT_EQ(results[0]["skips"][5].asUInt(),4620);
        EXPECT_EQ(results[0]["skips"][6].asUInt(),4619);
        EXPECT_EQ(results[0]["skips"][7].asUInt(),4618);
        EXPECT_EQ(results[0]["skips"][8].asUInt(),4617);
        EXPECT_EQ(results[0]["skips"][9].asUInt(),4616);
        EXPECT_EQ(results[0]["skips"][10].asUInt(),4608);
        EXPECT_EQ(results[0]["skips"][11].asUInt(),4607);
        EXPECT_EQ(results[0]["skips"][12].asUInt(),4606);
        EXPECT_EQ(results[0]["skips"][13].asUInt(),4604);
        EXPECT_EQ(results[0]["skips"][14].asUInt(),4603);
        EXPECT_EQ(results[0]["skips"][15].asUInt(),4602);
        EXPECT_EQ(results[0]["skips"][16].asUInt(),4601);
        EXPECT_EQ(results[0]["skips"][17].asUInt(),4600);
        EXPECT_EQ(results[1]["start"].asUInt(),4453);
        EXPECT_EQ(results[1]["skips"].size(),0);
        EXPECT_EQ(results[2]["start"].asUInt(),4353);
        EXPECT_EQ(results[2]["skips"].size(),7);
        EXPECT_EQ(results[3]["start"].asUInt(),4246);
        EXPECT_EQ(results[3]["skips"].size(),0);
        EXPECT_EQ(results[4]["start"].asUInt(),4146);
        EXPECT_EQ(results[4]["skips"].size(),6);
        EXPECT_EQ(results[5]["start"].asUInt(),4040);
        EXPECT_EQ(results[5]["skips"].size(),0);
        EXPECT_EQ(results[6]["start"].asUInt(),3940);
        EXPECT_EQ(results[6]["skips"].size(),0);
        EXPECT_EQ(results[7]["start"].asUInt(),3840);
        EXPECT_EQ(results[7]["skips"].size(),0);
        EXPECT_EQ(results[8]["start"].asUInt(),3740);
        EXPECT_EQ(results[8]["skips"].size(),0);
        EXPECT_EQ(results[9]["start"].asUInt(),3640);
        EXPECT_EQ(results[9]["skips"].size(),0);
    } catch (...) {
        EXPECT_TRUE(false);
    }
}
