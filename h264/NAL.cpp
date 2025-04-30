#include "NAL.h"

NAL::NAL()
{
}

void NAL::addByte(uint8_t b)
{
    bytes.push_back(b);
    if (bytes.size() == 1) {
        unitType = b & 0x1f;
        refIdc = (b >> 5) & 0x3;
    }
}

int NAL::size()
{
    return bytes.size();
}
