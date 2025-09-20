#include <string.h>
#include "cdp1802.h"

// #include <avr/pgmspace.h>

static unsigned char M[512];
static unsigned short R[16];
static unsigned char *rp = (unsigned char *)R;
static unsigned short P;
static unsigned short X;
static unsigned char D;
static unsigned char IE;

// "flags" are booleans, 1 is TRUE, 0 is FALSE
// can be used in math if necessary
static unsigned char flags[16]; // 0, q, z, df, ef0-3, and inverses

#define LO 0
#define HI 1

// flags
#define UF 0 // unconditional branch
#define NUF 8 // nop
#define QF 1
#define NQF 9
#define ZF 2
#define NZF 10
#define DF 3
#define NDF 11

#define UPDATE_ZF(n) \
    flags[ZF] = (n==0);\
    flags[NZF] = !flags[ZF];

#define UPDATE_CARRY(n) \
    flags[DF] = !!((n) & 0x0800);\
    flags[NDF] = !flags[DF];

#define UPDATE_BORROW(n) \
    flags[NDF] = !!(n & 0x0800);\
    flags[DF] = !flags[NDF];

void cdp1802_logic7(unsigned char N) {
    unsigned char m;
    unsigned short x;
    unsigned char carry;

    switch (N) {
    case 0x00: // return - M(R(X)) -> (X,P); R(X)+1; 1 -> IE 
        m = M[R[X]++];
        X = m >> 4;
        P = m & 0x0f;
        IE = 1;
        break;
    case 0x01: // disable - M(R(X)) -> (X,P), R(X)+1; 0 -> IE 
        m = M[R[X]++];
        X = m >> 4;
        P = m & 0x0f;
        IE = 0;
        break;
    case 0x02: // ldxa
        D = R[X]++;
        break;
    case 0x03: // stxd
        M[R[X]--] = D;
        break;
    case 0x04: // adc: M(R(X))+D+DF -? DF, D
        x = (unsigned short)M[R[X]] + D + flags[DF];
        D = x & 0xff;
        UPDATE_CARRY(x);
        break;
    case 0x05: // sdb: M(R(X))-D-(NOT DF) -> DF, D 
        x = (unsigned short)M[R[X]] - D - flags[NDF];
        D = x & 0xff;
        UPDATE_BORROW(x);
        break;
    case 0x06: // shrc: SHIFT D RIGHT; LSB(D) -> DF, DF -> MSB(O)
        carry = flags[DF];
        flags[DF] = D & 0x01;
        flags[NDF] = !flags[DF];
        D = D >> 1;
        if (flags[DF]) D = D | carry;
        break;
    case 0x07: // smb: D-M(R(X))-(NOT DF) -> DF, D   
        x = (unsigned short)D - M[R[X]] - flags[NDF];
        D = x & 0xff;
        UPDATE_BORROW(x);
        break;
    case 0x0c: // adci: M(R(P))+D+DF -> DF, D;R(P)+1 
        x = (unsigned short)M[R[P]++] + D + flags[DF];
        D = x & 0xff;
        UPDATE_CARRY(x);
        break;
    case 0x0d: // sdbi: M(R(P))-D-(NOT DF) -> DF, D; R(P)+1
        x = (unsigned short)M[R[P]++] - D - flags[NDF];
        D = x & 0xff;
        UPDATE_BORROW(x);
        break;
    case 0x0e: // shlc: SHIFT D LEFT; MSB(D) -> DF, 0-> LSB(D)
        carry = flags[DF];
        flags[DF] = D & 0x80;
        flags[NDF] = !flags[DF];
        D = D << 1;
        if (flags[DF]) D = D | carry;
        break;
    case 0x0f: // smbi: D-M(R(P))-(NOT DF) -> DF, D; R(P)+1  
        x = (unsigned short)D - M[R[P]++];
        if (flags[NDF]) x--;
        D = x & 0xff;
        UPDATE_BORROW(x);
        break;
    }
}

void cdp1802_logicf(unsigned char N) {
    unsigned short x;

    switch (N) {
    case 0x00: // ldx
        D = R[X];
        break;
    case 0x01: // or: M(R(X)) OR D -> D 
        D = M[R[X]] | D;
        break;
    case 0x02: // and: M(R(X)) AND D -> D 
        D = M[R[X]] & D;
        break;
    case 0x03: // xor: M(R(X)) XOR D -> D
        D = M[R[X]] ^ D;
        break;
    case 0x04: // add: M(R(X))+D -> DF, D 
        x = (unsigned short)M[R[X]] + D;
        D = x & 0xff;
        flags[DF] = x >> 8;
        flags[NDF] = !flags[DF];
        UPDATE_CARRY(x);
        break;
    case 0x05: // sd: M(R(X))-D -> DF,D
        x = (unsigned short)M[R[X]] - D;
        D = x & 0xff;
        flags[NDF] = x >> 8; // no borrow == 1
        flags[DF] = !flags[NDF];
        UPDATE_BORROW(x);
        break;
    case 0x06: // shr: SHIFT D RIGHT; LSB(D) ~ DF, O~ MSB(D)
        flags[DF] = D & 0x01;
        flags[NDF] = !flags[DF];
        D = D >> 1;
        break;
    case 0x07: // sm: D-M(R(X)) -> DF, D 
        x = (unsigned short)D - M[R[X]];
        D = x & 0xff;
        flags[NDF] = x >> 8; // no borrow == 1
        flags[DF] = !flags[NDF];
        UPDATE_BORROW(x);
        break;
    case 0x08: // ldi
        D = M[R[P]++];
        break;
    case 0x09: // ori: M(R(P)) OR D -> D; R(P)+l
        D = M[R[P]++] | D;
        break;
    case 0x0a: // andi: M(R(P)) AND D -> D; R(P)+1 
        D = M[R[P]++] & D;
        break;
    case 0x0b: // xori: M(R(P)) XOR D -> D; R(P)+1 
        D = M[R[P]++] ^ D;
        break;
    case 0x0c: // adi: M(R(P))+D -> DF, D; R(P)+1
        x = (unsigned short)M[R[P]++] + D;
        D = x &0xff;
        UPDATE_CARRY(x);
        break;
    case 0x0d: // sdi: M(R(P))-D -> DF,D; R(P)+1
        x = (unsigned short)M[R[P]++] - D;
        D = x & 0xff;
        UPDATE_BORROW(x);
        break;
    case 0x0e: // shl: SHIFT D LEFT; MSB(O) -> OF, 0-> LSB(O)
        flags[DF] = D & 0x80;
        flags[NDF] = !flags[DF];
        D = D << 1;
        break;
    case 0x0f: // smi: D-M(R(P)) -> DF, D; R(P)+1  
        x = (unsigned short)D - M[R[P]++];
        D = x & 0xff;
        flags[NDF] = x >> 8; // no borrow == 1
        flags[DF] = !flags[NDF];
        UPDATE_BORROW(x);
        break;
    }
}

void cdp1802_dispatch() {
    unsigned short pc = R[P]++;
    unsigned char m = M[pc];
    unsigned char I = m >> 4;
    unsigned char N = m &0x0f;
    switch(I) {
    case 0x00: // ld(N)
        D = M[R[N]];
        break;
    case 0x01: // inc(N)
        R[N]++;
        break;
    case 0x02: // dec(N)
        R[N]--;
        break;
    case 0x03: // short branch
        UPDATE_ZF(D);
        m = M[R[P]++];
        if (flags[N]) {
            rp[P+LO] = m;
        }
        break;
    case 0x04: // lda(N)
        D = M[R[N]++];
        break;
    case 0x05: // str(n)
        M[R[N]] = D;
        break;
    case 0x06: // i/o
        if (N==0) {
            R[X]++;
        } else {
            //
        }
        break;
    case 0x07: // control & arithmetic
        cdp1802_logic7(N);
        break;
    case 0x08: // glo(N)
        D = rp[(N<<1)+LO]; // get the low byte of the register
        break;
    case 0x09: // ghi(N)
        D = rp[(N<<1)+HI]; // get the high byte of the register
        break;
    case 0x0a: // setlo(N)
        rp[(N<<1)+LO] = D; // set the low byte of the register
        break;
    case 0x0b: //sethi(N)
        rp[(N<<1)+HI] = D; // set the high byte of the register
        break;
    case 0x0c: // long branch
        UPDATE_ZF(D);
        unsigned char hi = M[R[P]++];
        unsigned char lo = M[R[P]++];
        if (flags[N]) {
            rp[P+HI] = hi;
            rp[P+LO] = lo;
        }
        break;
    case 0x0d: // sep(N)
        P = N;
        break;
    case 0x0e: // sex(N)
        X = N;
        break;
    case 0x0f: // logicf
        cdp1802_logicf(N);
        break;
    }
}

void cdp1802_init() {
    memset(M, 0, sizeof(M)); // zero memory
    memset(flags, 0, 8); // set flags to false
    memset(&flags[8], 0x01, 8); // set negative flags to true
    flags[UF] = 0x01; // unconditional branch
    flags[NUF] = 0x00; // skp
    P = 0;
    X = 0;
    D = 0;
}

void cdp1802_main() {
    cdp1802_init();
    while(true) {
        cdp1802_dispatch();
    }
}

static cdp1802 cdp;

cdp1802 *cdp1802_info() {
    cdp.M = M;
    cdp.R = R;
    cdp.rp = rp;
    cdp.P = P;
    cdp.X = X;
    cdp.D = D;
    cdp.IE = IE;
    cdp.flags = flags;
    return &cdp;
}