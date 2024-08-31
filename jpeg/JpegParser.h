#ifndef __JPEGPARSER_H__
#define __JPEGPARSER_H__
#include <string>
#include <vector>
#include <deque>
#include "map"
#include "functional"

enum JpegMarker {
    SOI = 0xFFD8,
    EOI = 0xFFD9,
    SOF0 = 0xFFC0,
    SOF1 = 0xFFC1,
    DHT = 0xFFC4,
    SOS = 0xFFDA,
    DQT = 0xFFDB,
    DRI = 0xFFDD,
    APP0 = 0xFFE0,
    COM = 0xFFFE,
};

class JpegParser {
    public:
        JpegParser();
        virtual ~JpegParser();
        int loadFromFile(const std::string& fileName);

        int parseSOI();
        int parseEOI();
        int parseSOF0();
        int parseSOF1();
        int parseDHT();
        int parseSOS();
        int parseDQT();
        int parseDRI();
        int parseAPP0();
        int parseCOM();

    private:
        std::map<JpegMarker, std::function<int()>> markerTable_;
        std::vector<uint8_t> data_;
        int offset_ = 0;
        int width_ = 0;
        int height_ = 0;
        std::vector<std::vector<uint8_t>> dqt;
        std::vector<uint8_t> dhtLumaDCBits;
        std::vector<uint8_t> dhtLumaDCValue;
        std::vector<uint8_t> dhtLumaACBits;
        std::vector<uint8_t> dhtLumaACValue;
        std::vector<uint8_t> dhtChromaDCBits;
        std::vector<uint8_t> dhtChromaDCValue;
        std::vector<uint8_t> dhtChromaACBits;
        std::vector<uint8_t> dhtChromaACValue;
        std::map<uint64_t, uint8_t> dhtLumaDCMap;
        std::map<uint64_t, uint8_t> dhtLumaACMap;
        std::map<uint64_t, uint8_t> dhtChromaACMap;
        std::map<uint64_t, uint8_t> dhtChromaDCMap;
        std::vector<int32_t> parseValue(std::deque<int8_t>& bits, bool dc, bool luma);
        void parseYCbCr(std::deque<int8_t>& bits, int w, int h, bool luma, std::vector<std::vector<int32_t>>& deHuff, int& dc);


};

#endif
