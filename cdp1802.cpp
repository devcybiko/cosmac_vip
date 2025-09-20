#include <string.h>
#include "cdp1802.h"

// #include <avr/pgmspace.h>

static cdp1802 cdp;

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

#define MGET(addr) (cdp.mget((addr)))
#define MSET(addr,value) (cdp.mset((addr), (value)))

#define UPDATE_ZF(n) \
    cdp.flags[ZF] = (n==0);\
    cdp.flags[NZF] = !cdp.flags[ZF];

#define UPDATE_CARRY(n) \
    cdp.flags[DF] = !!((n) & 0x0800);\
    cdp.flags[NDF] = !cdp.flags[DF];

#define UPDATE_BORROW(n) \
    cdp.flags[NDF] = !!(n & 0x0800);\
    cdp.flags[DF] = !cdp.flags[NDF];

void cdp1802_logic7(unsigned char N) {
    unsigned char m;
    unsigned short x;
    unsigned char carry;

    switch (N) {
    case 0x00: // return - M(cdp.R(cdp.X)) -> (cdp.X,cdp.P); cdp.R(cdp.X)+1; 1 -> cdp.IE 
        m = cdp.mget(cdp.X); cdp.R[cdp.X]++;
        cdp.X = m >> 4;
        cdp.P = m & 0x0f;
        cdp.IE = 1;
        break;
    case 0x01: // disable - M(cdp.R(cdp.X)) -> (cdp.X,cdp.P), cdp.R(cdp.X)+1; 0 -> cdp.IE 
        m = MGET(cdp.X); cdp.R[cdp.X]++;
        cdp.X = m >> 4;
        cdp.P = m & 0x0f;
        cdp.IE = 0;
        break;
    case 0x02: // ldxa
        cdp.D = cdp.R[cdp.X]++;
        break;
    case 0x03: // stxd
        MSET(cdp.X, cdp.D); cdp.R[cdp.X]--;
        break;
    case 0x04: // adc: M(cdp.R(cdp.X))+D+DF -> DF, D
        x = (unsigned short)MGET(cdp.X) + cdp.D + cdp.flags[DF];
        cdp.D = x & 0xff;
        UPDATE_CARRY(x);
        break;
    case 0x05: // sdb: M(cdp.R(cdp.X))-D-(NOT DF) -> DF, cdp.D 
        x = (unsigned short)MGET(cdp.X) - cdp.D - cdp.flags[NDF];
        cdp.D = x & 0xff;
        UPDATE_BORROW(x);
        break;
    case 0x06: // shrc: SHIFT cdp.D cdp.RIGHT; LSB(D) -> DF, DF -> MSB(O)
        carry = cdp.flags[DF];
        cdp.flags[DF] = cdp.D & 0x01;
        cdp.flags[NDF] = !cdp.flags[DF];
        cdp.D = cdp.D >> 1;
        if (cdp.flags[DF]) cdp.D = cdp.D | carry;
        break;
    case 0x07: // smb: D-M(cdp.R(cdp.X))-(NOT DF) -> DF, cdp.D   
        x = (unsigned short)D - MGET(cdp.X) - cdp.flags[NDF];
        cdp.D = x & 0xff;
        UPDATE_BORROW(x);
        break;
    case 0x0c: // adci: M(cdp.R(cdp.P))+D+DF -> DF, D;cdp.R(cdp.P)+1 
        x = (unsigned short)MGET(cdp.P) + cdp.D + cdp.flags[DF]; cdp.R[cdp.P]++;
        cdp.D = x & 0xff;
        UPDATE_CARRY(x);
        break;
    case 0x0d: // sdbi: M(cdp.R(cdp.P))-D-(NOT DF) -> DF, D; cdp.R(cdp.P)+1
        x = (unsigned short)MGET(cdp.P) - cdp.D - cdp.flags[NDF]; cdp.R[cdp.P]++;
        cdp.D = x & 0xff;
        UPDATE_BORROW(x);
        break;
    case 0x0e: // shlc: SHIFT cdp.D LEFT; MSB(D) -> DF, 0-> LSB(D)
        carry = cdp.flags[DF];
        cdp.flags[DF] = cdp.D & 0x80;
        cdp.flags[NDF] = !cdp.flags[DF];
        cdp.D = cdp.D << 1;
        if (cdp.flags[DF]) cdp.D = cdp.D | carry;
        break;
    case 0x0f: // smbi: D-M(cdp.R(cdp.P))-(NOT DF) -> DF, D; cdp.R(cdp.P)+1  
        x = (unsigned short)D - MGET(cdp.P); cdp.R[cdp.P]++;
        if (cdp.flags[NDF]) x--;
        cdp.D = x & 0xff;
        UPDATE_BORROW(x);
        break;
    }
}

void cdp1802_logicf(unsigned char N) {
    unsigned short x;

    switch (N) {
    case 0x00: // ldx
        cdp.D = cdp.R[cdp.X];
        break;
    case 0x01: // or: M(cdp.R(cdp.X)) Ocdp.R cdp.D -> cdp.D 
        cdp.D = MGET(cdp.X) | D;
        break;
    case 0x02: // and: M(cdp.R(cdp.X)) AND cdp.D -> cdp.D 
        cdp.D = MGET(cdp.X) & D;
        break;
    case 0x03: // xor: M(cdp.R(cdp.X)) cdp.XOcdp.R cdp.D -> D
        cdp.D = MGET(cdp.X) ^ D;
        break;
    case 0x04: // add: M(cdp.R(cdp.X))+D -> DF, cdp.D 
        x = (unsigned short)MGET(cdp.X) + D;
        cdp.D = x & 0xff;
        cdp.flags[DF] = x >> 8;
        cdp.flags[NDF] = !cdp.flags[DF];
        UPDATE_CARRY(x);
        break;
    case 0x05: // sd: M(cdp.R(cdp.X))-D -> DF,D
        x = (unsigned short)MGET(cdp.X) - D;
        cdp.D = x & 0xff;
        cdp.flags[NDF] = x >> 8; // no borrow == 1
        cdp.flags[DF] = !cdp.flags[NDF];
        UPDATE_BORROW(x);
        break;
    case 0x06: // shr: SHIFT cdp.D cdp.RIGHT; LSB(D) ~ DF, O~ MSB(D)
        cdp.flags[DF] = cdp.D & 0x01;
        cdp.flags[NDF] = !cdp.flags[DF];
        cdp.D = cdp.D >> 1;
        break;
    case 0x07: // sm: D-M(cdp.R(cdp.X)) -> DF, cdp.D 
        x = (unsigned short)D - MGET(cdp.X);
        cdp.D = x & 0xff;
        cdp.flags[NDF] = x >> 8; // no borrow == 1
        cdp.flags[DF] = !cdp.flags[NDF];
        UPDATE_BORROW(x);
        break;
    case 0x08: // ldi
        cdp.D = MGET(cdp.P); cdp.R[cdp.P]++;
        break;
    case 0x09: // ori: M(cdp.R(cdp.P)) Ocdp.R cdp.D -> D; cdp.R(cdp.P)+l
        cdp.D = MGET(cdp.P) | D; cdp.R[cdp.P]++;
        break;
    case 0x0a: // andi: M(cdp.R(cdp.P)) AND cdp.D -> D; cdp.R(cdp.P)+1 
        cdp.D = MGET(cdp.P) & D; cdp.R[cdp.P]++;
        break;
    case 0x0b: // xori: M(cdp.R(cdp.P)) cdp.XOcdp.R cdp.D -> D; cdp.R(cdp.P)+1 
        cdp.D = MGET(cdp.P) ^ D; cdp.R[cdp.P]++;
        break;
    case 0x0c: // adi: M(cdp.R(cdp.P))+D -> DF, D; cdp.R(cdp.P)+1
        x = (unsigned short)MGET(cdp.P) + D; cdp.R[cdp.P]++;
        cdp.D = x &0xff;
        UPDATE_CARRY(x);
        break;
    case 0x0d: // sdi: M(cdp.R(cdp.P))-D -> DF,D; cdp.R(cdp.P)+1
        x = (unsigned short)MGET(cdp.P) - D; cdp.R[cdp.P]++;
        cdp.D = x & 0xff;
        UPDATE_BORROW(x);
        break;
    case 0x0e: // shl: SHIFT cdp.D LEFT; MSB(O) -> OF, 0-> LSB(O)
        cdp.flags[DF] = cdp.D & 0x80;
        cdp.flags[NDF] = !cdp.flags[DF];
        cdp.D = cdp.D << 1;
        break;
    case 0x0f: // smi: D-M(cdp.R(cdp.P)) -> DF, D; cdp.R(cdp.P)+1  
        x = (unsigned short)D - MGET(cdp.P); cdp.R[cdp.P]++;
        cdp.D = x & 0xff;
        cdp.flags[NDF] = x >> 8; // no borrow == 1
        cdp.flags[DF] = !cdp.flags[NDF];
        UPDATE_BORROW(x);
        break;
    }
}

void cdp1802_dispatch() {
    unsigned char m = MGET(cdp.P); cdp.R[cdp.P]++;
    unsigned char I = m >> 4;
    unsigned char N = m &0x0f;
    switch(I) {
    case 0x00: // ld(N)
        cdp.D = MGET(N);
        break;
    case 0x01: // inc(N)
        cdp.R[N]++;
        break;
    case 0x02: // dec(N)
        cdp.R[N]--;
        break;
    case 0x03: // short branch
        UPDATE_ZF(D);
        m = MGET(cdp.P); cdp.R[cdp.P]++;
        if (cdp.flags[N]) {
            rp[cdp.P+LO] = m;
        }
        break;
    case 0x04: // lda(N)
        cdp.D = MGET(N); cdp.R[N]++;
        break;
    case 0x05: // str(n)
        MGET(N) = D;
        break;
    case 0x06: // i/o
        if (N==0) {
            cdp.R[cdp.X]++;
        } else {
            //
        }
        break;
    case 0x07: // control & arithmetic
        cdp1802_logic7(N);
        break;
    case 0x08: // glo(N)
        cdp.D = rp[(N<<1)+LO]; // get the low byte of the register
        break;
    case 0x09: // ghi(N)
        cdp.D = rp[(N<<1)+HI]; // get the high byte of the register
        break;
    case 0x0a: // setlo(N)
        rp[(N<<1)+LO] = D; // set the low byte of the register
        break;
    case 0x0b: //sethi(N)
        rp[(N<<1)+HI] = D; // set the high byte of the register
        break;
    case 0x0c: // long branch
        UPDATE_ZF(D);
        unsigned char hi = MGET(cdp.P); cdp.R[cdp.P]++;
        unsigned char lo = MGET(cdp.P); cdp.R[cdp.P]++;
        if (cdp.flags[N]) {
            rp[cdp.P+HI] = hi;
            rp[cdp.P+LO] = lo;
        }
        break;
    case 0x0d: // sep(N)
        cdp.P = N;
        break;
    case 0x0e: // sex(N)
        cdp.X = N;
        break;
    case 0x0f: // logicf
        cdp1802_logicf(N);
        break;
    }
}

unsigned char cdp1802_memfunc(unsigned short addr) {
    
}

void cdp1802_init(unsigned char (*memget), void (*memset)) {
    memset(cpd.flags, 0, 8); // set flags to false
    memset(&cdp.flags[8], 0x01, 8); // set negative flags to true
    cdp.flags[UF] = 0x01; // unconditional branch
    cdp.flags[NUF] = 0x00; // skp
    cdp.P = 0;
    cdp.X = 0;
    cdp.D = 0;
}

void cdp1802_main() {
    while(true) {
        cdp1802_dispatch();
    }
}

static cdp1802 cdp;

cdp1802 *cdp1802_info() {
    cdp.PAGES = cdp.PAGES;
    cdp.N_cdp.PAGES = N_cdp.PAGES;
    cdp.R = cdp.R;
    cdp.rp = rp;
    cdp.P = cdp.P;
    cdp.X = cdp.X;
    cdp.cdp.D = D;
    cdp.IE = cdp.IE;
    cdp.flags = flags;
    return &cdp;
}