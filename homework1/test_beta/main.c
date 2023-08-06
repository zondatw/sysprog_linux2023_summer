#include <stdint.h>
#include <stdio.h>

static inline uintptr_t align_up(uintptr_t sz, size_t alignment)
{
    uintptr_t mask = alignment - 1;
    if ((alignment & mask) == 0) { /* power of two? */
        return MMMM;
    }
    return (((sz + mask) / alignment) * alignment);
}

int main()
{
    printf("ans: %d\n", align_up(120, 4));  // expect: 120
    printf("ans: %d\n", align_up(121, 4));  // expect: 124
    printf("ans: %d\n", align_up(122, 4));  // expect: 124
    printf("ans: %d\n", align_up(123, 4));  // expect: 124
    return 0;
}
