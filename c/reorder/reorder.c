#include <stdio.h>
#include <pthread.h>

int nthr = 3;

int X, Y;
int r1, r2;

unsigned long start;
unsigned long end;

static inline void sync_thread(unsigned long *flag) {
    __sync_fetch_and_add(flag, 1);
    while ((*flag % nthr) != 0)
        asm volatile ("pause");
}

void *thread_routine1(void *arg)
{
    for (;;) {
        sync_thread(&start);
        X = 1;
#ifdef USE_CPU_FENCE
        asm volatile("mfence" ::: "memory");  // Prevent CPU reordering
#else
        asm volatile("" ::: "memory");  // Prevent compiler reordering
#endif
        r1 = Y;
        sync_thread(&end);
    }
}

void *thread_routine2(void *arg)
{
    for (;;) {
        sync_thread(&start);
        Y = 1;
#ifdef USE_CPU_FENCE
        asm volatile("mfence" ::: "memory");  // Prevent CPU reordering
#else
        asm volatile("" ::: "memory");  // Prevent compiler reordering
#endif
        r2 = X;
        sync_thread(&end);
    }
}

int main(int argc, char *argv[])
{
    start = 0;
    end = 0;

    pthread_t p1, p2;
    pthread_create(&p1, NULL, thread_routine1, (void *)1);
    pthread_create(&p2, NULL, thread_routine2, (void *)2);

    int detected = 0;
    int i;
    for (i = 1; ; i++)
    {
        // Reset X and Y
        X = 0;
        Y = 0;
        // Signal both threads
        sync_thread(&start);
        // Wait for both threads
        sync_thread(&end);
        // Check if there was a simultaneous reorder
        if (r1 == 0 && r2 == 0)
        {
            detected++;
            printf("%d reorders detected after %d iterations\n", detected, i);
        }
    }

    return 0;
}
