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

#include "../Logger.h"

// // // base class for VGM writer, provides slightly higher abstraction

class CVGMWriterBase
{
protected:
	CVGMWriterBase(CVGMLogger &logger);

public:
	virtual ~CVGMWriterBase();

public:
	virtual VGMChip GetChip() const = 0;
	virtual void UpdateHeader(CVGMLogger::Header &h) const = 0;
	virtual void WriteReg(uint32_t adr, uint32_t val, uint32_t port = 0) const final;

private:
	virtual std::vector<char> Command(uint32_t adr, uint32_t val, uint32_t port) const = 0;

protected:
	CVGMLogger &m_Logger;
};
