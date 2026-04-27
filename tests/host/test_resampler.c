/* End-to-end sanity test: 1 kHz sine at 44.1 -> 23.4 -> 44.1 should
 * preserve roughly its amplitude after a settling period.
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "../../src/dsp/resampler.h"
#include "../../src/dsp/resampler.c"

int main(void) {
    downsampler_t down;
    upsampler_t up;
    downsampler_init(&down);
    upsampler_init(&up);

    const int total_blocks = 200;
    const int block_44 = 128;
    float in[128], mid[160], out[256];
    double peak = 0.0;

    for (int b = 0; b < total_blocks; b++) {
        for (int i = 0; i < block_44; i++) {
            float t = (b * block_44 + i) / 44100.0f;
            in[i] = 0.5f * sinf(2.0f * (float)M_PI * 1000.0f * t);
        }
        int mid_n = downsampler_process(&down, in, block_44, mid, 160);
        upsampler_process(&up, mid, mid_n, out, block_44);
        if (b > 20) {
            for (int i = 0; i < block_44; i++) {
                double a = fabs(out[i]);
                if (a > peak) peak = a;
            }
        }
    }

    printf("peak after warmup = %.3f (expected ~0.5)\n", peak);
    if (peak < 0.4 || peak > 0.6) {
        fprintf(stderr, "FAIL: peak amplitude out of range\n");
        return 1;
    }
    printf("resampler ok\n");
    return 0;
}
