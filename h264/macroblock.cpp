#include "macroblock.h"
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include "H264Decoder.h"
#include "common.h"

static inline int clip_uint8( int a )
{
    if (a&(~255))
        return (-a)>>31;
    else
        return a;
}

static inline void zigzag_to_4x4_full( int level[16], int16_t dct[4][4] )
{
    dct[0][0] = level[0];
    dct[0][1] = level[1];
    dct[1][0] = level[2];
    dct[2][0] = level[3];
    dct[1][1] = level[4];
    dct[0][2] = level[5];
    dct[0][3] = level[6];
    dct[1][2] = level[7];
    dct[2][1] = level[8];
    dct[3][0] = level[9];
     dct[3][1]= level[10];
     dct[2][2]= level[11];
     dct[1][3]= level[12];
     dct[2][3]= level[13];
     dct[3][2]= level[14];
     dct[3][3]= level[15];
#if 0
    int i;
    for( i = 0; i < 16; i++ )
    {
        level[i] = dct[scan_zigzag_y[i]][scan_zigzag_x[i]];
    }
#endif
}
static inline void zigzag_to_4x4( int level[15], int16_t dct[4][4] )
{
    dct[0][1] = level[0];
    dct[1][0] = level[1];
    dct[2][0] = level[2];
    dct[1][1] = level[3];
    dct[0][2] = level[4];
    dct[0][3] = level[5];
    dct[1][2] = level[6];
    dct[2][1] = level[7];
    dct[3][0] = level[8];
    dct[3][1] = level[9];
     dct[2][2]= level[10];
     dct[1][3]= level[11];
     dct[2][3]= level[12];
     dct[3][2]= level[13];
     dct[3][3]= level[14];
#if 0
    int i;
    for( i = 1; i < 16; i++ )
    {
        level[i - 1] = dct[scan_zigzag_y[i]][scan_zigzag_x[i]];
    }
#endif
}

static inline void zigzag_to_2x2_dc( int level[4], int16_t dct[2][2] )
{
    dct[0][0] = level[0];
    dct[0][1] = level[1];
    dct[1][0] = level[2];
    dct[1][1] = level[3];
}

static const int dequant_mf[6][4][4] =
{
    { {10, 13, 10, 13}, {13, 16, 13, 16}, {10, 13, 10, 13}, {13, 16, 13, 16} },
    { {11, 14, 11, 14}, {14, 18, 14, 18}, {11, 14, 11, 14}, {14, 18, 14, 18} },
    { {13, 16, 13, 16}, {16, 20, 16, 20}, {13, 16, 13, 16}, {16, 20, 16, 20} },
    { {14, 18, 14, 18}, {18, 23, 18, 23}, {14, 18, 14, 18}, {18, 23, 18, 23} },
    { {16, 20, 16, 20}, {20, 25, 20, 25}, {16, 20, 16, 20}, {20, 25, 20, 25} },
    { {18, 23, 18, 23}, {23, 29, 23, 29}, {18, 23, 18, 23}, {23, 29, 23, 29} }
};

void x264_mb_dequant_4x4( int16_t dct[4][4], int i_qscale )
{
    const int i_mf = i_qscale%6;
    const int i_qbits = i_qscale/6;
    int y;

    for( y = 0; y < 4; y++ )
    {
        dct[y][0] = ( dct[y][0] * dequant_mf[i_mf][y][0] ) << i_qbits;
        dct[y][1] = ( dct[y][1] * dequant_mf[i_mf][y][1] ) << i_qbits;
        dct[y][2] = ( dct[y][2] * dequant_mf[i_mf][y][2] ) << i_qbits;
        dct[y][3] = ( dct[y][3] * dequant_mf[i_mf][y][3] ) << i_qbits;
    }
}

/****************************************************************************
 * 4x4 prediction for intra luma block DC, H, V, P
 ****************************************************************************/
static void predict_4x4_dc_128( uint8_t *src, int i_stride )
{
    int x,y;
    for( y = 0; y < 4; y++ )
    {
        for( x = 0; x < 4; x++ )
        {
            src[x] = 128;
        }
        src += i_stride;
    }
}
static void predict_4x4_dc_left( uint8_t *src, int i_stride )
{
    int x,y;
    int dc = ( src[-1+0*i_stride] + src[-1+i_stride]+
               src[-1+2*i_stride] + src[-1+3*i_stride] + 2 ) >> 2;

    for( y = 0; y < 4; y++ )
    {
        for( x = 0; x < 4; x++ )
        {
            src[x] = dc;
        }
        src += i_stride;
    }
}
static void predict_4x4_dc_top( uint8_t *src, int i_stride )
{
    int x,y;
    int dc = ( src[0 - i_stride] + src[1 - i_stride] +
               src[2 - i_stride] + src[3 - i_stride] + 2 ) >> 2;

    for( y = 0; y < 4; y++ )
    {
        for( x = 0; x < 4; x++ )
        {
            src[x] = dc;
        }
        src += i_stride;
    }
}
static void predict_4x4_dc( uint8_t *src, int i_stride )
{
    int x,y;
    int dc = ( src[-1+0*i_stride] + src[-1+i_stride]+
               src[-1+2*i_stride] + src[-1+3*i_stride] +
               src[0 - i_stride]  + src[1 - i_stride] +
               src[2 - i_stride]  + src[3 - i_stride] + 4 ) >> 3;

    for( y = 0; y < 4; y++ )
    {
        for( x = 0; x < 4; x++ )
        {
            src[x] = dc;
        }
        src += i_stride;
    }
}
static void predict_4x4_h( uint8_t *src, int i_stride )
{
    int i,j;

    for( i = 0; i < 4; i++ )
    {
        uint8_t v;

        v = src[-1];

        for( j = 0; j < 4; j++ )
        {
            src[j] = v;
        }
        src += i_stride;
    }
}
static void predict_4x4_v( uint8_t *src, int i_stride )
{
    int i,j;

    for( i = 0; i < 4; i++ )
    {
        for( j = 0; j < 4; j++ )
        {
            src[i * i_stride +j] = src[j - i_stride];
        }
    }
}

#define PREDICT_4x4_LOAD_LEFT \
    const int l0 = src[-1+0*i_stride];   \
    const int l1 = src[-1+1*i_stride];   \
    const int l2 = src[-1+2*i_stride];   \
    const int l3 = src[-1+3*i_stride];

#define PREDICT_4x4_LOAD_TOP \
    const int t0 = src[0-1*i_stride];   \
    const int t1 = src[1-1*i_stride];   \
    const int t2 = src[2-1*i_stride];   \
    const int t3 = src[3-1*i_stride];

#define PREDICT_4x4_LOAD_TOP_RIGHT \
    const int t4 = src[4-1*i_stride];   \
    const int t5 = src[5-1*i_stride];   \
    const int t6 = src[6-1*i_stride];   \
    const int t7 = src[7-1*i_stride];


static void predict_4x4_ddl( uint8_t *src, int i_stride )
{
    PREDICT_4x4_LOAD_TOP
    PREDICT_4x4_LOAD_TOP_RIGHT

    src[0*i_stride+0] = ( t0 + 2*t1+ t2 + 2 ) >> 2;

    src[0*i_stride+1] =
    src[1*i_stride+0] = ( t1 + 2*t2+ t3 + 2 ) >> 2;

    src[0*i_stride+2] =
    src[1*i_stride+1] =
    src[2*i_stride+0] = ( t2 + 2*t3+ t4 + 2 ) >> 2;

    src[0*i_stride+3] =
    src[1*i_stride+2] =
    src[2*i_stride+1] =
    src[3*i_stride+0] = ( t3 + 2*t4+ t5 + 2 ) >> 2;

    src[1*i_stride+3] =
    src[2*i_stride+2] =
    src[3*i_stride+1] = ( t4 + 2*t5+ t6 + 2 ) >> 2;

    src[2*i_stride+3] =
    src[3*i_stride+2] = ( t5 + 2*t6+ t7 + 2 ) >> 2;

    src[3*i_stride+3] = ( t6 + 3 * t7 + 2 ) >> 2;
}
static void predict_4x4_ddr( uint8_t *src, int i_stride )
{
    const int lt = src[-1-i_stride];
    PREDICT_4x4_LOAD_LEFT
    PREDICT_4x4_LOAD_TOP

    src[0*i_stride+0] =
    src[1*i_stride+1] =
    src[2*i_stride+2] =
    src[3*i_stride+3] = ( t0 + 2*lt +l0 + 2 ) >> 2;

    src[0*i_stride+1] =
    src[1*i_stride+2] =
    src[2*i_stride+3] = ( lt + 2 * t0 + t1 + 2 ) >> 2;

    src[0*i_stride+2] =
    src[1*i_stride+3] = ( t0 + 2 * t1 + t2 + 2 ) >> 2;

    src[0*i_stride+3] = ( t1 + 2 * t2 + t3 + 2 ) >> 2;

    src[1*i_stride+0] =
    src[2*i_stride+1] =
    src[3*i_stride+2] = ( lt + 2 * l0 + l1 + 2 ) >> 2;

    src[2*i_stride+0] =
    src[3*i_stride+1] = ( l0 + 2 * l1 + l2 + 2 ) >> 2;

    src[3*i_stride+0] = ( l1 + 2 * l2 + l3 + 2 ) >> 2;
}

static void predict_4x4_vr( uint8_t *src, int i_stride )
{
    const int lt = src[-1-i_stride];
    PREDICT_4x4_LOAD_LEFT
    PREDICT_4x4_LOAD_TOP
    /* produce warning as l3 is unused */

    src[0*i_stride+0]=
    src[2*i_stride+1]= ( lt + t0 + 1 ) >> 1;

    src[0*i_stride+1]=
    src[2*i_stride+2]= ( t0 + t1 + 1 ) >> 1;

    src[0*i_stride+2]=
    src[2*i_stride+3]= ( t1 + t2 + 1 ) >> 1;

    src[0*i_stride+3]= ( t2 + t3 + 1 ) >> 1;

    src[1*i_stride+0]=
    src[3*i_stride+1]= ( l0 + 2 * lt + t0 + 2 ) >> 2;

    src[1*i_stride+1]=
    src[3*i_stride+2]= ( lt + 2 * t0 + t1 + 2 ) >> 2;

    src[1*i_stride+2]=
    src[3*i_stride+3]= ( t0 + 2 * t1 + t2 + 2) >> 2;

    src[1*i_stride+3]= ( t1 + 2 * t2 + t3 + 2 ) >> 2;
    src[2*i_stride+0]= ( lt + 2 * l0 + l1 + 2 ) >> 2;
    src[3*i_stride+0]= ( l0 + 2 * l1 + l2 + 2 ) >> 2;
}

static void predict_4x4_hd( uint8_t *src, int i_stride )
{
    const int lt= src[-1-1*i_stride];
    PREDICT_4x4_LOAD_LEFT
    PREDICT_4x4_LOAD_TOP
    /* produce warning as t3 is unused */

    src[0*i_stride+0]=
    src[1*i_stride+2]= ( lt + l0 + 1 ) >> 1;
    src[0*i_stride+1]=
    src[1*i_stride+3]= ( l0 + 2 * lt + t0 + 2 ) >> 2;
    src[0*i_stride+2]= ( lt + 2 * t0 + t1 + 2 ) >> 2;
    src[0*i_stride+3]= ( t0 + 2 * t1 + t2 + 2 ) >> 2;
    src[1*i_stride+0]=
    src[2*i_stride+2]= ( l0 + l1 + 1 ) >> 1;
    src[1*i_stride+1]=
    src[2*i_stride+3]= ( lt + 2 * l0 + l1 + 2 ) >> 2;
    src[2*i_stride+0]=
    src[3*i_stride+2]= ( l1 + l2+ 1 ) >> 1;
    src[2*i_stride+1]=
    src[3*i_stride+3]= ( l0 + 2 * l1 + l2 + 2 ) >> 2;
    src[3*i_stride+0]= ( l2 + l3 + 1 ) >> 1;
    src[3*i_stride+1]= ( l1 + 2 * l2 + l3 + 2 ) >> 2;
}

static void predict_4x4_vl( uint8_t *src, int i_stride )
{
    PREDICT_4x4_LOAD_TOP
    PREDICT_4x4_LOAD_TOP_RIGHT
    /* produce warning as t7 is unused */

    src[0*i_stride+0]= ( t0 + t1 + 1 ) >> 1;
    src[0*i_stride+1]=
    src[2*i_stride+0]= ( t1 + t2 + 1 ) >> 1;
    src[0*i_stride+2]=
    src[2*i_stride+1]= ( t2 + t3 + 1 ) >> 1;
    src[0*i_stride+3]=
    src[2*i_stride+2]= ( t3 + t4+ 1 ) >> 1;
    src[2*i_stride+3]= ( t4 + t5+ 1 ) >> 1;
    src[1*i_stride+0]= ( t0 + 2 * t1 + t2 + 2 ) >> 2;
    src[1*i_stride+1]=
    src[3*i_stride+0]= ( t1 + 2 * t2 + t3 + 2 ) >> 2;
    src[1*i_stride+2]=
    src[3*i_stride+1]= ( t2 + 2 * t3 + t4 + 2 ) >> 2;
    src[1*i_stride+3]=
    src[3*i_stride+2]= ( t3 + 2 * t4 + t5 + 2 ) >> 2;
    src[3*i_stride+3]= ( t4 + 2 * t5 + t6 + 2 ) >> 2;
}

static void predict_4x4_hu( uint8_t *src, int i_stride )
{
    PREDICT_4x4_LOAD_LEFT

    src[0*i_stride+0]= ( l0 + l1 + 1 ) >> 1;
    src[0*i_stride+1]= ( l0 + 2 * l1 + l2 + 2 ) >> 2;

    src[0*i_stride+2]=
    src[1*i_stride+0]= ( l1 + l2 + 1 ) >> 1;

    src[0*i_stride+3]=
    src[1*i_stride+1]= ( l1 + 2*l2 + l3 + 2 ) >> 2;

    src[1*i_stride+2]=
    src[2*i_stride+0]= ( l2 + l3 + 1 ) >> 1;

    src[1*i_stride+3]=
    src[2*i_stride+1]= ( l2 + 2 * l3 + l3 + 2 ) >> 2;

    src[2*i_stride+3]=
    src[3*i_stride+1]=
    src[3*i_stride+0]=
    src[2*i_stride+2]=
    src[3*i_stride+2]=
    src[3*i_stride+3]= l3;
}

typedef void (*x264_predict_t)( uint8_t *src, int i_stride );
x264_predict_t g_predict_4x4[12];

/****************************************************************************
 * 8x8 prediction for intra chroma block DC, H, V, P
 ****************************************************************************/
static void predict_8x8_dc_128( uint8_t *src, int i_stride )
{
    int x,y;

    for( y = 0; y < 8; y++ )
    {
        for( x = 0; x < 8; x++ )
        {
            src[x] = 128;
        }
        src += i_stride;
    }
}
static void predict_8x8_dc_left( uint8_t *src, int i_stride )
{
    int x,y;
    int dc0 = 0, dc1 = 0;

    for( y = 0; y < 4; y++ )
    {
        dc0 += src[y * i_stride     - 1];
        dc1 += src[(y+4) * i_stride - 1];
    }
    dc0 = ( dc0 + 2 ) >> 2;
    dc1 = ( dc1 + 2 ) >> 2;

    for( y = 0; y < 4; y++ )
    {
        for( x = 0; x < 8; x++ )
        {
            src[           x] = dc0;
            src[4*i_stride+x] = dc1;
        }
        src += i_stride;
    }
}
static void predict_8x8_dc_top( uint8_t *src, int i_stride )
{
    int x,y;
    int dc0 = 0, dc1 = 0;

    for( x = 0; x < 4; x++ )
    {
        dc0 += src[x     - i_stride];
        dc1 += src[x + 4 - i_stride];
    }
    dc0 = ( dc0 + 2 ) >> 2;
    dc1 = ( dc1 + 2 ) >> 2;

    for( y = 0; y < 8; y++ )
    {
        for( x = 0; x < 4; x++ )
        {
            src[x    ] = dc0;
            src[x + 4] = dc1;
        }
        src += i_stride;
    }
}
static void predict_8x8_dc( uint8_t *src, int i_stride )
{
    int x,y;
    int s0 = 0, s1 = 0, s2 = 0, s3 = 0;
    int dc0, dc1, dc2, dc3;
    int i;

    /* First do :
          s0 s1
       s2
       s3
    */
    for( i = 0; i < 4; i++ )
    {
        s0 += src[i - i_stride];
        s1 += src[i + 4 - i_stride];
        s2 += src[-1 + i * i_stride];
        s3 += src[-1 + (i+4)*i_stride];
    }
    /* now calculate
       dc0 dc1
       dc2 dc3
     */
    dc0 = ( s0 + s2 + 4 ) >> 3;
    dc1 = ( s1 + 2 ) >> 2;
    dc2 = ( s3 + 2 ) >> 2;
    dc3 = ( s1 + s3 + 4 ) >> 3;

    for( y = 0; y < 4; y++ )
    {
        for( x = 0; x < 4; x++ )
        {
            src[             x    ] = dc0;
            src[             x + 4] = dc1;
            src[4*i_stride + x    ] = dc2;
            src[4*i_stride + x + 4] = dc3;
        }
        src += i_stride;
    }
}

static void predict_8x8_h( uint8_t *src, int i_stride )
{
    int i,j;

    for( i = 0; i < 8; i++ )
    {
        uint8_t v;

        v = src[-1];

        for( j = 0; j < 8; j++ )
        {
            src[j] = v;
        }
        src += i_stride;
    }
}
static void predict_8x8_v( uint8_t *src, int i_stride )
{
    int i,j;

    for( i = 0; i < 8; i++ )
    {
        for( j = 0; j < 8; j++ )
        {
            src[i * i_stride +j] = src[j - i_stride];
        }
    }
}

static void predict_8x8_p( uint8_t *src, int i_stride )
{
    int i;
    int x,y;
    int a, b, c;
    int H = 0;
    int V = 0;
    int i00;

    for( i = 0; i < 4; i++ )
    {
        H += ( i + 1 ) * ( src[4+i - i_stride] - src[2 - i -i_stride] );
        V += ( i + 1 ) * ( src[-1 +(i+4)*i_stride] - src[-1+(2-i)*i_stride] );
    }

    a = 16 * ( src[-1+7*i_stride] + src[7 - i_stride] );
    b = ( 17 * H + 16 ) >> 5;
    c = ( 17 * V + 16 ) >> 5;
    i00 = a -3*b -3*c + 16;

    for( y = 0; y < 8; y++ )
    {
        for( x = 0; x < 8; x++ )
        {
            int pix;

            pix = (i00 +b*x) >> 5;
            src[x] = clip_uint8( pix );
        }
        src += i_stride;
        i00 += c;
    }
}

x264_predict_t g_predict_8x8[7];

/****************************************************************************
 * 16x16 prediction for intra block DC, H, V, P
 ****************************************************************************/
static void predict_16x16_dc( uint8_t *src, int i_stride )
{
    int dc = 0;
    int i, j;

    /* calculate DC value */
    for( i = 0; i < 16; i++ )
    {
        dc += src[-1 + i * i_stride];
        dc += src[i - i_stride];
    }
    dc = ( dc + 16 ) >> 5;

    for( i = 0; i < 16; i++ )
    {
        for( j = 0; j < 16; j++ )
        {
            src[j] = dc;
        }
        src += i_stride;
    }
}
static void predict_16x16_dc_left( uint8_t *src, int i_stride )
{
    int dc = 0;
    int i,j;

    for( i = 0; i < 16; i++ )
    {
        dc += src[-1 + i * i_stride];
    }
    dc = ( dc + 8 ) >> 4;

    for( i = 0; i < 16; i++ )
    {
        for( j = 0; j < 16; j++ )
        {
            src[j] = dc;
        }
        src += i_stride;
    }
}
static void predict_16x16_dc_top( uint8_t *src, int i_stride )
{
    int dc = 0;
    int i,j;

    for( i = 0; i < 16; i++ )
    {
        dc += src[i - i_stride];
    }
    dc = ( dc + 8 ) >> 4;

    for( i = 0; i < 16; i++ )
    {
        for( j = 0; j < 16; j++ )
        {
            src[j] = dc;
        }
        src += i_stride;
    }
}
static void predict_16x16_dc_128( uint8_t *src, int i_stride )
{
    int i,j;

    for( i = 0; i < 16; i++ )
    {
        for( j = 0; j < 16; j++ )
        {
            src[j] = 128;
        }
        src += i_stride;
    }
}
static void predict_16x16_h( uint8_t *src, int i_stride )
{
    int i,j;

    for( i = 0; i < 16; i++ )
    {
        uint8_t v;

        v = src[-1];
        for( j = 0; j < 16; j++ )
        {
            src[j] = v;
        }
        src += i_stride;

    }
}
static void predict_16x16_v( uint8_t *src, int i_stride )
{
    int i,j;

    for( i = 0; i < 16; i++ )
    {
        for( j = 0; j < 16; j++ )
        {
            src[i * i_stride +j] = src[j - i_stride];
        }
    }
}
static void predict_16x16_p( uint8_t *src, int i_stride )
{
    int x, y, i;
    int a, b, c;
    int H = 0;
    int V = 0;
    int i00;

    /* calcule H and V */
    for( i = 0; i <= 7; i++ )
    {
        H += ( i + 1 ) * ( src[ 8 + i - i_stride ] - src[6 -i -i_stride] );
        V += ( i + 1 ) * ( src[-1 + (8+i)*i_stride] - src[-1 + (6-i)*i_stride] );
    }

    a = 16 * ( src[-1 + 15*i_stride] + src[15 - i_stride] );
    b = ( 5 * H + 32 ) >> 6;
    c = ( 5 * V + 32 ) >> 6;

    i00 = a - b * 7 - c * 7 + 16;

    for( y = 0; y < 16; y++ )
    {
        for( x = 0; x < 16; x++ )
        {
            int pix;

            pix = (i00+b*x)>>5;

            src[x] = clip_uint8( pix );
        }
        src += i_stride;
        i00 += c;
    }
}

x264_predict_t g_predict_16x16[7];

void macroblock_init()
{
    g_predict_8x8[I_PRED_CHROMA_V ]     = predict_8x8_v;
    g_predict_8x8[I_PRED_CHROMA_H ]     = predict_8x8_h;
    g_predict_8x8[I_PRED_CHROMA_DC]     = predict_8x8_dc;
    g_predict_8x8[I_PRED_CHROMA_P ]     = predict_8x8_p;
    g_predict_8x8[I_PRED_CHROMA_DC_LEFT]= predict_8x8_dc_left;
    g_predict_8x8[I_PRED_CHROMA_DC_TOP ]= predict_8x8_dc_top;
    g_predict_8x8[I_PRED_CHROMA_DC_128 ]= predict_8x8_dc_128;

    g_predict_4x4[I_PRED_4x4_V]      = predict_4x4_v;
    g_predict_4x4[I_PRED_4x4_H]      = predict_4x4_h;
    g_predict_4x4[I_PRED_4x4_DC]     = predict_4x4_dc;
    g_predict_4x4[I_PRED_4x4_DDL]    = predict_4x4_ddl;
    g_predict_4x4[I_PRED_4x4_DDR]    = predict_4x4_ddr;
    g_predict_4x4[I_PRED_4x4_VR]     = predict_4x4_vr;
    g_predict_4x4[I_PRED_4x4_HD]     = predict_4x4_hd;
    g_predict_4x4[I_PRED_4x4_VL]     = predict_4x4_vl;
    g_predict_4x4[I_PRED_4x4_HU]     = predict_4x4_hu;
    g_predict_4x4[I_PRED_4x4_DC_LEFT]= predict_4x4_dc_left;
    g_predict_4x4[I_PRED_4x4_DC_TOP] = predict_4x4_dc_top;
    g_predict_4x4[I_PRED_4x4_DC_128] = predict_4x4_dc_128;

    g_predict_16x16[I_PRED_16x16_V ]     = predict_16x16_v;
    g_predict_16x16[I_PRED_16x16_H ]     = predict_16x16_h;
    g_predict_16x16[I_PRED_16x16_DC]     = predict_16x16_dc;
    g_predict_16x16[I_PRED_16x16_P ]     = predict_16x16_p;
    g_predict_16x16[I_PRED_16x16_DC_LEFT]= predict_16x16_dc_left;
    g_predict_16x16[I_PRED_16x16_DC_TOP ]= predict_16x16_dc_top;
    g_predict_16x16[I_PRED_16x16_DC_128 ]= predict_16x16_dc_128;
}


static void add4x4_idct( uint8_t *p_dst, int i_dst, int16_t dct[4][4] )
{
    int16_t d[4][4];
    int16_t tmp[4][4];
    int x, y;
    int i;

    for( i = 0; i < 4; i++ )
    {
        const int s02 =  dct[i][0]     +  dct[i][2];
        const int d02 =  dct[i][0]     -  dct[i][2];
        const int s13 =  dct[i][1]     + (dct[i][3]>>1);
        const int d13 = (dct[i][1]>>1) -  dct[i][3];

        tmp[i][0] = s02 + s13;
        tmp[i][1] = d02 + d13;
        tmp[i][2] = d02 - d13;
        tmp[i][3] = s02 - s13;
    }

    for( i = 0; i < 4; i++ )
    {
        const int s02 =  tmp[0][i]     +  tmp[2][i];
        const int d02 =  tmp[0][i]     -  tmp[2][i];
        const int s13 =  tmp[1][i]     + (tmp[3][i]>>1);
        const int d13 = (tmp[1][i]>>1) -   tmp[3][i];

        d[0][i] = ( s02 + s13 + 32 ) >> 6;
        d[1][i] = ( d02 + d13 + 32 ) >> 6;
        d[2][i] = ( d02 - d13 + 32 ) >> 6;
        d[3][i] = ( s02 - s13 + 32 ) >> 6;
    }


    for( y = 0; y < 4; y++ )
    {
        for( x = 0; x < 4; x++ )
        {
            p_dst[x] = clip_uint8( p_dst[x] + d[y][x] );
        }
        p_dst += i_dst;
    }
}

static void add8x8_idct( uint8_t *p_dst, int i_dst, int16_t dct[4][4][4] )
{
    add4x4_idct( p_dst, i_dst,             dct[0] );
    add4x4_idct( &p_dst[4], i_dst,         dct[1] );
    add4x4_idct( &p_dst[4*i_dst+0], i_dst, dct[2] );
    add4x4_idct( &p_dst[4*i_dst+4], i_dst, dct[3] );
}

static void add16x16_idct( uint8_t *p_dst, int i_dst, int16_t dct[16][4][4] )
{
    add8x8_idct( &p_dst[0], i_dst, &dct[0] );
    add8x8_idct( &p_dst[8], i_dst, &dct[4] );
    add8x8_idct( &p_dst[8*i_dst], i_dst, &dct[8] );
    add8x8_idct( &p_dst[8*i_dst+8], i_dst, &dct[12] );
}
 
void x264_mb_decode_i4x4(int qscale, uint8_t* dst, int stride, int *buf, int pred_mode)
{
    int16_t dct[4][4];
    zigzag_to_4x4_full(buf, dct);
    x264_mb_dequant_4x4(dct, qscale);
    g_predict_4x4[pred_mode](dst, stride);
    add4x4_idct(dst, stride, dct);
    printf("4x4 pred mode: %d, qp: %d\n", pred_mode, qscale);
    printf("luma4x4: ");
    for(int i = 0; i < 16; i++) {
        printf("%d ", buf[i]);
    }
    printf("\n");
    for(int i = 0; i < 4; i++) {
        uint8_t* p = dst+i*stride;
        for(int j = 0; j < 4; j++) {
            printf("%d ", p[j]);
        }
        printf("\n");
    }
    return;
}

void x264_mb_dequant_2x2_dc( int16_t dct[2][2], int i_qscale )
{
    const int i_qbits = i_qscale/6 - 1;

    if( i_qbits >= 0 )
    {
        const int i_dmf = dequant_mf[i_qscale%6][0][0] << i_qbits;

        dct[0][0] = dct[0][0] * i_dmf;
        dct[0][1] = dct[0][1] * i_dmf;
        dct[1][0] = dct[1][0] * i_dmf;
        dct[1][1] = dct[1][1] * i_dmf;
    }
    else
    {
        const int i_dmf = dequant_mf[i_qscale%6][0][0];

        dct[0][0] = ( dct[0][0] * i_dmf ) >> 1;
        dct[0][1] = ( dct[0][1] * i_dmf ) >> 1;
        dct[1][0] = ( dct[1][0] * i_dmf ) >> 1;
        dct[1][1] = ( dct[1][1] * i_dmf ) >> 1;
    }
}

static void dct2x2dc( int16_t d[2][2] )
{
    int tmp[2][2];

    tmp[0][0] = d[0][0] + d[0][1];
    tmp[1][0] = d[0][0] - d[0][1];
    tmp[0][1] = d[1][0] + d[1][1];
    tmp[1][1] = d[1][0] - d[1][1];

    d[0][0] = tmp[0][0] + tmp[0][1];
    d[0][1] = tmp[1][0] + tmp[1][1];
    d[1][0] = tmp[0][0] - tmp[0][1];
    d[1][1] = tmp[1][0] - tmp[1][1];
}

void x264_mb_predict_chroma_8x8(int chroma_pred_mode, uint8_t* dst[2],  int* stride)
{
    printf("8x8 pred mode: %d\n", chroma_pred_mode);
    for(int ch = 0; ch < 2; ch++ )
    {
        const int i_stride = stride[1+ch];
        g_predict_8x8[chroma_pred_mode](dst[ch], i_stride);
    }
}

void x264_mb_decode_8x8(int b_inter, int i_qscale, int* stride, uint8_t* dst[2], int chroma_dc[2][4], int* residual_ac[8])
{
    int i, ch;

    for( ch = 0; ch < 2; ch++ )
    {
        const int i_stride = stride[1+ch];

        int16_t dct2x2[2][2];
        int16_t dct4x4[4][4][4];

        zigzag_to_2x2_dc(chroma_dc[ch], dct2x2);
        dct2x2dc(dct2x2); //idct equal to dct
        x264_mb_dequant_2x2_dc(dct2x2, i_qscale);
        for(i = 0; i < 4; i++) {
            zigzag_to_4x4(residual_ac[i+ch*4], dct4x4[i]);
            x264_mb_dequant_4x4(dct4x4[i], i_qscale);
            dct4x4[i][0][0] = dct2x2[x264_scan8_y[i]][x264_scan8_x[i]];
        }
        add8x8_idct(dst[ch], i_stride, dct4x4);
        printf("--------\n");
        for(i=0; i<8;i++) {
            uint8_t* p=dst[ch]+i*i_stride;
            for(int j=0;j<8;j++) {
                printf("%d ", p[j]);
            }
            printf("\n");
        }

    }
}

static void idct4x4dc( int16_t d[4][4] )
{
    int16_t tmp[4][4];
    int s01, s23;
    int d01, d23;
    int i;

    for( i = 0; i < 4; i++ )
    {
        s01 = d[0][i] + d[1][i];
        d01 = d[0][i] - d[1][i];
        s23 = d[2][i] + d[3][i];
        d23 = d[2][i] - d[3][i];

        tmp[0][i] = s01 + s23;
        tmp[1][i] = s01 - s23;
        tmp[2][i] = d01 - d23;
        tmp[3][i] = d01 + d23;
    }

    for( i = 0; i < 4; i++ )
    {
        s01 = tmp[i][0] + tmp[i][1];
        d01 = tmp[i][0] - tmp[i][1];
        s23 = tmp[i][2] + tmp[i][3];
        d23 = tmp[i][2] - tmp[i][3];

        d[i][0] = s01 + s23;
        d[i][1] = s01 - s23;
        d[i][2] = d01 - d23;
        d[i][3] = d01 + d23;
    }
}

void x264_mb_dequant_4x4_dc( int16_t dct[4][4], int i_qscale )
{
    const int i_qbits = i_qscale/6 - 2;
    int x,y;

    if( i_qbits >= 0 )
    {
        const int i_dmf = dequant_mf[i_qscale%6][0][0] << i_qbits;

        for( y = 0; y < 4; y++ )
        {
            for( x = 0; x < 4; x++ )
            {
                dct[y][x] = dct[y][x] * i_dmf;
            }
        }
    }
    else
    {
        const int i_dmf = dequant_mf[i_qscale%6][0][0];
        const int f = 1 << ( 1 + i_qbits );

        for( y = 0; y < 4; y++ )
        {
            for( x = 0; x < 4; x++ )
            {
                dct[y][x] = ( dct[y][x] * i_dmf + f ) >> (-i_qbits);
            }
        }
    }
}

void x264_mb_decode_i16x16(int i_qscale, int i_stride, uint8_t* dst, int pred_mode, int* luma16x16_dc, int* residual_ac[16])
{
    int16_t dct4x4[16+1][4][4];

    printf("16x16 pred mode: %d, qp:%d\n", pred_mode, i_qscale);
    int i;
    g_predict_16x16[pred_mode](dst, i_stride);
    zigzag_to_4x4_full(luma16x16_dc, dct4x4[0]);
    idct4x4dc( dct4x4[0] );
    x264_mb_dequant_4x4_dc( dct4x4[0], i_qscale );  /* XXX not inversed */
    for(i = 0; i < 16; i++) {
        zigzag_to_4x4(residual_ac[i], dct4x4[i+1]);
        x264_mb_dequant_4x4(dct4x4[i+1], i_qscale);
        dct4x4[1+i][0][0] = dct4x4[0][x264_scan8_y[i]][x264_scan8_x[i]];
    }
    add16x16_idct(dst, i_stride, &dct4x4[1]);
    printf("--------\n");
    for(i=0; i<16;i++) {
        uint8_t* p=dst+i*i_stride;
        for(int j=0;j<16;j++) {
            printf("%d ", p[j]);
        }
        printf("\n");
    }
}

/* Deblocking filter (p153) */
static const int i_alpha_table[52] =
{
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  4,  4,  5,  6,
     7,  8,  9, 10, 12, 13, 15, 17, 20, 22,
    25, 28, 32, 36, 40, 45, 50, 56, 63, 71,
    80, 90,101,113,127,144,162,182,203,226,
    255, 255
};
static const int i_beta_table[52] =
{
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  2,  2,  2,  3,
     3,  3,  3,  4,  4,  4,  6,  6,  7,  7,
     8,  8,  9,  9, 10, 10, 11, 11, 12, 12,
    13, 13, 14, 14, 15, 15, 16, 16, 17, 17,
    18, 18
};
static const int i_tc0_table[52][3] =
{
    { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 },
    { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 },
    { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 1 },
    { 0, 0, 1 }, { 0, 0, 1 }, { 0, 0, 1 }, { 0, 1, 1 }, { 0, 1, 1 }, { 1, 1, 1 },
    { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 2 }, { 1, 1, 2 }, { 1, 1, 2 },
    { 1, 1, 2 }, { 1, 2, 3 }, { 1, 2, 3 }, { 2, 2, 3 }, { 2, 2, 4 }, { 2, 3, 4 },
    { 2, 3, 4 }, { 3, 3, 5 }, { 3, 4, 6 }, { 3, 4, 6 }, { 4, 5, 7 }, { 4, 5, 8 },
    { 4, 6, 9 }, { 5, 7,10 }, { 6, 8,11 }, { 6, 8,13 }, { 7,10,14 }, { 8,11,16 },
    { 9,12,18 }, {10,13,20 }, {11,15,23 }, {13,17,25 }
};

static inline int x264_clip3( int v, int i_min, int i_max )
{
    return ( (v < i_min) ? i_min : (v > i_max) ? i_max : v );
}

static inline float x264_clip3f( float v, float f_min, float f_max )
{
    return ( (v < f_min) ? f_min : (v > f_max) ? f_max : v );
}

static inline void deblocking_filter_edgev(SliceHeader* h, uint8_t *pix, int i_pix_stride, int bS[4], int i_QP )
{
    int i, d;
    const int i_index_a = x264_clip3( i_QP + h->sliceAlphaC0Offset, 0, 51 );
    const int alpha = i_alpha_table[i_index_a];
    const int beta  = i_beta_table[x264_clip3( i_QP + h->sliceBetaOffset, 0, 51 )];

    for( i = 0; i < 4; i++ )
    {
        if( bS[i] == 0 )
        {
            pix += 4 * i_pix_stride;
            continue;
        }

        if( bS[i] < 4 )
        {
            const int tc0 = i_tc0_table[i_index_a][bS[i] - 1];

            /* 4px edge length */
            for( d = 0; d < 4; d++ )
            {
                const int p0 = pix[-1];
                const int p1 = pix[-2];
                const int p2 = pix[-3];
                const int q0 = pix[0];
                const int q1 = pix[1];
                const int q2 = pix[2];

                if( abs( p0 - q0 ) < alpha &&
                    abs( p1 - p0 ) < beta &&
                    abs( q1 - q0 ) < beta )
                {
                    int tc = tc0;
                    int i_delta;

                    if( abs( p2 - p0 ) < beta )
                    {
                        pix[-2] = p1 + x264_clip3( ( p2 + ( ( p0 + q0 + 1 ) >> 1 ) - ( p1 << 1 ) ) >> 1, -tc0, tc0 );
                        tc++;
                    }
                    if( abs( q2 - q0 ) < beta )
                    {
                        pix[1] = q1 + x264_clip3( ( q2 + ( ( p0 + q0 + 1 ) >> 1 ) - ( q1 << 1 ) ) >> 1, -tc0, tc0 );
                        tc++;
                    }

                    i_delta = x264_clip3( (((q0 - p0 ) << 2) + (p1 - q1) + 4) >> 3, -tc, tc );
                    pix[-1] = clip_uint8( p0 + i_delta );    /* p0' */
                    pix[0]  = clip_uint8( q0 - i_delta );    /* q0' */
                }
                pix += i_pix_stride;
            }
        }
        else
        {
            /* 4px edge length */
            for( d = 0; d < 4; d++ )
            {
                const int p0 = pix[-1];
                const int p1 = pix[-2];
                const int p2 = pix[-3];

                const int q0 = pix[0];
                const int q1 = pix[1];
                const int q2 = pix[2];

                if( abs( p0 - q0 ) < alpha &&
                    abs( p1 - p0 ) < beta &&
                    abs( q1 - q0 ) < beta )
                {
                    if( abs( p0 - q0 ) < (( alpha >> 2 ) + 2 ) )
                    {
                        if( abs( p2 - p0 ) < beta )
                        {
                            const int p3 = pix[-4];
                            /* p0', p1', p2' */
                            pix[-1] = ( p2 + 2*p1 + 2*p0 + 2*q0 + q1 + 4 ) >> 3;
                            pix[-2] = ( p2 + p1 + p0 + q0 + 2 ) >> 2;
                            pix[-3] = ( 2*p3 + 3*p2 + p1 + p0 + q0 + 4 ) >> 3;
                        }
                        else
                        {
                            /* p0' */
                            pix[-1] = ( 2*p1 + p0 + q1 + 2 ) >> 2;
                        }
                        if( abs( q2 - q0 ) < beta )
                        {
                            const int q3 = pix[3];
                            /* q0', q1', q2' */
                            pix[0] = ( p1 + 2*p0 + 2*q0 + 2*q1 + q2 + 4 ) >> 3;
                            pix[1] = ( p0 + q0 + q1 + q2 + 2 ) >> 2;
                            pix[2] = ( 2*q3 + 3*q2 + q1 + q0 + p0 + 4 ) >> 3;
                        }
                        else
                        {
                            /* q0' */
                            pix[0] = ( 2*q1 + q0 + p1 + 2 ) >> 2;
                        }
                    }
                    else
                    {
                        /* p0', q0' */
                        pix[-1] = ( 2*p1 + p0 + q1 + 2 ) >> 2;
                        pix[0] = ( 2*q1 + q0 + p1 + 2 ) >> 2;
                    }
                }
                pix += i_pix_stride;
            }
        }
    }
}

static inline void deblocking_filter_edgecv(SliceHeader* h, uint8_t *pix, int i_pix_stride, int bS[4], int i_QP )
{
    int i, d;
    const int i_index_a = x264_clip3( i_QP + h->sliceAlphaC0Offset, 0, 51 );
    const int alpha = i_alpha_table[i_index_a];
    const int beta  = i_beta_table[x264_clip3( i_QP + h->sliceBetaOffset, 0, 51 )];

    for( i = 0; i < 4; i++ )
    {
        if( bS[i] == 0 )
        {
            pix += 2 * i_pix_stride;
            continue;
        }

        if( bS[i] < 4 )
        {
            const int tc = i_tc0_table[i_index_a][bS[i] - 1] + 1;
            /* 2px edge length (because we use same bS than the one for luma) */
            for( d = 0; d < 2; d++ )
            {
                const int p0 = pix[-1];
                const int p1 = pix[-2];
                const int q0 = pix[0];
                const int q1 = pix[1];

                if( abs( p0 - q0 ) < alpha &&
                    abs( p1 - p0 ) < beta &&
                    abs( q1 - q0 ) < beta )
                {
                    const int i_delta = x264_clip3( (((q0 - p0 ) << 2) + (p1 - q1) + 4) >> 3, -tc, tc );

                    pix[-1] = clip_uint8( p0 + i_delta );    /* p0' */
                    pix[0]  = clip_uint8( q0 - i_delta );    /* q0' */
                }
                pix += i_pix_stride;
            }
        }
        else
        {
            /* 2px edge length (because we use same bS than the one for luma) */
            for( d = 0; d < 2; d++ )
            {
                const int p0 = pix[-1];
                const int p1 = pix[-2];
                const int q0 = pix[0];
                const int q1 = pix[1];

                if( abs( p0 - q0 ) < alpha &&
                    abs( p1 - p0 ) < beta &&
                    abs( q1 - q0 ) < beta )
                {
                    pix[-1] = ( 2*p1 + p0 + q1 + 2 ) >> 2;   /* p0' */
                    pix[0]  = ( 2*q1 + q0 + p1 + 2 ) >> 2;   /* q0' */
                }
                pix += i_pix_stride;
            }
        }
    }
}

static inline void deblocking_filter_edgeh( SliceHeader* h, uint8_t *pix, int i_pix_stride, int bS[4], int i_QP )
{
    int i, d;
    const int i_index_a = x264_clip3( i_QP + h->sliceAlphaC0Offset, 0, 51 );
    const int alpha = i_alpha_table[i_index_a];
    const int beta  = i_beta_table[x264_clip3( i_QP + h->sliceBetaOffset, 0, 51 )];

    int i_pix_next  = i_pix_stride;

    for( i = 0; i < 4; i++ )
    {
        if( bS[i] == 0 )
        {
            pix += 4;
            continue;
        }

        if( bS[i] < 4 )
        {
            const int tc0 = i_tc0_table[i_index_a][bS[i] - 1];
            /* 4px edge length */
            for( d = 0; d < 4; d++ )
            {
                const int p0 = pix[-i_pix_next];
                const int p1 = pix[-2*i_pix_next];
                const int p2 = pix[-3*i_pix_next];
                const int q0 = pix[0];
                const int q1 = pix[1*i_pix_next];
                const int q2 = pix[2*i_pix_next];

                if( abs( p0 - q0 ) < alpha &&
                    abs( p1 - p0 ) < beta &&
                    abs( q1 - q0 ) < beta )
                {
                    int tc = tc0;
                    int i_delta;

                    if( abs( p2 - p0 ) < beta )
                    {
                        pix[-2*i_pix_next] = p1 + x264_clip3( ( p2 + ( ( p0 + q0 + 1 ) >> 1 ) - ( p1 << 1 ) ) >> 1, -tc0, tc0 );
                        tc++;
                    }
                    if( abs( q2 - q0 ) < beta )
                    {
                        pix[i_pix_next] = q1 + x264_clip3( ( q2 + ( ( p0 + q0 + 1 ) >> 1 ) - ( q1 << 1 ) ) >> 1, -tc0, tc0 );
                        tc++;
                    }

                    i_delta = x264_clip3( (((q0 - p0 ) << 2) + (p1 - q1) + 4) >> 3, -tc, tc );
                    pix[-i_pix_next] = clip_uint8( p0 + i_delta );    /* p0' */
                    pix[0]           = clip_uint8( q0 - i_delta );    /* q0' */
                }
                pix++;
            }
        }
        else
        {
            /* 4px edge length */
            for( d = 0; d < 4; d++ )
            {
                const int p0 = pix[-i_pix_next];
                const int p1 = pix[-2*i_pix_next];
                const int p2 = pix[-3*i_pix_next];
                const int q0 = pix[0];
                const int q1 = pix[1*i_pix_next];
                const int q2 = pix[2*i_pix_next];

                if( abs( p0 - q0 ) < alpha &&
                    abs( p1 - p0 ) < beta &&
                    abs( q1 - q0 ) < beta )
                {
                    const int p3 = pix[-4*i_pix_next];
                    const int q3 = pix[ 3*i_pix_next];

                    if( abs( p0 - q0 ) < (( alpha >> 2 ) + 2 ) )
                    {
                        if( abs( p2 - p0 ) < beta )
                        {
                            /* p0', p1', p2' */
                            pix[-1*i_pix_next] = ( p2 + 2*p1 + 2*p0 + 2*q0 + q1 + 4 ) >> 3;
                            pix[-2*i_pix_next] = ( p2 + p1 + p0 + q0 + 2 ) >> 2;
                            pix[-3*i_pix_next] = ( 2*p3 + 3*p2 + p1 + p0 + q0 + 4 ) >> 3;
                        }
                        else
                        {
                            /* p0' */
                            pix[-1*i_pix_next] = ( 2*p1 + p0 + q1 + 2 ) >> 2;
                        }
                        if( abs( q2 - q0 ) < beta )
                        {
                            /* q0', q1', q2' */
                            pix[0*i_pix_next] = ( p1 + 2*p0 + 2*q0 + 2*q1 + q2 + 4 ) >> 3;
                            pix[1*i_pix_next] = ( p0 + q0 + q1 + q2 + 2 ) >> 2;
                            pix[2*i_pix_next] = ( 2*q3 + 3*q2 + q1 + q0 + p0 + 4 ) >> 3;
                        }
                        else
                        {
                            /* q0' */
                            pix[0*i_pix_next] = ( 2*q1 + q0 + p1 + 2 ) >> 2;
                        }
                    }
                    else
                    {
                        /* p0' */
                        pix[-1*i_pix_next] = ( 2*p1 + p0 + q1 + 2 ) >> 2;
                        /* q0' */
                        pix[0*i_pix_next] = ( 2*q1 + q0 + p1 + 2 ) >> 2;
                    }
                }
                pix++;
            }

        }
    }
}

static inline void deblocking_filter_edgech( SliceHeader* h, uint8_t *pix, int i_pix_stride, int bS[4], int i_QP )
{
    int i, d;
    const int i_index_a = x264_clip3( i_QP + h->sliceAlphaC0Offset, 0, 51 );
    const int alpha = i_alpha_table[i_index_a];
    const int beta  = i_beta_table[x264_clip3( i_QP + h->sliceBetaOffset, 0, 51 )];

    int i_pix_next  = i_pix_stride;

    for( i = 0; i < 4; i++ )
    {
        if( bS[i] == 0 )
        {
            pix += 2;
            continue;
        }
        if( bS[i] < 4 )
        {
            int tc = i_tc0_table[i_index_a][bS[i] - 1] + 1;
            /* 2px edge length (see deblocking_filter_edgecv) */
            for( d = 0; d < 2; d++ )
            {
                const int p0 = pix[-1*i_pix_next];
                const int p1 = pix[-2*i_pix_next];
                const int q0 = pix[0];
                const int q1 = pix[1*i_pix_next];

                if( abs( p0 - q0 ) < alpha &&
                    abs( p1 - p0 ) < beta &&
                    abs( q1 - q0 ) < beta )
                {
                    int i_delta = x264_clip3( (((q0 - p0 ) << 2) + (p1 - q1) + 4) >> 3, -tc, tc );

                    pix[-i_pix_next] = clip_uint8( p0 + i_delta );    /* p0' */
                    pix[0]           = clip_uint8( q0 - i_delta );    /* q0' */
                }
                pix++;
            }
        }
        else
        {
            /* 2px edge length (see deblocking_filter_edgecv) */
            for( d = 0; d < 2; d++ )
            {
                const int p0 = pix[-1*i_pix_next];
                const int p1 = pix[-2*i_pix_next];
                const int q0 = pix[0];
                const int q1 = pix[1*i_pix_next];

                if( abs( p0 - q0 ) < alpha &&
                    abs( p1 - p0 ) < beta &&
                    abs( q1 - q0 ) < beta )
                {
                    pix[-i_pix_next] = ( 2*p1 + p0 + q1 + 2 ) >> 2;   /* p0' */
                    pix[0]           = ( 2*q1 + q0 + p1 + 2 ) >> 2;   /* q0' */
                }
                pix++;
            }
        }
    }
}

/* FIXME theses tables are duplicated with the ones in macroblock.c */
static const uint8_t block_idx_xy[4][4] =
{
    { 0, 2, 8,  10},
    { 1, 3, 9,  11},
    { 4, 6, 12, 14},
    { 5, 7, 13, 15}
};
static const int i_chroma_qp_table[52] =
{
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
    10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
    29, 30, 31, 32, 32, 33, 34, 34, 35, 35,
    36, 36, 37, 37, 37, 38, 38, 38, 39, 39,
    39, 39
};

#define IS_INTRA(type) ( (type) == I_4x4 || (type) == I_16x16 )
void x264_frame_deblocking_filter( FramePtr frame, int i_mb_stride, int i_mb_height, int i_chroma_qp_index_offset)
{
    const int s8x8 = 2 * i_mb_stride;
    const int s4x4 = 4 * i_mb_stride;

    int mb_y, mb_x;
    int loop_count = 0;
    int sum = 0;

    for( mb_y = 0, mb_x = 0; mb_y < i_mb_height; )
    {
        const int mb_xy  = mb_y * i_mb_stride + mb_x;
        const int mb_8x8 = 2 * s8x8 * mb_y + 2 * mb_x;
        const int mb_4x4 = 4 * s4x4 * mb_y + 4 * mb_x;
        int i_edge;
        int i_dir;
        printf("loop_count: %d\n", loop_count++);

        /* i_dir == 0 -> vertical edge
         * i_dir == 1 -> horizontal edge */
        for( i_dir = 0; i_dir < 2; i_dir++ )
        {
            int i_start;
            int i_qp, i_qpn;

            i_start = (( i_dir == 0 && mb_x != 0 ) || ( i_dir == 1 && mb_y != 0 ) ) ? 0 : 1;

            for( i_edge = i_start; i_edge < 4; i_edge++ )
            {
                int mbn_xy  = i_edge > 0 ? mb_xy  : ( i_dir == 0 ? mb_xy  - 1 : mb_xy - i_mb_stride );
                int mbn_8x8 = i_edge > 0 ? mb_8x8 : ( i_dir == 0 ? mb_8x8 - 2 : mb_8x8 - 2 * s8x8 );
                int mbn_4x4 = i_edge > 0 ? mb_4x4 : ( i_dir == 0 ? mb_4x4 - 4 : mb_4x4 - 4 * s4x4 );

                int bS[4];  /* filtering strength */

                MacroBlockPtr mb = frame->mbList[mb_xy];
                MacroBlockPtr mbn = frame->mbList[mbn_xy];
                /* *** Get bS for each 4px for the current edge *** */
                if( IS_INTRA( mb->type) || IS_INTRA( mbn->type ) )
                {
                    bS[0] = bS[1] = bS[2] = bS[3] = ( i_edge == 0 ? 4 : 3 );
                }
                else
                {
                    int i;
                    for( i = 0; i < 4; i++ )
                    {
                        int x  = i_dir == 0 ? i_edge : i;
                        int y  = i_dir == 0 ? i      : i_edge;
                        int xn = (x - (i_dir == 0 ? 1 : 0 ))&0x03;
                        int yn = (y - (i_dir == 0 ? 0 : 1 ))&0x03;

                        if( mb->nonZeroCount[x264_scan8[0]+x+y*8] != 0 ||
                            mbn->nonZeroCount[x264_scan8[0]+xn+yn*8] != 0 )
                        {
                            bS[i] = 2;
                        }
                        else if( frame->header.sliceType % 5 == SliceTypeP)
                        {
                            if( mb->ref[0][x264_scan8[0]+x+y*8] != mbn->ref[0][x264_scan8[0]+xn+yn*8] ||
                                abs( mb->mv[0][x264_scan8[0]+x+y*8][0] - mbn->mv[0][x264_scan8[0]+xn+yn*8][0] ) >= 4 ||
                                abs( mb->mv[0][x264_scan8[0]+x+y*8][1] - mbn->mv[0][x264_scan8[0]+xn+yn*8][1] ) >= 4 )
                            {
                                bS[i] = 1;
                            }
                            else
                            {
                                bS[i] = 0;
                            }
                        }
                        else
                        {
                            /* FIXME */
                            printf("deblocking filter doesn't work yet with B slice\n" );
                            return;
                        }
                    }
                }
                printf("bsbs %d %d %d %d\n", bS[0], bS[1], bS[2], bS[3]);

                /* *** filter *** */
                /* Y plane */
                i_qp = frame->mbList[mb_xy]->qp;
                i_qpn= frame->mbList[mbn_xy]->qp;
                sum+= bS[0]+bS[1]+bS[2]+bS[3]+i_qp+i_qpn+i_dir+i_edge+i_chroma_qp_index_offset;
                printf("qp %d qpn %d dir %d edge %d offset %d sum %d\n", i_qp, i_qpn, i_dir, i_edge, i_chroma_qp_index_offset, sum);

                if( i_dir == 0 )
                {
                    /* vertical edge */
                    deblocking_filter_edgev( &frame->header, &frame->plane[0][16 * mb_y * frame->stride[0]+ 16 * mb_x + 4 * i_edge],
                                                frame->stride[0], bS, (i_qp+i_qpn+1) >> 1);
                    if( (i_edge % 2) == 0  )
                    {
                        /* U/V planes */
                        int i_qpc = ( i_chroma_qp_table[x264_clip3( i_qp + i_chroma_qp_index_offset, 0, 51 )] +
                                      i_chroma_qp_table[x264_clip3( i_qpn + i_chroma_qp_index_offset, 0, 51 )] + 1 ) >> 1;
                        deblocking_filter_edgecv( &frame->header, &frame->plane[1][8*(mb_y*frame->stride[1]+mb_x)+i_edge*2],
                                                      frame->stride[1], bS, i_qpc );
                        deblocking_filter_edgecv( &frame->header, &frame->plane[2][8*(mb_y*frame->stride[2]+mb_x)+i_edge*2],
                                                  frame->stride[2], bS, i_qpc );
                    }
                }
                else
                {
                    /* horizontal edge */
                    deblocking_filter_edgeh( &frame->header, &frame->plane[0][(16*mb_y + 4 * i_edge) * frame->stride[0]+ 16 * mb_x],
                                                frame->stride[0], bS, (i_qp+i_qpn+1) >> 1 );
                    /* U/V planes */
                    if( ( i_edge % 2  ) == 0 )
                    {
                        int i_qpc = ( i_chroma_qp_table[x264_clip3( i_qp + i_chroma_qp_index_offset, 0, 51 )] +
                                      i_chroma_qp_table[x264_clip3( i_qpn + i_chroma_qp_index_offset, 0, 51 )] + 1 ) >> 1;
                        deblocking_filter_edgech( &frame->header, &frame->plane[1][8*(mb_y*frame->stride[1]+mb_x)+i_edge*2*frame->stride[1]],
                                                 frame->stride[1], bS, i_qpc );
                        deblocking_filter_edgech( &frame->header, &frame->plane[2][8*(mb_y*frame->stride[2]+mb_x)+i_edge*2*frame->stride[2]],
                                                 frame->stride[2], bS, i_qpc );
                    }
                }
            }
        }

        /* newt mb */
        mb_x++;
        if( mb_x >= i_mb_stride)
        {
            mb_x = 0;
            mb_y++;
        }
    }
}

static inline int x264_median( int a, int b, int c )
{
    int min = a, max =a;
    if( b < min )
        min = b;
    else
        max = b;    /* no need to do 'b > max' (more consuming than always doing affectation) */

    if( c < min )
        min = c;
    else if( c > max )
        max = c;

    return a + b + c - min - max;
}

void x264_mb_predict_mv(MacroBlockPtr mb, int i_list, int idx, int i_width, int mvp[2] )
{
    const int i8 = x264_scan8[idx];
    const int i_ref= mb->ref[i_list][i8];
    int     i_refa = mb->ref[i_list][i8 - 1];
    int16_t *mv_a  = mb->mv[i_list][i8 - 1];
    int     i_refb = mb->ref[i_list][i8 - 8];
    int16_t *mv_b  = mb->mv[i_list][i8 - 8];
    int     i_refc = mb->ref[i_list][i8 - 8 + i_width ];
    int16_t *mv_c  = mb->mv[i_list][i8 - 8 + i_width];

    int i_count;

    if( (idx&0x03) == 3 || ( i_width == 2 && (idx&0x3) == 2 )|| i_refc == -2 )
    {
        i_refc = mb->ref[i_list][i8 - 8 - 1];
        mv_c   = mb->mv[i_list][i8 - 8 - 1];
    }

    if( mb->type == P_16x8 )
    {
        if( idx == 0 && i_refb == i_ref )
        {
            mvp[0] = mv_b[0];
            mvp[1] = mv_b[1];
            return;
        }
        else if( idx != 0 && i_refa == i_ref )
        {
            mvp[0] = mv_a[0];
            mvp[1] = mv_a[1];
            return;
        }
    }
    else if( mb->type == P_8x16 )
    {
        if( idx == 0 && i_refa == i_ref )
        {
            mvp[0] = mv_a[0];
            mvp[1] = mv_a[1];
            return;
        }
        else if( idx != 0 && i_refc == i_ref )
        {
            mvp[0] = mv_c[0];
            mvp[1] = mv_c[1];
            return;
        }
    }

    i_count = 0;
    if( i_refa == i_ref ) i_count++;
    if( i_refb == i_ref ) i_count++;
    if( i_refc == i_ref ) i_count++;

    if( i_count > 1 )
    {
        mvp[0] = x264_median( mv_a[0], mv_b[0], mv_c[0] );
        mvp[1] = x264_median( mv_a[1], mv_b[1], mv_c[1] );
    }
    else if( i_count == 1 )
    {
        if( i_refa == i_ref )
        {
            mvp[0] = mv_a[0];
            mvp[1] = mv_a[1];
        }
        else if( i_refb == i_ref )
        {
            mvp[0] = mv_b[0];
            mvp[1] = mv_b[1];
        }
        else
        {
            mvp[0] = mv_c[0];
            mvp[1] = mv_c[1];
        }
    }
    else if( i_refb == -2 && i_refc == -2 && i_refa != -2 )
    {
        mvp[0] = mv_a[0];
        mvp[1] = mv_a[1];
    }
    else
    {
        mvp[0] = x264_median( mv_a[0], mv_b[0], mv_c[0] );
        mvp[1] = x264_median( mv_a[1], mv_b[1], mv_c[1] );
    }
}

void x264_mb_predict_mv_16x16(MacroBlockPtr mb, int i_list, int i_ref, int mvp[2] )
{
    int     i_refa = mb->ref[i_list][X264_SCAN8_0 - 1];
    int16_t *mv_a  = mb->mv[i_list][X264_SCAN8_0 - 1];
    int     i_refb = mb->ref[i_list][X264_SCAN8_0 - 8];
    int16_t *mv_b  = mb->mv[i_list][X264_SCAN8_0 - 8];
    int     i_refc = mb->ref[i_list][X264_SCAN8_0 - 8 + 4];
    int16_t *mv_c  = mb->mv[i_list][X264_SCAN8_0 - 8 + 4];

    int i_count;

    if( i_refc == -2 )
    {
        i_refc = mb->ref[i_list][X264_SCAN8_0 - 8 - 1];
        mv_c   = mb->mv[i_list][X264_SCAN8_0 - 8 - 1];
    }

    i_count = 0;
    if( i_refa == i_ref ) i_count++;
    if( i_refb == i_ref ) i_count++;
    if( i_refc == i_ref ) i_count++;

    if( i_count > 1 )
    {
        mvp[0] = x264_median( mv_a[0], mv_b[0], mv_c[0] );
        mvp[1] = x264_median( mv_a[1], mv_b[1], mv_c[1] );
    }
    else if( i_count == 1 )
    {
        if( i_refa == i_ref )
        {
            mvp[0] = mv_a[0];
            mvp[1] = mv_a[1];
        }
        else if( i_refb == i_ref )
        {
            mvp[0] = mv_b[0];
            mvp[1] = mv_b[1];
        }
        else
        {
            mvp[0] = mv_c[0];
            mvp[1] = mv_c[1];
        }
    }
    else if( i_refb == -2 && i_refc == -2 && i_refa != -2 )
    {
        mvp[0] = mv_a[0];
        mvp[1] = mv_a[1];
    }
    else
    {
        mvp[0] = x264_median( mv_a[0], mv_b[0], mv_c[0] );
        mvp[1] = x264_median( mv_a[1], mv_b[1], mv_c[1] );
    }
}


void x264_mb_predict_mv_pskip(MacroBlockPtr mb, int mv[2] )
{
    int     i_refa = mb->ref[0][X264_SCAN8_0 - 1];
    int     i_refb = mb->ref[0][X264_SCAN8_0 - 8];
    int16_t *mv_a  = mb->mv[0][X264_SCAN8_0 - 1];
    int16_t *mv_b  = mb->mv[0][X264_SCAN8_0 - 8];

    if( i_refa == -2 || i_refb == -2 ||
        ( i_refa == 0 && mv_a[0] == 0 && mv_a[1] == 0 ) ||
        ( i_refb == 0 && mv_b[0] == 0 && mv_b[1] == 0 ) )
    {
        mv[0] = mv[1] = 0;
    }
    else
    {
        x264_mb_predict_mv_16x16(mb, 0, 0, mv );
    }
}

static inline int x264_tapfilter( uint8_t *pix, int i_pix_next )
{
    return pix[-2*i_pix_next] - 5*pix[-1*i_pix_next] + 20*(pix[0] + pix[1*i_pix_next]) - 5*pix[ 2*i_pix_next] + pix[ 3*i_pix_next];
}
static inline int x264_tapfilter1( uint8_t *pix )
{
    return pix[-2] - 5*pix[-1] + 20*(pix[0] + pix[1]) - 5*pix[ 2] + pix[ 3];
}

static inline void pixel_avg( uint8_t *dst,  int i_dst_stride,
                              uint8_t *src1, int i_src1_stride,
                              uint8_t *src2, int i_src2_stride,
                              int i_width, int i_height )
{
    int x, y;
    for( y = 0; y < i_height; y++ )
    {
        for( x = 0; x < i_width; x++ )
        {
            dst[x] = ( src1[x] + src2[x] + 1 ) >> 1;
        }
        dst  += i_dst_stride;
        src1 += i_src1_stride;
        src2 += i_src2_stride;
    }
}


/* Clip1 table
 * XXX : only for tap filter.
 *
 * With tap filter (( 1, -5, 20, 20, -5, 1 ) + 16 )/ 32
 * -> (-2*5 * 255+16)/32 <= out <= (2*1*255 + 2*20*255+16)/32
 * -> -80 <= out <= 335
 * So we need a table of 80+335+1 = 416 entries
 */

static const uint8_t x264_mc_clip1_table[80+1+335] =
{
    /* -80 -> -1 */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,
    /* 0 -> 255 */
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17,
    18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
    36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53,
    54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71,
    72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89,
    90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100,101,102,103,104,105,106,107,
    108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,
    126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
    144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,
    162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,
    180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,
    198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
    216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,
    234,235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,
    252,253,254,255,
    /* 256 -> 340 */
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,
};

static inline uint8_t x264_mc_clip1( int x )
{
    return x264_mc_clip1_table[x+80];
}

typedef void (*pf_mc_t)(uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_width, int i_height );


static void mc_copy( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_width, int i_height )
{
    int y;

    for( y = 0; y < i_height; y++ )
    {
        memcpy( dst, src, i_width );

        src += i_src_stride;
        dst += i_dst_stride;
    }
}

static inline void mc_hh( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_width, int i_height )
{
    int x, y;

    for( y = 0; y < i_height; y++ )
    {
        for( x = 0; x < i_width; x++ )
        {
            dst[x] = x264_mc_clip1( ( x264_tapfilter1( &src[x] ) + 16 ) >> 5 );
        }
        src += i_src_stride;
        dst += i_dst_stride;
    }
}
static inline void mc_hv( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_width, int i_height )
{
    int x, y;

    for( y = 0; y < i_height; y++ )
    {
        for( x = 0; x < i_width; x++ )
        {
            dst[x] = x264_mc_clip1( ( x264_tapfilter( &src[x], i_src_stride ) + 16 ) >> 5 );
        }
        src += i_src_stride;
        dst += i_dst_stride;
    }
}
static inline void mc_hc( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_width, int i_height )
{
    uint8_t *out;
    uint8_t *pix;
    int x, y;

    for( x = 0; x < i_width; x++ )
    {
        int tap[6];

        pix = &src[x];
        out = &dst[x];

        tap[0] = x264_tapfilter1( &pix[-2*i_src_stride] );
        tap[1] = x264_tapfilter1( &pix[-1*i_src_stride] );
        tap[2] = x264_tapfilter1( &pix[ 0*i_src_stride] );
        tap[3] = x264_tapfilter1( &pix[ 1*i_src_stride] );
        tap[4] = x264_tapfilter1( &pix[ 2*i_src_stride] );

        for( y = 0; y < i_height; y++ )
        {
            tap[5] = x264_tapfilter1( &pix[ 3*i_src_stride] );

            *out = x264_mc_clip1( ( tap[0] - 5*tap[1] + 20 * tap[2] + 20 * tap[3] -5*tap[4] + tap[5] + 512 ) >> 10 );

            /* Next line */
            pix += i_src_stride;
            out += i_dst_stride;
            tap[0] = tap[1];
            tap[1] = tap[2];
            tap[2] = tap[3];
            tap[3] = tap[4];
            tap[4] = tap[5];
        }
    }
}

/* mc I+H */
static void mc_xy10( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_width, int i_height )
{
    uint8_t tmp[16*16];
    mc_hh( src, i_src_stride, tmp, i_width, i_width, i_height );
    pixel_avg( dst, i_dst_stride, src, i_src_stride, tmp, i_width, i_width, i_height );
}
static void mc_xy30( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_width, int i_height )
{
    uint8_t tmp[16*16];
    mc_hh( src, i_src_stride, tmp, i_width, i_width, i_height );
    pixel_avg( dst, i_dst_stride, src+1, i_src_stride, tmp, i_width, i_width, i_height );
}
/* mc I+V */
static void mc_xy01( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_width, int i_height )
{
    uint8_t tmp[16*16];
    mc_hv( src, i_src_stride, tmp, i_width, i_width, i_height );
    pixel_avg( dst, i_dst_stride, src, i_src_stride, tmp, i_width, i_width, i_height );
}
static void mc_xy03( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_width, int i_height )
{
    uint8_t tmp[16*16];
    mc_hv( src, i_src_stride, tmp, i_width, i_width, i_height );
    pixel_avg( dst, i_dst_stride, src+i_src_stride, i_src_stride, tmp, i_width, i_width, i_height );
}
/* H+V */
static void mc_xy11( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_width, int i_height )
{
    uint8_t tmp1[16*16];
    uint8_t tmp2[16*16];

    mc_hv( src, i_src_stride, tmp1, i_width, i_width, i_height );
    mc_hh( src, i_src_stride, tmp2, i_width, i_width, i_height );
    pixel_avg( dst, i_dst_stride, tmp1, i_width, tmp2, i_width, i_width, i_height );
}
static void mc_xy31( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_width, int i_height )
{
    uint8_t tmp1[16*16];
    uint8_t tmp2[16*16];

    mc_hv( src+1, i_src_stride, tmp1, i_width, i_width, i_height );
    mc_hh( src,   i_src_stride, tmp2, i_width, i_width, i_height );
    pixel_avg( dst, i_dst_stride, tmp1, i_width, tmp2, i_width, i_width, i_height );
}
static void mc_xy13( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_width, int i_height )
{
    uint8_t tmp1[16*16];
    uint8_t tmp2[16*16];

    mc_hv( src,              i_src_stride, tmp1, i_width, i_width, i_height );
    mc_hh( src+i_src_stride, i_src_stride, tmp2, i_width, i_width, i_height );
    pixel_avg( dst, i_dst_stride, tmp1, i_width, tmp2, i_width, i_width, i_height );
}
static void mc_xy33( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_width, int i_height )
{
    uint8_t tmp1[16*16];
    uint8_t tmp2[16*16];

    mc_hv( src+1,            i_src_stride, tmp1, i_width, i_width, i_height );
    mc_hh( src+i_src_stride, i_src_stride, tmp2, i_width, i_width, i_height );
    pixel_avg( dst, i_dst_stride, tmp1, i_width, tmp2, i_width, i_width, i_height );
}
static void mc_xy21( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_width, int i_height )
{
    uint8_t tmp1[16*16];
    uint8_t tmp2[16*16];

    mc_hc( src, i_src_stride, tmp1, i_width, i_width, i_height );
    mc_hh( src, i_src_stride, tmp2, i_width, i_width, i_height );
    pixel_avg( dst, i_dst_stride, tmp1, i_width, tmp2, i_width, i_width, i_height );
}
static void mc_xy12( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_width, int i_height )
{
    uint8_t tmp1[16*16];
    uint8_t tmp2[16*16];

    mc_hc( src, i_src_stride, tmp1, i_width, i_width, i_height );
    mc_hv( src, i_src_stride, tmp2, i_width, i_width, i_height );
    pixel_avg( dst, i_dst_stride, tmp1, i_width, tmp2, i_width, i_width, i_height );
}
static void mc_xy32( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_width, int i_height )
{
    uint8_t tmp1[16*16];
    uint8_t tmp2[16*16];

    mc_hc( src,   i_src_stride, tmp1, i_width, i_width, i_height );
    mc_hv( src+1, i_src_stride, tmp2, i_width, i_width, i_height );
    pixel_avg( dst, i_dst_stride, tmp1, i_width, tmp2, i_width, i_width, i_height );
}
static void mc_xy23( uint8_t *src, int i_src_stride, uint8_t *dst, int i_dst_stride, int i_width, int i_height )
{
    uint8_t tmp1[16*16];
    uint8_t tmp2[16*16];

    mc_hc( src,              i_src_stride, tmp1, i_width, i_width, i_height );
    mc_hh( src+i_src_stride, i_src_stride, tmp2, i_width, i_width, i_height );
    pixel_avg( dst, i_dst_stride, tmp1, i_width, tmp2, i_width, i_width, i_height );
}

static void motion_compensation_luma( uint8_t *src, int i_src_stride,
                                      uint8_t *dst, int i_dst_stride,
                                      int mvx,int mvy,
                                      int i_width, int i_height )
{
    static pf_mc_t pf_mc[4][4] =    /*XXX [dqy][dqx] */
    {
        { mc_copy,  mc_xy10,    mc_hh,      mc_xy30 },
        { mc_xy01,  mc_xy11,    mc_xy21,    mc_xy31 },
        { mc_hv,    mc_xy12,    mc_hc,      mc_xy32 },
        { mc_xy03,  mc_xy13,    mc_xy23,    mc_xy33 },
    };

    src += (mvy >> 2) * i_src_stride + (mvx >> 2);
    pf_mc[mvy&0x03][mvx&0x03]( src, i_src_stride, dst, i_dst_stride, i_width, i_height );

    std::cout<<"mc luma"<<std::endl;
    for(int h=0;h<i_height;h++){
        uint8_t* p = dst+h*i_dst_stride;
        for(int w=0;w<i_width;w++){
            std::cout << (int)p[w] << " ";
        }
        std::cout << std::endl;
    }
}

/* full chroma mc (ie until 1/8 pixel)*/
static void motion_compensation_chroma( uint8_t *src, int i_src_stride,
                                        uint8_t *dst, int i_dst_stride,
                                        int mvx, int mvy,
                                        int i_width, int i_height )
{
    uint8_t *srcp;
    int x, y;
    uint8_t *dst_orig = dst;

    const int d8x = mvx&0x07;
    const int d8y = mvy&0x07;

    const int cA = (8-d8x)*(8-d8y);
    const int cB = d8x    *(8-d8y);
    const int cC = (8-d8x)*d8y;
    const int cD = d8x    *d8y;

    src  += (mvy >> 3) * i_src_stride + (mvx >> 3);
    srcp = &src[i_src_stride];

    for( y = 0; y < i_height; y++ )
    {
        for( x = 0; x < i_width; x++ )
        {
            dst[x] = ( cA*src[x]  + cB*src[x+1] +
                       cC*srcp[x] + cD*srcp[x+1] + 32 ) >> 6;
        }
        dst  += i_dst_stride;

        src   = srcp;
        srcp += i_src_stride;
    }

    std::cout<<"mc chroma"<<std::endl;
    for(int h=0;h<i_height;h++){
        uint8_t* p = dst_orig+h*i_dst_stride;
        for(int w=0;w<i_width;w++){
            std::cout << (int)p[w] << " ";
        }
        std::cout << std::endl;
    }
}

void x264_macroblock_decode_pskip(MacroBlockPtr mb, uint8_t* src[3], int src_stride[3], uint8_t* dst[3], int dst_stride[3])
{
    const int mvx = x264_clip3( mb->mv[0][x264_scan8[0]][0],
                                mb->mv_min[0], mb->mv_max[0] );
    const int mvy = x264_clip3( mb->mv[0][x264_scan8[0]][1],
                                mb->mv_min[1], mb->mv_max[1] );

    std::cout << "mvx " << mvx << ", mvy " << mvy << std::endl;
    /* Motion compensation XXX probably unneeded */
    motion_compensation_luma( src[0], src_stride[0],
                    dst[0],       dst_stride[0],
                    mvx, mvy, 16, 16 );

    /* Chroma MC */
    motion_compensation_chroma( src[1], src_stride[1],
                      dst[1],       dst_stride[1],
                      mvx, mvy, 8, 8 );

    motion_compensation_chroma( src[2], src_stride[2],
                      dst[2],       dst_stride[2],
                      mvx, mvy, 8, 8 );

    int i;
    mb->cbpLuma= 0x00;
    mb->cbpChroma = 0x00;

    for( i = 0; i < 16+8; i++ )
    {
        mb->nonZeroCount[x264_scan8[i]] = 0;
    }

    /* store cbp */
    // h->mb.cbp[h->mb.i_mb_xy] = 0;

}

static inline void x264_mb_mc_0xywh(MacroBlockPtr mb, uint8_t* src[3], int src_stride[3], 
        uint8_t* dst[3], int dst_stride[3], int x, int y, int width, int height )
{
    const int i8 = x264_scan8[0]+x+8*y;
    const int i_ref = mb->ref[0][i8];
    assert(i_ref == 0);
    const int mvx   = x264_clip3( mb->mv[0][i8][0], mb->mv_min[0], mb->mv_max[0] );
    const int mvy   = x264_clip3( mb->mv[0][i8][1], mb->mv_min[1], mb->mv_max[1] );
    std::cout << "mvx " << mvx << " mvy " << mvy << std::endl;

    motion_compensation_luma( &src[0][4*y * src_stride[0]+4*x], src_stride[0],
                    &dst[0][4*y * dst_stride[0]+4*x],           dst_stride[0],
                    mvx, mvy, 4*width, 4*height );

    motion_compensation_chroma( &src[1][2*y*src_stride[1]+2*x], src_stride[1],
                      &dst[1][2*y*dst_stride[1]+2*x],           dst_stride[1],
                      mvx, mvy, 2*width, 2*height );

    motion_compensation_chroma( &src[2][2*y*src_stride[2]+2*x], src_stride[2],
                      &dst[2][2*y*dst_stride[2]+2*x],           dst_stride[2],
                      mvx, mvy, 2*width, 2*height );
}

void x264_mb_mc(MacroBlockPtr mb, uint8_t* src[3], int src_stride[3], uint8_t* dst[3], int dst_stride[3])
{
    if( mb->type == P_L0 )
    {
        x264_mb_mc_0xywh( mb, src, src_stride, dst, dst_stride, 0, 0, 4, 4 );
    } else if (mb->type == P_16x8) {
        x264_mb_mc_0xywh( mb, src, src_stride, dst, dst_stride, 0, 0, 4, 2 );
        x264_mb_mc_0xywh( mb, src, src_stride, dst, dst_stride, 0, 2, 4, 2 );
    } else if (mb->type == P_8x16) {
        x264_mb_mc_0xywh( mb, src, src_stride, dst, dst_stride, 0, 0, 2, 4 );
        x264_mb_mc_0xywh( mb, src, src_stride, dst, dst_stride, 2, 0, 2, 4 );
    }
    else if( mb->type == P_8x8)
    {
        int i;
        for( i = 0; i < 4; i++ )
        {
            const int x = 2*(i%2);
            const int y = 2*(i/2);
            switch( mb->subPartition[i] )
            {
                case D_L0_8x8:
                    x264_mb_mc_0xywh( mb, src, src_stride, dst, dst_stride, x,y,2,2);
                    break;
                case D_L0_8x4:
                    x264_mb_mc_0xywh( mb, src, src_stride, dst, dst_stride, x,y+0,2,1);
                    x264_mb_mc_0xywh( mb, src, src_stride, dst, dst_stride, x,y+1,2,1);
                    break;
                case D_L0_4x8:
                    x264_mb_mc_0xywh( mb, src, src_stride, dst, dst_stride, x+0,y,1,2);
                    x264_mb_mc_0xywh( mb, src, src_stride, dst, dst_stride, x+1,y,1,2);
                    break;
                case D_L0_4x4:
                    x264_mb_mc_0xywh( mb, src, src_stride, dst, dst_stride, x+0,y+0,1,1);
                    x264_mb_mc_0xywh( mb, src, src_stride, dst, dst_stride, x+1,y+0,1,1);
                    x264_mb_mc_0xywh( mb, src, src_stride, dst, dst_stride, x+0,y+1,1,1);
                    x264_mb_mc_0xywh( mb, src, src_stride, dst, dst_stride, x+1,y+1,1,1);
                    break;
            }
        }
    }
}

void x264_mb_decode_p16x16(int qscale, uint8_t* dst, int stride, int* luma4x4[16])
{
    int16_t dct4x4[16][4][4];
    for(int i = 0; i < 16; i++) {
        zigzag_to_4x4_full(luma4x4[i], dct4x4[i]);
        x264_mb_dequant_4x4(dct4x4[i], qscale);
    }
    add16x16_idct(dst, stride, dct4x4);
    std::cout << "p16x16 luma" << std::endl;
    for(int h=0;h<16;h++){
        uint8_t* p=dst+stride*h;
        for(int w=0;w<16;w++){
            std::cout << (int)p[w] << " ";
        }
        std::cout << std::endl;
    }

}

void x264_frame_expand_border( FramePtr frame )
{
    int w;
    int i, y;
    for( i = 0; i < 3; i++ )
    {
#define PPIXEL(x, y) ( frame->plane[i] + (x) +(y)*frame->stride[i] )
        w = ( i == 0 ) ? 32 : 16;

        for( y = 0; y < w; y++ )
        {
            /* upper band */
            memcpy( PPIXEL(0,-y-1), PPIXEL(0,0), frame->stride[i] - 2 * w);
            /* up left corner */
            memset( PPIXEL(-w,-y-1 ), PPIXEL(0,0)[0], w );
            /* up right corner */
            memset( PPIXEL(frame->stride[i] - 2*w,-y-1), PPIXEL( frame->stride[i]-1-2*w,0)[0], w );

            /* lower band */
            memcpy( PPIXEL(0, frame->lines[i]+y), PPIXEL(0,frame->lines[i]-1), frame->stride[i] - 2 * w );
            /* low left corner */
            memset( PPIXEL(-w, frame->lines[i]+y), PPIXEL(0,frame->lines[i]-1)[0], w);
            /* low right corner */
            memset( PPIXEL(frame->stride[i]-2*w, frame->lines[i]+y), PPIXEL(frame->stride[i]-1-2*w,frame->lines[i]-1)[0], w);

        }
        for( y = 0; y < frame->lines[i]; y++ )
        {
            /* left band */
            memset( PPIXEL( -w, y ), PPIXEL( 0, y )[0], w );
            /* right band */
            memset( PPIXEL( frame->stride[i]-2*w, y ), PPIXEL( frame->stride[i] - 1-2*w, y )[0], w );
        }
#undef PPIXEL
    }
}
