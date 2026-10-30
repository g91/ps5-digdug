// Pooka / Fygar behaviour (MONSTER.S)
#include "game.h"
#include <algorithm>
#include <cstdlib>

namespace dd {

void Game::monstersFrame(bool button) {
    shift8(proport);
    if (entry) return;
    pumper(button);
    fruiter();
    flamer();
    for (int x = 7; x >= 0; --x) movemons(monsters[x]);
    if (speedup == 0 && !fast && !escaper) {
        fast = 1;
        noise(SNG12);
        for (auto& m : monsters)
            if (!(m.stat & 0x80)) speeder(m);
    }
}

int Game::mshift(Monster& m) {
    int c = (m.speed >> 15) & 1;
    m.speed = ((m.speed << 1) & 0xFFFF) | c;
    return c;
}

void Game::movemons(Monster& m) {
    if (m.stat != 0) return;
    if (m.dir & 0x0A) {
        if (!(proport & 0x80)) return;
        if (!mshift(m)) return;
        dispchar(m);
    } else {
        if (!mshift(m)) return;
        if (shift8(proport2)) dispchar(m);
    }
    m.x = (m.x + XTABLE[m.dir]) & 0xFF;
    int ny = m.y + YTABLE[m.dir];
    if (ny >= 23 && ny < 192) {
        m.y = ny;
        int v = m.ymodc + YTABLE[m.dir];
        if (v == 12) v = 0;
        else if (v == -1) v = 11;
        m.ymodc = v;
    }
    if (m.ghost) {
        if ((m.x & 7) == 0) ghostonx(m);
        else if (m.ymodc == 0) ghostony(m);
        return;
    }
    if ((m.x & 7) || m.ymodc) return;
    movemode(m);
}

void Game::dispchar(Monster& m) {
    int n = m.seqn;
    if (n >= (int)m.seq->size()) n = 0;
    m.ix = (*m.seq)[n];
    m.seqn = n + 1;
}

int Game::getdirt(const Monster& m, int& idx) {
    idx = getdirtIdx(m.x, m.y);
    return dirtmap[idx];
}

int Game::chkother(int c, int idx) {
    int t = c;
    if (!(c & 2) && dirtmap[(idx - 16) & 0xFF] >= CHARD12) t |= 2;
    if (!(c & 4) && dirtmap[(idx + 1) & 0xFF] >= CHARD12) t |= 4;
    if (!(c & 8) && dirtmap[(idx + 16) & 0xFF] >= CHARD12) t |= 8;
    if (!(c & 16) && dirtmap[(idx - 1) & 0xFF] >= CHARD12) t |= 16;
    return t;
}

int Game::norev(const Monster& m) {
    int d = m.dir;
    if (d & 0x0A) return d | 5;
    return d | 0x0A;
}

void Game::movemode(Monster& m) {
    movecont(m);
    if (m.stat & 0x80) return;
    if (!m.fygar() || m.ghost || !(m.dir & 0x0A)) return;
    flamtime += 16;
    if (flamtime < 256) return;
    flamtime &= 0xFF;
    if (flamie) return;
    flame.ix = ZBLANK;
    m.stat = 1;
    setflam();
    flamie = m.index;
    flame.y = m.y;
    flame.x = (m.x + 8) & 0xFF;
}

void Game::movecont(Monster& m) {
    int idx;
    int c = getdirt(m, idx);
    int t = chkother(c, idx);
    int i = std::min(t >> 1, 15);
    int nr = norev(m);
    switch (i) {
        case 0: mode(m, nr); return;
        case 1: mode(m, nr & 0x0B); return;
        case 2: mode(m, nr & 0x07); return;
        case 3: ghoster(m, nr & 0x03); return;
        case 4: mode(m, nr & 0x0E); return;
        case 5: case 10: return;
        case 6: ghoster(m, nr & 0x06); return;
        case 7: ghoster(m, 2); return;
        case 8: mode(m, nr & 0x0D); return;
        case 9: ghoster(m, nr & 0x09); return;
        case 11: ghoster(m, 1); return;
        case 12: ghoster(m, nr & 0x0C); return;
        case 13: ghoster(m, 8); return;
        case 14: ghoster(m, 4); return;
        default: break;
    }
    int below = dirtmap[(idx + 16) & 0xFF];
    int t1 = ((below & 0xE2) ? 1 : 0) ^ 0x0B;
    int a = nr & t1;
    if (idx == 1) { mode(m, a & 0xFD); return; }
    if (idx == 0x0E) { ghoster(m, a & 0xF7); return; }
    mode(m, a);
}

void Game::mode(Monster& m, int a) {
    switch (m.mode) {
        case MODE_VERT: digvert(m, a); return;
        case MODE_HORIZ: dighoriz(m, a); return;
        case MODE_DIAG: digdiag(m, a); return;
        case MODE_ESCAPE: escape(m, a); return;
        default: offscr(m); return;
    }
}

int Game::getpoint(int x0, int x1, int y0, int y1) {
    int p = 0;
    int dx = x0 - x1, dy = y0 - y1;
    if (dx > 0) p |= 2;
    else if (dx < 0) p |= 8;
    if (dy > 0) p |= 1;
    else if (dy < 0) p |= 4;
    gp_dx = dx;
    gp_dy = dy;
    return p;
}

int Game::sub11(const Monster& m) {
    return getpoint(m.x, dd.x, m.y, dd.y);
}

bool Game::sub1(Monster& m, int a, int& r) {
    int p = sub11(m);
    if (m.ghost) {
        m.dir = p;
        if (p & 0x0A) m.facing = p & 0x0A;
        return false;
    }
    r = p & a;
    if (!r) r = a;
    return true;
}

void Game::pick(Monster& m, int r, const int* order) {
    for (int i = 0; i < 4; ++i) {
        if (r & order[i]) { r = order[i]; break; }
    }
    m.dir = r;
    newseq(m, r);
}

void Game::digvert(Monster& m, int a) {
    int r;
    if (!sub1(m, a, r)) return;
    static const int order[4] = {1, 4, 2, 8};
    pick(m, r, order);
}

void Game::dighoriz(Monster& m, int a) {
    int r;
    if (!sub1(m, a, r)) return;
    static const int order[4] = {8, 2, 4, 1};
    pick(m, r, order);
}

void Game::digdiag(Monster& m, int a) {
    int r;
    if (!sub1(m, a, r)) return;
    escont(m, r);
}

void Game::escont(Monster& m, int r) {
    static const int vert[4] = {4, 1, 8, 2};
    static const int horiz[4] = {8, 2, 4, 1};
    if (std::abs(gp_dy) >= std::abs(gp_dx)) pick(m, r, vert);
    else pick(m, r, horiz);
}

void Game::escape(Monster& m, int a) {
    int p = getpoint(m.x, 0x10, m.y, 0xBF);
    if (p == 0) { scram(m); return; }
    if (m.ghost) {
        m.dir = p;
        if (p & 0x0A) m.facing = p & 0x0A;
        return;
    }
    int r = p & a;
    if (r == 0) {
        if (m.y < 0xBF) {
            m.ghost = 1;
            m.dir = 4;
            newseq(m, 4);
            speeder(m);
            return;
        }
        if (m.x >= 0x10) {
            m.dir = 2;
            newseq(m, 2);
            speeder(m);
            return;
        }
        scram(m);
        return;
    }
    escont(m, r);
}

void Game::scram(Monster& m) {
    m.mode = MODE_OFFSCR;
    offscr(m);
}

void Game::offscr(Monster& m) {
    m.ghost = 0;
    m.dir = 2;
    newseq(m, 2);
    speeder(m);
    if (m.x >= 128 || m.x == 0) offscr2(m);
}

void Game::offscr2(Monster& m) {
    m.stat = 0x80;
    m.y = 0;
}

void Game::ghoster(Monster& m, int a) {
    int ghosttmp = (~norev(m)) & 0x0F;
    if (m.mode == MODE_ESCAPE) { ghoster2(m, a); return; }
    int rk = bonzo ? 1 : rack();
    int v = ((rk * 2) & 0xFF) + 10;
    if (v <= 255) {
        v += flee;
        if (v <= 255) {
            v += ghostime;
            if (v <= 255) {
                ghostime = v;
                v += digrest >> 5;
                if (v <= 255) {
                    ghostime = v;
                    if (ghostout) { ghoster2(m, a); return; }
                    znxt9(m, a, ghosttmp, v);
                    return;
                }
            }
        }
    }
    znxt9(m, a, ghosttmp, v & 0xFF);
}

void Game::ghoster2(Monster& m, int d) {
    m.dir = d;
    newseq(m, d);
    if (m.mode == MODE_ESCAPE) mode(m, m.dir);
}

void Game::znxt9(Monster& m, int a, int ghosttmp, int v) {
    ghostime = v;
    digrest = 0;
    int p = sub11(m);
    if (a == ghosttmp || (p & a) == 0) {
        ghostout = 4;
        m.ghost = 1;
        m.dir = p;
        newseq(m, p);
        speeder(m);
        return;
    }
    ghoster2(m, a);
}

void Game::ghostonx(Monster& m) {
    int idx;
    int c = getdirt(m, idx);
    if (c >= CHARD12) { mode(m, c); return; }
    if (c >= CHARDF) {
        m.ghost = 0;
        speeder(m);
        mode(m, 0x0A);
        return;
    }
    int above = dirtmap[(idx - 16) & 0xFF];
    if (above >= CHARD12) { mode(m, above); return; }
    if (((above >> 2) | c) & 2) { mode(m, (above >> 2) | c); return; }
    m.ghost = 0;
    speeder(m);
    if (m.ymodc != 0) { mode(m, 5); return; }
    int t = chkother(c, idx);
    if ((t >> 1) & 4) {
        m.dir = 4;
        mode(m, 4);
        return;
    }
    mode(m, 5);
}

void Game::ghostony(Monster& m) {
    int idx;
    int c = getdirt(m, idx);
    if (c >= CHARD12) { mode(m, c); return; }
    if (c >= CHARDF) {
        m.ghost = 0;
        speeder(m);
        mode(m, 0x0A);
        return;
    }
    int right = dirtmap[(idx + 1) & 0xFF];
    if (right >= CHARD12) { mode(m, right); return; }
    if (((right >> 2) | c) & 4) { mode(m, (right >> 2) | c); return; }
    m.ghost = 0;
    speeder(m);
    mode(m, 0x0A);
}

void Game::newseq(Monster& m, int d) {
    if (d & 0x0A) m.facing = d & 0x0A;
    int i = (((m.fygar() ? 4 : 0) | m.facing) >> 1) ^ (m.ghost ? 1 : 0);
    m.seq = SEQTAB[i & 7];
    m.seqn = 0;
    m.ix = (*m.seq)[0];
}

void Game::speeder(Monster& m) {
    int i = (m.fygar() ? 4 : 0) | (fast ? 2 : 0) | (m.ghost ? 1 : 0);
    m.speed = SPEEDTAB[i];
}

void Game::avoid() {
    for (int x = 7; x >= 0; --x) {
        Monster& m = monsters[x];
        if ((m.stat & 0x80) || m.ghost || m.dir != 4 || m.ymodc < 3) continue;
        if (escaper) continue;
        m.mode = MODE_VERT;
        m.dir = 1;
    }
}

void Game::getmode() {
    int y = 0;
    for (int x = 7; x >= 0; --x) {
        Monster& m = monsters[x];
        if (!(m.stat & 0x80)) m.mode = MODETAB[y++ & 7];
    }
    y -= 1;
    if (y == 0) {
        Monster* last = nullptr;
        for (int x = 7; x >= 0; --x)
            if (!(monsters[x].stat & 0x80)) { last = &monsters[x]; break; }
        if (!last) return;
        if (!(last->stat & 0x40) && (last->stat & 6) != 2) noise(SNG9);
        last->mode = MODE_ESCAPE;
        escaper++;
        return;
    }
    if (fast || speedup < 3) return;
    int rk = bonzo ? 1 : rack();
    if (rk < 4) return;
    int x = std::min(rk >> 2, 5);
    if (y >= 0 && y < WHNSPEED[x - 1]) speedup = 1;
}

void Game::fruiter() {
    if (fruit == 0) return;
    if (!(racktime & 1)) return;
    int a = fruit;
    fruit = (fruit + 1) & 0xFF;
    if (a == 0xC0) {
        fruit = 0;
        fruitStamp.y = 0;
    } else if (a == 0x0E) {
        dfruit();
    }
}

void Game::setflam() {
    flamsize = 0;
    flamnum = -24;
    flamwait = 7;
}

void Game::flamout4(Monster& m) {
    flame.y = 0;
    flamie = 0;
    m.stat = 0;
}

void Game::flamer() {
    if (!flamie) return;
    Monster& m = monsters[flamie];
    if (flamnum < 0) {
        flamnum++;
        int t = ((~(flamnum - 1)) & 0x0F) >> 2;
        int y = t * 2 + ((m.facing & 2) ? 1 : 0);
        m.ix = FLAMTABL[y & 7];
        return;
    }
    if (flamnum == 0 && flamsize == 0) {
        int idx;
        getdirt(m, idx);
        if (m.facing & 2) {
            int n = 0;
            bool hit = false;
            for (int k = 0; k < 3; ++k) {
                idx--;
                n++;
                if (dirtmap[idx & 0xFF] >= CHARD12) { hit = true; break; }
            }
            if (!hit) n++;
            flamsize = -(n + 1);
        } else {
            int n = 0;
            bool hit = false;
            for (int k = 0; k < 3; ++k) {
                idx++;
                n++;
                if (dirtmap[idx & 0xFF] >= CHARD12) { hit = true; break; }
            }
            if (!hit) n++;
            flamsize = n;
        }
        noise(SNG3);
    }
    int size = flamsize;
    if (size < 0) {
        size = (~size) & 0xFF;
        if (size == flamnum) { flamout2(m); return; }
        int n = flamnum & 3;
        flame.x = (m.x - FLAMPOS[n]) & 0xFF;
        flame.ix = LFLAMSEQ[n];
    } else {
        if (size == flamnum) { flamout2(m); return; }
        flame.ix = RFLAMSEQ[flamnum & 3];
    }
    flamnum++;
    flamcoll(m);
}

void Game::flamout2(Monster& m) {
    flamwait--;
    if (flamwait) { flamcoll(m); return; }
    flame.ix = ZBLANK;
    if (proport & 0x80) flamout4(m);
    else setflam();
}

void Game::flamcoll(Monster& m) {
    if (squash) return;
    int d = flame.x - 7 - dd.x;
    if (d >= 0) return;
    int a = ((-d) + 2) >> 3;
    if (a >= flamnum) return;
    if (std::abs(dd.y - flame.y) >= 10) return;
    m.stat = 0;
    flamie = 0;
    yescoll();
}

}  // namespace dd
