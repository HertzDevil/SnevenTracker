### plans

fix `CChannelMap`
- contained by `CFamiTrackerDoc`, as originally planned
- `unordered_map` lookup by chip name?
- multiple instrument types for each sound chip

`CSongView`
- decouples cursor/selection stuff from the pattern editor
- also put the iterator classes from 0cc-ft near here
- should be possible to reorder channels with this
- the actual pattern editor view must be able to hide channels without disabling them

each channel handler will control its `CExternal`-derived class immediately
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

opn2 / ym2612
- 24 tracks, since each operator has one; 6 masters (op4), 18 slaves (op1-3)
- `N00` - `N07` changes which operators are automatically controlled by the master tracks, default `N07` so that other operator tracks can be ignored unless needed
- slave tracks have sound effect track semantics (refine this), trigger/halt controls operator
- ch3 extended mode is just a special case where 3 slave tracks get their frequencies written
- `Kxx` and `Vxx` will dispatch to the different operator parameters on the current track
- no plans for one instrument sequence per operator parameter
- no plans for software-mixed pcm, but 0cc-ft's successor should have them by leveraging ch6's 4 tracks directly

