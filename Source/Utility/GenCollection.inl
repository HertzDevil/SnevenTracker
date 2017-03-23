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

#include <algorithm>

template <class T, size_t N>
CGenCollection<T, N>::CGenCollection(const CGenCollection &other)
{
	for (size_t i = 0; i < N; ++i)
		if (T *o = other.m_data[i].get())
			m_data[i].reset(new T(*o));
}

template <class T, size_t N>
CGenCollection<T, N>::CGenCollection(CGenCollection &&other)
{
	swap(*this, other);
}

template <class T, size_t N>
CGenCollection<T, N> &
CGenCollection<T, N>::operator=(CGenCollection other)
{
	swap(*this, other);
	return *this;
}

template <class T, size_t N>
void swap(CGenCollection<T, N> &a, CGenCollection<T, N> &b)
{
	std::swap(a.m_data, b.m_data);
}



template <class T, size_t N>
template <class... Args>
T *CGenCollection<T, N>::Create(size_t index, Args&&... args)
{
	if (index >= N)
		return nullptr;
	T *o = new T {std::forward<Args>(args)...};
	m_data[index].reset(o);
	return o;
}

template <class T, size_t N>
void CGenCollection<T, N>::Replace(size_t index, T *obj)
{
	if (index < N)
		m_data[index].reset(obj);
}

template <class T, size_t N>
T *CGenCollection<T, N>::Detach(size_t index)
{
	return index >= N ? nullptr : m_data[index].release();
}

template <class T, size_t N>
void CGenCollection<T, N>::Swap(size_t A, size_t B)
{
	if (A < N && B < N)
		m_data[A].swap(m_data[B]);
}

template <class T, size_t N>
void CGenCollection<T, N>::Trim()
{
	size_t p = 0;
	for (size_t i : ElementIndices())
		Swap(i, p++);
}

template <class T, size_t N>
void CGenCollection<T, N>::Trim(std::function<void(size_t, size_t)> callback)
{
	size_t p = 0;
	for (size_t i : ElementIndices()) {
		callback(i, p);
		Swap(i, p++);
	}
}

template <class T, size_t N>
std::array<size_t, N> CGenCollection<T, N>::TrimMap()
{
	std::array<size_t, N> m;
	for (size_t &x : m)
		x = -1;
	Trim([&] (size_t Old, size_t New) { m[Old] = New; });
	return m;
}



template <class T, size_t N>
T *CGenCollection<T, N>::GetAt(size_t index)
{
	return index >= N ? nullptr : m_data[index].get();
}

template <class T, size_t N>
T *CGenCollection<T, N>::operator[](size_t index)
{
	return GetAt(index);
}

template <class T, size_t N>
const T *CGenCollection<T, N>::GetAt(size_t index) const
{
	return index >= N ? nullptr : m_data[index].get();
}

template <class T, size_t N>
const T *CGenCollection<T, N>::operator[](size_t index) const
{
	return GetAt(index);
}



template <class T, size_t N>
constexpr size_t CGenCollection<T, N>::GetCapacity()
{
	return N;
}

template <class T, size_t N>
bool CGenCollection<T, N>::ContainsAt(size_t index) const
{
	return index < N && m_data[index].get() != nullptr;
}

template <class T, size_t N>
size_t CGenCollection<T, N>::GetUsedCount() const
{
	return std::count_if(m_data.begin(), m_data.end(),
		[] (const element_type &x) { return x.get() != nullptr; }
	);
}

template <class T, size_t N>
size_t CGenCollection<T, N>::GetNextUsed(size_t whence) const
{
	auto Iterator = std::find_if(m_data.begin() + (++whence), m_data.end(),
		[] (const element_type &x) { return x.get() != nullptr; }
	);
	return Iterator == m_data.end() ? -1 : Iterator - m_data.begin();
}

template <class T, size_t N>
size_t CGenCollection<T, N>::GetNextFree(size_t whence) const
{
	auto Iterator = std::find_if(m_data.begin() + (++whence), m_data.end(),
		[] (const element_type &x) { return x.get() == nullptr; }
	);
	return Iterator == m_data.end() ? -1 : Iterator - m_data.begin();
}



template <class T, size_t N>
typename std::array<std::unique_ptr<T>, N>::iterator
begin(CGenCollection<T, N> &col)
{
	return col.m_data.begin();
}

template <class T, size_t N>
typename std::array<std::unique_ptr<T>, N>::iterator
end(CGenCollection<T, N> &col)
{
	return col.m_data.end();
}



template <class T, size_t N>
typename CGenCollection<T, N>::Range
CGenCollection<T, N>::ElementIndices(size_t whence)
{
	return Range {this, std::mem_fn(&CGenCollection::GetNextUsed), whence};
}

template <class T, size_t N>
typename CGenCollection<T, N>::Range
CGenCollection<T, N>::EmptyIndices(size_t whence)
{
	return Range {this, std::mem_fn(&CGenCollection::GetNextFree), whence};
}



template <class T, size_t N>
CGenCollection<T, N>::Range::
Range(const CGenCollection *col, func_type f, size_t whence) :
	_b {col, f, whence}, _e {col, f, N}
{
}

template <class T, size_t N>
typename CGenCollection<T, N>::Range::Iterator
CGenCollection<T, N>::Range::begin()
{
	return _b;
}

template <class T, size_t N>
typename CGenCollection<T, N>::Range::Iterator
CGenCollection<T, N>::Range::end()
{
	return _e;
}



template <class T, size_t N>
CGenCollection<T, N>::Range::Iterator::
Iterator (const CGenCollection *col, func_type f, size_t n) :
	pCol(col), step(f), pos(n)
{
	++*this;
}

template <class T, size_t N>
bool CGenCollection<T, N>::Range::Iterator::
operator!=(const Iterator &other)
{
	return pos != other.pos;
}

template <class T, size_t N>
size_t CGenCollection<T, N>::Range::Iterator::
operator*() const
{
	return pos;
}

template <class T, size_t N>
typename CGenCollection<T, N>::Range::Iterator &
CGenCollection<T, N>::Range::Iterator::operator++()
{
	pos = step(pCol, pos);
	return *this;
}
