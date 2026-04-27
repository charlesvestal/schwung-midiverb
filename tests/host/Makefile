CC := cc
CFLAGS := -O2 -Wall -Wno-unused-function

test_compile_smoke: test_compile_smoke.c
	$(CC) $(CFLAGS) $< -o $@

.PHONY: smoke
smoke: test_compile_smoke
	./test_compile_smoke

test_resampler: test_resampler.c ../../src/dsp/resampler.c ../../src/dsp/resampler.h ../../src/dsp/resampler_coeffs.h
	$(CC) $(CFLAGS) -I../../src/dsp test_resampler.c -o $@ -lm

.PHONY: resampler
resampler: test_resampler
	./test_resampler

test_core: test_core.c ../../src/dsp/midiverb_core.c ../../src/dsp/midiverb_core.h \
           ../../src/dsp/resampler.c ../../src/dsp/decompiled-midiverb.h \
           ../../src/dsp/decompiled-midifex.h ../../src/dsp/decompiled-midiverb2.h
	$(CC) $(CFLAGS) -I../../src/dsp -Wno-unused-function \
	    ../../src/dsp/midiverb_core.c ../../src/dsp/resampler.c test_core.c \
	    -o $@ -lm

.PHONY: core
core: test_core
	./test_core

.PHONY: all
all: smoke resampler core

.PHONY: clean
clean:
	rm -f test_compile_smoke test_resampler test_core
