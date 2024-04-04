//
// Created by mctrivia on 31/03/24.
//

#include "AppMain.h"
#include "utils.h"
#include "BitcoinRpcServerMethods.h"
#include "gtest/gtest.h"
#include "../tests/RPCMethods.h"

using namespace std;


TEST_F(RPCMethodsTest, getassetdata) {
    ///rpc method we will be testing(if using as reference make sure you change value above and bellow this line)
    const std::string METHOD="getassetdata";
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
    //2 parameters
    try {
        Json::Value params=Json::arrayValue;
        params.append("bad1");
        params.append("bad2");
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
        params.append("bad1");
        params.append("bad2");
        params.append(1);
        params.append("bad4");
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
        params.append(false);
        rpcMethods[METHOD](params);
        result=false;
    } catch (const DigiByteException& e) {
        result=true;
    } catch (...) {
        result=false;
    }
    EXPECT_TRUE(result);

    //test invalid parameters throws an exception of type DigiByteException
    //3 parameter but wrong type
    try {
        Json::Value params=Json::arrayValue;
        params.append("La");
        params.append("La");
        params.append("La");
        rpcMethods[METHOD](params);
        result=false;
    } catch (const DigiByteException& e) {
        result=true;
    } catch (...) {
        result=false;
    }
    EXPECT_TRUE(result);

    //test invalid parameters throws an exception of type DigiByteException
    //3 parameter but wrong type
    try {
        Json::Value params=Json::arrayValue;
        params.append(1);
        params.append("La");
        params.append(5);
        rpcMethods[METHOD](params);
        result=false;
    } catch (const DigiByteException& e) {
        result=true;
    } catch (...) {
        result=false;
    }
    EXPECT_TRUE(result);

    //test invalid parameters throws an exception of type DigiByteException
    //3 parameter but wrong type
    try {
        Json::Value params=Json::arrayValue;
        params.append("La");
        params.append(1);
        params.append(5);
        rpcMethods[METHOD](params);
        result=false;
    } catch (const DigiByteException& e) {
        result=true;
    } catch (...) {
        result=false;
    }
    EXPECT_TRUE(result);

    //test asset that doesn't exist that hasn't been used
    try {
        Json::Value params=Json::arrayValue;
        params.append(50000);
        auto results=rpcMethods[METHOD](params);
        result=false;
    } catch (const DigiByteException& e) {
        result=true;
    } catch (...) {
        result=false;
    }
    EXPECT_TRUE(result);

    //test asset with invalid assetId
    try {
        Json::Value params=Json::arrayValue;
        params.append("La8u7WcjtGmDdZaUhSmspUw5oPkzCch1ALrJzh");
        auto results=rpcMethods[METHOD](params);
        result=false;
    } catch (const DigiByteException& e) {
        result=true;
    } catch (...) {
        result=false;
    }
    EXPECT_TRUE(result);

    //test asset with invalid txid
    try {
        Json::Value params=Json::arrayValue;
        params.append("Uh8u7WcjtGmDdZaUhSmspUw5oPkzCch1ALrJzh");
        params.append("3e71e807bda09dff2afea4f2edff3eadf581931632d81eb77f566b4b484dbfeb");
        params.append(0);
        auto results=rpcMethods[METHOD](params);
        result=false;
    } catch (const DigiByteException& e) {
        result=true;
    } catch (...) {
        result=false;
    }
    EXPECT_TRUE(result);

    //test known good asset using assetIndex
    try {
        Json::Value params=Json::arrayValue;
        params.append(245);
        auto results=rpcMethods[METHOD](params);
        EXPECT_TRUE(results.isObject());
        EXPECT_EQ(results["assetId"].asString(),"La3LdaAT4PEYsN4qZ2vnjdeWbE6hY7VrMZHYBU");
        EXPECT_EQ(results["assetIndex"].asUInt(),245);
        EXPECT_EQ(results["cid"].asString(),"bafkreihmr3pnmiq27obhm2epq2b6crfcbe54dclaz326zibunti5k7szaa");
        EXPECT_EQ(results["count"].asUInt64(),20999397);
        EXPECT_EQ(results["decimals"].asUInt(),0);
        EXPECT_TRUE(results["ipfs"].isObject());
        EXPECT_EQ(results["ipfs"]["data"]["assetName"].asString(),"d-BTC");
        EXPECT_EQ(results["ipfs"]["data"]["description"].asString(),"Collectible Bitcoin on DigiByte - The fastest Bitcoin on the planet. \r\nMeme of BTC with supply of 21 million as supposed to be, only now it is secured by five algorithms of the most advanced Digibyte blockchain - https://digibyte.io/");
        EXPECT_EQ(results["ipfs"]["data"]["issuer"].asString(),"CreateDigiAssets");
        EXPECT_EQ(results["ipfs"]["data"]["urls"][0]["mimeType"].asString(),"image/png");
        EXPECT_EQ(results["ipfs"]["data"]["urls"][0]["name"].asString(),"icon");
        EXPECT_EQ(results["ipfs"]["data"]["urls"][0]["url"].asString(),"https://i.imgur.com/JMwXKoE.gif");
        //todo when add hash verification this asset should fail
        EXPECT_EQ(results["issuer"]["address"].asString(),"DPdHP3JSC42wQNotcj9tuWFy8hwvKQMW73");
        EXPECT_FALSE(results.isMember("rules"));
    } catch (...) {
        EXPECT_TRUE(false);
    }

    //test known good asset using assetIndex and basic data
    try {
        Json::Value params=Json::arrayValue;
        params.append(245);
        params.append(true);
        auto results=rpcMethods[METHOD](params);
        EXPECT_TRUE(results.isObject());
        EXPECT_EQ(results["assetId"].asString(),"La3LdaAT4PEYsN4qZ2vnjdeWbE6hY7VrMZHYBU");
        EXPECT_EQ(results["assetIndex"].asUInt(),245);
        EXPECT_EQ(results["cid"].asString(),"bafkreihmr3pnmiq27obhm2epq2b6crfcbe54dclaz326zibunti5k7szaa");
        EXPECT_EQ(results["count"].asUInt64(),20999397);
        EXPECT_EQ(results["decimals"].asUInt(),0);
        EXPECT_FALSE(results.isMember("ipfs"));
        EXPECT_FALSE(results.isMember("issuer"));
        EXPECT_FALSE(results.isMember("rules"));
    } catch (...) {
        EXPECT_TRUE(false);
    }

    //test known good asset using assetId
    try {
        Json::Value params=Json::arrayValue;
        params.append("La5qRZgfCe6rWNesjb6g587ZxcPMJaq684sJQD");
        auto results=rpcMethods[METHOD](params);
        EXPECT_TRUE(results.isObject());
        EXPECT_EQ(results["assetId"].asString(),"La5qRZgfCe6rWNesjb6g587ZxcPMJaq684sJQD");
        EXPECT_EQ(results["assetIndex"].asUInt(),4610);
        EXPECT_EQ(results["cid"].asString(),"bafkreigi7batlvadnz6cqkbihwsnq6tuo4uivemd6vihyjs2ciosa6ysze");
        EXPECT_EQ(results["count"].asUInt64(),299);
        EXPECT_EQ(results["decimals"].asUInt(),0);
        EXPECT_TRUE(results["ipfs"].isObject());
        EXPECT_EQ(results["ipfs"]["data"]["assetName"].asString(),"2-tone background");
        EXPECT_EQ(results["ipfs"]["data"]["description"].asString(),"You pick the colors for this background. Special items are very collectible because they will be combinable with your Digibyte Elf to make an even more rare 1/1 Elf. (The Special item and your Elf will be sent to Majestic Jay and burned.) You will receive the new elf that is created. You can get these chips by doing special quests on twitter posted by Majestic Jay, by becoming a Digibyte Elf Society board member (Receive 10), or you may see them in the elf store on some days. To set up the special item combination. 1. Message @majesticjay218 on twitter or on telegram @majesticjay21 and tell him what you would like to combine. Give Digibyte elf number and Item name. (DO NOT DO NEXT STEPS UNTIL MAJESTIC JAY HAS CONFIRMED OR YOU COULD LOSE ITEMS.) 2. Send in the Digibyte Elf and the Item to Wallet DLstC5wedL6cZ5hVAG5wA5BKfqdksBPqkM 3. These assets will be burned. 4. Receive your new elf NFT. Must use on or before Dec 31 2023");
        EXPECT_EQ(results["ipfs"]["data"]["issuer"].asString(),"@MajesticJay218");
        EXPECT_EQ(results["ipfs"]["data"]["site"]["type"].asString(),"web");
        EXPECT_EQ(results["ipfs"]["data"]["site"]["url"].asString(),"https://digibyteelfsociety.digiassetx.com/");
        EXPECT_EQ(results["ipfs"]["data"]["urls"][0]["mimeType"].asString(),"image/jpeg");
        EXPECT_EQ(results["ipfs"]["data"]["urls"][0]["name"].asString(),"icon");
        EXPECT_EQ(results["ipfs"]["data"]["urls"][0]["url"].asString(),"ipfs://QmXgNyvDjfFpW5EcRS14GLQzYzF5wsdDWmgHPdTH1qo9wB");
        //todo when add hash verification this asset should pass
        EXPECT_EQ(results["issuer"]["address"].asString(),"DLstC5wedL6cZ5hVAG5wA5BKfqdksBPqkM");
        EXPECT_EQ(results["issuer"]["country"].asString(),"USA");
        EXPECT_EQ(results["issuer"]["hash"].asString(),"863537bdb07313a16a355e2efb9418055418f6d22560944a8bb2ab4344cac4f9");
        EXPECT_FALSE(results.isMember("rules"));
    } catch (...) {
        EXPECT_TRUE(false);
    }

    //test known good hybrid asset using assetId and basic data
    try {
        Json::Value params=Json::arrayValue;
        params.append("Uh8u7WcjtGmDdZaUhSmspUw5oPkzCch1ALrJzh");
        params.append("2e71e807bda09dff2afea4f2edff3eadf581931632d81eb77f566b4b484dbfeb");
        params.append(0);
        params.append(true);
        auto results=rpcMethods[METHOD](params);
        EXPECT_TRUE(results.isObject());
        EXPECT_EQ(results["assetId"].asString(),"Uh8u7WcjtGmDdZaUhSmspUw5oPkzCch1ALrJzh");
        EXPECT_EQ(results["assetIndex"].asUInt(),4618);
        EXPECT_EQ(results["cid"].asString(),"bafkreibwvxn2ud63o2zuijiughydmetmbk5pqtwsgdsyip3tetzcgb23vy");
        EXPECT_EQ(results["count"].asUInt64(),0);
        EXPECT_EQ(results["decimals"].asUInt(),0);
        EXPECT_FALSE(results.isMember("ipfs"));
        EXPECT_FALSE(results.isMember("issuer"));
        EXPECT_FALSE(results.isMember("rules"));
    } catch (...) {
        EXPECT_TRUE(false);
    }
    //test known good hybrid asset using assetId
    try {
        Json::Value params=Json::arrayValue;
        params.append("Uh8u7WcjtGmDdZaUhSmspUw5oPkzCch1ALrJzh");
        params.append("2e71e807bda09dff2afea4f2edff3eadf581931632d81eb77f566b4b484dbfeb");
        params.append(0);
        auto results=rpcMethods[METHOD](params);
        EXPECT_TRUE(results.isObject());
        EXPECT_EQ(results["assetId"].asString(),"Uh8u7WcjtGmDdZaUhSmspUw5oPkzCch1ALrJzh");
        EXPECT_EQ(results["assetIndex"].asUInt(),4618);
        EXPECT_EQ(results["cid"].asString(),"bafkreibwvxn2ud63o2zuijiughydmetmbk5pqtwsgdsyip3tetzcgb23vy");
        EXPECT_EQ(results["count"].asUInt64(),0);
        EXPECT_EQ(results["decimals"].asUInt(),0);
        EXPECT_TRUE(results["ipfs"].isObject());
        EXPECT_EQ(results["ipfs"]["data"]["assetName"].asString(),"DigiByte Desktop Donation Asset");
        EXPECT_EQ(results["ipfs"]["data"]["description"].asString(),"This DigiAsset has been issued to you in return for your donation to the DigiByte Desktop Wallet project in the 'Phase 1: Coin management' stage. Thanks for your support! Please, keep this DigiAsset, it's a governance token on this project.");
        EXPECT_EQ(results["ipfs"]["data"]["issuer"].asString(),"DigiByte Desktop");
        EXPECT_EQ(results["ipfs"]["data"]["site"]["type"].asString(),"web");
        EXPECT_EQ(results["ipfs"]["data"]["site"]["url"].asString(),"https://digibytedesktop.org/");
        EXPECT_EQ(results["ipfs"]["data"]["urls"][0]["mimeType"].asString(),"image/png");
        EXPECT_EQ(results["ipfs"]["data"]["urls"][0]["name"].asString(),"icon");
        EXPECT_EQ(results["ipfs"]["data"]["urls"][0]["url"].asString(),"ipfs://QmcrDoLzCQB1dnJfGm52xVBZEd1SmV35LZAbiPj4RJCXT2");
        //todo when add hash verification this asset should pass
        EXPECT_EQ(results["issuer"]["address"].asString(),"DDeskxHKkHxc3J9g98ZtEvkots3r19u3gp");
        EXPECT_EQ(results["issuer"]["country"].asString(),"PER");
        EXPECT_EQ(results["issuer"]["hash"].asString(),"8af4ebe735f40efc2a296fc5ca53afd142cd86495c8b828d2c8eaa38c04cf019");
        EXPECT_FALSE(results.isMember("rules"));
    } catch (...) {
        EXPECT_TRUE(false);
    }

    //todo test known good asset with decimals

    //test assets with rules
    try {
        Json::Value params=Json::arrayValue;
        params.append(2005);
        auto results=rpcMethods[METHOD](params);
        EXPECT_TRUE(results.isObject());
        EXPECT_EQ(results["assetId"].asString(),"La5oE1Y6Yt2ofTuTJVj87cH6hpar4bR2KtT8Ad");
        EXPECT_EQ(results["assetIndex"].asUInt(),2005);
        EXPECT_EQ(results["cid"].asString(),"bafkreia2qu7gybs7kvedli6vqodxlxsv3anmer7lybqvepmc6yi3pp3xru");
        EXPECT_EQ(results["count"].asUInt64(),5);
        EXPECT_EQ(results["decimals"].asUInt(),0);
        EXPECT_TRUE(results["ipfs"].isObject());
        EXPECT_EQ(results["ipfs"]["data"]["assetName"].asString(),"Digibyte Elf #201");
        EXPECT_EQ(results["ipfs"]["data"]["description"].asString(),"Welcome to the Digibyte Elf Society!\nThis elf is 1 of 5 copies in this (wave 1) set of 1000\nIf you own one of the first 250 elves, you will be put in a wheel spin to win the Digibyte Elf #1! \nIf you own 10, contact Majestic Jay, you are eligible to be a Board Member with perks.\nThese Digibyte Elves were designed to be cool and different so they could help start conversations on your social media and bring people to Digibyte. \nMake sure to bid or pay from your digiassetx wallet.\nTo calculate rarity, go to http://www.digibyteelfsociety.com/ and click on the rarity tab. While you are there, look at the road map.\n");
        EXPECT_EQ(results["ipfs"]["data"]["issuer"].asString(),"@MajesticJay218");
        EXPECT_EQ(results["ipfs"]["data"]["site"]["type"].asString(),"web");
        EXPECT_EQ(results["ipfs"]["data"]["site"]["url"].asString(),"https://digibyteelfsociety.digiassetx.com/");
        EXPECT_EQ(results["ipfs"]["data"]["urls"][0]["mimeType"].asString(),"image/jpeg");
        EXPECT_EQ(results["ipfs"]["data"]["urls"][0]["name"].asString(),"icon");
        EXPECT_EQ(results["ipfs"]["data"]["urls"][0]["url"].asString(),"ipfs://QmXgG98WdTWHrXVbgumupY8NANZUKn6teNSVbNiieCYwQW");
        //todo when add hash verification this asset should pass
        EXPECT_EQ(results["issuer"]["address"].asString(),"DLstC5wedL6cZ5hVAG5wA5BKfqdksBPqkM");
        EXPECT_EQ(results["issuer"]["country"].asString(),"USA");
        EXPECT_EQ(results["issuer"]["hash"].asString(),"863537bdb07313a16a355e2efb9418055418f6d22560944a8bb2ab4344cac4f9");
        EXPECT_TRUE(results.isMember("rules"));
        EXPECT_EQ(results["rules"]["changeable"].asBool(),false);
        EXPECT_EQ(results["rules"]["royalty"]["addresses"]["dgb1qln5tskmffn3a4cq0gz56lagzzwmvk7zwnlnn0c"].asUInt64(),1980000000);
        EXPECT_EQ(results["rules"]["royalty"]["addresses"]["dgb1qwlnzswupjvlczfclqxgwcsgzlzf803yzzv97q8"].asUInt64(),20000000);
        EXPECT_FALSE(results["rules"]["royalty"].isMember("units"));
        EXPECT_FALSE(results["rules"].isMember("deflation"));
        EXPECT_FALSE(results["rules"].isMember("expiry"));
        EXPECT_FALSE(results["rules"].isMember("geofence"));
        EXPECT_FALSE(results["rules"].isMember("voting"));
        EXPECT_FALSE(results["rules"].isMember("approval"));
    } catch (...) {
        EXPECT_TRUE(false);
    }
}
