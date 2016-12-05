# Change Log

### Version 0.2.2

- Overloaded `NE0` / `NE1` for noise reset enable effect
- Added SN76489 stereo separation to mixer menu
- Re-added file association
- SN76489 VGM logger now eliminates extra register writes that have no side effects
- Extended VGM header size so that it will not be misinterpreted by certain players
- Fixed text export and import (for SN7T only)
- Renamed `NCx` to "Channel swap"
- Samples are now properly downmixed to mono for visualizers

### Version 0.2.1

- Overloaded `N00` - `N1F` for Game Gear stereo control
- Channels no longer reduce to zero volume when the mixed volume is less than 0 (to match 0CC-FT's 5B behaviour)
- Fixed arpeggio on noise channel now maps 0 to `L-#` and 2 to `H-#` **(backward-incompatible change, modify your modules accordingly)**
- Fixed VGM logs putting 0 in the sample count fields

### Version 0.2.0

- Added VGM logger (with `vgm_cmp` postprocessing and proper GD3 tag support); use `vgmlpfnd` manually for looped songs
- Added `NCx` noise pitch rebind effect
- Channels now use subtractive mixing instead of multiplicative mixing for channel volume and instrument volume

### Version 0.1.0

- Initial release
