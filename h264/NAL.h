#ifndef H264_NAL_H_
#define H264_NAL_H_

#include <cstdint>
#include <vector>
class NAL {
    public:
        NAL();
        void addByte(uint8_t b);
        int size();
        int32_t unitType;
        int32_t refIdc;

        std::vector<uint8_t> bytes;
};
#endif // !H264_NAL_H_
