//
// Created by mctrivia on 01/04/24.
//

#include "AppMain.h"
#include "utils.h"
#include "BitcoinRpcServerMethods.h"
#include "gtest/gtest.h"
#include "../tests/RPCMethods.h"

using namespace std;


TEST_F(RPCMethodsTest, getexchangerates) {
    ///rpc method we will be testing(if using as reference make sure you change value above and bellow this line)
    const std::string METHOD="getexchangerates";
    bool result;

    //test invalid parameters throws an exception of type DigiByteException
    //2 parameters
    try {
        Json::Value params=Json::arrayValue;
        params.append(15000000);
        params.append(0);
        rpcMethods[METHOD](params);
        result=false;
    } catch (const DigiByteException& e) {
        result=true;
    } catch (...) {
        result=false;
    }
    EXPECT_TRUE(result);

    //test invalid parameters throws an exception of type DigiByteException
    //1 parameter but wrong type
    try {
        Json::Value params=Json::arrayValue;
        params.append("bad");
        rpcMethods[METHOD](params);
        result=false;
    } catch (const DigiByteException& e) {
        result=true;
    } catch (...) {
        result=false;
    }
    EXPECT_TRUE(result);

    //test getting rates before any rates where published
    try {
        Json::Value params=Json::arrayValue;
        params.append(5000);
        auto results=rpcMethods[METHOD](params);
        EXPECT_TRUE(results.isArray());
        EXPECT_TRUE(results.empty());
    } catch (...) {
        result=false;
    }
    EXPECT_TRUE(result);

    //test getting rates at a specific height that just happens to be height of publishing
    try {
        Json::Value params=Json::arrayValue;
        params.append(13774816);
        auto results=rpcMethods[METHOD](params);
        EXPECT_TRUE(results.isArray());
        EXPECT_EQ(results.size(),20);
        for (unsigned int i=0;i<10;i++) {
            EXPECT_EQ(results[i]["address"].asString(), "dgb1qlk3hldeynl3prqw259u8gv0jh7w5nwppxlvt3v");
            EXPECT_EQ(results[i]["height"].asUInt(), 13774816);
            EXPECT_EQ(results[i]["index"].asUInt(), i);
        }
        EXPECT_DOUBLE_EQ(results[0]["value"].asDouble(),98337072582571.75);
        EXPECT_DOUBLE_EQ(results[1]["value"].asDouble(),6713708633446.6816);
        EXPECT_DOUBLE_EQ(results[2]["value"].asDouble(),334257966149.66522);
        EXPECT_DOUBLE_EQ(results[3]["value"].asDouble(),240420084914.31442);
        EXPECT_DOUBLE_EQ(results[4]["value"].asDouble(),197186971.28990719);
        EXPECT_DOUBLE_EQ(results[5]["value"].asDouble(),214976452.98930287);
        EXPECT_DOUBLE_EQ(results[6]["value"].asDouble(),43002544.146079332);
        EXPECT_DOUBLE_EQ(results[7]["value"].asDouble(),4931416.8204720467);
        EXPECT_DOUBLE_EQ(results[8]["value"].asDouble(),1316241716.5177231);
        EXPECT_DOUBLE_EQ(results[9]["value"].asDouble(),27042686.293393936);
        for (unsigned int i=0;i<10;i++) {
            EXPECT_EQ(results[i+10]["address"].asString(), "dgb1qunxh378eltj2jrwza5sj9grvu5xud43vqvudwh");
            EXPECT_EQ(results[i+10]["height"].asUInt(), 13774816);
            EXPECT_EQ(results[i+10]["index"].asUInt(), i);
        }
        EXPECT_DOUBLE_EQ(results[10]["value"].asDouble(),1527467667.5596864);
        EXPECT_DOUBLE_EQ(results[11]["value"].asDouble(),1910775557.3557332);
        EXPECT_DOUBLE_EQ(results[12]["value"].asDouble(),2217759938.1127491);
        EXPECT_DOUBLE_EQ(results[13]["value"].asDouble(),2603546855.4896159);
        EXPECT_DOUBLE_EQ(results[14]["value"].asDouble(),1392788384.7739499);
        EXPECT_DOUBLE_EQ(results[15]["value"].asDouble(),17140480.088481843);
        EXPECT_DOUBLE_EQ(results[16]["value"].asDouble(),295650079.06799471);
        EXPECT_DOUBLE_EQ(results[17]["value"].asDouble(),215324503.40973082);
        EXPECT_DOUBLE_EQ(results[18]["value"].asDouble(),348658529.3479178);
        EXPECT_DOUBLE_EQ(results[19]["value"].asDouble(),2057862009.7660003);
    } catch (...) {
        EXPECT_TRUE(false);
    }


    //test getting rates at a specific height that isnt height of publishing and some exchange rates are missing
    try {
        Json::Value params=Json::arrayValue;
        params.append(15697995);
        auto results=rpcMethods[METHOD](params);
        EXPECT_TRUE(results.isArray());
        EXPECT_EQ(results.size(),20);
        for (unsigned int i=0;i<10;i++) {
            EXPECT_EQ(results[i]["address"].asString(), "dgb1qlk3hldeynl3prqw259u8gv0jh7w5nwppxlvt3v");
            EXPECT_EQ(results[i]["height"].asUInt(), (i==9)?15468096:15697988);
            EXPECT_EQ(results[i]["index"].asUInt(), i);
        }
        EXPECT_DOUBLE_EQ(results[0]["value"].asDouble(),189579459005608.72);
        EXPECT_DOUBLE_EQ(results[1]["value"].asDouble(),14815276047351.615);
        EXPECT_DOUBLE_EQ(results[2]["value"].asDouble(),575757675751.08789);
        EXPECT_DOUBLE_EQ(results[3]["value"].asDouble(),292631751511.54907);
        EXPECT_DOUBLE_EQ(results[4]["value"].asDouble(),340168615.3631295);
        EXPECT_DOUBLE_EQ(results[5]["value"].asDouble(),271515822.07743371);
        EXPECT_DOUBLE_EQ(results[6]["value"].asDouble(),29811593.723492645);
        EXPECT_DOUBLE_EQ(results[7]["value"].asDouble(),2949863.6538303359);
        EXPECT_DOUBLE_EQ(results[8]["value"].asDouble(),517551923.08531177);
        EXPECT_DOUBLE_EQ(results[9]["value"].asDouble(),385567140.12445772);
        for (unsigned int i=0;i<10;i++) {
            EXPECT_EQ(results[i+10]["address"].asString(), "dgb1qunxh378eltj2jrwza5sj9grvu5xud43vqvudwh");
            EXPECT_EQ(results[i+10]["height"].asUInt(), 15697988);
            EXPECT_EQ(results[i+10]["index"].asUInt(), i);
        }
        EXPECT_DOUBLE_EQ(results[10]["value"].asDouble(),7283716180.3362379);
        EXPECT_DOUBLE_EQ(results[11]["value"].asDouble(),9561589159.9936924);
        EXPECT_DOUBLE_EQ(results[12]["value"].asDouble(),9516465102.5124207);
        EXPECT_DOUBLE_EQ(results[13]["value"].asDouble(),11015891304.157017);
        EXPECT_DOUBLE_EQ(results[14]["value"].asDouble(),6497291065.9989128);
        EXPECT_DOUBLE_EQ(results[15]["value"].asDouble(),68095939.208079651);
        EXPECT_DOUBLE_EQ(results[16]["value"].asDouble(),1378578479.5979228);
        EXPECT_DOUBLE_EQ(results[17]["value"].asDouble(),519836276.13743567);
        EXPECT_DOUBLE_EQ(results[18]["value"].asDouble(),1837994011.519433);
        EXPECT_DOUBLE_EQ(results[19]["value"].asDouble(),9745933491.994772);
    } catch (...) {
        EXPECT_TRUE(false);
    }

    //test getting newest rate
    try {
        Json::Value params=Json::arrayValue;
        auto results=rpcMethods[METHOD](params);
        EXPECT_TRUE(results.isArray());
        EXPECT_EQ(results.size(),20);
        for (unsigned int i=0;i<10;i++) {
            EXPECT_EQ(results[i]["address"].asString(), "dgb1qlk3hldeynl3prqw259u8gv0jh7w5nwppxlvt3v");
            EXPECT_EQ(results[i]["height"].asUInt(), 17579454);
            EXPECT_EQ(results[i]["index"].asUInt(), i);
        }
        EXPECT_DOUBLE_EQ(results[0]["value"].asDouble(),381777420134995.75);
        EXPECT_DOUBLE_EQ(results[1]["value"].asDouble(),24270194112464.758);
        EXPECT_DOUBLE_EQ(results[2]["value"].asDouble(),1171582756226.8718);
        EXPECT_DOUBLE_EQ(results[3]["value"].asDouble(),189190233047.31625);
        EXPECT_DOUBLE_EQ(results[4]["value"].asDouble(),271994111.58515);
        EXPECT_DOUBLE_EQ(results[5]["value"].asDouble(),233321126.81288275);
        EXPECT_DOUBLE_EQ(results[6]["value"].asDouble(),59111898.915325947);
        EXPECT_DOUBLE_EQ(results[7]["value"].asDouble(),771861.75312100188);
        EXPECT_DOUBLE_EQ(results[8]["value"].asDouble(),947581829.91005576);
        EXPECT_DOUBLE_EQ(results[9]["value"].asDouble(),9.3862996206609823e+21);
        for (unsigned int i=0;i<10;i++) {
            EXPECT_EQ(results[i+10]["address"].asString(), "dgb1qunxh378eltj2jrwza5sj9grvu5xud43vqvudwh");
            EXPECT_EQ(results[i+10]["height"].asUInt(), 17579454);
            EXPECT_EQ(results[i+10]["index"].asUInt(), i);
        }
        EXPECT_DOUBLE_EQ(results[10]["value"].asDouble(),9887716273.3456039);
        EXPECT_DOUBLE_EQ(results[11]["value"].asDouble(),13054547951.462606);
        EXPECT_DOUBLE_EQ(results[12]["value"].asDouble(),14419045426.414383);
        EXPECT_DOUBLE_EQ(results[13]["value"].asDouble(),16825590294.62875);
        EXPECT_DOUBLE_EQ(results[14]["value"].asDouble(),8839281756.2276745);
        EXPECT_DOUBLE_EQ(results[15]["value"].asDouble(),92578083.702799648);
        EXPECT_DOUBLE_EQ(results[16]["value"].asDouble(),1827161556.9174917);
        EXPECT_DOUBLE_EQ(results[17]["value"].asDouble(),479430261.26714659);
        EXPECT_DOUBLE_EQ(results[18]["value"].asDouble(),2731725087.7183938);
        EXPECT_DOUBLE_EQ(results[19]["value"].asDouble(),15085279522.219233);
    } catch (...) {
        EXPECT_TRUE(false);
    }
}
