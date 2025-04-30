#include "MP4Parser.h"
#include <cassert>
#include <cstring>
#include <netinet/in.h>
#include <stdio.h>
#include <iostream>
#include <arpa/inet.h>

using namespace mp4parser;

struct BoxAttr {
    std::string type;
    bool leaf;
    int dataLen;
};


static BoxAttr gBoxAttr[] = {
    {"ftyp", true, -1},
    {"free", true, -1},
    {"skip", true, -1},
    {"mdat", true, -1},
    {"moov", false, 0},
    {"mvhd", true, 100}, // length vary according to version
    {"trak", false, 0},
    {"tkhd", true, 84}, // length vary according to version
    {"edts", false, 0},
    {"elst", true, 20}, //length vary according to version and entry_count
    {"mdia", false, 0},
    {"mdhd", true, 24}, // length vary according to version
    {"hdlr", true, -1},
    {"minf", false, 0},
    {"vmhd", true, 12}, // version
    {"dinf", false, 0},
    {"dref", false, 8}, //version
    {"url ", true, -1},
    {"stbl", false, 0},
    {"stsd", false, 8},
    {"stts", true, -1},
    {"stss", true, -1},
    {"stsc", true, -1},
    {"stsz", true, -1},
    {"stco", true, -1},
    {"smhd", true, -1}

};

Box::Box(char* type, uint8_t* data, int len)
{
    type_ = type;
    if (len > 0) {
        data_.assign(data, data + len);
    }
}

Box::~Box()
{

}

void Box::addChild(Box* c)
{
    child_.push_back(c);
}

void Box::removeChild(std::string type)
{
    for (auto iter = child_.begin(); iter != child_.end(); iter++) {
        if ((*iter)->type_ == type) {
            child_.erase(iter);
            break;
        }
    }
}

std::vector<Box*> Box::findChild(std::string type)
{
    std::vector<Box*> ret;
    for(auto c: child_) {
        if (type == c->type_) {
            ret.push_back(c);
            continue;
        }
        std::vector<Box*> ch = c->findChild(type);
        if (ch.size() > 0) {
            ret.insert(ret.end(), ch.begin(), ch.end());
            continue;
        }
    }

    return ret;
}

int Box::size()
{
    int ret = 8;
    ret += data_.size();
    for(auto b: child_) {
        ret += b->size();
    }

    return ret;
}

MP4Parser::MP4Parser(): root_("isof", nullptr, 0)
{

}

MP4Parser::~MP4Parser()
{

}

//return total bytes parsed
int MP4Parser::parseBoxRecursive(FILE* fp, Box* parent, long index, long size)
{
    if (index + 8 >= size) {
        std::cout << "reach file end, index: " << index << ", totalSize: " << size << std::endl;
        return 0;
    }

    std::cout << "parse offset " << index << std::endl;
    int boxSize = 0;
    char type[5] = {0};
    int ret = fread(&boxSize, 4, 1, fp);
    index += 4;
    assert(ret == 1);
    boxSize = ntohl(boxSize);
    ret = fread(type, 4, 1, fp);
    index += 4;
    assert(ret == 1);
    std::cout << "box size: " << boxSize << ", type: " << type << std::endl;
    
    BoxAttr* pAttr = nullptr;
    for(int i = 0; i < sizeof(gBoxAttr)/sizeof(gBoxAttr[0]); i++) {
        if (strcmp(type, gBoxAttr[i].type.c_str()) == 0) {
            pAttr = &gBoxAttr[i];
            break;
        }
    }

    int dataSize = boxSize - 8;
    if (pAttr == nullptr || pAttr->leaf) {
        if (dataSize == 0) {
            Box* b = new Box(type, nullptr, 0);
            parent->addChild(b);
        } else {
            std::vector<uint8_t> data(dataSize, 0); //TODO
            int ret = fread(data.data(), dataSize, 1, fp);
            index += dataSize;
            assert(ret == 1);
            Box* b = new Box(type, data.data(), dataSize);
            parent->addChild(b);
        }
        return boxSize;
    }

    //non-leaf, parse child box
    long leftSize = dataSize;
    Box* b = nullptr;
    if (pAttr->dataLen == 0) {
        b = new Box(type, nullptr, 0);
        parent->addChild(b);
    } else {
        std::vector<uint8_t> data(pAttr->dataLen, 0); //TODO
        int ret = fread(data.data(), pAttr->dataLen, 1, fp);
        index += pAttr->dataLen;
        assert(ret == 1);
        b = new Box(type, data.data(), pAttr->dataLen);
        parent->addChild(b);
    }
    leftSize -= pAttr->dataLen;
    while (leftSize > 0) {
        int childSize = parseBoxRecursive(fp, b, index, size);
        leftSize -= childSize;
    }

    return boxSize;
}

void MP4Parser::printBox(Box* b, int depth)
{
    for (int i = 0; i < depth; i++) {
        std::cout << "   |";
    }
    std::cout << b->type_  << ": " << b->data_.size() << std::endl;

    for(auto ch: b->child_) {
        printBox(ch, depth+1);
    }
}

struct STXX {
    struct ENT {
        int data[3];
    };
    std::vector<ENT> entries;
    void addEntry(int d1, int d2, int d3, bool bigEndian) {
        ENT e;
        if (bigEndian) {
            e.data[0] = ntohl(d1);
            e.data[1] = ntohl(d2);
            e.data[2] = ntohl(d3);
        } else {
            e.data[0] = d1;
            e.data[1] = d2;
            e.data[2] = d3;
        }
        entries.push_back(e);
    }
};

int MP4Parser::parseData(FILE* fp)
{
    std::vector<Box*> traks = root_.findChild("trak");
    for(auto trakBox: traks) {
        std::cout << "------------- trak --------------" << std::endl;
        tracks_.push_back(Track());
        auto& t = tracks_.back();
        if (trakBox->findChild("vmhd").size() > 0) {
            t.type = VIDEO;
        } else if (trakBox->findChild("smhd").size() > 0){
            t.type = AUDIO;
        } else {
            perror("no video or audio track");
        }

        auto sttsBox = trakBox->findChild("stts");
        auto stcoBox = trakBox->findChild("stco");
        auto stszBox = trakBox->findChild("stsz");
        auto stscBox = trakBox->findChild("stsc");
        auto stssBox = trakBox->findChild("stss");
        assert(sttsBox.size() == 1);
        assert(stcoBox.size() == 1);
        assert(stscBox.size() == 1);
        assert(stszBox.size() == 1);
        //parse stts
        STXX stts;
        int* data = (int*)sttsBox[0]->data_.data();
        data += 1; //ignore version, flag
        int entry_count = ntohl(*data);
        std::cout << "stts size: " << sttsBox[0]->data_.size() << ", entry: " << entry_count << std::endl;
        data++;
        for(int i = 0; i < entry_count; i++) {
            stts.addEntry(*data, *(data+1), 0, true);
            data += 2;
        }
        //parse stco
        STXX stco;
        data = (int*)stcoBox[0]->data_.data();
        data += 1; //ignore version, flag
        entry_count = ntohl(*data);
        std::cout << "stco entry: " << entry_count << std::endl;
        data++;
        for(int i = 0; i < entry_count; i++) {
            stco.addEntry(*data, 0, 0, true);
            data++;
        }
        //parse stsz
        STXX stsz;
        data = (int*)stszBox[0]->data_.data();
        data += 1; //ignore version, flag
        data += 1; //ignore sample size
        entry_count = ntohl(*data);
        std::cout << "stsz entry: " << entry_count << std::endl;
        data++;
        for(int i = 0; i < entry_count; i++) {
            stsz.addEntry(*data, 0, 0, true);
            data++;
        }
        //parse stsc
        STXX stsc;
        data = (int*)stscBox[0]->data_.data();
        data += 1; //ignore version, flag
        entry_count = ntohl(*data);
        std::cout << "stsc entry: " << entry_count << std::endl;
        data++;
        for(int i = 0; i < entry_count; i++) {
            stsc.addEntry(*data, *(data+1), *(data+2), true);
            data+=3;
        }

        std::cout << "stsz.entries: " << stsz.entries.size() << std::endl;
        for(auto& it: stsz.entries) {
            int size = it.data[0];
            t.samples.push_back({std::vector<uint8_t>(size, 0), 0, false});
        }

        int timestamp = 0;
        int k = 0;
        for(auto& it: stts.entries) {
            int count = it.data[0];
            int delta = it.data[1];
            for(int i = 0; i < count; i++) {
                t.samples[k+i].timestamp = timestamp;
                timestamp += delta;
            }
            k += count;
        }
        std::cout << "k: " << k << ", samples: " << t.samples.size() << std::endl;
        assert(k == t.samples.size());

        int chunk_idx = 0;
        int sample_idx = 0;
        int spc = 0;
        for(auto& it: stsc.entries) {
            int idx = it.data[0] - 1;
            int new_spc = it.data[1];
            for(; chunk_idx < idx; chunk_idx++) {
                int offset = stco.entries[chunk_idx].data[0];
                for(int s = 0; s < spc; s++) {
                    auto& smp = t.samples[sample_idx];
                    int size = smp.data.size();
                    // std::cout << "offset: " << offset << ", size: " << size << ", timestamp: " << smp.timestamp << std::endl;
                    fseek(fp, offset, SEEK_SET);
                    assert(1 == fread(smp.data.data(), size, 1, fp));
                    sample_idx++;
                    offset += size;
                }
            }
            spc = new_spc;
        }
        for(; chunk_idx < stco.entries.size(); chunk_idx++) {
            int offset = stco.entries[chunk_idx].data[0];
            for(int s = 0; s < spc; s++) {
                auto& smp = t.samples[sample_idx];
                int size = smp.data.size();
                // std::cout << "offset: " << offset << ", size: " << size << ", timestamp: " << smp.timestamp << std::endl;
                fseek(fp, offset, SEEK_SET);
                assert(1 == fread(smp.data.data(), size, 1, fp));
                sample_idx++;
                offset += size;
            }
        }
    }

    return 0;
}

struct STInfo{
    STXX stsc;
    STXX stco;
    STXX stsz;
    STXX stts;
    STXX stss;
    int chunkIndex = 1;
    int samplePerChunk = 0;
    int chunkSize = 0;
};

static std::vector<uint8_t> convertIntBE(int n[], int N)
{
    std::vector<uint8_t> ret;
    for(int i = 0; i < N; i++) {
        ret.push_back((n[i] >> 24) & 0xff);
        ret.push_back((n[i] >> 16) & 0xff);
        ret.push_back((n[i] >> 8) & 0xff);
        ret.push_back(n[i] & 0xff);
    }
    return ret;
}

int MP4Parser::storeData()
{
    STInfo vInfo, aInfo;
    Track* videoTrack = nullptr, *audioTrack = nullptr;
    for(auto& track: tracks_) {
        switch (track.type) {
            case VIDEO:
                videoTrack = &track;
                break;
            case AUDIO:
                audioTrack = &track;
                break;
        }
    }
    assert(videoTrack != nullptr || audioTrack != nullptr);
    std::vector<Sample> tmp;
    auto videoIter = videoTrack == nullptr ? tmp.begin() : videoTrack->samples.begin();
    auto audioIter = audioTrack == nullptr ? tmp.begin() : audioTrack->samples.begin();
    int offset = 0;
    for(auto box: root_.child_) {
        if (box->type_ != "mdat") {
            offset += 8 + box->data_.size();
        } else {
            offset += 8;
            break;
        }
    }

    auto dataBox = root_.findChild("mdat").front();
    dataBox->data_.clear();
    while (true) {
        std::vector<Sample>::iterator smp;
        bool isVideo = true;
        if (videoIter == tmp.begin() || videoIter == videoTrack->samples.end()) {
            //audio only
            if (audioIter == tmp.begin() || audioIter == audioTrack->samples.end()) {
                break;
            }
            smp = audioIter;
            isVideo = false;
        } else if (audioIter == tmp.begin() || audioIter == audioTrack->samples.end()) {
            //video only
            if (videoIter == tmp.begin() || videoIter == videoTrack->samples.end()) {
                break;
            }
            smp = videoIter;
            isVideo = true;
        } else {
            if (videoIter->timestamp < audioIter->timestamp) {
                smp = videoIter;
                isVideo = true;
            } else {
                smp = audioIter;
                isVideo = false;
            }
        }
        std::vector<Sample>::iterator endIter;
        STInfo* pInfo;
        if (isVideo) {
            videoIter++;
            endIter = videoTrack->samples.end();
            pInfo = &vInfo;
            if (aInfo.chunkSize > 0) {
                aInfo.stco.addEntry(offset, 0, 0, false);
                aInfo.stsc.addEntry(aInfo.chunkIndex, aInfo.samplePerChunk, 1, false);
                offset += aInfo.chunkSize;
                aInfo.chunkIndex++;
                aInfo.samplePerChunk = 0;
                aInfo.chunkSize = 0;
            }
        } else {
            audioIter++;
            endIter = audioTrack->samples.end();
            pInfo = &aInfo;
            if (vInfo.chunkSize > 0) {
                vInfo.stco.addEntry(offset, 0, 0, false);
                vInfo.stsc.addEntry(vInfo.chunkIndex, vInfo.samplePerChunk, 1, false);
                offset += vInfo.chunkSize;
                vInfo.chunkIndex++;
                vInfo.samplePerChunk = 0;
                vInfo.chunkSize = 0;
            }
        }

        if (smp + 1 != endIter) {
            pInfo->stts.addEntry(1, (smp+1)->timestamp - smp->timestamp, 0, false);
        }
        pInfo->stsz.addEntry(smp->data.size(), 0, 0, false);
        pInfo->chunkSize += smp->data.size();
        pInfo->samplePerChunk++;
        dataBox->data_.insert(dataBox->data_.end(), smp->data.begin(), smp->data.end());
    }
    if (aInfo.chunkSize > 0) {
        aInfo.stco.addEntry(offset, 0, 0, false);
        aInfo.stsc.addEntry(aInfo.chunkIndex, aInfo.samplePerChunk, 1, false);
        offset += aInfo.chunkSize;
        aInfo.chunkIndex++;
        aInfo.samplePerChunk = 0;
        aInfo.chunkSize = 0;
    }
    if (vInfo.chunkSize > 0) {
        vInfo.stco.addEntry(offset, 0, 0, false);
        vInfo.stsc.addEntry(vInfo.chunkIndex, vInfo.samplePerChunk, 1, false);
        offset += vInfo.chunkSize;
        vInfo.chunkIndex++;
        vInfo.samplePerChunk = 0;
        vInfo.chunkSize = 0;
    }

    auto traks = root_.findChild("trak");
    for(auto trak: traks) {
        STInfo* pInfo;
        auto vmhd = trak->findChild("vmhd");
        if (vmhd.size() > 0) {
            //video trak
            pInfo = &vInfo;
        }
        auto smhd = trak->findChild("smhd");
        if (smhd.size() > 0) {
            //audio trak
            pInfo = &aInfo;
        }
        Box* sttsBox = trak->findChild("stts").front();
        sttsBox->data_.clear();
        {
            int flagAndEntries[2] = {0, static_cast<int>(pInfo->stts.entries.size())};
            auto flag = convertIntBE(flagAndEntries, 2);
            sttsBox->data_.insert(sttsBox->data_.end(), flag.begin(), flag.end());
        }
        for(auto e: pInfo->stts.entries) {
            auto ent = convertIntBE(e.data, 2);
            sttsBox->data_.insert(sttsBox->data_.end(), ent.begin(), ent.end());
        }
        Box* stcoBox = trak->findChild("stco").front();
        stcoBox->data_.clear();
        {
            int flagAndEntries[2] = {0, static_cast<int>(pInfo->stco.entries.size())};
            auto flag = convertIntBE(flagAndEntries, 2);
            stcoBox->data_.insert(stcoBox->data_.end(), flag.begin(), flag.end());
        }
        for(auto e: pInfo->stco.entries) {
            auto ent = convertIntBE(e.data, 1);
            stcoBox->data_.insert(stcoBox->data_.end(), ent.begin(), ent.end());
        }
        Box* stszBox = trak->findChild("stsz").front();
        stszBox->data_.clear();
        {
            int flagAndEntries[3] = {0, 0, static_cast<int>(pInfo->stsz.entries.size())};
            auto flag = convertIntBE(flagAndEntries, 3);
            stszBox->data_.insert(stszBox->data_.end(), flag.begin(), flag.end());
        }
        for(auto e: pInfo->stsz.entries) {
            auto ent = convertIntBE(e.data, 1);
            stszBox->data_.insert(stszBox->data_.end(), ent.begin(), ent.end());
        }
        Box* stscBox = trak->findChild("stsc").front();
        stscBox->data_.clear();
        {
            int flagAndEntries[2] = {0, static_cast<int>(pInfo->stsc.entries.size())};
            auto flag = convertIntBE(flagAndEntries, 2);
            stscBox->data_.insert(stscBox->data_.end(), flag.begin(), flag.end());
        }
        for(auto e: pInfo->stsc.entries) {
            auto ent = convertIntBE(e.data, 3);
            stscBox->data_.insert(stscBox->data_.end(), ent.begin(), ent.end());
        }
    }
    return 0;
}

void MP4Parser::dumpAV()
{

}

int MP4Parser::loadFile(std::string path)
{
    FILE* fp = fopen(path.c_str(), "r");
    assert(fp != NULL);
    fseek(fp, 0, SEEK_END);
    long fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    std::cout << "fileSize: " << fileSize <<std::endl;
    long offset = 0;
    while(offset < fileSize) {
        long bytes = parseBoxRecursive(fp, &root_, offset, fileSize);
        offset += bytes;
    }
    printBox(&root_, 0);
    parseData(fp);
    fclose(fp);

    return 0;
}

int MP4Parser::saveFile(std::string path)
{
    FILE* fp = fopen(path.c_str(), "w");
    assert(fp != NULL);
    storeData();
    std::cout << "--------------output mp4-------------" << std::endl;
    printBox(&root_, 0);
    for(auto b: root_.child_) {
        writeBoxRecursive(fp, b);
    }
    fclose(fp);
    return 0;
}

int MP4Parser::writeBoxRecursive(FILE* fp, Box* b)
{
    int size = htonl(b->size());
    fwrite(&size, 4, 1, fp);
    fwrite(b->type_.c_str(), 4, 1, fp);
    if (b->data_.size() > 0) {
        fwrite(b->data_.data(), b->data_.size(), 1, fp);
    }
    for(auto c: b->child_) {
        writeBoxRecursive(fp, c);
    }

    return 0;
}

int MP4Parser::replaceAudio(std::vector<uint8_t>& data, int sampleRate, int format)
{
    for(auto& trak: tracks_) {
        if (trak.type == AUDIO) {
            int pos = 0, end = data.size();
            for(auto& smp: trak.samples) {
                int size = smp.data.size();
                smp.data.clear();
                smp.data.insert(smp.data.end(), data.data() + pos, data.data() + pos + size * 2);
                pos += size * 2;
                if (pos + size * 2 >= end) {
                    pos = 0;
                }
            }
            break;
        }
    }

    auto box = root_.findChild("ulaw").front();
    box->type_ = "sowt";
    return 0;
}

int MP4Parser::replaceVideo(std::vector<std::vector<uint8_t>>& data)
{
    for(auto& trak: tracks_) {
        if (trak.type == VIDEO) {
            int idx = 0;
            for(auto& smp: trak.samples) {
                int size = smp.data.size();
                smp.data.clear();
                smp.data.insert(smp.data.end(), data[idx].begin(), data[idx].end());
                idx++;
            }
            break;
        }
    }

    auto box = root_.findChild("hvc1").front();
    box->type_ = "jpeg";
    box->child_.clear();
    return 0;
}
