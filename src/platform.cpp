// PS5 platform layer.  The VideoOut setup follows the hardware-proven CPU path of
// ps5-native-app-boilerplate (direct memory, tiled RGBA8 sRGB scan-out buffers).
#include "platform.h"
#include <atomic>
#include <cstddef>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

extern "C" {
size_t sceKernelGetDirectMemorySize();
int sceKernelAllocateDirectMemory(int64_t searchStart, int64_t searchEnd, size_t length, size_t alignment,
                                  int memoryType, int64_t* physicalAddress);
int sceKernelMapDirectMemory(void** address, size_t length, int protection, int flags,
                             int64_t physicalAddress, size_t alignment);
int sceKernelSendNotificationRequest(uint32_t device, void* request, size_t size, int blocking);
int sceKernelUsleep(uint32_t microseconds);
int sceSystemServiceHideSplashScreen();
int sceVideoOutOpen(int32_t userId, int32_t busType, int32_t index, const void* param);
int sceVideoOutSetFlipRate(int32_t handle, int32_t rate);
void sceVideoOutSetBufferAttribute2(void* attribute, uint64_t pixelFormat, uint32_t tilingMode, uint32_t width,
                                    uint32_t height, uint64_t option, uint32_t dccControl, uint64_t dccClearColor);
int sceVideoOutRegisterBuffers2(int32_t handle, int32_t setIndex, int32_t bufferIndexStart, void* buffers,
                                int32_t bufferCount, void* attribute, int32_t category, void* option);
int sceVideoOutSubmitFlip(int32_t handle, int32_t bufferIndex, uint32_t flipMode, int64_t flipArgument);
int sceVideoOutWaitVblank(int32_t handle);
int sceUserServiceInitialize(void* params);
int sceUserServiceGetInitialUser(int32_t* userId);
int scePadInit();
int scePadOpen(int32_t userId, int32_t type, int32_t index, const void* param);
int scePadReadState(int32_t handle, void* data);
int sceAudioOutInit();
int sceAudioOutOpen(int32_t userId, int32_t type, int32_t index, uint32_t length, uint32_t rate, uint32_t format);
int sceAudioOutOutput(int32_t handle, const void* data);
}

namespace ps5 {
namespace {

// ------------------------------------------------------------ messages ---
struct NotificationRequest {
    uint8_t reserved[45];
    char message[3075];
};
NotificationRequest notification;

// --------------------------------------------------------------- video ---
constexpr int SCREEN_W = 1920, SCREEN_H = 1080;
constexpr size_t FRAME_BYTES = 0x1000000;
constexpr size_t MEMORY_ALIGNMENT = 0x200000;
constexpr int MEMORY_TYPE_WC_GARLIC = 3;
constexpr int MAP_PROTECTION = 0x33;
constexpr uint64_t PIXEL_FORMAT_RGBA8_SRGB = 0x8000000022000000ull;
constexpr uint32_t FLIP_MODE_VSYNC = 1;

// Scan-out memory is tiled in 128x128-pixel, 64 KiB blocks.
constexpr int BLOCK = 128, BLOCK_PIXELS = BLOCK * BLOCK;
constexpr int BLOCKS_X = (SCREEN_W + BLOCK - 1) / BLOCK, BLOCKS_Y = (SCREEN_H + BLOCK - 1) / BLOCK;

// The 160x192 game image; each pixel is 10x5 on screen (the 7800's wide pixels).
constexpr int GAME_W = 160, GAME_H = 192, SCALE_X = 10, SCALE_Y = 5;
constexpr int ORIGIN_X = (SCREEN_W - GAME_W * SCALE_X) / 2, ORIGIN_Y = (SCREEN_H - GAME_H * SCALE_Y) / 2;
constexpr uint32_t BORDER = 0xFF000000;

struct VideoBuffer {
    void* data;
    void* metadata;
    void* reserved0;
    void* reserved1;
};
struct VideoAttribute {
    uint8_t reserved[80];
};

int video = -1;
uint32_t* frames[2];
int shown = 0;
int64_t flipArgument = 0;
uint8_t tileX[BLOCK_PIXELS], tileY[BLOCK_PIXELS];   // tile-local pixel index -> (x, y)
int16_t columnSource[BLOCKS_X * BLOCK];              // screen x -> game x, or -1
int16_t rowSource[BLOCKS_Y * BLOCK];                 // screen y -> game y, or -1
bool rowDimmed[BLOCKS_Y * BLOCK];                    // last screen line of a game line
bool blockLive[BLOCKS_Y][BLOCKS_X];                  // block overlaps the game image
uint32_t bright[GAME_W * GAME_H], dimmed[GAME_W * GAME_H];

constexpr uint32_t tileByteOffset(uint32_t x, uint32_t y) {
    return ((y << 4) & 0x70U) ^ ((y << 5) & 0xf00U) ^ ((y << 9) & 0x1000U) ^ ((y << 8) & 0x4000U) ^
           ((x << 2) & 0xcU) ^ ((x << 5) & 0x380U) ^ ((x << 4) & 0x400U) ^ ((x << 6) & 0x800U) ^
           ((x << 9) & 0xa000U);
}

void buildTables() {
    for (uint32_t y = 0; y < BLOCK; ++y)
        for (uint32_t x = 0; x < BLOCK; ++x) {
            uint32_t i = tileByteOffset(x, y) >> 2;
            tileX[i] = (uint8_t)x;
            tileY[i] = (uint8_t)y;
        }
    for (int x = 0; x < BLOCKS_X * BLOCK; ++x) {
        int g = x - ORIGIN_X;
        columnSource[x] = (int16_t)(g >= 0 && g < GAME_W * SCALE_X ? g / SCALE_X : -1);
    }
    for (int y = 0; y < BLOCKS_Y * BLOCK; ++y) {
        int g = y - ORIGIN_Y;
        bool inside = y < SCREEN_H && g >= 0 && g < GAME_H * SCALE_Y;
        rowSource[y] = (int16_t)(inside ? g / SCALE_Y : -1);
        rowDimmed[y] = inside && g % SCALE_Y == SCALE_Y - 1;
    }
    for (int by = 0; by < BLOCKS_Y; ++by)
        for (int bx = 0; bx < BLOCKS_X; ++bx) {
            bool live = false;
            for (int y = by * BLOCK; y < (by + 1) * BLOCK && !live; ++y)
                if (rowSource[y] >= 0)
                    for (int x = bx * BLOCK; x < (bx + 1) * BLOCK; ++x)
                        if (columnSource[x] >= 0) { live = true; break; }
            blockLive[by][bx] = live;
        }
}

// Writes the screen into a scan-out buffer, walking the tiled memory in order.
void fillFrame(uint32_t* base, bool everything, bool scanlines) {
    for (int by = 0; by < BLOCKS_Y; ++by)
        for (int bx = 0; bx < BLOCKS_X; ++bx) {
            if (!everything && !blockLive[by][bx]) continue;
            uint32_t* dst = base + (size_t)(by * BLOCKS_X + bx) * BLOCK_PIXELS;
            const int x0 = bx * BLOCK, y0 = by * BLOCK;
            for (int i = 0; i < BLOCK_PIXELS; ++i) {
                int gx = columnSource[x0 + tileX[i]];
                int y = y0 + tileY[i];
                int gy = rowSource[y];
                uint32_t v = BORDER;
                if (gx >= 0 && gy >= 0) v = (scanlines && rowDimmed[y] ? dimmed : bright)[gy * GAME_W + gx];
                dst[i] = v;
            }
        }
}

void flushRange(void* address, size_t length) {
    auto* at = static_cast<uint8_t*>(address);
    for (const uint8_t* end = at + length; at < end; at += 64) __asm__ volatile("clflush (%0)" : : "r"(at) : "memory");
    __asm__ volatile("mfence" ::: "memory");
}

void flushFrame(uint32_t* base, bool everything) {
    for (int by = 0; by < BLOCKS_Y; ++by)
        for (int bx = 0; bx < BLOCKS_X; ++bx)
            if (everything || blockLive[by][bx])
                flushRange(base + (size_t)(by * BLOCKS_X + bx) * BLOCK_PIXELS, BLOCK_PIXELS * 4);
}

// ---------------------------------------------------------- user / pad ---
bool userServiceReady = false;
int padHandle = -1;

void userServiceUp() {
    if (userServiceReady) return;
    (void)sceUserServiceInitialize(nullptr);   // fails harmlessly if already initialised
    userServiceReady = true;
}

// ScePadData: 120 bytes, buttons first, sticks at 4, connected flag at 0x4c.
struct alignas(8) PadData {
    uint32_t buttons;
    uint8_t leftX, leftY, rightX, rightY;
    uint8_t l2, r2;
    uint8_t pad0[2];
    uint8_t motion[40];
    uint8_t touch[24];
    uint8_t connected;
    uint8_t pad1[3];
    uint64_t timestamp;
    uint8_t extension[32];
};
static_assert(sizeof(PadData) == 120, "ScePadData size");
static_assert(offsetof(PadData, connected) == 0x4c, "ScePadData connected offset");
constexpr uint32_t PAD_INTERCEPTED = 0x80000000;   // the system UI owns the pad

}  // namespace

void notify(const char* message) {
    size_t n = 0;
    while (message[n] && n < sizeof(notification.message) - 1) {
        notification.message[n] = message[n];
        ++n;
    }
    notification.message[n] = '\0';
    (void)sceKernelSendNotificationRequest(0, &notification, sizeof(notification), 0);
}

void halt(const char* message) {
    notify(message);
    // Returning from main crashes this launch context; the shell closes the app.
    for (;;) (void)sceKernelUsleep(1000000);
}

bool videoInit() {
    buildTables();
    (void)sceSystemServiceHideSplashScreen();
    video = sceVideoOutOpen(0xFF, 0, 0, nullptr);
    if (video < 0) return false;

    const size_t memoryBytes = FRAME_BYTES * 2;
    const size_t pool = sceKernelGetDirectMemorySize();
    if (pool < memoryBytes) return false;
    int64_t physical = 0;
    if (sceKernelAllocateDirectMemory(0, (int64_t)pool, memoryBytes, MEMORY_ALIGNMENT, MEMORY_TYPE_WC_GARLIC,
                                      &physical) < 0)
        return false;
    void* mapped = nullptr;
    if (sceKernelMapDirectMemory(&mapped, memoryBytes, MAP_PROTECTION, 0, physical, MEMORY_ALIGNMENT) < 0)
        return false;
    frames[0] = static_cast<uint32_t*>(mapped);
    frames[1] = reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(mapped) + FRAME_BYTES);

    for (uint32_t& p : bright) p = BORDER;
    for (int f = 0; f < 2; ++f) {
        fillFrame(frames[f], true, false);
        flushFrame(frames[f], true);
    }

    VideoBuffer buffers[2] = {{frames[0], nullptr, nullptr, nullptr}, {frames[1], nullptr, nullptr, nullptr}};
    VideoAttribute attribute{};
    (void)sceVideoOutSetFlipRate(video, 0);
    sceVideoOutSetBufferAttribute2(&attribute, PIXEL_FORMAT_RGBA8_SRGB, 0, SCREEN_W, SCREEN_H, 0, 0, 0);
    if (sceVideoOutRegisterBuffers2(video, 0, 0, buffers, 2, &attribute, 0, nullptr) < 0) return false;
    if (sceVideoOutSubmitFlip(video, 0, FLIP_MODE_VSYNC, ++flipArgument) < 0) return false;
    (void)sceVideoOutWaitVblank(video);
    shown = 0;
    return true;
}

void videoPresent(const uint32_t* pixels, bool scanlines) {
    // 0x00RRGGBB -> RGBA8 in memory (0xAABBGGRR); scanlines darken to 5/8
    for (int i = 0; i < GAME_W * GAME_H; ++i) {
        uint32_t c = pixels[i];
        uint32_t r = (c >> 16) & 0xFF, g = (c >> 8) & 0xFF, b = c & 0xFF;
        bright[i] = 0xFF000000 | b << 16 | g << 8 | r;
        dimmed[i] = 0xFF000000 | (b * 5 / 8) << 16 | (g * 5 / 8) << 8 | (r * 5 / 8);
    }
    const int back = shown ^ 1;
    fillFrame(frames[back], false, scanlines);
    flushFrame(frames[back], false);
    bool queued = false;
    for (int attempt = 0; attempt < 3 && !queued; ++attempt) {
        queued = sceVideoOutSubmitFlip(video, back, FLIP_MODE_VSYNC, ++flipArgument) >= 0;
        if (!queued) (void)sceVideoOutWaitVblank(video);   // the previous flip is still pending
    }
    (void)sceVideoOutWaitVblank(video);
    if (queued) shown = back;
}

bool padInit() {
    userServiceUp();
    static bool padLibraryReady = false;
    if (!padLibraryReady) {
        if (scePadInit() != 0) return false;
        padLibraryReady = true;
    }
    int32_t user = 0;
    if (sceUserServiceGetInitialUser(&user) != 0) return false;
    padHandle = scePadOpen(user, 0, 0, nullptr);
    return padHandle >= 0;
}

PadState padRead() {
    PadState s;
    if (padHandle < 0) return s;
    PadData d{};
    if (scePadReadState(padHandle, &d) != 0 || !d.connected) return s;
    s.connected = true;
    if (d.buttons & PAD_INTERCEPTED) return s;
    s.buttons = d.buttons;
    s.lx = (int)d.leftX - 128;
    s.ly = (int)d.leftY - 128;
    return s;
}

int audioOpen(int grain, int rate) {
    userServiceUp();
    (void)sceAudioOutInit();   // fails harmlessly if already initialised
    constexpr int32_t SYSTEM_USER = 0xFF, PORT_MAIN = 0;
    constexpr uint32_t FORMAT_S16_STEREO = 1;
    int handle = sceAudioOutOpen(SYSTEM_USER, PORT_MAIN, 0, (uint32_t)grain, (uint32_t)rate, FORMAT_S16_STEREO);
    if (handle < 0) {
        int32_t user = 0;
        if (sceUserServiceGetInitialUser(&user) == 0)
            handle = sceAudioOutOpen(user, PORT_MAIN, 0, (uint32_t)grain, (uint32_t)rate, FORMAT_S16_STEREO);
    }
    return handle;
}

void audioOutput(int port, const int16_t* grain) {
    if (sceAudioOutOutput(port, grain) < 0) (void)sceKernelUsleep(5000);
}

bool startThread(void* (*entry)(void*), void* arg) {
    pthread_t thread;
    if (pthread_create(&thread, nullptr, entry, arg) != 0) return false;
    (void)pthread_detach(thread);
    return true;
}

double nowSeconds() {
    timespec ts{};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + ts.tv_nsec * 1e-9;
}

uint32_t entropy() {
    timespec ts{};
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint32_t)ts.tv_nsec ^ (uint32_t)ts.tv_sec * 2654435761u;
}

bool readFile(const char* path, char* buffer, size_t capacity, size_t* length) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) return false;
    size_t total = 0;
    while (total < capacity) {
        ssize_t n = read(fd, buffer + total, capacity - total);
        if (n <= 0) break;
        total += (size_t)n;
    }
    close(fd);
    *length = total;
    return true;
}

bool writeFile(const char* path, const char* data, size_t length) {
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) return false;
    size_t done = 0;
    while (done < length) {
        ssize_t n = write(fd, data + done, length - done);
        if (n <= 0) break;
        done += (size_t)n;
    }
    close(fd);
    return done == length;
}

}  // namespace ps5
