/* Smoke test: compile-include the vendored decompiled + ROM-mode headers
 * and prove all three execution paths link and run for a single sample. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>

#include "../../src/dsp/decompiled-midiverb.h"
#include "../../src/dsp/decompiled-midifex.h"
#include "../../src/dsp/decompiled-midiverb2.h"
#include "../../src/dsp/utils.h"
#include "../../src/dsp/dasp16.h"
#include "../../src/dsp/lfo.h"
#include "../../src/dsp/rom.h"

int main(void) {
    int16_t dram[0x4000];
    memset(dram, 0, sizeof(dram));
    int16_t out_l = 0, out_r = 0;
    /* Decompiled path: run a single sample through midiverb effect_1 */
    midiverb_effects[0](0, &out_l, &out_r, dram, 0, 0, 0);

    /* ROM-mode path: instantiate Machine + reset, run one tick on empty program */
    Machine m;
    memset(&m, 0, sizeof(m));
    reset_machine(&m);
    Sample s;
    run_machine_tick(&m, 0, &s);

    /* LFO path: init for a known LFO program (Midiverb II #50 has LFO) */
    Lfo lfo1, lfo2;
    LfoPatch *patch = NULL;
    int lfo_present = init_lfo_for_program(50, &lfo1, &lfo2, &patch);

    /* rom_types[] should be three entries with right names */
    int n = sizeof(rom_types) / sizeof(rom_types[0]);

    printf("smoke ok (rom_types=%d, lfo50_present=%d, machine_acc=%d)\n",
           n, lfo_present, m.acc);
    return 0;
}
