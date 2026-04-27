#include "midiverb_core.h"

#include <string.h>

#include "decompiled-midiverb.h"
#include "decompiled-midifex.h"
#include "decompiled-midiverb2.h"

static const char* unit_names[MV_UNIT_COUNT] = {
    "Midiverb", "Midifex", "Midiverb II",
};

static const char* midiverb_program_names[] = {
#include "names-midiverb.h"
};
static const char* midifex_program_names[] = {
#include "names-midifex.h"
};
static const char* midiverb2_program_names[] = {
#include "names-midiverb2.h"
};

static int program_counts[MV_UNIT_COUNT] = {
    sizeof(midiverb_program_names)  / sizeof(midiverb_program_names[0]),
    sizeof(midifex_program_names)   / sizeof(midifex_program_names[0]),
    sizeof(midiverb2_program_names) / sizeof(midiverb2_program_names[0]),
};

static mv_effect_fn dispatch_for(mv_unit_t u, int prog) {
    int max = program_counts[u];
    if (prog < 0) prog = 0;
    if (prog >= max) prog = max - 1;
    switch (u) {
        case MV_UNIT_MIDIVERB:  return midiverb_effects[prog];
        case MV_UNIT_MIDIFEX:   return midifex_effects[prog];
        case MV_UNIT_MIDIVERB2: return midiverb2_effects[prog];
        default:                return midiverb_effects[0];
    }
}

void mv_instance_init(mv_instance_t *inst) {
    memset(inst, 0, sizeof(*inst));
    inst->unit = MV_UNIT_MIDIVERB;
    inst->program = 0;
    inst->effect_fn = dispatch_for(inst->unit, inst->program);
    inst->mix = 0.35f;
    inst->feedback = 0.0f;
    inst->input_gain = 1.0f;
    inst->output_gain = 1.0f;
    inst->predelay_ms = 0.0f;
    inst->low_cut_hz = 20.0f;
    inst->high_cut_hz = 20000.0f;
    inst->width = 1.0f;
    inst->pending_unit = -1;
    inst->pending_program = -1;
    downsampler_init(&inst->down_l);
    downsampler_init(&inst->down_r);
    upsampler_init(&inst->up_l);
    upsampler_init(&inst->up_r);
}

const char* mv_unit_name(mv_unit_t u) {
    if ((int)u < 0 || u >= MV_UNIT_COUNT) return "?";
    return unit_names[u];
}

int mv_program_count(mv_unit_t u) {
    if ((int)u < 0 || u >= MV_UNIT_COUNT) return 0;
    return program_counts[u];
}

const char* mv_program_name(mv_unit_t u, int idx) {
    if (idx < 0 || idx >= mv_program_count(u)) return "?";
    switch (u) {
        case MV_UNIT_MIDIVERB:  return midiverb_program_names[idx];
        case MV_UNIT_MIDIFEX:   return midifex_program_names[idx];
        case MV_UNIT_MIDIVERB2: return midiverb2_program_names[idx];
        default:                return "?";
    }
}

mv_effect_fn mv_dispatch_for(mv_unit_t u, int prog) {
    return dispatch_for(u, prog);
}
