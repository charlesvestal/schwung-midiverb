# Third-party licenses

## midiverb_emulator (decompiled DSP and preset names)

The following files are vendored from
https://github.com/thement/midiverb_emulator (GPL-3.0):

- `decompiled-midiverb.h`
- `decompiled-midifex.h`
- `decompiled-midiverb2.h`
- `names-midiverb.h`
- `names-midifex.h`
- `names-midiverb2.h`
- `dasp16.h` — DASP cycle-accurate interpreter (used in ROM mode).
  Modification: functions made `static inline` so the header can be
  included in multiple translation units.
- `lfo.h` — LFO oscillator + program LFO patches.
  Modification: functions and `lfo_patches[]` made `static`/`static inline`
  for the same reason.
- `rom.h` — ROM type table + load_rom. We reference `rom_types[]` for
  the per-unit signature/length metadata but provide our own safe ROM
  loader (upstream's calls `die()` on any error, which would crash the
  audio host).
- `utils.h` — helpers. We don't use the exit-on-fail `read_bytes`; included
  for `ARRAY_SIZE`.

This module is therefore distributed under the GPL-3.0 (see the
top-level LICENSE).
