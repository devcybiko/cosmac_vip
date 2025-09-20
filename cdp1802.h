typedef struct _cdp1802 {
    unsigned char **PAGES;
    unsigned int N_PAGES;
    unsigned short *R;
    unsigned char *rp;
    unsigned short P;
    unsigned short X;
    unsigned char D;
    unsigned char IE;
    unsigned char *flags;
} cdp1802;

extern void cdp1802_init(unsigned char **pages, unsigned int n_pages);
extern void cdp1802_dispatch();
extern cdp1802* cdp1802_info();