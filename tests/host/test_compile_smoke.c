/* Smoke test: just compile-include the vendored headers and link. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../../src/dsp/decompiled-midiverb.h"
#include "../../src/dsp/decompiled-midifex.h"
#include "../../src/dsp/decompiled-midiverb2.h"

int main(void) {
    int16_t dram[0x4000];
    memset(dram, 0, sizeof(dram));
    int16_t out_l = 0, out_r = 0;
    /* Run a single sample through midiverb effect_1 to prove linkage */
    midiverb_effects[0](0, &out_l, &out_r, dram, 0, 0, 0);
    printf("smoke ok\n");
    return 0;
}
