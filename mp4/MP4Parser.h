/* Copyright 2024 xujiannan */
#ifndef MP4_MP4PARSER_H__
#define MP4_MP4PARSER_H__
#include <string>
#include <vector>

namespace mp4parser {
    class Box {
        public:
            std::string type_;
            std::vector<uint8_t> data_;
            std::vector<Box*> child_;

        public:
            Box(char* type, uint8_t* data, int len);
            virtual ~Box();
            void addChild(Box*);
            void removeChild(std::string type);
            int size();
            std::vector<Box*> findChild(std::string type);
    };

    struct Sample {
        std::vector<uint8_t> data;
        int timestamp;
        bool key;
    };

    enum TrackType {
        VIDEO,
        AUDIO,
        SUBTITLE,
    };

    struct Track {
        TrackType type;
        std::vector<Sample> samples;
    };

    class MP4Parser {
        public:
            MP4Parser();
            virtual ~MP4Parser();
            int loadFile(std::string path);
            int saveFile(std::string path);
            int replaceAudio(std::vector<uint8_t>& data, int sampleRate, int format);
            int replaceVideo(std::vector<std::vector<uint8_t>>& data);
            int writeBoxRecursive(FILE* fp, Box* b);

            Box root_;
            std::vector<Track> tracks_;
        private:
            int parseBoxRecursive(FILE* fp, Box* parent, long index, long size);
            void printBox(Box* b, int depth);
            int parseData(FILE* fp);
            int storeData();
            void dumpAV();

    };

}
#endif  // MP4_MP4PARSER_H__
