#include "NAL.h"
#include "assert.h"
#include <iostream>
#include <ostream>

NAL::NAL()
    :read_offset(0)
{
}

void NAL::addByte(uint8_t b)
{
    bytes.push_back(b);
    if (bytes.size() == 1) {
        read_bits(1);
        refIdc = read_bits(2);
        unitType = read_bits(5);
        if (unitType == 14 || unitType == 20){
            std::cout << "not support svc_extension" <<std::endl;
            assert(0);
        }
    }
}

int NAL::size()
{
    return bytes.size();
}

uint32_t NAL::read_bits(int size)
{
    assert(size < 32);
    int val = 0;
    while(size > 0) {
        int idx = read_offset / 8;
        int left_bits = 8 - (read_offset % 8);
        if (left_bits >= size) {
            val <<= size;
            val |= (bytes[idx] >> (left_bits - size)) & ((1<<size) - 1);
            read_offset += size;
            size = 0;
        } else {
            val <<= left_bits;
            val |= (bytes[idx] & ((1 << left_bits) - 1));
            read_offset += left_bits;
            size -= left_bits;
        }

    }
    return val;
}

uint32_t NAL::read_bits_align()
{
    int left_bits = 8 - (read_offset % 8);
    if (left_bits < 8) {
        return read_bits(left_bits);
    }

    return 0;
}


uint32_t NAL::read_ue()
{
    int zero_count = 0;
    while(read_bits(1) == 0) {
        zero_count++;
    }
    if (zero_count == 0) {
        return 0;
    }
    uint32_t val = read_bits(zero_count);
    val |= (1 << zero_count);
    val -= 1;
    return val;
}

int32_t NAL::read_se()
{
    int zero_count = 0;
    while(read_bits(1) == 0) {
        zero_count++;
    }
    if (zero_count == 0) {
        return 0;
    }
    int32_t val = read_bits(zero_count - 1);
    val |= (1 << (zero_count - 1));
    if (read_bits(1) == 1) {
        val = -val;
    }

    return val;
}

bool NAL::eof()
{
    if (read_offset == bytes.size()) {
        return true;
    }

    return false;
}
