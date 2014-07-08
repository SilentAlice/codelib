#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define GLIBC_QSORT 1

#define WRONG_CMP   1

unsigned char chinadns[][4] = {
    { 74,125,127,102 },
    { 74,125,155,102 },
    { 74,125,39,102 },
    { 74,125,39,113 },
    { 209,85,229,138 },
    { 128,121,126,139 },
    { 159,106,121,75 },
    { 169,132,13,103 },
    { 192,67,198,6 },
    { 202,106,1,2 },
    { 202,181,7,85 },
    { 203,161,230,171 },
    { 203,98,7,65 },
    { 207,12,88,98 },
    { 208,56,31,43 },
    { 209,145,54,50 },
    { 209,220,30,174 },
    { 209,36,73,33 },
    { 211,94,66,147 },
    { 213,169,251,35 },
    { 216,221,188,182 },
    { 216,234,179,13 },
    { 243,185,187,39 },
    { 37,61,54,158 },
    { 4,36,66,178 },
    { 46,82,174,68 },
    { 59,24,3,173 },
    { 64,33,88,161 },
    { 64,33,99,47 },
    { 64,66,163,251 },
    { 65,104,202,252 },
    { 65,160,219,113 },
    { 66,45,252,237 },
    { 72,14,205,104 },
    { 72,14,205,99 },
    { 78,16,49,15 },
    { 8,7,198,45 },
    { 93,46,8,89 },
};

void uclibc_qsort(void  *base,
                  size_t nel,
                  size_t width,
                  int (*comp)(const void *, const void *))
{
    size_t wgap, i, j, k;
    char tmp;

    if ((nel > 1) && (width > 0)) {
        assert(nel <= ((size_t)(-1)) / width); /* check for overflow */
        wgap = 0;
        do {
            wgap = 3 * wgap + 1;
        } while (wgap < (nel-1)/3);
        /* From the above, we know that either wgap == 1 < nel or */
        /* ((wgap-1)/3 < (int) ((nel-1)/3) <= (nel-1)/3 ==> wgap <  nel. */
        wgap *= width;			/* So this can not overflow if wnel doesn't. */
        nel *= width;			/* Convert nel to 'wnel' */
        do {
            i = wgap;
            do {
                j = i;
                do {
                    register char *a;
                    register char *b;

                    j -= wgap;
                    a = j + ((char *)base);
                    b = a + wgap;
                    if ((*comp)(a, b) <= 0) {
                        break;
                    }
                    k = width;
                    do {
                        tmp = *a;
                        *a++ = *b;
                        *b++ = tmp;
                    } while (--k);
                } while (j >= wgap);
                i += width;
            } while (i < nel);
            wgap = (wgap - width)/3;
        } while (wgap);
    }
}

int cmp(const void *a, const void *b) {
    unsigned int *aa = (unsigned int *)a;
    unsigned int *bb = (unsigned int *)b;

#if WRONG_CMP
    /* It's wrong to simply return the *aa - *bb */
    return *aa - *bb;
#else
    if (*aa > *bb)
        return 1;
    if (*aa == *bb)
        return 0;
    return -1;
#endif
}

int main() {
    int i;

#if GLIBC_QSORT
    qsort(chinadns, 38, sizeof(chinadns[0]), cmp);
#else
    uclibc_qsort(chinadns, 38, sizeof(chinadns[0]), cmp);
#endif

    for (i = 0; i < 38; i++) {
        printf("%08x %d.%d.%d.%d\n", *(int *)(chinadns[i]),
               chinadns[i][0],
               chinadns[i][1],
               chinadns[i][2],
               chinadns[i][3]);
    }

    unsigned char target[] = { 59,24,3,173 };
    printf("%s\n", bsearch(target, chinadns, 38, sizeof(chinadns[0]), cmp)?"yes":"no");

    return 0;
}
