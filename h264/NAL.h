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
        uint32_t read_bits(int size);
        uint32_t read_bits_align();
        uint32_t read_ue();
        int32_t read_se();
        bool eof();

    private:
        std::vector<uint8_t> bytes;
        int read_offset; //bit
};
#endif // !H264_NAL_H_
