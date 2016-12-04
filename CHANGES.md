# Change Log

### Version 0.2.1

- Channels no longer reduce to zero volume when the mixed volume is less than 0 (to match 0CC-FT's 5B behaviour)
- Fixed arpeggio on noise channel now maps 0 to `L-#` and 2 to `H-#` **(backward-incompatible change, modify your modules accordingly)**
- Fixed VGM logs putting 0 in the sample count fields

### Version 0.2.0

- Added VGM logger (with `vgm_cmp` postprocessing and proper GD3 tag support); use `vgmlpfnd` manually for looped songs
- Added `NCx` noise pitch rebind effect
- Channels now use subtractive mixing instead of multiplicative mixing for channel volume and instrument volume

### Version 0.1.0

- Initial release
