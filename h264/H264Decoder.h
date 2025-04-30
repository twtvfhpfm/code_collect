#ifndef __H264_DECODER_H__
#define __H264_DECODER_H__
#include "NAL.h"
#include <cstdint>
#include <fstream>
#include <string>

class H264Decoder {
    public:
        explicit H264Decoder(std::string& fileName);
        virtual ~H264Decoder();
        void decode();

    private:
        std::fstream inFile;
        uint8_t profileIdc;
        uint8_t levelIdc;
        uint8_t spsId;
        void decodeNAL(NAL* n);
        void decodeSPS(NAL* n);
};

#endif // !__H264_DECODER_H__
