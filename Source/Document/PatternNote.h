/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** SnevenTracker is (C) HertzDevil 2016
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

#include "EffectCommand.h"
#include "../FamiTrackerTypes.h" // constant

namespace FTExt {

struct CPatternNote
{
	uint8_t Note = 0u;
	uint8_t Octave = 0u;
	uint8_t Inst = MAX_INSTRUMENTS;
	uint8_t Vol = MAX_VOLUME;
	CEffectCommand Effect[MAX_EFFECT_COLUMNS] = { };

	void Reset();
	operator bool() const;

private:
	static const CPatternNote BLANK;
};

} // namespace FTExt
