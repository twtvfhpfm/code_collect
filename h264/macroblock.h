#ifndef __MACROBLOCK_H__
#define __MACROBLOCK_H__

#include "H264Decoder.h"
#include <cstdint>

enum mb_partition_e
{
    /* sub partition type for P_8x8 and B_8x8 */
    D_L0_4x4        = 0,
    D_L0_8x4        = 1,
    D_L0_4x8        = 2,
    D_L0_8x8        = 3,

    /* sub partition type for B_8x8 only */
    D_L1_4x4        = 4,
    D_L1_8x4        = 5,
    D_L1_4x8        = 6,
    D_L1_8x8        = 7,

    D_BI_4x4        = 8,
    D_BI_8x4        = 9,
    D_BI_4x8        = 10,
    D_BI_8x8        = 11,
    D_DIRECT_8x8    = 12,

    /* partition */
    D_8x8           = 13,
    D_16x8          = 14,
    D_8x16          = 15,
    D_16x16         = 16,
};

static const int x264_mb_partition_listX_table[2][17] =
{{
    1, 1, 1, 1, /* D_L0_* */
    0, 0, 0, 0, /* D_L1_* */
    1, 1, 1, 1, /* D_BI_* */
    0,          /* D_DIRECT_8x8 */
    0, 0, 0, 0  /* 8x8 .. 16x16 */
},
{
    0, 0, 0, 0, /* D_L0_* */
    1, 1, 1, 1, /* D_L1_* */
    1, 1, 1, 1, /* D_BI_* */
    0,          /* D_DIRECT_8x8 */
    0, 0, 0, 0  /* 8x8 .. 16x16 */
}};

void x264_mb_dequant_4x4( int16_t dct[4][4], int i_qscale );
void x264_mb_decode_i4x4(int qscale, uint8_t* dst, int stride, int *buf, int pred_mode);
void x264_mb_decode_8x8(int b_inter, int i_qscale, int* stride, uint8_t* dst[2], int chroma_dc[2][4], int* residual_ac[8]);
void macroblock_init();
void x264_mb_decode_i16x16(int i_qscale, int i_stride, uint8_t* dst, int pred_mode, int* luma16x16_dc, int* residual_ac[16]);
void x264_frame_deblocking_filter( FramePtr frame, int i_mb_stride, int i_mb_height, int i_chroma_qp_index_offset);
void x264_mb_predict_mv(MacroBlockPtr mb, int i_list, int idx, int i_width, int mvp[2] );
void x264_mb_predict_mv_pskip(MacroBlockPtr mb, int mv[2] );
void x264_macroblock_decode_pskip(MacroBlockPtr mb, uint8_t* src[3], int src_stride[3], uint8_t* dst[3], int dst_stride[3]);
void x264_mb_mc(MacroBlockPtr mb, uint8_t* src[3], int src_stride[3], uint8_t* dst[3], int dst_stride[3]);
void x264_mb_decode_p16x16(int qscale, uint8_t* dst, int stride, int* luma4x4[16]);
void x264_mb_predict_chroma_8x8(int chroma_pred_mode, uint8_t* dst[2],  int* stride);
void x264_frame_expand_border( FramePtr frame );

#endif // _MACROBLOCK_H__
