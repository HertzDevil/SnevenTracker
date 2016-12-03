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

enum class VGMChip
{
	SN76489,	// $0C
	YM2413,		// $10
	YM2612,		// $2C
	YM2151,		// $30
	SegaPCM,	// $38
	RF5C68,		// $40
	YM2203,		// $44
	YM2608,		// $48
	YM2610,		// $4C
	YM3812,		// $50
	YM3526,		// $54
	Y8950,		// $58
	YMF262,		// $5C
	YMF278B,	// $60
	YMF271,		// $64
	YMZ280B,	// $68
	RF5C164,	// $6C
	PWM,		// $70
	AY8910,		// $74
	GB,			// $80
	NES,		// $84
	MultiPCM,	// $88
	uPD7759,	// $8C
	OKIM6258,	// $90
	OKIM6295,	// $98
	SCC1,		// $9C
	K054539,	// $A0
	HuC6280,	// $A4
	C140,		// $A8
	K053260,	// $AC
	Pokey,		// $B0
	QSound,		// $B4
//		SCSP,		// $B8
//		WonderSwan,	// $C0
//		VSU,		// $C4
//		SAA1099,	// $C8
//		ES5503,		// $CC
//		ES5506,		// $D0
//		X1_010,		// $D8
//		C352,		// $DC
//		GA20,		// $E0
	_Count,
};
