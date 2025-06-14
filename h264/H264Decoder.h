#ifndef __H264_DECODER_H__
#define __H264_DECODER_H__
#include "NAL.h"
#include "common.h"
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>
#include <memory>

struct SPS {
    uint8_t profileIdc;
    uint8_t levelIdc;
    uint8_t spsId;
    uint8_t log2MaxFrameNum;
    uint8_t picOrderCountType;
    uint8_t log2MaxPicOrderCntLsb;
    uint8_t maxNumRefFrames;
    uint8_t gapsInFrameNumValueAllowedFlag;
    uint8_t picWidthInMbs;
    uint8_t picHeightInMapUnits;
    uint8_t frameMbsOnlyFlag;
    uint8_t direct8x8InferenceFlag;
    uint8_t frameCroppingFlag;
    uint8_t vuiParametersPresentFlag;
};

struct PPS {
    uint8_t ppsId;
    uint8_t spsId;
    uint8_t entropyCodingModeFlag;
    uint8_t bottomFildPidOrderInFramePresentFlag;
    uint8_t numSliceGroups;
    uint8_t numRefIdxL0DefaultActive;
    uint8_t numRefIdxL1DefaultActive;
    uint8_t weightedPredFlag;
    uint8_t weightedBiPredIdc;
    uint8_t picInitQp;
    uint8_t picInitQs;
    int8_t chromaQpIndexOffset;
    uint8_t deblockingFilterControlPresentFlag;
    uint8_t constrainedIntraPredFlag;
    uint8_t redundantPicCntPresentFlag;
};

struct SliceHeader {
    uint8_t firstMbInSlice;
    uint8_t sliceType;
    uint8_t picParameterSetId;
    uint8_t frameNum;
    uint8_t idrPicId;
    uint8_t picOrderCntLsb;
    uint8_t numRefIdxActiveOverrideFlag;
    uint8_t numRedIdxL0Active;
    int8_t sliceQpDelta;
    uint8_t disableDeblockingFilterIdc;
    int8_t sliceAlphaC0Offset;
    int8_t sliceBetaOffset;
};

struct MacroBlock {
    uint8_t type;
    uint8_t partition;
    uint8_t cbpLuma;
    uint8_t cbpChroma;
    uint8_t intra16x16PredMode;
    int8_t intra4x4PredMode[X264_SCAN8_SIZE];
    int nonZeroCount[X264_SCAN8_SIZE];
    uint8_t chromaPredMode;
    uint8_t luma[16][16];
    uint8_t cb[8][8];
    uint8_t cr[8][8];
    int8_t qp;
    union {
        int residualAC[15];
        int luma4x4[16];
    }block[16+8];
    int chromaDc[2][4];
    int luma16x16Dc[16];
    int8_t ref[2][48];
    int16_t mv[2][48][2];
    int16_t mv_min[2];
    int16_t mv_max[2];
    int subPartition[4];
};
typedef std::shared_ptr<MacroBlock> MacroBlockPtr;

struct Frame {
    SliceHeader header;
    std::vector<MacroBlockPtr> mbList;
    int stride[3];
    int lines[3];
    uint8_t* buffer[3];
    uint8_t* plane[3];
};
typedef std::shared_ptr<Frame> FramePtr;

class H264Decoder {
    public:
        explicit H264Decoder(std::string& fileName);
        virtual ~H264Decoder();
        void decode();

    private:
        std::fstream inFile;
        SPS sps;
        PPS pps;
        std::vector<FramePtr> refFrame;
        int8_t mbLastQp;
        int16_t mv_min[2];
        int16_t mv_max[2];
        void decodeNAL(NAL* n);
        void decodeSPS(NAL* n);
        void decodePPS(NAL* n);
        void decodeIFrame(NAL* n);
        void decodePFrame(NAL* n);
        void decodeSliceHeader(NAL* n, Frame& frame);
        void decodeSliceData(NAL* n, Frame& frame);
        void decodeMb(NAL* n, Frame& frame);
        void decodeMbSkip(NAL* n, Frame& frame);
        void decodeMbIPCM(NAL* n, Frame& frame, MacroBlockPtr mb);
        void decodeMbI4x4(NAL* n, Frame& frame, MacroBlockPtr mb);
        void decodeMbI16x16(NAL* n, Frame& frame, MacroBlockPtr mb);
        void decodeMbPL0(NAL* n, Frame& frame, MacroBlockPtr mb);
        void decodeMbP8x8(NAL* n, Frame& frame, MacroBlockPtr mb, bool subRef0);
        void decodeMbP8x16(NAL* n, Frame& frame, MacroBlockPtr mb);
        void decodeMbP16x8(NAL* n, Frame& frame, MacroBlockPtr mb);
        int8_t predictIntra4x4Mode(MacroBlockPtr mb, int idx);
        void blockResidualReadCavlc(NAL* n, MacroBlockPtr mb, int idx, int* buf, int count);
        void initMb(Frame& frame, MacroBlockPtr mb);
        void fixMb(Frame& frame, MacroBlockPtr mb);
        void showFrame(FramePtr pf);
        void subMbMvReadCavlc(MacroBlockPtr mb, NAL* n, int i_list );
        void decodeI4x4(MacroBlockPtr mb, FramePtr f, int x, int y);
        void decodeI16x16(MacroBlockPtr mb, FramePtr f, int x, int y);
};

#endif // !__H264_DECODER_H__
