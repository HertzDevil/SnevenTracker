/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful, 
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
** Library General Public License for more details.  To obtain a 
** copy of the GNU Library General Public License, write to the Free 
** Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
*/

#pragma once

#define MIDI_NOTE(octave, note)		((octave) * 12 + (note) - 1)
#define GET_OCTAVE(midi_note)		((midi_note) / 12)
#define GET_NOTE(midi_note)			((midi_note) % 12 + 1)

/*
 * Here are the constants that defines the limits in the tracker
 * change if needed (some might cause side effects)
 *
 */

// Maximum number of instruments to use
const int MAX_INSTRUMENTS = 64;

// Maximum number of sequence lists
const int MAX_SEQUENCES	= 128;

// Maximum number of items in each sequence
const int MAX_SEQUENCE_ITEMS = /*128*/ 253;		// TODO: need to check if this exports correctly

// Maximum number of patterns per channel
const int MAX_PATTERN = 128;

// Maximum number of frames
const int MAX_FRAMES = 128;

// Maximum length of patterns (in rows). 256 is max in NSF
const int MAX_PATTERN_LENGTH = 256;

// // //

// Number of effect columns allowed
const int MAX_EFFECT_COLUMNS = 4;

// Maximum numbers of tracks allowed (NSF limit is 256, but dunno if the bankswitcher can handle that)
const unsigned int MAX_TRACKS = 64;

// Max tempo
const int MAX_TEMPO	= 255;

// Min tempo
//const int MIN_TEMPO	= 21;

// Max speed
//const int MAX_SPEED = 20;

// Min speed
const int MIN_SPEED = 1;

// Number of avaliable channels (max) TODO: should not be used anymore!
// instead, check the channelsavailable variable and allocate dynamically
const int MAX_CHANNELS	 = 5 + 3 + 2 + 6 + 1 + 8 + 3;		

const int CHANNELS_DEFAULT = 4;		// // //

const int OCTAVE_RANGE = 8;
const int NOTE_RANGE   = 12;

const int INVALID_INSTRUMENT = -1;

// Max allowed value in volume column. The actual meaning is no specific volume information, rather than max volume.
const int MAX_VOLUME = 0x10;

// Sequence types (shared with VRC6)

enum sequence_t {
	SEQ_VOLUME,
	SEQ_ARPEGGIO,
	SEQ_PITCH,
	SEQ_HIPITCH,		// TODO: remove this eventually
	SEQ_DUTYCYCLE,

	SEQ_COUNT
};

// Channel effects
enum effect_t {
	EF_NONE = 0,
	EF_SPEED,
	EF_JUMP,
	EF_SKIP,
	EF_HALT,
	EF_VOLUME,
	EF_PORTAMENTO,
	EF_PORTAOFF,				// unused!!
	EF_SWEEPUP,		// // // unused
	EF_SWEEPDOWN,		// // // unused
	EF_ARPEGGIO,
	EF_VIBRATO,
	EF_TREMOLO,
	EF_PITCH,
	EF_DELAY,
	EF_DAC,		// // // unused
	EF_PORTA_UP,
	EF_PORTA_DOWN,
	EF_DUTY_CYCLE,
	EF_SAMPLE_OFFSET,		// // // unused
	EF_SLIDE_UP,
	EF_SLIDE_DOWN,
	EF_VOLUME_SLIDE,
	EF_NOTE_CUT,
	// // //
//	EF_TARGET_VOLUME_SLIDE, 
/*
	EF_VRC7_MODULATOR,
	EF_VRC7_CARRIER,
	EF_VRC7_LEVELS,
*/
	EF_COUNT
};

// // //

// Channel effect letters
const char EFF_CHAR[] = {
	'F',	// Speed
	'B',	// Jump 
	'D',	// Skip 
	'C',	// Halt
	'E',	// Volume
	'3',	// Porta on
	 0,		// Porta off		// unused
	 0,		// // // unused
	 0,		// // // unused
	'0',	// Arpeggio
	'4',	// Vibrato
	'7',	// Tremolo
	'P',	// Pitch
	'G',	// Note delay
	 0,		// // // unused
	'1',	// Portamento up
	'2',	// Portamento down
	'V',	// Duty cycle
	 0,		// // // unused
	'Q',	// Slide up
	'R',	// Slide down
	'A',	// Volume slide
	'S',	// Note cut
	// // //
};


enum note_t {
	NONE = 0,	// No note
	C, Cs, D, Ds, E, F, Fs, G, Gs, A, As, B,
	RELEASE,	// Release, begin note release sequence
	HALT,		// Halt, stops note
};

enum machine_t {
	NTSC,
	PAL
};

enum vibrato_t {
	VIBRATO_OLD = 0,
	VIBRATO_NEW,
};
