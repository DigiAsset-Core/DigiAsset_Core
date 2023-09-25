//
// Created by mctrivia on 13/06/23.
//

#include "gtest/gtest.h"
#include "IPFS.h"

using namespace std;


TEST(IPFS, sha256ToCID) {
    EXPECT_EQ(IPFS::sha256ToCID("51D3CC662F89E8535D9CF74751DA0F91335A083CF12CB4C9BA81FFF25458274D"),
              "bafkreicr2pggml4j5bjv3hhxi5i5ud4rgnnaqphrfs2mtoub77zfiwbhju");
    BitIO testData = BitIO::makeHexString("51D3CC662F89E8535D9CF74751DA0F91335A083CF12CB4C9BA81FFF25458274D");
    EXPECT_EQ(IPFS::sha256ToCID(testData), "bafkreicr2pggml4j5bjv3hhxi5i5ud4rgnnaqphrfs2mtoub77zfiwbhju");
}