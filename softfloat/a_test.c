#include <stdio.h>
#include "f16_mul.c" 

int main() {
    union ui16_f16 a;
    a.ui = 10;
    union ui16_f16 b;
    b.ui = 10;
    float16_t c = f16_mul(a.f, b.f);
    printf("%f\n", c);
    return 0;
}