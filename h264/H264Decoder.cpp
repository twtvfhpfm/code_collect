#include "H264Decoder.h"
#include "NAL.h"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>

H264Decoder::H264Decoder(std::string& fileName)
{
    inFile.open(fileName, inFile.in | inFile.binary);
}

H264Decoder::~H264Decoder()
{

}

void H264Decoder::decode()
{
    if (!inFile.is_open()) {
        std::cout << "failed to open input file" << std::endl;
        return;
    }

    std::vector<uint8_t> buffer;
    NAL* nal = NULL;
    while(1) {

        char byte;
        inFile.get(byte);
        if (inFile.eof()) {
            break;
        }
        buffer.push_back(byte);
        if (buffer.size() == 4) {
            if (buffer[0] == 0 && buffer[1] == 0 && buffer[2] == 0 && buffer[3] == 1) {
                // find start code
                if (nal != NULL) {
                    decodeNAL(nal);
                    delete nal;
                }
                nal = new NAL();
                buffer.clear();
            } else if (buffer[0] == 0 && buffer[1] == 0 && buffer[2] == 1) {
                if (nal != NULL) {
                    decodeNAL(nal);
                    delete nal;
                }
                nal = new NAL();
                buffer.erase(buffer.begin());
                buffer.erase(buffer.begin());
                buffer.erase(buffer.begin());
            } else if (buffer[0] == 0 && buffer[1] == 0 && buffer[2] == 3) {
                nal->addByte(buffer[0]);
                nal->addByte(buffer[1]);
                buffer.erase(buffer.begin());
                buffer.erase(buffer.begin());
                buffer.erase(buffer.begin());
            } else {
                nal->addByte(buffer[0]);
                buffer.erase(buffer.begin());
            }
        } else if (buffer.size() == 3) {
            if (buffer[0] == 0 && buffer[1] == 0 && buffer[2] == 1) {
                // find start code
                if (nal != NULL) {
                    decodeNAL(nal);
                    delete nal;
                }
                nal = new NAL();
                buffer.clear();
            } else if (buffer[0] == 0 && buffer[1] == 0 && buffer[2] == 3) {
                nal->addByte(buffer[0]);
                nal->addByte(buffer[1]);
                buffer.clear();
            }
        }
    }
    if (nal != NULL) {
        for(int i = 0; i < buffer.size(); i++) {
            nal->addByte(buffer[i]);
        }
        decodeNAL(nal);
        delete nal;
    }

}

void H264Decoder::decodeNAL(NAL* n)
{
    std::cout << "nal type: " << n->unitType << ", ref: " << n->refIdc << ", size: " << n->size() << std::endl;
    if (n->unitType == 7) {
        decodeSPS(n);
    }
}

void H264Decoder::decodeSPS(NAL* n)
{
    int off = 1;
    profileIdc = n->bytes[off++];
    off++; //constraint set flag
    levelIdc = n->bytes[off++];
}
