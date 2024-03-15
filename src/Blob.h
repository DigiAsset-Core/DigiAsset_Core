//
// Created by mctrivia on 07/04/23.
//

#ifndef DIGIBYTECORE_BLOB_H
#define DIGIBYTECORE_BLOB_H


#include <cstring>
#include <string>
#include <vector>

class Blob {
public:
    Blob(const void* data, int length);
    explicit Blob(const std::vector<uint8_t>& data);
    explicit Blob(const std::string& hex);
    ~Blob();
    std::string toHex() const;
    unsigned char* data() const;
    std::vector<uint8_t> vector() const;
    size_t length() const;

    Blob(const Blob& other);
    Blob& operator=(const Blob& other);


    bool operator==(const Blob& b) const {
        if (b._length != _length) return false;
        if (_data != b._data) {
            return memcmp(_data, b._data, _length) == 0;
        }
        return true;
    }

    bool operator!=(const Blob& b) const {
        return !(*this == b);
    }

private:
    unsigned char* _data;
    size_t _length;
};


#endif //DIGIBYTECORE_BLOB_H
