#include "sound.h"
#include <cstring>
#include "platform.h"

namespace dd {

// ------------------------------------------------------------- song data ---
static const std::vector<int> F0_1 = {0x0F, 0x0E, 0x0F, 0x0E, 0x0F, 0x0E, 0x0F, 0x0E, 0x1F, 0x1E, 0x1F, 0x1E, 0x1F, 0x1E, 0x1F, 0x1E};
static const std::vector<int> F0_2 = {0x8F, 0x8F, 0xCF, 0x8F, 0x8F, 0x8F, 0x8F, 0x8F, 0x8F, 0xCF, 0x8F, 0x8F, 0x8F, 0x32,
                                      0xD1, 0xD1, 0x31, 0x32, 0x31, 0x2F, 0xD1, 0xD1, 0x31, 0x32, 0x31, 0x10};
static const std::vector<int> F0_4 = {0x60};
static const std::vector<int> F0_5 = {0x1B, 0x18, 0x15, 0x14, 0x12, 0x10, 0x0E, 0x0D};
static const std::vector<int> F0_6 = {0x14, 0x12, 0x14, 0x13, 0x0F, 0x0E, 0x0F, 0x0E};
static const std::vector<int> F0_7 = {0x30, 0x2F, 0x2E, 0x2D, 0x2E};
static const std::vector<int> F0_8 = {0x0B, 0x08, 0x06, 0x0B, 0x08, 0x06, 0x0B, 0x08, 0x06};
static const std::vector<int> F0_9 = {0x9A, 0x99, 0x98, 0x97, 0x96, 0x95, 0x94, 0x93, 0x32, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C};
static const std::vector<int> F0_10 = {0x1F, 0x1E, 0x1F, 0x1E, 0x1F, 0x1E, 0x1F, 0x1E, 0x0F, 0x0E, 0x0F, 0x10, 0x11, 0x10, 0x0E,
                                       0x0D, 0x0E, 0x0F, 0x0E, 0x0F};
static const std::vector<int> F0_11 = {0x1E, 0x1C, 0x1E, 0x1C, 0x14, 0x15, 0x13, 0x14};
static const std::vector<int> F0_12 = {0x0F, 0x12, 0x15, 0x19, 0x0F};
static const std::vector<int> F0_13 = {0x97, 0x97, 0x9C, 0x97, 0x94, 0x92, 0x91, 0x8F, 0x8D, 0x8D, 0x8D, 0x8D, 0x60, 0x40};
static const std::vector<int> F0_14 = {0x12, 0x11, 0x10, 0x0F, 0x0D, 0x0F};
static const std::vector<int> F0_15 = {0x17, 0x14, 0x12, 0x17, 0x9F, 0x8D, 0x6F};
static const std::vector<int> F0_16 = {0x8F, 0x92, 0x91, 0x92, 0x8E, 0x92, 0x91, 0x92, 0x8D, 0x92, 0x91, 0x12};
static const std::vector<int> F0_17 = {0x12, 0x11, 0x10, 0x8F, 0x12, 0x91, 0x14, 0x92, 0x17, 0x94, 0x18, 0x17, 0x97, 0x2B, 0x17};
static const std::vector<int> F0_18 = {0x0D, 0x0E, 0x0F, 0x0E, 0x0F, 0x10, 0x0F, 0x10, 0x11, 0x10, 0x11, 0x12, 0x33, 0x2C};
static const std::vector<int> F0_19 = {0x8F, 0x8F, 0x8F, 0x6F, 0x72, 0x2F, 0x72, 0x2F, 0x2D, 0x92, 0x92, 0x72, 0x34, 0x77, 0x76,
                                       0x7F, 0x36, 0x7F, 0x76, 0x94, 0x94, 0x54};
static const std::vector<int> F1_2 = {0xCF, 0xCF, 0xD0, 0xD0, 0xD1, 0xD1, 0xD2, 0xD2, 0xD3, 0xD3, 0xD3, 0xD3, 0xD4, 0xD4, 0xD2, 0x50};
static const std::vector<int> F1_5 = {0x1C, 0x19, 0x16, 0x15, 0x13, 0x11, 0x0F, 0x0E};
static const std::vector<int> F1_7 = {0x8A, 0x8A, 0x8A, 0x8A, 0x8A};
static const std::vector<int> F1_12 = {0x0E, 0x11, 0x14, 0x18, 0x0E};
static const std::vector<int> F1_13 = {0x8B, 0x8F, 0x20, 0x8F, 0x8B, 0x8F, 0x20, 0x8F, 0x8B, 0x8F, 0x20, 0x8F, 0x20, 0x8F, 0x8F, 0x0F};
static const std::vector<int> F1_16 = {0x20, 0x93, 0x93, 0x93, 0x20, 0x93, 0x93, 0x93, 0x20, 0x93, 0x93, 0x13};
static const std::vector<int> F1_17 = {0x40, 0x94, 0x00, 0x93, 0x00, 0x92, 0x00, 0x90, 0x00, 0x8F, 0x40, 0x0F};
static const std::vector<int> F1_18 = {0x0E, 0x0F, 0x10, 0x0F, 0x10, 0x11, 0x10, 0x11, 0x12, 0x11, 0x12, 0x13, 0x34, 0x2D};
static const std::vector<int> F1_19 = {0x34, 0x32, 0x30, 0x2F, 0x34, 0x2F, 0x34, 0x2F, 0x34, 0x2F, 0x34, 0x2F, 0x34, 0x2F, 0x34, 0x2F,
                                       0x34, 0x2F, 0x34, 0x31, 0x94, 0x94, 0x94, 0x31, 0x34, 0x31, 0x34, 0x30, 0x34, 0x30, 0x34, 0x10};

static const std::vector<int>* SONG_F0[20] = {nullptr, &F0_1, &F0_2, &F0_2, &F0_4, &F0_5, &F0_6, &F0_7, &F0_8, &F0_9,
                                              &F0_10, &F0_11, &F0_12, &F0_13, &F0_14, &F0_15, &F0_16, &F0_17, &F0_18, &F0_19};
static const std::vector<int>* SONG_F1[20] = {nullptr, nullptr, &F1_2, &F1_2, nullptr, &F1_5, nullptr, &F1_7, nullptr, nullptr,
                                              nullptr, nullptr, &F1_12, &F1_13, nullptr, nullptr, &F1_16, &F1_17, &F1_18, &F1_19};
static const int SNGC0[20] = {0xD4, 0x04, 0xD4, 0xD4, 0x00, 0x11, 0x0D, 0x47, 0x04, 0x04,
                              0x04, 0x00, 0xD4, 0xD4, 0x04, 0x04, 0xDD, 0xD4, 0xDD, 0xD4};
static const int SNGVV0[20] = {0xD4, 0x03, 0x22, 0x22, 0x00, 0x33, 0x03, 0x33, 0x03, 0x00,
                               0x04, 0x07, 0x74, 0x34, 0x07, 0x05, 0x33, 0x44, 0x00, 0x44};
static const int SNGDM[20] = {22, 1, 2, 1, 7, 1, 1, 1, 2, 0, 0, 2, 3, 3, 2, 6, 3, 6, 3, 4};
static const int SNG7C0[8] = {0x0D, 0x0D, 0x0D, 0x0D, 0x04, 0x04, 0x04, 0x04};
static const int SNG4V0[15] = {10, 10, 10, 10, 10, 10, 10, 10, 4, 4, 4, 4, 4, 4, 4};
static const int SNG14V0[14] = {6, 6, 6, 5, 5, 5, 4, 4, 4, 3, 3, 3, 2, 8};
static const int SNG14V1[14] = {5, 5, 5, 4, 4, 4, 3, 3, 3, 2, 2, 2, 1, 8};

// ------------------------------------------------------------- TIA voice ---
int TiaVoice::stepP4() {
    int b = ((p4 >> 1) ^ p4) & 1;
    p4 = (p4 >> 1) | (b << 3);
    return p4 & 1;
}
int TiaVoice::stepP5() {
    int b = ((p5 >> 2) ^ p5) & 1;
    p5 = (p5 >> 1) | (b << 4);
    return p5 & 1;
}
int TiaVoice::stepP9() {
    int b = ((p9 >> 4) ^ p9) & 1;
    p9 = (p9 >> 1) | (b << 8);
    return p9 & 1;
}

void TiaVoice::tick(int audc) {
    switch (audc) {
        case 0: out = 1; break;
        case 1: out = stepP4(); break;
        case 2: div = (div + 1) % 15; if (div == 0) out = stepP4(); break;
        case 3: if (stepP5()) out = stepP4(); break;
        case 4: case 5: out ^= 1; break;
        case 6: case 10: div = (div + 1) % 31; out = div < 18 ? 1 : 0; break;
        case 7: case 9: out = stepP5(); break;
        case 8: out = stepP9(); break;
        case 12: case 13: div = (div + 1) % 6; if (div == 0) out ^= 1; break;
        case 14: div = (div + 1) % 93; out = div < 47 ? 1 : 0; break;
        default: div = (div + 1) % 6; if (div == 0) out = stepP5(); break;
    }
}

void TiaVoice::render(int audc, int audf, int audv, int n, std::vector<int16_t>& o, int scale) {
    audc &= 0x0F;
    audf &= 0x1F;
    for (int i = 0; i < n; ++i) {
        cnt++;
        if (cnt > audf) {
            cnt = 0;
            tick(audc);
        }
        o[i] = (int16_t)(o[i] + out * audv * scale);
    }
}

// ---------------------------------------------------------- sound engine ---
SoundEngine::SoundEngine() {}

// The process never exits (the shell closes it), so the port stays open.
SoundEngine::~SoundEngine() {}

bool SoundEngine::init() {
    port = ps5::audioOpen(GRAIN, OUT_RATE);
    if (port < 0) return false;
    if (!ps5::startThread(&SoundEngine::feederMain, this)) return false;
    ready = true;
    return true;
}

void* SoundEngine::feederMain(void* self) {
    static_cast<SoundEngine*>(self)->feed();
    return nullptr;
}

// Feeder thread: AudioOut blocks until the previous grain has played, so it paces itself.
void SoundEngine::feed() {
    static int16_t grain[GRAIN * 2];
    bool primed = false;
    for (;;) {
        if (flushReq.exchange(false)) {
            tail.store(head.load(std::memory_order_acquire), std::memory_order_release);
            primed = false;
        }
        uint32_t h = head.load(std::memory_order_acquire);
        uint32_t t = tail.load(std::memory_order_relaxed);
        uint32_t have = h - t;
        if (!primed && have >= RING_PRIME) primed = true;
        if (primed && have >= (uint32_t)GRAIN) {
            for (int i = 0; i < GRAIN; ++i) {
                int16_t s = ring[(t + i) & (RING - 1)];
                grain[2 * i] = grain[2 * i + 1] = s;
            }
            tail.store(t + GRAIN, std::memory_order_release);
        } else {
            // underrun: play silence and rebuild a little buffer first
            primed = false;
            std::memset(grain, 0, sizeof(grain));
        }
        ps5::audioOutput(port, grain);
    }
}

void SoundEngine::request(int song) {
    if (song > sngtemp1) sngtemp1 = song;
}

void SoundEngine::stop() {
    cursong = 0;
    sngtemp1 = 0;
    audv[0] = audv[1] = 0;
    if (ready) flushReq.store(true);
}

void SoundEngine::savdun() {
    saved[0] = curindx[0]; saved[1] = curindx[1];
    saved[2] = rest[0]; saved[3] = rest[1];
    saved[4] = durctr;
    saved[5] = tunindx[0]; saved[6] = tunindx[1];
}

void SoundEngine::restoreDD() {
    curindx[0] = saved[0]; curindx[1] = saved[1];
    rest[0] = saved[2]; rest[1] = saved[3];
    durctr = saved[4];
    tunindx[0] = saved[5]; tunindx[1] = saved[6];
}

void SoundEngine::initalls() {
    curindx[0] = curindx[1] = 0;
    rest[0] = rest[1] = 0;
    durctr = 0;
    tunindx[0] = tunindx[1] = -1;
}

void SoundEngine::initdigs() {
    for (int i = 0; i < 5; ++i) saved[i] = 0;
    saved[5] = saved[6] = -1;
}

void SoundEngine::newsong(bool walking, bool fast, bool dying, bool entry) {
    audv[0] = audv[1] = 0;
    sngtemp1 = 0;
    int x = cursong;
    cursong = 0;
    if (x >= 5) { request(SNGNULL); return; }
    if (dying || entry) { request(SNGNULL); return; }
    if (walking) { request(fast ? SNGDF : SNGD); return; }
    if (x == SNGNULL || x == SNGC) { request(SNGC); return; }
    request(SNGNULL);
}

void SoundEngine::frame(bool walking, bool fast, bool dying, bool entry, bool nonoise) {
    if (nonoise) {
        audv[0] = audv[1] = 0;
        cursong = 0;
        sngtemp1 = 0;
        synth();
        return;
    }
    // SOUND2
    bool doSwitch = false;
    if (cursong == sngtemp1) {
        if (cursong == SNG1 || cursong == SNG4) doSwitch = true;
    } else if (sngtemp1 > cursong) {
        doSwitch = true;
    }
    if (doSwitch) {
        if (isDD(cursong)) savdun();
        cursong = sngtemp1;
        sngtemp1 = 0;
        if (isDD(cursong)) restoreDD();
        else initalls();
    }
    // SOUNDOK
    if (cursong == 0) {
        newsong(walking, fast, dying, entry);
        synth();
        return;
    }
    if (isDD(cursong) && (!walking || dying)) {
        savdun();
        newsong(walking, fast, dying, entry);
        synth();
        return;
    }
    durctr--;
    if (durctr < 0 && decode()) newsong(walking, fast, dying, entry);
    synth();
}

bool SoundEngine::decode() {
    int x = cursong;
    const std::vector<int>& f0 = *SONG_F0[x];
    curindx[0]--;
    if (curindx[0] < 0) {
        if (rest[0] & 0x80) {
            rest[0] = 0;
            audv[0] = 0;
            curindx[0] = 0;
        } else {
            tunindx[0]++;
            if (tunindx[0] >= (int)f0.size()) {
                if (isDD(x)) initdigs();
                return true;
            }
            int b = f0[tunindx[0]];
            rest[0] = b;
            audf[0] = b & 0x1F;
            curindx[0] = (b >> 5) & 3;
            int vol;
            if ((b & 0x1F) == 0) {
                vol = 0;
            } else {
                vol = SNGVV0[x] & 0x0F;
                if (vol == 0) vol = (x == SNG4) ? SNG4V0[tunindx[0] % 15] : SNG14V0[tunindx[0] % 14];
            }
            audv[0] = vol;
            int ctl = SNGC0[x] & 0x0F;
            if (ctl == 0) ctl = SNG7C0[tunindx[0] % 8];
            audc[0] = ctl;
        }
    }
    durctr = SNGDM[x];
    const std::vector<int>* f1 = SONG_F1[x];
    if (!f1) {
        audv[1] = 0;
        return false;
    }
    curindx[1]--;
    if (curindx[1] >= 0) return false;
    if (rest[1] & 0x80) {
        rest[1] = 0;
        curindx[1] = 0;
        audv[1] = 0;
        return false;
    }
    tunindx[1]++;
    int b = (*f1)[tunindx[1] % f1->size()];
    rest[1] = b;
    audf[1] = b & 0x1F;
    curindx[1] = (b >> 5) & 3;
    int vol;
    if ((b & 0x1F) == 0) {
        vol = 0;
    } else {
        vol = (SNGVV0[x] >> 4) & 0x0F;
        if (vol == 0) vol = SNG14V1[tunindx[1] % 14];
    }
    audv[1] = vol;
    audc[1] = (SNGC0[x] >> 4) & 0x0F;
    return false;
}

void SoundEngine::synth() {
    if (!ready) return;
    acc += TIA_CLOCK / 60.0;
    int n = (int)acc;
    acc -= n;
    buf.assign(n, 0);
    if (!muted) {
        for (int v = 0; v < 2; ++v)
            voices[v].render(audc[v], audf[v], audv[v], n, buf, 400);
    }
    // resample the TIA-rate block to the 48 kHz port (nearest sample, like the raw chip)
    const double stepIn = (double)TIA_CLOCK / OUT_RATE;
    uint32_t h = head.load(std::memory_order_relaxed);
    uint32_t t = tail.load(std::memory_order_acquire);
    // keep latency bounded: if the ring already holds several frames, drop this one
    bool drop = h - t > RING_MAX;
    double pos = phase;
    for (; pos < n; pos += stepIn) {
        if (!drop) ring[h++ & (RING - 1)] = buf[(int)pos];
    }
    phase = pos - n;
    head.store(h, std::memory_order_release);
}

}  // namespace dd
