#ifndef __MACROBLOCK_H__
#define __MACROBLOCK_H__

#include "H264Decoder.h"
#include <cstdint>
void x264_mb_dequant_4x4( int16_t dct[4][4], int i_qscale );
void x264_mb_decode_i4x4(int qscale, uint8_t* dst, int stride, int *buf, int pred_mode);
void x264_mb_decode_8x8(int b_inter, int i_qscale, int* stride, uint8_t* dst[2], int chroma_dc[2][4], int* residual_ac[8], int chroma_pred_mode);
void macroblock_init();
void x264_mb_decode_i16x16(int i_qscale, int i_stride, uint8_t* dst, int pred_mode, int* luma16x16_dc, int* residual_ac[16]);
void x264_frame_deblocking_filter( FramePtr frame, int i_mb_stride, int i_mb_height, int i_chroma_qp_index_offset);

#endif // _MACROBLOCK_H__
