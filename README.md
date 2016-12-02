# SnevenTracker

SnevenTracker (SN76489 Tracker) is a fork of FamiTracker version 0.4.6 that
emulates the Texas Instruments SN76489 sound chip, which is used in a wide
variety of computers and gaming consoles. The project's ultimate aim is to
implement multiple sound chips in a more modular manner than 0.4.6, in order to
provide insight into eventually supporting them in
[0CC-FamiTracker](https://github.com/HertzDevil/0CC-FamiTracker).

As in 0CC-FT, all changes to the source code are marked with `// // //`.

### Progress

Replacing "FamiTracker" with "SnevenTracker".

### Notes

- Replaces 2A03 with SN76489, removes all expansion chips
- Tracker functionality chiefly based on MOD2PSG2
- Produces `.snm` files, zero compatibility with FamiTracker or 0CC-FT except
  for clipboard formats
- Should support multiple master clock rates and both versions of the LFSR,
  also Game Gear stereo functionality
- Should support two instances of the SN76489, plus an extra sound chip
- Should support VGM logging (may be backported from 0CC-FamiTracker), KSS
  export is planned
