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

#include "PatternData_new.h"
#include "PatternNote.h"
#include <stdexcept>

using namespace FTExt;



const size_t CPatternData::MAX_SIZE = MAX_PATTERN_LENGTH;



CPatternData::CPatternData(size_t MaxSize) :
	m_pNotes(MaxSize),
	m_iActualSize {MaxSize}
{
	if (m_iActualSize > MAX_SIZE)
		throw std::runtime_error {"Pattern is too large"};
}

CPatternData &CPatternData::operator=(CPatternData other)
{
	swap(*this, other);
	return *this;
}

void FTExt::swap(CPatternData &a, CPatternData &b)
{
	std::swap(a.m_iActualSize, b.m_iActualSize);
	a.m_pNotes.swap(b.m_pNotes);
}



CPatternNote CPatternData::GetNote(size_t Row) const
{
	return m_pNotes[Row];
}

void CPatternData::SetNote(size_t Row, const CPatternNote &Note)
{
	m_pNotes[Row] = Note;
}



size_t CPatternData::GetSize() const
{
	return m_iActualSize;
}

void CPatternData::SetSize(size_t Size)
{
	if (Size > MAX_SIZE)
		throw std::runtime_error {"Pattern size beyond limit"};
	m_iActualSize = Size;
}



bool CPatternData::IsEmpty() const
{
	for (size_t i = 0u; i < m_iActualSize; ++i)
		if (m_pNotes[i])
			return false;
	return true;
}

void CPatternData::Clear()
{
	for (auto &x : m_pNotes)
		x.Reset();
}



typename std::vector<FTExt::CPatternNote>::iterator
CPatternData::begin()
{
	return m_pNotes.begin();
}

typename std::vector<FTExt::CPatternNote>::iterator
CPatternData::end()
{
	return begin() + m_iActualSize;
}
