/* Host-side WAV runner: feeds a stereo 44.1k int16 WAV through the full
 * midiverb plugin (audio_fx_api_v2 path), writes a WAV out. Lets us A/B
 * against upstream's CLI without involving Move hardware.
 *
 * Build: see Makefile target "wav_runner".
 * Usage: ./wav_runner <unit:0|1|2> <program:0..> <input.wav> <output.wav>
 *        unit 0=Midiverb 1=Midifex 2=Midiverb II
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sndfile.h>

#include "../../src/dsp/audio_fx_api_v2.h"

extern audio_fx_api_v2_t* move_audio_fx_init_v2(const host_api_v1_t *host);

#define BLOCK 128

int main(int argc, char **argv) {
    if (argc != 5) {
        fprintf(stderr, "usage: %s <unit:0|1|2> <program> <input.wav> <output.wav>\n", argv[0]);
        return 1;
    }
    int unit = atoi(argv[1]);
    int program = atoi(argv[2]);
    const char *in_path = argv[3];
    const char *out_path = argv[4];

    SF_INFO si = {0};
    SNDFILE *in_sf = sf_open(in_path, SFM_READ, &si);
    if (!in_sf) { fprintf(stderr, "open %s: %s\n", in_path, sf_strerror(NULL)); return 1; }
    if (si.channels != 2) { fprintf(stderr, "input must be stereo\n"); return 1; }
    if (si.samplerate != 44100) { fprintf(stderr, "warning: input sr=%d, plugin assumes 44100\n", si.samplerate); }

    SF_INFO so = si;
    SNDFILE *out_sf = sf_open(out_path, SFM_WRITE, &so);
    if (!out_sf) { fprintf(stderr, "create %s: %s\n", out_path, sf_strerror(NULL)); return 1; }

    audio_fx_api_v2_t *api = move_audio_fx_init_v2(NULL);
    if (!api || api->api_version != AUDIO_FX_API_VERSION_2) {
        fprintf(stderr, "plugin init failed\n"); return 1;
    }
    void *inst = api->create_instance(".", NULL);
    if (!inst) { fprintf(stderr, "create_instance failed\n"); return 1; }

    char buf[64];
    snprintf(buf, sizeof(buf), "%d", unit);
    api->set_param(inst, "unit", buf);
    snprintf(buf, sizeof(buf), "%d", program);
    api->set_param(inst, "program", buf);
    api->set_param(inst, "mix", "0.45");
    api->set_param(inst, "feedback", "0");
    api->set_param(inst, "input_gain", "1");
    api->set_param(inst, "output_gain", "1");
    api->set_param(inst, "low_cut_hz", "20");
    api->set_param(inst, "high_cut_hz", "20000");
    api->set_param(inst, "width", "1");

    int16_t block[BLOCK * 2];
    sf_count_t n;
    long total = 0;
    while ((n = sf_readf_short(in_sf, block, BLOCK)) > 0) {
        if (n < BLOCK) memset(block + n*2, 0, (BLOCK - n) * 2 * sizeof(int16_t));
        api->process_block(inst, block, BLOCK);
        sf_writef_short(out_sf, block, n);
        total += n;
    }
    fprintf(stderr, "processed %ld frames through unit=%d program=%d (%s)\n",
            total, unit, program, in_path);

    api->destroy_instance(inst);
    sf_close(in_sf);
    sf_close(out_sf);
    return 0;
}
