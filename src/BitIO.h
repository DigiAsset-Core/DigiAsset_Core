//
// Created by mctrivia on 10/04/23.
//

#ifndef DIGIBYTECORE_BITIO_H
#define DIGIBYTECORE_BITIO_H


#include <cstdint>
#include <cstdio>
#include <vector>
#include <limits>
#include <stdexcept>

#define BITIO_SIZEOF_LONG 64

#define BITIO_FILL_ZEROS 0
#define BITIO_FILL_ONES 1
#define BITIO_FILL_RANDOM 2

#define BITIO_CHARSET_3B40 "0123456789abcdefghijklmnopqrstuvwxyz#$&."
#define BITIO_CHARSET_ALPHA "0123456789abcdefghijklmnopqrstuvwxyz $%*+-./:"
#define BITIO_CHARSET_HEX "0123456789abcdef"

#define BITIO_ENCODE_ALPHA 0
#define BITIO_ENCODE_UTF8 1
#define BITIO_ENCODE_HEX 2
#define BITIO_ENCODE_3B40 3

//TYPE 0 is bool but will never be returned since bool and int can't be differentiated
#define BITIO_BITCOIN_TYPE_DATA (-2)
#define BITIO_BITCOIN_OP_RETURN 106



/**
 * Variable length encoding scheme.  you can store keys from 1 to 2^xLength-1 at any depth with this encoding system.
 * the higher the depth the more space needed so you should prioritize keys so more common keys are on lower depths
 */
struct xBitValue {
    uint64_t depth;   //how many layers deep we should go
    uint64_t key;   //the key for value to be encoded(0 is never valid)
    uint64_t xBitLength;//number of bits each depth section takes up

    //allow comparison
    bool operator==(const xBitValue& b) const {
        return ((depth == b.depth) && (key == b.key));
    }

    bool operator!=(const xBitValue& b) const {
        return ((depth != b.depth) || (key != b.key));
    }

    //verify entered values
    void verify() const {
        if (key == 0) throw std::out_of_range("key can not be zero");
        if (xBitLength == 0) throw std::out_of_range("xBitLength must be greater than zero");
        if (xBitLength == 64) return; //if xBitLength=64 any value compiler will allow is correct
        if (key >= (uint64_t) 1 << xBitLength) throw std::out_of_range("value must fit in xBitLength bits");
    }
};

/**
 * When using the make and decode best string functions you need to provide an array of header options
 */
struct stringHeaderOption {
    xBitValue header;   //xBit encoded header value should this option be chosen
    size_t encoder;     //which BITIO_ENCODE_n function to use for this option
};

const std::vector<stringHeaderOption> defaultHeaderOptions = {
        {{0, 1, 2}, BITIO_ENCODE_ALPHA},
        {{0, 2, 2}, BITIO_ENCODE_3B40},
        {{0, 3, 2}, BITIO_ENCODE_UTF8},
        {{1, 1, 2}, BITIO_ENCODE_HEX}
};


class BitIO {
    std::vector<uint64_t> _bits;
    size_t _position = 0;
    size_t _length = 0;

    //bitwise math TestHelpers
    static uint64_t makeRightMask(size_t length);
    static uint64_t makeCenterMask(size_t length, size_t shiftLeft);
    static uint64_t makeLeftMask(size_t length);
    static uint64_t getRandomLong();

    //string TestHelpers
    static size_t stringLength(const std::string& message);

    //bitwise position
    size_t getLongIndex() const;
    size_t getEndLongIndex();
    size_t getBitIndex() const;
    size_t getEndBitIndex() const;
    void resizeIfNeeded();

    //common error checking
    static void checkLength(size_t length, size_t max = BITIO_SIZEOF_LONG);
    static void invalidLength();
    void checkAvailableBits(size_t length) const;
    static void checkValueSize(uint64_t value, size_t length);
    static void checkValueRange(int64_t value, int64_t min = std::numeric_limits<int64_t>::min(),
                                int64_t max = std::numeric_limits<int64_t>::max());
    static void checkValueRange(uint64_t value, uint64_t min = 0, uint64_t max = std::numeric_limits<uint64_t>::max());
    void checkPosition(size_t position) const;
    static void invalidCharacter();
    static size_t getCharSetPos(char character, const std::string& charSet);
    static void invalidOpCode();
    static void invalidEncoder();

public:
    //create blank
    BitIO() = default;

    //integer math TestHelpers
    static uint64_t pow10(uint64_t exponent);

    //byte wise raw data
    explicit BitIO(std::vector<uint8_t> data, size_t length = 0);

    //read/write
    uint64_t getBits(size_t length);
    bool checkBits(uint64_t value,
                   size_t length);    //checks the next length bits match a specific value.(does not move pointer)
    void insertBits(uint64_t value, size_t length);   //inserts bits at current position moving all data back
    void insertBits(BitIO& value);
    void setBits(uint64_t value,
                 size_t length);      //writes bits at current position overwriting any data there and adding to end if needed
    void setBits(BitIO& value);
    void appendBits(uint64_t value, size_t length);   //appends bits to end of data(does not move pointer)
    void appendBits(BitIO& value);
    BitIO
    copyBits(size_t length);                              //same as getBits but allows returning larger than 64 bits
    void padWidth(size_t multiple = 8, unsigned char fillStyle = BITIO_FILL_ZEROS);
    void appendZeros(size_t count);                 //append count zeros in a row

    //position and size manipulation
    void movePositionBy(int amount);
    void movePositionTo(size_t bitIndex = 0);
    void movePositionToBeginning();
    void movePositionToEnd();
    size_t getPosition() const;
    size_t getLength() const;
    size_t getNumberOfBitLeft() const;

    //strings  ALL message VALUES MUST BE UTF8 ENCODED
    static BitIO makeBestString(const std::string& message, size_t lengthBits,
                                const std::vector<stringHeaderOption>& headerOptions = defaultHeaderOptions);
    std::string
    getBestString(size_t lengthBits, const std::vector<stringHeaderOption>& headerOptions = defaultHeaderOptions);
    static BitIO makeAlphaString(const std::string& message);
    std::string getAlphaString(size_t length);
    static BitIO makeUTF8String(const std::string& message);
    std::string getUTF8String(size_t length);
    static BitIO makeHexString(const std::string& message);
    std::string getHexString(size_t length);
    static BitIO make3B40String(const std::string& message);
    std::string get3B40String(size_t length);

    //numbers
    static BitIO makeFixedPrecision(uint64_t value);
    uint64_t getFixedPrecision();
    static BitIO makeDouble(double value, bool isLE = true);
    double getDouble(bool isLE = true);

    //Bitcoin
    static BitIO makeBitcoin(int8_t value, bool includeOpReturn = false);
    static BitIO makeBitcoin(const std::vector<uint8_t>& value, bool includeOpReturn = false);
    static void makeBitcoin(BitIO& value, bool includeOpReturn = false);  //alters value
    bool checkIsBitcoinOpReturn();  //moves pointer if true
    int8_t
    getBitcoinDataHeader();  //returns -2 if data and does not move header, -1 to 16 is int value(0 or 1 may be bool but is indistinguishable)
    std::vector<uint8_t> getBitcoinData();
    BitIO copyBitcoinData();    //same as get but returns as BitIO object


    //Variable Length Encoding
    static BitIO makeXBit(const xBitValue& value);
    xBitValue getXBit(size_t xLength);


    friend class BitIO_MakeRightMask_Test;

    friend class BitIO_MakeLeftMask_Test;

    friend class BitIO_MakeCenterMask_Test;

    friend class BitIO_GetRandomLong_Test;

    friend class BitIO_StringLength_Test;

};


#endif //DIGIBYTECORE_BITIO_H
