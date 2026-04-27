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

Osirus-style: jog wheel scrolls programs, Unit submenu picks between the
three units. Knobs 1-8: Mix, Feedback, Input Gain, Output Gain, Pre-delay,
Low Cut, High Cut, Width.

## Build

```bash
./scripts/build.sh
./scripts/install.sh
```

## License

GPL-3.0 — see LICENSE.
