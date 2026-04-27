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

.PHONY: all
all: smoke resampler

.PHONY: clean
clean:
	rm -f test_compile_smoke test_resampler
