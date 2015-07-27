/* public domain, 2013 */

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define R(a,b) (((a) << (b)) | ((a) >> (32 - (b))))
static void salsa_scrambler(uint32_t out[16], uint32_t x[16])
{
    int i;
    /* This is a quickly mutilated Salsa20 of only 1 round */
    x[ 4] ^= R(x[ 0] + x[12],  7);
    x[ 8] ^= R(x[ 4] + x[ 0],  9);
    x[12] ^= R(x[ 8] + x[ 4], 13);
    x[ 0] ^= R(x[12] + x[ 8], 18);
    x[ 9] ^= R(x[ 5] + x[ 1],  7);
    x[13] ^= R(x[ 9] + x[ 5],  9);
    x[ 1] ^= R(x[13] + x[ 9], 13);
    x[ 5] ^= R(x[ 1] + x[13], 18);
    x[14] ^= R(x[10] + x[ 6],  7);
    x[ 2] ^= R(x[14] + x[10],  9);
    x[ 6] ^= R(x[ 2] + x[14], 13);
    x[10] ^= R(x[ 6] + x[ 2], 18);
    x[ 3] ^= R(x[15] + x[11],  7);
    x[ 7] ^= R(x[ 3] + x[15],  9);
    x[11] ^= R(x[ 7] + x[ 3], 13);
    x[15] ^= R(x[11] + x[ 7], 18);
    for (i = 0; i < 16; ++i)
        out[i] = x[i];
}

#define CHUNK 2048

int main(void)
{
    uint32_t bufA[CHUNK];
    uint32_t bufB[CHUNK];
    uint32_t *input = bufA, *output = bufB;
    int i;

    /* Initialize seed */
    srand(time(NULL));
    for (i = 0; i < CHUNK; i++)
        input[i] = rand();

    while (1) {
        for (i = 0; i < CHUNK/16; i++) {
            salsa_scrambler(output + 16*i, input + 16*i);
        }
        write(1, output, sizeof(bufA));

        {
            uint32_t *tmp = output;
            output = input;
            input = tmp;
        }
    }
    return 0;
}
