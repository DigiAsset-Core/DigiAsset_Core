//
// Created by mctrivia on 06/07/23.
//

#ifndef DIGIASSET_CORE_SERIALIZE_H
#define DIGIASSET_CORE_SERIALIZE_H

#include <vector>
#include <cstdint>
#include <cstddef>
#include <string>
#include <stdexcept>

using namespace std;




/**
 * Serialize function.  Takes input serializes it and adds it to the serializedData
 */
static void serialize(vector<uint8_t>& serializedData, const uint8_t& input) {
    serializedData.push_back(input);
}

static void deserialize(const vector<uint8_t>& serializedData, size_t& i, uint8_t& output) {
    if (i >= serializedData.size()) throw out_of_range("read past end of data");
    output = serializedData[i];
    i++;
}

static void serialize(vector<uint8_t>& serializedData, const uint64_t& input) {
    for (size_t shift = 56; shift > 0; shift -= 8) {
        serializedData.push_back((input >> shift) & 0xff);
    }
    serializedData.push_back(input & 0xff);
}

static void deserialize(const vector<uint8_t>& serializedData, size_t& i, uint64_t& output) {
    if (i + 8 > serializedData.size()) throw out_of_range("read past end of data");
    output = 0;
    for (size_t shift = 56; shift > 0; shift -= 8) {
        output += ((uint64_t) serializedData[i] << shift);
        i++;
    }
    output += serializedData[i];
    i++;
}

static void serialize(vector<uint8_t>& serializedData, const string& input) {
    //store number of elements
    serialize(serializedData, (uint64_t) input.size());

    //store elements
    for (const char& letter: input) {
        serialize(serializedData, (uint8_t) letter);
    }
}

static void deserialize(const vector<uint8_t>& serializedData, size_t& i, string& output) {
    //get length
    uint64_t size;
    deserialize(serializedData, i, size);

    //error check
    if (i + size > serializedData.size()) throw out_of_range("read past end of data");

    //decode element
    output.clear();
    output.resize(size);
    for (size_t ii = 0; ii < size; ii++) {
        output[ii] = serializedData[i];
        i++;
    }
}

/**
 * Generic function to serialize vector of any type
 * Still need a function to serialize the type inside the vector for this to work
 */
template<typename T>
static void serialize(vector<uint8_t>& serializedData, const vector<T>& input) {
    //store number of elements
    serialize(serializedData, (uint64_t) input.size());

    //store elements
    for (const T& element: input) {
        serialize(serializedData, element);
    }
}


template<typename T>
static void deserialize(const vector<uint8_t>& serializedData, size_t& i, vector<T>& output) {
    //get length
    uint64_t size;
    deserialize(serializedData, i, size);

    //decode element
    output.clear();
    output.resize(size);
    for (T& value: output) {
        deserialize(serializedData, i, value);
    }
}


#endif //DIGIASSET_CORE_SERIALIZE_H