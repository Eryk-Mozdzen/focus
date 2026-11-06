/*
    gcc generate_math_lookup_sin.c -lm -o generate_math_lookup_sin
    ./generate_math_lookup_sin 512 focus/math_lookup_sin.c
*/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if(argc != 3) {
        fprintf(stderr, "wrong number of arguments\n");
        return -1;
    }

    const int num = strtol(argv[1], NULL, 10);
    const char *filename = argv[2];

    FILE *file = fopen(filename, "w");

    if(file == NULL) {
        fprintf(stderr, "error opening file: %s\n", filename);
        return -2;
    }

    fprintf(file, "/*\n");
    fprintf(file, "    File generated automatically via command:\n");
    fprintf(file, "   ");
    for(int i = 0; i < argc; i++) {
        fprintf(file, " %s", argv[i]);
    }
    fprintf(file, "\n\n");
    fprintf(file, "    Do not modify manually!\n");
    fprintf(file, "*/\n\n");

    fprintf(file, "#ifndef FOCUS_MATH_LOOKUP_SIN_H\n");
    fprintf(file, "#define FOCUS_MATH_LOOKUP_SIN_H\n\n");

    fprintf(file, "const int focus_math_lookup_sin_num = %d;\n\n", num);

    fprintf(file, "const float focus_math_lookup_sin[%d] = {", num + 1);
    for(int i = 0; i < (num + 1); i++) {
        if((i % 6) == 0) {
            fprintf(file, "\n   ");
        }

        const double arg = (0.5 * M_PI) * (((double)i) / ((double)num));
        const double sine = sin(arg);

        fprintf(file, " %12.10ff,", sine);
    }
    fprintf(file, "\n};\n\n");

    fprintf(file, "#endif\n");

    fclose(file);

    return 0;
}
