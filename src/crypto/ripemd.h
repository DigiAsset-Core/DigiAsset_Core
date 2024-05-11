//
// Created by mctrivia on 30/03/24.
//

#ifndef RIPEMD160_H
#define RIPEMD160_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void ripemd160(const uint8_t* msg, uint32_t msg_len, uint8_t* hash);

#ifdef __cplusplus
}
#endif

#endif // RIPEMD160_H
