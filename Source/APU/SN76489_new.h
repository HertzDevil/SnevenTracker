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
	virtual void	Reset() = 0;
	virtual void	Write(uint16 Address, uint8 Value) = 0;
	virtual void	Process(uint32 Time) = 0;

	virtual void	SetStereo(bool Left, bool Right) final;
	virtual bool	GetLeftOutput() const final;
	virtual bool	GetRightOutput() const final;

private:
	bool m_bLeft = true;
	bool m_bRight = true;
};



class CSNSquare final : public CSN76489Channel
{
public:
	CSNSquare(CMixer *pMixer, int ID);
	~CSNSquare();

	void	Reset() override final;
	void	Write(uint16 Address, uint8 Value) override final;
	void	Process(uint32 Time) override final;

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

class CSNNoise final : public CSN76489Channel
{
public:
	CSNNoise(CMixer *pMixer);
	~CSNNoise();

	void	Reset() override final;
	void	Write(uint16 Address, uint8 Value) override final;
	void	Process(uint32 Time) override final;

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

	// TODO: CExternal should become a composite of CExChannel
	void	SetVGMWriter(const CVGMWriterBase *pWrite);
	
	static const size_t CHANNEL_COUNT = 4;
	static const uint16 STEREO_PORT;
	static const uint16 VOLUME_TABLE[16];

private:
	void	UpdateNoisePeriod() const;

private:
	const CVGMWriterBase *m_pVGMWriter = nullptr;
	CSN76489Channel *m_pChannels[CHANNEL_COUNT];
	uint8	m_iAddressLatch;
};
