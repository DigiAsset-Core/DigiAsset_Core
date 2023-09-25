//
// Created by mctrivia on 21/04/23.
//
#include <cmath>
#include "gtest/gtest.h"
#include "BitIO.h"

using namespace std;


/*
 * ██████╗░██████╗░██╗██╗░░░██╗░█████╗░████████╗███████╗
 * ██╔══██╗██╔══██╗██║██║░░░██║██╔══██╗╚══██╔══╝██╔════╝
 * ██████╔╝██████╔╝██║╚██╗░██╔╝███████║░░░██║░░░█████╗░░
 * ██╔═══╝░██╔══██╗██║░╚████╔╝░██╔══██║░░░██║░░░██╔══╝░░
 * ██║░░░░░██║░░██║██║░░╚██╔╝░░██║░░██║░░░██║░░░███████╗
 * ╚═╝░░░░░╚═╝░░╚═╝╚═╝░░░╚═╝░░░╚═╝░░╚═╝░░░╚═╝░░░╚══════╝
 */

TEST(BitIO, MakeRightMask) {
    EXPECT_EQ(BitIO::makeRightMask(0), 0x00);
    EXPECT_EQ(BitIO::makeRightMask(1), 0x01);
    EXPECT_EQ(BitIO::makeRightMask(2), 0x03);
    EXPECT_EQ(BitIO::makeRightMask(3), 0x07);
    EXPECT_EQ(BitIO::makeRightMask(4), 0x0f);
    EXPECT_EQ(BitIO::makeRightMask(8), 0xff);
    EXPECT_EQ(BitIO::makeRightMask(12), 0xfff);
    EXPECT_EQ(BitIO::makeRightMask(16), 0xffff);
    EXPECT_EQ(BitIO::makeRightMask(32), 0xffffffff);
    EXPECT_EQ(BitIO::makeRightMask(64), 0xffffffffffffffff);
}

TEST(BitIO, MakeLeftMask) {
    EXPECT_EQ(BitIO::makeLeftMask(0), 0x00);
    EXPECT_EQ(BitIO::makeLeftMask(1), 0x8000000000000000);
    EXPECT_EQ(BitIO::makeLeftMask(2), 0xc000000000000000);
    EXPECT_EQ(BitIO::makeLeftMask(3), 0xe000000000000000);
    EXPECT_EQ(BitIO::makeLeftMask(4), 0xf000000000000000);
    EXPECT_EQ(BitIO::makeLeftMask(8), 0xff00000000000000);
    EXPECT_EQ(BitIO::makeLeftMask(12), 0xfff0000000000000);
    EXPECT_EQ(BitIO::makeLeftMask(16), 0xffff000000000000);
    EXPECT_EQ(BitIO::makeLeftMask(32), 0xffffffff00000000);
    EXPECT_EQ(BitIO::makeLeftMask(64), 0xffffffffffffffff);
}

TEST(BitIO, MakeCenterMask) {
    EXPECT_EQ(BitIO::makeCenterMask(0, 0), 0x00);
    EXPECT_EQ(BitIO::makeCenterMask(0, 1), 0x00);
    EXPECT_EQ(BitIO::makeCenterMask(0, 64), 0x00);
    EXPECT_EQ(BitIO::makeCenterMask(1, 0), 0x01);
    EXPECT_EQ(BitIO::makeCenterMask(2, 0), 0x03);
    EXPECT_EQ(BitIO::makeCenterMask(8, 0), 0xff);
    EXPECT_EQ(BitIO::makeCenterMask(8, 8), 0xff00);
    EXPECT_EQ(BitIO::makeCenterMask(32, 32), 0xffffffff00000000);
    EXPECT_EQ(BitIO::makeCenterMask(8, 48), 0x00ff000000000000);
    EXPECT_EQ(BitIO::makeCenterMask(63, 1), 0xfffffffffffffffe);
    EXPECT_EQ(BitIO::makeCenterMask(64, 0), 0xffffffffffffffff);
}

TEST(BitIO, GetRandomLong) {
    /**
     * make sure random number generator sets all bits to 1 at least once
     * on average should take only 10 loops but we will give many times that in case of really bad luck
     */
    uint64_t ored = 0;
    for (size_t i = 0; i < 65536; i++) {
        uint64_t test = BitIO::getRandomLong();
        ored |= test;
        if (ored == 0xffffffffffffffff) return;
    }
    EXPECT_TRUE(false);
}

TEST(BitIO, StringLength) {
    EXPECT_EQ(BitIO::stringLength("test"), 4);
    EXPECT_EQ(BitIO::stringLength(u8"🌈📈 cooL éठ"), 10);
}

TEST(BitIO, Pow10) {
    //checck all valid exponents
    uint64_t answer = 1;
    for (size_t exp = 0; exp < 19; exp++) {
        EXPECT_EQ(BitIO::pow10(exp), answer);
        answer *= 10;
    }

    //check first invalid exponent
    bool failed = true;
    try {
        BitIO::pow10(19);
    } catch (const std::exception& e) {
        failed = false;
    }
    EXPECT_FALSE(failed);
}







/*
 * ░█████╗░██████╗░███████╗░█████╗░████████╗███████╗
 * ██╔══██╗██╔══██╗██╔════╝██╔══██╗╚══██╔══╝██╔════╝
 * ██║░░╚═╝██████╔╝█████╗░░███████║░░░██║░░░█████╗░░
 * ██║░░██╗██╔══██╗██╔══╝░░██╔══██║░░░██║░░░██╔══╝░░
 * ╚█████╔╝██║░░██║███████╗██║░░██║░░░██║░░░███████╗
 * ░╚════╝░╚═╝░░╚═╝╚══════╝╚═╝░░╚═╝░░░╚═╝░░░╚══════╝
 */

TEST(BitIO, Create) {
    //1 byte, length set to 0
    std::vector<uint8_t> inputA{0x85};
    BitIO test = BitIO(inputA);
    EXPECT_EQ(test.getLength(), 8);
    EXPECT_EQ(test.getPosition(), 0);
    EXPECT_EQ(test.getBits(8), 0x85);

    //2 byte, length set to 0
    std::vector<uint8_t> inputB{0x85, 0xa8};
    test = BitIO(inputB);
    EXPECT_EQ(test.getLength(), 16);
    EXPECT_EQ(test.getPosition(), 0);
    EXPECT_EQ(test.getBits(16), 0x85a8);

    //8 byte, length set to 0
    std::vector<uint8_t> inputC{0x85, 0xa8, 0x00, 0x9f, 0xa1, 0xcc, 0x52, 0x08};
    test = BitIO(inputC);
    EXPECT_EQ(test.getLength(), 64);
    EXPECT_EQ(test.getPosition(), 0);
    EXPECT_EQ(test.getBits(64), 0x85a8009fa1cc5208);

    //9 byte length set to 0
    std::vector<uint8_t> inputD{0x85, 0xa8, 0x00, 0x9f, 0xa1, 0xcc, 0x52, 0x08, 0x78};
    test = BitIO(inputD);
    EXPECT_EQ(test.getLength(), 72);
    EXPECT_EQ(test.getPosition(), 0);
    EXPECT_EQ(test.getBits(64), 0x85a8009fa1cc5208);
    EXPECT_EQ(test.getBits(8), 0x78);

    //1 byte, length set to 8
    test = BitIO(inputA, 8);
    EXPECT_EQ(test.getLength(), 8);
    EXPECT_EQ(test.getPosition(), 0);
    EXPECT_EQ(test.getBits(8), 0x85);

    //2 byte, length set to 16
    test = BitIO(inputB, 16);
    EXPECT_EQ(test.getLength(), 16);
    EXPECT_EQ(test.getPosition(), 0);
    EXPECT_EQ(test.getBits(16), 0x85a8);

    //8 byte, length set to 64
    test = BitIO(inputC, 64);
    EXPECT_EQ(test.getLength(), 64);
    EXPECT_EQ(test.getPosition(), 0);
    EXPECT_EQ(test.getBits(64), 0x85a8009fa1cc5208);

    //9 byte length set to 72
    test = BitIO(inputD, 72);
    EXPECT_EQ(test.getLength(), 72);
    EXPECT_EQ(test.getPosition(), 0);
    EXPECT_EQ(test.getBits(64), 0x85a8009fa1cc5208);
    EXPECT_EQ(test.getBits(8), 0x78);

    //1 byte, length set to 7 but 8 is required
    bool failed = true;
    try {
        test = BitIO(inputA, 7);
    } catch (const std::exception& e) {
        failed = false;
    }
    EXPECT_FALSE(failed);

    //1 byte, length set to 7
    inputA[0] = 0x78;
    test = BitIO(inputA, 7);
    EXPECT_EQ(test.getLength(), 7);
    EXPECT_EQ(test.getPosition(), 0);
    EXPECT_EQ(test.getBits(7), 0x3c);    //bits are shifted 1 to right from what was entered

    //2 byte, length set to 15
    test = BitIO(inputB, 15);
    EXPECT_EQ(test.getLength(), 15);
    EXPECT_EQ(test.getPosition(), 0);
    EXPECT_EQ(test.getBits(15), 0x42d4);

    //8 byte, length set to 63
    test = BitIO(inputC, 63);
    EXPECT_EQ(test.getLength(), 63);
    EXPECT_EQ(test.getPosition(), 0);
    EXPECT_EQ(test.getBits(63), 0x42d4004fd0e62904);

    //9 byte length set to 71
    test = BitIO(inputD, 71);
    EXPECT_EQ(test.getLength(), 71);
    EXPECT_EQ(test.getPosition(), 0);
    EXPECT_EQ(test.getBits(64), 0x85a8009fa1cc5208);
    EXPECT_EQ(test.getBits(7), 0x3c);

    //2 byte, length set to 8
    failed = true;
    try {
        test = BitIO(inputB, 8);
    } catch (const std::exception& e) {
        failed = false;
    }
    EXPECT_FALSE(failed);

    //2 byte, length set to 17
    failed = true;
    try {
        test = BitIO(inputB, 17);
    } catch (const std::exception& e) {
        failed = false;
    }
    EXPECT_FALSE(failed);

    //0 byte, length set to 0
    inputA.clear();
    test = BitIO(inputA);
    EXPECT_EQ(test.getLength(), 0);
    EXPECT_EQ(test.getPosition(), 0);
}





/*
 * ██████╗░░█████╗░░██████╗██╗████████╗██╗░█████╗░███╗░░██╗
 * ██╔══██╗██╔══██╗██╔════╝██║╚══██╔══╝██║██╔══██╗████╗░██║
 * ██████╔╝██║░░██║╚█████╗░██║░░░██║░░░██║██║░░██║██╔██╗██║
 * ██╔═══╝░██║░░██║░╚═══██╗██║░░░██║░░░██║██║░░██║██║╚████║
 * ██║░░░░░╚█████╔╝██████╔╝██║░░░██║░░░██║╚█████╔╝██║░╚███║
 * ╚═╝░░░░░░╚════╝░╚═════╝░╚═╝░░░╚═╝░░░╚═╝░╚════╝░╚═╝░░╚══╝
 */


TEST(BitIO, MovePositionBy) {
    //build test object
    std::vector<uint8_t> input{
            0xc5, 0xa3, 0xc3, 0xef, 0x0c, 0x52, 0xda, 0x9c,
            0x5f, 0x46, 0xfb, 0xcf, 0x60, 0xe7, 0xfe, 0x46
    };
    BitIO test{input};

    //try moving forward valid amount
    test.movePositionBy(5);
    EXPECT_EQ(test.getPosition(), 5);

    //try moving backwards valid amount
    test.movePositionBy(-2);
    EXPECT_EQ(test.getPosition(), 3);

    //try moving backward by invalid amount
    bool failed = true;
    try {
        test.movePositionBy(-4);
    } catch (const std::exception& e) {
        failed = false;
    }
    EXPECT_FALSE(failed);
    EXPECT_EQ(test.getPosition(), 3);

    //try moving back to the beginning
    test.movePositionBy(-3);
    EXPECT_EQ(test.getPosition(), 0);

    //try moving forward by more than a long
    test.movePositionBy(70);
    EXPECT_EQ(test.getPosition(), 70);

    //test moving forward by an invalid amount
    failed = true;
    try {
        test.movePositionBy(59);
    } catch (const std::exception& e) {
        failed = false;
    }
    EXPECT_FALSE(failed);
    EXPECT_EQ(test.getPosition(), 70);

    //try moving to end
    test.movePositionBy(58);
    EXPECT_EQ(test.getPosition(), 128);
}


TEST(BitIO, MovePositionTo) {
    //build test object
    std::vector<uint8_t> input{
            0xc5, 0xa3, 0xc3, 0xef, 0x0c, 0x52, 0xda, 0x9c,
            0x5f, 0x46, 0xfb, 0xcf, 0x60, 0xe7, 0xfe, 0x46
    };
    BitIO test{input};

    //try moving to middle
    test.movePositionTo(60);
    EXPECT_EQ(test.getPosition(), 60);

    //try moving to the beginning
    test.movePositionTo(0);
    EXPECT_EQ(test.getPosition(), 0);

    //try moving to end
    test.movePositionBy(128);
    EXPECT_EQ(test.getPosition(), 128);

    //try past end of data
    bool failed = true;
    try {
        test.movePositionBy(129);
    } catch (const std::exception& e) {
        failed = false;
    }
    EXPECT_FALSE(failed);
    EXPECT_EQ(test.getPosition(), 128);
}

TEST(BitIO, MovePositionToBeginning) {
    //build test object
    std::vector<uint8_t> input{
            0xc5, 0xa3, 0xc3, 0xef, 0x0c, 0x52, 0xda, 0x9c,
            0x5f, 0x46, 0xfb, 0xcf, 0x60, 0xe7, 0xfe, 0x46
    };
    BitIO test{input};
    test.movePositionTo(60);
    EXPECT_EQ(test.getPosition(), 60);

    //try moving to the beginning
    test.movePositionToBeginning();
    EXPECT_EQ(test.getPosition(), 0);
}

TEST(BitIO, MovePositionEnd) {
    //build test object
    std::vector<uint8_t> input{
            0xc5, 0xa3, 0xc3, 0xef, 0x0c, 0x52, 0xda, 0x9c,
            0x5f, 0x46, 0xfb, 0xcf, 0x60, 0xe7, 0xfe, 0x46
    };
    BitIO test{input};

    //try moving to the end
    test.movePositionToEnd();
    EXPECT_EQ(test.getPosition(), 128);
}

TEST(BitIO, GetPosition) {
    //build test object
    std::vector<uint8_t> input{
            0xc5, 0xa3, 0xc3, 0xef, 0x0c, 0x52, 0xda, 0x9c,
            0x5f, 0x46, 0xfb, 0xcf, 0x60, 0xe7, 0xfe, 0x46
    };
    BitIO test{input};

    //already tested but let's do it anyway
    test.movePositionToEnd();
    EXPECT_EQ(test.getPosition(), 128);
}

TEST(BitIO, GetLength) {
    //build test object
    std::vector<uint8_t> input{
            0xc5, 0xa3, 0xc3, 0xef, 0x0c, 0x52, 0xda, 0x9c,
            0x5f, 0x46, 0xfb, 0xcf, 0x60, 0xe7, 0xfe, 0x46
    };
    BitIO test{input};

    //test get length that aligns with long
    EXPECT_EQ(test.getLength(), 128);

    //test get length when length is zero
    test = BitIO();
    EXPECT_EQ(test.getLength(), 0);

    //test length that does not align with long
    test.appendBits(0, 4);
    EXPECT_EQ(test.getLength(), 4);
}

TEST(BitIO, GetNumberOfBitsLeft) {
    //build test object
    std::vector<uint8_t> input{
            0xc5, 0xa3, 0xc3, 0xef, 0x0c, 0x52, 0xda, 0x9c,
            0x5f, 0x46, 0xfb, 0xcf, 0x60, 0xe7, 0xfe, 0x46
    };
    BitIO test{input};

    //test at beginning
    EXPECT_EQ(test.getNumberOfBitLeft(), 128);

    //test in middle
    test.movePositionTo(50);
    EXPECT_EQ(test.getNumberOfBitLeft(), 78);

    //test at end
    test.movePositionTo(128);
    EXPECT_EQ(test.getNumberOfBitLeft(), 0);
}




/*
 * ██████╗░██╗████████╗  ██████╗░███████╗░█████╗░██████╗░░░░░██╗░██╗░░░░░░░██╗██████╗░██╗████████╗███████╗
 * ██╔══██╗██║╚══██╔══╝  ██╔══██╗██╔════╝██╔══██╗██╔══██╗░░░██╔╝░██║░░██╗░░██║██╔══██╗██║╚══██╔══╝██╔════╝
 * ██████╦╝██║░░░██║░░░  ██████╔╝█████╗░░███████║██║░░██║░░██╔╝░░╚██╗████╗██╔╝██████╔╝██║░░░██║░░░█████╗░░
 * ██╔══██╗██║░░░██║░░░  ██╔══██╗██╔══╝░░██╔══██║██║░░██║░██╔╝░░░░████╔═████║░██╔══██╗██║░░░██║░░░██╔══╝░░
 * ██████╦╝██║░░░██║░░░  ██║░░██║███████╗██║░░██║██████╔╝██╔╝░░░░░╚██╔╝░╚██╔╝░██║░░██║██║░░░██║░░░███████╗
 * ╚═════╝░╚═╝░░░╚═╝░░░  ╚═╝░░╚═╝╚══════╝╚═╝░░╚═╝╚═════╝░╚═╝░░░░░░░╚═╝░░░╚═╝░░╚═╝░░╚═╝╚═╝░░░╚═╝░░░╚══════╝
 */

TEST(BitIO, GetBits) {
    //build test object
    std::vector<uint8_t> input{
            0xc5, 0xa3, 0xc3, 0xef, 0x0c, 0x52, 0xda, 0x9c,
            0x5f, 0x46, 0xfb, 0xcf, 0x60, 0xe7, 0xfe, 0x46
    };
    BitIO test{input};


    //get length 0(error)
    bool failed = true;
    try {
        test.movePositionToBeginning();
        test.getBits(0);
    } catch (const std::exception& e) {
        failed = false;
    }
    EXPECT_FALSE(failed);

    //get length >64(error)
    failed = true;
    try {
        test.movePositionToBeginning();
        test.getBits(65);
    } catch (const std::exception& e) {
        failed = false;
    }
    EXPECT_FALSE(failed);

    //get length 1 at beginning of long
    test.movePositionToBeginning();
    EXPECT_EQ(test.getBits(1), 1);
    EXPECT_EQ(test.getPosition(), 1);

    //get length 4 at beginning of long
    test.movePositionToBeginning();
    EXPECT_EQ(test.getBits(4), 0xc);
    EXPECT_EQ(test.getPosition(), 4);

    //get length 8 at beginning of long
    test.movePositionToBeginning();
    EXPECT_EQ(test.getBits(8), 0xc5);
    EXPECT_EQ(test.getPosition(), 8);

    //get length 15 at beginning of long
    test.movePositionToBeginning();
    EXPECT_EQ(test.getBits(15), 0x62d1);
    EXPECT_EQ(test.getPosition(), 15);

    //get length 64 at beginning of long
    test.movePositionToBeginning();
    EXPECT_EQ(test.getBits(64), 0xc5a3c3ef0c52da9c);
    EXPECT_EQ(test.getPosition(), 64);

    //get length 1 at middle of long
    test.movePositionTo(8);
    EXPECT_EQ(test.getBits(1), 1);
    EXPECT_EQ(test.getBits(1), 0);
    EXPECT_EQ(test.getPosition(), 10);

    //get length 8 at middle of long
    test.movePositionTo(10);
    EXPECT_EQ(test.getBits(8), 0x8f);
    EXPECT_EQ(test.getPosition(), 18);

    //get length 15 at middle of long
    test.movePositionTo(11);
    EXPECT_EQ(test.getBits(15), 0xf0f);
    EXPECT_EQ(test.getPosition(), 26);

    //get length 1 at end of long
    test.movePositionTo(63);
    EXPECT_EQ(test.getBits(1), 0);
    EXPECT_EQ(test.getPosition(), 64);

    //get length 15 at end of long
    test.movePositionTo(49);
    EXPECT_EQ(test.getBits(15), 0x5a9c);
    EXPECT_EQ(test.getPosition(), 64);

    //get length 7 spanning 2 longs
    test.movePositionTo(60);
    EXPECT_EQ(test.getBits(7), 0x62);
    EXPECT_EQ(test.getPosition(), 67);

    //get length 64 spanning 2 longs
    test.movePositionTo(60);
    EXPECT_EQ(test.getBits(64), 0xc5f46fbcf60e7fe4);
    EXPECT_EQ(test.getPosition(), 124);

    //get length 1 from 2nd long
    test.movePositionTo(64);
    EXPECT_EQ(test.getBits(1), 0);
    EXPECT_EQ(test.getBits(1), 1);
    EXPECT_EQ(test.getPosition(), 66);

    //get length 4 from 2nd long
    EXPECT_EQ(test.getBits(4), 0x7);
    EXPECT_EQ(test.getPosition(), 70);

    //get length 8 from 2nd long
    EXPECT_EQ(test.getBits(8), 0xd1);
    EXPECT_EQ(test.getPosition(), 78);

    //get length 15 from 2nd long
    EXPECT_EQ(test.getBits(15), 0x5f79);
    EXPECT_EQ(test.getPosition(), 93);

    //get length 64 from 2nd long
    test.movePositionTo(64);
    EXPECT_EQ(test.getBits(64), 0x5f46fbcf60e7fe46);
    EXPECT_EQ(test.getPosition(), 128);

    //get bits that don't exist
    failed = true;
    try {
        test.movePositionToEnd();
        test.getBits(2);
    } catch (const std::exception& e) {
        failed = false;
    }
    EXPECT_FALSE(failed);
}

TEST(BitIO, CheckBits) {
    //build test object
    std::vector<uint8_t> input{
            0xc5, 0xa3, 0xc3, 0xef, 0x0c, 0x52, 0xda, 0x9c,
            0x5f, 0x46, 0xfb, 0xcf, 0x60, 0xe7, 0xfe, 0x46
    };
    BitIO test{input};


    //check length 0(error)
    bool failed = true;
    try {
        test.movePositionToBeginning();
        test.checkBits(0, 0);
    } catch (const std::exception& e) {
        failed = false;
    }
    EXPECT_FALSE(failed);

    //get length 1 at beginning of long
    test.movePositionToBeginning();
    EXPECT_TRUE(test.checkBits(1, 1));
    EXPECT_FALSE(test.checkBits(0, 1));
    EXPECT_EQ(test.getPosition(), 0);

    //get length 4 at beginning of long
    EXPECT_TRUE(test.checkBits(0xc, 4));
    EXPECT_FALSE(test.checkBits(0xf, 4));
    EXPECT_EQ(test.getPosition(), 0);

    //get length 8 at beginning of long
    EXPECT_TRUE(test.checkBits(0xc5, 8));
    EXPECT_FALSE(test.checkBits(0xcf, 8));
    EXPECT_EQ(test.getPosition(), 0);

    //get length 15 at beginning of long
    EXPECT_TRUE(test.checkBits(0x62d1, 15));
    EXPECT_FALSE(test.checkBits(0xcf21, 15));
    EXPECT_EQ(test.getPosition(), 0);

    //get length 64 at beginning of long
    EXPECT_TRUE(test.checkBits(0xc5a3c3ef0c52da9c, 64));
    EXPECT_FALSE(test.checkBits(0xc5a3c3ef0c52da91, 64));
    EXPECT_EQ(test.getPosition(), 0);

    //get length 1 at middle of long
    test.movePositionTo(8);
    EXPECT_TRUE(test.checkBits(1, 1));
    EXPECT_EQ(test.getPosition(), 8);

    //get length 8 at middle of long
    test.movePositionTo(10);
    EXPECT_TRUE(test.checkBits(0x8f, 8));
    EXPECT_EQ(test.getPosition(), 10);

    //get length 15 at middle of long
    test.movePositionTo(11);
    EXPECT_TRUE(test.checkBits(0xf0f, 15));
    EXPECT_EQ(test.getPosition(), 11);

    //get length 1 at end of long
    test.movePositionTo(63);
    EXPECT_TRUE(test.checkBits(0, 1));
    EXPECT_EQ(test.getPosition(), 63);

    //get length 15 at end of long
    test.movePositionTo(49);
    EXPECT_TRUE(test.checkBits(0x5a9c, 15));
    EXPECT_EQ(test.getPosition(), 49);

    //get length 7 spanning 2 longs
    test.movePositionTo(60);
    EXPECT_TRUE(test.checkBits(0x62, 7));
    EXPECT_EQ(test.getPosition(), 60);

    //get length 64 spanning 2 longs
    test.movePositionTo(60);
    EXPECT_TRUE(test.checkBits(0xc5f46fbcf60e7fe4, 64));
    EXPECT_EQ(test.getPosition(), 60);

    //get length 64 from 2nd long
    test.movePositionTo(64);
    EXPECT_TRUE(test.checkBits(0x5f46fbcf60e7fe46, 64));
    EXPECT_EQ(test.getPosition(), 64);

    //test with check value that is too long
    test.movePositionTo(60);
    EXPECT_FALSE(test.checkBits(0x82, 7));

    //test checking with not enough bits
    test.movePositionTo(120);
    EXPECT_FALSE(test.checkBits(0x0, 20));
    EXPECT_FALSE(test.checkBits(0x4a09, 20));

    //test checking at end
    test.movePositionToEnd();
    EXPECT_FALSE(test.checkBits(0x0, 5));
    EXPECT_FALSE(test.checkBits(0x3, 5));
}

TEST(BitIO, InsertBits) {
    //build test object
    std::vector<uint8_t> input{
            0xaf, 0x79, 0x70, 0xb0, 0xc0, 0xc4, 0xd4, 0x8a,
            0x7b, 0xa7, 0x6c, 0x6f, 0x29, 0x1a, 0x00, 0x6b
    };
    BitIO test{input};

    test.movePositionToBeginning();
    EXPECT_EQ(test.getBits(64), 0xaf7970b0c0c4d48a);
    EXPECT_EQ(test.getBits(64), 0x7ba76c6f291a006b);

    //insert length 0 at beginning(error)
    bool failed = true;
    try {
        test.movePositionToBeginning();
        test.insertBits(0, 0);
    } catch (const std::exception& e) {
        failed = false;
    }
    EXPECT_FALSE(failed);
    EXPECT_EQ(test.getPosition(), 0);
    EXPECT_EQ(test.getLength(), 128);
    EXPECT_EQ(test.getBits(64), 0xaf7970b0c0c4d48a);
    EXPECT_EQ(test.getBits(64), 0x7ba76c6f291a006b);

    //insert length 1 at beginning of long
    test.movePositionToBeginning();
    test.insertBits(1, 1);
    EXPECT_EQ(test.getPosition(), 1);
    EXPECT_EQ(test.getLength(), 129);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getBits(64), 0xd7bcb85860626a45);
    EXPECT_EQ(test.getBits(64), 0x3dd3b637948d0035);
    EXPECT_EQ(test.getBits(1), 0x1);

    //insert length 4 at beginning of long
    test.movePositionToBeginning();
    test.insertBits(0x9, 4);
    EXPECT_EQ(test.getPosition(), 4);
    EXPECT_EQ(test.getLength(), 133);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getBits(64), 0x9d7bcb85860626a4);
    EXPECT_EQ(test.getBits(64), 0x53dd3b637948d003);
    EXPECT_EQ(test.getBits(5), 0xb);

    //insert length 8 at beginning of long
    test.movePositionToBeginning();
    test.insertBits(0x37, 8);
    EXPECT_EQ(test.getPosition(), 8);
    EXPECT_EQ(test.getLength(), 141);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getBits(64), 0x379d7bcb85860626);
    EXPECT_EQ(test.getBits(64), 0xa453dd3b637948d0);
    EXPECT_EQ(test.getBits(13), 0x6b);

    //insert length 11 at beginning of long
    test.movePositionToBeginning();
    test.insertBits(0x097, 11);
    EXPECT_EQ(test.getPosition(), 11);
    EXPECT_EQ(test.getLength(), 152);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getBits(64), 0x12e6f3af7970b0c0);
    EXPECT_EQ(test.getBits(64), 0xc4d48a7ba76c6f29);
    EXPECT_EQ(test.getBits(24), 0x1a006b);

    //insert length 64 at beginning of long
    test.movePositionToBeginning();
    test.insertBits(0xda9fe72a02824bc8, 64);
    EXPECT_EQ(test.getPosition(), 64);
    EXPECT_EQ(test.getLength(), 216);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getBits(64), 0xda9fe72a02824bc8);
    EXPECT_EQ(test.getBits(64), 0x12e6f3af7970b0c0);
    EXPECT_EQ(test.getBits(64), 0xc4d48a7ba76c6f29);
    EXPECT_EQ(test.getBits(24), 0x1a006b);

    //insert length 1 at middle of long
    test.movePositionTo(8);
    test.insertBits(1, 1);
    EXPECT_EQ(test.getPosition(), 9);
    EXPECT_EQ(test.getLength(), 217);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getBits(64), 0xdacff395014125e4);
    EXPECT_EQ(test.getBits(64), 0x097379d7bcb85860);
    EXPECT_EQ(test.getBits(64), 0x626a453dd3b63794);
    EXPECT_EQ(test.getBits(25), 0x11a006b);

    //insert length 8 at middle of long
    test.movePositionTo(10);
    test.insertBits(0xdc, 8);
    EXPECT_EQ(test.getPosition(), 18);
    EXPECT_EQ(test.getLength(), 225);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getBits(64), 0xdaf70ff395014125);
    EXPECT_EQ(test.getBits(64), 0xe4097379d7bcb858);
    EXPECT_EQ(test.getBits(64), 0x60626a453dd3b637);
    EXPECT_EQ(test.getBits(33), 0x1291a006b);

    //insert length 11 at middle of long
    test.movePositionTo(15);
    test.insertBits(0x618, 11);
    EXPECT_EQ(test.getPosition(), 26);
    EXPECT_EQ(test.getLength(), 236);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getBits(64), 0xdaf78621fe72a028);
    EXPECT_EQ(test.getBits(64), 0x24bc812e6f3af797);
    EXPECT_EQ(test.getBits(64), 0x0b0c0c4d48a7ba76);
    EXPECT_EQ(test.getBits(44), 0xc6f291a006b);

    //insert length 1 at end of long(0)
    test.movePositionTo(63);
    test.insertBits(0, 1);
    EXPECT_EQ(test.getPosition(), 64);
    EXPECT_EQ(test.getLength(), 237);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getBits(64), 0xdaf78621fe72a028);
    EXPECT_EQ(test.getBits(64), 0x125e4097379d7bcb);
    EXPECT_EQ(test.getBits(64), 0x85860626a453dd3b);
    EXPECT_EQ(test.getBits(45), 0xc6f291a006b);

    //insert length 1 at end of long(1)
    test.movePositionTo(63);
    test.insertBits(1, 1);
    EXPECT_EQ(test.getPosition(), 64);
    EXPECT_EQ(test.getLength(), 238);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getBits(64), 0xdaf78621fe72a029);
    EXPECT_EQ(test.getBits(64), 0x092f204b9bcebde5);
    EXPECT_EQ(test.getBits(64), 0xc2c303135229ee9d);
    EXPECT_EQ(test.getBits(46), 0x2c6f291a006b);

    //insert length 12 near end
    test.movePositionTo(49);
    test.insertBits(0x60b, 12);
    EXPECT_EQ(test.getPosition(), 61);
    EXPECT_EQ(test.getLength(), 250);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getBits(64), 0xdaf78621fe72b05a);
    EXPECT_EQ(test.getBits(64), 0x029092f204b9bceb);
    EXPECT_EQ(test.getBits(64), 0xde5c2c303135229e);
    EXPECT_EQ(test.getBits(58), 0x3a76c6f291a006b);

    //insert length 7 spanning 2 longs
    test.movePositionTo(60);
    test.insertBits(0x4e, 7);
    EXPECT_EQ(test.getPosition(), 67);
    EXPECT_EQ(test.getLength(), 257);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getBits(64), 0xdaf78621fe72b059);
    EXPECT_EQ(test.getBits(64), 0xd4052125e4097379);
    EXPECT_EQ(test.getBits(64), 0xd7bcb85860626a45);
    EXPECT_EQ(test.getBits(64), 0x3dd3b637948d0035);
    EXPECT_EQ(test.getBits(1), 0x1);

    //insert length 64 spanning 2 longs
    test.movePositionTo(60);
    test.insertBits(0x57135e86c5294200, 64);
    EXPECT_EQ(test.getPosition(), 124);
    EXPECT_EQ(test.getLength(), 321);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getBits(64), 0xdaf78621fe72b055);
    EXPECT_EQ(test.getBits(64), 0x7135e86c52942009);
    EXPECT_EQ(test.getBits(64), 0xd4052125e4097379);
    EXPECT_EQ(test.getBits(64), 0xd7bcb85860626a45);
    EXPECT_EQ(test.getBits(64), 0x3dd3b637948d0035);
    EXPECT_EQ(test.getBits(1), 0x1);

    //insert length 1 into 2nd long
    test.movePositionTo(64);
    test.insertBits(0x1, 1);
    EXPECT_EQ(test.getPosition(), 65);
    EXPECT_EQ(test.getLength(), 322);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getBits(64), 0xdaf78621fe72b055);
    EXPECT_EQ(test.getBits(64), 0xb89af436294a1004);
    EXPECT_EQ(test.getBits(64), 0xea029092f204b9bc);
    EXPECT_EQ(test.getBits(64), 0xebde5c2c30313522);
    EXPECT_EQ(test.getBits(64), 0x9ee9db1bca46801a);
    EXPECT_EQ(test.getBits(2), 0x3);

    //insert length(62) that causes final length to align with a long
    test.movePositionTo(9);
    test.insertBits(0x3fa820624b7cee06, 62);
    EXPECT_EQ(test.getPosition(), 71);
    EXPECT_EQ(test.getLength(), 384);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getBits(64), 0xdaff5040c496f9dc);
    EXPECT_EQ(test.getBits(64), 0x0dde1887f9cac156);
    EXPECT_EQ(test.getBits(64), 0xe26bd0d8a5284013);
    EXPECT_EQ(test.getBits(64), 0xa80a424bc812e6f3);
    EXPECT_EQ(test.getBits(64), 0xaf7970b0c0c4d48a);
    EXPECT_EQ(test.getBits(64), 0x7ba76c6f291a006b);

    //insert write at the end that creates new long
    test.movePositionToEnd();
    test.insertBits(0x65, 7);
    EXPECT_EQ(test.getPosition(), 391);
    EXPECT_EQ(test.getLength(), 391);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getBits(64), 0xdaff5040c496f9dc);
    EXPECT_EQ(test.getBits(64), 0x0dde1887f9cac156);
    EXPECT_EQ(test.getBits(64), 0xe26bd0d8a5284013);
    EXPECT_EQ(test.getBits(64), 0xa80a424bc812e6f3);
    EXPECT_EQ(test.getBits(64), 0xaf7970b0c0c4d48a);
    EXPECT_EQ(test.getBits(64), 0x7ba76c6f291a006b);
    EXPECT_EQ(test.getBits(7), 0x65);

    //insert write at the end that does not create new long
    test.movePositionToEnd();
    test.insertBits(0xd, 4);
    EXPECT_EQ(test.getPosition(), 395);
    EXPECT_EQ(test.getLength(), 395);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getBits(64), 0xdaff5040c496f9dc);
    EXPECT_EQ(test.getBits(64), 0x0dde1887f9cac156);
    EXPECT_EQ(test.getBits(64), 0xe26bd0d8a5284013);
    EXPECT_EQ(test.getBits(64), 0xa80a424bc812e6f3);
    EXPECT_EQ(test.getBits(64), 0xaf7970b0c0c4d48a);
    EXPECT_EQ(test.getBits(64), 0x7ba76c6f291a006b);
    EXPECT_EQ(test.getBits(11), 0x65d);

    //insert short BitIO object at beginning
    BitIO test2;
    test2.appendBits(0x26c, 10);
    test2.movePositionTo(2);
    test.movePositionToBeginning();
    test.insertBits(test2);
    EXPECT_EQ(test2.getPosition(), 2);   //shouldn't move position on inserted
    EXPECT_EQ(test2.getLength(), 10);    //shouldn't have changed length of inserted
    test2.movePositionToBeginning();
    EXPECT_EQ(test2.getBits(10), 0x26c); //shouldn't have changed value of inserted
    test.movePositionToBeginning();
    EXPECT_EQ(test.getBits(64), 0x9b36bfd4103125be);
    EXPECT_EQ(test.getBits(64), 0x7703778621fe72b0);
    EXPECT_EQ(test.getBits(64), 0x55b89af436294a10);
    EXPECT_EQ(test.getBits(64), 0x04ea029092f204b9);
    EXPECT_EQ(test.getBits(64), 0xbcebde5c2c303135);
    EXPECT_EQ(test.getBits(64), 0x229ee9db1bca4680);
    EXPECT_EQ(test.getBits(21), 0x35e5d);


    //insert short BitIO object in middle
    test.movePositionTo(70);
    test.insertBits(test2);
    EXPECT_EQ(test.getPosition(), 80);
    EXPECT_EQ(test.getLength(), 415);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getBits(64), 0x9b36bfd4103125be);
    EXPECT_EQ(test.getBits(64), 0x766cc0dde1887f9c);
    EXPECT_EQ(test.getBits(64), 0xac156e26bd0d8a52);
    EXPECT_EQ(test.getBits(64), 0x84013a80a424bc81);
    EXPECT_EQ(test.getBits(64), 0x2e6f3af7970b0c0c);
    EXPECT_EQ(test.getBits(64), 0x4d48a7ba76c6f291);
    EXPECT_EQ(test.getBits(31), 0x50035e5d);

    //insert short BitIO object at end
    test.movePositionToEnd();
    test.insertBits(test2);
    EXPECT_EQ(test.getPosition(), 425);
    EXPECT_EQ(test.getLength(), 425);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getBits(64), 0x9b36bfd4103125be);
    EXPECT_EQ(test.getBits(64), 0x766cc0dde1887f9c);
    EXPECT_EQ(test.getBits(64), 0xac156e26bd0d8a52);
    EXPECT_EQ(test.getBits(64), 0x84013a80a424bc81);
    EXPECT_EQ(test.getBits(64), 0x2e6f3af7970b0c0c);
    EXPECT_EQ(test.getBits(64), 0x4d48a7ba76c6f291);
    EXPECT_EQ(test.getBits(41), 0x1400d79766c);

    //insert long BitIO object
    test2.appendBits(0x64590161955b350e, 64);
    test2.appendBits(0x029565e533b28d77, 64);
    test2.appendBits(0x5bf16b9bacb2f176, 64);
    test.movePositionTo(90);
    test.insertBits(test2);
    EXPECT_EQ(test.getPosition(), 292);
    EXPECT_EQ(test.getLength(), 627);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getBits(64), 0x9b36bfd4103125be);
    EXPECT_EQ(test.getBits(64), 0x766cc0e6c6459016);
    EXPECT_EQ(test.getBits(64), 0x1955b350e029565e);
    EXPECT_EQ(test.getBits(64), 0x533b28d775bf16b9);
    EXPECT_EQ(test.getBits(64), 0xbacb2f176778621f);
    EXPECT_EQ(test.getBits(64), 0xe72b055b89af4362);
    EXPECT_EQ(test.getBits(64), 0x94a1004ea029092f);
    EXPECT_EQ(test.getBits(64), 0x204b9bcebde5c2c3);
    EXPECT_EQ(test.getBits(64), 0x03135229ee9db1bc);
    EXPECT_EQ(test.getBits(51), 0x523400d79766c);
}

TEST(BitIO, SetBits) {
    //build test object
    std::vector<uint8_t> input{
            0xaf, 0x79, 0x70, 0xb0, 0xc0, 0xc4, 0xd4, 0x8a,
            0x7b, 0xa7, 0x6c, 0x6f, 0x29, 0x1a, 0x00, 0x6b
    };
    BitIO test{input};

    //set length 0 at beginning(error)
    bool failed = true;
    try {
        test.movePositionToBeginning();
        test.insertBits(0, 0);
    } catch (const std::exception& e) {
        failed = false;
    }
    EXPECT_FALSE(failed);
    EXPECT_EQ(test.getPosition(), 0);
    EXPECT_EQ(test.getLength(), 128);
    EXPECT_EQ(test.getBits(64), 0xaf7970b0c0c4d48a);
    EXPECT_EQ(test.getBits(64), 0x7ba76c6f291a006b);

    //set length 1 at beginning of long(1)
    test.movePositionToBeginning();
    test.setBits(1, 1);
    EXPECT_EQ(test.getPosition(), 1);
    EXPECT_EQ(test.getLength(), 128);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getBits(64), 0xaf7970b0c0c4d48a);
    EXPECT_EQ(test.getBits(64), 0x7ba76c6f291a006b);

    //set length 1 at beginning of long(0)
    test.movePositionToBeginning();
    test.setBits(0, 1);
    EXPECT_EQ(test.getPosition(), 1);
    EXPECT_EQ(test.getLength(), 128);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getBits(64), 0x2f7970b0c0c4d48a);
    EXPECT_EQ(test.getBits(64), 0x7ba76c6f291a006b);

    //set length 4 at beginning of long
    test.movePositionToBeginning();
    test.setBits(0x9, 4);
    EXPECT_EQ(test.getPosition(), 4);
    EXPECT_EQ(test.getLength(), 128);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getBits(64), 0x9f7970b0c0c4d48a);
    EXPECT_EQ(test.getBits(64), 0x7ba76c6f291a006b);

    //set length 8 at beginning of long
    test.movePositionToBeginning();
    test.setBits(0x37, 8);
    EXPECT_EQ(test.getPosition(), 8);
    EXPECT_EQ(test.getLength(), 128);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getBits(64), 0x377970b0c0c4d48a);
    EXPECT_EQ(test.getBits(64), 0x7ba76c6f291a006b);

    //set length 11 at beginning of long
    test.movePositionToBeginning();
    test.setBits(0x097, 11);
    EXPECT_EQ(test.getPosition(), 11);
    EXPECT_EQ(test.getLength(), 128);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getBits(64), 0x12f970b0c0c4d48a);
    EXPECT_EQ(test.getBits(64), 0x7ba76c6f291a006b);

    //set length 64 at beginning of long
    test.movePositionToBeginning();
    test.setBits(0xda9fe72a02824bc8, 64);
    EXPECT_EQ(test.getPosition(), 64);
    EXPECT_EQ(test.getLength(), 128);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getBits(64), 0xda9fe72a02824bc8);
    EXPECT_EQ(test.getBits(64), 0x7ba76c6f291a006b);

    //set length 1 at middle of long
    test.movePositionTo(8);
    test.setBits(0, 1);
    EXPECT_EQ(test.getPosition(), 9);
    EXPECT_EQ(test.getLength(), 128);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getBits(64), 0xda1fe72a02824bc8);
    EXPECT_EQ(test.getBits(64), 0x7ba76c6f291a006b);

    //set length 8 at middle of long
    test.movePositionTo(10);
    test.setBits(0xdc, 8);
    EXPECT_EQ(test.getPosition(), 18);
    EXPECT_EQ(test.getLength(), 128);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getBits(64), 0xda37272a02824bc8);
    EXPECT_EQ(test.getBits(64), 0x7ba76c6f291a006b);

    //set length 11 at middle of long
    test.movePositionTo(15);
    test.setBits(0x618, 11);
    EXPECT_EQ(test.getPosition(), 26);
    EXPECT_EQ(test.getLength(), 128);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getBits(64), 0xda37862a02824bc8);
    EXPECT_EQ(test.getBits(64), 0x7ba76c6f291a006b);

    //set length 1 at end of long(0)
    test.movePositionTo(63);
    test.setBits(0, 1);
    EXPECT_EQ(test.getPosition(), 64);
    EXPECT_EQ(test.getLength(), 128);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getBits(64), 0xda37862a02824bc8);
    EXPECT_EQ(test.getBits(64), 0x7ba76c6f291a006b);

    //set length 1 at end of long(1)
    test.movePositionTo(63);
    test.setBits(1, 1);
    EXPECT_EQ(test.getPosition(), 64);
    EXPECT_EQ(test.getLength(), 128);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getBits(64), 0xda37862a02824bc9);
    EXPECT_EQ(test.getBits(64), 0x7ba76c6f291a006b);

    //set length 15 at end of long
    test.movePositionTo(49);
    test.setBits(0x60b, 15);
    EXPECT_EQ(test.getPosition(), 64);
    EXPECT_EQ(test.getLength(), 128);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getBits(64), 0xda37862a0282060b);
    EXPECT_EQ(test.getBits(64), 0x7ba76c6f291a006b);

    //set length 7 spanning 2 longs
    test.movePositionTo(60);
    test.setBits(0x4e, 7);
    EXPECT_EQ(test.getPosition(), 67);
    EXPECT_EQ(test.getLength(), 128);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getBits(64), 0xda37862a02820609);
    EXPECT_EQ(test.getBits(64), 0xdba76c6f291a006b);

    //set length 64 spanning 2 longs
    test.movePositionTo(60);
    test.setBits(0x57135e86c5294200, 64);
    EXPECT_EQ(test.getPosition(), 124);
    EXPECT_EQ(test.getLength(), 128);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getBits(64), 0xda37862a02820605);
    EXPECT_EQ(test.getBits(64), 0x7135e86c5294200b);

    //set length 1 into 2nd long
    test.movePositionTo(64);
    test.setBits(0x1, 1);
    EXPECT_EQ(test.getPosition(), 65);
    EXPECT_EQ(test.getLength(), 128);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getBits(64), 0xda37862a02820605);
    EXPECT_EQ(test.getBits(64), 0xf135e86c5294200b);

    //set length 8 that extends the data
    test.movePositionTo(124);
    test.setBits(0xe4, 8);
    EXPECT_EQ(test.getPosition(), 132);
    EXPECT_EQ(test.getLength(), 132);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getBits(64), 0xda37862a02820605);
    EXPECT_EQ(test.getBits(64), 0xf135e86c5294200e);
    EXPECT_EQ(test.getBits(4), 0x4);

    //set length at end that extends the data and causes final length to align with a long
    test.movePositionToEnd();
    test.insertBits(0xfa820624b7cee06, 60);
    EXPECT_EQ(test.getPosition(), 192);
    EXPECT_EQ(test.getLength(), 192);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getBits(64), 0xda37862a02820605);
    EXPECT_EQ(test.getBits(64), 0xf135e86c5294200e);
    EXPECT_EQ(test.getBits(64), 0x4fa820624b7cee06);

    //set length 12 at end of long that creates a new long
    test.movePositionToEnd();
    test.setBits(0xa65, 12);
    EXPECT_EQ(test.getPosition(), 204);
    EXPECT_EQ(test.getLength(), 204);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getBits(64), 0xda37862a02820605);
    EXPECT_EQ(test.getBits(64), 0xf135e86c5294200e);
    EXPECT_EQ(test.getBits(64), 0x4fa820624b7cee06);
    EXPECT_EQ(test.getBits(12), 0xa65);

    //set write at the end that does not create new long
    test.movePositionToEnd();
    test.setBits(0xd, 4);
    EXPECT_EQ(test.getPosition(), 208);
    EXPECT_EQ(test.getLength(), 208);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getBits(64), 0xda37862a02820605);
    EXPECT_EQ(test.getBits(64), 0xf135e86c5294200e);
    EXPECT_EQ(test.getBits(64), 0x4fa820624b7cee06);
    EXPECT_EQ(test.getBits(16), 0xa65d);

    //set short BitIO object at beginning
    BitIO test2;
    test2.appendBits(0x26c, 10);
    test2.movePositionTo(2);
    test.movePositionToBeginning();
    test.setBits(test2);
    EXPECT_EQ(test2.getPosition(), 2);   //shouldn't move position on inserted
    EXPECT_EQ(test2.getLength(), 10);    //shouldn't have changed length of inserted
    test2.movePositionToBeginning();
    EXPECT_EQ(test2.getBits(10), 0x26c); //shouldn't have changed value of inserted
    test.movePositionToBeginning();
    EXPECT_EQ(test.getBits(64), 0x9b37862a02820605);
    EXPECT_EQ(test.getBits(64), 0xf135e86c5294200e);
    EXPECT_EQ(test.getBits(64), 0x4fa820624b7cee06);
    EXPECT_EQ(test.getBits(16), 0xa65d);

    //set short BitIO object in middle
    test.movePositionTo(90);
    test.setBits(test2);
    EXPECT_EQ(test.getPosition(), 100);
    EXPECT_EQ(test.getLength(), 208);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getBits(64), 0x9b37862a02820605);
    EXPECT_EQ(test.getBits(64), 0xf135e866c294200e);
    EXPECT_EQ(test.getBits(64), 0x4fa820624b7cee06);
    EXPECT_EQ(test.getBits(16), 0xa65d);

    //set short BitIO object at end
    test.movePositionToEnd();
    test.setBits(test2);
    EXPECT_EQ(test.getPosition(), 218);
    EXPECT_EQ(test.getLength(), 218);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getBits(64), 0x9b37862a02820605);
    EXPECT_EQ(test.getBits(64), 0xf135e866c294200e);
    EXPECT_EQ(test.getBits(64), 0x4fa820624b7cee06);
    EXPECT_EQ(test.getBits(26), 0x299766c);

    //set long BitIO object that extends the object
    test2.appendBits(0x64590161955b350e, 64);
    test2.appendBits(0x029565e533b28d77, 64);
    test2.appendBits(0x5bf16b9bacb2f176, 64);
    test.movePositionTo(90);
    test.setBits(test2);
    EXPECT_EQ(test.getPosition(), 292);
    EXPECT_EQ(test.getLength(), 292);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getBits(64), 0x9b37862a02820605);
    EXPECT_EQ(test.getBits(64), 0xf135e866c6459016);
    EXPECT_EQ(test.getBits(64), 0x1955b350e029565e);
    EXPECT_EQ(test.getBits(64), 0x533b28d775bf16b9);
    EXPECT_EQ(test.getBits(36), 0xbacb2f176);

    //set long BitIO object that does not extend the object
    test.movePositionTo(54);
    test.setBits(test2);
    EXPECT_EQ(test.getPosition(), 256);
    EXPECT_EQ(test.getLength(), 292);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getBits(64), 0x9b37862a0282066c);
    EXPECT_EQ(test.getBits(64), 0x64590161955b350e);
    EXPECT_EQ(test.getBits(64), 0x029565e533b28d77);
    EXPECT_EQ(test.getBits(64), 0x5bf16b9bacb2f176);
    EXPECT_EQ(test.getBits(36), 0xbacb2f176);
}

TEST(BitIO, AppendBits) {
    //append 1 bite
    BitIO test;
    test.movePositionToBeginning();
    test.appendBits(0, 1);
    EXPECT_EQ(test.getPosition(), 0);
    EXPECT_EQ(test.getLength(), 1);
    EXPECT_EQ(test.getBits(1), 0x0);

    //append 0 bits(error)
    test.movePositionToBeginning();
    bool failed = true;
    try {
        test.insertBits(0, 0);
    } catch (const std::exception& e) {
        failed = false;
    }
    EXPECT_FALSE(failed);
    EXPECT_EQ(test.getPosition(), 0);
    EXPECT_EQ(test.getLength(), 1);

    //append 64 bits(spanning 2 longs)
    test.movePositionToBeginning();
    test.appendBits(0xaedff895ef4fd00b, 64);
    EXPECT_EQ(test.getPosition(), 0);
    EXPECT_EQ(test.getLength(), 65);
    EXPECT_EQ(test.getBits(64), 0x576ffc4af7a7e805);
    EXPECT_EQ(test.getBits(1), 0x1);

    //append bits(63) to line up with long
    test.movePositionToBeginning();
    test.appendBits(0x0c2c57c4ae94806f, 63);
    EXPECT_EQ(test.getPosition(), 0);
    EXPECT_EQ(test.getLength(), 128);
    EXPECT_EQ(test.getBits(64), 0x576ffc4af7a7e805);
    EXPECT_EQ(test.getBits(64), 0x8c2c57c4ae94806f);

    //append 64 bits(not spanning 2 longs)
    test.movePositionToBeginning();
    test.appendBits(0x6ff8dfee3f754a96, 64);
    EXPECT_EQ(test.getPosition(), 0);
    EXPECT_EQ(test.getLength(), 192);
    EXPECT_EQ(test.getBits(64), 0x576ffc4af7a7e805);
    EXPECT_EQ(test.getBits(64), 0x8c2c57c4ae94806f);
    EXPECT_EQ(test.getBits(64), 0x6ff8dfee3f754a96);

    //append 15 bits
    test.movePositionToBeginning();
    test.appendBits(0x39c4, 15);
    EXPECT_EQ(test.getPosition(), 0);
    EXPECT_EQ(test.getLength(), 207);
    EXPECT_EQ(test.getBits(64), 0x576ffc4af7a7e805);
    EXPECT_EQ(test.getBits(64), 0x8c2c57c4ae94806f);
    EXPECT_EQ(test.getBits(64), 0x6ff8dfee3f754a96);
    EXPECT_EQ(test.getBits(15), 0x39c4);

    //append 60 bits(spanning 2 longs)
    test.movePositionToBeginning();
    test.appendBits(0x155da5d2e6dd792, 60);
    EXPECT_EQ(test.getPosition(), 0);
    EXPECT_EQ(test.getLength(), 267);
    EXPECT_EQ(test.getBits(64), 0x576ffc4af7a7e805);
    EXPECT_EQ(test.getBits(64), 0x8c2c57c4ae94806f);
    EXPECT_EQ(test.getBits(64), 0x6ff8dfee3f754a96);
    EXPECT_EQ(test.getBits(64), 0x73882abb4ba5cdba);
    EXPECT_EQ(test.getBits(11), 0x792);

    //append short BitIO object
    BitIO test2;
    test2.appendBits(0x26c, 10);
    test2.movePositionTo(2);
    test.movePositionToBeginning();
    test.appendBits(test2);
    EXPECT_EQ(test2.getPosition(), 2);   //shouldn't move position on inserted
    EXPECT_EQ(test2.getLength(), 10);    //shouldn't have changed length of inserted
    test2.movePositionToBeginning();
    EXPECT_EQ(test2.getBits(10), 0x26c); //shouldn't have changed value of inserted
    test.movePositionToBeginning();
    EXPECT_EQ(test.getBits(64), 0x576ffc4af7a7e805);
    EXPECT_EQ(test.getBits(64), 0x8c2c57c4ae94806f);
    EXPECT_EQ(test.getBits(64), 0x6ff8dfee3f754a96);
    EXPECT_EQ(test.getBits(64), 0x73882abb4ba5cdba);
    EXPECT_EQ(test.getBits(21), 0x1e4a6c);

    //append long BitIO object
    test2.appendBits(0x64590161955b350e, 64);
    test2.appendBits(0x029565e533b28d77, 64);
    test2.appendBits(0x5bf16b9bacb2f176, 64);
    test.movePositionToBeginning();
    test.appendBits(test2);
    EXPECT_EQ(test.getPosition(), 0);
    EXPECT_EQ(test.getLength(), 479);
    EXPECT_EQ(test.getBits(64), 0x576ffc4af7a7e805);
    EXPECT_EQ(test.getBits(64), 0x8c2c57c4ae94806f);
    EXPECT_EQ(test.getBits(64), 0x6ff8dfee3f754a96);
    EXPECT_EQ(test.getBits(64), 0x73882abb4ba5cdba);
    EXPECT_EQ(test.getBits(31), 0x7929b26c);
    EXPECT_EQ(test.getBits(64), 0x64590161955b350e);
    EXPECT_EQ(test.getBits(64), 0x029565e533b28d77);
    EXPECT_EQ(test.getBits(64), 0x5bf16b9bacb2f176);
}

TEST(BitIO, CopyBits) {
    //build test object
    std::vector<uint8_t> input{
            0xaf, 0x79, 0x70, 0xb0, 0xc0, 0xc4, 0xd4, 0x8a,
            0x7b, 0xa7, 0x6c, 0x6f, 0x29, 0x1a, 0x00, 0x6b
    };
    BitIO test{input};

    test.movePositionToBeginning();
    EXPECT_EQ(test.getBits(64), 0xaf7970b0c0c4d48a);
    EXPECT_EQ(test.getBits(64), 0x7ba76c6f291a006b);

    //copy bits from beginning of long
    test.movePositionToBeginning();
    BitIO testVal = test.copyBits(16);
    EXPECT_EQ(testVal.getLength(), 16);
    EXPECT_EQ(testVal.getBits(16), 0xaf79);

    //copy bits from middle of long
    test.movePositionTo(20);
    testVal = test.copyBits(12);
    EXPECT_EQ(testVal.getLength(), 12);
    EXPECT_EQ(testVal.getBits(12), 0x0b0);

    //copy bits from end of long
    test.movePositionTo(52);
    testVal = test.copyBits(12);
    EXPECT_EQ(testVal.getLength(), 12);
    EXPECT_EQ(testVal.getBits(12), 0x48a);

    //copy full long
    test.movePositionToBeginning();
    testVal = test.copyBits(64);
    EXPECT_EQ(testVal.getLength(), 64);
    EXPECT_EQ(testVal.getBits(64), 0xaf7970b0c0c4d48a);

    //copy bits spanning 2 longs
    test.movePositionTo(52);
    testVal = test.copyBits(24);
    EXPECT_EQ(testVal.getLength(), 24);
    EXPECT_EQ(testVal.getBits(24), 0x48a7ba);

    //copy more than 1 long lining up with longs
    test.movePositionToBeginning();
    testVal = test.copyBits(128);
    EXPECT_EQ(testVal.getLength(), 128);
    EXPECT_EQ(testVal.getBits(64), 0xaf7970b0c0c4d48a);
    EXPECT_EQ(testVal.getBits(64), 0x7ba76c6f291a006b);

    //copy more than 1 long not lining up with longs
    test.movePositionTo(10);
    testVal = test.copyBits(100);
    EXPECT_EQ(testVal.getLength(), 100);
    EXPECT_EQ(testVal.getBits(64), 0xe5c2c303135229ee);
    EXPECT_EQ(testVal.getBits(36), 0x9db1bca46);

    //copy bits past the end(error)
    test.movePositionTo(30);
    bool failed = true;
    try {
        test.copyBits(1000);
    } catch (const std::exception& e) {
        failed = false;
    }
    EXPECT_FALSE(failed);
}

TEST(BitIO, PadWidth) {
    //build test object
    std::vector<uint8_t> input{
            0xaf, 0x79, 0x70, 0xb0, 0xc0, 0xc4, 0xd4, 0x8a,
            0x7b, 0xa7, 0x6c, 0x6f, 0x29, 0x1a, 0x00, 0x6b
    };
    BitIO test{input};

    test.movePositionToBeginning();
    EXPECT_EQ(test.getBits(64), 0xaf7970b0c0c4d48a);
    EXPECT_EQ(test.getBits(64), 0x7ba76c6f291a006b);

    //pad(zero) multiple of 7 bits with 5 left to fill
    test.movePositionToBeginning();
    test.padWidth(7);
    EXPECT_EQ(test.getPosition(), 0);
    EXPECT_EQ(test.getLength(), 133);
    EXPECT_EQ(test.getBits(64), 0xaf7970b0c0c4d48a);
    EXPECT_EQ(test.getBits(64), 0x7ba76c6f291a006b);
    EXPECT_EQ(test.getBits(5), 0);

    //pad(one) multiple of 8 bits with 3 left to fill
    test.movePositionToBeginning();
    test.padWidth(8, BITIO_FILL_ONES);
    EXPECT_EQ(test.getPosition(), 0);
    EXPECT_EQ(test.getLength(), 136);
    EXPECT_EQ(test.getBits(64), 0xaf7970b0c0c4d48a);
    EXPECT_EQ(test.getBits(64), 0x7ba76c6f291a006b);
    EXPECT_EQ(test.getBits(8), 0x7);

    //pad multiple of 8 bits with 0 left to fill
    test.movePositionToBeginning();
    test.padWidth();
    EXPECT_EQ(test.getPosition(), 0);
    EXPECT_EQ(test.getLength(), 136);
    EXPECT_EQ(test.getBits(64), 0xaf7970b0c0c4d48a);
    EXPECT_EQ(test.getBits(64), 0x7ba76c6f291a006b);
    EXPECT_EQ(test.getBits(8), 0x7);

    //pad(random) multiple of 10 bits with 4 left to fill
    test.movePositionToBeginning();
    test.padWidth(25, BITIO_FILL_RANDOM);
    EXPECT_EQ(test.getPosition(), 0);
    EXPECT_EQ(test.getLength(), 150);
    EXPECT_EQ(test.getBits(64), 0xaf7970b0c0c4d48a);
    EXPECT_EQ(test.getBits(64), 0x7ba76c6f291a006b);
    EXPECT_EQ(test.getBits(8), 0x7);
    uint64_t testVal = test.getBits(14);
    EXPECT_GT(testVal, 0);   //not likely all 0s
    EXPECT_LT(testVal, 0x3fff);//not likely all 1s

    //pad(zero) multiple of 74 bits with 72 left to fill
    test.movePositionToBeginning();
    test.padWidth(74);
    EXPECT_EQ(test.getPosition(), 0);
    EXPECT_EQ(test.getLength(), 222);
    EXPECT_EQ(test.getBits(64), 0xaf7970b0c0c4d48a);
    EXPECT_EQ(test.getBits(64), 0x7ba76c6f291a006b);
    EXPECT_EQ(test.getBits(8), 0x7);
    test.movePositionBy(14);    //skip random
    EXPECT_EQ(test.getBits(64), 0x0);
    EXPECT_EQ(test.getBits(8), 0x0);

    //pad(one) multiple of 100 bits with 78 left to fill
    test.movePositionToBeginning();
    test.padWidth(100, BITIO_FILL_ONES);
    EXPECT_EQ(test.getPosition(), 0);
    EXPECT_EQ(test.getLength(), 300);
    EXPECT_EQ(test.getBits(64), 0xaf7970b0c0c4d48a);
    EXPECT_EQ(test.getBits(64), 0x7ba76c6f291a006b);
    EXPECT_EQ(test.getBits(8), 0x7);
    test.movePositionBy(14);    //skip random
    EXPECT_EQ(test.getBits(64), 0x0);
    EXPECT_EQ(test.getBits(64), 0x00ffffffffffffff);
    EXPECT_EQ(test.getBits(22), 0x3fffff);

    //pad(random) multiple of 500 bits with 200 left to fill
    test.movePositionToBeginning();
    test.padWidth(500, BITIO_FILL_RANDOM);
    EXPECT_EQ(test.getPosition(), 0);
    EXPECT_EQ(test.getLength(), 500);
    EXPECT_EQ(test.getBits(64), 0xaf7970b0c0c4d48a);
    EXPECT_EQ(test.getBits(64), 0x7ba76c6f291a006b);
    EXPECT_EQ(test.getBits(8), 0x7);
    test.movePositionBy(14);    //skip random
    EXPECT_EQ(test.getBits(64), 0x0);
    EXPECT_EQ(test.getBits(64), 0x00ffffffffffffff);
    EXPECT_EQ(test.getBits(22), 0x3fffff);
    for (size_t i = 0; i < 4; i++) {
        uint64_t value = test.getBits(50);
        EXPECT_GT(value, 0);
        EXPECT_LT(value, 0x3ffffffffffff);
    }

    //pad multiple of 1(makes no changes)
    test.padWidth(1);
    EXPECT_EQ(test.getLength(), 500);
}


TEST(BitIO, AppendZeros) {
    //append when nothing there
    BitIO test;
    test.appendZeros(4);
    EXPECT_EQ(test.getLength(), 4);
    EXPECT_EQ(test.getBits(4), 0);

    //append more than 64 bits
    test.appendZeros(80);
    EXPECT_EQ(test.getLength(), 84);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getBits(64), 0);
    EXPECT_EQ(test.getBits(20), 0);

    //lets put some none zero values in and append more zeros
    test.appendBits(0x387, 10);
    test.appendZeros(34);
    EXPECT_EQ(test.getLength(), 128);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getBits(64), 0);
    EXPECT_EQ(test.getBits(64), 0xe1c00000000);
}





/*
 * ░██████╗████████╗██████╗░██╗███╗░░██╗░██████╗░░██████╗
 * ██╔════╝╚══██╔══╝██╔══██╗██║████╗░██║██╔════╝░██╔════╝
 * ╚█████╗░░░░██║░░░██████╔╝██║██╔██╗██║██║░░██╗░╚█████╗░
 * ░╚═══██╗░░░██║░░░██╔══██╗██║██║╚████║██║░░╚██╗░╚═══██╗
 * ██████╔╝░░░██║░░░██║░░██║██║██║░╚███║╚██████╔╝██████╔╝
 * ╚═════╝░░░░╚═╝░░░╚═╝░░╚═╝╚═╝╚═╝░░╚══╝░╚═════╝░╚═════╝░
 */

TEST(BitIO, MakeAlphaString) {
    //example found on https://www.thonky.com/qr-code-tutorial/alphanumeric-mode-encoding to make sure matches QR code standards(except we use lower case)
    BitIO test = BitIO::makeAlphaString("he");
    test.movePositionToBeginning();
    EXPECT_EQ(test.getLength(), 11);
    EXPECT_EQ(test.getBits(11), 0x30b);

    //long string
    test = BitIO::makeAlphaString("mctrivia rocks");
    test.movePositionToBeginning();
    EXPECT_EQ(test.getLength(), 77);
    EXPECT_EQ(test.getBits(64), 0x7d54d1a4b34cdf11);
    EXPECT_EQ(test.getBits(13), 0x3a0);

    //use extreme character limits
    test = BitIO::makeAlphaString("::");
    test.movePositionToBeginning();
    EXPECT_EQ(test.getLength(), 11);
    EXPECT_EQ(test.getBits(11), 0x7e8);
    test = BitIO::makeAlphaString(":");
    test.movePositionToBeginning();
    EXPECT_EQ(test.getLength(), 6);
    EXPECT_EQ(test.getBits(6), 0x2c);
    test = BitIO::makeAlphaString("0");
    test.movePositionToBeginning();
    EXPECT_EQ(test.getLength(), 6);
    EXPECT_EQ(test.getBits(6), 0x0);
}

TEST(BitIO, GetAlphaString) {
    //long string
    vector<uint8_t> initialValue{0x7d, 0x54, 0xd1, 0xa4, 0xb3, 0x4c, 0xdf, 0x11, 0x1d, 0x00};
    BitIO test(initialValue, 77);
    EXPECT_EQ(test.getAlphaString(14), "mctrivia rocks");

    //use extreme character limits
    initialValue = {0xfd, 0x16, 0x00};
    BitIO test2(initialValue, 17);
    EXPECT_EQ(test2.getAlphaString(3), ":::");
}

TEST(BitIO, MakeUTF8String) {

    //test with 3 byte characters
    BitIO test = BitIO::makeUTF8String("こんにちは");
    /*
     * condensed format replaces original utf8 headers with the following(https://en.wikipedia.org/wiki/UTF-8 for more info on UTF8)
     * 1 byte starts with 0 followed by 7 bits
     * 2 bytes starts with 10 followed by 11 bits
     * 3 bytes start with 110 followed by 16 bits
     * 4 bytes starts with 111 followed by 21 bits
     *
     *     xxxx   xxxxxx   xxxxxx           condensed
     * 11100011 10000001 10010011           1100011000001010011
     * 11100011 10000010 10010011           1100011000010010011
     * 11100011 10000001 10101011           1100011000001101011
     * 11100011 10000001 10100001           1100011000001100001
     * 11100011 10000001 10101111           1100011000001101111
     *
     * expected binary output:
     * 1100011000001010011110001100001001001111000110000011010111100011
     * 0000011000011100011000001101111
     */
    EXPECT_EQ(test.getLength(), 95);
    EXPECT_EQ(test.getBits(64), 0xc60a78c24f1835e3);
    EXPECT_EQ(test.getBits(31), 0x30e306f);

    //test with 2 byte characters
    test = BitIO::makeUTF8String(u8"سلام");
    /*
     *    xxxxx   xxxxxx            condensed
     * 11011000 10110011            1011000110011
     * 11011001 10000100            1011001000100
     * 11011000 10100111            1011000100111
     * 11011001 10000101            1011001000101
     *
     * expected binary output:
     * 1011000110011101100100010010110001001111011001000101
     */
    EXPECT_EQ(test.getLength(), 52);
    EXPECT_EQ(test.getBits(52), 0xb19d912c4f645);

    //test with 1 and 2 byte characters
    test = BitIO::makeUTF8String(u8"Γειά σου");
    /*                              condensed
     * 11001110 10010011            1001110010011
     * 11001110 10110101            1001110110101
     * 11001110 10111001            1001110111001
     * 11001110 10101100            1001110101100
     * 00100000                     00100000
     * 11001111 10000011            1001111000011
     * 11001110 10111111            1001110111111
     * 11001111 10000101            1001111000101
     *
     * expected binary output:
     * 1001110010011100111011010110011101110011001110101100001000001001
     * 11100001110011101111111001111000101
     */
    EXPECT_EQ(test.getLength(), 99);
    EXPECT_EQ(test.getBits(64), 0x9c9ced67733ac209);
    EXPECT_EQ(test.getBits(35), 0x70e77f3c5);

    //test 1 to 4 byte characters
    test = BitIO::makeUTF8String(u8"🌈📈 cooL éठ");
    /*                                          condensed
     * 11110000 10011111 10001100 10001000      111000011111001100001000
     * 11110000 10011111 10010011 10001000      111000011111010011001000
     * 00100000                                 00100000
     * 01100011                                 01100011
     * 01101111                                 01101111
     * 01101111                                 01101111
     * 01001100                                 01001100
     * 00100000                                 00100000
     * 11000011 10101001                        1000011101001
     * 11100000 10100100 10100000               1100000100100100000
     *
     * expected binary output:
     * 1110000111110011000010001110000111110100110010000010000001100011
     * 0110111101101111010011000010000010000111010011100000100100100000
     */
    EXPECT_EQ(test.getLength(), 128);
    EXPECT_EQ(test.getBits(64), 0xe1f308e1f4c82063);
    EXPECT_EQ(test.getBits(64), 0x6f6f4c20874e0920);

    //test with empty string
    test = BitIO::makeUTF8String("");
    EXPECT_EQ(test.getLength(), 0);
}

TEST(BitIO, GetUTF8String) {
    //test with 3 byte characters
    BitIO test;
    test.appendBits(0xc60a78c24f1835e3, 64);
    test.appendBits(0x30e306f, 31);
    EXPECT_EQ(test.getUTF8String(5), u8"こんにちは");

    //test with 2 byte characters
    test = BitIO();
    test.appendBits(0xb19d912c4f645, 52);
    EXPECT_EQ(test.getUTF8String(4), u8"سلام");

    //test with 1 and 2 byte characters
    test = BitIO();
    test.appendBits(0x9c9ced67733ac209, 64);
    test.appendBits(0x70e77f3c5, 35);
    EXPECT_EQ(test.getUTF8String(8), u8"Γειά σου");

    //test 1 to 4 byte characters
    test = BitIO();
    test.appendBits(0xe1f308e1f4c82063, 64);
    test.appendBits(0x6f6f4c20874e0920, 64);
    EXPECT_EQ(test.getUTF8String(10), u8"🌈📈 cooL éठ");

    //test with empty string
    test = BitIO();
    EXPECT_EQ(test.getUTF8String(0), "");
}

TEST(BitIO, MakeHexString) {
    //make 1 long of hex
    BitIO test = BitIO::makeHexString("cfd827f2d343771b");
    EXPECT_EQ(test.getLength(), 64);
    EXPECT_EQ(test.getBits(64), 0xcfd827f2d343771b);

    //make 1 byte of hex
    test = BitIO::makeHexString("f4");
    EXPECT_EQ(test.getLength(), 8);
    EXPECT_EQ(test.getBits(8), 0xf4);

    //make 1.5 long
    test = BitIO::makeHexString("aaff5998e8aab40769295413");
    EXPECT_EQ(test.getLength(), 96);
    EXPECT_EQ(test.getBits(64), 0xaaff5998e8aab407);
    EXPECT_EQ(test.getBits(32), 0x69295413);

    //make invalid string
    bool failed = true;
    try {
        BitIO::makeHexString("aaff5998e8gab40769295413"); //there is a "g" in the string
    } catch (const std::exception& e) {
        failed = false;
    }
    EXPECT_FALSE(failed);
}

TEST(BitIO, GetHexString) {
    //make 1 long of hex
    vector<uint8_t> initValue{0xcf, 0xd8, 0x27, 0xf2, 0xd3, 0x43, 0x77, 0x1b, 0xf4, 0xaa, 0xff, 0x59, 0x98, 0xe8, 0xaa,
                              0xb4, 0x07, 0x69, 0x29, 0x54, 0x13};
    BitIO test(initValue);
    EXPECT_EQ(test.getHexString(16), "cfd827f2d343771b");

    //get 1 byte of hex
    EXPECT_EQ(test.getHexString(2), "f4");

    //get 1.5 long
    EXPECT_EQ(test.getHexString(24), "aaff5998e8aab40769295413");
}

TEST(BitIO, Make3B40String) {
    //common file extensions
    BitIO test = BitIO::make3B40String("pdf");
    uint64_t val = test.getBits(test.getLength());
    EXPECT_EQ(test.getLength(), 16);
    EXPECT_EQ(val, 0x9e57);
    test = BitIO::make3B40String("jpg");
    val = test.getBits(test.getLength());
    EXPECT_EQ(test.getLength(), 16);
    EXPECT_EQ(val, 0x7ab8);
    test = BitIO::make3B40String("txt");
    val = test.getBits(test.getLength());
    EXPECT_EQ(test.getLength(), 16);
    EXPECT_EQ(val, 0xba85);
    test = BitIO::make3B40String("bmp");
    val = test.getBits(test.getLength());
    EXPECT_EQ(test.getLength(), 16);
    EXPECT_EQ(val, 0x4849);
    test = BitIO::make3B40String("gif");
    val = test.getBits(test.getLength());
    EXPECT_EQ(test.getLength(), 16);
    EXPECT_EQ(val, 0x66df);
    test = BitIO::make3B40String("png");
    val = test.getBits(test.getLength());
    EXPECT_EQ(test.getLength(), 16);
    EXPECT_EQ(val, 0x9fe8);
    test = BitIO::make3B40String("zip");
    val = test.getBits(test.getLength());
    EXPECT_EQ(test.getLength(), 16);
    EXPECT_EQ(val, 0xdda9);
    test = BitIO::make3B40String("avi");
    val = test.getBits(test.getLength());
    EXPECT_EQ(test.getLength(), 16);
    EXPECT_EQ(val, 0x436a);
    test = BitIO::make3B40String("tiff");
    val = test.getBits(test.getLength());
    EXPECT_EQ(test.getLength(), 21);
    EXPECT_EQ(val, 0x1703ef);
    test = BitIO::make3B40String("rtf");
    val = test.getBits(test.getLength());
    EXPECT_EQ(test.getLength(), 16);
    EXPECT_EQ(val, 0xad57);
    test = BitIO::make3B40String("psd");
    val = test.getBits(test.getLength());
    EXPECT_EQ(test.getLength(), 16);
    EXPECT_EQ(val, 0xa0ad);
    test = BitIO::make3B40String("mp4");
    val = test.getBits(test.getLength());
    EXPECT_EQ(test.getLength(), 16);
    EXPECT_EQ(val, 0x8d6c);
    test = BitIO::make3B40String("wav");
    val = test.getBits(test.getLength());
    EXPECT_EQ(test.getLength(), 16);
    EXPECT_EQ(val, 0xc9af);
    test = BitIO::make3B40String("mov");
    val = test.getBits(test.getLength());
    EXPECT_EQ(test.getLength(), 16);
    EXPECT_EQ(val, 0x8d5f);
    test = BitIO::make3B40String("svg");
    val = test.getBits(test.getLength());
    EXPECT_EQ(test.getLength(), 16);
    EXPECT_EQ(val, 0xb3e8);
    test = BitIO::make3B40String("mp3");
    val = test.getBits(test.getLength());
    EXPECT_EQ(test.getLength(), 16);
    EXPECT_EQ(val, 0x8d6b);
    test = BitIO::make3B40String("7z");
    val = test.getBits(test.getLength());
    EXPECT_EQ(test.getLength(), 11);
    EXPECT_EQ(val, 0x13b);
    test = BitIO::make3B40String("obj");
    val = test.getBits(test.getLength());
    EXPECT_EQ(test.getLength(), 16);
    EXPECT_EQ(val, 0x97cb);
    test = BitIO::make3B40String("xlsm");
    val = test.getBits(test.getLength());
    EXPECT_EQ(test.getLength(), 21);
    EXPECT_EQ(val, 0x1a3496);
    test = BitIO::make3B40String("...");
    val = test.getBits(test.getLength());
    EXPECT_EQ(test.getLength(), 16);
    EXPECT_EQ(val, 0xf9ff);

    //make invalid string
    bool failed = true;
    try {
        BitIO::makeHexString("fu_"); //_ is not valid in this format
    } catch (const std::exception& e) {
        failed = false;
    }
    EXPECT_FALSE(failed);
}

TEST(BitIO, Get3B40String) {
    vector<uint8_t> testVals = {0x9e, 0x57, 0x7a, 0xb8, 0xba, 0x85, 0x48, 0x49,
                                0x66, 0xdf, 0x9f, 0xe8, 0xdd, 0xa9, 0x43, 0x6a,
                                0xad, 0x57, 0xa0, 0xad, 0x8d, 0x6c, 0xc9, 0xaf,
                                0x8d, 0x5f, 0xb3, 0xe8, 0x8d, 0x6b, 0x97, 0xcb,
                                0xb8, 0x1f, 0x79, 0x3b, 0xd1, 0xa4, 0xb0, 0xf9, 0xff};
    BitIO test(testVals);

    EXPECT_EQ(test.get3B40String(3), "pdf");
    EXPECT_EQ(test.get3B40String(3), "jpg");
    EXPECT_EQ(test.get3B40String(3), "txt");
    EXPECT_EQ(test.get3B40String(3), "bmp");
    EXPECT_EQ(test.get3B40String(3), "gif");
    EXPECT_EQ(test.get3B40String(3), "png");
    EXPECT_EQ(test.get3B40String(3), "zip");
    EXPECT_EQ(test.get3B40String(3), "avi");
    EXPECT_EQ(test.get3B40String(3), "rtf");
    EXPECT_EQ(test.get3B40String(3), "psd");
    EXPECT_EQ(test.get3B40String(3), "mp4");
    EXPECT_EQ(test.get3B40String(3), "wav");
    EXPECT_EQ(test.get3B40String(3), "mov");
    EXPECT_EQ(test.get3B40String(3), "svg");
    EXPECT_EQ(test.get3B40String(3), "mp3");
    EXPECT_EQ(test.get3B40String(3), "obj");
    EXPECT_EQ(test.get3B40String(4), "tiff");
    EXPECT_EQ(test.get3B40String(2), "7z");
    EXPECT_EQ(test.get3B40String(4), "xlsm");
    test.movePositionBy(3);
    EXPECT_EQ(test.get3B40String(3), "...");
}

TEST(BitIO, MakeBestString) {
    //should pick 3b40
    BitIO test = BitIO::makeBestString("exe", 5);
    EXPECT_EQ(test.getLength(), 23);
    EXPECT_EQ(test.getBits(23), 0x435cb6);

    //alpha:01 00011 01010010111001110
    //3b40: 10 00011 0101110010110110
    //utf8: 11 00011 011001010111100001100101
    //hex:  error
    //3b40 wins

    //should throw an error(to few length bits)
    bool failed = true;
    try {
        test = BitIO::makeBestString("https://digiassetx.com", 3);
    } catch (const std::exception& e) {
        failed = false;
    }
    EXPECT_FALSE(failed);

    //let's try again with a good length bit should pick alpha
    test = BitIO::makeBestString("https://digiassetx.com", 6);
    EXPECT_EQ(test.getLength(), 129);
    EXPECT_EQ(test.getBits(64), 0x566354ca8c7ba4b6);
    EXPECT_EQ(test.getBits(64), 0xb88ef4faa75dba27);
    EXPECT_EQ(test.getBits(1), 0);

    //alpha:01 010110 0110001101010100110010101000110001111011101001001011011010111000100011101111010011111010101001110101110110111010001001110
    //3b40: error
    //utf8: 11 010110 01101000011101000111010001110000011100110011101000101111001011110110010001101001011001110110100101100001011100110111001101100101011101000111100000101110011000110110111101101101
    //hex:  error
    //alpha wins

    //should pick utf8
    test = BitIO::makeBestString(u8"आपके शांति के साथ रहें", 16);
    EXPECT_EQ(test.getLength(), 392);
    EXPECT_EQ(test.getBits(64), 0xc005b04836092ac1);
    EXPECT_EQ(test.getBits(64), 0x22B8251C83049B60);
    EXPECT_EQ(test.getBits(64), 0x93EC120582493049);
    EXPECT_EQ(test.getBits(64), 0xF9060915C128E418);
    EXPECT_EQ(test.getBits(64), 0x24E3049F6092520C);
    EXPECT_EQ(test.getBits(64), 0x1261824e704a3e09);
    EXPECT_EQ(test.getBits(8), 0x02);

    //should pick hex
    test = BitIO::makeBestString("1261824e", 6);
    EXPECT_EQ(test.getLength(), 42);
    EXPECT_EQ(test.getBits(42), 0x481261824e);

    //provide custom header options.  would have been hex but we didn't includ
    test = BitIO::makeBestString("1261824e", 6, {
            {{0, 1, 2}, BITIO_ENCODE_ALPHA},
            {{0, 2, 2}, BITIO_ENCODE_3B40},
            {{0, 3, 2}, BITIO_ENCODE_UTF8}
    });
    EXPECT_EQ(test.getLength(), 51);
    EXPECT_EQ(test.getBits(51), 0x44034b03c10ae);

    //provide custom header options with unknown encoder
    failed = true;
    try {
        test = BitIO::makeBestString("1261824e", 6, {
                {{0, 1, 2}, BITIO_ENCODE_ALPHA},
                {{0, 2, 2}, BITIO_ENCODE_3B40},
                {{0, 3, 2}, 5}
        });
    } catch (const std::exception& e) {
        failed = false;
    }
    EXPECT_FALSE(failed);

    //provide custom header options where value can't be encoded
    failed = true;
    try {
        test = BitIO::makeBestString("CAPS", 6, {
                {{0, 1, 2}, BITIO_ENCODE_ALPHA},
                {{0, 2, 2}, BITIO_ENCODE_3B40}
        });
    } catch (const std::exception& e) {
        failed = false;
    }
    EXPECT_FALSE(failed);
}

TEST(BitIO, GetBestString) {
    //decode 3b40
    BitIO test;
    test.appendBits(0x435cb6, 23);
    EXPECT_EQ(test.getBestString(5), "exe");

    //decode alpha
    test = BitIO();
    test.appendBits(0x566354ca8c7ba4b6, 64);
    test.appendBits(0xb88ef4faa75dba27, 64);
    test.appendBits(0, 1);
    EXPECT_EQ(test.getBestString(6), "https://digiassetx.com");

    //decode utf8
    test = BitIO();
    test.appendBits(0xc005b04836092ac1, 64);
    test.appendBits(0x22B8251C83049B60, 64);
    test.appendBits(0x93EC120582493049, 64);
    test.appendBits(0xF9060915C128E418, 64);
    test.appendBits(0x24E3049F6092520C, 64);
    test.appendBits(0x1261824e704a3e09, 64);
    test.appendBits(0x02, 8);
    EXPECT_EQ(test.getBestString(16), u8"आपके शांति के साथ रहें");

    //decode hex
    test = BitIO();
    test.appendBits(0x481261824e, 42);
    EXPECT_EQ(test.getBestString(6), "1261824e");

    //decode string with unknown encoder(depth 1,key 3)
    bool failed = true;
    test = BitIO();
    test.appendBits(0x0370, 16);
    try {
        test.getBestString(6);
    } catch (const std::exception& e) {
        failed = false;
    }
    EXPECT_FALSE(failed);
}





/*
 * ███╗░░██╗██╗░░░██╗███╗░░░███╗██████╗░███████╗██████╗░░██████╗
 * ████╗░██║██║░░░██║████╗░████║██╔══██╗██╔════╝██╔══██╗██╔════╝
 * ██╔██╗██║██║░░░██║██╔████╔██║██████╦╝█████╗░░██████╔╝╚█████╗░
 * ██║╚████║██║░░░██║██║╚██╔╝██║██╔══██╗██╔══╝░░██╔══██╗░╚═══██╗
 * ██║░╚███║╚██████╔╝██║░╚═╝░██║██████╦╝███████╗██║░░██║██████╔╝
 * ╚═╝░░╚══╝░╚═════╝░╚═╝░░░░░╚═╝╚═════╝░╚══════╝╚═╝░░╚═╝╚═════╝░
 */


TEST(BitIO, MakeFixedPrecision) {
    //test 0
    BitIO test = BitIO::makeFixedPrecision(0);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getLength(), 8);
    EXPECT_EQ(test.getBits(8), 0);

    //test small value that should be 1 byte
    test = BitIO::makeFixedPrecision(19);
    EXPECT_EQ(test.getLength(), 8);
    EXPECT_EQ(test.getBits(8), 0x13);

    //test value that can be compressed
    test = BitIO::makeFixedPrecision(5004000);
    EXPECT_EQ(test.getLength(), 24);
    EXPECT_EQ(test.getBits(24), 0x4138c3);

    //test largest value that can be compressed by most
    test = BitIO::makeFixedPrecision((uint64_t) 1000000000000000);
    EXPECT_EQ(test.getLength(), 16);
    EXPECT_EQ(test.getBits(16), 0x201f);


    //test max value
    test = BitIO::makeFixedPrecision((uint64_t) 18014398509481983);
    EXPECT_EQ(test.getLength(), 56);
    EXPECT_EQ(test.getBits(56), 0xffffffffffffff);

    //test 1 over max
    bool failed = true;
    try {
        test = BitIO::makeFixedPrecision((uint64_t) 18014398509481984);
    } catch (const std::exception& e) {
        failed = false;
    }
    EXPECT_FALSE(failed);

    //test large number that has more 0s than we can compress
    test = BitIO::makeFixedPrecision((uint64_t) 3355443200000000);
    EXPECT_EQ(test.getLength(), 40);
    EXPECT_EQ(test.getBits(40), 0x80a0000007);

    //test number with 6 byte output
    test = BitIO::makeFixedPrecision((uint64_t) 1800000000200000);
    EXPECT_EQ(test.getLength(), 48);
    EXPECT_EQ(test.getBits(48), 0xa0218711a015);

    //test number with 4 byte output
    test = BitIO::makeFixedPrecision((uint64_t) 3355443100000000);
    EXPECT_EQ(test.getLength(), 32);
    EXPECT_EQ(test.getBits(32), 0x7ffffff8);
}

TEST(BitIO, GetFixedPrecision) {
    //test 0
    vector<uint8_t> testVals = {0x00, 0x13, 0x41, 0x38, 0xc3, 0x20, 0x1f, 0x80, 0x2f, 0xae, 0x67, 0xf0, 0xff,
                                0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80, 0xa0, 0x00, 0x00, 0x07, 0x7f, 0xff,
                                0xff, 0xf8, 0xa0, 0x21, 0x87, 0x11, 0xa0, 0x15};
    BitIO test = BitIO(testVals);
    EXPECT_EQ(test.getFixedPrecision(), 0);
    EXPECT_EQ(test.getFixedPrecision(), 19);
    EXPECT_EQ(test.getFixedPrecision(), 5004000);
    EXPECT_EQ(test.getFixedPrecision(), (uint64_t) 1000000000000000);
    EXPECT_EQ(test.getFixedPrecision(), (uint64_t) 99994878);
    EXPECT_EQ(test.getFixedPrecision(), (uint64_t) 18014398509481983);
    EXPECT_EQ(test.getFixedPrecision(), (uint64_t) 3355443200000000);
    EXPECT_EQ(test.getFixedPrecision(), (uint64_t) 3355443100000000);
    EXPECT_EQ(test.getFixedPrecision(), (uint64_t) 1800000000200000);
}

TEST(BitIO, MakeDouble) {
    //test 1/3
    BitIO io = BitIO::makeDouble(1.0 / 3.0);
    io.movePositionToBeginning();
    EXPECT_EQ(io.getBits(64), 0x555555555555d53f);   //little endian 1/3

    //test 1
    io = BitIO::makeDouble(1.0);
    io.movePositionToBeginning();
    EXPECT_EQ(io.getBits(64), 0xf03f);   //little endian 1

    //test 1.0000000000000002
    io = BitIO::makeDouble(1.0000000000000002);
    io.movePositionToBeginning();
    EXPECT_EQ(io.getBits(64), 0x010000000000f03f);   //little endian 1

    //test 2
    io = BitIO::makeDouble(2.0);
    io.movePositionToBeginning();
    EXPECT_EQ(io.getBits(64), 0x40);   //little endian 2

    //test -2
    io = BitIO::makeDouble(-2.0);
    io.movePositionToBeginning();
    EXPECT_EQ(io.getBits(64), 0xc0);   //little endian -2

    io = BitIO::makeDouble(-0.0000000000000000000000002);

    //test 0
    io = BitIO::makeDouble(0.0);
    io.movePositionToBeginning();
    EXPECT_EQ(io.getBits(64), 0x00);   //little endian 0

    //test -0
    io = BitIO::makeDouble(-0.0);
    io.movePositionToBeginning();
    EXPECT_EQ(io.getBits(64), 0x80);   //little endian -0

    //test infinity
    io = BitIO::makeDouble(INFINITY);
    io.movePositionToBeginning();
    EXPECT_EQ(io.getBits(64), 0xf07f);   //little endian infinity

    //test negative infinity
    io = BitIO::makeDouble(-INFINITY);
    io.movePositionToBeginning();
    EXPECT_EQ(io.getBits(64), 0xf0ff);   //little endian -infinity

    //test 2^-1022
    io = BitIO::makeDouble(pow(2, -1022));
    io.movePositionToBeginning();
    EXPECT_EQ(io.getBits(64), 0x1000);   //little endian 2^-1022

    //test big endian 1/3
    io = BitIO::makeDouble(1.0 / 3.0, false);
    io.movePositionToBeginning();
    EXPECT_EQ(io.getBits(64), 0x3fd5555555555555);   //big endian 1/3

    //test real world value
    io = BitIO::makeDouble(1215786649.6655264);
    io.movePositionToBeginning();
    EXPECT_EQ(io.getBits(64), 0xfc976aa6db1dd241);
}

TEST(BitIO, GetDouble) {
    BitIO io;

    //test 1/3
    io.appendBits(0x555555555555d53f, 64);
    io.movePositionToBeginning();
    double value = io.getDouble();
    EXPECT_FLOAT_EQ(value, 1.0 / 3.0);

    //test 1
    io.movePositionToBeginning();
    io.setBits(0xf03f, 64);
    io.movePositionToBeginning();
    value = io.getDouble();
    EXPECT_FLOAT_EQ(value, 1.0);

    //test 1.0000000000000002
    io.movePositionToBeginning();
    io.setBits(0x010000000000f03f, 64);
    io.movePositionToBeginning();
    value = io.getDouble();
    EXPECT_FLOAT_EQ(value, 1.0000000000000002);

    //test 2
    io.movePositionToBeginning();
    io.setBits(0x40, 64);
    io.movePositionToBeginning();
    value = io.getDouble();
    EXPECT_FLOAT_EQ(value, 2.0);

    //test -2
    io.movePositionToBeginning();
    io.setBits(0xc0, 64);
    io.movePositionToBeginning();
    value = io.getDouble();
    EXPECT_FLOAT_EQ(value, -2.0);

    //test 0
    io.movePositionToBeginning();
    io.setBits(0x00, 64);
    io.movePositionToBeginning();
    value = io.getDouble();
    EXPECT_FLOAT_EQ(value, 0.0);

    //test -0
    io.movePositionToBeginning();
    io.setBits(0x80, 64);
    io.movePositionToBeginning();
    value = io.getDouble();
    EXPECT_FLOAT_EQ(value, -0.0);

    //test infinity
    io.movePositionToBeginning();
    io.setBits(0xf07f, 64);
    io.movePositionToBeginning();
    value = io.getDouble();
    EXPECT_FLOAT_EQ(value, INFINITY);

    //test negative infinity
    io.movePositionToBeginning();
    io.setBits(0xf0ff, 64);
    io.movePositionToBeginning();
    value = io.getDouble();
    EXPECT_FLOAT_EQ(value, -INFINITY);

    //test 2^-1022
    io.movePositionToBeginning();
    io.setBits(0x1000, 64);
    io.movePositionToBeginning();
    value = io.getDouble();
    EXPECT_FLOAT_EQ(value, pow(2, -1022));

    //test big endian 1/3
    io.movePositionToBeginning();
    io.setBits(0x3fd5555555555555, 64);
    io.movePositionToBeginning();
    value = io.getDouble(false);
    EXPECT_FLOAT_EQ(value, 1.0 / 3.0);

    //test real world value
    io.movePositionToBeginning();
    io.setBits(0xfc976aa6db1dd241, 64);
    io.movePositionToBeginning();
    value = io.getDouble();
    EXPECT_FLOAT_EQ(value, 1215786649.6655264);
}




/*
 * █████╗░██╗████████╗░█████╗░░█████╗░██╗███╗░░██╗
 * ██╔══██╗██║╚══██╔══╝██╔══██╗██╔══██╗██║████╗░██║
 * ██████╦╝██║░░░██║░░░██║░░╚═╝██║░░██║██║██╔██╗██║
 * ██╔══██╗██║░░░██║░░░██║░░██╗██║░░██║██║██║╚████║
 * ██████╦╝██║░░░██║░░░╚█████╔╝╚█████╔╝██║██║░╚███║
 * ╚═════╝░╚═╝░░░╚═╝░░░░╚════╝░░╚════╝░╚═╝╚═╝░░╚══╝
 */
TEST(BitIO, MakeBitcoin) {
    //try false
    BitIO test = BitIO::makeBitcoin(false);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getLength(), 8);
    EXPECT_EQ(test.getBits(8), 0);
    test = BitIO::makeBitcoin(false, true);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getLength(), 16);
    EXPECT_EQ(test.getBits(8), 0x6a);
    EXPECT_EQ(test.getBits(8), 0);

    //try true
    test = BitIO::makeBitcoin(true);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getLength(), 8);
    EXPECT_EQ(test.getBits(8), 81);
    test = BitIO::makeBitcoin(true, true);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getLength(), 16);
    EXPECT_EQ(test.getBits(8), 0x6a);
    EXPECT_EQ(test.getBits(8), 81);

    //try -2
    bool failed = true;
    try {
        test = BitIO::makeBitcoin(-2);
    } catch (const std::exception& e) {
        failed = false;
    }
    EXPECT_FALSE(failed);

    //try -1
    test = BitIO::makeBitcoin(-1);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getLength(), 8);
    EXPECT_EQ(test.getBits(8), 79);

    //try 16
    test = BitIO::makeBitcoin(16);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getLength(), 8);
    EXPECT_EQ(test.getBits(8), 96);

    //try 17
    failed = true;
    try {
        test = BitIO::makeBitcoin(17);
    } catch (const std::exception& e) {
        failed = false;
    }
    EXPECT_FALSE(failed);

    //try short binary data
    vector<uint8_t> sampleData0 = {
            0x13, 0xab, 0x68, 0x7a, 0x03, 0x06, 0xa1, 0xbc,
            0xcd, 0x41, 0x88, 0xec, 0x53, 0xaa, 0x8a, 0xb0
    };
    test = BitIO::makeBitcoin(sampleData0);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getLength(), 136);
    EXPECT_EQ(test.getBits(8), 16);
    EXPECT_EQ(test.getBits(64), 0x13ab687a0306a1bc);
    EXPECT_EQ(test.getBits(64), 0xcd4188ec53aa8ab0);
    test = BitIO::makeBitcoin(sampleData0, true);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getLength(), 144);
    EXPECT_EQ(test.getBits(8), 0x6a);
    EXPECT_EQ(test.getBits(8), 16);
    EXPECT_EQ(test.getBits(64), 0x13ab687a0306a1bc);
    EXPECT_EQ(test.getBits(64), 0xcd4188ec53aa8ab0);

    //try medium length data(80 bytes)
    vector<uint8_t> sampleData1 = {
            0x3d, 0x45, 0x19, 0xba, 0x83, 0xd3, 0x52, 0x20,
            0x59, 0x46, 0x37, 0xe8, 0xce, 0x0c, 0xcd, 0x24,
            0x3b, 0xdf, 0xe1, 0xbf, 0xec, 0x4d, 0xa8, 0x3b,
            0x77, 0xe6, 0xa5, 0xb1, 0xaa, 0x18, 0x74, 0x0c,
            0x7d, 0x30, 0xe1, 0x9e, 0x5e, 0xaf, 0x16, 0x7a,
            0x47, 0x36, 0xc3, 0xc1, 0x2a, 0x3e, 0x56, 0xb2,
            0xf4, 0xbe, 0x19, 0x9f, 0x44, 0x4c, 0x36, 0x68,
            0x38, 0x8a, 0x08, 0x7a, 0x16, 0x8c, 0xcf, 0xff,
            0xfe, 0x91, 0x94, 0x6b, 0xd0, 0x79, 0x66, 0x87,
            0x69, 0x36, 0x26, 0x4e, 0xcd, 0xe2, 0x9a, 0x9e
    };
    test = BitIO::makeBitcoin(sampleData1);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getLength(), 656);
    EXPECT_EQ(test.getBits(8), 76);
    EXPECT_EQ(test.getBits(8), 80);
    EXPECT_EQ(test.getBits(64), 0x3d4519ba83d35220);
    EXPECT_EQ(test.getBits(64), 0x594637e8ce0ccd24);
    EXPECT_EQ(test.getBits(64), 0x3bdfe1bfec4da83b);
    EXPECT_EQ(test.getBits(64), 0x77e6a5b1aa18740c);
    EXPECT_EQ(test.getBits(64), 0x7d30e19e5eaf167a);
    EXPECT_EQ(test.getBits(64), 0x4736c3c12a3e56b2);
    EXPECT_EQ(test.getBits(64), 0xf4be199f444c3668);
    EXPECT_EQ(test.getBits(64), 0x388a087a168ccfff);
    EXPECT_EQ(test.getBits(64), 0xfe91946bd0796687);
    EXPECT_EQ(test.getBits(64), 0x6936264ecde29a9e);

    //testing 256 and 65546 byte lengths isn't practical and should work identically.  also it is extremely unlikely that greater than 80 will ever be used.

    //use BitIO data
    test = BitIO(sampleData1);
    BitIO::makeBitcoin(test);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getLength(), 656);
    EXPECT_EQ(test.getBits(8), 76);
    EXPECT_EQ(test.getBits(8), 80);
    EXPECT_EQ(test.getBits(64), 0x3d4519ba83d35220);
    EXPECT_EQ(test.getBits(64), 0x594637e8ce0ccd24);
    EXPECT_EQ(test.getBits(64), 0x3bdfe1bfec4da83b);
    EXPECT_EQ(test.getBits(64), 0x77e6a5b1aa18740c);
    EXPECT_EQ(test.getBits(64), 0x7d30e19e5eaf167a);
    EXPECT_EQ(test.getBits(64), 0x4736c3c12a3e56b2);
    EXPECT_EQ(test.getBits(64), 0xf4be199f444c3668);
    EXPECT_EQ(test.getBits(64), 0x388a087a168ccfff);
    EXPECT_EQ(test.getBits(64), 0xfe91946bd0796687);
    EXPECT_EQ(test.getBits(64), 0x6936264ecde29a9e);
    test = BitIO(sampleData1);
    BitIO::makeBitcoin(test, true);
    test.movePositionToBeginning();
    EXPECT_EQ(test.getLength(), 664);
    EXPECT_EQ(test.getBits(8), 0x6a);
    EXPECT_EQ(test.getBits(8), 76);
    EXPECT_EQ(test.getBits(8), 80);
    EXPECT_EQ(test.getBits(64), 0x3d4519ba83d35220);
    EXPECT_EQ(test.getBits(64), 0x594637e8ce0ccd24);
    EXPECT_EQ(test.getBits(64), 0x3bdfe1bfec4da83b);
    EXPECT_EQ(test.getBits(64), 0x77e6a5b1aa18740c);
    EXPECT_EQ(test.getBits(64), 0x7d30e19e5eaf167a);
    EXPECT_EQ(test.getBits(64), 0x4736c3c12a3e56b2);
    EXPECT_EQ(test.getBits(64), 0xf4be199f444c3668);
    EXPECT_EQ(test.getBits(64), 0x388a087a168ccfff);
    EXPECT_EQ(test.getBits(64), 0xfe91946bd0796687);
    EXPECT_EQ(test.getBits(64), 0x6936264ecde29a9e);

    //try using BitIO data that is not a multiple of 8
    test = BitIO();
    test.appendBits(0x5, 7);
    failed = true;
    try {
        BitIO::makeBitcoin(test);
    } catch (const std::exception& e) {
        failed = false;
    }
    EXPECT_FALSE(failed);

    //try using relitevely short BitIO data
    test = BitIO();
    test.appendBits(0x5490, 16);
    BitIO::makeBitcoin(test);
    EXPECT_EQ(test.getLength(), 24);
    EXPECT_EQ(test.getBits(24), 0x025490);


    //256 byte length and longer not practical to test so will skip


}

TEST(BitIO, CheckIsBitcoinOpReturn) {
    vector<uint8_t> testData = {
            0x00, 0x6a, 0x4c, 0x50,
            0x3d, 0x45, 0x19, 0xba, 0x83, 0xd3, 0x52, 0x20,
            0x59, 0x46, 0x37, 0xe8, 0xce, 0x0c, 0xcd, 0x24,
            0x3b, 0xdf, 0xe1, 0xbf, 0xec, 0x4d, 0xa8, 0x3b,
            0x77, 0xe6, 0xa5, 0xb1, 0xaa, 0x18, 0x74, 0x0c,
            0x7d, 0x30, 0xe1, 0x9e, 0x5e, 0xaf, 0x16, 0x7a,
            0x47, 0x36, 0xc3, 0xc1, 0x2a, 0x3e, 0x56, 0xb2,
            0xf4, 0xbe, 0x19, 0x9f, 0x44, 0x4c, 0x36, 0x68,
            0x38, 0x8a, 0x08, 0x7a, 0x16, 0x8c, 0xcf, 0xff,
            0xfe, 0x91, 0x94, 0x6b, 0xd0, 0x79, 0x66, 0x87,
            0x69, 0x36, 0x26, 0x4e, 0xcd, 0xe2, 0x9a, 0x9e
    };
    BitIO test = BitIO(testData);

    //try checking for OP_RETURN when not an OP_RETURN
    EXPECT_EQ(test.getPosition(), 0);
    EXPECT_FALSE(test.checkIsBitcoinOpReturn());
    EXPECT_EQ(test.getPosition(), 0);

    //check when is OP_RETURN
    test.movePositionTo(8);
    EXPECT_EQ(test.getPosition(), 8);
    EXPECT_TRUE(test.checkIsBitcoinOpReturn());
    EXPECT_EQ(test.getPosition(), 16);

    //test failure do to not enough bits
    test = BitIO();
    test.appendBits(0x5, 7);
    EXPECT_FALSE(test.checkIsBitcoinOpReturn());
    EXPECT_EQ(test.getPosition(), 0);
}

TEST(BitIO, GetBitcoinDataHeader) {
    vector<uint8_t> testData = {
            0x00, 0x4f, 0x51, 0x4c, 0x50,
            0x3d, 0x45, 0x19, 0xba, 0x83, 0xd3, 0x52, 0x20,
            0x59, 0x46, 0x37, 0xe8, 0xce, 0x0c, 0xcd, 0x24,
            0x3b, 0xdf, 0xe1, 0xbf, 0xec, 0x4d, 0xa8, 0x3b,
            0x77, 0xe6, 0xa5, 0xb1, 0xaa, 0x18, 0x74, 0x0c,
            0x7d, 0x30, 0xe1, 0x9e, 0x5e, 0xaf, 0x16, 0x7a,
            0x47, 0x36, 0xc3, 0xc1, 0x2a, 0x3e, 0x56, 0xb2,
            0xf4, 0xbe, 0x19, 0x9f, 0x44, 0x4c, 0x36, 0x68,
            0x38, 0x8a, 0x08, 0x7a, 0x16, 0x8c, 0xcf, 0xff,
            0xfe, 0x91, 0x94, 0x6b, 0xd0, 0x79, 0x66, 0x87,
            0x69, 0x36, 0x26, 0x4e, 0xcd, 0xe2, 0x9a, 0x9e
    };
    BitIO test = BitIO(testData);

    //check integer results
    EXPECT_EQ(test.getPosition(), 0);
    EXPECT_EQ(test.getBitcoinDataHeader(), 0);
    EXPECT_EQ(test.getPosition(), 8);
    EXPECT_EQ(test.getBitcoinDataHeader(), -1);
    EXPECT_EQ(test.getPosition(), 16);
    EXPECT_EQ(test.getBitcoinDataHeader(), 1);
    EXPECT_EQ(test.getPosition(), 24);

    //check binary data
    EXPECT_EQ(test.getBitcoinDataHeader(), BITIO_BITCOIN_TYPE_DATA);
    EXPECT_EQ(test.getPosition(), 24);

    //check invalid op codes
    bool failed = true;
    test.movePositionTo(32);    //move to an 80 in data
    try {
        test.getBitcoinDataHeader();
    } catch (const std::exception& e) {
        failed = false;
    }
    EXPECT_FALSE(failed);
    test.movePositionTo(64);    //move to a 97 in data
    try {
        test.getBitcoinDataHeader();
    } catch (const std::exception& e) {
        failed = false;
    }
    EXPECT_FALSE(failed);
}

TEST(BitIO, GetBitcoinData) {
    vector<uint8_t> testData = {
            0x4c, 0x50,
            0x3d, 0x45, 0x19, 0xba, 0x83, 0xd3, 0x52, 0x20,
            0x59, 0x46, 0x37, 0xe8, 0xce, 0x0c, 0xcd, 0x24,
            0x3b, 0xdf, 0xe1, 0xbf, 0xec, 0x4d, 0xa8, 0x3b,
            0x77, 0xe6, 0xa5, 0xb1, 0xaa, 0x18, 0x74, 0x0c,
            0x7d, 0x30, 0xe1, 0x9e, 0x5e, 0xaf, 0x16, 0x7a,
            0x47, 0x36, 0xc3, 0xc1, 0x2a, 0x3e, 0x56, 0xb2,
            0xf4, 0xbe, 0x19, 0x9f, 0x44, 0x4c, 0x36, 0x68,
            0x38, 0x8a, 0x08, 0x7a, 0x16, 0x8c, 0xcf, 0xff,
            0xfe, 0x91, 0x94, 0x6b, 0xd0, 0x79, 0x66, 0x87,
            0x69, 0x36, 0x26, 0x4e, 0xcd, 0xe2, 0x9a, 0x9e
    };
    BitIO test = BitIO(testData);

    //decode bitcoin data
    vector<uint8_t> bitcoinData = test.getBitcoinData();
    for (size_t i = 0; i < 80; i++) {
        EXPECT_EQ(bitcoinData[i], testData[i + 2]);
    }

    //test with invalid op_code
    bool failed = true;
    test.movePositionTo(8);    //move to an 80 in data
    try {
        test.getBitcoinData();
    } catch (const std::exception& e) {
        failed = false;
    }
    EXPECT_FALSE(failed);
}

TEST(BitIO, CopyBitcoinData) {
    vector<uint8_t> testData = {
            0x4c, 0x50,
            0x3d, 0x45, 0x19, 0xba, 0x83, 0xd3, 0x52, 0x20,
            0x59, 0x46, 0x37, 0xe8, 0xce, 0x0c, 0xcd, 0x24,
            0x3b, 0xdf, 0xe1, 0xbf, 0xec, 0x4d, 0xa8, 0x3b,
            0x77, 0xe6, 0xa5, 0xb1, 0xaa, 0x18, 0x74, 0x0c,
            0x7d, 0x30, 0xe1, 0x9e, 0x5e, 0xaf, 0x16, 0x7a,
            0x47, 0x36, 0xc3, 0xc1, 0x2a, 0x3e, 0x56, 0xb2,
            0xf4, 0xbe, 0x19, 0x9f, 0x44, 0x4c, 0x36, 0x68,
            0x38, 0x8a, 0x08, 0x7a, 0x16, 0x8c, 0xcf, 0xff,
            0xfe, 0x91, 0x94, 0x6b, 0xd0, 0x79, 0x66, 0x87,
            0x69, 0x36, 0x26, 0x4e, 0xcd, 0xe2, 0x9a, 0x9e
    };
    BitIO test = BitIO(testData);

    //decode bitcoin data
    BitIO bitcoinData = test.copyBitcoinData();
    for (size_t i = 0; i < 80; i++) {
        EXPECT_EQ(bitcoinData.getBits(8), testData[i + 2]);
    }
}








/*
 * ██╗░░░██╗░█████╗░██████╗░██╗░█████╗░██████╗░██╗░░░░░███████╗  ██╗░░░░░███████╗███╗░░██╗░██████╗░████████╗██╗░░██╗
 * ██║░░░██║██╔══██╗██╔══██╗██║██╔══██╗██╔══██╗██║░░░░░██╔════╝  ██║░░░░░██╔════╝████╗░██║██╔════╝░╚══██╔══╝██║░░██║
 * ╚██╗░██╔╝███████║██████╔╝██║███████║██████╦╝██║░░░░░█████╗░░  ██║░░░░░█████╗░░██╔██╗██║██║░░██╗░░░░██║░░░███████║
 * ░╚████╔╝░██╔══██║██╔══██╗██║██╔══██║██╔══██╗██║░░░░░██╔══╝░░  ██║░░░░░██╔══╝░░██║╚████║██║░░╚██╗░░░██║░░░██╔══██║
 * ░░╚██╔╝░░██║░░██║██║░░██║██║██║░░██║██████╦╝███████╗███████╗  ███████╗███████╗██║░╚███║╚██████╔╝░░░██║░░░██║░░██║
 * ░░░╚═╝░░░╚═╝░░╚═╝╚═╝░░╚═╝╚═╝╚═╝░░╚═╝╚═════╝░╚══════╝╚══════╝  ╚══════╝╚══════╝╚═╝░░╚══╝░╚═════╝░░░░╚═╝░░░╚═╝░░╚═╝
 */

TEST(BitIO, MakeXBit) {
    //value with depth 0
    BitIO test = BitIO::makeXBit({0, 1, 4});
    EXPECT_EQ(test.getLength(), 4);
    EXPECT_EQ(test.getBits(4), 0x1);

    //value with key 0
    bool failed = true;
    try {
        test = BitIO::makeXBit({0, 0, 4});
    } catch (const std::exception& e) {
        failed = false;
    }
    EXPECT_FALSE(failed);

    //value with depth 1
    test = BitIO::makeXBit({1, 4, 4});
    EXPECT_EQ(test.getLength(), 8);
    EXPECT_EQ(test.getBits(8), 0x04);
}
/*
TEST(BitIO,GetXBit) {
    vector<uint8_t> testData={0x40,0x14};
    BitIO test=BitIO(testData);

    EXPECT_EQ(test.getXBit(4),xBitValue{0,4,4});
    EXPECT_EQ(test.getXBit(4),xBitValue{1,1,4});
    EXPECT_EQ(test.getXBit(2),xBitValue{0,1,2});
}
*/
