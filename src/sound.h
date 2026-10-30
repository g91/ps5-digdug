// Sound: the SOUND.S sequencer driving a small real-time TIA emulation.
#pragma once
#include <atomic>
#include <cstdint>
#include <vector>
#include "game.h"

namespace dd {

class TiaVoice {
public:
    void render(int audc, int audf, int audv, int n, std::vector<int16_t>& out, int scale);
private:
    int p4 = 0x0F, p5 = 0x1F, p9 = 0x1FF, div = 0, out = 0, cnt = 0;
    int stepP4();
    int stepP5();
    int stepP9();
    void tick(int audc);
};

class SoundEngine : public SoundSink {
public:
    SoundEngine();
    ~SoundEngine();
    bool init();                     // opens the AudioOut port and starts its feeder thread
    void request(int song) override;
    void frame(bool walking, bool fast, bool dying, bool entry, bool nonoise);
    void stop();
    bool muted = false;

private:
    static constexpr int TIA_CLOCK = 31400;
    static constexpr int OUT_RATE = 48000;          // AudioOut main port rate
    static constexpr int GRAIN = 256;               // frames per sceAudioOutOutput call
    static constexpr uint32_t RING = 8192;          // mono samples at OUT_RATE, power of two
    static constexpr uint32_t RING_MAX = 3200;      // ~4 frames: bounds the latency
    static constexpr uint32_t RING_PRIME = 1024;    // buffered before playback (re)starts
    int port = -1;
    bool ready = false;
    double acc = 0;
    double phase = 0;                               // resampler position in TIA samples
    TiaVoice voices[2];
    std::vector<int16_t> buf;

    // single-producer (game thread) / single-consumer (feeder thread) ring
    int16_t ring[RING] = {};
    std::atomic<uint32_t> head{0}, tail{0};
    std::atomic<bool> flushReq{false};
    static void* feederMain(void* self);
    void feed();

    // sequencer state (names follow the assembly)
    int cursong = 0, sngtemp1 = 0;
    int curindx[2] = {0, 0}, rest[2] = {0, 0}, tunindx[2] = {-1, -1}, durctr = 0;
    int saved[7] = {0, 0, 0, 0, 0, -1, -1};
    int audc[2] = {0, 0}, audf[2] = {0, 0}, audv[2] = {0, 0};

    static bool isDD(int s) { return s == SNGD || s == SNGDF; }
    void savdun();
    void restoreDD();
    void initalls();
    void initdigs();
    void newsong(bool walking, bool fast, bool dying, bool entry);
    bool decode();   // true when the song ended
    void synth();
};

}  // namespace dd
