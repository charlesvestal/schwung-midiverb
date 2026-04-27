#ifndef MIDIVERB_CORE_H
#define MIDIVERB_CORE_H

#include <stdint.h>
#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>

#include "resampler.h"
#include "utils.h"
#include "dasp16.h"
#include "lfo.h"

#define MIDIVERB_DRAM_LEN 0x4000
#define MV_MODULE_DIR_LEN 256
#define MV_STATUS_LEN 64

typedef enum {
    MV_UNIT_MIDIVERB  = 0,
    MV_UNIT_MIDIFEX   = 1,
    MV_UNIT_MIDIVERB2 = 2,
    MV_UNIT_COUNT
} mv_unit_t;

typedef enum {
    MV_SOURCE_DECOMPILED = 0,
    MV_SOURCE_ROM        = 1,
} mv_source_t;

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

    int16_t  dram[MIDIVERB_DRAM_LEN];   /* used in decompiled mode */
    int      ptr;

    /* LFO state — used by both decompiled (for has_lfo units) and ROM modes,
     * matching upstream's main loop semantics. */
    Lfo         lfo1, lfo2;
    LfoPatch   *lfo_patch;
    int         lfo_active;
    uint32_t    lfo1_val, lfo2_val;
    uint64_t    sample_counter;         /* for /8-rate LFO updates */

    /* ROM-mode state (only touched when source == MV_SOURCE_ROM) */
    Machine     machine;

    /* Source selection + status */
    mv_source_t source;
    char        module_dir[MV_MODULE_DIR_LEN];
    char        rom_status[MV_STATUS_LEN];

    float mix;
    float feedback;
    float input_gain;
    float output_gain;
    float predelay_ms;
    float low_cut_hz;
    float high_cut_hz;
    float width;
    float damping;
    float saturation;
    float tilt;
    float lfo_rate;
    float lfo_depth;

    downsampler_t down_l, down_r;
    upsampler_t   up_l, up_r;

    float hpf_l, hpf_r;
    float lpf_l, lpf_r;

    float fb_l, fb_r;
    float fb_lpf_l, fb_lpf_r;            /* one-pole damping in feedback path */
} mv_instance_t;

void          mv_instance_init(mv_instance_t *inst);
const char*   mv_unit_name(mv_unit_t u);
int           mv_program_count(mv_unit_t u);
const char*   mv_program_name(mv_unit_t u, int idx);
mv_effect_fn  mv_dispatch_for(mv_unit_t u, int prog);

/* Try to load ROM for the current unit/program. Returns 1 on success
 * (sets inst->source = MV_SOURCE_ROM), 0 on failure (no change). Updates
 * inst->rom_status either way. Safe to call from the audio thread (does
 * not exit on failure). */
int           mv_try_load_rom(mv_instance_t *inst);

/* Cheap existence + size check for a unit's ROM file (no parsing).
 * Returns 1 if file present at expected size, 0 otherwise. */
int           mv_unit_has_rom(const mv_instance_t *inst, mv_unit_t u);

/* (Re)initialize LFO state for the current unit/program. Mirrors upstream
 * main loop's `if (rom_type->has_lfo) init_lfo_for_program(...)` step.
 * Called from apply_pending after unit/program changes. */
void          mv_init_lfo(mv_instance_t *inst);

/* Apply current rom file naming convention */
const char*   mv_rom_filename(mv_unit_t u);

/* Map mv_unit_t to upstream's rom_types[] index */
int           mv_rom_type_index(mv_unit_t u);

#endif
