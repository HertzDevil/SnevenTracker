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

class CSN76489Channel : public CExChannel
{
public:
	using CExChannel::CExChannel;
	virtual void	Reset();
	virtual void	Process(uint32 Time) = 0;
	
	virtual void	SetVGMWriter(const CVGMWriterBase *pWrite) final;

	void	SetStereo(bool Left, bool Right);

	void	SetAttenuation(uint8 Value);
	int32	GetVolume() const;

protected:
	const CVGMWriterBase *m_pVGMWriter = nullptr;
	
	uint8 m_iAttenuation;
	bool m_bLeft;
	bool m_bRight;
};



class CSNSquare final : public CSN76489Channel
{
public:
	CSNSquare(CMixer *pMixer, int ID);
	~CSNSquare();

	void	Reset() override final;
	void	Process(uint32 Time) override final;
	
	void	SetPeriodLo(uint8 Value);
	void	SetPeriodHi(uint8 Value);
	void	FlushPeriod() const;

	uint16	GetPeriod() const;

	static const uint16 CUTOFF_PERIOD;

private:
	uint32	m_iSquareCounter;
	uint8	m_iSquarePeriodLo, m_iSquarePeriodHi;
	uint16	m_iPrevPeriod;
	bool	m_bSqaureActive;
};



enum SN_noise_cfg_t
{
	SN_NOI_DISCRETE,
	SN_NOI_INTEGRATED,
};

enum SN_noise_div_t
{
	SN_NOI_DIV_512  = 0x0,
	SN_NOI_DIV_1024 = 0x1,
	SN_NOI_DIV_2048 = 0x2,
	SN_NOI_DIV_CH3  = 0x3,
};

enum SN_noise_fb_t
{
	SN_NOI_FB_SHORT = 0x0,
	SN_NOI_FB_LONG  = 0x4,
};

class CSNNoise final : public CSN76489Channel
{
public:
	CSNNoise(CMixer *pMixer);
	~CSNNoise();

	void	Reset() override final;
	void	Process(uint32 Time) override final;

	void	SetControlMode(uint8 Value);

	void	CachePeriod(uint16 Period);

private:
	uint32	m_iSquareCounter;
	uint16	m_iLFSRState;
	uint16	m_iCH3Period;

	bool	m_bSqaureActive;

	SN_noise_div_t	m_iNoiseFreq;
	SN_noise_fb_t	m_iNoiseFeedback;

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

	// TODO: CExternal should become a composite of CExChannel
	void	SetVGMWriter(const CVGMWriterBase *pWrite);

	static const size_t CHANNEL_COUNT = 4;
	static const uint16 STEREO_PORT;
	static const uint16 VOLUME_TABLE[16];

private:
	CSNSquare *GetSquare(uint8 ID) const;
	CSNNoise *GetNoise() const;

	void	UpdateNoisePeriod() const;

private:
	const CVGMWriterBase *m_pVGMWriter = nullptr;
	CSN76489Channel *m_pChannels[CHANNEL_COUNT];
	uint8	m_iAddressLatch;
};
