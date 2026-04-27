CC := cc
CFLAGS := -O2 -Wall -Wno-unused-function

test_compile_smoke: test_compile_smoke.c
	$(CC) $(CFLAGS) $< -o $@

.PHONY: smoke
smoke: test_compile_smoke
	./test_compile_smoke

.PHONY: clean
clean:
	rm -f test_compile_smoke
