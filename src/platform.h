// PS5 platform layer: VideoOut presentation, DualSense input, AudioOut,
// threads, timing and /download0 files.  Replaces SDL in the desktop port.
#pragma once
#include <cstddef>
#include <cstdint>

namespace ps5 {

// ScePadData button bits
constexpr uint32_t PAD_L3 = 0x2, PAD_R3 = 0x4, PAD_OPTIONS = 0x8;
constexpr uint32_t PAD_UP = 0x10, PAD_RIGHT = 0x20, PAD_DOWN = 0x40, PAD_LEFT = 0x80;
constexpr uint32_t PAD_L2 = 0x100, PAD_R2 = 0x200, PAD_L1 = 0x400, PAD_R1 = 0x800;
constexpr uint32_t PAD_TRIANGLE = 0x1000, PAD_CIRCLE = 0x2000, PAD_CROSS = 0x4000, PAD_SQUARE = 0x8000;
constexpr uint32_t PAD_TOUCHPAD = 0x100000;

struct PadState {
    bool connected = false;
    uint32_t buttons = 0;
    int lx = 0, ly = 0;   // left stick, -128..127, centred on 0
};

void notify(const char* message);
[[noreturn]] void halt(const char* message);

// Opens VideoOut at 1920x1080 with two CPU-written scan-out buffers.
bool videoInit();
// Scales the 160x192 game framebuffer (0x00RRGGBB) to the screen and flips on vblank.
void videoPresent(const uint32_t* pixels, bool scanlines);

bool padInit();       // false while no user/pad is available; call again later
PadState padRead();

int audioOpen(int grain, int rate);              // 16-bit stereo main port, or < 0
void audioOutput(int port, const int16_t* grain); // blocks until the previous grain played

bool startThread(void* (*entry)(void*), void* arg);
double nowSeconds();
uint32_t entropy();

bool readFile(const char* path, char* buffer, size_t capacity, size_t* length);
bool writeFile(const char* path, const char* data, size_t length);

}  // namespace ps5
