#include "common.h"
#include <cstdint>

uint8_t ISliceMbTable[26][5] = {
    //type, 16x16 pred mode, 4x4 pred mode, cbp chroma, cbp luma
    [0] = {I_4x4, 0, 0, 0, 0},
    [1] = {I_16x16, I_PRED_16x16_V, 0, 0, 0},
    [2] = {I_16x16, I_PRED_16x16_H, 0, 0, 0},
    [3] = {I_16x16, I_PRED_16x16_DC, 0, 0, 0},
    [4] = {I_16x16, I_PRED_16x16_P, 0, 0, 0},
    [5] = {I_16x16, I_PRED_16x16_V, 0, 1, 0},
    [6] = {I_16x16, I_PRED_16x16_H, 0, 1, 0},
    [7] = {I_16x16, I_PRED_16x16_DC, 0, 1, 0},
    [8] = {I_16x16, I_PRED_16x16_P, 0, 1, 0},
    [9] = {I_16x16, I_PRED_16x16_V, 0, 2, 0},
    [10] = {I_16x16, I_PRED_16x16_H, 0, 2, 0},
    [11] = {I_16x16, I_PRED_16x16_DC, 0, 2, 0},
    [12] = {I_16x16, I_PRED_16x16_P, 0, 2, 0},
    [13] = {I_16x16, I_PRED_16x16_V, 0, 0, 15},
    [14] = {I_16x16, I_PRED_16x16_H, 0, 0, 15},
    [15] = {I_16x16, I_PRED_16x16_DC, 0, 0, 15},
    [16] = {I_16x16, I_PRED_16x16_P, 0, 0, 15},
    [17] = {I_16x16, I_PRED_16x16_V, 0, 1, 15},
    [18] = {I_16x16, I_PRED_16x16_H, 0, 1, 15},
    [19] = {I_16x16, I_PRED_16x16_DC, 0, 1, 15},
    [20] = {I_16x16, I_PRED_16x16_P, 0, 1, 15},
    [21] = {I_16x16, I_PRED_16x16_V, 0, 2, 15},
    [22] = {I_16x16, I_PRED_16x16_H, 0, 2, 15},
    [23] = {I_16x16, I_PRED_16x16_DC, 0, 2, 15},
    [24] = {I_16x16, I_PRED_16x16_P, 0, 2, 15},
    [25] = {I_PCM, 0, 0, 0, 0},
};

uint8_t PSliceMbTable[6] = {
    [0] = P_L0,
    [1] = P_16x8,
    [2] = P_8x16,
    [3] = P_8x8,
    [4] = P_8x8,
    [5] = P_SKIP
};
