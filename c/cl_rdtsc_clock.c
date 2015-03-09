// use rdtsc to measure CPU clock rate
// Requirements:
//   constant TSC
//   a constant TSC keeps all TSC’s synchronized across all cores in a system,
//   and an invariant (or nonstop) TSC keeps the TSC running at a constant
//   rate regardless of changes in CPU frequency
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

static inline unsigned long long rdtsc_ll(void) {
    unsigned long long __h__, __l__;
    __asm__ __volatile__
        ("rdtsc" : "=d" (__h__), "=a" (__l__));
    return (__h__ << 32) | __l__;
}

int main()
{
    unsigned long long start, stop;
    struct timeval begin, end;
    unsigned long tmp;
    int i;

    start = rdtsc_ll();
    gettimeofday(&begin, NULL);

    for (i = 0; i < 1000000; i++) {}
    // sleep(1);

    stop = rdtsc_ll();
    gettimeofday(&end, NULL);
    tmp = (end.tv_sec - begin.tv_sec) * 1000000 + (end.tv_usec - begin.tv_usec);
    printf("%.2f\n", (stop - start) / (tmp * 1000.0));

    return 0;
}
