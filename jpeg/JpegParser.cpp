#include "JpegParser.h"
#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <cstring>
#include <vector>
#include <iostream>
#include <algorithm>
#include <numeric>
#include <deque>
#include "opencv2/core.hpp"
#include "opencv2/core/matx.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgcodecs.hpp"

static char* input_img = NULL;
static const uint8_t zigzag[64] =
{
    0,   1,  5,  6, 14, 15, 27, 28,
    2,   4,  7, 13, 16, 26, 29, 42,
    3,   8, 12, 17, 25, 30, 41, 43,
    9,  11, 18, 24, 31, 40, 44, 53,
    10, 19, 23, 32, 39, 45, 52, 54,
    20, 22, 33, 38, 46, 51, 55, 60,
    21, 34, 37, 47, 50, 56, 59, 61,
    35, 36, 48, 49, 57, 58, 62, 63,
};

JpegParser::JpegParser() {
    markerTable_[SOI] = std::bind(&JpegParser::parseSOI, this);
    markerTable_[EOI] = std::bind(&JpegParser::parseEOI, this);
    markerTable_[SOF0] = std::bind(&JpegParser::parseSOF0, this);
    markerTable_[SOF1] = std::bind(&JpegParser::parseSOF1, this);
    markerTable_[DHT] = std::bind(&JpegParser::parseDHT, this);
    markerTable_[SOS] = std::bind(&JpegParser::parseSOS, this);
    markerTable_[DQT] = std::bind(&JpegParser::parseDQT, this);
    markerTable_[DRI] = std::bind(&JpegParser::parseDRI, this);
    markerTable_[APP0] = std::bind(&JpegParser::parseAPP0, this);
    markerTable_[COM] = std::bind(&JpegParser::parseCOM, this);
}

JpegParser::~JpegParser()
{}

static uint16_t readShortBE(uint8_t* ptr) {
    uint16_t val = (ptr[0] << 8) | (ptr[1]);
    return val;
}

int JpegParser::parseSOI() {
    std::cout << "parse SOI" << std::endl;
    return 0;
}

int JpegParser::parseEOI() {
    std::cout << "parse EOI" << std::endl;
    return 0;
}

int JpegParser::parseSOF0() {
    std::cout << "parse SOF0" << std::endl;
    uint16_t len = readShortBE(&data_[offset_]);
    uint8_t bitsPerSample = data_[offset_+2];
    height_ = readShortBE(&data_[offset_+3]);
    width_ = readShortBE(&data_[offset_+5]);
    std::cout << "width: " << width_ << ", height: " << height_ << std::endl;
    uint8_t components = data_[offset_+7];
    //TODO
    for(int i = 0; i < components;i++) {
        int componentId = data_[offset_+8+3*i];
        int smpV = data_[offset_+9+3*i] & 0xf;
        int smpH = data_[offset_+9+3*i] >> 4;
        int qtIdx = data_[offset_ + 10+3*i];
        std::cout << "componentId: " << componentId << ", smpV: " << smpV
            << ", smpH: " << smpH << ", qtIdx: " << qtIdx << std::endl;
    }
    offset_ += len;
    return 0;
}

int JpegParser::parseSOF1() {
    std::cout << "parse SOF1" << std::endl;
    uint16_t len = readShortBE(&data_[offset_]);
    offset_ += len;
    return 0;
}

int JpegParser::parseDHT() {
    std::cout << "parse DHT" << std::endl;
    uint16_t len = readShortBE(&data_[offset_]);
    uint8_t htIdx = data_[offset_+2] & 0xf; //0: luma 1: chrome
    uint8_t htType = (data_[offset_+2] >> 4) & 0x1; //0: dc, 1: ac
    assert(htIdx < 2 && htType < 2);
    std::vector<uint8_t>* bits = nullptr;
    std::vector<uint8_t>* values = nullptr;
    std::map<uint64_t, uint8_t>* dhtMap = nullptr;
    if (htIdx == 0 && htType == 0) {
        bits = &dhtLumaDCBits;
        values = &dhtLumaDCValue;
        dhtMap = &dhtLumaDCMap;
        std::cout << "LumaDC" << std::endl;
    } else if (htIdx == 0 && htType == 1) {
        bits = &dhtLumaACBits;
        values = &dhtLumaACValue;
        dhtMap = &dhtLumaACMap;
        std::cout << "LumaAC" << std::endl;
    } else if (htIdx == 1 && htType == 1) {
        bits = &dhtChromaACBits;
        values = &dhtChromaACValue;
        dhtMap = &dhtChromaACMap;
        std::cout << "ChromaAC" << std::endl;
    } else if (htIdx == 1 && htType == 0) {
        bits = &dhtChromaDCBits;
        values = &dhtChromaDCValue;
        dhtMap = &dhtChromaDCMap;
        std::cout << "ChromaDC" << std::endl;
    }
    assert(bits != nullptr && values != nullptr);
    bits->insert(bits->end(), &data_[offset_+3], &data_[offset_+3+16]);
    auto sum = std::accumulate(bits->begin(), bits->end(), 0,
            [](int carry, int n){ return carry + n;});
    values->insert(values->end(), &data_[offset_+3+16], &data_[offset_+3+16+sum]);
    std::cout << "len: " << len << ", sum: " << sum << std::endl;
    assert(len == 3+16+sum);

    uint64_t code = 0x10000;
    int idx = 0;
    for (int i = 0; i < 16; i++) {
        int num = (*bits)[i];
        int n_bits = i + 1;

        for (int j = 0; j < num; j++) {
            uint8_t symbol = (*values)[idx++];
            (*dhtMap)[code] = symbol;

            code++;
        }
        code <<= 1;
    }

    offset_ += len;
    return 0;
}

static int slow_idct(int u, int v, int* data)
{
#define kPI 3.14159265f
    float res = 0.0f;
    for ( int y = 0; y < 8; ++y ) {
        for ( int x = 0; x < 8; ++x ) {
            float cx = (x == 0) ? 0.70710678118654f : 1;
            float cy = (y == 0) ? 0.70710678118654f : 1;
            res += (data[y * 8 + x]) * cx * cy *
                    cosf(((2.0f * u + 1.0f) * x * kPI) / 16.0f) *
                    cosf(((2.0f * v + 1.0f) * y * kPI) / 16.0f);
        }
    }
    res *= 0.25f;
    return res;
#undef kPI
}


void JpegParser::parseYCbCr(std::deque<int8_t>& bits, int w, int h, bool isLuma, std::vector<std::vector<int32_t>>& deHuff, int& dc)
{
    bool isDC = false;
    std::vector<int32_t> buf;
    while (buf.size() < 64) {
        if (buf.size() == 0) {
            isDC = true;
        } else {
            isDC = false;
        }
        auto values = parseValue(bits, isDC, isLuma);
        if (values.size() == 0) {
            //all 0 behind
            buf.insert(buf.end(), 64 - buf.size(), 0);
        } else {
            buf.insert(buf.end(), values.begin(), values.end());
        }
        // std::cout << "buf.size=" << buf.size() << std::endl;
    }
    assert(buf.size() == 64);

    int idctData[64];
    for(int i = 0; i < 8; i++) {
        for(int j = 0; j < 8; j++) {
            if (i == 0 && j == 0) {
                dc = dc + buf[0];
                idctData[i*8 + j] = dc;
            } else {
                idctData[i*8 + j] = buf[zigzag[i*8+j]];
            }
            if (isLuma) {
                idctData[i*8 + j] *= dqt[0][zigzag[i*8+j]];
            } else {
                idctData[i*8 + j] *= dqt[1][zigzag[i*8+j]];
            }
        }
    }
    int bias = isLuma ? 128 : 0;
    for(int i = 0; i < 8; i++){
        for(int j = 0; j < 8; j++) {
            deHuff[h+i][w+j] = slow_idct(j, i, idctData) + bias;
        }
    }
}

int JpegParser::parseSOS() {
    std::cout << "parse SOS" << std::endl;
    uint16_t len = readShortBE(&data_[offset_]);
    uint8_t components = data_[offset_+2];
    for(int i = 0; i < components; i++) {
        uint8_t id = data_[offset_+3+2*i];
        uint8_t dhtAC = data_[offset_+3+2*i+1] & 0xf;
        uint8_t dhtDC = data_[offset_+3+2*i+1] >> 4;
        std::cout << "id: " << static_cast<int>(id)
            << ", dhtAC: " << static_cast<int>(dhtAC)
            << ", dhtDC: " << static_cast<int>(dhtDC) << std::endl;
    }
    //TODO parse y,cb,cr
    assert(3 + 2*components + 3 == len);
    offset_ += len;

    int widthAlign = (width_ + 7) & (~7);
    int heightAlign = (height_ + 7) &(~7);
    std::vector<std::vector<int32_t>> deHuffY(heightAlign, std::move(std::vector<int32_t>(widthAlign , 0)));
    std::vector<std::vector<int32_t>> deHuffB(heightAlign, std::move(std::vector<int32_t>(widthAlign , 0)));
    std::vector<std::vector<int32_t>> deHuffR(heightAlign, std::move(std::vector<int32_t>(widthAlign , 0)));
    int dcY = 0;
    int dcB = 0;
    int dcR = 0;
    std::deque<int8_t> bits;
    for (int h = 0; h < heightAlign; h+= 8) {
        for (int w = 0; w < widthAlign; w+=8) {
            // std::cout << "h: " << h << ", w: " << w << ", offset: " << offset_ << std::endl;
            parseYCbCr(bits, w, h, true, deHuffY, dcY);
            parseYCbCr(bits, w, h, false, deHuffB, dcB);
            parseYCbCr(bits, w, h, false, deHuffR, dcR);
        }
    }

    std::vector<std::vector<int32_t>> R(height_, std::move(std::vector<int32_t>(width_, 0)));
    std::vector<std::vector<int32_t>> G(height_, std::move(std::vector<int32_t>(width_, 0)));
    std::vector<std::vector<int32_t>> B(height_, std::move(std::vector<int32_t>(width_, 0)));
    for(int h = 0; h < height_; h++) {
        for(int w = 0; w < width_; w++) {
            R[h][w] = deHuffY[h][w] + 1.402 * deHuffR[h][w];
            G[h][w] = deHuffY[h][w] - 0.34414 * deHuffB[h][w] - 0.71414 * deHuffR[h][w];
            B[h][w] = deHuffY[h][w] + 1.772 * deHuffB[h][w];
        }
    }
    std::cout << "left bits: " << bits.size() << std::endl;
    if (bits.size() > 0) {
        offset_ -= bits.size() / 8;
    }

    cv::Mat img = cv::Mat(height_, width_, CV_8UC3, cv::Scalar(255,0,0));
    for(int h = 0; h < height_; h++) {
        for(int w = 0; w < width_; w++) {
            img.at<cv::Vec3b>(h, w) = cv::Vec3b(B[h][w], G[h][w], R[h][w]);
        }
    }
    cv::imshow("img", img);
    cv::Mat origImg = cv::imread(std::string(input_img));
    cv::imshow("orig", origImg);
    cv::waitKey();

    return 0;
}

std::vector<int32_t> JpegParser::parseValue(std::deque<int8_t>& bits, bool dc, bool luma)
{
    static int total_bits = 0;
    std::map<uint64_t, uint8_t>* dhtMap = nullptr;
    if (dc && luma) {
        dhtMap = &dhtLumaDCMap;
    } else if (!dc && luma) {
        dhtMap = &dhtLumaACMap;
    } else if (dc && !luma) {
        dhtMap = &dhtChromaDCMap;
    } else {
        dhtMap = &dhtChromaACMap;
    }
    while(bits.size() < 32 && offset_ < data_.size()) {
        // std::cout << "push offset: " << offset_ << ", data: " << (int)data_[offset_] << std::endl;
        uint8_t b = data_[offset_++];
        for(int i = 7; i >= 0; i--) {
            bits.push_back((b>>i) & 0x1);
        }
        if (b == 0xff && data_[offset_] == 0) {
            // std::cout << "find 0xff" << std::endl;
            offset_++;
        }
    }
    // std::cout << "bits after push " << bits.size() << std::endl;
    assert(bits.size() > 0);
    uint64_t c = 0x10000;
    int bitsSize1 = bits.size();
    do {
        c = c | (bits[0] & 0x1);
        bits.pop_front();
        total_bits++;
        assert(bits.size() >= 16);
        // std::cout << "c=" << std::hex << c << std::dec << std::endl;
    } while (dhtMap->find(c) == dhtMap->end() && (c = c << 1));
    int bitsSize2 = bits.size();
    // std::cout << "use bits: " << bitsSize1 - bitsSize2 << std::endl;
    // std::cout << "total bits: " << total_bits << std::endl;
    assert(bitsSize1 - bitsSize2 <= 16);

    uint8_t value = dhtMap->at(c);
    int zeroNum = (value >> 4);
    int bitNum = value & 0xf;
    std::vector<int32_t> ret;
    // std::cout << "zeroNum: " << zeroNum << std::endl;
    // assert(zeroNum < 10);
    for(int i = 0; i < zeroNum; i++) {
        ret.push_back(0);
    }
    int32_t num = 0;
    bool negative = false;
    // std::cout << "bitNum: " << bitNum << ", bits: " << bits.size() << std::endl;
    for(int i =0; i < bitNum; i++) {
        if (i == 0 && bits[0] == 0) {
            negative = true;
        }
        if (negative) {
            num = (num << 1) | (1-(bits[0] & 0x1));
        } else {
            num = (num << 1) | (bits[0] & 0x1);
        }
        bits.pop_front();
        total_bits++;
    }
    if (negative) {
        num = -num;
    }
    // std::cout << "num: " << num << std::endl;
    ret.push_back(num);

    if (!dc && ret.size() == 1 && ret[0]==0) {
        ret.clear();
    }
    return ret;
}

int JpegParser::parseDQT() {
    std::cout << "parse DQT" << std::endl;
    uint16_t len = readShortBE(&data_[offset_]);
    uint8_t qtIdx = data_[offset_+2] & 0xf;
    uint8_t qtPrecision = data_[offset_+2] >> 4;
    assert(qtPrecision == 0 || qtPrecision == 1);

    assert(qtIdx == dqt.size());
    dqt.push_back(std::vector<uint8_t>(&data_[offset_+3],
                &data_[offset_+3+64*(qtPrecision+1)]));
    assert(len == 3+64*(qtPrecision+1));
    offset_ += len;
    return 0;
}

int JpegParser::parseDRI() {
    std::cout << "parse DRI" << std::endl;
    uint16_t len = readShortBE(&data_[offset_]);
    offset_ += len;
    return 0;
}

int JpegParser::parseAPP0() {
    std::cout << "parse APP0" << std::endl;
    uint16_t len = readShortBE(&data_[offset_]);
    offset_ += 2;
    assert(len >= 16);

    uint8_t flag[5] = {'J', 'F', 'I', 'F', 0};
    assert(memcmp(flag, &data_[offset_], 5) == 0);
    offset_ += 5;
    assert(data_[offset_++] == 1); //major version
    assert(data_[offset_++] > 0); //minor version
    offset_ += 5;
    uint8_t thumbX = data_[offset_++];
    uint8_t thumbY = data_[offset_++];
    offset_ += 3 * thumbX * thumbY;
    return 0;
}

int JpegParser::parseCOM() {
    std::cout << "parse COM" << std::endl;
    uint16_t len = readShortBE(&data_[offset_]);
    offset_ += len;
    return 0;
}

int JpegParser::loadFromFile(const std::string& fileName) {
    FILE* fp = fopen(fileName.c_str(), "r");
    assert(fp != NULL);
    fseek(fp, 0, SEEK_END);
    int64_t fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    data_.insert(data_.end(), fileSize, 0);

    int ret = fread(data_.data(), fileSize, 1, fp);
    assert(ret == 1);
    fclose(fp);

    while ( offset_ < fileSize - 1 ) {
        JpegMarker marker = (JpegMarker)readShortBE(&data_[offset_]);
        auto it = markerTable_.find(marker);
        offset_ += 2;
        if (it == markerTable_.end()) {
            std::cout << "unknown marker: " << std::hex << marker << std::endl;
            uint16_t len = readShortBE(&data_[offset_]);
            offset_ += len;
        } else {
            it->second();
        }
    }
    std::cout << "parse finish, offset = " << offset_ << std::endl;

    return 0;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "usage: <jpg>" << std::endl;
        return -1;
    }
    input_img = argv[1];
    JpegParser parser;
    parser.loadFromFile(input_img);
    return 0;
}
