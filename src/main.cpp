// Dig Dug (Atari 7800) -- PlayStation 5 port of the C++ version.
//
// Controls (DualSense)
//   Left stick / D-pad   move              Cross / Square / Circle / R1  pump
//   Cross or OPTIONS     start game        Up/Down    1 or 2 players (title)
//   Left/Right           easy/normal       OPTIONS    pause
//   Triangle             sound on/off      Touch pad  scanlines on/off
#include <cstdint>
#include <cstdlib>
#include <string>
#include "framebuffer.h"
#include "game.h"
#include "platform.h"
#include "sound.h"

using namespace dd;

static const int TITLE_TIME = 60 * 12;
static const char* const HISCORE_PATH = "/download0/hiscore.txt";

static int loadHiscore() {
    char text[16];
    size_t n = 0;
    if (!ps5::readFile(HISCORE_PATH, text, sizeof text, &n)) return 10000;
    int v = 0;
    for (size_t i = 0; i < n && text[i] >= '0' && text[i] <= '9'; ++i) v = v * 10 + (text[i] - '0');
    return v > 0 ? v : 10000;
}

static void saveHiscore(int v) {
    std::string s = std::to_string(v);
    ps5::writeFile(HISCORE_PATH, s.data(), s.size());
}

struct App {
    Framebuffer fb;
    SoundEngine sound;
    Game* game = nullptr;
    enum State { TITLE, ATTRACT, PLAY, GAMEOVER } state = TITLE;
    int titleTimer = 0, gameoverTimer = 0;
    int numPlayers = 1;
    bool bonzo = false;
    bool paused = false;
    bool scanlines = false;
    int hiscore = 10000;
    bool hiscoreDirty = false;
    bool havePad = false;
    int padRetry = 0;
    ps5::PadState pad;
    uint32_t keys = 0, prevKeys = 0;

    void startGame(bool attract) {
        sound.stop();
        delete game;
        game = new Game(numPlayers, bonzo, attract, &sound);
        state = attract ? ATTRACT : PLAY;
        paused = false;
    }

    void endGame() {
        state = TITLE;
        delete game;
        game = nullptr;
        sound.stop();
        if (hiscoreDirty) {
            saveHiscore(hiscore);
            hiscoreDirty = false;
        }
    }

    // D-pad and left stick both count as directions.
    void pollPad() {
        if (!havePad && --padRetry <= 0) {
            havePad = ps5::padInit();
            padRetry = 120;
        }
        pad = ps5::padRead();
        keys = pad.buttons;
        if (pad.lx > 64) keys |= ps5::PAD_RIGHT;
        if (pad.lx < -64) keys |= ps5::PAD_LEFT;
        if (pad.ly > 64) keys |= ps5::PAD_DOWN;
        if (pad.ly < -64) keys |= ps5::PAD_UP;
    }

    void readJoy(uint8_t& joy, bool& button) {
        joy = 0xFF;
        if (keys & ps5::PAD_RIGHT) joy &= ~JOY_E;
        if (keys & ps5::PAD_LEFT) joy &= ~JOY_W;
        if (keys & ps5::PAD_DOWN) joy &= ~JOY_S;
        if (keys & ps5::PAD_UP) joy &= ~JOY_N;
        button = (keys & (ps5::PAD_CROSS | ps5::PAD_SQUARE | ps5::PAD_CIRCLE | ps5::PAD_R1 | ps5::PAD_R2)) != 0;
    }

    void onPress(uint32_t pressed) {
        if (pressed & ps5::PAD_TRIANGLE) {
            sound.muted = !sound.muted;
            if (sound.muted) sound.stop();
        }
        if (pressed & ps5::PAD_TOUCHPAD) scanlines = !scanlines;
        if (state == TITLE || state == ATTRACT) {
            if (pressed & (ps5::PAD_CROSS | ps5::PAD_OPTIONS)) startGame(false);
            else if (pressed & ps5::PAD_UP) numPlayers = 2;
            else if (pressed & ps5::PAD_DOWN) numPlayers = 1;
            else if (pressed & ps5::PAD_LEFT) bonzo = true;
            else if (pressed & ps5::PAD_RIGHT) bonzo = false;
        } else if (state == PLAY) {
            if (pressed & ps5::PAD_OPTIONS) paused = !paused;
        }
    }

    void step() {
        if (state == TITLE) {
            if (++titleTimer > TITLE_TIME) {
                titleTimer = 0;
                startGame(true);
            }
            return;
        }
        if (paused) return;
        if (state == PLAY && !pad.connected) return;   // wait for the controller
        Game& g = *game;
        if (state == ATTRACT) {
            g.frame(0xFF, false);
            if (g.gameOver) endGame();
            return;
        }
        uint8_t joy;
        bool button;
        readJoy(joy, button);
        g.frame(joy, button);
        int best = g.score[0] > g.score[1] ? g.score[0] : g.score[1];
        if (best > hiscore) { hiscore = best; hiscoreDirty = true; }
        sound.frame(g.walking != 0, (g.fast || g.escaper) != 0,
                    (g.dethwish || g.death || g.squash) != 0, g.entry != 0, g.nonoise != 0);
        if (state == PLAY && g.gameOver) {
            state = GAMEOVER;
            gameoverTimer = 0;
        }
        if (state == GAMEOVER) {
            if (++gameoverTimer > 240) endGame();
        }
    }

    void drawTitle() {
        fb.clear();
        std::string logo = "DIG DUG";
        int x = 80 - (int)logo.size() * 5 / 2;
        for (int dy = 0; dy < 3; ++dy)
            for (int dx = 0; dx < 3; ++dx) fb.drawText(logo, x + dx, 30 + dy, RED);
        fb.drawText(logo, x + 1, 31, YELLOW);
        fb.drawStamp(2, 2, IPALETTE[0], 24, 26);
        fb.drawStamp(2, 128, IPALETTE[1], 124, 26);
        fb.drawStamp(2, 136, IPALETTE[3], 110, 26);
        fb.drawText("PLAYERS  " + std::to_string(numPlayers), 80, 70, WHITE, true);
        fb.drawText(std::string("LEVEL  ") + (bonzo ? "TEDDY (EASY)" : "PINEAPPLE"), 80, 82, WHITE, true);
        static const uint8_t tedPal[4] = {0, 0x16, 0x1A, 0x14};
        static const uint8_t pinePal[4] = {0, 0xD2, 0x1A, 0x24};
        fb.drawStamp(1, bonzo ? TEDDY : 72, bonzo ? tedPal : pinePal, 128, 79);
        fb.drawText("UP/DOWN  PLAYERS", 80, 100, SKYBLUE, true);
        fb.drawText("LEFT/RIGHT  LEVEL", 80, 108, SKYBLUE, true);
        fb.drawText("STICK MOVE  CROSS PUMP", 80, 120, SKYBLUE, true);
        fb.drawText("OPTIONS PAUSE  TRIANGLE SOUND", 80, 128, SKYBLUE, true);
        if ((titleTimer / 30) % 2 == 0) fb.drawText("PRESS CROSS TO START", 80, 142, YELLOW, true);
        fb.drawText("HIGH SCORE " + std::to_string(hiscore), 80, 160, WHITE, true);
        fb.drawText("(C) 1983 ATARI  (C) 1982 NAMCO", 80, 180, WHITE, true);
    }

    void draw() {
        if (state == TITLE) {
            drawTitle();
        } else {
            fb.drawGame(*game, hiscore);
            if (state == ATTRACT) {
                if ((titleTimer / 30) % 2 == 0) fb.drawText("PRESS CROSS", 80, 100, WHITE, true);
                titleTimer++;
            }
            if (state == PLAY && !pad.connected) fb.drawText("CONNECT CONTROLLER", 80, 100, WHITE, true);
            else if (paused) fb.drawText("PAUSED", 80, 100, WHITE, true);
        }
        ps5::videoPresent(fb.px, scanlines);
    }

    // One game step per 60 Hz tick; presentation waits for vblank.  A display
    // running at (almost exactly) 60 Hz is locked to one step per frame.
    [[noreturn]] void run() {
        const double tick = 1.0 / 60.0;
        double last = ps5::nowSeconds(), acc = 0;
        for (;;) {
            pollPad();
            onPress(keys & ~prevKeys);
            prevKeys = keys;
            double now = ps5::nowSeconds();
            double dt = now - last;
            last = now;
            if (dt > tick - 0.002 && dt < tick + 0.002) dt = tick;
            acc += dt;
            if (acc > 0.1) acc = tick;   // fell far behind (system menu); resynchronise
            while (acc >= tick - 1e-6) {
                step();
                acc -= tick;
            }
            draw();
        }
    }
};

static App app;

int main() {
    std::srand(ps5::entropy());
    if (!ps5::videoInit()) ps5::halt("Dig Dug: video initialisation failed");
    if (!app.sound.init()) ps5::notify("Dig Dug: audio unavailable");
    app.hiscore = loadHiscore();
    app.run();
}
