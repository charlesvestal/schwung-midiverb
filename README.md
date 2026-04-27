# schwung-midiverb

Schwung audio-FX module that emulates the Alesis **Midiverb**, **Midifex**,
and **Midiverb II** rack reverbs (1986, 13-bit DASP DSP at 23.4 kHz).

DSP based on [thement/midiverb_emulator](https://github.com/thement/midiverb_emulator)
(GPL-3.0).

## Programs

- Midiverb: 64 programs
- Midifex: 64 programs
- Midiverb II: 100 programs

## UI

Jog wheel scrolls programs, Unit submenu picks between the
three units. Knobs 1-8: Mix, Feedback, Input Gain, Output Gain, Pre-delay,
Low Cut, High Cut, Width.

## Build

```bash
./scripts/build.sh
./scripts/install.sh
```

## Credits

- DSP: [thement/midiverb_emulator](https://github.com/thement/midiverb_emulator) (decompiled DASP effect functions, cycle-accurate interpreter, LFO tables) — GPL-3.0
- Schwung integration: Charles Vestal
- Original hardware: Alesis (1986–1987)

Alesis, Midiverb, Midifex, and Midiverb II are trademarks of inMusic
Brands, Inc. This module is an unofficial software emulation, not
affiliated with or endorsed by Alesis.

## License

GPL-3.0-or-later. See:
- [LICENSE](LICENSE) — full GPL-3 text
- [NOTICE](NOTICE) — project attribution + trademark disclaimer
- [src/dsp/THIRD_PARTY_LICENSES.md](src/dsp/THIRD_PARTY_LICENSES.md) — per-file vendoring details
