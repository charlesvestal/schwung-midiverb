/* schwung-midiverb plugin entry */
#include "audio_fx_api_v2.h"
#include "midiverb_core.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

static const host_api_v1_t *g_host = NULL;

static void apply_pending(mv_instance_t *inst) {
    int dirty = 0;
    if (inst->pending_unit >= 0) {
        if (inst->pending_unit != (int)inst->unit) {
            inst->unit = (mv_unit_t)inst->pending_unit;
            int max = mv_program_count(inst->unit);
            if (inst->program >= max) inst->program = max - 1;
            dirty = 1;
        }
        inst->pending_unit = -1;
    }
    if (inst->pending_program >= 0) {
        int max = mv_program_count(inst->unit);
        int p = inst->pending_program;
        if (p >= max) p = max - 1;
        if (p != inst->program) {
            inst->program = p;
            dirty = 1;
        }
        inst->pending_program = -1;
    }
    if (inst->reset_dsp_state || dirty) {
        memset(inst->dram, 0, sizeof(inst->dram));
        inst->fb_l = inst->fb_r = 0.0f;
        inst->effect_fn = mv_dispatch_for(inst->unit, inst->program);
        inst->reset_dsp_state = 0;
    }
}

static void* mv_create(const char *module_dir, const char *config_json) {
    (void)module_dir; (void)config_json;
    mv_instance_t *inst = (mv_instance_t*)calloc(1, sizeof(mv_instance_t));
    if (!inst) return NULL;
    mv_instance_init(inst);
    return inst;
}

static void mv_destroy(void *vp) {
    free(vp);
}

static inline float onepole_coef(float fc) {
    return expf(-2.0f * (float)M_PI * fc / 23400.0f);
}

static void mv_process(void *vp, int16_t *audio_inout, int frames) {
    mv_instance_t *inst = (mv_instance_t*)vp;
    apply_pending(inst);

    static float in_l[256], in_r[256], mid_l[160], mid_r[160], out_l[256], out_r[256];
    static float wet_l[160], wet_r[160];
    if (frames > 256) frames = 256;
    for (int i = 0; i < frames; i++) {
        in_l[i] = audio_inout[2*i + 0] / 32768.0f;
        in_r[i] = audio_inout[2*i + 1] / 32768.0f;
    }

    int n_mid_l = downsampler_process(&inst->down_l, in_l, frames, mid_l, 160);
    int n_mid_r = downsampler_process(&inst->down_r, in_r, frames, mid_r, 160);
    int n_mid = n_mid_l < n_mid_r ? n_mid_l : n_mid_r;

    float a_hp = onepole_coef(inst->low_cut_hz);
    float a_lp = onepole_coef(inst->high_cut_hz);

    for (int i = 0; i < n_mid; i++) {
        float mono = (mid_l[i] + mid_r[i]) * 0.5f * inst->input_gain;
        mono += (inst->fb_l + inst->fb_r) * inst->feedback * 0.5f;

        inst->hpf_l = a_hp * inst->hpf_l + (1.0f - a_hp) * mono;
        float hp = mono - inst->hpf_l;

        float clamped = hp;
        if (clamped >  1.0f) clamped =  1.0f;
        if (clamped < -1.0f) clamped = -1.0f;
        int16_t in16 = (int16_t)(clamped * 32767.0f);
        int16_t ol = 0, or_ = 0;
        inst->effect_fn(in16, &ol, &or_, inst->dram, inst->ptr,
                        inst->lfo1, inst->lfo2);
        inst->ptr = (inst->ptr + 1) & 0x3fff;
        inst->lfo1 += 1;
        inst->lfo2 += 2;

        float wl = ol / 32768.0f;
        float wr = or_ / 32768.0f;
        inst->lpf_l = a_lp * inst->lpf_l + (1.0f - a_lp) * wl;
        inst->lpf_r = a_lp * inst->lpf_r + (1.0f - a_lp) * wr;
        wl = inst->lpf_l;
        wr = inst->lpf_r;

        float mid = 0.5f * (wl + wr);
        float side = 0.5f * (wl - wr) * inst->width;
        wl = mid + side;
        wr = mid - side;

        wet_l[i] = wl;
        wet_r[i] = wr;
        inst->fb_l = wl;
        inst->fb_r = wr;
    }

    upsampler_process(&inst->up_l, wet_l, n_mid, out_l, frames);
    upsampler_process(&inst->up_r, wet_r, n_mid, out_r, frames);

    float mix = inst->mix;
    float og = inst->output_gain;
    for (int i = 0; i < frames; i++) {
        float l = (1.0f - mix) * in_l[i] + mix * out_l[i];
        float r = (1.0f - mix) * in_r[i] + mix * out_r[i];
        l *= og; r *= og;
        if (l >  1.0f) l =  1.0f;
        if (l < -1.0f) l = -1.0f;
        if (r >  1.0f) r =  1.0f;
        if (r < -1.0f) r = -1.0f;
        audio_inout[2*i + 0] = (int16_t)(l * 32767.0f);
        audio_inout[2*i + 1] = (int16_t)(r * 32767.0f);
    }
}

static void mv_set_param(void *vp, const char *key, const char *val) {
    mv_instance_t *inst = (mv_instance_t*)vp;
    if (!inst || !key || !val) return;
    if (strcmp(key, "unit") == 0) {
        int u = atoi(val);
        if (u >= 0 && u < MV_UNIT_COUNT) inst->pending_unit = u;
    } else if (strcmp(key, "program") == 0) {
        inst->pending_program = atoi(val);
    } else if (strcmp(key, "mix") == 0) {
        inst->mix = atof(val);
    } else if (strcmp(key, "feedback") == 0) {
        inst->feedback = atof(val);
    } else if (strcmp(key, "input_gain") == 0) {
        inst->input_gain = atof(val);
    } else if (strcmp(key, "output_gain") == 0) {
        inst->output_gain = atof(val);
    } else if (strcmp(key, "predelay_ms") == 0) {
        inst->predelay_ms = atof(val);
    } else if (strcmp(key, "low_cut_hz") == 0) {
        inst->low_cut_hz = atof(val);
    } else if (strcmp(key, "high_cut_hz") == 0) {
        inst->high_cut_hz = atof(val);
    } else if (strcmp(key, "width") == 0) {
        inst->width = atof(val);
    }
}

static int mv_get_param(void *vp, const char *key, char *buf, int buf_len) {
    mv_instance_t *inst = (mv_instance_t*)vp;
    if (!inst || !key || !buf || buf_len <= 0) return -1;
    int n = -1;

    if (strcmp(key, "unit") == 0) {
        n = snprintf(buf, buf_len, "%d", (int)inst->unit);
    } else if (strcmp(key, "program") == 0) {
        n = snprintf(buf, buf_len, "%d", inst->program);
    } else if (strcmp(key, "program_count") == 0) {
        n = snprintf(buf, buf_len, "%d", mv_program_count(inst->unit));
    } else if (strcmp(key, "program_name") == 0) {
        n = snprintf(buf, buf_len, "%s", mv_program_name(inst->unit, inst->program));
    } else if (strcmp(key, "unit_list") == 0) {
        n = snprintf(buf, buf_len,
            "[{\"index\":0,\"label\":\"Midiverb\"},"
             "{\"index\":1,\"label\":\"Midifex\"},"
             "{\"index\":2,\"label\":\"Midiverb II\"}]");
    } else if (strcmp(key, "mix") == 0) {
        n = snprintf(buf, buf_len, "%.3f", inst->mix);
    } else if (strcmp(key, "feedback") == 0) {
        n = snprintf(buf, buf_len, "%.3f", inst->feedback);
    } else if (strcmp(key, "input_gain") == 0) {
        n = snprintf(buf, buf_len, "%.3f", inst->input_gain);
    } else if (strcmp(key, "output_gain") == 0) {
        n = snprintf(buf, buf_len, "%.3f", inst->output_gain);
    } else if (strcmp(key, "predelay_ms") == 0) {
        n = snprintf(buf, buf_len, "%.1f", inst->predelay_ms);
    } else if (strcmp(key, "low_cut_hz") == 0) {
        n = snprintf(buf, buf_len, "%.0f", inst->low_cut_hz);
    } else if (strcmp(key, "high_cut_hz") == 0) {
        n = snprintf(buf, buf_len, "%.0f", inst->high_cut_hz);
    } else if (strcmp(key, "width") == 0) {
        n = snprintf(buf, buf_len, "%.3f", inst->width);
    } else if (strcmp(key, "chain_params") == 0) {
        int max = mv_program_count(inst->unit) - 1;
        n = snprintf(buf, buf_len,
            "["
            "{\"key\":\"unit\",\"name\":\"Unit\",\"type\":\"enum\",\"options\":[\"Midiverb\",\"Midifex\",\"Midiverb II\"]},"
            "{\"key\":\"program\",\"name\":\"Program\",\"type\":\"int\",\"min\":0,\"max\":%d},"
            "{\"key\":\"mix\",\"name\":\"Mix\",\"type\":\"float\",\"min\":0,\"max\":1,\"step\":0.01},"
            "{\"key\":\"feedback\",\"name\":\"Feedback\",\"type\":\"float\",\"min\":0,\"max\":0.95,\"step\":0.01},"
            "{\"key\":\"input_gain\",\"name\":\"Input Gain\",\"type\":\"float\",\"min\":0,\"max\":2,\"step\":0.01,\"unit\":\"x\"},"
            "{\"key\":\"output_gain\",\"name\":\"Output Gain\",\"type\":\"float\",\"min\":0,\"max\":2,\"step\":0.01,\"unit\":\"x\"},"
            "{\"key\":\"predelay_ms\",\"name\":\"Pre-delay\",\"type\":\"float\",\"min\":0,\"max\":200,\"step\":1,\"unit\":\"ms\"},"
            "{\"key\":\"low_cut_hz\",\"name\":\"Low Cut\",\"type\":\"float\",\"min\":20,\"max\":1000,\"step\":1,\"unit\":\"Hz\"},"
            "{\"key\":\"high_cut_hz\",\"name\":\"High Cut\",\"type\":\"float\",\"min\":1000,\"max\":20000,\"step\":50,\"unit\":\"Hz\"},"
            "{\"key\":\"width\",\"name\":\"Width\",\"type\":\"float\",\"min\":0,\"max\":1.5,\"step\":0.01}"
            "]", max);
    } else if (strcmp(key, "ui_hierarchy") == 0) {
        n = snprintf(buf, buf_len,
            "{"
              "\"modes\":null,"
              "\"levels\":{"
                "\"root\":{"
                  "\"label\":\"Midiverb\","
                  "\"list_param\":\"program\","
                  "\"count_param\":\"program_count\","
                  "\"name_param\":\"program_name\","
                  "\"knobs\":[\"mix\",\"feedback\",\"input_gain\",\"output_gain\",\"predelay_ms\",\"low_cut_hz\",\"high_cut_hz\",\"width\"],"
                  "\"params\":["
                    "{\"key\":\"mix\",\"label\":\"Mix\"},"
                    "{\"key\":\"feedback\",\"label\":\"Feedback\"},"
                    "{\"key\":\"input_gain\",\"label\":\"Input Gain\"},"
                    "{\"key\":\"output_gain\",\"label\":\"Output Gain\"},"
                    "{\"key\":\"predelay_ms\",\"label\":\"Pre-delay\"},"
                    "{\"key\":\"low_cut_hz\",\"label\":\"Low Cut\"},"
                    "{\"key\":\"high_cut_hz\",\"label\":\"High Cut\"},"
                    "{\"key\":\"width\",\"label\":\"Width\"},"
                    "{\"level\":\"unit\",\"label\":\"Unit\"}"
                  "]"
                "},"
                "\"unit\":{"
                  "\"label\":\"Unit\","
                  "\"items_param\":\"unit_list\","
                  "\"select_param\":\"unit\","
                  "\"knobs\":[],"
                  "\"params\":[]"
                "}"
              "}"
            "}");
    }
    if (n < 0) return -1;
    if (n >= buf_len) return buf_len - 1;
    return n;
}

static void mv_on_midi(void *vp, const uint8_t *msg, int len, int source) {
    (void)vp; (void)msg; (void)len; (void)source;
}

static audio_fx_api_v2_t API = {
    .api_version = AUDIO_FX_API_VERSION_2,
    .create_instance = mv_create,
    .destroy_instance = mv_destroy,
    .process_block = mv_process,
    .set_param = mv_set_param,
    .get_param = mv_get_param,
    .on_midi = mv_on_midi,
};

audio_fx_api_v2_t* move_audio_fx_init_v2(const host_api_v1_t *host) {
    g_host = host;
    return &API;
}
