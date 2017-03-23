/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2016 HertzDevil
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

#include <array>
#include <memory>
#include <functional>

/*!
	\brief A fixed-size container for general objects.
*/
template <class T, size_t N>
class CGenCollection
{
public:
	using base_type = CGenCollection<T, N>;
	using element_type = std::unique_ptr<T>;

	/*!	\brief Default constructor. Creates an empty collection. */
	CGenCollection() = default;
	/*!	\brief Default destructor. */
	~CGenCollection() = default;
	CGenCollection(const CGenCollection &);
	CGenCollection(CGenCollection &&);
	CGenCollection &operator=(CGenCollection);
	template <class _T, size_t _N>
	friend void swap(CGenCollection<_T, _N> &, CGenCollection<_T, _N> &);

	template <class... Args>
	T *Create(size_t index, Args&&... args);
	void Replace(size_t index, T *obj);
	T *Detach(size_t index);
	void Swap(size_t A, size_t B);
	void Trim();
	void Trim(std::function<void(size_t, size_t)> callback);
	std::array<size_t, N> TrimMap();

	T *GetAt(size_t index);
	T *operator[](size_t index);
	const T *GetAt(size_t index) const;
	const T *operator[](size_t index) const;
	static constexpr size_t GetCapacity();

	bool ContainsAt(size_t index) const;
	size_t GetUsedCount() const;
	size_t GetNextUsed(size_t whence = -1) const;
	size_t GetNextFree(size_t whence = -1) const;

protected:
	class Range;
public:
	Range ElementIndices(size_t whence = -1);
	Range EmptyIndices(size_t whence = -1);

private:
	template <class _T, size_t _N>
	friend typename std::array<std::unique_ptr<_T>, _N>::iterator
	begin(CGenCollection<_T, _N> &col);
	template <class _T, size_t _N>
	friend typename std::array<std::unique_ptr<_T>, _N>::iterator
	end(CGenCollection<_T, _N> &col);

private:
	std::array<std::unique_ptr<T>, N> m_data;
};



template <class T, size_t N>
class CGenCollection<T, N>::Range
{
private:
	using func_type = std::function<size_t(const CGenCollection *, size_t)>;
	class Iterator
	{
	public:
		Iterator(const CGenCollection *col, func_type f, size_t n);
		bool operator!=(const Iterator &other);
		size_t operator*() const;
		Iterator &operator++();
	private:
		const CGenCollection *pCol;
		const func_type step;
		size_t pos;
	};
public:
	Range(const CGenCollection *col, func_type f, size_t whence = -1);
	Iterator begin();
	Iterator end();
private:
	Iterator _b, _e;
};



#include "GenCollection.inl"
