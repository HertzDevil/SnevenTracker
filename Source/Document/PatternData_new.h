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

#include <vector>
#include "PatternNote.h"

namespace FTExt {

class CPatternData
{
public:
	explicit CPatternData(size_t MaxSize = MAX_SIZE);
	~CPatternData() = default;

	CPatternData(const CPatternData &other) = default;
	CPatternData(CPatternData &&other) = default;
	CPatternData &operator=(CPatternData other);
	friend void swap(CPatternData &a, CPatternData &b);

	CPatternNote GetNote(size_t Row) const;
	void SetNote(size_t Row, const CPatternNote &Note);

	size_t GetSize() const;
	void SetSize(size_t Size);

	bool IsEmpty() const;
	void Clear();

	typename std::vector<CPatternNote>::iterator begin();
	typename std::vector<CPatternNote>::iterator end();

public:
	static const size_t MAX_SIZE;

private:
	std::vector<CPatternNote> m_pNotes;
	size_t m_iActualSize = 0u;
};

} // namespace FTExt
