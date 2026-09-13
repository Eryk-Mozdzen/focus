// gcc -I focus/include -Os -c focus/math.c
// gcc -I focus/include -Os -c tests/sincos.c
// gcc sincos.o math.o

#include <stdio.h>
#include <stdlib.h>

#include "focus/math.h"

int main(int argc, char **argv) {
    const float x = strtof(argv[1], NULL);
    const float y = focus_math_sin(x);
    const float z = focus_math_cos(x);
    printf("%f\n", y);
    printf("%f\n", z);
    return 0;
}
