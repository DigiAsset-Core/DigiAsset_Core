//
// Created by mctrivia on 07/04/23.
//

#include "Blob.h"
#include <cstring>
#include <stdexcept>

Blob::Blob(const void* data, int length) {
    //reserve needed memory
    _data = (unsigned char*) malloc(length);
    if (_data == nullptr) throw std::exception(); //failed to get needed memory

    //copy data
    memcpy(_data, data, length);

    //store length
    _length = length;
}

int char2int(char input) {
    if (input >= '0' && input <= '9') {
        return input - '0';
    }
    if (input >= 'A' && input <= 'F') {
        return input - 'A' + 10;
    }
    if (input >= 'a' && input <= 'f') {
        return input - 'a' + 10;
    }
    throw std::invalid_argument("Invalid input string");
}

Blob::Blob(const std::string& hex) {
    //get number of bytes and make sure not an odd number of nibles
    _length = hex.length();
    if (_length % 2 != 0) throw std::invalid_argument("Invalid input string");
    _length /= 2;

    //convert string to byte array
    _data = (unsigned char*) malloc(_length);
    for (size_t i = 0; i < _length; i++) {
        _data[i] = char2int(hex[i * 2]) * 16 + char2int(hex[i * 2 + 1]);
    }
}

Blob::~Blob() {
    free(_data);
}

constexpr char hexmap[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                           '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

std::string Blob::toHex() const {
    std::string s(_length * 2, ' ');
    for (size_t i = 0; i < _length; ++i) {
        s[2 * i] = hexmap[(_data[i] & 0xF0) >> 4];
        s[2 * i + 1] = hexmap[_data[i] & 0x0F];
    }
    return s;
}

unsigned char* Blob::data() const {
    return _data;
}

size_t Blob::length() const {
    return _length;
}

std::vector<uint8_t> Blob::vector() const {
    std::vector<uint8_t> result(_length);
    if (_length > 0) {
        memcpy(result.data(), _data, _length);
    }
    return result;
}

Blob::Blob(const std::vector<uint8_t>& data) {
    //convert string to byte array
    _length = data.size();
    _data = (unsigned char*) malloc(data.size());
    for (size_t i = 0; i < data.size(); i++) {
        _data[i] = data[i];
    }
}

Blob::Blob(const Blob& other) : _data(nullptr), _length(0) {
    if (other._length > 0) {
        _data = (unsigned char*)malloc(other._length);
        if (_data == nullptr) throw std::bad_alloc();
        memcpy(_data, other._data, other._length);
        _length = other._length;
    }
}

Blob& Blob::operator=(const Blob& other) {
    if (this != &other) { // Protect against self-assignment
        // Free the existing resource.
        free(_data);

        _data = nullptr;
        _length = 0;

        if (other._length > 0) {
            _data = (unsigned char*)malloc(other._length);
            if (_data == nullptr) throw std::bad_alloc();
            memcpy(_data, other._data, other._length);
            _length = other._length;
        }
    }
    return *this;
}
