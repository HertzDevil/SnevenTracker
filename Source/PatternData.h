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


// Channel note struct, holds the data for each row in patterns
struct stChanNote {
	unsigned char Note;
	unsigned char Octave;
	unsigned char Vol;
	unsigned char Instrument;
	unsigned char EffNumber[MAX_EFFECT_COLUMNS];
	unsigned char EffParam[MAX_EFFECT_COLUMNS];

	static const stChanNote BLANK;
};

const stChanNote stChanNote::BLANK = {0, 0, MAX_VOLUME, MAX_INSTRUMENTS, {EF_NONE, EF_NONE, EF_NONE, EF_NONE}, {0, 0, 0, 0}};

// TODO rename to CTrack perhaps?

// CPatternData holds all notes in the patterns
class CPatternData {
public:
	CPatternData(unsigned int PatternLength, unsigned int Speed, unsigned int Tempo);
	~CPatternData();

	bool IsCellFree(unsigned int Channel, unsigned int Pattern, unsigned int Row) const;
	bool IsPatternEmpty(unsigned int Channel, unsigned int Pattern) const;
	bool IsPatternInUse(unsigned int Channel, unsigned int Pattern) const;

	void ClearEverything();
	void ClearPattern(unsigned int Channel, unsigned int Pattern);

	stChanNote *GetPatternData(unsigned int Channel, unsigned int Pattern, unsigned int Row);

	unsigned int GetPatternLength() const;		// // //
	unsigned int GetFrameCount() const;
	unsigned int GetSongSpeed() const;
	unsigned int GetSongTempo() const;
	int GetEffectColumnCount(int Channel) const;
	unsigned int GetFramePattern(unsigned int Frame, unsigned int Channel) const;
	unsigned int GetFirstRowHighlight() const;
	unsigned int GetSecondRowHighlight() const;

	void SetPatternLength(unsigned int Length);
	void SetFrameCount(unsigned int Count);
	void SetSongSpeed(unsigned int Speed);
	void SetSongTempo(unsigned int Tempo);
	void SetEffectColumnCount(int Channel, int Count);
	void SetFramePattern(unsigned int Frame, unsigned int Channel, unsigned int Pattern);
	void SetHighlight(unsigned int First, unsigned int Second);

private:
	stChanNote *GetPatternData(unsigned int Channel, unsigned int Pattern, unsigned int Row) const;
	void AllocatePattern(unsigned int Channel, unsigned int Patterns);

	// Pattern data
private:

	// Track parameters
	unsigned int m_iPatternLength;			// Amount of rows in one pattern
	unsigned int m_iFrameCount;				// Number of frames
	unsigned int m_iSongSpeed;				// Song speed
	unsigned int m_iSongTempo;				// Song tempo

	// Row highlight settings
	unsigned int m_iRowHighlight1;
	unsigned int m_iRowHighlight2;

	// Number of visible effect columns for each channel
	unsigned char m_iEffectColumns[MAX_CHANNELS];

	// List of the patterns assigned to frames
	unsigned char m_iFrameList[MAX_FRAMES][MAX_CHANNELS];		

	// All accesses to m_pPatternData must go through GetPatternData()
	stChanNote *m_pPatternData[MAX_CHANNELS][MAX_PATTERN];
};
