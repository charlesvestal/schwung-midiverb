#include "resampler.h"

#include <string.h>

#define SR_IN  44100.0f
#define SR_OUT 23400.0f
#define DOWN_RATIO (SR_OUT / SR_IN)
#define UP_RATIO   (SR_IN / SR_OUT)

void downsampler_init(downsampler_t *r) {
    memset(r, 0, sizeof(*r));
    /* Pre-fill RESAMPLER_TAPS zeros so the FIR window has a valid lookahead
     * from the first call. Without this, the upsampler downstream gets fewer
     * mid samples on the first call, leaving its buffer too short to produce
     * a full output block. */
    r->buf_pos = RESAMPLER_TAPS;
}

void upsampler_init(upsampler_t *r) {
    memset(r, 0, sizeof(*r));
    /* Pre-fill RESAMPLER_TAPS zeros so the FIR window has a valid lookahead
     * for the convolve from the very first output. Without this, the
     * upsampler underflows by ~32 samples per block and emits silence for
     * the second half of each block. */
    r->buf_pos = RESAMPLER_TAPS;
}

static inline float convolve(const float *taps, const float *coeffs) {
    float acc = 0.0f;
    for (int i = 0; i < RESAMPLER_TAPS; i++) {
        acc += taps[i] * coeffs[i];
    }
    return acc;
}

int downsampler_process(downsampler_t *r, const float *in, int n_in,
                        float *out, int out_capacity) {
    if (r->buf_pos + n_in > (int)(sizeof(r->buf) / sizeof(float))) {
        int tail = r->buf_pos - (RESAMPLER_TAPS - 1);
        if (tail < 0) tail = 0;
        int kept = r->buf_pos - tail;
        memmove(r->buf, r->buf + tail, kept * sizeof(float));
        r->buf_pos = kept;
    }
    memcpy(r->buf + r->buf_pos, in, n_in * sizeof(float));
    r->buf_pos += n_in;

    int n_out = 0;
    while (n_out < out_capacity) {
        int integer = (int)r->phase;
        if (integer + RESAMPLER_TAPS > r->buf_pos) break;
        float frac = r->phase - integer;
        int phase_idx = (int)(frac * RESAMPLER_PHASES);
        if (phase_idx >= RESAMPLER_PHASES) phase_idx = RESAMPLER_PHASES - 1;
        out[n_out++] = convolve(&r->buf[integer], down_coeffs[phase_idx]);
        r->phase += 1.0f / DOWN_RATIO;
    }

    int consumed = (int)r->phase;
    if (consumed > 0) {
        int kept = r->buf_pos - consumed;
        if (kept < 0) kept = 0;
        memmove(r->buf, r->buf + consumed, kept * sizeof(float));
        r->buf_pos = kept;
        r->phase -= consumed;
    }
    return n_out;
}

void upsampler_process(upsampler_t *r, const float *in, int n_in,
                       float *out, int n_out) {
    if (r->buf_pos + n_in > (int)(sizeof(r->buf) / sizeof(float))) {
        int tail = r->buf_pos - (RESAMPLER_TAPS - 1);
        if (tail < 0) tail = 0;
        int kept = r->buf_pos - tail;
        memmove(r->buf, r->buf + tail, kept * sizeof(float));
        r->buf_pos = kept;
    }
    memcpy(r->buf + r->buf_pos, in, n_in * sizeof(float));
    r->buf_pos += n_in;

    for (int i = 0; i < n_out; i++) {
        int integer = (int)r->phase;
        if (integer + RESAMPLER_TAPS > r->buf_pos) {
            out[i] = 0.0f;
            r->phase += 1.0f / UP_RATIO;
            continue;
        }
        float frac = r->phase - integer;
        int phase_idx = (int)(frac * RESAMPLER_PHASES);
        if (phase_idx >= RESAMPLER_PHASES) phase_idx = RESAMPLER_PHASES - 1;
        out[i] = convolve(&r->buf[integer], up_coeffs[phase_idx]);
        r->phase += 1.0f / UP_RATIO;
    }

    int consumed = (int)r->phase;
    if (consumed > 0) {
        int kept = r->buf_pos - consumed;
        if (kept < 0) kept = 0;
        memmove(r->buf, r->buf + consumed, kept * sizeof(float));
        r->buf_pos = kept;
        r->phase -= consumed;
    }
}
