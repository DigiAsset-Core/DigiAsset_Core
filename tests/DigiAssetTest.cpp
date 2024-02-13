//
// Created by mctrivia on 01/07/23.
//

#include "DigiAsset.h"
#include "AppMain.h"
#include "DigiByteCore.h"
#include "DigiByteTransaction.h"
#include "gtest/gtest.h"
#include <cmath>

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <vector>

using namespace std;

std::string uint8_vector_to_hex_string(const vector<uint8_t>& v) {
    std::string result;
    result.reserve(v.size() * 2); // two digits per character

    static constexpr char hex[] = "0123456789abcdef";

    for (uint8_t c: v) {
        result.push_back(hex[c / 16]);
        result.push_back(hex[c % 16]);
    }

    return result;
}

/*
 * ██████╗░██████╗░██╗██╗░░░██╗░█████╗░████████╗███████╗
 * ██╔══██╗██╔══██╗██║██║░░░██║██╔══██╗╚══██╔══╝██╔════╝
 * ██████╔╝██████╔╝██║╚██╗░██╔╝███████║░░░██║░░░█████╗░░
 * ██╔═══╝░██╔══██╗██║░╚████╔╝░██╔══██║░░░██║░░░██╔══╝░░
 * ██║░░░░░██║░░██║██║░░╚██╔╝░░██║░░██║░░░██║░░░███████╗
 * ╚═╝░░░░░╚═╝░░╚═╝╚═╝░░░╚═╝░░░╚═╝░░╚═╝░░░╚═╝░░░╚══════╝
 */

TEST(DigiAsset, calcSimpleScriptPubKey) {

    //Test compatible address ST2ZgmwKyaEUsRZ8BWPhNVDCFA3KtUKCFR
    vin_t test1;
    test1.txid = "02dd717266dfec19717724b9febcb8937c61e3642acc46ec6bba0c1c6a99fefa";
    test1.n = 0;
    test1.scriptSig = {
            .assm = "0014c55f251fd8218063d0e45230326e6fd845ecb3af",
            .hex = "160014c55f251fd8218063d0e45230326e6fd845ecb3af"};
    test1.txinwitness = {
            "3044022065c8aafef38f03ca36c74e34d01041ae7bf84d29eeb474c810ffa85b5f5121a20220269a23a1640aeac6fddb86e0325c5716ce257f744dc3a53abda3b331453b35f601"
            "0272a35d03a404224fd6c8798b66dde3162678ebf1aadb0d01deb2f6818a15feba"};
    vector<uint8_t> test1Result = DigiAsset::calcSimpleScriptPubKey(test1);
    EXPECT_EQ(uint8_vector_to_hex_string(test1Result), "a9143ee55b86d113278c234a5e6064dd2f997de45d7587");

    //Test segwit address dgb1qpup70e8dxutmhjt9dsf3gq3hu5jl2yrfv7kl4a
    vin_t test2;
    test2.txid = "4d46ba8df74f4deb08687660c786d8220335c48ed348c29fe267d5f79768a99d";
    test2.n = 2;
    test2.scriptSig = {
            .assm = "",
            .hex = ""};
    test2.txinwitness = {
            "3044022043779e8b2b9b71fcf5170feb299ebbf76aa5f46e5461091ce37603e9d240dd060220318d48f26af24ccfdd1b0267a3e2ba63919c8e4750433875358b590e9f02bab801",
            "03ff56d1ac84e4070f5a84e7eeedd45b56ad95792e1fed5278dc8f2d8f5b77c3f1"};
    vector<uint8_t> test2Result = DigiAsset::calcSimpleScriptPubKey(test2);
    EXPECT_EQ(uint8_vector_to_hex_string(test2Result), "00140f03e7e4ed3717bbc9656c13140237e525f51069");

    //Test legacy address DRD546mgDoaKHwBBqrArHku6QVCRezWo9d
    vin_t test3;
    test3.txid = "ede9629b9c982f1f32b4d02653f95ef21f7eaee10d83cb70bb02726c5ea665f1";
    test3.n = 0;
    test3.scriptSig = {
            .assm = "3045022100f102988d4717bf94bdc2f998c98f6abee31169d13417d6949f1ca36db03569c402205cea2f37bded24e98d89214fe756500f954be0d96ba5d5a7f6bfc0f03f28263a[ALL] 02cc61f2991e75457be8f12c381681b07050afafc720e48be3a851e964f2d7732b",
            .hex = "483045022100f102988d4717bf94bdc2f998c98f6abee31169d13417d6949f1ca36db03569c402205cea2f37bded24e98d89214fe756500f954be0d96ba5d5a7f6bfc0f03f28263a012102cc61f2991e75457be8f12c381681b07050afafc720e48be3a851e964f2d7732b"};
    vector<uint8_t> test3Result = DigiAsset::calcSimpleScriptPubKey(test3);
    EXPECT_EQ(uint8_vector_to_hex_string(test3Result), "76a914dc22f0833765a3a39753235cd462fcba22841e6788ac");

    //test multisig address SgkRCP6ping27jrR9dmtwMLg7mgUzWePmM
    vin_t test4;
    test4.txid = "b4cbcfb860bfd6808e10081dc3a984212244d017cc541b9a135247f91a4183c0";
    test4.n = 0;
    test4.scriptSig = {
            .assm = "0 3045022100a985632a8004fc0473adadb53b8b9a6930be18264b2645b4d122eade2a425e4702201a23a8c148cf6048c3ab7976273c40c4c4cd49ad92c8be117aaf2fc27f4b4eb4[ALL] 3044022076506c16b0c3f39bd36ac4b869084f4a10b6f46a61c6722f0e58e38f6fb3dc6402205644d0857df855c609698b29be2b91562841240f9d20c034905bfbb62fc8dab3[ALL] 52210304571615cb513a646c65d0f6cd2bb6b69d4326e5caffe3e0eeb3e36371e864e221038ad986fd4d1449db772cce40cbce94b15bf36d5a3555e8074ef3156be96f603552ae",
            .hex = "00483045022100a985632a8004fc0473adadb53b8b9a6930be18264b2645b4d122eade2a425e4702201a23a8c148cf6048c3ab7976273c40c4c4cd49ad92c8be117aaf2fc27f4b4eb401473044022076506c16b0c3f39bd36ac4b869084f4a10b6f46a61c6722f0e58e38f6fb3dc6402205644d0857df855c609698b29be2b91562841240f9d20c034905bfbb62fc8dab3014752210304571615cb513a646c65d0f6cd2bb6b69d4326e5caffe3e0eeb3e36371e864e221038ad986fd4d1449db772cce40cbce94b15bf36d5a3555e8074ef3156be96f603552ae"};
    vector<uint8_t> test4Result = DigiAsset::calcSimpleScriptPubKey(test4);
    EXPECT_EQ(uint8_vector_to_hex_string(test4Result), "a914d569605b4d3ce2bd89dbc8e5bf301f96f756299987");
}

TEST(DigiAsset, calculateAssetId) {

    //test first issuance produces the correct AssetId
    vin_t testData1;
    testData1.txid = "dd49f460b638b34c820d9627db7e7d57860a95ccce2f7dc5a9c32ecf01906b01";
    testData1.n = 0;
    testData1.scriptSig = {
            .assm = "304402200b78670e1af4311c30577f907580e90aa0bedd15a72c46d3da06360dfd138eb202201062fd5cfec6ea6b24219d1c4f075cad87303ac756b6595af8e97c42e29f54d4[ALL] 0361c0d3a2175c32c1ad08972a22004779d9aee8d4d72fb3c038b373883b70b67c",
            .hex = "47304402200b78670e1af4311c30577f907580e90aa0bedd15a72c46d3da06360dfd138eb202201062fd5cfec6ea6b24219d1c4f075cad87303ac756b6595af8e97c42e29f54d401210361c0d3a2175c32c1ad08972a22004779d9aee8d4d72fb3c038b373883b70b67c"};
    testData1.sequence = 4294967295;
    DigiAsset test1;
    test1._locked = false;
    string result = test1.calculateAssetId(testData1, 0x40);
    EXPECT_EQ(result, "Ua94nEKabzhJeDJtxGFXdviT185tYeHqyHKeWC");

    //try a locked asset
    vin_t testData2;
    testData2.txid = "e4941631db44727eacd85f7ccae9b03bf987aba97c8962abcdaf7a9833588280";
    testData2.n = 1;
    testData2.scriptSig = {
            .assm = "3045022100d611e1a57a299e8bdd09224bea7a5f4c03501ae01ec76e3188c3d75f1dce48f0022059e9e14d96f15080fa7980d77df7233a9522850ca9792defa0469a9df7b07d61[ALL] 02b294257f114aa8c2423b76e29b75b354ebd0fc280ea8538262672396b4799257",
            .hex = "483045022100d611e1a57a299e8bdd09224bea7a5f4c03501ae01ec76e3188c3d75f1dce48f0022059e9e14d96f15080fa7980d77df7233a9522850ca9792defa0469a9df7b07d61012102b294257f114aa8c2423b76e29b75b354ebd0fc280ea8538262672396b4799257"};
    testData2.sequence = 4294967295;
    DigiAsset test2;
    test2._locked = true;
    result = test2.calculateAssetId(testData2, 0x10);
    EXPECT_EQ(result, "La3fq5SvLvxHssNL9FJESiemGwMVsou89tuD3m");

    //todo need some hybrid and unique assets
}

TEST(DigiAsset, getStrCount) {
    //initialize prerequisites
    AppMain* main = AppMain::GetInstance();
    DigiByteCore dgb;
    dgb.setFileName("config.cfg");
    dgb.makeConnection();
    main->setDigiByteCore(&dgb);
    Database db("../tests/testFiles/assetTest.db");
    main->setDatabase(&db);

    //do test
    DigiAsset test(0, "Ua94nEKabzhJeDJtxGFXdviT185tYeHqyHKeWC", "", KYC(), DigiAssetRules(), 1, 1, 5);
    test.setCount(5);
    EXPECT_EQ(test.getStrCount(), "0.05");
    test.setCount(500);
    EXPECT_EQ(test.getStrCount(), "5.00");
    test.setCount(5000);
    EXPECT_EQ(test.getStrCount(), "50.00");

    //clean up
    main->reset();
}