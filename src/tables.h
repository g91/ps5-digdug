// Game data tables transcribed from the Atari 7800 Dig Dug assembly source.
// See python/digdug_tables.py for commentary; this is the same data.
#pragma once
#include <cstdint>
#include <vector>

namespace dd {

constexpr int DDSTRTX = 96, DDSTRTY = 191;
constexpr int DDMIDLX = 56, DDMIDLY = 107;
constexpr int MINDDX = 8, MAXDDX = 112;
constexpr int MINDDY = 23, MAXDDY = 191;
inline const int TOPZONE[17] = {11, 23, 35, 47, 59, 71, 83, 95, 107, 119, 131, 143, 155, 167, 179, 191, 191};
inline const int MAPLOW[16] = {0xEF, 0xE0, 0xD0, 0xC0, 0xB0, 0xA0, 0x90, 0x80, 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00};

constexpr int FULLDIRT = 36, FULLSKY = 30, NODIRT = 0;
constexpr int DTOPEND = 0x16, DVMIDDLE = 0x14, DBOTEND = 0x1C;
constexpr int DLEFTEND = 0x1A, DHMIDDLE = 0x0A, DRHTEND = 0x0E;
constexpr int BIGFLOWR = 32, SMLFLOWR = 34;
constexpr int BLANKR = 74, BLANKL = 76;
constexpr int LSKYHALF = 140, RSKYHALF = 136;
constexpr int CHARDF = 0x1E, CHARD12 = 0x24;

inline const int SOUTH7[7] = {36, 38, 40, 42, 28, 46, 20};
inline const int SOUTH1[7] = {44, 12, 48, 24, 44, 8, 48};
inline const int NORTH7[7] = {36, 38, 22, 42, 44, 46, 20};
inline const int NORTH1[7] = {40, 6, 40, 18, 48, 2, 48};
inline const int WEST5[7] = {36, 38, 40, 26, 44, 10, 48};
inline const int WEST1[7] = {42, 46, 18, 42, 24, 46, 16};
inline const int EAST5[7] = {36, 14, 40, 42, 44, 10, 48};
inline const int EAST1[7] = {38, 38, 6, 46, 12, 46, 4};
inline const int VERTTOP[18] = {0, 2, 4, 6, 0, 2, 4, 6, 16, 18, 20, 22, 16, 18, 20, 30, 32, 34};
inline const int VERTBOT[18] = {0, 0, 4, 4, 8, 8, 12, 12, 16, 16, 20, 20, 24, 24, 28, 30, 32, 34};
inline const int HORRGT[18] = {0, 2, 4, 6, 8, 10, 12, 14, 0, 2, 4, 6, 8, 10, 12, 30, 32, 34};
inline const int HORLFT[18] = {0, 2, 0, 2, 8, 10, 8, 10, 16, 18, 16, 18, 24, 26, 24, 30, 32, 34};

inline const int* const HALFXFM[4] = {EAST5, WEST5, SOUTH7, NORTH7};
inline const int* const FROMXFM[4] = {HORLFT, HORRGT, VERTTOP, VERTBOT};
inline const int* const TOXFM[4] = {HORRGT, HORLFT, VERTBOT, VERTTOP};
inline const int* const ENTRXFM[4] = {EAST1, WEST1, SOUTH1, NORTH1};

inline const int VONE[4] = {1, 7, 10, 0};
inline const int VBOUND[4] = {0, 0, 11, 11};
inline const int VHALF[4] = {5, 3, 4, 6};
inline const int VPRE1[4] = {7, 1, 0, 10};
inline const int VPRE2[4] = {6, 2, 1, 9};
inline const int VPRE3[4] = {6, 2, 2, 8};

inline const int XTOPL[20] = {8, 0, 0, 0, 8, -8, 0, 0, 3, -3, 0, 0, 16, -8, 0, 0, 7, -7, 0, 0};
inline const int YTOPL[20] = {0, 0, -12, 0, 0, 0, -12, 12, 12, 12, 0, 17, 0, 0, -24, 12, 12, 12, 0, 23};
inline const int XFROMPL[4] = {0, 8, 0, 0};
inline const int YFROMPL[4] = {0, 0, 0, -12};
inline const int CVTDIR[4] = {2, 1, 4, 3};

constexpr int DHFAST = 0xAAD5, DHSLOW = 0x5252;
constexpr int DIGFAST = 0xF7, DIGSLOW = 0x5B;
constexpr int PROPORT_INIT = 0xB5;

inline const int RDIGDUG[3] = {0, 2, 4};
inline const int LDIGDUG[3] = {6, 8, 10};
inline const int DANIMS[12] = {0, 6, 154, 148, 2, 8, 156, 150, 4, 10, 158, 152};
constexpr int SQDIGDUG = 160, SDEATH = 106;
inline const int ADEATH[10] = {222, 106, 112, 110, 108, 222, 106, 118, 116, 114};
inline const int DANIMIX[4] = {4, 9, 9, 4};
inline const int HARPSTMP[4] = {16, 18, 122, 120};
inline const int HARPXPL[4] = {2, -2, 0, 0};
inline const int HARPYPL[4] = {0, 0, -4, 4};
constexpr int ZBLANK = 222;

inline const int RPUMP[6] = {53, 52, 51, 50, 49, 48};
inline const int LPUMP[6] = {69, 67, 64, 60, 55, 46};
constexpr int DPUMP6 = 172, DPUMP12 = 173, DFULL6 = 174, DFULL12 = 175;
constexpr int UPUMP6 = 176, UPUMP12 = 177, UFULL6 = 104, UFULL12 = 105;
inline const int PDOWNS0[6] = {DPUMP6, DPUMP12, DFULL6, DFULL12, DFULL6, DFULL12};
inline const int PDOWNS1[6] = {ZBLANK, ZBLANK, DPUMP6, DPUMP12, DFULL6, DFULL12};
inline const int PDOWNS2[6] = {ZBLANK, ZBLANK, ZBLANK, ZBLANK, DPUMP6, DPUMP12};
inline const int PUPS0[6] = {UPUMP6, UPUMP12, UPUMP12, UPUMP12, UPUMP12, UPUMP12};
inline const int PUPS1[6] = {ZBLANK, ZBLANK, UFULL6, UFULL12, UFULL12, UFULL12};
inline const int PUPS2[6] = {ZBLANK, ZBLANK, ZBLANK, ZBLANK, UFULL6, UFULL12};

inline const int XTABLE[16] = {0, 0, -1, -1, 0, 0, -1, -1, 1, 1, 0, 0, 1, 1, 0, 0};
inline const int YTABLE[16] = {0, -1, 0, -1, 1, 0, 1, 0, 0, -1, 0, -1, 1, 0, 1, 0};
inline const int SPEEDTAB[8] = {0xEEEE, 0x9248, 0x7FFF, 0x954A, 0xF777, 0x8924, 0xFFFF, 0xA954};

inline const std::vector<int> RPOOKA_SEQ = {126, 126, 124, 124, 124, 124, 124, 124};
inline const std::vector<int> LPOOKA_SEQ = {130, 130, 128, 128, 128, 128, 128, 128};
inline const std::vector<int> GPOOKA_SEQ = {140, 142, 142, 142};
inline const std::vector<int> RFYGAR_SEQ = {132, 132, 134, 134, 132, 132, 134, 134};
inline const std::vector<int> LFYGAR_SEQ = {136, 136, 138, 138, 136, 136, 138, 138};
inline const std::vector<int> GFYGAR_SEQ = {144, 146, 146, 146};
inline const std::vector<int>* const SEQTAB[8] = {&GPOOKA_SEQ, &LPOOKA_SEQ, &GFYGAR_SEQ, &LFYGAR_SEQ,
                                                  &RPOOKA_SEQ, &GPOOKA_SEQ, &RFYGAR_SEQ, &GFYGAR_SEQ};
constexpr int SQPOOKA = 164, SQFYGAR = 162;
inline const int FLAMTABL[8] = {134, 138, 12, 14, 134, 138, 12, 14};
inline const int RFLAMSEQ[4] = {20, 22, 24, 28};
inline const int LFLAMSEQ[4] = {21, 34, 36, 40};
inline const int FLAMPOS[4] = {4, 8, 16, 24};
inline const int BLWSTMPS[16] = {232, 234, 236, 238, 224, 226, 228, 230, 248, 250, 252, 254, 240, 242, 244, 246};
inline const int BPOINTS[8] = {184, 98, 181, 178, 218, 187, 101, 98};
inline const int BURSTPTS[8] = {500, 400, 300, 200, 1000, 800, 600, 400};
enum Mode { MODE_DIAG = 0, MODE_VERT, MODE_HORIZ, MODE_ESCAPE, MODE_OFFSCR };
inline const int MODETAB[8] = {MODE_DIAG, MODE_VERT, MODE_HORIZ, MODE_VERT, MODE_HORIZ, MODE_DIAG, MODE_VERT, MODE_HORIZ};
inline const int WHNSPEED[5] = {2, 3, 4, 5, 6};
inline const int CRYOFST[8] = {0, 12, 24, 12, 0, 12, 24, 12};
inline const int CRXOFST[8] = {8, 16, 8, 0, 8, 16, 8, 0};

constexpr int STRTROCK = 166, SPLTROCK = 168, CRMBROCK = 170, TOTRROCK = 86;
inline const int RFALLTBL[7] = {1, 2, 2, 2, 1, 2, 2};
inline const int RKANIM[2] = {SPLTROCK, CRMBROCK};
inline const int RKTIM[3] = {16, 16, 30};
inline const int DISPTS[8] = {218, 190, 194, 198, 202, 206, 210, 214};
inline const int FLATPTS[8] = {1000, 2500, 4000, 6000, 8000, 10000, 12000, 15000};
constexpr int ONEDGE = 2;

constexpr int TEDDY = 50;
inline const int FRUITAB[19] = {50, 52, 54, 56, 58, 58, 60, 60, 62, 62, 64, 64, 66, 66, 68, 68, 70, 70, 72};
inline const int FPOINTS[19] = {178, 98, 101, 187, 218, 218, 70, 70, 74, 74, 194, 194, 78, 78, 198, 198, 82, 82, 202};
inline const int VEGPTS[19] = {200, 400, 600, 800, 1000, 1000, 2000, 2000, 3000, 3000, 4000, 4000,
                               5000, 5000, 6000, 6000, 7000, 7000, 8000};
constexpr int NUMVEG = 17;
inline const uint8_t VEGCOL1[19] = {0xD2, 0x1E, 0x3F, 0xF4, 0xF4, 0x50, 0x50, 0xD4, 0xD4, 0x32, 0x32, 0x26, 0x26, 0x34, 0x34, 0x74, 0x74, 0xD2, 0x16};
inline const uint8_t VEGCOL2[19] = {0x34, 0xD6, 0x22, 0xD6, 0xD6, 0x52, 0x52, 0x3A, 0x3A, 0xD2, 0xD2, 0x1E, 0x1E, 0xD8, 0xD8, 0x1A, 0x1A, 0x1A, 0x1A};
inline const uint8_t VEGCOL3[19] = {0x38, 0xEE, 0x04, 0xFC, 0xFC, 0x58, 0x58, 0xE8, 0xE8, 0x36, 0x36, 0x16, 0x16, 0x1A, 0x1A, 0x54, 0x54, 0x24, 0x14};

inline const uint8_t DIRTCOLR[12] = {0x1A, 0x27, 0x24, 0x10, 0x0A, 0x15, 0x1E, 0x27, 0x0A, 0x15, 0xB2, 0x04};
inline const uint8_t PEBBCOLR[12] = {0x24, 0x14, 0x17, 0x26, 0x52, 0x42, 0x24, 0x10, 0x52, 0x42, 0x00, 0x52};
constexpr uint8_t WHITE = 0x0E, RED = 0x37, SKYBLUE = 0x86, DDBLUE = 0x85, YELLOW = 0x1A, BLACK = 0x00;
inline const uint8_t IPALETTE[8][4] = {{0x00, 0x85, 0x0E, 0x00}, {0x00, 0x34, 0x1A, 0x0E},
                                       {0x00, 0x30, 0x00, 0x44}, {0x00, 0xD6, 0x24, 0x0E},
                                       {0x00, 0x1A, 0x85, 0xA0}, {0x00, 0x86, 0x86, 0x0E},
                                       {0x00, 0x1A, 0x85, 0x24}, {0x00, 0xD4, 0x86, 0x34}};

inline const std::vector<std::vector<int>> PTUNNELS = {
    {0x2A, 0xA3, 0x22, 0x9A},
    {0x97, 0xBB, 0x3B, 0x62, 0x92},
    {0x2B, 0x52, 0xC9, 0x38, 0x96},
    {0x34, 0xA4, 0xC8, 0x38, 0x7C},
    {0x32, 0x93, 0xAB, 0x36, 0x2A, 0x6B},
    {0x34, 0x34, 0x8A, 0x92, 0xB6, 0x53},
    {0x3A, 0x3A, 0x93, 0x93, 0xB9, 0x26, 0x26},
    {0x32, 0x4A, 0xAA, 0xC5, 0x36, 0x36, 0x83},
    {0x59, 0x59, 0x96, 0xB2, 0xB2, 0xBB, 0x35, 0x5D},
    {0x48, 0x48, 0xA4, 0xBA, 0x2C, 0x2C, 0x54, 0x6A},
    {0x44, 0x44, 0x62, 0x62, 0x4A, 0x4A, 0x6D, 0x93},
    {0x43, 0x43, 0x58, 0x58, 0xAA, 0xAA, 0xB5, 0x4D},
    {0x3A, 0x3A, 0xA6, 0xA6, 0xCB, 0xCB, 0x6C, 0x54},
    {0x38, 0x38, 0x8A, 0xA4, 0xA4, 0x34, 0x34, 0x4D},
    {0x2B, 0x6A, 0x72, 0x72, 0x36, 0x36, 0x96, 0x98},
    {0x3B, 0xA2},
    {0x23, 0xA7},
    {0x6B, 0x42, 0xBA},
    {0x23, 0x93, 0x3A},
    {0x6B, 0x42, 0xA7},
    {0xB2, 0xBA, 0x23, 0x3A},
    {0x23, 0xBA},
    {0xB2, 0xA7, 0xBA},
    {0x23, 0x3A, 0xA7},
    {0x93, 0x6B, 0x42, 0x3A},
};
inline const uint8_t CREATS[17] = {0x47, 0x63, 0xA3, 0x4B, 0x65, 0x3C, 0xA9, 0xA3, 0x3C, 0x63, 0x47, 0x4E, 0x3C, 0x3A, 0x33, 0x87, 0x78};
inline const uint8_t TUNLDIR[17] = {0x30, 0x08, 0x18, 0x18, 0x1C, 0x04, 0x06, 0x0E, 0x03, 0x0F, 0x0F, 0x01, 0x03, 0x07, 0x0F, 0x40, 0x80};
inline const uint8_t RANDCRET[8] = {0xAA, 0xE1, 0x55, 0x0F, 0x8D, 0x69, 0x2D, 0x96};
inline const uint8_t ROCKS[17][5] = {
    {0x35, 0xB4, 0x9B, 0x00, 0x00}, {0x23, 0x39, 0x8D, 0xB5, 0x00}, {0x23, 0x6C, 0xBD, 0x94, 0x00},
    {0x43, 0x2B, 0x5C, 0xBD, 0x84}, {0x63, 0x29, 0x3D, 0xBB, 0xA6}, {0x28, 0x6B, 0x97, 0xB3, 0x00},
    {0x29, 0x52, 0x6D, 0xAC, 0x00}, {0x54, 0x6D, 0x96, 0xB8, 0x00}, {0x28, 0x3C, 0x7A, 0x72, 0xB6},
    {0x34, 0x72, 0x8C, 0xB8, 0x00}, {0x33, 0x3B, 0xBC, 0xA6, 0x00}, {0x39, 0x7B, 0x82, 0xCA, 0x00},
    {0x32, 0x29, 0xBA, 0xA3, 0x00}, {0x32, 0x4B, 0xB9, 0x83, 0x00}, {0x52, 0x39, 0x9A, 0xB4, 0x00},
    {0x53, 0x9B, 0x00, 0x00, 0x00}, {0x3B, 0x00, 0x00, 0x00, 0x00}};
inline const uint8_t RANDROKS[8] = {0x29, 0x35, 0x64, 0x82, 0x8D, 0x9A, 0xA5, 0xC9};
constexpr int MAXEFFR = 17;

constexpr int FULL0 = 150;
constexpr int NUMCREAT = 7, NUMROCK = 4;

// sound numbers
enum Song { SNGC = 1, SNGD = 2, SNGDF = 3, SNGNULL = 4, SNG1 = 5, SNG2 = 6, SNG3 = 7, SNG5 = 8, SNG4 = 9,
            SNG6 = 10, SNG7 = 11, SNG8 = 12, SNG9 = 13, SNG10 = 14, SNG11 = 15, SNG12 = 16, SNG13 = 17,
            SNG14 = 18, SNG15 = 19 };

inline void getrxy(int idx, int& x, int& y) {
    int row = idx >> 4;
    x = (idx & 0x0F) * 8;
    y = TOPZONE[15 - row];
}

inline int calcindx(int x, int y) {
    int zone = ((y & 0xFF) >> 2) / 3;
    if (zone > 15) zone = 15;
    return (MAPLOW[zone] + ((x & 0xFF) >> 3)) & 0xFF;
}

inline bool rowhere(int y) {
    for (int i = 0; i < 16; ++i)
        if (TOPZONE[i] == y) return true;
    return false;
}

}  // namespace dd
