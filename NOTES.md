# Notes

~~i have no time to make a proper manual for this~~

## Sound Chips

#### Texas Instruments SN76489 (PSG)

_(TODO)_

#### T6W28 (NGP)

_(TODO)_

#### Yamaha YM2413 (OPLL)

_(TODO)_

#### Yamaha YM2612 (OPN2)

_(TODO)_

## New Effects

#### N00 - N1F

- **Name:** Panning setting
- **Target:** SN76489, ~~T6W28~~
- **Default:** `N10`
- **Description:** Changes the volume balance for the left and right output channels. `N01` is left, `N10` is center, `N1F` is right. The special `N00` silences both output channels. ~~Intermediate values are supported only by the T6W28 chip; it effectively decreases the output volume in one of the output channels, but never silences it unless `N01` or `N1F` is used.~~

#### NCx

- **Name:** Channel swap
- **Target:** SN76489
- **Default:** `NC3`
- **Description:** Exchanges Square `x` with Square 3. This allows any square channel to control the pitch of the noise channel. Doing so will cause a pop in the audio output as the channel registers must be rewritten.

#### NE0, NE1

- **Name:** Noise reset enable
- **Target:** SN76489 Noise
- **Default:** `NE0`
- **Description:** Configures whether the noise channel resets its shift register state on new notes. `NE0` disables it, `NE1` enables it and additionally resets the noise state immediately. (Changing the pitch or duty always resets the noise state. This is normal SN76489 behaviour.)

<!--
- **Name:** a
- **Target:** a
- **Default:** ``
- **Description:** a
-->

## New features

#### VGM Logging

Under the **Tracker** menu is a new option called "Log VGM File...". Select it to save a VGM file, then play any song to start logging all audio events, and stop the player to finish logging.

## Compatibility

SnevenTracker modules (`.snm` files) share the same format with `.ftm` and `.0cc` files but are incompatible with them. Copying and pasting of frames and patterns still works across these trackers.
