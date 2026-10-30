#include "game.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace dd {

static const int JOYSWCH[4] = {0x7F, 0xBF, 0xDF, 0xEF};
static const int REVSWCH[4] = {0xBF, 0x7F, 0xEF, 0xDF};
static const int LOWSCOR = 178;

// ------------------------------------------------------------------ init ---
Game::Game(int numPlayers, bool bonzo_, bool attract_, SoundSink* snd, int startRack)
    : sound(snd) {
    numplayr = numPlayers > 1 ? 1 : 0;
    bonzo = bonzo_ ? 1 : 0;
    attract = attract_ ? 1 : 0;
    racknum[0] = racknum[1] = startRack;
    std::memset(dirtmap, 0, sizeof dirtmap);
    std::memset(backdirt, 0, sizeof backdirt);
    for (int i = 0; i < 8; ++i) monsters[i].index = i;
    for (int i = 0; i < 5; ++i) { rocks[i].ix = STRTROCK; rocks[i].prior = 1; }
    for (auto& p : pump) p.prior = 1;
    fruitStamp.prior = 1;   // ZPRIOR1: drawn behind Dig Dug and the creatures
    fruitStamp.cset = 1;
    entry = attract ? 0 : 1;
    initVars();
    initGame();
}

void Game::initVars() {
    lastmove = 0; death = 0; dethwish = 0; racktime = 0; speedup = 0; flamwait = 0;
    ghostout = 0; flee = 0; fast = 0; nxtsec = 0; scndtim = 0; notunnel = 0; fallcnt = 0;
    brcreat = 0; brtunnl = 0; hitrock = 0; maxpump = 0; pumpct = 0;
    proport = proport2 = proport3 = PROPORT_INIT;
    digspeed = DIGFAST; dhorspd = DHFAST;
    astage = 0; ignore = 0; flamsize = 0; flamnum = 0; pumpie = -1; gp_dx = gp_dy = 0;
    zero20();
}

void Game::zero20() {
    animcnt = digging = pumpnum = squash = notgrid = needdir = fscore = fscorctr = 0;
    ghostime = escaper = pumpBtn = pumping = pumpcnt = walking = freezeDd = flamie = 0;
    flamtime = fruit = digrest = scndtim = digtemp = 0;
}

void Game::initGame() {
    p2init = 0; playnum = 0; death = 0; dethwish = 0; ignore = 0;
    for (Stamp* s : allStamps()) s->y = 0;
    if (!attract && numplayr) {
        pushSeq(SEQ_PLAYREDY, 1);   // arg 1: run afterInitGame when done
    } else {
        afterInitGame();
    }
}

void Game::afterInitGame() {
    initrack();
    if (attract) {
        nummen[0] = 2; nummen[1] = 0; astage = 0;
    } else {
        nummen[0] = 5; nummen[1] = numplayr ? 5 : 0;
        score[0] = score[1] = 0;
    }
    newDlst();
}

void Game::newDlst() {
    nonoise = attract;
    freeze = 40;
}

std::vector<Stamp*> Game::allStamps() {
    std::vector<Stamp*> v;
    v.push_back(&dd);
    for (auto& m : monsters) v.push_back(&m);
    for (auto& r : rocks) v.push_back(&r);
    v.push_back(&flame);
    for (auto& p : pump) v.push_back(&p);
    v.push_back(&fruitStamp);
    return v;
}

std::vector<const Stamp*> Game::getStamps() const {
    std::vector<const Stamp*> all;
    all.push_back(&dd);
    for (auto& m : monsters) all.push_back(&m);
    for (auto& r : rocks) all.push_back(&r);
    all.push_back(&flame);
    for (auto& p : pump) all.push_back(&p);
    all.push_back(&fruitStamp);
    std::vector<const Stamp*> out;
    for (int prior = 1; prior >= 0; --prior)
        for (auto* s : all)
            if (s->prior == prior && s->y != 0) out.push_back(s);
    return out;
}

int Game::rnd() { return std::rand() & 0xFF; }

// ----------------------------------------------------------------- score ---
void Game::addscore(int pts) {
    if (attract || entry) return;
    int x = playnum;
    int old = score[x];
    int nw = std::min(old + pts, 999990);
    score[x] = nw;
    if (nw / 10000 != old / 10000) {
        int h = nw / 10000;
        if (h == 2 || h % 10 == 0 || h % 10 == 5) {
            if (nummen[x] < 10) {
                nummen[x]++;
                noise(SNG11);
            }
        }
    }
}

// ------------------------------------------------------------- per frame ---
void Game::frame(uint8_t joy, bool button) {
    frmcnt = (frmcnt + 1) & 0xFF;
    secondTimer();
    if (freeze > 0) {
        freeze--;
        return;
    }
    if (!seqs.empty()) {
        runSeq();
        return;
    }
    racktime = (racktime + 1) & 0xFF;
    if (death) {
        pushSeq(SEQ_DEATH);
        return;
    }
    if (chkrack()) return;
    if (!dethwish) {
        ddmove(joy);
        eatveg();
        monstersFrame(button);
        collison();
        blowup();
    }
    fall();
}

void Game::secondTimer() {
    if (frmcnt == nxtsec) {
        nxtsec = (nxtsec + 60) & 0xFF;
        scndtim = (scndtim + 1) & 0xFF;
        if (speedup) speedup--;
        if (ghostout) ghostout--;
    }
}

// --------------------------------------------------------------- Dig Dug ---
void Game::ddmove(uint8_t joy) {
    digtemp = digging;
    if (attract) {
        if (squash) { nogo(); return; }
        int x = lastmove;
        if (digrest == 0 && astage != 0) {
            astage--;
            prezjoy(JOYSWCH[x]);
            return;
        }
        int r = rnd();
        astage = (r & 3) + (std::rand() & 1);
        int y = r & 3;
        while (JOYSWCH[y] == REVSWCH[x]) y = (y + 1) & 3;
        prezjoy(JOYSWCH[y]);
        return;
    }
    if (entry) {
        if (dd.x == DDSTRTX && dd.y == DDSTRTY) {
            nonoise = 0;
            noise(SNG15);
        }
        if (dd.y != DDSTRTY) {
            if (dd.y == DDMIDLY) {
                dd.ix = RDIGDUG[1];
                entry = 0;
                dostart();
                freeze = 40;
                return;
            }
            prezjoy(0xDF);
            return;
        }
        if (dd.x != DDMIDLX) { prezjoy(0xBF); return; }
        prezjoy(0xDF);
        return;
    }
    if (squash || freezeDd) { nogo(); return; }
    int a = (joy | 0x0F) & 0xFF;
    if (a == 0xFF) {
        digrest = (digrest + 1) & 0xFF;
        nogo();
        return;
    }
    walking = 10;
    int code;
    if ((a & 0xC0) == 0 || (a & 0x30) == 0) code = 0xFF;
    else if ((a & 0x90) == 0) code = 0x7F;
    else if ((a & 0x10) == 0) code = 0xEF;
    else if ((a & 0x40) == 0) code = 0xBF;
    else if ((a & 0x20) == 0) code = 0xDF;
    else code = 0x7F;
    prezjoy(code);
}

void Game::nogo() {
    if (walking) walking--;
}

void Game::prezjoy(int a) {
    digrest = (digrest + 1) & 0xFF;
    zjoy(a);
}

void Game::zjoy(int a) {
    int y;
    for (y = 0; y < 4; ++y)
        if (!(a & (0x80 >> y))) break;
    if (y == 4) { nogo(); return; }
    if (y >= 2) mddn(y);
    else mdde(y);
}

int Game::shift8(int& v) {
    int c = (v >> 7) & 1;
    v = ((v << 1) & 0xFF) | c;
    return c;
}

int Game::shift16(int& v) {
    int c = (v >> 15) & 1;
    v = ((v << 1) & 0xFFFF) | c;
    return c;
}

void Game::mddn(int y) {
    if (dd.x & 7) { zjoy(JOYSWCH[lastmove]); return; }
    int a = JOYSWCH[lastmove];
    if (!shift8(digspeed)) {
        if (a != REVSWCH[y]) { digrest = 0; return; }
    }
    lastmove = y;
    if (a == JOYSWCH[y]) {
        if (shift8(proport3)) anim(y);
    } else {
        digtemp = 0;
        anim(y);
    }
    int r = chkrock(y);
    if (r >= 0) {
        if (!(y == DIR_S && rocks[r].tumble >= 7)) {
            digtemp = 0;
            finmov1();
            return;
        }
    }
    int ny = dd.y;
    if (y == DIR_S) {
        if (ny == MINDDY) { digtemp = 0; finmov1(); return; }
        ny--;
    } else {
        ny++;
        if (ny >= TOPZONE[15]) {
            dd.ix = DANIMS[0];
            dd.y = 191;
            lastmove = 0;
            return;
        }
    }
    dd.y = ny;
    finvert(ny % 12, y);
}

void Game::mdde(int y) {
    if (!rowhere(dd.y)) { zjoy(JOYSWCH[lastmove]); return; }
    int a = JOYSWCH[lastmove];
    if (!shift16(dhorspd)) {
        if (a != REVSWCH[y]) { digrest = 0; return; }
    }
    lastmove = y;
    if (a != JOYSWCH[y]) digtemp = 0;
    anim(y);
    if (chkrock(y) >= 0) { digtemp = 0; finmov1(); return; }
    int nx = dd.x;
    if (y == DIR_W) {
        if (nx == MINDDX) { digtemp = 0; finmov1(); return; }
        nx--;
    } else {
        if (nx == MAXDDX) { digtemp = 0; finmov1(); return; }
        nx++;
    }
    dd.x = nx;
    finvert(nx & 7, y);
}

void Game::finvert(int a, int y) {
    notgrid = 1;
    digdisp(a, y);
    finmov();
}

void Game::anim(int y) {
    animcnt = animcnt > 0 ? animcnt - 1 : 2;
    dd.ix = DANIMS[animcnt * 4 + y];
}

int Game::chkrock(int yi) {
    int tx = (dd.x + XTOPL[4 + yi]) & 0xFF;
    int ty = (dd.y + YTOPL[4 + yi]) & 0xFF;
    for (int x = 4; x >= 0; --x) {
        const Rock& r = rocks[x];
        if (tx != r.x) continue;
        if (r.ix >= LOWSCOR) continue;
        if (ty == r.y) return x;
        if (r.tumble == 0) continue;
        if (!(lastmove & 2)) continue;
        int d = r.y + 7 - ty;
        if (d < 0 || d >= 17) continue;
        if (dd.y == MINDDY) return x;
        int c = dirtmap[calcindx(tx, dd.y - 12)];
        if (c == 36 || c == 44) return x;
    }
    return -1;
}

void Game::adjustto(int y, int& tx, int& ty) {
    tx = (dd.x + XTOPL[y]) & 0xFF;
    ty = (dd.y + YTOPL[y]) & 0xFF;
}

bool Game::chknxtd(int a) {
    int y = a + lastmove;
    int tx, ty;
    adjustto(y, tx, ty);
    int c = dirtmap[calcindx(tx, ty)];
    if (c < 36) {
        digtemp = 0;
        return false;
    }
    return true;
}

void Game::digdisp(int a, int y) {
    if (a == VPRE1[y] || a == VPRE2[y] || a == VPRE3[y]) {
        chknxtd(12);
        return;
    }
    if (a == VONE[y]) {
        int fx = (dd.x + XFROMPL[y]) & 0xFF;
        int fy = (dd.y + YFROMPL[y]) & 0xFF;
        int i = calcindx(fx, fy);
        int c = dirtmap[i];
        if (c < 36) dirtmap[i] = FROMXFM[y][c >> 1];
        digtemp = 0;
        int tx, ty;
        adjustto(y, tx, ty);
        i = calcindx(tx, ty);
        c = dirtmap[i];
        if (c < 36) dirtmap[i] = TOXFM[y][c >> 1];
        else digger(i, c, ENTRXFM[y]);
        return;
    }
    if (a == VHALF[y]) {
        digtemp = 0;
        int tx, ty;
        adjustto(y, tx, ty);
        int i = calcindx(tx, ty);
        int c = dirtmap[i];
        if (c >= 36) digger(i, c, HALFXFM[y]);
        return;
    }
    if (a == VBOUND[y]) {
        if (rowhere(dd.y) && (dd.x & 7) == 0) {
            if (!chknxtd(4)) notgrid = 0;
        }
    }
}

void Game::digger(int i, int c, const int* table) {
    int k = (c - 36) >> 1;
    if (k < 0 || k >= 7) return;
    int nw = table[k];
    dirtmap[i] = nw;
    if (nw < 30) {
        addscore(10);
        if (c < 46) digtemp = 1;
    } else if (nw != c) {
        digtemp = 1;
    }
}

int Game::rockpush(int r) {
    const Rock& rk = rocks[r];
    return dirtmap[calcindx(rk.x, (rk.y - 12) & 0xFF)];
}

void Game::finmov() {
    digrest = 0;
    if (needdir != 0) {
        if (needdir < 0 || needdir - 1 == lastmove) {
            rocks[hitrock].tumble = ONEDGE;
            needdir = 0;
        }
    }
    int r = chkrock(4 + lastmove);
    if (r >= 0) {
        chkfall2(r, CVTDIR[lastmove]);
    } else {
        r = chkrock(3);
        if (r >= 0) chkfall2(r, -1);
        else betty();
    }
    finmov1();
}

void Game::chkfall2(int r, int a) {
    if (rocks[r].tumble) return;
    needdir = a;
    hitrock = r;
}

void Game::betty() {
    int r = chkrock(12 + lastmove);
    if (r < 0 || rocks[r].tumble) return;
    if (rockpush(r) < 36) chkfall2(r, CVTDIR[lastmove]);
}

void Game::finmov1() {
    if (digtemp != digging) {
        digging = digtemp;
        if (digging) {
            dhorspd = DHSLOW;
            digspeed = DIGSLOW;
        } else {
            setdfast();
        }
    }
    if (digging) harp();
    else vanpump();
}

void Game::setdfast() {
    dhorspd = DHFAST;
    digspeed = DIGFAST;
}

void Game::harp() {
    int lm = lastmove;
    Stamp& p = pump[0];
    p.ix = HARPSTMP[lm];
    p.x = (dd.x + HARPXPL[lm]) & 0xFF;
    int py = dd.y + HARPYPL[lm];
    if (py < MINDDY) vanpump();
    else p.y = py;
}

void Game::vanpump() {
    pumpie = -1;
    for (auto& p : pump) p.y = 0;
}

// ------------------------------------------------------------ vegetables ---
void Game::eatveg() {
    if (fscore) {
        fscorctr--;
        if (fscorctr) return;
        fscore = 0;
        fruitStamp.y = 0;
    }
    if (fruit < 0x0F) return;
    if (dd.y != 0x6B || dd.x != 0x38) return;
    fscore = 1;
    fruit = 0;
    fscorctr = 0x30;
    int x = 0;
    if (!bonzo) x = std::min(rack(), 18);
    fruitStamp.ix = FPOINTS[x];
    fruitStamp.cset = 2;
    fruitStamp.x = 53;
    addscore(VEGPTS[x]);
    noise(SNG10);
}

void Game::dfruit() {
    int y = 0;
    if (!bonzo) y = std::min(rack(), 0x12);
    fruitStamp.ix = FRUITAB[y];
    fruitStamp.cset = 1;
    fruitStamp.x = 56;
    fruitStamp.y = 107;
}

// ------------------------------------------------------------ collisions ---
void Game::collison() {
    if (squash) return;
    for (int x = 7; x >= 0; --x) {
        const Monster& m = monsters[x];
        if (m.stat >= 2) continue;
        if (std::abs(dd.x - m.x) >= 5) continue;
        if (std::abs(dd.y - m.y) >= 7) continue;
        yescoll();
        return;
    }
}

void Game::yescoll() {
    dethwish = 1;
    dd.ix = ADEATH[DANIMIX[lastmove]];
    vanpump();
    noise(SNG8);
}

// ------------------------------------------------------------------ pump ---
void Game::inputButton(bool button) {
    if (!button) {
        pumpcnt = 0; pumping = 0; pumpBtn = 0;
        return;
    }
    if (pumpcnt == 0) {
        pumpBtn = 1;
        pumpcnt = 1;
        if (pumpie != -1 && pumpct == 0) pumpct = 0x10;
        return;
    }
    pumpcnt++;
    if (pumpcnt == 0x18) {
        pumping = 1;
        pumpcnt = 1;
    } else {
        pumping = 0;
        pumpBtn = 0;
    }
}

void Game::pumper(bool button) {
    inputButton(button);
    if (pumpct) {
        pumpct--;
        if (pumpct == 0 && pumpie != -1) {
            pumping = 1;
            pumpcnt = 1;
        }
    }
    if (pumpnum == 0) {
        if (digging || squash || !pumpBtn) return;
        freezeDd = 1;
        noise(SNG5);
        getmaxp();
        extend2();
        return;
    }
    if (pumpnum < 0) {
        int x = pumpie;
        if (x != -1) {
            const Monster& m = monsters[x];
            if (!(m.stat & 0xC0) && (m.stat & 4)) {
                if (pumpcnt) return;
                freezeDd = 0;
                if (walking == 0) return;
            }
        }
        pumpOff();
        return;
    }
    if (maxpump == 1) { pumpOff(); return; }
    extend2();
}

void Game::pumpOff() {
    freezeDd = 0;
    pumpnum = 0;
    vanpump();
}

void Game::extend2() {
    disppump();
    if (pumpcoll()) {
        pumpnum = -1;
        return;
    }
    int a = pumpnum;
    pumpnum++;
    if (a == maxpump && maxpump != 1) pumpOff();
}

void Game::disppump() {
    int lm = lastmove;
    Stamp& p0 = pump[0];
    Stamp& p1 = pump[1];
    Stamp& p2 = pump[2];
    if (lm < 2) {
        int t1 = (pumpnum & 1) ? 2 : 0;
        if (lm == DIR_E) {
            p0.x = (dd.x + t1 + 6) & 0xFF;
            p0.y = dd.y;
            p0.ix = RPUMP[pumpnum >> 1];
        } else {
            int t2 = pumpnum >> 1;
            p0.x = (dd.x - t1 - 2 - t2 * 4) & 0xFF;
            p0.y = dd.y;
            p0.ix = LPUMP[t2];
        }
        p1.ix = ZBLANK;
        p2.ix = ZBLANK;
        return;
    }
    int a;
    if (lm == DIR_N) {
        int t2 = pumpnum >> 1;
        p0.ix = PUPS0[t2]; p1.ix = PUPS1[t2]; p2.ix = PUPS2[t2];
        a = pumpnum * 3 + dd.y + 3;
        if (a >= 192) a = 191;
    } else {
        int t1 = (pumpnum & 1) ? 3 : 0;
        int t2 = pumpnum >> 1;
        p0.ix = PDOWNS0[t2]; p1.ix = PDOWNS1[t2]; p2.ix = PDOWNS2[t2];
        a = dd.y - t1 - 9;
        if (p0.ix == DPUMP6) a += 5;
        if (a < 23) a = 23;
    }
    p0.y = a;
    if (p1.ix == DPUMP6) a += 6;
    a = disney(a);
    p1.y = a;
    if (p2.ix == DPUMP6) a += 6;
    a = disney(a);
    p2.y = a;
    for (auto& p : pump) p.x = (dd.x + 2) & 0xFF;
}

int Game::disney(int a) {
    a -= 12;
    return a < 23 ? 23 : a;
}

bool Game::pumpcoll() {
    int lm = lastmove;
    const Stamp& p0 = pump[0];
    int t1, t2;
    if (lm < 2) {
        if (lm == DIR_E) t1 = (pumpnum * 2 + dd.x + 1) & 0xFF;
        else t1 = (p0.x - 5) & 0xFF;
        t2 = (p0.y - 4) & 0xFF;
    } else {
        if (lm == DIR_N) t2 = p0.y;
        else t2 = (dd.y - pumpnum * 3 - 9) & 0xFF;
        t1 = (p0.x - 5) & 0xFF;
    }
    for (int x = 7; x >= 0; --x) {
        Monster& m = monsters[x];
        if (m.stat & 0xC0) continue;
        if ((m.stat & 0x0C) == 4) continue;
        if ((m.stat & 6) == 2) continue;
        int d = m.x - t1;
        if (d < 0 || d >= 6) continue;
        int my = ((m.stat & 2) && m.blwypos) ? m.blwypos : m.y;
        d = my - t2;
        if (d < 0 || d >= 8) continue;
        pumpie = x;
        if (x == flamie) flamout4(m);
        m.stat = 7;
        pumping = 1;
        return true;
    }
    return pumping != 0;
}

int Game::lookdirt(int idx) {
    int c = dirtmap[idx & 0xFF];
    if (c >= CHARD12) return 0x0F;
    if (c >= CHARDF) return 2;
    return c >> 1;
}

int Game::getdirtIdx(int x, int y) {
    int z = ((y - 11) >> 2) / 3;
    if (z > 15) z = 15;
    if (z < 0) z = 0;
    int row = 15 - z;
    return (row * 16 + ((x & 0xFF) >> 3)) & 0xFF;
}

void Game::getmaxp() {
    maxpump = 0;
    pump[0].x = dd.x;
    pump[0].y = dd.y;
    int idx = getdirtIdx(dd.x, dd.y);
    int lm = lastmove;
    int x = 2;
    if (lm == DIR_E) {
        if (notgrid == 0) maxpump += 4;
        while (x >= 0) {
            idx++;
            if (lookdirt(idx) & 2) break;
            maxpump += 4;
            x--;
        }
        int a = dd.x & 7;
        if (a) a = ((a - 1) >> 1) ^ 3;
        sub2(a);
        return;
    }
    if (lm == DIR_W) {
        if (notgrid == 0) {
            maxpump += 4;
            idx--;
            x--;
        }
        while (x >= 0) {
            if (lookdirt(idx) & 8) break;
            maxpump += 4;
            idx--;
            x--;
        }
        sub2((dd.x & 7) >> 1);
        return;
    }
    if (lm == DIR_N) {
        if (notgrid == 0) maxpump += 4;
        while (x >= 0) {
            idx -= 16;
            if (lookdirt(idx) & 1) break;
            maxpump += 4;
            x--;
        }
        int a = (dd.y + 1) % 12;
        if (a) a = (12 - a) & 0xFF;
        sub2(a / 3);
        return;
    }
    if (notgrid == 0) {
        maxpump += 4;
        idx += 16;
        x--;
    }
    while (x >= 0) {
        if (lookdirt(idx) & 4) break;
        maxpump += 4;
        idx += 16;
        x--;
    }
    int a = (dd.y + 1) % 12;
    sub2(a / 3);
}

void Game::sub2(int a) {
    int v = a + maxpump;
    if (v >= 12) v = 11;
    maxpump = v;
}

// --------------------------------------------------------------- blow up ---
void Game::blowup() {
    for (int x = NUMCREAT; x >= 0; --x) {
        Monster& m = monsters[x];
        if (m.stat & 0x80) continue;
        int y = m.blwstat;
        if (m.stat & 2) {
            if (y == 5) {
                if (--m.blwctr) continue;
                offscr2(m);
                getmode();
                continue;
            }
            if (y == 4) {
                if (--m.blwctr) continue;
                carol(m);
                continue;
            }
        }
        if (pumping && x == pumpie) {
            if (m.blwstat == 0) {
                m.preblwix = m.ix;
                m.blwypos = m.y;
                m.y = std::min(m.y + 3, 191);
            }
            m.blwstat++;
            m.blwctr = 0x20;
            puff(m);
            noise(SNG4);
            if (m.blwstat != 4) continue;
            noise(SNG6);
            m.stat &= ~4;
            m.blwlstmv = lastmove;
            m.blwctr = 30;
            vanpump();
            continue;
        }
        if (!(m.stat & 2)) continue;
        if (--m.blwctr) continue;
        m.blwctr = 0x20;
        m.blwstat--;
        if (m.blwstat) {
            puff(m);
            continue;
        }
        m.stat &= ~7;
        m.ix = m.preblwix;
        m.y = m.blwypos;
        if (x == pumpie) vanpump();
    }
}

void Game::puff(Monster& m) {
    int i = (m.blwstat - 1) + ((m.facing & 8) ? 4 : 0) + (m.fygar() ? 8 : 0);
    m.ix = BLWSTMPS[i & 15];
}

void Game::carol(Monster& m) {
    int a = m.blwypos - 4;
    int y = 0;
    while (a >= 12) { a -= 12; y++; }
    y >>= 2;
    if (m.fygar() && !(m.blwlstmv & 2)) y += 4;
    m.ix = BPOINTS[y & 7];
    if (m.x < 8) m.x = 8;
    m.y = m.blwypos;
    m.prior = 1;
    addscore(BURSTPTS[y & 7]);
    m.blwctr = 30;
    m.blwstat++;
}

// ----------------------------------------------------------------- rocks ---
void Game::fall() {
    rocksnow = 0;
    for (int x = NUMROCK; x >= 0; --x) rockmov(x);
    if (rocksnow == 0 && dethwish) {
        dethwish = 0;
        death = 1;
    }
}

void Game::rockmov(int x) {
    Rock& r = rocks[x];
    if (r.tumble < ONEDGE) return;
    rocksnow++;
    int t = r.tumble;
    if (t == 2) {
        noise(SNG7);
        endstat2(r);
    } else if (t == 3) {
        rtotter(r);
    } else if (t == 4) {
        rfall(r, x);
    } else if (t == 5) {
        rbottom(r, x);
    } else if (t == 6 || t == 7) {
        r.ix = RKANIM[t - 6];
        r.stat++;
        if (r.stat >= RKTIM[t - 6]) endstat2(r);
    } else if (t == 8) {
        if (r.numsqsh == 0) {
            endstat2(r);
        } else {
            r.ix = DISPTS[std::min(r.numsqsh, 8) - 1];
            r.stat++;
            if (r.stat >= RKTIM[2]) endstat2(r);
        }
    } else {
        rfinish(r, x);
    }
}

void Game::endstat2(Rock& r) {
    r.stat = 0;
    r.tumble++;
}

void Game::rtotter(Rock& r) {
    r.ix = (r.stat & 8) ? STRTROCK : TOTRROCK;
    r.stat++;
    if (r.stat >= 0x19) { endstat2(r); return; }
    if (r.stat < 8) return;
    if (r.stat == 0x18) flee = 0x20;
    avoid();
}

void Game::rfall(Rock& r, int x) {
    if (r.y == MINDDY) { endstat2(r); return; }
    r.y -= RFALLTBL[r.stat % 7];
    if (r.stat >= 6) {
        int off = r.y % 12;
        if (off == 11) {
            int idx = calcindx(r.x, (r.y - 12) & 0xFF);
            int c = dirtmap[idx];
            if (idx >= 0xF0) { endstat2(r); return; }
            if (c >= 36 && c != 48 && c != 44) { endstat2(r); return; }
        } else if (off == 4) {
            int idx = calcindx(r.x, (r.y - 12) & 0xFF);
            int c = dirtmap[idx];
            if (c >= 36) {
                if (c != 48) { endstat2(r); return; }
                dirtmap[idx] = 20;
            }
        } else if (off == 10) {
            int idx = calcindx(r.x, (r.y - 12) & 0xFF);
            int c = dirtmap[idx];
            if (c < 36) dirtmap[idx] = VERTBOT[c >> 1];
            idx = calcindx(r.x, r.y);
            c = dirtmap[idx];
            if (c < 36) dirtmap[idx] = VERTTOP[c >> 1];
        }
    }
    crush(r, x);
}

void Game::crush(Rock& r, int x) {
    for (int y = NUMCREAT; y >= 0; --y) {
        Monster& m = monsters[y];
        if ((m.stat & 0x80) || (m.stat & 0x86) == 2) continue;
        int vy = ((m.stat & 6) == 6) ? m.blwypos : m.y;
        int ny;
        if (!crunch(r, m.x, vy, ny)) continue;
        m.y = ny;
        if (!(m.stat & 0x40)) {
            m.rocknum = x;
            noise(SNG1);
        }
        m.stat = (m.stat & 0xF9) | 0x41;
        m.ix = m.fygar() ? SQFYGAR : SQPOOKA;
        if (y == flamie) {
            flamie = 0;
            flame.y = 0;
        }
        if (y == pumpie) vanpump();
    }
    int ny;
    if (crunch(r, dd.x, dd.y, ny)) {
        dd.y = ny;
        vanpump();
        if (!squash) noise(SNG1);
        squash = 1;
        dd.ix = SQDIGDUG;
    }
    r.stat++;
    avoid();
}

bool Game::crunch(const Rock& r, int vx, int vy, int& ny) {
    if (std::abs(r.x - vx) >= 6) return false;
    int d = r.y + 6 - vy;
    if (d < 0 || d >= 17) return false;
    int t = r.y - 5;
    if (t < MINDDY) { ny = MINDDY; return true; }
    if (t - vy < 4) { ny = t; return true; }
    ny = vy;
    return true;
}

void Game::rbottom(Rock& r, int x) {
    flee = 0;
    r.stat++;
    if (r.stat < 0x10) {
        if (escaper) return;
        if (r.stat == 1) getmode();
        return;
    }
    endstat2(r);
    r.numsqsh = 0;
    for (int y = NUMCREAT; y >= 0; --y) {
        Monster& m = monsters[y];
        if ((m.stat & 0x40) && m.rocknum == x) {
            r.numsqsh++;
            m.y = 0;
        }
    }
    noise(SNG2);
}

void Game::rfinish(Rock& r, int x) {
    r.tumble = 0;
    r.y = 0;
    rocksnow--;
    fallcnt++;
    if (fallcnt == 2) fruit = 1;
    if (squash) {
        squash = 0;
        dethwish = 1;
    }
    for (int y = NUMCREAT; y >= 0; --y) {
        Monster& m = monsters[y];
        if (m.rocknum == x && (m.stat & 0x40)) m.stat = 0x80;
    }
    int n = r.numsqsh;
    if (n) addscore(FLATPTS[std::min(n, 8) - 1]);
    getmode();
}

// ------------------------------------------------------------- rack flow ---
bool Game::chkrack() {
    if (testrackCondition()) {
        pushSeq(SEQ_RACKEND);
        return true;
    }
    return false;
}

bool Game::testrackCondition() {
    if (dethwish || squash) return false;
    for (auto& m : monsters)
        if (!(m.stat & 0x80)) return false;
    return true;
}

void Game::ddmiddle() {
    dd.y = DDMIDLY;
    dd.x = DDMIDLX;
    dd.ix = RDIGDUG[0];
}

void Game::initrack() {
    for (auto& m : monsters) { m.stat = 0; m.y = 0; m.prior = 0; }
    flame.y = 0;
    fruitStamp.y = 0;
    for (auto& p : pump) p.y = 0;
    for (auto& r : rocks) r.y = 0;
    notunnel = 0;
    fallcnt = 0;
    racktime = 0;
    initdirt();
    initrock();
    if (entry) {
        dd.x = DDSTRTX;
        dd.y = DDSTRTY;
        dd.ix = LDIGDUG[0];
    } else {
        ddmiddle();
        for (int row = 1; row < 7; ++row) dirtmap[row * 16 + 7] = 20;
        dirtmap[7 * 16 + 7] = 8;
    }
    int rk = rack();
    int big = rk / 10, small = rk % 10;
    int y = 0x0E;
    for (int i = 0; i < big; ++i) {
        dirtmap[y] = BIGFLOWR;
        if (--y == 0) return;
    }
    for (int i = 0; i < small; ++i) {
        dirtmap[y] = SMLFLOWR;
        if (--y == 0) return;
    }
    while (y > 0) dirtmap[y--] = FULLSKY;
}

void Game::hcarve(int x) {
    dirtmap[x & 0xFF] = DLEFTEND;
    dirtmap[(x + 1) & 0xFF] = DHMIDDLE;
    dirtmap[(x + 2) & 0xFF] = DRHTEND;
}

void Game::initdirt() {
    int px = playnum;
    if (!notunnel) {
        for (int i = 0x10; i < 0xF0; ++i) dirtmap[i] = FULLDIRT;
        hcarve(0x76);
        for (int row = 1; row < 15; ++row) {
            dirtmap[row * 16] = BLANKL;
            dirtmap[row * 16 + 15] = BLANKR;
        }
        dirtmap[0xEF] = BLANKR;
        dirtmap[0] = LSKYHALF;
        dirtmap[15] = RSKYHALF;
        for (int i = 0xF0; i < 0x100; ++i) dirtmap[i] = 0;
        int eff;
        if (attract) {
            eff = 0;
        } else {
            int rk = racknum[px];
            if (rk == 0) rk = racknum[px] = 1;
            int y = rk - 1;
            if (bonzo) eff = std::min(y + 15, MAXEFFR);
            else if (y < 15) eff = y;
            else eff = (rk & 3) + 11;
        }
        effrack[px] = eff;
    }
    int eff = effrack[px];
    int rcreats, rtunnels;
    const std::vector<int>* tun;
    if (eff >= MAXEFFR) {
        int x;
        if (notunnel) {
            rcreats = RANDCRET[brcreat];
            rtunnels = brtunnl;
            x = brcreat;
        } else {
            x = frmcnt & 7;
            rcreats = RANDCRET[x];
            brcreat = x;
            rtunnels = rnd();
            brtunnl = rtunnels;
        }
        tun = &PTUNNELS[x + 17];
    } else {
        rcreats = CREATS[eff];
        rtunnels = TUNLDIR[eff];
        tun = &PTUNNELS[eff];
    }
    int tunnels[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    for (size_t i = 0; i < tun->size() && i < 8; ++i) tunnels[i] = (*tun)[i];
    int pcount = 0, fcount = 4;
    for (int i = 0; i < 8; ++i) {
        int t = tunnels[i];
        bool vertical = (rtunnels & 0x80) != 0;
        rtunnels = (rtunnels << 1) & 0xFF;
        bool isFygar = (rcreats & 0x80) != 0;
        rcreats = (rcreats << 1) & 0xFF;
        int cx = isFygar ? fcount++ : pcount++;
        Monster& m = monsters[cx];
        if (t == 0) {
            m.y = 0;
            m.stat = 0x80;
            continue;
        }
        if (!notunnel) {
            if (vertical) {
                dirtmap[t] = DTOPEND;
                dirtmap[(t + 0x10) & 0xFF] = DVMIDDLE;
                dirtmap[(t + 0x20) & 0xFF] = DBOTEND;
            } else {
                hcarve(t);
            }
        }
        int x0, y0;
        getrxy(t, x0, y0);
        if (vertical) {
            y0 -= CRYOFST[cx];
            if (y0 < 0) y0 = 0;
        }
        m.y = y0;
        if (y0 == 0) {
            m.stat = 0x80;
            continue;
        }
        if (!vertical) x0 += CRXOFST[cx];
        m.x = x0 & 0xFF;
        if (m.stat & 0x80) m.y = 0;
        else m.stat = 0;
    }
    int vx;
    if (bonzo) {
        vx = NUMVEG + 1;
    } else {
        vx = racknum[px];
        if (vx) vx = std::min(vx - 1, NUMVEG);
    }
    vegcol[0] = VEGCOL1[vx];
    vegcol[1] = VEGCOL2[vx];
    vegcol[2] = VEGCOL3[vx];
    fruitStamp.cset = 1;
    int rk = racknum[px];
    int x = rk ? rk - 1 : 0;
    if (rk <= 1 && !notunnel && (attract || !bonzo)) {
        dirtmap[0x2C] = DHMIDDLE;
        dirtmap[0xA5] = DHMIDDLE;
        dirtmap[0x2D] = DRHTEND;
        dirtmap[0xA6] = DRHTEND;
        dirtmap[0x42] = dirtmap[0x52] = dirtmap[0xBA] = dirtmap[0xCA] = DVMIDDLE;
        dirtmap[0x62] = DBOTEND;
        dirtmap[0xDA] = DBOTEND;
    }
    dirtScheme = (x % 12) & 0xC;
    // INCREATS
    dostart();
    dd.prior = 0;
    for (auto& m : monsters) m.prior = 0;
    for (auto& m : monsters) {
        m.blwstat = 0;
        m.ghost = 0;
        m.ymodc = 0;
        m.facing = 8;
        int idx;
        int c = getdirt(m, idx);
        int t = chkother(c, idx) >> 1;
        int d = 4;
        if (t & 1) {
            d = 8;
            if (t & 2) {
                d = 1;
                if (t & 4) {
                    d = 2;
                    if (t & 8) d = 0;
                }
            }
        }
        m.dir = d;
        newseq(m, d);
        speeder(m);
    }
    getmode();
}

void Game::dostart() {
    nxtsec = (frmcnt + 60) & 0xFF;
    int r = bonzo ? 1 : rack();
    speedup = r < 24 ? 67 : 2;
    setdfast();
    zero20();
    pumpie = -1;
    flee = 0;
    fast = 0;
    lastmove = 0;
    proport = proport2 = proport3 = PROPORT_INIT;
    ghostout = 4;
}

void Game::initrock() {
    int eff = effrack[playnum];
    bool used[8] = {false, false, false, false, false, false, false, false};
    for (int y = 4; y >= 0; --y) {
        Rock& r = rocks[y];
        int slot = 4 - y;
        int idx = 0;
        if (eff >= MAXEFFR) {
            if (rnd() & 1) {
                int k;
                do { k = rnd() & 7; } while (used[k]);
                used[k] = true;
                idx = RANDROKS[k];
            }
        } else {
            idx = ROCKS[eff][slot];
        }
        if (idx) getrxy(idx, r.x, r.y);
        else r.x = r.y = 0;
        r.ix = STRTROCK;
        r.tumble = 0;
        r.stat = 0;
        r.numsqsh = 0;
    }
}

// ------------------------------------------------------------- sequences ---
void Game::pushSeq(int type, int arg) {
    seqs.push_back(Seq{type, 0, arg, 0, 0});
}

void Game::runSeq() {
    Seq& s = seqs.back();
    switch (s.type) {
        case SEQ_DEATH: seqDeath(s); break;
        case SEQ_RACKEND: seqRackend(s); break;
        case SEQ_PLAYREDY: seqPlayredy(s); break;
        case SEQ_NEWPLAYR: seqNewplayr(s); break;
    }
}

void Game::seqRackend(Seq& s) {
    if (s.step == 0) {
        noise(SNG13);
        freeze = 154;
        s.step = 1;
        return;
    }
    racknum[playnum]++;
    initrack();
    lastmove = 0;
    ddmiddle();
    newDlst();
    seqs.pop_back();
}

void Game::seqPlayredy(Seq& s) {
    if (s.step == 0) {
        nonoise = 1;
        message = "PLAYER " + std::to_string(playnum + 1);
        messageBlank = true;
        freeze = 100;
        s.step = 1;
        return;
    }
    message.clear();
    messageBlank = false;
    nonoise = 0;
    int arg = s.arg;
    seqs.pop_back();
    if (arg == 1) afterInitGame();
}

void Game::goAttract() {
    if (attract) {
        gameOver = true;
        return;
    }
    attract = 1;
    ignore = 1;
    gameOver = true;
    message = "GAME OVER";
    messageBlank = true;
    nonoise = 1;
}

void Game::seqDeath(Seq& s) {
    switch (s.step) {
        case 0:
            if (fruit > 0 && fruit < 0x0F) dfruit();
            if (needdir) rocks[hitrock].tumble = ONEDGE;
            freeze = 40;
            s.step = 1;
            return;
        case 1:
            for (int x = NUMCREAT; x >= 0; --x) {
                Monster& m = monsters[x];
                if ((m.stat & 0x86) == 2) {
                    if (m.blwstat == 4) carol(m);
                    offscr2(m);
                }
                m.y = 0;
            }
            flame.y = 0;
            fruitStamp.y = 0;
            freeze = 8;
            s.step = 2;
            return;
        case 2:
            noise(SNG14);
            s.x = DANIMIX[lastmove];
            s.cnt = 5;
            s.step = 3;
            // fall through
        case 3:
            if (s.cnt > 0) {
                dd.ix = ADEATH[s.x];
                s.x--;
                s.cnt--;
                freeze = 18;
                return;
            }
            s.step = 4;
            // fall through
        case 4:
            s.step = 5;
            if (testrackCondition()) {
                pushSeq(SEQ_RACKEND);
                return;
            }
            // fall through
        case 5: {
            death = 0;
            int px = playnum;
            nummen[px]--;
            if (nummen[px] == 0 && (numplayr == 0 || attract)) {
                seqs.pop_back();
                goAttract();
                return;
            }
            if (!attract && numplayr) {
                int other = px ^ 1;
                if (nummen[other]) {
                    seqs.pop_back();
                    pushSeq(SEQ_NEWPLAYR, other);
                    return;
                }
                if (nummen[px] == 0) {
                    seqs.pop_back();
                    goAttract();
                    return;
                }
            }
            s.step = 6;
            if (!attract && numplayr) {
                pushSeq(SEQ_PLAYREDY, 0);
                return;
            }
        }
            // fall through
        case 6:
            notunnel = 1;
            initdirt();
            ddmiddle();
            lastmove = 0;
            freeze = 40;
            seqs.pop_back();
            return;
    }
}

void Game::seqNewplayr(Seq& s) {
    switch (s.step) {
        case 0: {
            playnum = s.arg;
            uint8_t tmp[256];
            std::memcpy(tmp, dirtmap, 256);
            std::memcpy(dirtmap, backdirt, 256);
            std::memcpy(backdirt, tmp, 256);
            Snapshot cur = snapshot();
            if (back.valid) restore(back);
            back = cur;
            for (auto& r : rocks) r.ix = STRTROCK;
            if (!p2init) {
                p2init = 1;
                entry = 1;
                s.step = 1;
            } else {
                s.step = 2;
            }
            pushSeq(SEQ_PLAYREDY, 0);
            return;
        }
        case 1:
            initrack();
            lastmove = 0;
            freeze = 40;
            seqs.pop_back();
            return;
        default:
            notunnel = 1;
            initdirt();
            ddmiddle();
            lastmove = 0;
            freeze = 40;
            seqs.pop_back();
            return;
    }
}

Game::Snapshot Game::snapshot() const {
    Snapshot s;
    s.valid = true;
    const Stamp* all[19];
    int n = 0;
    all[n++] = &dd;
    for (auto& m : monsters) all[n++] = &m;
    for (auto& r : rocks) all[n++] = &r;
    all[n++] = &flame;
    for (auto& p : pump) all[n++] = &p;
    all[n++] = &fruitStamp;
    for (int i = 0; i < 19; ++i) {
        s.sx[i] = all[i]->x; s.sy[i] = all[i]->y; s.six[i] = all[i]->ix;
    }
    for (int i = 0; i < 5; ++i) {
        s.tumble[i] = rocks[i].tumble; s.rstat[i] = rocks[i].stat; s.numsqsh[i] = rocks[i].numsqsh;
    }
    for (int i = 0; i < 8; ++i) s.monstat[i] = monsters[i].stat;
    s.hitrock = hitrock; s.fallcnt = fallcnt; s.brcreat = brcreat; s.brtunnl = brtunnl;
    return s;
}

void Game::restore(const Snapshot& s) {
    std::vector<Stamp*> all = allStamps();
    for (int i = 0; i < 19; ++i) {
        all[i]->x = s.sx[i]; all[i]->y = s.sy[i]; all[i]->ix = s.six[i];
    }
    for (int i = 0; i < 5; ++i) {
        rocks[i].tumble = s.tumble[i]; rocks[i].stat = s.rstat[i]; rocks[i].numsqsh = s.numsqsh[i];
    }
    for (int i = 0; i < 8; ++i) monsters[i].stat = s.monstat[i];
    hitrock = s.hitrock; fallcnt = s.fallcnt; brcreat = s.brcreat; brtunnl = s.brtunnl;
}

}  // namespace dd
