//
// Created by mctrivia on 07/04/23.
//

#ifndef DIGIBYTECORE_BLOB_H
#define DIGIBYTECORE_BLOB_H


#include <string>
#include <vector>

class Blob {
public:
    Blob(const void* data, int length);
    explicit Blob(const std::vector<uint8_t>& data);
    explicit Blob(const std::string& hex);
    ~Blob();
    std::string toHex();
    unsigned char* data();
    std::vector<uint8_t> vector();
    size_t length();

private:
    unsigned char* _data;
    size_t _length;
};


#endif //DIGIBYTECORE_BLOB_H
