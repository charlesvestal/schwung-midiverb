#ifndef MIDIVERB_RESAMPLER_H
#define MIDIVERB_RESAMPLER_H

#include <stdint.h>

#include "resampler_coeffs.h"

#define RESAMPLER_MAX_BLOCK_44 256
#define RESAMPLER_MAX_BLOCK_23 160

typedef struct {
    float buf[RESAMPLER_TAPS + RESAMPLER_MAX_BLOCK_44];
    int   buf_pos;
    float phase;
} downsampler_t;

typedef struct {
    float buf[RESAMPLER_TAPS + RESAMPLER_MAX_BLOCK_23];
    int   buf_pos;
    float phase;
} upsampler_t;

void downsampler_init(downsampler_t *r);
void upsampler_init(upsampler_t *r);

int  downsampler_process(downsampler_t *r, const float *in, int n_in,
                         float *out, int out_capacity);
void upsampler_process(upsampler_t *r, const float *in, int n_in,
                       float *out, int n_out);

#endif
