typedef struct _cdp1802 {
    unsigned char *M;
    unsigned short *R;
    unsigned char *rp;
    unsigned short P;
    unsigned short X;
    unsigned char D;
    unsigned char IE;
    unsigned char *flags;
} cdp1802;

extern void cdp1802_init();
extern void cdp1802_dispatch();
extern cdp1802* cdp1802_info();