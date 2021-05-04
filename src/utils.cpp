#include <math.h>

// The svg input files usually has values that are slightly different than what was intended
// when creating them due to floating point errors. We currently round the input shapes to the
// nearest thousandth to correct this.
float RoundFloatingInput(float x) {
   return roundf(x * 1000) / 1000;
}

char* FindChar(char *str, char ch) {
    while (*str && *str != ch )
        str++;

    return str;
}

char* SkipChar(char *str, char ch) {
    str = FindChar(str, ch);
    str++;

    return str;
}