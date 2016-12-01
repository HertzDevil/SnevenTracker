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

// // // SN76489 implementation

#include "DCSG.h"
#include "Mixer.h"

CDCSG::CDCSG(CMixer *pMixer) : CExternal(pMixer)
{
	SN76489_Init(pMixer, SN_DISCRETE);
	SN76489_Config(0, 4, 0, 0xFF);
}

CDCSG::~CDCSG()
{
}

void CDCSG::Reset()
{
	SN76489_Reset();
	Write(0, 0x0F);
	Write(1, 0x18);
//	Write(2, 0xF3);
//	Write(3, 0xF4);
//	Write(4, 0xF5);
//	Write(5, 0xF6);
//	Write(6, 0xF7);
//	Write(7, 0xF8);
}

void CDCSG::Process(uint32 Time)
{
	SN76489_Update(Time * 7);
}

void CDCSG::EndFrame()
{
}

void CDCSG::Write(uint16 Address, uint8 Value)
{
	SN76489_Write(0, (0x80 | (Address << 4) | (Value & 0x7F)) & 0xFF);
}

uint8 CDCSG::Read(uint16 Address, bool &Mapped)
{
	return 0;
}
