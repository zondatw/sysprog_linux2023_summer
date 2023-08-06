#include <stddef.h>
#include <stdio.h>

/*
Execute command: gcc struct_test.c -o struct_test && ./struct_test

Original struct is
```c
struct data {
    short a;
    char b;
    double c;
};
```

When only define this, it will display wrong value, because `structure padding`
will alloc more memory for variable, ex: char will allocate 4 bytes spaces.

Reference: https://hackmd.io/@sysprog/c-memory
*/

// Solution 1:
// #pragma pack(push, 1)
// struct data {
//     short a;
//     char b;
//     double c;
// };
// #pragma pack(pop)

// Solution 2:
// struct data {
//     short a;
//     char b;
//     double c;
// } __attribute__((packed));

struct data {
    short a;
    char b;
    double c;
};

int main()
{
    struct data x = {.a = 25, .b = 'A', .c = 12.45};
    char *p = (char *) &x;
    printf("a=%d\n", *((short *) p));
    p += sizeof(short);
    printf("b=%c\n", *((char *) p));
    // Solutoin 3:
    p = (char *) &x + offsetof(struct data, c);
    printf("c=%lf\n", *((double *) p));

    printf("p=%p, &x.c=%p\n", p, &(x.c));
    return 0;
}
