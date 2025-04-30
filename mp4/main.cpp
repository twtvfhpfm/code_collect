#include "MP4Parser.h"
#include <cassert>
#include <iostream>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>

int parseMP4(int argc, char** argv)
{
    mp4parser::MP4Parser parser;
    parser.loadFile(argv[1]);

    // parser.saveFile(argv[2]);
    // auto traks = parser.root_.findChild("trak");
    // for(auto trak: traks) {
    //     auto vmhd = trak->findChild("vmhd");
    //     if (vmhd.size() == 1) {
    //         auto elst = trak->findChild("elst").front();
    //         if (elst->data_.size() > 20) {
    //             std::cout << argv[1] << " !!!!!!!" << std::endl;
    //         } else {
    //             std::cout << "video elst normal" << std::endl;
    //         }
    //     }
    // }
    return 0;
}

int replaceMP4(int argc, char** argv)
{
    mp4parser::MP4Parser parser;
    parser.loadFile(argv[1]);

    FILE* fp = fopen("demo_audio.raw", "r");
    assert(fp != NULL);
    fseek(fp, 0, SEEK_END);
    long fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    std::vector<uint8_t> audioData(fileSize, 0);
    fread(audioData.data(), fileSize, 1, fp);
    fclose(fp);
    parser.replaceAudio(audioData, 8000, 0);

    std::vector<std::vector<uint8_t>> videoData;
    for(int i = 1; i < 1801; i++) {
        char name[32];
        snprintf(name, sizeof(name), "jpegs/foo_%05d.jpeg", i);
        FILE* fp = fopen(name, "r");
        assert(fp != NULL);
        fseek(fp, 0, SEEK_END);
        long fileSize = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        std::vector<uint8_t> data(fileSize, 0);
        fread(data.data(), fileSize, 1, fp);
        fclose(fp);
        videoData.push_back(std::move(data));
    }
    parser.replaceVideo(videoData);

    parser.saveFile(argv[2]);
    // auto traks = parser.root_.findChild("trak");
    // for(auto trak: traks) {
    //     auto vmhd = trak->findChild("vmhd");
    //     if (vmhd.size() == 1) {
    //         auto elst = trak->findChild("elst").front();
    //         if (elst->data_.size() > 20) {
    //             std::cout << argv[1] << " !!!!!!!" << std::endl;
    //         } else {
    //             std::cout << "video elst normal" << std::endl;
    //         }
    //     }
    // }
    return 0;
}

int buildMP4(int argc, char** argv) {
    mp4parser::MP4Parser parser;
    uint8_t buf[1024];
    int len = 0;

    memcpy(buf + len, "isom", 4); //major brand
    len += 4;
    memcpy(buf + len, "\x20\x24\x08\x00", 4); //minor version
    len += 4;
    memcpy(buf + len, "isomiso2mp41", 12); //compatible brands
    len += 12;
    mp4parser::Box ftypBox("ftyp", buf, len);
    parser.root_.addChild(&ftypBox);

    std::vector<int> frameSize;
    int ts = 0;
    std::vector<uint8_t> videoData;
    int offset = 0;
    for(int i = 1; i < 802; i+=200) {
        char name[32];
        snprintf(name, sizeof(name), "jpegs/foo_%05d.jpeg", i);
        FILE* fp = fopen(name, "r");
        assert(fp != NULL);
        fseek(fp, 0, SEEK_END);
        long fileSize = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        frameSize.push_back(fileSize);
        videoData.insert(videoData.end(), fileSize, 0);
        fread(videoData.data() + offset, fileSize, 1, fp);
        offset += fileSize;
        fclose(fp);
    }
    char duration_1000[] = "\x00\x00\x13\x88"; //5s
    char duration_90000[] = "\x00\x06\xdd\xd0";

    mp4parser::Box mdatBox("mdat", videoData.data(), videoData.size());
    parser.root_.addChild(&mdatBox);

    mp4parser::Box moovBox("moov", nullptr, 0);
    parser.root_.addChild(&moovBox);

    len = 0;
    memcpy(buf + len, "\0\0\0\0", 4); //version and flag
    len += 4;
    memcpy(buf + len, "\xe2\xb6\x3d\x3a", 4); //create time, seconds since 1904.01.01
    len += 4;
    memcpy(buf + len, "\xe2\xb6\x3d\x3a", 4); //modify time
    len += 4;
    memcpy(buf + len, "\x00\x00\x03\xe8", 4); //timescale 1000
    len += 4;
    memcpy(buf + len, duration_1000, 4); //duration
    len += 4;
    memcpy(buf + len, "\x00\x01\x00\xe0", 4); //rate 1.0
    len += 4;
    memcpy(buf + len, "\x01\x00", 2); //volume 1.0
    len += 2;
    memcpy(buf + len, "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 10); // reserved
    len += 10;
    memcpy(buf + len, "\x00\x01\x00\x00", 4); //matrix 16.16
    len += 4;
    memcpy(buf + len, "\x00\x00\x00\x00", 4); //matrix 16.16
    len += 4;
    memcpy(buf + len, "\x00\x00\x00\x00", 4); //matrix 2.30
    len += 4;
    memcpy(buf + len, "\x00\x00\x00\x00", 4); //matrix 16.16
    len += 4;
    memcpy(buf + len, "\x00\x01\x00\x00", 4); //matrix 16.16
    len += 4;
    memcpy(buf + len, "\x00\x00\x00\x00", 4); //matrix 2.30
    len += 4;
    memcpy(buf + len, "\x00\x00\x00\x00", 4); //matrix 16.16
    len += 4;
    memcpy(buf + len, "\x00\x00\x00\x00", 4); //matrix 16.16
    len += 4;
    memcpy(buf + len, "\x40\x00\x00\x00", 4); //matrix 2.30
    len += 4;
    memset(buf + len, 0, 24); // many times
    len += 24;
    memcpy(buf + len, "\x00\x00\x00\x02", 4); //next track id
    len += 4;
    mp4parser::Box mvhdBox("mvhd", buf, len);
    moovBox.addChild(&mvhdBox);

    mp4parser::Box trakBox("trak", nullptr, 0);
    moovBox.addChild(&trakBox);

    len = 0;
    memcpy(buf + len, "\x00\x00\x00\x03", 4); //version and flag, track enable, track in movie
    len+=4;
    memcpy(buf + len, "\xe2\xb6\x3d\x3a", 4); //create time, seconds since 1904.01.01
    len += 4;
    memcpy(buf + len, "\xe2\xb6\x3d\x3a", 4); //modify time
    len += 4;
    memcpy(buf + len, "\x00\x00\x00\x01", 4); //track id
    len += 4;
    memcpy(buf + len, "\x00\x00\x00\x00", 4); //reserve
    len += 4;
    memcpy(buf + len, duration_1000, 4); //duration, 3000ms
    len += 4;
    memset(buf + len, 0, 8); //reserve
    len += 8;
    memcpy(buf + len, "\x00\x00", 2); //layer
    len += 2;
    memcpy(buf + len, "\x00\x00", 2); //alternate group
    len += 2;
    memcpy(buf + len, "\x00\x00", 2); //volume
    len += 2;
    memcpy(buf + len, "\x00\x00", 2); //reserve
    len += 2;
    memcpy(buf + len, "\x00\x01\x00\x00", 4); //matrix 16.16
    len += 4;
    memcpy(buf + len, "\x00\x00\x00\x00", 4); //matrix 16.16
    len += 4;
    memcpy(buf + len, "\x00\x00\x00\x00", 4); //matrix 2.30
    len += 4;
    memcpy(buf + len, "\x00\x00\x00\x00", 4); //matrix 16.16
    len += 4;
    memcpy(buf + len, "\x00\x01\x00\x00", 4); //matrix 16.16
    len += 4;
    memcpy(buf + len, "\x00\x00\x00\x00", 4); //matrix 2.30
    len += 4;
    memcpy(buf + len, "\x00\x00\x00\x00", 4); //matrix 16.16
    len += 4;
    memcpy(buf + len, "\x00\x00\x00\x00", 4); //matrix 16.16
    len += 4;
    memcpy(buf + len, "\x40\x00\x00\x00", 4); //matrix 2.30
    len += 4;
    memcpy(buf + len, "\x09\x00\x00\x00", 4); //width, 16.16
    len += 4;
    memcpy(buf + len, "\x05\x10\x00\x00", 4); //height, 16.16
    len += 4;
    mp4parser::Box tkhdBox("tkhd", buf, len);
    trakBox.addChild(&tkhdBox);

    mp4parser::Box edtsBox("edts", nullptr, 0);
    trakBox.addChild(&edtsBox);

    len = 0;
    memcpy(buf + len, "\x00\x00\x00\x00", 4); //version and flag
    len += 4;
    memcpy(buf + len, "\x00\x00\x00\x01", 4); //entry count
    len += 4;
    memcpy(buf + len, duration_1000, 4); //duration, 3000ms
    len += 4;
    memcpy(buf + len, "\x00\x00\x00\x00", 4); //media time
    len += 4;
    memcpy(buf + len, "\x00\x01\x00\x00", 4); //media rate, 16.16
    len += 4;
    mp4parser::Box elstBox("elst", buf, len);
    edtsBox.addChild(&elstBox);

    mp4parser::Box mdiaBox("mdia", nullptr, 0);
    trakBox.addChild(&mdiaBox);

    len = 0;
    memcpy(buf + len, "\x00\x00\x00\x00", 4); //version and flag
    len += 4;
    memcpy(buf + len, "\xe2\xb6\x3d\x3a", 4); //create time, seconds since 1904.01.01
    len += 4;
    memcpy(buf + len, "\xe2\xb6\x3d\x3a", 4); //modify time
    len += 4;
    memcpy(buf + len, "\x00\x01\x5f\x90", 4); //timescale, 90000
    len += 4;
    memcpy(buf + len, duration_90000, 4); //duration, 270000
    len += 4;
    memcpy(buf + len, "\x00\x21", 2); //language
    len += 2;
    memcpy(buf + len, "\x00\x00", 2); //quality
    len += 2;
    mp4parser::Box mdhdBox("mdhd", buf, len);
    mdiaBox.addChild(&mdhdBox);

    len = 0;
    memcpy(buf + len, "\x00\x00\x00\x00", 4); //version and flag
    len += 4;
    memcpy(buf + len, "\x00\x00\x00\x00", 4); //predefined
    len += 4;
    memcpy(buf + len, "vide", 4); //handler type
    len += 4;
    memset(buf + len, 0, 12); //reserved
    len += 12;
    memcpy(buf + len, "VideoHandler", 12); //name
    len += 12;
    mp4parser::Box hdlrBox("hdlr", buf, len);
    mdiaBox.addChild(&hdlrBox);

    mp4parser::Box minfBox("minf", nullptr, 0);
    mdiaBox.addChild(&minfBox);

    len = 0;
    memcpy(buf + len, "\x00\x00\x00\x01", 4); //version and flag
    len += 4;
    memcpy(buf + len, "\x00\x00", 2); //graphics mode
    len += 2;
    memcpy(buf + len, "\x00\x00", 2); //opcode
    len += 2;
    memcpy(buf + len, "\x00\x00", 2); //opcode
    len += 2;
    memcpy(buf + len, "\x00\x00", 2); //opcode
    len += 2;
    mp4parser::Box vmhdBox("vmhd", buf, len);
    minfBox.addChild(&vmhdBox);

    mp4parser::Box dinfBox("dinf", nullptr, 0);
    minfBox.addChild(&dinfBox);

    len = 0;
    memcpy(buf + len, "\x00\x00\x00\x00", 4); //version and flag
    len += 4;
    memcpy(buf + len, "\x00\x00\x00\x01", 4); //entry count
    len += 4;
    mp4parser::Box drefBox("dref", buf, len);
    dinfBox.addChild(&drefBox);

    len = 0;
    memcpy(buf + len, "\x00\x00\x00\x01", 4); //version and flag
    len += 4;
    mp4parser::Box urlBox("url ", buf, len);
    drefBox.addChild(&urlBox);

    mp4parser::Box stblBox("stbl", nullptr, 0);
    minfBox.addChild(&stblBox);

    len = 0;
    memcpy(buf + len, "\x00\x00\x00\x00", 4); //version and flag
    len += 4;
    memcpy(buf + len, "\x00\x00\x00\x01", 4); //entry count
    len += 4;
    mp4parser::Box stsdBox("stsd", buf, len);
    stblBox.addChild(&stsdBox);

    len = 0;
    memcpy(buf + len, "\x00\x00\x00\x00", 4); //reserved
    len += 4;
    memcpy(buf + len, "\x00\x00", 2); //reserved
    len += 2;
    memcpy(buf + len, "\x00\x01", 2); //dref id
    len += 2;
    memcpy(buf + len, "\x00\x00\x00\x00", 4); //version and revision level
    len += 4;
    memcpy(buf + len, "\x00\x00\x00\x00", 4); //vendor
    len += 4;
    memcpy(buf + len, "\x00\x00\x00\x00", 4); //temporal quality
    len += 4;
    memcpy(buf + len, "\x00\x00\x00\x00", 4); //spatial quality
    len += 4;
    memcpy(buf + len, "\x09\x00", 2); //width
    len += 2;
    memcpy(buf + len, "\x05\x10", 2); //height
    len += 2;
    memcpy(buf + len, "\x00\x00\x00\x00", 4); //horiz resolution
    len += 4;
    memcpy(buf + len, "\x00\x00\x00\x00", 4); //vertical resolution
    len += 4;
    memcpy(buf + len, "\x00\x00\x00\x00", 4); //data size
    len += 4;
    memcpy(buf + len, "\x00\x01", 2); //frames per sample
    len += 2;
    memset(buf + len, 0, 32); //codec name
    len += 32;
    memcpy(buf + len, "\x00\x18", 2); //bits per sample
    len += 2;
    memcpy(buf + len, "\xff\xff", 2); //color table id
    len += 2;
    mp4parser::Box jpegBox("jpeg", buf, len);
    stsdBox.addChild(&jpegBox);

    len = 0;
    memcpy(buf + len, "\x00\x00\x00\x00", 4); //version and flag
    len += 4;
    memcpy(buf + len, "\x00\x00\x00\x05", 4); //entry count
    len += 4;
    memcpy(buf + len, "\x00\x00\x00\x01", 4); //sample count
    len += 4;
    memcpy(buf + len, "\x00\x01\x5f\x90", 4); //sample duration
    len += 4;
    memcpy(buf + len, "\x00\x00\x00\x01", 4); //sample count
    len += 4;
    memcpy(buf + len, "\x00\x01\x5f\x90", 4); //sample duration
    len += 4;
    memcpy(buf + len, "\x00\x00\x00\x01", 4); //sample count
    len += 4;
    memcpy(buf + len, "\x00\x01\x5f\x90", 4); //sample duration
    len += 4;
    memcpy(buf + len, "\x00\x00\x00\x01", 4); //sample count
    len += 4;
    memcpy(buf + len, "\x00\x01\x5f\x90", 4); //sample duration
    len += 4;
    memcpy(buf + len, "\x00\x00\x00\x01", 4); //sample count
    len += 4;
    memcpy(buf + len, "\x00\x01\x5f\x90", 4); //sample duration
    len += 4;
    mp4parser::Box sttsBox("stts", buf, len);
    stblBox.addChild(&sttsBox);

    len = 0;
    memcpy(buf + len, "\x00\x00\x00\x00", 4); //version and flag
    len += 4;
    memcpy(buf + len, "\x00\x00\x00\x01", 4); //entry count
    len += 4;
    memcpy(buf + len, "\x00\x00\x00\x01", 4); //first chunk
    len += 4;
    memcpy(buf + len, "\x00\x00\x00\x05", 4); //samples per chunk
    len += 4;
    memcpy(buf + len, "\x00\x00\x00\x01", 4); //samples description index
    len += 4;
    mp4parser::Box stscBox("stsc", buf, len);
    stblBox.addChild(&stscBox);

    len = 0;
    memcpy(buf + len, "\x00\x00\x00\x00", 4); //version and flag
    len += 4;
    memcpy(buf + len, "\x00\x00\x00\x00", 4); //sample size
    len += 4;
    memcpy(buf + len, "\x00\x00\x00\x05", 4); //sample count
    len += 4;
    for(int i = 0; i < 5; i++) {
        int size = htonl(frameSize[i]);
        memcpy(buf + len, &size, 4);
        len += 4;
    }
    mp4parser::Box stszBox("stsz", buf, len);
    stblBox.addChild(&stszBox);

    len = 0;
    memcpy(buf + len, "\x00\x00\x00\x00", 4); //version and flag
    len += 4;
    memcpy(buf + len, "\x00\x00\x00\x01", 4); //entry count
    len += 4;
    memcpy(buf + len, "\x00\x00\x00\x24", 4); //chunk offset
    len += 4;
    mp4parser::Box stcoBox("stco", buf, len);
    stblBox.addChild(&stcoBox);

    FILE* fp = fopen("build_output1.mp4", "w");
    assert(fp != NULL);
    std::cout << "--------------output mp4-------------||" << std::endl;
    for(auto b: parser.root_.child_) {
        parser.writeBoxRecursive(fp, b);
    }
    fclose(fp);
    return 0;
}

int main(int argc, char** argv) 
{
    // return parseMP4(argc, argv);
    return buildMP4(argc, argv);
}
