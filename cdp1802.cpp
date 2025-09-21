#include <string.h>
#include <stdint.h>
#include "cdp1802.h"

// #include <avr/pgmspace.h>

// Compiler optimization hints
#pragma GCC optimize("O3")
#pragma GCC optimize("unroll-loops")

static cdp1802 cdp;
static unsigned char (*mget)(unsigned short addr);
static void (*mset)(unsigned short addr, unsigned char byte);
static unsigned short R[16];
static unsigned short P;
static unsigned short X;
static unsigned char D;
static unsigned char IE;
static unsigned char flags[16]; // 1, z, q, df, ef1-4, 0, ~z, ~q, ~df, ~ef1-~ef4
static unsigned short debug[16];

// flags
#define DF 3
#define NDF 11

#define MGET0(reg) (mget(R[(reg)]))
#define MGET(reg) (mget(R[(reg)]++))
#define MSET(reg,value) (mset(R[(reg)], (value)))

#define UPDATE_ZF(n) \
    flags[_ZF] = (n==0);

#define UPDATE_CARRY(n) \
    flags[_DF] = !!((n) & 0x0100);  // Bit 8 for 8-bit carry

#define UPDATE_BORROW(n) \
    flags[_DF] = !((n) & 0x0100); \

static inline void cdp1802_logic7(unsigned char N) {
    unsigned char m;
    unsigned short x;
    unsigned char carry;

    if (N == 0x00) { // return - M(R(X)) -> (X,P); R(X)+1; 1 -> IE 
        m = MGET(X);
        X = m >> 4;
        P = m & 0x0f;
        IE = 1;
    } else if (N == 0x01) { // disable - M(R(X)) -> (X,P), R(X)+1; 0 -> IE 
        m = MGET(X);
        X = m >> 4;
        P = m & 0x0f;
        IE = 0;
    } else if (N == 0x02) { // ldxa
        D = R[X]++;
    } else if (N == 0x03) { // stxd
        MSET(X, D); R[X]--;
    } else if (N == 0x04) { // adc: M(R(X))+D+DF -> DF, D
        x = (unsigned short)MGET0(X) + D + flags[_DF];
        D = x & 0xff;
        UPDATE_CARRY(x);
    } else if (N == 0x05) { // sdb: M(R(X))-D-(NOT DF) -> DF, D 
        x = (unsigned short)MGET0(X) - D - (!flags[_DF]);
        D = x & 0xff;
        UPDATE_BORROW(x);
    } else if (N == 0x06) { // shrc: SHIFT D RIGHT; LSB(D) -> DF, DF -> MSB(O)
        carry = flags[_DF];
        flags[_DF] = D & 0x01;
        D = D >> 1;
        if (flags[_DF]) D = D | carry;
    } else if (N == 0x07) { // smb: D-M(R(X))-(NOT DF) -> DF, D   
        x = (unsigned short)D - MGET0(X) - (!flags[_DF]);
        D = x & 0xff;
        UPDATE_BORROW(x);
    } else if (N == 0x0a) { // smb: D-M(R(X))-(NOT DF) -> DF, D   
        flags[_QF] = 0;
    } else if (N == 0x0b) { // smb: D-M(R(X))-(NOT DF) -> DF, D   
        flags[_QF] = 1;
    } else if (N == 0x0c) { // adci: M(R(P))+D+DF -> DF, D;R(P)+1 
        x = (unsigned short)MGET(P) + D + flags[_DF]; 
        D = x & 0xff;
        UPDATE_CARRY(x);
    } else if (N == 0x0d) { // sdbi: M(R(P))-D-(NOT DF) -> DF, D; R(P)+1
        x = (unsigned short)MGET(P) - D - (!flags[_DF]); 
        D = x & 0xff;
        UPDATE_BORROW(x);
    } else if (N == 0x0e) { // shlc: SHIFT D LEFT; MSB(D) -> DF, 0-> LSB(D)
        carry = flags[_DF];
        flags[_DF] = D & 0x80;
        D = D << 1;
        if (flags[_DF]) D = D | carry;
    } else if (N == 0x0f) { // smbi: D-M(R(P))-(NOT DF) -> DF, D; R(P)+1  
        x = (unsigned short)D - MGET(P); 
        if (!flags[_DF]) x--;
        D = x & 0xff;
        UPDATE_BORROW(x);
    }
}

static inline void cdp1802_logicf(unsigned char N) {
    unsigned short x;
    debug[3] = N;
    if (N == 0x00) { // ldx
        D = MGET0(X);
    } else if (N == 0x01) { // or: M(R(X)) OR D -> D 
        D = MGET0(X) | D;
    } else if (N == 0x02) { // and: M(R(X)) AND D -> D 
        D = MGET0(X) & D;
    } else if (N == 0x03) { // xor: M(R(X)) XOR D -> D
        D = MGET0(X) ^ D;
    } else if (N == 0x04) { // add: M(R(X))+D -> DF, D 
        x = (unsigned short)MGET0(X) + D;
        D = x & 0xff;
        flags[_DF] = x >> 8;
        UPDATE_CARRY(x);
    } else if (N == 0x05) { // sd: M(R(X))-D -> DF,D
        x = (unsigned short)MGET0(X) - D;
        D = x & 0xff;
        UPDATE_BORROW(x);
    } else if (N == 0x06) { // shr: SHIFT D RIGHT; LSB(D) ~ DF, O~ MSB(D)
        flags[_DF] = D & 0x01;
        D = D >> 1;
    } else if (N == 0x07) { // sm: D-M(R(X)) -> DF, D 
        x = (unsigned short)D - MGET0(X);
        D = x & 0xff;
        UPDATE_BORROW(x);
    } else if (N == 0x08) { // ldi
        D = MGET(P);
    } else if (N == 0x09) { // ori: M(R(P)) OR D -> D; R(P)+l
        D = MGET(P) | D;
    } else if (N == 0x0a) { // andi: M(R(P)) AND D -> D; R(P)+1 
        D = MGET(P) & D;
    } else if (N == 0x0b) { // xori: M(R(P)) XOR D -> D; R(P)+1 
        D = MGET(P) ^ D;
    } else if (N == 0x0c) { // adi: M(R(P))+D -> DF, D; R(P)+1
        x = (unsigned short)MGET(P) + D; 
        D = x &0xff;
        UPDATE_CARRY(x);
    } else if (N == 0x0d) { // sdi: M(R(P))-D -> DF,D; R(P)+1
        x = (unsigned short)MGET(P) - D; 
        D = x & 0xff;
        UPDATE_BORROW(x);
    } else if (N == 0x0e) { // shl: SHIFT D LEFT; MSB(O) -> OF, 0-> LSB(O)
        flags[_DF] = D & 0x80;
        D = D << 1;
    } else if (N == 0x0f) { // smi: D-M(R(P)) -> DF, D; R(P)+1  
        x = (unsigned short)D - MGET(P); 
        D = x & 0xff;
        UPDATE_BORROW(x);
    }
}

void cdp1802_dispatch() {
    unsigned char m = MGET(P); 
    unsigned char I = m >> 4;
    unsigned char N = m & 0x0f;
    
    if (I == 0x00) { // ld(N)
        D = MGET0(N);
    } else if (I == 0x01) { // inc(N)
        R[N]++;
    } else if (I == 0x02) { // dec(N)
        R[N]--;
    } else if (I == 0x03) { // short branch
        m = MGET(P);
        UPDATE_ZF(D);
        unsigned char f = flags[N&0x7];
        debug[0] = f;
        if (N&0x8) f = !f;
        debug[1] = f;
        if (f) R[P] = (R[P]&0xff00)|m;
    } else if (I == 0x04) { // lda(N)
        D = MGET(N);
    } else if (I == 0x05) { // str(n)
        MSET(N, D);
    } else if (I == 0x06) { // i/o
        if (N==0) {
            R[X]++;
        } else {
            //
        }
    } else if (I == 0x07) { // control & arithmetic
        cdp1802_logic7(N);
    } else if (I == 0x08) { // glo(N)
        D = R[N] & 0xff; // get the low byte of the register
    } else if (I == 0x09) { // ghi(N)
        D = R[N] >> 8; // get the high byte of the register
    } else if (I == 0x0a) { // setlo(N)
        R[N] &= 0xff00;
        R[N] |= (unsigned short)D; // set the low byte of the register
    } else if (I == 0x0b) { //sethi(N)
        R[N] &= 0x00ff;
        R[N] |= ((unsigned short)D) << 8; // set the low byte of the register
    } else if (I == 0x0c) { // long branch
        if ((N > 0x07) ? flags[N] : !flags[N]) {
            R[P] = ((unsigned short) MGET(P) << 8) | MGET(P);
        } else {
            R[P] += 2;
        }
    } else if (I == 0x0d) { // sep(N)
        P = N;
    } else if (I == 0x0e) { // sex(N)
        X = N;
    } else if (I == 0x0f) { // logicf
        cdp1802_logicf(N);
    } else { // default
        debug[7] = 0xff; // error
    }    
}

static inline bool takeBranch(uint8_t N) {
  // N ∈ [0..15]; for 0..7 use !flags[N], for 8..15 use  flags[N]
  return (N & 0x08) ? flags[N] : !flags[N];
}

void cdp1802_dispatch_new() {
  uint8_t m = MGET(P);             // fetch opcode (your MGET likely auto-increments P)
  uint8_t I = m >> 4;
  uint8_t N = m & 0x0F;

  switch (I) {
    case 0x0: D = MGET0(N); UPDATE_ZF(D); break;                 // LDn
    case 0x1: R[N]++; break;                                     // INCn
    case 0x2: R[N]--; break;                                     // DECn
    case 0x3: {                                                 // BR short
      uint8_t lo = MGET(P);                                     // next byte
      if (takeBranch(N)) { R[P] = (R[P] & 0xFF00) | lo; }
      /* else P already advanced by MGET(P) */
    } break;
    case 0x4: D = MGET0(N); break;                               // LDAn
    case 0x5: MSET(N, D); break;                                 // STRn
    case 0x6: if (N == 0) { R[X]++; } /* else ... IO */ break;   // I/O
    case 0x7: cdp1802_logic7(N); break;
    case 0x8: D =  R[N]        & 0xFF; break;                    // GLO
    case 0x9: D = (R[N] >> 8)  & 0xFF; break;                    // GHI
    case 0xA: R[N] = (R[N] & 0xFF00) | (uint16_t)D; break;       // PLO
    case 0xB: R[N] = (R[N] & 0x00FF) | ((uint16_t)D << 8); break;// PHI
    case 0xC: {                                                 // LBR long
      if (takeBranch(N)) {
        uint8_t hi = MGET(P);
        uint8_t lo = MGET(P);
        R[P] = ((uint16_t)hi << 8) | lo;
      } else {
        R[P] += 2; // or rely on two MGET(P) side-effects if you prefer reading then ignoring
      }
    } break;
    case 0xD: P = N; break;                                      // SEP
    case 0xE: X = N; break;                                      // SEX
    case 0xF: cdp1802_logicf(N); break;
    default: debug[7] = 0xFF; break;
  }
}

cdp1802 *cdp1802_info() {
    cdp.mget = mget;
    cdp.mset = mset;
    cdp.R = R;
    cdp.P = P;
    cdp.X = X;
    cdp.D = D;
    cdp.IE = IE;
    cdp.flags = flags;
    cdp.debug = debug;
    return &cdp;
}

cdp1802 *cdp1802_init(unsigned char (*_mget)(unsigned short), void (*_mset)(unsigned short, unsigned char)) {
    mget = _mget;
    mset = _mset;
    memset(flags, 0, 8); // set flags to false
    memset(&flags[8], 0x01, 8); // set negative flags to true
    flags[_UF] = 0x01; // unconditional branch
    P = 0;
    X = 0;
    D = 0;
    return cdp1802_info();
}

void cdp1802_main() {
    while(true) {
        cdp1802_dispatch();
    }
}

