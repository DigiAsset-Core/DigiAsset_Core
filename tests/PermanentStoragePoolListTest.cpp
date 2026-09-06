//
// Tests for src/PermanentStoragePool/PermanentStoragePoolList.cpp
// Needs bin/config.cfg to exist(same requirement as the other PSP tests).
//

#include "AppMain.h"
#include "Database.h"
#include "PermanentStoragePool/PermanentStoragePoolList.h"
#include "gtest/gtest.h"
#include <cstdio>

TEST(PermanentStoragePoolList, poolAccess) {
    //pool initialization checks the database for content that needs repinning
    const std::string dbPath = "../tests/testFiles/pspListTest.db";
    remove(dbPath.c_str());
    remove((dbPath + "-wal").c_str());
    remove((dbPath + "-shm").c_str());
    Database db(dbPath);
    AppMain* appMain = AppMain::GetInstance();
    appMain->setDatabase(&db);

    {
        PermanentStoragePoolList list("config.cfg");

        //there are currently 2 known pools: local(0) and mctrivia(1)
        ASSERT_EQ(list.getPoolCount(), (unsigned int) 2);
        EXPECT_EQ(list.getPool(0)->getName(), "Local Storage");
        EXPECT_FALSE(list.getPool(1)->getName().empty());

        //out of range throws
        EXPECT_THROW(list.getPool(2), std::out_of_range);

        //iteration sees every pool
        unsigned int count = 0;
        for (const auto& pool: list) {
            EXPECT_NE(pool.get(), nullptr);
            count++;
        }
        EXPECT_EQ(count, list.getPoolCount());

        //getRandomPool never returns the local pool(index 0)
        for (int i = 0; i < 50; i++) {
            EXPECT_NE(list.getRandomPool(), list.getPool(0));
        }
    }

    appMain->reset();
    remove(dbPath.c_str());
    remove((dbPath + "-wal").c_str());
    remove((dbPath + "-shm").c_str());
}
