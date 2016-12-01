# DCSG-Tracker

DCSG-Tracker (Digital complex sound generator tracker) is a fork of FamiTracker
version 0.4.6 that emulates the Texas Instruments SN76489 sound chip, which is
used in a wide variety of computers and gaming consoles. The project's ultimate
aim is to produce a diff between the vanilla 0.4.6 source and its source code,
in order to provide insight into eventually supporting other sound chips in
[0CC-FamiTracker](https://github.com/HertzDevil/0CC-FamiTracker).

As in 0CC-FT, all changes to the source code are marked with `// // //`.

### Progress

Replacing "FamiTracker" with "DCSG-Tracker".

### Notes

- Replaces 2A03 with SN76489, removes all expansion chips
- Tracker functionality chiefly based on MOD2PSG2
- Produces `.snm` files, same format as `.ftm` files but incompatible
- At least one export target must be supported (preferably `.kss` since 0CC-FT
  already does `.vgm` logging)
