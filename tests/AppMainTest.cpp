//
// Tests for the AppMain singleton(src/AppMain.cpp)
//

#include "AppMain.h"
#include "gtest/gtest.h"
#include <stdexcept>

TEST(AppMain, singletonReturnsSameInstance) {
    AppMain* a = AppMain::GetInstance();
    AppMain* b = AppMain::GetInstance();
    EXPECT_EQ(a, b);
    EXPECT_NE(a, nullptr);
}

TEST(AppMain, settersGettersAndReset) {
    AppMain* main = AppMain::GetInstance();
    main->reset();

    //getters throw when nothing has been loaded
    EXPECT_THROW(main->getDatabase(), std::runtime_error);
    EXPECT_THROW(main->getIPFS(), std::runtime_error);
    EXPECT_THROW(main->getDigiByteCore(), std::runtime_error);
    EXPECT_FALSE(main->isDigiByteCoreSet());

    //set and get(pointers don't need to point at real objects for this test)
    Database* fakeDb = reinterpret_cast<Database*>(0x1);
    DigiByteCore* fakeDgb = reinterpret_cast<DigiByteCore*>(0x2);
    main->setDatabase(fakeDb);
    main->setDigiByteCore(fakeDgb);
    EXPECT_EQ(main->getDatabase(), fakeDb);
    EXPECT_EQ(main->getDigiByteCore(), fakeDgb);
    EXPECT_TRUE(main->isDigiByteCoreSet());

    //reset clears everything again
    main->reset();
    EXPECT_THROW(main->getDatabase(), std::runtime_error);
    EXPECT_THROW(main->getDigiByteCore(), std::runtime_error);
    EXPECT_FALSE(main->isDigiByteCoreSet());
}
