// Dig Dug game logic -- a faithful port of the Atari 7800 assembly.
// One call to Game::frame() advances the simulation by one 60 Hz frame.
#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "tables.h"

namespace dd {

enum Dir { DIR_E = 0, DIR_W = 1, DIR_S = 2, DIR_N = 3 };
constexpr uint8_t JOY_E = 0x80, JOY_W = 0x40, JOY_S = 0x20, JOY_N = 0x10;

struct Stamp {
    int x = 0;
    int y = 0;      // 0 = not displayed
    int ix = 0;
    int cset = 2;
    int prior = 0;
};

struct Monster : Stamp {
    int index = 0;
    int stat = 0x80;
    int dir = 0;
    int facing = 8;
    int ghost = 0;
    int speed = 0;
    int ymodc = 0;
    const std::vector<int>* seq = &RPOOKA_SEQ;
    int seqn = 0;
    int mode = MODE_VERT;
    int blwstat = 0;
    int blwctr = 0;
    int blwypos = 0;
    int preblwix = 0;
    int blwlstmv = 0;
    int rocknum = -1;
    bool fygar() const { return index >= 4; }
};

struct Rock : Stamp {
    int tumble = 0;
    int stat = 0;
    int numsqsh = 0;
};

class SoundSink {
public:
    virtual ~SoundSink() {}
    virtual void request(int song) = 0;
};

class Game {
public:
    Game(int numPlayers, bool bonzo, bool attract, SoundSink* snd, int startRack = 1);

    void frame(uint8_t joy = 0xFF, bool button = false);
    std::vector<const Stamp*> getStamps() const;

    // ---- public state read by the renderer / front end
    uint8_t dirtmap[256];
    int score[2] = {0, 0};
    int nummen[2] = {0, 0};
    int racknum[2] = {1, 1};
    int playnum = 0;
    int numplayr = 0;
    int attract = 0;
    int bonzo = 0;
    int nonoise = 0;
    bool gameOver = false;
    std::string message;
    bool messageBlank = false;
    uint8_t vegcol[3] = {0x86, 0x86, 0x0E};
    int dirtScheme = 0;
    int walking = 0, fast = 0, escaper = 0, dethwish = 0, death = 0, squash = 0, entry = 0;
    int rack() const { return racknum[playnum]; }

    // ---- exposed for tests
    Stamp dd;
    Stamp pump[3];
    Stamp flame;
    Stamp fruitStamp;
    Monster monsters[8];
    Rock rocks[5];
    int lastmove = 0;
    int digging = 0;
    int pumpnum = 0;
    int maxpump = 0;
    int pumpie = -1;
    int freeze = 0;
    void initrack();

private:
    // sequences (the blocking GENFREEZ chains of the original)
    enum SeqType { SEQ_DEATH, SEQ_RACKEND, SEQ_PLAYREDY, SEQ_NEWPLAYR };
    struct Seq { int type; int step; int arg; int x; int cnt; };
    std::vector<Seq> seqs;
    void runSeq();
    void seqDeath(Seq& s);
    void seqRackend(Seq& s);
    void seqPlayredy(Seq& s);
    void seqNewplayr(Seq& s);
    void pushSeq(int type, int arg = 0);

    SoundSink* sound;
    uint8_t backdirt[256];
    struct Snapshot {
        bool valid = false;
        int sx[19], sy[19], six[19];
        int tumble[5], rstat[5], numsqsh[5];
        int monstat[8];
        int hitrock, fallcnt, brcreat, brtunnl;
    } back;
    Snapshot snapshot() const;
    void restore(const Snapshot& s);

    // ---- variables (names follow the assembly)
    int frmcnt = 0;
    int effrack[2] = {0, 0};
    int p2init = 0;
    int racktime = 0, speedup = 0, flamwait = 0, ghostout = 0, flee = 0;
    int nxtsec = 0, scndtim = 0, notunnel = 0, fallcnt = 0, brcreat = 0, brtunnl = 0;
    int hitrock = 0, pumpct = 0;
    int proport = PROPORT_INIT, proport2 = PROPORT_INIT, proport3 = PROPORT_INIT;
    int digspeed = DIGFAST, dhorspd = DHFAST;
    int astage = 0, ignore = 0, flamsize = 0, flamnum = 0;
    int gp_dx = 0, gp_dy = 0;
    int rocksnow = 0;
    // the 20 registers cleared by INCREATS
    int animcnt = 0, pumpBtn = 0, notgrid = 0, needdir = 0, fscore = 0, fscorctr = 0;
    int ghostime = 0, pumping = 0, pumpcnt = 0, freezeDd = 0, flamie = 0, flamtime = 0;
    int fruit = 0, digrest = 0, digtemp = 0;

    void initVars();
    void zero20();
    void initGame();
    void afterInitGame();
    void newDlst();
    std::vector<Stamp*> allStamps();
    void noise(int n) { if (sound) sound->request(n); }
    int rnd();
    void secondTimer();

    // score
    void addscore(int pts);

    // Dig Dug
    void ddmove(uint8_t joy);
    void nogo();
    void prezjoy(int a);
    void zjoy(int a);
    int shift8(int& v);
    int shift16(int& v);
    void mddn(int y);
    void mdde(int y);
    void finvert(int a, int y);
    void anim(int y);
    int chkrock(int yi);
    void adjustto(int y, int& tx, int& ty);
    bool chknxtd(int a);
    void digdisp(int a, int y);
    void digger(int i, int c, const int* table);
    int rockpush(int r);
    void finmov();
    void chkfall2(int r, int a);
    void betty();
    void finmov1();
    void setdfast();
    void harp();
    void vanpump();
    void eatveg();
    void dfruit();
    void collison();
    void yescoll();

    // pump
    void inputButton(bool button);
    void pumper(bool button);
    void pumpOff();
    void extend2();
    void disppump();
    static int disney(int a);
    bool pumpcoll();
    int lookdirt(int idx);
    void getmaxp();
    void sub2(int a);
    int getdirtIdx(int x, int y);

    // blow up
    void blowup();
    void puff(Monster& m);
    void carol(Monster& m);

    // rocks
    void fall();
    void rockmov(int x);
    void endstat2(Rock& r);
    void rtotter(Rock& r);
    void rfall(Rock& r, int x);
    void crush(Rock& r, int x);
    static bool crunch(const Rock& r, int vx, int vy, int& ny);
    void rbottom(Rock& r, int x);
    void rfinish(Rock& r, int x);

    // rack flow
    bool chkrack();
    bool testrackCondition();
    void ddmiddle();
    void hcarve(int x);
    void initdirt();
    void dostart();
    void initrock();
    void goAttract();

    // monsters (monsters.cpp)
    void monstersFrame(bool button);
    int mshift(Monster& m);
    void movemons(Monster& m);
    void dispchar(Monster& m);
    int getdirt(const Monster& m, int& idx);
    int chkother(int c, int idx);
    int norev(const Monster& m);
    void movemode(Monster& m);
    void movecont(Monster& m);
    void mode(Monster& m, int a);
    int getpoint(int x0, int x1, int y0, int y1);
    int sub11(const Monster& m);
    bool sub1(Monster& m, int a, int& r);
    void pick(Monster& m, int r, const int* order);
    void digvert(Monster& m, int a);
    void dighoriz(Monster& m, int a);
    void digdiag(Monster& m, int a);
    void escont(Monster& m, int r);
    void escape(Monster& m, int a);
    void scram(Monster& m);
    void offscr(Monster& m);
    void offscr2(Monster& m);
    void ghoster(Monster& m, int a);
    void ghoster2(Monster& m, int d);
    void znxt9(Monster& m, int a, int ghosttmp, int v);
    void ghostonx(Monster& m);
    void ghostony(Monster& m);
    void newseq(Monster& m, int d);
    void speeder(Monster& m);
    void avoid();
    void getmode();
    void fruiter();
    void setflam();
    void flamout4(Monster& m);
    void flamer();
    void flamout2(Monster& m);
    void flamcoll(Monster& m);
};

}  // namespace dd
