#ifndef COBS_C_H
#define COBS_C_H

#include <cinttypes>

namespace cobs {

    void encode(const uint8_t* ptr, uint32_t length, uint8_t* dst);
    size_t decode(uint8_t* buffer, const size_t size);
}

#endif // COBS_C_H