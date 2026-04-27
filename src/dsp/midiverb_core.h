#ifndef MIDIVERB_CORE_H
#define MIDIVERB_CORE_H

#include <stdint.h>

#include "resampler.h"

#define MIDIVERB_DRAM_LEN 0x4000

typedef enum {
    MV_UNIT_MIDIVERB  = 0,
    MV_UNIT_MIDIFEX   = 1,
    MV_UNIT_MIDIVERB2 = 2,
    MV_UNIT_COUNT
} mv_unit_t;

typedef void (*mv_effect_fn)(int16_t input, int16_t *out_left, int16_t *out_right,
                             int16_t *DRAM, int ptr,
                             uint32_t lfo1_value, uint32_t lfo2_value);

typedef struct {
    int   pending_unit;
    int   pending_program;
    int   reset_dsp_state;

    mv_unit_t    unit;
    int          program;
    mv_effect_fn effect_fn;

    int16_t  dram[MIDIVERB_DRAM_LEN];
    int      ptr;
    uint32_t lfo1, lfo2;

    float mix;
    float feedback;
    float input_gain;
    float output_gain;
    float predelay_ms;
    float low_cut_hz;
    float high_cut_hz;
    float width;

    downsampler_t down_l, down_r;
    upsampler_t   up_l, up_r;

    float predelay_buf[44100 / 4];
    int   predelay_pos;

    float hpf_l, hpf_r;
    float lpf_l, lpf_r;

    float fb_l, fb_r;
} mv_instance_t;

void          mv_instance_init(mv_instance_t *inst);
const char*   mv_unit_name(mv_unit_t u);
int           mv_program_count(mv_unit_t u);
const char*   mv_program_name(mv_unit_t u, int idx);
mv_effect_fn  mv_dispatch_for(mv_unit_t u, int prog);

#endif
