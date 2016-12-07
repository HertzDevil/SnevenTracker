### plans

fix `CChannelMap`
- contained by `CFamiTrackerDoc`, as originally planned
- `unordered_map` lookup by chip name?
- multiple instrument types for each sound chip

each channel handler will control its CExternal-derived class immediately
- no shared register writes among chips, no need to emulate mapper circuitry tbh
- if that is required, add also a `CSoundSystem` class containing at most one instance of each chip

`CSoundChip`
- present in 0cc-ft, but here it should use composite pattern (over `CExChannel`)
- interface for dealing with chip-specific configuration (refresh rate, region etc.)

`CChipHandler`
- **dual sn76489** (this is actually prohibited since the vgm format will enable t6w28 mode for that)
- can this use composite just like `CSoundChip`?

will i be able to implement the new pattern effect interface here???
- turn `effect_t` into a true effect type
- proper dispatch for effect types
- effect handler, each CChannelHandler stores a list of them
- channel state object, filtered through entire effect chain, final state processed by `RefreshChannel`

