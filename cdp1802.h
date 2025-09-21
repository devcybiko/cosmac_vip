
/**
 * NOTE: This struct is returned by cdp1802_info()
 * The pointers / arrays will always be "current" since they're pointers to arrays
 * But the scalars (mget, mset, P, X, D, IE) will not.
 * To update the scalars call cdp1802_info()
 * Likewise, modifying the scalars will have no effect on the emulator
 * BUT - modifying the arrays will have an immediate effect.
 */
typedef struct _cdp1802 {
    unsigned char (*mget)(unsigned short addr);
    void (*mset)(unsigned short addr, unsigned char byte);
    unsigned short *R; // R[16]
    unsigned short P;
    unsigned short X;
    unsigned char D;
    unsigned char IE;
    unsigned char *flags; // flags[16] 1, z, q, df, ef1-4, 0, ~z, ~q, ~df, ~ef1-~ef4
    unsigned short *debug; // debug[16]
} cdp1802;

extern cdp1802 *cdp1802_init(unsigned char (*mget)(unsigned short addr), void (*mset)(unsigned short addr, unsigned char byte));
extern void cdp1802_dispatch();
extern cdp1802* cdp1802_info();

#define _UF 0 // unconditional branch
#define _QF 1
#define _ZF 2
#define _DF 3
#define _EF1 4
#define _EF2 5
#define _EF3 6
#define _EF4 7