/* Bisect runner: takes 23.4kHz stereo WAV, runs through effect_fn directly
 * with my exact int16 scaling code (no resampler, no HPF/LPF/width), writes
 * 23.4kHz output. If output sounds clean -> resampler is the bug. If
 * bitcrushed -> scaling/mono/mix is the bug.
 *
 * Mirrors upstream midiverb.c structure as closely as possible.
 *
 * Usage: ./mid_runner <unit:0|1|2> <program> <input_23k.wav> <output_23k.wav>
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sndfile.h>

#include "../../src/dsp/midiverb_core.h"

int main(int argc, char **argv) {
    if (argc != 5) {
        fprintf(stderr, "usage: %s <unit:0|1|2> <program> <input_23k.wav> <output_23k.wav>\n", argv[0]);
        return 1;
    }
    int unit = atoi(argv[1]);
    int program = atoi(argv[2]);
    const char *in_path = argv[3];
    const char *out_path = argv[4];

    SF_INFO si = {0};
    SNDFILE *in_sf = sf_open(in_path, SFM_READ, &si);
    if (!in_sf) { fprintf(stderr, "open %s\n", in_path); return 1; }
    if (si.channels != 2) { fprintf(stderr, "stereo required\n"); return 1; }

    SF_INFO so = si;
    SNDFILE *out_sf = sf_open(out_path, SFM_WRITE, &so);

    mv_instance_t inst;
    mv_instance_init(&inst);
    inst.unit = (mv_unit_t)unit;
    inst.program = program;
    inst.effect_fn = mv_dispatch_for(inst.unit, inst.program);

    float mix = 0.45f;

    int16_t buf[2];
    long total = 0;
    while (sf_readf_short(in_sf, buf, 1) == 1) {
        /* Mono sum (matches upstream: ((L+R)*0.5) clipped) */
        int32_t mono32 = ((int32_t)buf[0] + buf[1]) / 2;
        if (mono32 > 32767) mono32 = 32767;
        if (mono32 < -32768) mono32 = -32768;
        int16_t input_clipped = (int16_t)mono32;
        /* 13-bit downshift */
        int16_t input_mono = input_clipped >> 3;

        int16_t ol = 0, or_ = 0;
        inst.effect_fn(input_mono, &ol, &or_, inst.dram, inst.ptr,
                       inst.lfo1, inst.lfo2);
        inst.ptr = (inst.ptr + 1) & 0x3fff;
        if ((inst.ptr & 7) == 0) { inst.lfo1 += 1; inst.lfo2 += 2; }

        /* 13-bit -> 16-bit with clip */
        int32_t out_l = (int32_t)ol << 3;
        int32_t out_r = (int32_t)or_ << 3;

        /* Mix: wet*mix + dry*(1-mix) — match upstream exactly */
        int32_t mixed_l = (int32_t)(out_l * mix + buf[0] * (1.0f - mix));
        int32_t mixed_r = (int32_t)(out_r * mix + buf[1] * (1.0f - mix));
        if (mixed_l > 32767) mixed_l = 32767;
        if (mixed_l < -32768) mixed_l = -32768;
        if (mixed_r > 32767) mixed_r = 32767;
        if (mixed_r < -32768) mixed_r = -32768;
        int16_t out_buf[2] = { (int16_t)mixed_l, (int16_t)mixed_r };
        sf_writef_short(out_sf, out_buf, 1);
        total++;
    }
    fprintf(stderr, "processed %ld frames at 23.4k through unit=%d program=%d\n", total, unit, program);
    sf_close(in_sf);
    sf_close(out_sf);
    return 0;
}
