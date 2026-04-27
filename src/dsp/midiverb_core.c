#include "midiverb_core.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "decompiled-midiverb.h"
#include "decompiled-midifex.h"
#include "decompiled-midiverb2.h"
#include "rom.h"

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
    inst->source = MV_SOURCE_DECOMPILED;
    snprintf(inst->rom_status, MV_STATUS_LEN, "Decompiled (no ROM loaded)");
    inst->mix = 0.35f;
    inst->feedback = 0.0f;
    inst->input_gain = 1.0f;
    inst->output_gain = 1.0f;
    inst->predelay_ms = 0.0f;
    inst->low_cut_hz = 20.0f;
    inst->high_cut_hz = 20000.0f;
    inst->width = 1.0f;
    inst->damping = 0.0f;
    inst->saturation = 0.0f;
    inst->tilt = 0.0f;
    inst->lfo_rate = 1.0f;
    inst->lfo_depth = 1.0f;
    inst->pending_unit = -1;
    inst->pending_program = -1;
    downsampler_init(&inst->down_l);
    downsampler_init(&inst->down_r);
    upsampler_init(&inst->up_l);
    upsampler_init(&inst->up_r);
}

const char* mv_rom_filename(mv_unit_t u) {
    switch (u) {
        case MV_UNIT_MIDIVERB:  return "midiverb.rom";
        case MV_UNIT_MIDIFEX:   return "midifex.rom";
        case MV_UNIT_MIDIVERB2: return "midiverb2.rom";
        default:                return NULL;
    }
}

int mv_rom_type_index(mv_unit_t u) {
    /* rom_types[] order in upstream rom.h:
     *   [0] MidiVerb   16384B  programs 1..64
     *   [1] MidiFex    16384B  programs 1..64
     *   [2] MidiVerb 2 32768B  programs 0..99
     */
    switch (u) {
        case MV_UNIT_MIDIVERB:  return 0;
        case MV_UNIT_MIDIFEX:   return 1;
        case MV_UNIT_MIDIVERB2: return 2;
        default:                return -1;
    }
}

int mv_unit_has_rom(const mv_instance_t *inst, mv_unit_t u) {
    if (!inst || inst->module_dir[0] == '\0') return 0;
    int idx = mv_rom_type_index(u);
    const char *fname = mv_rom_filename(u);
    if (idx < 0 || !fname) return 0;
    char path[MV_MODULE_DIR_LEN + 32];
    snprintf(path, sizeof(path), "%s/roms/%s", inst->module_dir, fname);
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    return st.st_size == (off_t)rom_types[idx].length;
}

/* Safe ROM loader — does NOT call exit() on any failure path. Returns 1
 * if rom was successfully loaded into inst->machine, 0 otherwise.
 * Always updates inst->rom_status. */
int mv_try_load_rom(mv_instance_t *inst) {
    if (!inst || inst->module_dir[0] == '\0') {
        snprintf(inst->rom_status, MV_STATUS_LEN, "Decompiled (no module dir)");
        return 0;
    }
    int idx = mv_rom_type_index(inst->unit);
    const char *fname = mv_rom_filename(inst->unit);
    if (idx < 0 || !fname) return 0;
    RomType *rt = &rom_types[idx];

    char path[MV_MODULE_DIR_LEN + 32];
    snprintf(path, sizeof(path), "%s/roms/%s", inst->module_dir, fname);

    struct stat st;
    if (stat(path, &st) != 0) {
        snprintf(inst->rom_status, MV_STATUS_LEN,
                 "Decompiled (no %s)", fname);
        return 0;
    }
    if (st.st_size != (off_t)rt->length) {
        snprintf(inst->rom_status, MV_STATUS_LEN,
                 "Decompiled (%s wrong size)", fname);
        return 0;
    }

    FILE *f = fopen(path, "rb");
    if (!f) {
        snprintf(inst->rom_status, MV_STATUS_LEN,
                 "Decompiled (%s open fail)", fname);
        return 0;
    }

    /* Optional signature check (non-fatal mismatch) */
    uint8_t sig[4];
    if (fread(sig, 1, 4, f) != 4 || memcmp(sig, rt->signature, 4) != 0) {
        /* Not a hard error — some ROM variants may differ; log + continue. */
    }

    /* Read program bytecode for the current program */
    int program_index = inst->program;  /* our program is already 0-indexed */
    long offset = (long)rt->offset_to_bytecode + (long)program_index * ProgramLength;
    if (fseek(f, offset, SEEK_SET) != 0 ||
        fread(inst->machine.program, 1, ProgramLength, f) != ProgramLength) {
        fclose(f);
        snprintf(inst->rom_status, MV_STATUS_LEN,
                 "Decompiled (%s read fail)", fname);
        return 0;
    }

    /* Interpolation patch table for units with LFO (Midiverb II only) */
    if (rt->has_lfo && rt->offset_to_interpolation_patch_table) {
        if (fseek(f, rt->offset_to_interpolation_patch_table, SEEK_SET) != 0 ||
            fread(inst->machine.interpolation_patch_table, 1,
                  InterpolationPatchTableLength, f)
                != InterpolationPatchTableLength) {
            fclose(f);
            snprintf(inst->rom_status, MV_STATUS_LEN,
                     "Decompiled (%s patch table fail)", fname);
            return 0;
        }
    }
    fclose(f);

    /* Reset machine state, set memory_shift from the rom type */
    inst->machine.address = 0;
    inst->machine.acc = 0;
    inst->machine.memory_shift = rt->memory_shift;
    memset(inst->machine.dram, 0, sizeof(inst->machine.dram));

    /* Init LFO if program uses one (uses upstream's 1-indexed program number) */
    int prog_no = inst->program + rt->first_program_number;
    inst->rom_lfo_active = init_lfo_for_program(
        prog_no, &inst->rom_lfo1, &inst->rom_lfo2, &inst->rom_lfo_patch);
    inst->rom_lfo1_val = 0;
    inst->rom_lfo2_val = 0;

    inst->source = MV_SOURCE_ROM;
    snprintf(inst->rom_status, MV_STATUS_LEN, "ROM (%s)", rt->name);
    return 1;
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
