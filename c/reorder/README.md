# Synopsis

This is a demo C program to demonstrate CPU memory access reordering.

# Description

Due to out-of-order execution of modern CPUs, a read operation and a write operation targetting different memory addresses can be reordered. For more detail please refer to Intel's document.

This program is originally invented by [Jeff Preshing](http://preshing.com/20120515/memory-reordering-caught-in-the-act/)

# Usage

Compile the program with/without the `-DUSE_CPU_FENCE` flag to enable/disable `mfence` instruction. You can replace `mfence` instruction with `__sync_synchronize()` builtin function of GCC.

    gcc -g -Wall -DUSE_CPU_FENCE reorder.c -o reorder
    
    gcc -g -Wall reorder.c -o reorder

And the mfence-disabled version will print detected reordering info like

    1 reorders detected after 2 iterations
    2 reorders detected after 3 iterations
    3 reorders detected after 4 iterations
    4 reorders detected after 7 iterations
    5 reorders detected after 8 iterations
    6 reorders detected after 9 iterations
    7 reorders detected after 13 iterations
    8 reorders detected after 14 iterations
    9 reorders detected after 16 iterations
    10 reorders detected after 17 iterations
    11 reorders detected after 19 iterations
    12 reorders detected after 21 iterations
