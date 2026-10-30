// Software framebuffer: draws the game at the original 160 x 192 resolution.
// No SDL dependency so it can be used by the headless test program.
#pragma once
#include <cstdint>
#include <cstring>
#include <string>
#include "game.h"
#include "gfx_data.h"

namespace dd {

constexpr int FB_W = 160, FB_H = 192;

class Framebuffer {
public:
    uint32_t px[FB_W * FB_H];

    Framebuffer() { clear(); }

    void clear(uint32_t rgb = 0) {
        for (int i = 0; i < FB_W * FB_H; ++i) px[i] = rgb;
    }

    static uint32_t ntsc(int c) { return gfx::NTSC[c & 0xFF]; }

    void plot(int x, int y, uint32_t rgb) {
        if (x < 0 || y < 0 || x >= FB_W || y >= FB_H) return;
        px[y * FB_W + x] = rgb;
    }

    // Draw a stamp using the raw MARIA character data.
    void drawStamp(int cset, int idx, const uint8_t pal[4], int x, int y, int widthBytes = -1, int height = -1) {
        const uint8_t (*rows)[256] = cset == 2 ? gfx::CSET2 : gfx::CSET1;
        int pw = gfx::STMPPALW[(idx >> 1) & 0x7F];
        if (widthBytes < 0) {
            widthBytes = 32 - (pw & 31);
            if (widthBytes > 8) widthBytes = 2;
        }
        if (height < 0) height = (cset == 2 && idx >= 224) ? 16 : 12;
        uint32_t cols[4] = {0, ntsc(pal[1]), ntsc(pal[2]), ntsc(pal[3])};
        for (int t = 0; t < height; ++t) {
            const uint8_t* row = rows[t];
            for (int bx = 0; bx < widthBytes; ++bx) {
                uint8_t b = row[(idx + bx) & 0xFF];
                for (int p = 0; p < 4; ++p) {
                    int v = (b >> (6 - 2 * p)) & 3;
                    if (v) plot(x + bx * 4 + p, y + t, cols[v]);
                }
            }
        }
    }

    void paletteForStamp(int cset, int idx, const Game& g, uint8_t out[4]) {
        int pw = gfx::STMPPALW[(idx >> 1) & 0x7F];
        int p = pw >> 5;
        if (cset == 1 && idx >= 50 && idx < 74) {
            out[0] = 0; out[1] = g.vegcol[0]; out[2] = g.vegcol[1]; out[3] = g.vegcol[2];
            return;
        }
        std::memcpy(out, IPALETTE[p], 4);
    }

    void dirtPalette(const Game& g, int row, uint8_t out[4]) {
        if (row == 0) { std::memcpy(out, IPALETTE[7], 4); return; }
        int layer = row <= 3 ? 0 : row <= 7 ? 1 : row <= 11 ? 2 : 3;
        out[0] = 0;
        out[1] = DIRTCOLR[g.dirtScheme + layer];
        out[2] = SKYBLUE;
        out[3] = PEBBCOLR[g.dirtScheme + layer];
    }

    void drawDirt(const Game& g) {
        uint8_t pal[4];
        for (int row = 0; row < 15; ++row) {
            dirtPalette(g, row, pal);
            int y = 12 + row * 12;
            for (int col = 0; col < 16; ++col) {
                int ch = g.dirtmap[row * 16 + col];
                if (ch == 0 && row != 0) continue;
                drawStamp(1, ch, pal, 16 + col * 8, y, 2, 12);
            }
        }
        for (int y = 0; y < FB_H; ++y) {
            for (int x = 0; x < 16; ++x) { px[y * FB_W + x] = 0; px[y * FB_W + 144 + x] = 0; }
        }
    }

    void drawDigits(int value, int x, int y, int ndigits) {
        static const uint8_t pal[4] = {0, SKYBLUE, SKYBLUE, WHITE};
        std::string s = std::to_string(value);
        while ((int)s.size() < ndigits) s = " " + s;
        for (int i = 0; i < (int)s.size(); ++i) {
            if (s[i] == ' ') continue;
            drawStamp(1, FULL0 + 2 * (s[i] - '0'), pal, x + i * 8, y, 2, 12);
        }
    }

    void drawScoreLine(const Game& g, int hiscore) {
        for (int col = 1; col < 15; ++col) drawStamp(1, 30, IPALETTE[7], 16 + col * 8, 0, 2, 12);
        drawDigits(g.score[0] / 10 * 10, 24, 0, 6);
        if (g.numplayr) drawDigits(g.score[1] / 10 * 10, 88, 0, 6);
        int lives = g.nummen[g.playnum] - 1;
        if (lives < 0) lives = 0;
        if (lives > 9) lives = 9;
        drawStamp(1, FULL0 + 2 * lives, IPALETTE[4], 76, 0, 2, 12);
        if (!g.numplayr) {
            drawText("HI", 92, 3, WHITE);
            drawText(std::to_string(hiscore), 104, 3, WHITE);
        }
    }

    void drawGame(const Game& g, int hiscore) {
        clear();
        if (g.messageBlank && !g.message.empty()) {
            drawText(g.message, 80, 90, WHITE, true);
            return;
        }
        drawScoreLine(g, hiscore);
        drawDirt(g);
        uint8_t pal[4];
        for (const Stamp* s : g.getStamps()) {
            paletteForStamp(s->cset, s->ix, g, pal);
            drawStamp(s->cset, s->ix, pal, 16 + s->x, 203 - s->y);
        }
        if (!g.message.empty()) drawText(g.message, 80, 96, WHITE, true);
    }

    // 4x5 font
    static const char* glyph(char c) {
        switch (c) {
            case 'A': return "0110100111111001" "1001"; case 'B': return "1110100111101001" "1110";
            case 'C': return "0111100010001000" "0111"; case 'D': return "1110100110011001" "1110";
            case 'E': return "1111100011101000" "1111"; case 'F': return "1111100011101000" "1000";
            case 'G': return "0111100010111001" "0111"; case 'H': return "1001100111111001" "1001";
            case 'I': return "0111001000100010" "0111"; case 'J': return "0001000100011001" "0110";
            case 'K': return "1001101011001010" "1001"; case 'L': return "1000100010001000" "1111";
            case 'M': return "1001111111111001" "1001"; case 'N': return "1001110110111001" "1001";
            case 'O': return "0110100110011001" "0110"; case 'P': return "1110100111101000" "1000";
            case 'Q': return "0110100110011010" "0101"; case 'R': return "1110100111101010" "1001";
            case 'S': return "0111100001100001" "1110"; case 'T': return "0111001000100010" "0010";
            case 'U': return "1001100110011001" "0110"; case 'V': return "1001100110010110" "0110";
            case 'W': return "1001100111111111" "1001"; case 'X': return "1001011001100110" "1001";
            case 'Y': return "1001100101100010" "0010"; case 'Z': return "1111000101101000" "1111";
            case '0': return "0110100110011001" "0110"; case '1': return "0010011000100010" "0111";
            case '2': return "0110100100100100" "1111"; case '3': return "1110000101100001" "1110";
            case '4': return "1001100111110001" "0001"; case '5': return "1111100011100001" "1110";
            case '6': return "0111100011101001" "0110"; case '7': return "1111000100100100" "0100";
            case '8': return "0110100101101001" "0110"; case '9': return "0110100101110001" "1110";
            case '-': return "0000000011110000" "0000"; case '.': return "0000000000000000" "0010";
            case ':': return "0000001000000010" "0000"; case '/': return "0001001000100100" "1000";
            case '(': return "0010010001000100" "0010"; case ')': return "0100001000100010" "0100";
            case '!': return "0010001000100000" "0010"; case '<': return "0001001001000010" "0001";
            case '>': return "1000010000100100" "1000"; case '&': return "0100101001001010" "0101";
            default: return "0000000000000000" "0000";
        }
    }

    void drawText(const std::string& text, int x, int y, uint8_t colour = WHITE, bool centre = false) {
        uint32_t col = ntsc(colour);
        if (centre) x -= (int)text.size() * 5 / 2;
        for (char ch : text) {
            if (ch >= 'a' && ch <= 'z') ch = ch - 'a' + 'A';
            const char* gl = glyph(ch);
            for (int ry = 0; ry < 5; ++ry)
                for (int rx = 0; rx < 4; ++rx)
                    if (gl[ry * 4 + rx] == '1') plot(x + rx, y + ry, col);
            x += 5;
        }
    }
};

}  // namespace dd
