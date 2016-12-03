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

#include "SN76489.h"

CVGMWriterSN76489::CVGMWriterSN76489(CVGMLogger &logger, Mode m) :
	CVGMWriterBase(logger), m_iMode(m), m_iClockRate(3579545)
{
}

VGMChip CVGMWriterSN76489::GetChip() const
{
	return VGMChip::SN76489;
}

std::vector<char>
CVGMWriterSN76489::Command(uint32_t adr, uint32_t val, uint32_t port) const
{
	return {0x50, (char)val};
}

void CVGMWriterSN76489::UpdateHeader(CVGMLogger::Header &h) const
{
	const size_t CLOCK_ADR    = 0x0C;
	const size_t FEEDBACK_ADR = 0x28;
	const size_t WIDTH_ADR    = 0x2A;
	const size_t FLAGS_ADR    = 0x2B;

	h.WriteAt(CLOCK_ADR, m_iClockRate);

	switch (m_iMode) {
	case Mode::GameGear:
		h.WriteAt<uint16_t>(FEEDBACK_ADR, 0x0009);
		h.WriteAt<uint8_t>(WIDTH_ADR, 0x10);
		break;
	case Mode::BBCMicro:
		h.WriteAt<uint16_t>(FEEDBACK_ADR, 0x0003);
		h.WriteAt<uint8_t>(WIDTH_ADR, 0x10);
		break;
	case Mode::SN76496:
		h.WriteAt<uint16_t>(FEEDBACK_ADR, 0x0006);
		h.WriteAt<uint8_t>(WIDTH_ADR, 0x0F);
		break;
	}

	h.WriteAt<uint8_t>(FLAGS_ADR, 0x00);
}

void CVGMWriterSN76489::SetClockRate(uint32_t hz)
{
	m_iClockRate = hz;
}
