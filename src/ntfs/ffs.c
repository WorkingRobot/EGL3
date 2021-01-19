#include "ffs.h"

int ffs(int i) {
    int bit;

    if (0 == i)
        return 0;

    for (bit = 1; !(i & 1); ++bit)
        i >>= 1;
    return bit;
}