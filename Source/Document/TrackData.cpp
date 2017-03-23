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

#include "TrackData.h"
#include "PatternData_new.h"

using namespace FTExt;



const CPatternData *CTrackData::GetPattern(size_t Index) const
{
	return m_pPatterns[Index];
}

CPatternData *CTrackData::GetPattern(size_t Index)
{
	if (m_pPatterns.ContainsAt(Index))
		return m_pPatterns[Index];
	return m_pPatterns.Create(Index);
}

CPatternData *CTrackData::NewPattern(size_t Index, size_t Size)
{
	return m_pPatterns.Create(Index, Size);
}



const CPatternData *CTrackData::GetPatternAtFrame(size_t Frame) const
{
	return GetPattern(GetPatternIndex(Frame));
}

CPatternData *CTrackData::GetPatternAtFrame(size_t Frame)
{
	return GetPattern(GetPatternIndex(Frame));
}


/*
size_t CTrackData::GetTrackLength() const
{
	return m_iLength;
}

void CTrackData::SetTrackLength(size_t Size)
{
	m_iLength = Size;
}
*/


size_t CTrackData::GetPatternIndex(size_t Frame) const
{
	return m_iFrameList[Frame];
}

void CTrackData::SetPatternIndex(size_t Frame, size_t Index)
{
	m_iFrameList[Frame] = Index;
}



int CTrackData::GetEffectColumnCount() const
{
	return m_iEffectColumnCount;
}

void CTrackData::SetEffectColumnCount(int Count)
{
	if (Count < 1 || Count > MAX_EFFECT_COLUMNS)
		throw std::runtime_error {"Effect column count out of bounds"};
	m_iEffectColumnCount = Count;
}
