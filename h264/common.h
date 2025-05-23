#ifndef __COMMON_H__
#define __COMMON_H__

#include <cstdint>
enum SliceType {
    SliceTypeP = 0,
    SliceTypeB,
    SliceTypeI,
    SliceTypeSP,
    SliceTypeSI,
    //5-9同上
};

enum MbType {
    I_4x4 = 0,
    I_16x16 = 1,
    I_PCM = 2,
    P_L0 = 3,
    P_16x8 = 4,
    P_8x16 = 5,
    P_8x8 = 6,
    P_SKIP = 7,
};

enum IntraChromaPred
{
    I_PRED_CHROMA_DC = 0,
    I_PRED_CHROMA_H  = 1,
    I_PRED_CHROMA_V  = 2,
    I_PRED_CHROMA_P  = 3,

    I_PRED_CHROMA_DC_LEFT = 4,
    I_PRED_CHROMA_DC_TOP  = 5,
    I_PRED_CHROMA_DC_128  = 6
};

enum Intra16x16Pred
{
    I_PRED_16x16_V  = 0,
    I_PRED_16x16_H  = 1,
    I_PRED_16x16_DC = 2,
    I_PRED_16x16_P  = 3,

    I_PRED_16x16_DC_LEFT = 4,
    I_PRED_16x16_DC_TOP  = 5,
    I_PRED_16x16_DC_128  = 6,
};

enum Intra4x4Pred
{
    I_PRED_4x4_V  = 0,
    I_PRED_4x4_H  = 1,
    I_PRED_4x4_DC = 2,
    I_PRED_4x4_DDL= 3,
    I_PRED_4x4_DDR= 4,
    I_PRED_4x4_VR = 5,
    I_PRED_4x4_HD = 6,
    I_PRED_4x4_VL = 7,
    I_PRED_4x4_HU = 8,

    I_PRED_4x4_DC_LEFT = 9,
    I_PRED_4x4_DC_TOP  = 10,
    I_PRED_4x4_DC_128  = 11,
};

extern uint8_t ISliceMbTable[26][5];
extern uint8_t PSliceMbTable[6];

#define X264_SCAN8_SIZE (6*8)
#define X264_SCAN8_0 (4+1*8)

static const int x264_scan8[16+2*4] =
{
    /* Luma */
    4+1*8, 5+1*8, 4+2*8, 5+2*8,
    6+1*8, 7+1*8, 6+2*8, 7+2*8,
    4+3*8, 5+3*8, 4+4*8, 5+4*8,
    6+3*8, 7+3*8, 6+4*8, 7+4*8,

    /* Cb */
    1+1*8, 2+1*8,
    1+2*8, 2+2*8,

    /* Cr */
    1+4*8, 2+4*8,
    1+5*8, 2+5*8,
};

static const int x264_scan8_x[16] = {0,1,0,1,2,3,2,3,0,1,0,1,2,3,2,3};
static const int x264_scan8_y[16] = {0,0,1,1,0,0,1,1,2,2,3,3,2,2,3,3};

#define BLOCK_INDEX_CHROMA_DC   (-1)
#define BLOCK_INDEX_LUMA_DC     (-2)

static const int zigzag[16] = {
    0,1,4,8,5,2,3,6,9,12,13,10,7,11,14,15
};

#endif // !__COMMON_H__
