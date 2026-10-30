// Host-side check of the PS5 platform layer: the sce* calls are stubbed, the
// real platform.cpp / game / sound code runs, and the tiled scan-out buffer is
// de-tiled into raw RGB screenshots; AudioOut submissions are captured as WAV.
//
//   clang++-18 -std=c++20 -O2 -Isrc tests/host_preview.cpp src/platform.cpp \
//       src/game.cpp src/monsters.cpp src/sound.cpp -lpthread -o build/host_preview
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <set>
#include <unistd.h>
#include <vector>
#include "framebuffer.h"
#include "game.h"
#include "platform.h"
#include "sound.h"

using namespace dd;

static uint8_t* scanout[2];
static std::atomic<int> lastFlip{-1};
static std::mutex audioLock;
static std::vector<int16_t> captured;   // interleaved stereo
static std::atomic<bool> capturing{false};

extern "C" {
size_t sceKernelGetDirectMemorySize() { return (size_t)1 << 30; }
int sceKernelAllocateDirectMemory(int64_t, int64_t, size_t, size_t, int, int64_t* p) { *p = 0; return 0; }
int sceKernelMapDirectMemory(void** a, size_t n, int, int, int64_t, size_t align) {
    *a = std::aligned_alloc(align, n);
    std::memset(*a, 0xCD, n);
    return 0;
}
int sceKernelSendNotificationRequest(uint32_t, void* r, size_t, int) {
    std::printf("notify: %s\n", (char*)r + 45);
    return 0;
}
int sceKernelUsleep(uint32_t us) { return usleep(us); }
int sceSystemServiceHideSplashScreen() { return 0; }
int sceVideoOutOpen(int32_t, int32_t, int32_t, const void*) { return 1; }
int sceVideoOutSetFlipRate(int32_t, int32_t) { return 0; }
void sceVideoOutSetBufferAttribute2(void*, uint64_t, uint32_t, uint32_t, uint32_t, uint64_t, uint32_t, uint64_t) {}
int sceVideoOutRegisterBuffers2(int32_t, int32_t, int32_t, void* b, int32_t n, void*, int32_t, void*) {
    void** v = (void**)b;
    for (int i = 0; i < n; ++i) scanout[i] = (uint8_t*)v[i * 4];
    return 0;
}
int sceVideoOutSubmitFlip(int32_t, int32_t index, uint32_t, int64_t) { lastFlip = index; return 0; }
int sceVideoOutWaitVblank(int32_t) { return 0; }
int sceUserServiceInitialize(void*) { return 0; }
int sceUserServiceGetInitialUser(int32_t* u) { *u = 1; return 0; }
int scePadInit() { return 0; }
int scePadOpen(int32_t, int32_t, int32_t, const void*) { return 1; }
int scePadReadState(int32_t, void* d) { std::memset(d, 0, 120); ((uint8_t*)d)[0x4c] = 1; return 0; }
int sceAudioOutInit() { return 0; }
int sceAudioOutOpen(int32_t, int32_t, int32_t, uint32_t, uint32_t, uint32_t) { return 7; }
int sceAudioOutOutput(int32_t, const void* data) {
    usleep(5333);   // 256 frames at 48 kHz
    if (capturing) {
        std::lock_guard<std::mutex> g(audioLock);
        const int16_t* s = (const int16_t*)data;
        captured.insert(captured.end(), s, s + 512);
    }
    return 0;
}
}

// Reference tiling (ps5-native-app-boilerplate's hardware-proven formula).
static uint32_t tiled(uint32_t x, uint32_t y) {
    uint32_t off = ((y << 4) & 0x70U) ^ ((y << 5) & 0xf00U) ^ ((y << 9) & 0x1000U) ^ ((y << 8) & 0x4000U) ^
                   ((x << 2) & 0xcU) ^ ((x << 5) & 0x380U) ^ ((x << 4) & 0x400U) ^ ((x << 6) & 0x800U) ^
                   ((x << 9) & 0xa000U);
    uint32_t block = (y >> 7) * ((1920 + 127) >> 7) + (x >> 7);
    return (block << 16) + off;
}

static void dumpScreen(const char* path) {
    const uint8_t* base = scanout[lastFlip];
    FILE* f = std::fopen(path, "wb");
    for (uint32_t y = 0; y < 1080; ++y)
        for (uint32_t x = 0; x < 1920; ++x) std::fwrite(base + tiled(x, y), 1, 3, f);   // R, G, B
    std::fclose(f);
    std::printf("wrote %s (buffer %d)\n", path, lastFlip.load());
}

static void dumpWav(const char* path) {
    std::lock_guard<std::mutex> g(audioLock);
    FILE* f = std::fopen(path, "wb");
    uint32_t bytes = (uint32_t)(captured.size() * 2);
    uint32_t hdr[11] = {0x46464952, 36 + bytes, 0x45564157, 0x20746d66, 16, 0x00020001, 48000, 48000 * 4,
                        0x00100004, 0x61746164, bytes};
    std::fwrite(hdr, 4, 11, f);
    std::fwrite(captured.data(), 2, captured.size(), f);
    std::fclose(f);
    std::printf("wrote %s (%.1f s)\n", path, captured.size() / 2 / 48000.0);
}

int main(int argc, char** argv) {
    const char* out = argc > 1 ? argv[1] : ".";
    // the tile formula must be a bijection on each 64 KiB block
    std::set<uint32_t> seen;
    for (uint32_t y = 0; y < 128; ++y)
        for (uint32_t x = 0; x < 128; ++x) seen.insert(tiled(x, y));
    std::printf("tile bijection: %s\n", seen.size() == 16384 && *seen.rbegin() == 65532 ? "ok" : "BROKEN");

    std::srand(1);
    if (!ps5::videoInit()) return 1;
    SoundEngine sound;
    if (!sound.init()) return 2;
    Framebuffer fb;
    char path[512];

    // a real one-player game, steered by a scripted joystick, paced at 60 Hz
    Game g(1, false, false, &sound);
    capturing = true;
    for (int f = 0; f < 720; ++f) {
        uint8_t joy = 0xFF;
        bool button = false;
        int phase = (f / 90) % 4;
        if (f > 150) joy &= ~(phase == 0 ? JOY_S : phase == 1 ? JOY_E : phase == 2 ? JOY_S : JOY_W);
        if (f > 150 && (f % 40) < 6) button = true;
        g.frame(joy, button);
        sound.frame(g.walking != 0, (g.fast || g.escaper) != 0, (g.dethwish || g.death || g.squash) != 0,
                    g.entry != 0, g.nonoise != 0);
        fb.drawGame(g, 10000);
        bool scan = f >= 600;
        ps5::videoPresent(fb.px, scan);
        if (f == 420) { std::snprintf(path, sizeof path, "%s/play.rgb", out); dumpScreen(path); }
        if (f == 719) { std::snprintf(path, sizeof path, "%s/play_scanlines.rgb", out); dumpScreen(path); }
        usleep(16667);
    }
    capturing = false;
    std::snprintf(path, sizeof path, "%s/play.wav", out);
    dumpWav(path);
    std::printf("score %d, lives %d\n", g.score[0], g.nummen[0]);
    return 0;
}
