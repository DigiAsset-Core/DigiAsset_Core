//
// Created by mctrivia on 10/04/23.
//

#include <stdexcept>
#include <random>
#include <cmath>
#include <iostream>
#include "BitIO.h"



/**
 * Construct BitIO object from an array of bytes
 * @param data - array of bytes to use(if length is not a multiple of 8 the last bytes data should be aligned with MSB 11=0xC0
 * @param length - length in bits.  only use if data may not be exact multiple of 8 bits
 */
BitIO::BitIO(std::vector<uint8_t> data, size_t length) {
    //error checking
    if (length != 0) {
        //if length provided then check it is possible for data provided
        size_t byteCount = length / 8;                          //gets number of whole bytes in data
        if (length % 8 != 0) {
            byteCount++;                       //if a partial byte add 1 to byteCount
            if ((data.back() & makeRightMask(8 - length % 8)) > 0) {
                invalidLength();
            }   //if any right bits that won't be saved are set
        }
        if (data.size() != byteCount) throw std::out_of_range("incorrect length provided");
    } else {
        //if length is not provided than assume full bytes
        length = 8 * data.size();
    }

    //initialize key values
    _length = length;
    _bits.reserve(length / BITIO_SIZEOF_LONG +
                  1);                  //pre reserve the space we need to store the data(+1 to prevent needing extra checks)

    //convert byte data to longs
    uint64_t temp = 0;
    size_t offset = BITIO_SIZEOF_LONG;                            //start offset with full size of long because first thing we do is subtract 8 bits from it
    for (unsigned char BI: data) {
        offset -= 8;                                              //shift to next bit
        temp = temp | ((uint64_t) BI
                << offset);                             //shift to correct spot and or the data to the temp long
        if (offset == 0) {                                        //if we finish the long
            _bits.push_back(temp);                              //store the long
            temp = 0;                                             //clear temp
            offset = BITIO_SIZEOF_LONG;                           //start offset with full size of long because first thing we do is subtract 8 bits from it
        }
    }
    if (offset != BITIO_SIZEOF_LONG) {
        _bits.push_back(temp);
    }                       //store any leftover data
}


/**
 * Returns the current long of data being pointed to
 * @return
 */
size_t BitIO::getLongIndex() const {
    return _position / BITIO_SIZEOF_LONG;
}

/**
 * Returns the current bit in the long being pointed to(from left to right)
 * @return
 */
size_t BitIO::getBitIndex() const {
    return _position % BITIO_SIZEOF_LONG;
}

/**
 * Returns the last long index
 * @return
 */
size_t BitIO::getEndLongIndex() {
    return _bits.size() - 1;
}

/**
 * Returns the last bit in the last long(from left to right)
 * @return
 */
size_t BitIO::getEndBitIndex() const {
    return _length % BITIO_SIZEOF_LONG;
}

/**
 * Checks how many longs of data are needed for the current set length and resizes the vector to accommodate it
 */
void BitIO::resizeIfNeeded() {
    size_t newLongLength = _length / BITIO_SIZEOF_LONG + ((_length % BITIO_SIZEOF_LONG != 0) ? 1 : 0);
    _bits.resize(newLongLength);
}


/**
 * Error checking for data types
 * Checks a length value is between 1 and max inclusive.
 * @param length
 * @param max - value must be at least 1
 */
void BitIO::checkLength(size_t length, size_t max) {
    if (length == 0) {
        throw std::out_of_range("length can not be zero");
    }
    if (length > max) {
        throw std::out_of_range("length must be smaller than data type");
    }
}

/**
 * Error checking to see if a value can fit in a specific number of bits
 * @param value
 * @param length - number from 0 to BITIO_SIZEOF_LONG
 */
void BitIO::checkValueSize(uint64_t value, size_t length) {
    if (length == 0) {
        throw std::out_of_range("value must be smaller than length bits");
    }
    if (length == BITIO_SIZEOF_LONG) {
        return;
    } //if length=size of long by definition it can fit entire value
    if (value >= (uint64_t) 1 << length)
        throw std::out_of_range("value must be smaller than length bits");
}

void BitIO::invalidLength() {
    throw std::out_of_range("invalid length provided");
}

void BitIO::checkAvailableBits(size_t length) const {
    if (_position + length > _length) {
        throw std::out_of_range("not enough bits left");
    }
}

void BitIO::checkValueRange(int64_t value, int64_t min, int64_t max) {
    if (value < min) {
        throw std::out_of_range("value is to small");
    }
    if (value > max) {
        throw std::out_of_range("value is to large");
    }
}

void BitIO::checkValueRange(uint64_t value, uint64_t min, uint64_t max) {
    if (value < min) {
        throw std::out_of_range(
                "value is to small");
    }   //never used since current code always sets min to 0.  here for possible future versions
    if (value > max)
        throw std::out_of_range("value is to large");
}

void BitIO::checkPosition(size_t position) const {
    if (position > _length) {
        throw std::out_of_range("position must be inside the data");
    }
}

void BitIO::invalidCharacter() {
    throw std::out_of_range("invalid character for encoding method");
}

size_t BitIO::getCharSetPos(char character, const std::string& charSet) {
    size_t i = charSet.find(character);
    if (i == std::string::npos) {
        BitIO::invalidCharacter();
    }
    return i;
}

void BitIO::invalidOpCode() {
    throw std::out_of_range("invalid op_return data length header");
}

void BitIO::invalidEncoder() {
    throw std::out_of_range("invalid encoder selected");
}


uint64_t BitIO::makeRightMask(size_t length) {
    if (length == 64) {
        return 0xffffffffffffffff;
    }   //below formula would fail for 64
    return ((uint64_t) 1 << length) - (uint64_t) 1;
}

uint64_t BitIO::makeCenterMask(size_t length, size_t shiftLeft) {
    return makeRightMask(length) << shiftLeft;
}

uint64_t BitIO::makeLeftMask(size_t length) {
    return ~makeRightMask(BITIO_SIZEOF_LONG - length);
}

uint64_t BitIO::getRandomLong() {
    std::random_device rd;                                      //get system random source
    std::mt19937_64 eng(rd());                                  //use 64bit mersenne twister
    std::uniform_int_distribution<uint64_t> distribution;       //distribute results over 64bit number space
    return distribution(eng);
}

/**
 * Takes a UTF8 string and determines how long it is
 * @param message
 * @return
 */
size_t BitIO::stringLength(const std::string& message) {
    size_t length = 0;
    for (uint8_t byte: message) {
        if ((byte & 0xc0) != 0x80) {
            length++;
        }    //don't count non header bytes
    }
    return length;
}

/**
 * fast function to do 10^exponent since the built in pow function in c++ returns a floating point number and we dont want rounding
 * Since uint64_t can only store numbers in up to 10^18 we can just store every possible value and get rid of all the time consuming multiplication
 * @param exponent
 * @return
 */
uint64_t BitIO::pow10(uint64_t exponent) {
    checkValueRange(exponent, 0, 18);
    static uint64_t powers[19] =
            {1, 10, 100, 1000, 10000,
             100000, 1000000, 10000000, 100000000, 1000000000,
             10000000000, 100000000000, 1000000000000, 10000000000000, 100000000000000,
             1000000000000000, 10000000000000000, 100000000000000000, 1000000000000000000
            };
    return powers[exponent];
}


/**
 * Gets requested bits and moves pointer to next unread bit
 * @param length - length of data to read in bits
 * @return
 */
uint64_t BitIO::getBits(size_t length) {
    //initial error checking
    checkLength(length);
    checkAvailableBits(length);

    //find current position
    size_t lI = getLongIndex();                                   //find which long position is in
    size_t numberOfBitsLeftInLong = BITIO_SIZEOF_LONG - getBitIndex();  //find which bit in the long we are in

    //move pointer to end position
    _position += length;

    //get data from first long
    size_t captureLength = std::min(numberOfBitsLeftInLong, length); //find how many bits we can take from this long
    size_t shift = numberOfBitsLeftInLong -
                   captureLength;       //number of bits we need to shift the data by to get the part we want at lsb
    uint64_t result = (_bits[lI] >> shift) & makeRightMask(
            captureLength);  //shift bits to the right so the desired data is at the LSB then apply mask to remove unwanted data
    if (captureLength == length) {
        return result;
    }                   //if all data was in 1 long then return

    //get data from next long(if data spanned 2 longs)
    lI++;                                                       //move long index ahead
    captureLength = length - captureLength;                         //get number of bits that remain to be collected
    result = result
            << captureLength;                               //shift result to the left so there is room for the rest
    shift = BITIO_SIZEOF_LONG -
            captureLength;                        //get number of bits we need to shift the data to get the part we want to lsb
    return result | (_bits[lI]
            >> shift);                          //or the data from the first and second parts together and return
}

/**
 * Checks the next bits equal a specific value
 * does not throw any errors and does not update the pointer
 * if not enough bits exist will return false
 * @param value
 * @param length
 * @return
 */
bool BitIO::checkBits(uint64_t value, size_t length) {
    if (getNumberOfBitLeft() < length) {
        return false;
    }  //not enough bits so can't be a match

    //get the next bits without moving pointer
    uint64_t nextBits = getBits(length);
    _position -= length;  //put pointer back

    //check if match
    return (nextBits == value);
}

/**
 * writes the bits to the object and moves the pointer to the next unwritten bit.  If more bits are written then there were bits to write the object is extended
 * @param value - value to write
 * @param length - number of bits to write
 */
void BitIO::setBits(uint64_t value, size_t length) {
    //initial error checking
    checkLength(length);
    checkValueSize(value, length);

    //find current position
    size_t lI = getLongIndex();                                   //find which long position is in
    size_t numberOfBitsLeftInLong = BITIO_SIZEOF_LONG - getBitIndex(); //find which bit in the long we are in

    //make sure pointers values will be correct
    _length = std::max(_length, _position +
                                length);                 //if we write more bits than currently in object then we need to extend length of object
    _position += length;

    //if data aligns with long do the easy way
    if ((numberOfBitsLeftInLong == BITIO_SIZEOF_LONG) & (length == BITIO_SIZEOF_LONG)) {
        _bits[lI] = value;
        return;
    }

    //set data in first long
    if (_bits.size() == lI) {
        _bits.push_back(0);
    }                   //if bits was full add a 0 to end
    size_t setLength = std::min(numberOfBitsLeftInLong, length);  //find how many bits we can set in this long
    uint64_t mask = ~makeCenterMask(setLength, numberOfBitsLeftInLong -
                                               setLength);//mask for which bits we want to keep from the original data
    uint64_t valueShifted = (setLength == length) ? (value << (numberOfBitsLeftInLong - setLength)) : (value
            >> (length - setLength));
    _bits[lI] = (_bits[lI] & mask) |
                valueShifted;       //keep the bits we won't write.  shift the bits from input to correct spot and or together
    if (setLength == length)
        return;                              //if all data was in 1 long then return

    //set data for next long(if data spanned 2 longs)
    lI++;                                                       //move long index ahead
    if (_bits.size() == lI)
        _bits.push_back(0);                   //if bits was full add a 0 to end
    setLength = length - setLength;                                 //get number of bits that remain to be set
    mask = ~makeCenterMask(setLength,
                           BITIO_SIZEOF_LONG - setLength);  //mask for which bits we want to keep from the original data
    _bits[lI] = (_bits[lI] & mask) | (value << (BITIO_SIZEOF_LONG -
                                                setLength)); //keep the bits we won't write.  shift the bits from input to correct spot and or together
}

/**
 * writes the bits to the object and moves the pointer to the next unwritten bit.  If more bits are written then there were bits to write the object is extended
 */
void BitIO::setBits(BitIO& value) {
    //save pointer
    size_t valuePointerStartPosition = value.getPosition();

    //make sure there are enough longs
    size_t minLength = _position + value.getLength();
    if (_length < minLength) {
        _length = minLength;
        resizeIfNeeded();
    }

    //set the bits
    value.movePositionTo(0);                                    //move pointer to beginning
    size_t length = BITIO_SIZEOF_LONG - getEndBitIndex();           //fill long first
    while (size_t remaining = value.getNumberOfBitLeft()) {  //loop until run out of data
        length = std::min(remaining, length);                      //make sure number of bits requested is possible
        setBits(value.getBits(length), length);                  //get a block of data and set it
        length = BITIO_SIZEOF_LONG;                               //all future blocks should be full long if possible
    }

    //return pointer
    value.movePositionTo(valuePointerStartPosition);            //return pointer back to where it was
}

/**
 * writes the bits to the end of the object.  pointer is not moved.
 * @param value - value to write
 * @param length - number of bits to write
 */
void BitIO::appendBits(uint64_t value, size_t length) {
    //initial error checking
    checkLength(length);
    checkValueSize(value, length);

    //find current end position
    size_t lI = getEndLongIndex();                                //find which long position is in
    size_t numberOfBitsLeftInLong = BITIO_SIZEOF_LONG - getEndBitIndex(); //find which bit in the long we are in

    //make sure pointers values will be correct
    _length += length;                                            //increase size by added bits

    //if write at end of last long simple add
    if (numberOfBitsLeftInLong == BITIO_SIZEOF_LONG) {
        _bits.push_back(value << (BITIO_SIZEOF_LONG - length));     //shift to correct spot and add
        return;
    }

    //if fits in 1 long then shift to correct spot and save it
    if (length <= numberOfBitsLeftInLong) {
        _bits[lI] = _bits[lI] | (value << (numberOfBitsLeftInLong - length));
        return;
    }

    //can't write everything to one long so write what we can than add the rest to a new long
    _bits[lI] = _bits[lI] | (value >> (length - numberOfBitsLeftInLong));
    uint64_t valueToAdd = value << (BITIO_SIZEOF_LONG - (length - numberOfBitsLeftInLong));
    _bits.push_back(valueToAdd);
}

/**
 * same as getBits but returns a BitIO object
 * @param length - number of bits to copy
 * @return
 */
BitIO BitIO::copyBits(size_t length) {
    BitIO result;
    size_t copyLength = BITIO_SIZEOF_LONG;
    while (length > 0) {
        if (length < BITIO_SIZEOF_LONG) {
            copyLength = length;
        }
        result.appendBits(getBits(copyLength), copyLength);
        length -= copyLength;
    }
    return result;
}


/**
 * writes the bits to the end of the object.  pointer is not moved on current object but will be moved on value
 * @param value
 */
void BitIO::appendBits(BitIO& value) {
    size_t valuePointerStartPosition = value.getPosition();       //save pointer
    value.movePositionTo(0);                                    //move pointer to beginning
    size_t length = BITIO_SIZEOF_LONG - getEndBitIndex();           //fill long first
    while (size_t remaining = value.getNumberOfBitLeft()) {           //loop until run out of data
        length = std::min(remaining, length);                      //make sure number of bits requested is possible
        appendBits(value.getBits(length), length);               //get a block of data and set it
        length = BITIO_SIZEOF_LONG;                               //all future blocks should be full long if possible
    }
    value.movePositionTo(valuePointerStartPosition);            //return pointer back to where it was
}

/**
 * inserts the bits at current position shifting all data after that back
 * moves the pointer to the next unwritten bit.
 * @param value - value to write
 * @param length - number of bits to write
 */
void BitIO::insertBits(uint64_t value, size_t length) {
    //initial error checking
    checkValueSize(value, length);

    //resize if needed
    _length += length;
    resizeIfNeeded();

    //move bits back
    size_t lIEnd = getLongIndex();
    if (length == BITIO_SIZEOF_LONG) {

        //if length is exactly one long we can move back easily by copying longs
        for (size_t lI = getEndLongIndex(); lI > lIEnd; lI--) {
            _bits[lI] = _bits[lI - 1];
        }

    } else {

        //length not exactly one long so we need to shift bits for all longs after the current
        for (size_t lI = getEndLongIndex(); lI > lIEnd; lI--) {
            _bits[lI] = (_bits[lI] >> length) |
                        (_bits[lI - 1] << (BITIO_SIZEOF_LONG - length));//move the bits over length bits
        }

        //long where data is to be inserted needs to be handled special
        size_t bI = getBitIndex();
        uint64_t notMovingPart = (bI == 0) ? 0 : _bits[lIEnd] & makeLeftMask(
                bI);        //part of long to left of position that will not move
        uint64_t movedPart = (BITIO_SIZEOF_LONG - bI < length) ? 0 : (_bits[lIEnd] >> length) & makeRightMask(
                BITIO_SIZEOF_LONG - bI - length);    //part of long that was moved
        _bits[lIEnd] = notMovingPart | movedPart;                //put the 2 parts together

    }

    //write data
    setBits(value, length);
}

/**
 * inserts the bits at current position shifting all data after that back
 * moves the pointer to the next unwritten bit.
 */
void BitIO::insertBits(BitIO& value) {
    size_t valuePointerStartPosition = value.getPosition();       //save pointer
    value.movePositionTo(0);                                    //move pointer to beginning

    //resize if needed
    _length += value.getLength();
    resizeIfNeeded();

    //move bits back by full longs
    size_t lIEnd = getLongIndex();
    size_t offsetAmount = value.getLength() / BITIO_SIZEOF_LONG;
    if (offsetAmount > 0) {
        for (size_t lI = getEndLongIndex(); lI - offsetAmount + 1 >
                                            lIEnd; lI--) {            //since >=0 will never evaluate false add 1 from left side so >0 has same meaning
            _bits[lI] = _bits[lI - offsetAmount];
        }
        lIEnd += offsetAmount;                                    //already moved these longs so let's not move again
    }

    //if not an exact multiple of long then move by leftover
    if (value.getLength() % BITIO_SIZEOF_LONG != 0) {

        //length not exactly one long so we need to shift bits for all longs after the current
        size_t length = value.getLength() % BITIO_SIZEOF_LONG;
        for (size_t lI = getEndLongIndex(); lI > lIEnd; lI--) {
            _bits[lI] = (_bits[lI] >> length) |
                        (_bits[lI - 1] << (BITIO_SIZEOF_LONG - length));//move the bits over length bits
        }

        if (offsetAmount == 0) {
            //long where data is to be inserted needs to be handled special(not moving more than 1 long)
            size_t bI = getBitIndex();
            uint64_t notMovingPart = (bI == 0) ? 0 : _bits[lIEnd] & makeLeftMask(
                    bI);        //part of long to left of position that will not move
            uint64_t movedPart = (bI + length > BITIO_SIZEOF_LONG) ? 0 : (_bits[lIEnd] >> length) & makeRightMask(
                    BITIO_SIZEOF_LONG - (bI + length));    //part of long that was moved
            _bits[lIEnd] = notMovingPart | movedPart;                //put the 2 parts together
        } else {
            //moved by more than 1 long
            size_t bI = getBitIndex();
            _bits[lIEnd - offsetAmount] = (bI == 0) ? 0 : _bits[lIEnd] & makeLeftMask(
                    bI);        //part of long to left of position that will not move
            _bits[lIEnd] = (bI + length > BITIO_SIZEOF_LONG) ? 0 : (_bits[lIEnd] >> length) & makeRightMask(
                    BITIO_SIZEOF_LONG - (bI + length));    //part of long that was moved
        }

    }

    //write data
    setBits(value);
    value.movePositionTo(valuePointerStartPosition);            //return pointer back to where it was
}

void BitIO::padWidth(size_t multiple, unsigned char fillStyle) {
    //check if padding needed
    if (multiple <= 1) {
        return;
    }                                    //multiples of 0 are not possible and 1 will always be correct so stop processing
    size_t bitsPastMultiple = _length % multiple;
    if (bitsPastMultiple == 0)
        return;                            //already correct length so return

    //adjust size
    size_t neededExtraBits = multiple - bitsPastMultiple;           //compute number of bits needed
    _length += neededExtraBits;                                   //update bits length to new length
    resizeIfNeeded();
    if (fillStyle == BITIO_FILL_ZEROS)
        return;                    //already padded with 0s to desired length so return

    //fill bits in first long
    uint64_t fillMask = (fillStyle == BITIO_FILL_RANDOM) ? getRandomLong()
                                                         : ~0;    //value used to either fill with 1s or random bits
    size_t position = _length - neededExtraBits;                //compute first bit to change to a 1
    size_t lI = position / BITIO_SIZEOF_LONG;                   //get the first long to process
    size_t bitsLeftInLong =
            BITIO_SIZEOF_LONG - (position % BITIO_SIZEOF_LONG);//get number of bits left in long that can be changed
    size_t bitsToChange = std::min(neededExtraBits, bitsLeftInLong);//number of bits to write now
    _bits[lI] = _bits[lI] | (makeCenterMask(bitsToChange, bitsLeftInLong - bitsToChange) & fillMask);
    neededExtraBits -= bitsToChange;

    //write extra longs
    while (neededExtraBits > 0) {
        lI++;
        if (fillStyle == BITIO_FILL_RANDOM)
            fillMask = getRandomLong();
        if (neededExtraBits >= BITIO_SIZEOF_LONG) {
            _bits[lI] = fillMask;                                 //set all bits to 1
            neededExtraBits -= BITIO_SIZEOF_LONG;
        } else {
            _bits[lI] = makeLeftMask(neededExtraBits) & fillMask; //set left most bits to 1s
            neededExtraBits = 0;
        }
    }
}

void BitIO::appendZeros(size_t count) {
    _length += count;                                   //update bits length to new length
    resizeIfNeeded();
}

void BitIO::movePositionTo(size_t bitIndex) {
    checkPosition(bitIndex);                            //initial error checking
    _position = bitIndex;                                         //move position pointer
}

void BitIO::movePositionBy(int amount) {
    movePositionTo(_position + amount);
}

/**
 * moves position to beginning
 * slightly faster than movePositionTo(0) since less error checking
 */
void BitIO::movePositionToBeginning() {
    _position = 0;
}

/**
 * moves pointer to after last bit
 */
void BitIO::movePositionToEnd() {
    _position = _length;
}

size_t BitIO::getPosition() const {
    return _position;
}

size_t BitIO::getLength() const {
    return _length;
}

size_t BitIO::getNumberOfBitLeft() const {
    return _length - _position;
}

/**
 * This generates a binary string that can be read knowing only the length bit length and header options.
 * output format is {header}{length}{data}
 * header is encoded using xBit encoding.  Default uses an xLength of 2 bits.
 * length is number of characters in string
 * header option allows you to define what encoding methods are allowed and set custom headers for each.  the default allows all known encoders to be tried but a custom list may be slightly more efficient.
 * @param message
 * @param lengthBits
 * @param headerOptions
 * @return
 */
BitIO BitIO::makeBestString(const std::string& message, size_t lengthBits,
                            const std::vector<stringHeaderOption>& headerOptions) {
    BitIO best;
    size_t bestLength = SIZE_MAX;

    //check length bits is valid
    size_t characterCount = BitIO::stringLength(message);
    checkValueSize(characterCount, lengthBits);

    //try each encoder type
    for (const stringHeaderOption& headerOption: headerOptions) {
        bool invalidEncoderDetected = false;  //because of catch can't just throw error direct so need to watch for flag
        try {
            //try encoding in the current format
            BitIO temp;
            BitIO result = makeXBit(headerOption.header);
            result.appendBits(characterCount, lengthBits);
            switch (headerOption.encoder) {
                case BITIO_ENCODE_ALPHA:
                    temp = BitIO::makeAlphaString(message);
                    break;
                case BITIO_ENCODE_UTF8:
                    temp = BitIO::makeUTF8String(message);
                    break;
                case BITIO_ENCODE_HEX:
                    temp = BitIO::makeHexString(message);
                    break;
                case BITIO_ENCODE_3B40:
                    temp = BitIO::make3B40String(message);
                    break;
                default:
                    invalidEncoderDetected = true;
            }
            result.appendBits(temp);

            //see if best
            if (result.getLength() < bestLength) {
                best = result;
                bestLength = result.getLength();  //needed because bests length is 0 before one is set as best.
            }

        } catch (const std::exception& e) {
            //encoding method won't work for this input but that is ok as long as a different one works
        }
        if (invalidEncoderDetected) {
            invalidEncoder();
        }
    }

    //see if any encoding methods worked
    if (bestLength == SIZE_MAX) {
        invalidCharacter();
    }

    //we managed to encode so return
    return best;
}

std::string BitIO::getBestString(size_t lengthBits, const std::vector<stringHeaderOption>& headerOptions) {
    //read header and make sure it is valid
    xBitValue header = getXBit(headerOptions.front().header.xBitLength);
    size_t encoder = SIZE_MAX;
    for (const stringHeaderOption& headerOption: headerOptions) {
        if (headerOption.header == header) {
            encoder = headerOption.encoder;
            break;
        }
    }

    //get length
    size_t length = getBits(lengthBits);

    //decode string
    switch (encoder) {
        case BITIO_ENCODE_ALPHA:
            return getAlphaString(length);
        case BITIO_ENCODE_UTF8:
            return getUTF8String(length);
        case BITIO_ENCODE_HEX:
            return getHexString(length);
        case BITIO_ENCODE_3B40:
            return get3B40String(length);
        default:
            invalidEncoder();
    }
    return "";   //unreachable but makes clang happy
}

/**
 * Encodes a string into Alpha encoding and return as BitIO Object
 * if there are any characters that can not be encoded an out_of_range error will be thrown
 * valid character set:  BITIO_CHARSET_ALPHA
 * @param message
 * @return
 */
BitIO BitIO::makeAlphaString(const std::string& message) {
    BitIO result;
    const std::string charSet = BITIO_CHARSET_ALPHA;

    //go through each character in message
    uint64_t val = 0;
    size_t charI = 0;
    for (; charI < message.size(); charI++) {
        val = val * 45 + getCharSetPos(message[charI], charSet);

        //write every group of 2 characters to result as 11 bit number
        if (charI % 2 == 1) {
            result.appendBits(val, 11);
            val = 0;
        }
    }

    //if there is an odd number of characters encode last character as 6 bit number
    if (charI % 2 == 1) {
        result.appendBits(val, 6);
    }

    return result;
}

/**
 * Decodes an Alpha encoded string
 * Pointer will be moved to the end of the read data
 * @param length - number of letters to be decoded
 * @return
 */
std::string BitIO::getAlphaString(size_t length) {
    std::string result;
    const std::string charSet = BITIO_CHARSET_ALPHA;

    //go through each set of 2 characters
    for (size_t charI = 0; charI < length / 2; charI++) { //length/2 will round down
        uint64_t chars = getBits(11);
        result.push_back(charSet[chars / 45]);
        result.push_back(charSet[chars % 45]);
    }

    //if odd number of characters read last one
    if (length % 2 == 1) {
        result.push_back(charSet[getBits(6)]);
    }

    return result;
}

BitIO BitIO::makeUTF8String(const std::string& message) {
    BitIO result;
    uint8_t remainingBytes = 0;  //not a truly accurate name
    size_t bitLength;
    uint64_t block;
    for (uint8_t byte: message) {
        //check if 1 byte character
        if ((byte & 0x80) == 0) {
            if (remainingBytes != 0) {
                invalidCharacter();
            }    //each byte in a multibyte sequences always start with a 1
            result.appendBits(byte, 8);      //header 0+7bit ascii
            continue;
        }

        //if multibyte, check if it is first byte
        if (remainingBytes == 0) {
            //check 2 bytes
            if (byte < 0xdf) {
                remainingBytes = 2;
                bitLength = 13;
                block = byte & 0x7f;    //remove first 1 from header so now 10
                continue;
            }

            //check 3 bytes
            if (byte < 0xef) {
                remainingBytes = 3;
                bitLength = 19;
                block = byte & 0x7f;    //remove first 1 from header so now 110
                continue;
            }

            //4 bytes
            if (byte < 0xf7) {
                remainingBytes = 4;
                bitLength = 24;
                block = (byte & 0x7) | 0x38; //remove original header of 11110 and replace with 111
                continue;
            }

            //invalid length
            invalidCharacter();
        }

        //non header bytes
        remainingBytes--;
        block = block << 6;
        block |= (byte & 0x3f);
        if (remainingBytes == 1) {
            result.appendBits(block, bitLength);
            remainingBytes = 0;
        }
    }

    return result;
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "cppcoreguidelines-narrowing-conversions"

std::string BitIO::getUTF8String(size_t length) {
    std::string result;

    for (size_t i = 0; i < length; i++) {
        //check single byte
        if (getBits(1) == 0) {
            result.push_back(getBits(7));
            continue;
        }

        //check 2 byte
        if (getBits(1) == 0) {
            result.push_back(0xc0 | getBits(5));
            result.push_back(0x80 | getBits(6));
            continue;
        }

        //check 3 byte
        if (getBits(1) == 0) {
            result.push_back(0xe0 | getBits(4));
            result.push_back(0x80 | getBits(6));
            result.push_back(0x80 | getBits(6));
            continue;
        }

        //4 byte
        result.push_back(0xf0 | getBits(3));
        result.push_back(0x80 | getBits(6));
        result.push_back(0x80 | getBits(6));
        result.push_back(0x80 | getBits(6));
    }
    return result;
}

#pragma clang diagnostic pop

/**
 * Encodes a hex string in to binary nibbles.
 * if there are any characters that can not be encoded an out_of_range error will be thrown
 * valid character set:  BITIO_CHARSET_HEX (case insensitive)
 * @param message
 * @return
 */
BitIO BitIO::makeHexString(const std::string& message) {
    BitIO result;
    const std::string charSet = BITIO_CHARSET_HEX;

    //go through each character in message
    size_t charI = 0;
    for (; charI < message.size(); charI++) {
        result.appendBits(getCharSetPos(tolower(message[charI]), charSet),
                          4); // NOLINT(cppcoreguidelines-narrowing-conversions)
    }
    return result;
}

/**
 * Decodes data to hex string
 * Pointer will be moved to the end of the read data
 * @param length - number of hex characters to return
 * @return
 */
std::string BitIO::getHexString(size_t length) {
    std::string result;
    const std::string charSet = BITIO_CHARSET_HEX;

    //go through each character
    for (size_t charI = 0; charI < length; charI++) {
        result.push_back(charSet[getBits(4)]);
    }

    return result;
}

BitIO BitIO::make3B40String(const std::string& message) {
    BitIO result;
    const std::string charSet = BITIO_CHARSET_3B40;

    //go through each character in message
    uint64_t val = 0;
    size_t charI = 0;
    for (; charI < message.size(); charI++) {
        val = val * 40 + getCharSetPos(message[charI], charSet);

        //write every group of 3 characters to result as 16-bit number
        if (charI % 3 == 2) {
            result.appendBits(val, 16);
            val = 0;
        }
    }

    //if there is not a multiple of 3 characters write remainder as 5 or 11 bit number
    if (charI % 3 == 1) result.appendBits(val, 5);
    if (charI % 3 == 2) result.appendBits(val, 11);

    return result;
}

std::string BitIO::get3B40String(size_t length) {
    std::string result;
    const std::string charSet = BITIO_CHARSET_3B40;

    //go through each set of 3 characters
    for (size_t charI = 0; charI < length / 3; charI++) { //length/3 will round down
        uint64_t chars = getBits(16);
        result.push_back(charSet[chars / 1600]);
        chars = chars % 1600;
        result.push_back(charSet[chars / 40]);
        result.push_back(charSet[chars % 40]);
    }

    //if odd number of characters read last one
    if (length % 3 == 1) result.push_back(charSet[getBits(5)]);
    if (length % 3 == 2) {
        uint64_t chars = getBits(11);
        result.push_back(charSet[chars / 40]);
        result.push_back(charSet[chars % 40]);
    }

    return result;
}

BitIO BitIO::makeFixedPrecision(uint64_t value) {
    checkValueRange(value, (uint64_t) 0,
                    (uint64_t) 18014398509481983);                 //make sure value can be encoded in this encoding method
    BitIO result;

    //check if the simple case of 1 byte value
    if (value < 32) {
        result.appendBits(value, 8);
        return result;
    }

    //compute exponent
    uint64_t exponent = 0;
    while (value % 10 == 0) {
        exponent++;
        value /= 10;
    }
    if (value > 4398046511103) {                  //with large values we can't compress the value using exponent bits
        value *= pow10(exponent);
        exponent = 0;
    }
    if ((exponent > 7) && (value > 33554431)) {       //max encodable exponent value is 7(3 bits)
        value *= pow10(exponent - 7);
        exponent = 7;
    }

    //return binary value
    if (value > 4398046511103) {          //7 bytes
        result.appendBits(3, 2);
        result.appendBits(value, 54);
    } else if (value > 17179869183) {     //6 bytes
        result.appendBits(5, 3);
        result.appendBits(value, 42);
        result.appendBits(exponent, 3);
    } else if (value > 33554431) {        //5 bytes
        result.appendBits(4, 3);
        result.appendBits(value, 34);
        result.appendBits(exponent, 3);
    } else if (value > 131071) {          //4 bytes
        result.appendBits(3, 3);
        result.appendBits(value, 25);
        result.appendBits(exponent, 4);
    } else if (value > 511) {             //3 bytes
        result.appendBits(2, 3);
        result.appendBits(value, 17);
        result.appendBits(exponent, 4);
    } else {                            //2 bytes
        result.appendBits(1, 3);
        result.appendBits(value, 9);
        result.appendBits(exponent, 4);
    }
    return result;
}

uint64_t BitIO::getFixedPrecision() {
    uint64_t length = getBits(3) + 1;

    //max length is 7 bytes so if 7 or 8 this is part of bit data
    if (length >= 7) {
        movePositionBy(-1);
        length = 7;
    }

    //split into parts
    uint64_t mantissa = 0;
    uint64_t exponent = 0;
    if (length == 1) {                                           //1 byte number
        mantissa = getBits(5);
    } else if (length < 5) {                                      //2 to 4 byte number
        mantissa = getBits(length * 8 - 7);
        exponent = getBits(4);
    } else if (length < 7) {                                      //5 to 6 byte number
        mantissa = getBits(length * 8 - 6);
        exponent = getBits(3);
    } else {                                                    //7 byte number
        mantissa = getBits(54);
    }

    //calculate the number and return
    return mantissa * pow10(exponent);
}




#pragma clang diagnostic push
#pragma ide diagnostic ignored "cppcoreguidelines-narrowing-conversions"

BitIO BitIO::makeDouble(double value, bool isLE) {
    /** adapted from nodejs ieee754. BSD-3-Clause License. Feross Aboukhadijeh <https://feross.org/opensource> */
    uint8_t buffer[8];
    size_t nBytes = 8;
    int mLen = 52;       //mantissa length
    size_t offset = 0;

    uint64_t m;
    int e;
    double c;
    int eLen = (nBytes * 8) - mLen - 1;  //exponent length
    uint64_t eMax = makeRightMask(eLen);
    uint64_t eBias = eMax >> 1;
    double rt = (mLen == 23 ? pow(2, -24) - pow(2, -77) : 0);
    size_t i = isLE ? 0 : (nBytes - 1); //start byte
    int d = isLE ? 1 : -1;       //direction
    int s = value < 0 || (value == 0 && 1 / value < 0) ? 1 : 0;//sign

    if (value < 0) {
        value = 0 - value;
    } //get absolute value of value

    if (std::isnan(value) || std::isinf(value)) {
        m = std::isnan(value) ? 1 : 0;
        e = eMax;
    } else if (value == 0) {
        m = 0;
        e = 0;
    } else {
        e = floor(log2(value));  //floor(log(value) / Math.LN2)
        c = pow(2, -e);
        if (value * c < 1) {
            //todo tests do not execute.  find test value that does
            e--;
            c *= 2;
        }
        if (e + eBias >= 1) {
            value += rt / c;
        } else {
            //todo tests do not execute.  find test value that does
            value += rt * pow(2, 1 - eBias);
        }
        if (value * c >= 2) {
            //todo tests do not execute.  find test value that does
            e++;
            c /= 2;
        }

        if (e + eBias >= eMax) {
            //todo tests do not execute.  find test value that does
            m = 0;
            e = eMax;
        } else if (e + eBias >= 1) {
            m = ((value * c) - 1) * pow(2, mLen);
            e = e + eBias;
        } else {
            //todo tests do not execute.  find test value that does
            m = value * pow(2, eBias - 1) * pow(2, mLen);
            e = 0;
        }
    }

    for (; mLen >= 8; buffer[offset + i] = m & 0xff, i += d, m /= 256, mLen -= 8) {}

    e = (e << mLen) | m;
    eLen += mLen;
    for (; eLen > 0; buffer[offset + i] = e & 0xff, i += d, e /= 256, eLen -= 8) {};

    buffer[offset + i - d] |= s * 128;

    BitIO result;
    for (unsigned char bufferValue: buffer) {
        result.appendBits(bufferValue, 8);
    }
    return result;
}

double BitIO::getDouble(bool isLE) {
    /** adapted from nodejs ieee754. BSD-3-Clause License. Feross Aboukhadijeh <https://feross.org/opensource> */
    size_t nBytes = 8;
    int mLen = 52;//mantissa length
    size_t offset = 0;

    //load the bytes
    uint8_t buffer[8];
    for (size_t byteIndex = 0; byteIndex < nBytes; byteIndex++) {
        buffer[byteIndex] = getBits(8);
    }

    uint64_t m;
    int e;
    int eLen = (nBytes * 8) - mLen - 1; //exponent length
    uint64_t eMax = makeRightMask(eLen);
    uint64_t eBias = eMax >> 1;
    int nBits = -7;
    size_t i = isLE ? (nBytes - 1) : 0;
    int d = isLE ? -1 : 1;

    //get sign
    uint64_t s = buffer[offset + i];

    //get exponent
    i += d;
    e = s & makeRightMask(-nBits);  //remove sign part from exponent
    s >>= (-nBits);     //remove exponent part from sign
    nBits += eLen;
    for (; nBits > 0; e = (e * 256) + buffer[offset + i], i += d, nBits -= 8) {}

    //separate shared bits of exponent and mantissa
    m = e & ((1 << (-nBits)) - 1);
    e >>= (-nBits);
    nBits += mLen;

    //get mantis bits
    for (; nBits > 0; m = (m * 256) + buffer[offset + i], i += d, nBits -= 8) {}

    if (e == 0) {
        e = 1 - eBias;
    } else if (static_cast<uint64_t>(e) == eMax) {  //e will never be negative at this point so safe to cast to uin64_t for comparison
        return m ? nan("") : ((s ? -1 : 1) * INFINITY);
    } else {
        m = m + ((uint64_t) 1 << mLen);      //1<<mLen is same as 2^mLen
        e = e - eBias;
    }
    return (s ? -1.0 : 1.0) * m * pow(2, e - mLen);
}

#pragma clang diagnostic pop




/**
 * Can encode an integer between -1 and 16 inclusive
 * @param value
 * @return
 */
BitIO BitIO::makeBitcoin(int8_t value, bool includeOpReturn) {
    checkValueRange((int64_t) value, (int64_t) -1, (int64_t) 16);
    if (value > 0) {
        value += 80;
    } //-1=79,0=0,1=81,2=82...
    if (value == -1)
        value = 79;
    BitIO result;
    if (includeOpReturn)
        result.appendBits(BITIO_BITCOIN_OP_RETURN, 8);
    result.appendBits(value, 8);
    return result;
}

/**
 * encodes binary data into Bitcoin format
 * @param value
 * @return
 */
BitIO BitIO::makeBitcoin(const std::vector<uint8_t>& value, bool includeOpReturn) {
    BitIO result;
    if (includeOpReturn) {
        result.appendBits(BITIO_BITCOIN_OP_RETURN, 8);
    }
    if (value.size() > 0xffff) {
        result.appendBits(0x4e, 8);
        result.appendBits(value.size(), 32);
    } else if (value.size() > 0xff) {
        result.appendBits(0x4d, 8);
        result.appendBits(value.size(), 16);
    } else if (value.size() > 75) {
        result.appendBits(0x4c, 8);
        result.appendBits(value.size(), 8);
    } else {
        result.appendBits(value.size(), 8);
    }
    for (unsigned char i: value) {
        result.appendBits(i, 8);
    }
    return result;
}

/**
 * inserts Bitcoin header to beginning of provided data
 * data provided must be a multiple of 8 bits and no more than 2^32 bytes long
 * @param value
 */
void BitIO::makeBitcoin(BitIO& value, bool includeOpReturn) {
    if (value.getLength() % 8 != 0) {
        invalidLength();
    }
    size_t length = value.getLength() / 8;
    if (length > 0xffffffff) {
        invalidLength();
    }
    value.movePositionToBeginning();
    if (length > 0xffff) {
        value.insertBits(((uint64_t) 0x4e << 32) | length, 40);
    } else if (length > 0xff) {
        value.insertBits(((uint64_t) 0x4d << 16) | length, 24);
    } else if (length > 75) {
        value.insertBits(((uint64_t) 0x4c << 8) | length, 16);
    } else {
        value.insertBits(length, 8);
    }
    value.movePositionToBeginning();
    if (includeOpReturn) {
        value.insertBits(BITIO_BITCOIN_OP_RETURN, 8);
    }
}

/**
 * Checks if next 8 bits is an op_return op code.  If it is will return true and move pointer forward 8bits.  If it is not will not move pointer and return false
 * @return
 */
bool BitIO::checkIsBitcoinOpReturn() {
    size_t pos = getPosition();
    try {
        bool isOpReturn = (getBits(8) == BITIO_BITCOIN_OP_RETURN);
        if (!isOpReturn) {
            movePositionTo(pos);
        }
        return isOpReturn;
    } catch (const std::exception& e) {
        //if there was an error than it was not an opReturn(probably not enough bits)
        movePositionTo(pos);
        return false;
    }
}

/**
 * returns -2 if data and does not move header so can be recovered with getBitcoinData command
 * returns -1 to 16 if is int value(0 or 1 may be bool but are indistinguishable) and moves pointer to after value
 * otherwise throws error
 * @return
 */
int8_t BitIO::getBitcoinDataHeader() {
    uint64_t opCode = getBits(8);
    if (opCode == 0) {
        return 0;
    }
    if (opCode == 80) {
        invalidOpCode();
    }
    if ((opCode >= 79) && (opCode <= 96)) {
        return (int8_t) opCode - 80;
    } // NOLINT(cppcoreguidelines-narrowing-conversions)
    movePositionBy(-8);     //move pointer back
    if ((opCode >= 1) && (opCode <= 78))
        return BITIO_BITCOIN_TYPE_DATA;
    invalidOpCode();
    return 0;   //unreachable but makes clang happy
}

/**
 * Retrieves Bitcoin encoded data and moves pointer to end of the data.
 * Should use getBitcoinDataHeader() to read the OP_RETURN header first and let you know if only a number was encoded
 * @return
 */
std::vector<uint8_t> BitIO::getBitcoinData() {
    std::vector<uint8_t> result;
    uint64_t opCode = getBits(8);
    if ((opCode == 0) || (opCode >= 79)) {
        invalidOpCode();
    } //valid data opcodes are 1 to 78

    //check length
    size_t length = opCode;
    if (opCode == 76)
        length = getBits(8);
    if (opCode == 77)
        length = getBits(16);
    if (opCode == 78)
        length = getBits(32);

    //get the data
    for (size_t i = 0; i < length; i++) {
        result.push_back(getBits(8));
    }
    return result;
}


/**
 * Retrieves Bitcoin encoded data and moves pointer to end of the data.
 * Should use getBitcoinDataHeader() to read the OP_RETURN header first and let you know if only a number was encoded
 * @return
 */
BitIO BitIO::copyBitcoinData() {
    BitIO result;
    uint64_t opCode = getBits(8);
    if ((opCode == 0) || (opCode >= 79)) {
        invalidOpCode();
    } //valid data opcodes are 1 to 78

    //check length
    size_t length = opCode;
    if (opCode == 76)
        length = getBits(8);
    if (opCode == 77)
        length = getBits(16);
    if (opCode == 78)
        length = getBits(32);

    //get the data
    return copyBits(length * 8);
}

/**
 * Creates a
 * @param value - value to be written.  should be a pair
 * @param xLength - number of bits per encoding segment
 * @return
 */
BitIO BitIO::makeXBit(const xBitValue& value) {
    //error check
    value.verify();

    //write prefix 0s
    BitIO result;
    result.appendZeros(value.depth * value.xBitLength); //fills with that many 0s

    //write key
    result.appendBits(value.key, value.xBitLength);
    return result;
}

/**
 * Returns value and updates pointer.
 * In this encoding scheme we take x bits at a time if not all are 0 we stop.  If they are we repeat
 * @param xLength - number of bits per encoding segment
 * @return
 */
xBitValue BitIO::getXBit(size_t xLength) {
    xBitValue result = {0, 1, xLength};
    do {
        result.key = getBits(xLength);
        result.depth++;
    } while (result.key == 0);
    result.depth--; //we did ++ one too many times
    return result;
}
