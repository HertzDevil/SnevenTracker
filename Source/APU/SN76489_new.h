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


#pragma once

// // // port of sn76489 emulation code

#include "../Common.h"
#include "Mixer.h"
#include "Channel.h"
#include "External.h"

class CSNSquare : public CExChannel
{
public:
	CSNSquare(CMixer *pMixer, int ID);
	~CSNSquare();

	void	Reset();
	void	Write(uint16 Address, uint8 Value);
	void	Process(uint32 Time);

	uint16	GetPeriod() const;

	static const uint16 CUTOFF_PERIOD;

private:
	uint16	m_iSquarePeriod;
	uint8	m_iAttenuation;

	bool	m_bSqaureActive;
	uint32	m_iSquareCounter;
};



enum SN_noise_cfg_t
{
	SN_NOI_DISCRETE,
	SN_NOI_INTEGRATED,
};

enum SN_noise_fb_t
{
	SN_NOI_DIV_512,
	SN_NOI_DIV_1024,
	SN_NOI_DIV_2048,
	SN_NOI_DIV_CH3,
};

class CSNNoise : public CExChannel
{
public:
	CSNNoise(CMixer *pMixer);
	~CSNNoise();

	void	Reset();
	void	Write(uint16 Address, uint8 Value);
	void	Process(uint32 Time);

	void	CachePeriod(uint16 Period);

private:
	uint8	m_iFeedbackMode;
	bool	m_bShortNoise;
	uint8	m_iAttenuation;
	
	bool	m_bSqaureActive;
	uint16	m_iLFSRState;
	uint16	m_iCH3Period;
	uint32	m_iSquareCounter;
	static const uint16 LFSR_INIT;
};



class CSN76489 : public CExternal
{
public:
	CSN76489(CMixer *pMixer);
	virtual ~CSN76489();

	void	Reset() override;
	void	Process(uint32 Time) override;
	void	EndFrame() override;

	void	Write(uint16 Address, uint8 Value) override;
	uint8	Read(uint16 Address, bool &Mapped) override;
	
	static const uint16 VOLUME_TABLE[16];

private:
	CSNSquare *m_SquareChannel[3];
	CSNNoise *m_NoiseChannel;
	uint8	m_iAddressLatch;
};
