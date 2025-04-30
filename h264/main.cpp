#include "H264Decoder.h"
#include <string>

int main(int argc, char** argv)
{
    std::string fileName("a.h264");
    H264Decoder decoder(fileName);
    decoder.decode();

    return 0;

}
