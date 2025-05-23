#include "H264Decoder.h"
#include "NAL.h"
#include "common.h"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>
#include "macroblock.h"
#include "vlc.h"
#include <cstring>
#include "opencv2/core.hpp"
#include "opencv2/core/matx.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgcodecs.hpp"

H264Decoder::H264Decoder(std::string& fileName)
{
    macroblock_init();
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
    } else if (n->unitType == 8) {
        decodePPS(n);
    } else if (n->unitType == 5) {
        decodeIFrame(n);
    } else if (n->unitType == 1) {
        decodePFrame(n);
    } else {
        std::cout << "unitType not support: " << n->unitType << std::endl;
    }
}

void H264Decoder::decodeSPS(NAL* n)
{
    sps.profileIdc = n->read_bits(8);
    n->read_bits(8); //constrained_flag
    sps.levelIdc = n->read_bits(8);
    sps.spsId = n->read_ue();
    switch(sps.profileIdc) {
        case 100:
        case 110:
        case 122:
        case 244:
        case 44:
        case 83:
        case 86:
        case 118:
        case 128:
            std::cout << "profile idc not support" << std::endl;
            assert(0);
            break;
    }
    sps.log2MaxFrameNum = n->read_ue() + 4;
    sps.picOrderCountType = n->read_ue();
    if (sps.picOrderCountType == 0) {
        sps.log2MaxPicOrderCntLsb = n->read_ue() + 4;
    } else {
        std::cout << "pic order count type not support: " << sps.picOrderCountType << std::endl;
        assert(0);
    }

    sps.maxNumRefFrames = n->read_ue();
    assert(sps.maxNumRefFrames == 1);
    sps.gapsInFrameNumValueAllowedFlag = n->read_bits(1);
    sps.picWidthInMbs = n->read_ue() + 1;
    sps.picHeightInMapUnits = n->read_ue() + 1;
    sps.frameMbsOnlyFlag = n->read_bits(1);
    assert(sps.frameMbsOnlyFlag == 1);
    sps.direct8x8InferenceFlag = n->read_bits(1);
    sps.frameCroppingFlag = n->read_bits(1);
    assert(sps.frameCroppingFlag == 0);
    sps.vuiParametersPresentFlag = n->read_bits(1);
    // assert(sps.vuiParametersPresentFlag == 0);
    return;
}

void H264Decoder::decodePPS(NAL* n)
{
    pps.ppsId = n->read_ue();
    pps.spsId = n->read_ue();
    pps.entropyCodingModeFlag = n->read_bits(1);
    pps.bottomFildPidOrderInFramePresentFlag = n->read_bits(1);
    pps.numSliceGroups = n->read_ue() + 1;
    assert(pps.numSliceGroups == 1);
    pps.numRefIdxL0DefaultActive = n->read_ue() + 1;
    pps.numRefIdxL1DefaultActive = n->read_ue() + 1;
    pps.weightedPredFlag = n->read_bits(1);
    pps.weightedBiPredIdc = n->read_bits(2);
    pps.picInitQp = n->read_se() + 26;
    pps.picInitQs = n->read_se() + 26;
    pps.chromaQpIndexOffset = n->read_se();
    pps.deblockingFilterControlPresentFlag = n->read_bits(1);
    pps.constrainedIntraPredFlag = n->read_bits(1);
    pps.redundantPicCntPresentFlag = n->read_bits(1);
    return;
}

void H264Decoder::decodeSliceHeader(NAL* n, Frame& f)
{
    f.header.firstMbInSlice = n->read_ue();
    f.header.sliceType = n->read_ue();
    f.header.picParameterSetId = n->read_ue();
    f.header.frameNum = n->read_bits(sps.log2MaxFrameNum);
    if (n->unitType == 5) {
        f.header.idrPicId = n->read_ue();
    }
    if (sps.picOrderCountType == 0) {
        f.header.picOrderCntLsb = n->read_bits(sps.log2MaxPicOrderCntLsb);
    }
    if (f.header.sliceType % 5 != SliceTypeI && f.header.sliceType % 5 != SliceTypeP) {
        std::cout << "sliceType not support: " << f.header.sliceType << std::endl;
        assert(0);
    }
    if (f.header.sliceType % 5 == 0) { // p slice
        f.header.numRefIdxActiveOverrideFlag = n->read_bits(1);
        if (f.header.numRefIdxActiveOverrideFlag) {
            f.header.numRedIdxL0Active = n->read_ue() + 1;
        }
    }
    if (f.header.sliceType % 5 != SliceTypeI && f.header.sliceType % 5 != SliceTypeSI) {
        uint8_t refPicListModificationFlagL0 = n->read_bits(1);
        assert(refPicListModificationFlagL0 == 0);
    }
    if (n->refIdc != 0) {
        if (n->unitType == 5) {
            uint8_t noOutputOfPriorPicsFlag = n->read_bits(1);
            uint8_t longTermReferenceFlag = n->read_bits(1);
        } else {
            uint8_t adaptiveRefPicMarkingModeFlag = n->read_bits(1);
            assert(adaptiveRefPicMarkingModeFlag == 0);
        }
    }
    f.header.sliceQpDelta = n->read_se();
    mbLastQp = pps.picInitQp + f.header.sliceQpDelta;
    if (pps.deblockingFilterControlPresentFlag) {
        f.header.disableDeblockingFilterIdc = n->read_ue();
        if (f.header.disableDeblockingFilterIdc != 1) {
            f.header.sliceAlphaC0Offset = n->read_se() << 1;
            f.header.sliceBetaOffset = n->read_se() << 1;
        }
    }
}

void H264Decoder::decodeIFrame(NAL* n)
{
    FramePtr pf = std::make_shared<Frame>();
    std::memset(pf.get(), 0, sizeof(Frame));
    Frame& f = *pf;
    f.stride[0] = sps.picWidthInMbs * 16 + 64;
    f.lines[0] = sps.picHeightInMapUnits * 16;

    for( int i = 0; i < 3; i++ )
    {
        int i_divh = 1;
        int i_divw = 1;
        if( i > 0 )
        {
            i_divh = i_divw = 2;
        }
        f.stride[i] = f.stride[0] / i_divw;
        f.lines[i] = f.lines[0] / i_divh;
        f.buffer[i] = (uint8_t*)malloc(f.stride[i]*(f.lines[i] + 64/i_divh));
        memset(f.buffer[i], 128, f.stride[i]*(f.lines[i]+64/i_divh));

        f.plane[i] = ((uint8_t*)f.buffer[i]) +
                          f.stride[i] * 32 / i_divh + 32 / i_divw;
    }
    decodeSliceHeader(n, f);
    decodeSliceData(n, f);
    for(int i = 0; i < f.mbList.size(); i++) {
        printf("mb: %d\n", i);
        MacroBlockPtr mb = f.mbList[i];
        int x = (i % sps.picWidthInMbs) * 16;
        int y = (i / sps.picWidthInMbs) * 16;
        if (mb->type == I_4x4) {
            for(int j = 0; j < 16; j++) {
                int x1 = x264_scan8_x[j];
                int y1 = x264_scan8_y[j];
                printf("idx: %d\n", j);
                int pred_mode = mb->intra4x4PredMode[x264_scan8[j]];
                if (pred_mode == I_PRED_4x4_DC) {
                    if (y+4*y1 > 0 && x+4*x1 > 0) {

                    } else if (y+4*y1 > 0) {
                        pred_mode = I_PRED_4x4_DC_TOP;
                    } else if (x+4*x1 > 0) {
                        pred_mode = I_PRED_4x4_DC_LEFT;
                    } else {
                        pred_mode = I_PRED_4x4_DC_128;
                    }
                }
                x264_mb_decode_i4x4(mb->qp, f.plane[0]+(y+4*y1)*f.stride[0] + x + 4*x1, f.stride[0], mb->block[j].luma4x4, pred_mode);
            }
            // assert(mb->cbpChroma != 0);
            uint8_t* chroma_dst[2] = {
                f.plane[1]+(y/2)*f.stride[1]+x/2,
                f.plane[2]+(y/2)*f.stride[2]+x/2,
            };
            int* residualAC[8] = {
                mb->block[16+0].residualAC,
                mb->block[16+1].residualAC,
                mb->block[16+2].residualAC,
                mb->block[16+3].residualAC,
                mb->block[16+4].residualAC,
                mb->block[16+5].residualAC,
                mb->block[16+6].residualAC,
                mb->block[16+7].residualAC,
            };
            int pred_mode = mb->chromaPredMode;
            if (pred_mode == I_PRED_CHROMA_DC) {
                if (x > 0 && y > 0) {

                } else if (x> 0) {
                    pred_mode = I_PRED_CHROMA_DC_LEFT;
                } else if (y>0) {
                    pred_mode = I_PRED_CHROMA_DC_TOP;
                } else {
                    pred_mode = I_PRED_CHROMA_DC_128;
                }
            }
            x264_mb_decode_8x8(0, (int)mb->qp, f.stride, chroma_dst, mb->chromaDc, residualAC, pred_mode);
        } else if (mb->type == I_16x16) {
            int* lumaAC[16] = {
                mb->block[0].residualAC,
                mb->block[1].residualAC,
                mb->block[2].residualAC,
                mb->block[3].residualAC,
                mb->block[4].residualAC,
                mb->block[5].residualAC,
                mb->block[6].residualAC,
                mb->block[7].residualAC,
                mb->block[8].residualAC,
                mb->block[9].residualAC,
                mb->block[10].residualAC,
                mb->block[11].residualAC,
                mb->block[12].residualAC,
                mb->block[13].residualAC,
                mb->block[14].residualAC,
                mb->block[15].residualAC,
            };
            int pred_mode = mb->intra16x16PredMode;
            if (pred_mode == I_PRED_16x16_DC) {
                if (x > 0 && y > 0) {

                } else if (x> 0) {
                    pred_mode = I_PRED_16x16_DC_LEFT;
                } else if (y>0) {
                    pred_mode = I_PRED_16x16_DC_TOP;
                } else {
                    pred_mode = I_PRED_16x16_DC_128;
                }
            }
            x264_mb_decode_i16x16(mb->qp, f.stride[0], f.plane[0]+y*f.stride[0]+x, pred_mode, mb->luma16x16Dc, lumaAC);
            uint8_t* chroma_dst[2] = {
                f.plane[1]+(y/2)*f.stride[1]+x/2,
                f.plane[2]+(y/2)*f.stride[2]+x/2,
            };
            int* residualAC[8] = {
                mb->block[16+0].residualAC,
                mb->block[16+1].residualAC,
                mb->block[16+2].residualAC,
                mb->block[16+3].residualAC,
                mb->block[16+4].residualAC,
                mb->block[16+5].residualAC,
                mb->block[16+6].residualAC,
                mb->block[16+7].residualAC,
            };
            pred_mode = mb->chromaPredMode;
            if (pred_mode == I_PRED_CHROMA_DC) {
                if (x > 0 && y > 0) {

                } else if (x> 0) {
                    pred_mode = I_PRED_CHROMA_DC_LEFT;
                } else if (y>0) {
                    pred_mode = I_PRED_CHROMA_DC_TOP;
                } else {
                    pred_mode = I_PRED_CHROMA_DC_128;
                }
            }
            x264_mb_decode_8x8(0, (int)mb->qp, f.stride, chroma_dst, mb->chromaDc, residualAC, pred_mode);
        }
    }
    x264_frame_deblocking_filter(pf, sps.picWidthInMbs, sps.picHeightInMapUnits, pps.chromaQpIndexOffset);
    refFrame.push_back(pf);
    showFrame(pf);
    return;

}

void H264Decoder::decodePFrame(NAL* n)
{
    FramePtr pf = std::make_shared<Frame>();
    std::memset(pf.get(), 0, sizeof(Frame));
    Frame& f = *pf;
    f.stride[0] = sps.picWidthInMbs * 16 + 64;
    f.lines[0] = sps.picHeightInMapUnits * 16;

    for( int i = 0; i < 3; i++ )
    {
        int i_divh = 1;
        int i_divw = 1;
        if( i > 0 )
        {
            i_divh = i_divw = 2;
        }
        f.stride[i] = f.stride[0] / i_divw;
        f.lines[i] = f.lines[0] / i_divh;
        f.buffer[i] = (uint8_t*)malloc(f.stride[i]*f.lines[i] + 64/i_divh);
        memset(f.buffer[i], 128, f.stride[i]*f.lines[i]+64/i_divh);

        f.plane[i] = ((uint8_t*)f.buffer[i]) +
                          f.stride[i] * 32 / i_divh + 32 / i_divw;
    }
    decodeSliceHeader(n, f);
    decodeSliceData(n, f);
    
    refFrame.push_back(pf);
    return;

}

void H264Decoder::decodeSliceData(NAL* n, Frame& f)
{
    int mbIdx = 0;
    int mbCount = sps.picWidthInMbs * sps.picHeightInMapUnits;
    while(!n->eof()) {
        if (f.header.sliceType % 5 != 2) {
            uint8_t mbSkipRun = n->read_ue();
            mbIdx += mbSkipRun;
        }
        if (mbIdx >= mbCount) {
            break;
        }
        std::cout << "decode mb idx " << mbIdx << std::endl;
        decodeMb(n, f);
        mbIdx++;
    }

}

void H264Decoder::initMb(Frame& frame, MacroBlockPtr mb)
{
    memset(mb.get(), 0, sizeof(MacroBlock));
    int x = frame.mbList.size() % sps.picWidthInMbs;
    int y = frame.mbList.size() / sps.picWidthInMbs;
    for(int i = 0; i < X264_SCAN8_SIZE; i++) {
        mb->intra4x4PredMode[i] = -1;
        mb->nonZeroCount[i] = 0x80;
    }
    for(int i = 0; i < 16 + 8; i++) {
        mb->nonZeroCount[x264_scan8[i]] = 0;
    }
    if (x > 0) {
        int left = frame.mbList.size() - 1;
        if (frame.mbList[left]->type == I_4x4){
            mb->intra4x4PredMode[x264_scan8[0] - 1] = frame.mbList[left]->intra4x4PredMode[x264_scan8[5]];
            mb->intra4x4PredMode[x264_scan8[2] - 1] = frame.mbList[left]->intra4x4PredMode[x264_scan8[7]];
            mb->intra4x4PredMode[x264_scan8[8] - 1] = frame.mbList[left]->intra4x4PredMode[x264_scan8[13]];
            mb->intra4x4PredMode[x264_scan8[10] - 1] = frame.mbList[left]->intra4x4PredMode[x264_scan8[15]];
        } else {
            mb->intra4x4PredMode[x264_scan8[0] - 1] = I_PRED_4x4_DC;
            mb->intra4x4PredMode[x264_scan8[2] - 1] = I_PRED_4x4_DC;
            mb->intra4x4PredMode[x264_scan8[8] - 1] = I_PRED_4x4_DC;
            mb->intra4x4PredMode[x264_scan8[10] - 1] = I_PRED_4x4_DC;
        }
        mb->nonZeroCount[x264_scan8[0] - 1] = frame.mbList[left]->nonZeroCount[x264_scan8[5]];
        mb->nonZeroCount[x264_scan8[2] - 1] = frame.mbList[left]->nonZeroCount[x264_scan8[7]];
        mb->nonZeroCount[x264_scan8[8] - 1] = frame.mbList[left]->nonZeroCount[x264_scan8[13]];
        mb->nonZeroCount[x264_scan8[10] - 1] = frame.mbList[left]->nonZeroCount[x264_scan8[15]];

        mb->nonZeroCount[x264_scan8[16 + 0] - 1] = frame.mbList[left]->nonZeroCount[x264_scan8[16 + 1]];
        mb->nonZeroCount[x264_scan8[16 + 2] - 1] = frame.mbList[left]->nonZeroCount[x264_scan8[16 + 3]];

        mb->nonZeroCount[x264_scan8[16 + 4 + 0] - 1] = frame.mbList[left]->nonZeroCount[x264_scan8[16 + 4 + 1]];
        mb->nonZeroCount[x264_scan8[16 + 4 + 2] - 1] = frame.mbList[left]->nonZeroCount[x264_scan8[16 + 4 + 3]];
    }
    if (y > 0) {
        int top = (y - 1) * sps.picWidthInMbs + x;
        if (frame.mbList[top]->type == I_4x4) {
            mb->intra4x4PredMode[x264_scan8[0] - 8] = frame.mbList[top]->intra4x4PredMode[x264_scan8[10]];
            mb->intra4x4PredMode[x264_scan8[1] - 8] = frame.mbList[top]->intra4x4PredMode[x264_scan8[11]];
            mb->intra4x4PredMode[x264_scan8[4] - 8] = frame.mbList[top]->intra4x4PredMode[x264_scan8[14]];
            mb->intra4x4PredMode[x264_scan8[5] - 8] = frame.mbList[top]->intra4x4PredMode[x264_scan8[15]];
        } else {
            mb->intra4x4PredMode[x264_scan8[0] - 8] = I_PRED_4x4_DC;
            mb->intra4x4PredMode[x264_scan8[1] - 8] = I_PRED_4x4_DC;
            mb->intra4x4PredMode[x264_scan8[4] - 8] = I_PRED_4x4_DC;
            mb->intra4x4PredMode[x264_scan8[5] - 8] = I_PRED_4x4_DC;
        }
        mb->nonZeroCount[x264_scan8[0] - 8] = frame.mbList[top]->nonZeroCount[x264_scan8[10]];
        mb->nonZeroCount[x264_scan8[1] - 8] = frame.mbList[top]->nonZeroCount[x264_scan8[11]];
        mb->nonZeroCount[x264_scan8[4] - 8] = frame.mbList[top]->nonZeroCount[x264_scan8[14]];
        mb->nonZeroCount[x264_scan8[5] - 8] = frame.mbList[top]->nonZeroCount[x264_scan8[15]];

        mb->nonZeroCount[x264_scan8[16 + 0] - 8] = frame.mbList[top]->nonZeroCount[x264_scan8[16 + 2]];
        mb->nonZeroCount[x264_scan8[16 + 1] - 8] = frame.mbList[top]->nonZeroCount[x264_scan8[16 + 3]];

        mb->nonZeroCount[x264_scan8[16 + 4 + 0] - 8] = frame.mbList[top]->nonZeroCount[x264_scan8[16 + 4 + 2]];
        mb->nonZeroCount[x264_scan8[16 + 4 + 1] - 8] = frame.mbList[top]->nonZeroCount[x264_scan8[16 + 4 + 3]];
    }
}

void H264Decoder::decodeMb(NAL* n, Frame& frame)
{
    uint8_t mbType = n->read_ue();
    std::cout << "mbType: " << static_cast<int>(mbType) << std::endl;
    MacroBlockPtr mb = std::make_shared<MacroBlock>();
    initMb(frame, mb);
    switch(frame.header.sliceType % 5) {
        case SliceTypeI:
            assert(mbType < 26);
            mb->type = ISliceMbTable[mbType][0];
            mb->intra16x16PredMode = ISliceMbTable[mbType][1];
            mb->cbpLuma = ISliceMbTable[mbType][4];
            mb->cbpChroma = ISliceMbTable[mbType][3];
            if (mb->type == I_PCM) {
                decodeMbIPCM(n, frame, mb);
            } else if (mb->type == I_4x4) {
                decodeMbI4x4(n, frame, mb);
            } else if (mb->type == I_16x16) {
                decodeMbI16x16(n, frame, mb);
            }
            mbLastQp = mb->qp;
            break;
        case SliceTypeP:
            assert(mbType < 6);
            mb->type = PSliceMbTable[mbType];
            break;
        default:
            std::cout << "unknown slicetype " << (int)frame.header.sliceType << std::endl;
            assert(0);
    }
    fixMb(frame, mb);
    frame.mbList.push_back(mb);
    std::cout << "mb count " << frame.mbList.size() << std::endl;

}

void H264Decoder::fixMb(Frame& frame, MacroBlockPtr mb)
{
    if (mb->type != I_4x4) {
        mb->intra4x4PredMode[x264_scan8[5]] = I_PRED_4x4_DC;
        mb->intra4x4PredMode[x264_scan8[7]] = I_PRED_4x4_DC;
        mb->intra4x4PredMode[x264_scan8[13]] = I_PRED_4x4_DC;
        mb->intra4x4PredMode[x264_scan8[15]] = I_PRED_4x4_DC;
        mb->intra4x4PredMode[x264_scan8[10]] = I_PRED_4x4_DC;
        mb->intra4x4PredMode[x264_scan8[11]] = I_PRED_4x4_DC;
        mb->intra4x4PredMode[x264_scan8[14]] = I_PRED_4x4_DC;
    }
}

void H264Decoder::decodeMbIPCM(NAL* n, Frame& f, MacroBlockPtr mb)
{
    n->read_bits_align();
    for(int i = 0; i < 16*16; i++) {
        mb->luma[i/16][i%16] = n->read_bits(8);
    }
    for(int i = 0; i < 8*8; i++) {
        mb->cb[i/8][i%8] = n->read_bits(8);
    }
    for(int i = 0; i < 8*8; i++) {
        mb->cr[i/8][i%8] = n->read_bits(8);
    }

}

static const uint8_t intra4x4GolombToCbp[48]={
    47,31,15,0,23,27,29,30,7,11,13,14,39,43,45,46,16,
    3,5,10,12,19,21,26,28,35,37,42,44,1,2,4,8,17,18,
    20,24,6,9,22,25,32,33,34,36,40,38,41
};

void H264Decoder::decodeMbI4x4(NAL* n, Frame& f, MacroBlockPtr mb)
{
    for(int i = 0; i < 16; i++) {
        uint8_t prev_flag = n->read_bits(1);
        std::cout << "prev flag " << (int)prev_flag << std::endl;
        int8_t prev = predictIntra4x4Mode(mb, i);
        if (prev_flag) {
            mb->intra4x4PredMode[x264_scan8[i]] = prev;
        } else {
            int8_t current = n->read_bits(3);
            std::cout << "mode " << (int)current << std::endl;
            if (current < prev) {
                mb->intra4x4PredMode[x264_scan8[i]] = current;
            } else {
                mb->intra4x4PredMode[x264_scan8[i]] = current+1;
            }
        }
        assert(mb->intra4x4PredMode[x264_scan8[i]] >= 0);
    }
    mb->chromaPredMode = n->read_ue();
    std::cout << "chroma pred mode " << (int)mb->chromaPredMode << std::endl;
    //code block pattern
    int cbpUe = n->read_ue();
    std::cout << "cbp " << cbpUe << std::endl;
    uint8_t cbp = intra4x4GolombToCbp[cbpUe];
    mb->cbpLuma = cbp & 0xf;
    mb->cbpChroma = cbp >> 4;
    if (mb->cbpChroma != 0 || mb->cbpLuma != 0) {
        mb->qp = n->read_se() + mbLastQp;
        std::cout << "delta qp " << mb->qp - mbLastQp << std::endl;
        for(int i = 0; i < 16; i++) {
            if (mb->cbpLuma & (1 << (i/4))) {
                std::cout << "read cavlc " << i << std::endl;
                blockResidualReadCavlc(n, mb, i, mb->block[i].luma4x4, 16);
            }
        }
    }
    if (mb->cbpChroma != 0) {
        int buf[4];
        blockResidualReadCavlc(n, mb, BLOCK_INDEX_CHROMA_DC, mb->chromaDc[0], 4);
        blockResidualReadCavlc(n, mb, BLOCK_INDEX_CHROMA_DC, mb->chromaDc[1], 4);
        if (mb->cbpChroma & 0x02){
            for(int i = 0; i < 8; i++) {
                blockResidualReadCavlc(n, mb, 16+i, mb->block[16+i].residualAC, 15);
            }
        }
    }
}

void H264Decoder::decodeMbI16x16(NAL* n, Frame& f, MacroBlockPtr mb)
{
    mb->chromaPredMode = n->read_ue();
    mb->qp = n->read_se() + mbLastQp;
    blockResidualReadCavlc(n, mb, BLOCK_INDEX_LUMA_DC, mb->luma16x16Dc, 16);
    if (mb->cbpLuma != 0) {
        for(int i = 0; i < 16; i++) {
            blockResidualReadCavlc(n, mb, i, mb->block[i].residualAC, 15);
        }
    }
    if (mb->cbpChroma != 0) {
        int buf[4];
        blockResidualReadCavlc(n, mb, BLOCK_INDEX_CHROMA_DC, mb->chromaDc[0], 4);
        blockResidualReadCavlc(n, mb, BLOCK_INDEX_CHROMA_DC, mb->chromaDc[1], 4);
        if (mb->cbpChroma & 0x02){
            for(int i = 0; i < 8; i++) {
                blockResidualReadCavlc(n, mb, 16+i, mb->block[16+i].residualAC, 15);
            }
        }
    }
}

int8_t H264Decoder::predictIntra4x4Mode(MacroBlockPtr mb, int idx)
{
    int8_t prev = int(std::min(mb->intra4x4PredMode[x264_scan8[idx]-1], mb->intra4x4PredMode[x264_scan8[idx]-8]));
    if (prev < 0) {
        return I_PRED_4x4_DC;
    }
    return prev;

}

int mbPredictNonZeroCode( MacroBlockPtr mb, int idx )
{
    if (idx < 0) {
        idx = 0;
    }
    const int za = mb->nonZeroCount[x264_scan8[idx] - 1];
    const int zb = mb->nonZeroCount[x264_scan8[idx] - 8];

    int i_ret = za + zb;

    if( i_ret < 0x80 )
    {
        i_ret = ( i_ret + 1 ) >> 1;
    }
    return i_ret & 0x7f;
}

void H264Decoder::blockResidualReadCavlc(NAL* n, MacroBlockPtr mb, int idx, int* buf, int count)
{
    static const int ct_index[17] = {0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,3 };
    int nC = mbPredictNonZeroCode(mb, idx);
    if (idx == BLOCK_INDEX_LUMA_DC) {
        nC = mbPredictNonZeroCode(mb, 0);
    }
    int coeff_table = 4;
    if (idx != BLOCK_INDEX_CHROMA_DC) {
        coeff_table = ct_index[nC];
    }
    int value = 0;
    int bits = 0;
    int totalCoeff = -1;
    int trailingOnes = -1;
    while(1) {
        value = (value << 1) | (n->read_bits(1));
        bits++;
        int found = 0;
        for(int i = 0; i < 17*4; i++) {
            vlc_t v = x264_coeff_token[coeff_table][i];
            if (v.i_size == bits && v.i_bits == value) {
                found = 1;
                trailingOnes = i % 4;
                totalCoeff = i / 4;
                break;
            }
        }
        if (found) {
            break;
        }
    }
    std::cout << "nc " << nC << " total " << totalCoeff << " trailing " << trailingOnes << std::endl;
    if (idx >= 0) {
        mb->nonZeroCount[x264_scan8[idx]] = totalCoeff;
    }
    if (totalCoeff <= 0) {
        for(int i = 0; i < count; i++) {
            buf[i] = 0;
        }
        return;
    }
    int suffixLength = totalCoeff > 10 && trailingOnes < 3 ? 1: 0;
    uint8_t sign = 0;
    if (trailingOnes > 0) {
        sign = n->read_bits(trailingOnes);
    }
    //read level prefix
    int level[totalCoeff];
    int run[totalCoeff];
    for(int i = 0; i < trailingOnes; i++) {
        if (((sign >> (trailingOnes - 1 - i)) & 0x1) == 1) {
            level[i] = -1;
        } else {
            level[i] = 1;
        }
    }
    for (int i = trailingOnes; i < totalCoeff; i++) {
        int prefixBits = 0;
        while(1) {
            uint8_t v = n->read_bits(1);
            if (v == 1) {
                break;
            }
            prefixBits++;
        }
        int levelCode = 0;
        if (prefixBits < 14) {
            levelCode = prefixBits << suffixLength;
            if (suffixLength > 0) {
                levelCode |= n->read_bits(suffixLength);
            }
        }else if (prefixBits == 14) {
            if (suffixLength == 0) {
                levelCode = n->read_bits(4) + 14;
            } else {
                levelCode = (14 << suffixLength) | n->read_bits(suffixLength);
            }
        } else {
            assert(prefixBits == 15);
            levelCode = n->read_bits(12);
            if (suffixLength == 0) {
                levelCode += 15;
            }
            levelCode += 15 << suffixLength;
        }
        if (i == trailingOnes && trailingOnes < 3) {
            levelCode += 2;
        }
        if (levelCode % 2 == 0) {
            level[i] = (levelCode + 2) / 2;
        } else {
            level[i] = (levelCode + 1) / -2;
        }
        std::cout << "level " << level[i] << std::endl;
        if ( suffixLength == 0 ) {
            suffixLength++;
        }
        if( abs( level[i] ) > ( 3 << ( suffixLength - 1 )) && suffixLength < 6) {
            suffixLength++;
        }
    }
    int totalZeros = 0;
    //read total zeros
    if (totalCoeff < count) {
        int value = 0;
        int bits = 0;
        while(1) {
            value = (value << 1) | n->read_bits(1);
            bits++;
            int found = 0;
            if (idx == BLOCK_INDEX_CHROMA_DC) {
                for(int j = 0; j < 4; j++) {
                    vlc_t v = x264_total_zeros_dc[totalCoeff - 1][j];
                    if (v.i_size == bits && v.i_bits == value) {
                        found = 1;
                        totalZeros = j;
                    }
                }
            } else {
                for(int j = 0; j < 16; j++) {
                    vlc_t v = x264_total_zeros[totalCoeff - 1][j];
                    if (v.i_size == bits && v.i_bits == value) {
                        found = 1;
                        totalZeros = j;
                    }
                }
            }
            if (found) {
                break;
            }
        }
        // assert(totalZeros > 0);
    }
    std::cout << "total zero " << totalZeros << std::endl;

    int i, zeroLeft;
    for( i = 0, zeroLeft = totalZeros; i < totalCoeff - 1; i++ )
    {
        int i_zl;

        if( zeroLeft <= 0 )
        {
            break;
        }

        i_zl = std::min( zeroLeft - 1, 6 );

        int value = 0;
        int bits = 0;
        while(1) {
            value = (value << 1) | (n->read_bits(1));
            bits++;
            int found = 0;
            for(int j = 0; j < 15; j++) {
                vlc_t v = x264_run_before[i_zl][j];
                if (v.i_size == bits && v.i_bits == value) {
                    found = 1;
                    run[i] = j;
                    std::cout << "run " << j << std::endl;
                    break;
                }
            }
            if (found) {
                break;
            }
        }

        zeroLeft -= run[i];
    }

    if (zeroLeft >= 0) {
        run[i] = zeroLeft;
        std::cout << "last run " << run[i] << std::endl;
    }
    int k = 0;
    for(int j = totalCoeff - 1; j > i; j--) {
        std::cout << "buf[" << k << "]=" << level[j] << std::endl;
        buf[k++] = level[j];
    }
    for(int j = i; j >= 0; j--) {
        std::cout << "write zero run[" << j << "]=" << run[j] << std::endl;
        for(int w = 0; w < run[j]; w++) {
            std::cout << "buf[" << k << "]=0" << std::endl;
            buf[k++] = 0;
        }
        std::cout << "buf[" << k << "]=" << level[j] << std::endl;
        buf[k++] = level[j];
    }
    while(k < count) {
        buf[k++] = 0;
    }
}


void H264Decoder::showFrame(FramePtr pf)
{
    int height_ = pf->lines[0];
    int width_ = pf->stride[0] - 64;
    std::vector<std::vector<int32_t>> R(height_, std::move(std::vector<int32_t>(width_, 0)));
    std::vector<std::vector<int32_t>> G(height_, std::move(std::vector<int32_t>(width_, 0)));
    std::vector<std::vector<int32_t>> B(height_, std::move(std::vector<int32_t>(width_, 0)));
    for(int h = 0; h < height_; h++) {
        for(int w = 0; w < width_; w++) {
            int32_t Y = *(pf->plane[0] + h * pf->stride[0] + w);
            int32_t U = *(pf->plane[1] + h/2*pf->stride[1] + w/2) - 128;
            int32_t V = *(pf->plane[2] + h/2*pf->stride[2] + w/2) - 128;
            if (h == 256 && w == 368) {
                // goto aa;
            }
            R[h][w] = Y + 1.402 * V;
            G[h][w] = Y - 0.34414 * U - 0.71414 * V;
            B[h][w] = Y + 1.772 * U;
            R[h][w] = std::max(0, std::min(255, R[h][w]));
            G[h][w] = std::max(0, std::min(255, G[h][w]));
            B[h][w] = std::max(0, std::min(255, B[h][w]));
        }
    }

aa:
    cv::Mat img = cv::Mat(height_, width_, CV_8UC3, cv::Scalar(255,0,0));
    for(int h = 0; h < height_; h++) {
        for(int w = 0; w < width_; w++) {
            img.at<cv::Vec3b>(h, w) = cv::Vec3b(B[h][w], G[h][w], R[h][w]);
        }
    }
    cv::imshow("img", img);
    // cv::Mat origImg = cv::imread(std::string(input_img));
    // cv::imshow("orig", origImg);
    cv::waitKey();
}
