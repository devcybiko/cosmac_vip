typedef struct _cdp1802 {
    unsigned char (*mget)(unsigned short addr);
    unsigned char (*mset)(unsigned short addr, unsigned char byte);
    unsigned short R[16];
    unsigned char *rp;
    unsigned short P;
    unsigned short X;
    unsigned char D;
    unsigned char IE;
    unsigned char flags[16]; // 1, z, q, df, ef0-3, 0, ~z, ~q, ~df, ~ef0-~ef3
} cdp1802;

extern void cdp1802_init(unsigned char **pages, unsigned int n_pages);
extern void cdp1802_dispatch();
extern cdp1802* cdp1802_info();