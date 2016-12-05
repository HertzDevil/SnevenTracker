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

#include <algorithm>		// // //
#include <vector>
#include "../stdafx.h"
#include <cstdio>
#include <memory>
#include <cmath>
#include "APU.h"
#include "SN76489_new.h"		// // //
#include "../VGM/Writer/Base.h"		// // //

const uint32 CAPU::BASE_FREQ_NTSC		= 3579540;		// // //
const uint32 CAPU::BASE_FREQ_PAL		= 3546893;
const uint8	 CAPU::FRAME_RATE_NTSC		= 60;
const uint8	 CAPU::FRAME_RATE_PAL		= 50;

// // //

CAPU::CAPU(IAudioCallback *pCallback) :		// // //
	m_pParent(pCallback),
	m_iFrameCycles(0),
	m_pSoundBuffer(NULL),
	m_pMixer(new CMixer()),
	m_iExternalSoundChip(0),
	m_iCyclesToRun(0)
{
	m_pSN76489 = new CSN76489(m_pMixer);		// // //

#ifdef LOGGING
	m_pLog = new CFile("apu_log.txt", CFile::modeCreate | CFile::modeWrite);
	m_iFrame = 0;
#endif
}

CAPU::~CAPU()
{
	SAFE_RELEASE(m_pSN76489);		// // //

	SAFE_RELEASE(m_pMixer);

	SAFE_RELEASE(m_pSoundBuffer);

#ifdef LOGGING
	m_pLog->Close();
	delete m_pLog;
#endif
}

// // //

inline void CAPU::RunSN(uint32 Time)		// // //
{
	m_pSN76489->Process(Time);
}

// The main APU emulation
//
// The amount of cycles that will be emulated is added by CAPU::AddCycles
//
void CAPU::Process()
{	
	while (m_iCyclesToRun > 0) {
		uint32 Time = std::min(m_iCyclesToRun, m_iFrameClock);		// // //
		
		// Run internal channels
		RunSN(Time);		// // //

		m_iFrameCycles	  += Time;
		m_iFrameClock	  -= Time;
		m_iCyclesToRun	  -= Time;

		// // //

		if (m_iFrameClock == 0)
			EndFrame();
	}
}

// End of audio frame, flush the buffer if enough samples has been produced, and start a new frame
void CAPU::EndFrame()
{
	// The APU will always output audio in 32 bit signed format
	
	m_pSN76489->EndFrame();		// // //

	int SamplesAvail = m_pMixer->FinishBuffer(m_iFrameCycles);
	int ReadSamples	= m_pMixer->ReadBuffer(SamplesAvail, m_pSoundBuffer, m_bStereoEnabled);
	m_pParent->FlushBuffer(m_pSoundBuffer, ReadSamples);
	
	m_iFrameClock /*+*/= m_iFrameCycleCount;
	m_iFrameCycles = 0;

#ifdef LOGGING
	++m_iFrame;
#endif
}

void CAPU::Reset()
{
	// Reset APU
	//
	
	m_iCyclesToRun		= 0;
	m_iFrameCycles		= 0;
	m_iFrameSequence	= 0;
	m_iFrameClock		= m_iFrameCycleCount;
	
	m_pMixer->ClearBuffer();

	m_pSN76489->Reset();		// // //

#ifdef LOGGING
	m_iFrame = 0;
#endif
}

void CAPU::SetupMixer(int LowCut, int HighCut, int HighDamp, int Volume) const
{
	// New settings
	m_pMixer->UpdateSettings(LowCut, HighCut, HighDamp, float(Volume) / 100.0f);
	// // //
}

void CAPU::SetExternalSound(uint8 Chip)
{
	// Set expansion chip
	m_iExternalSoundChip = Chip;
	m_pMixer->ExternalSound(Chip);

	// // //

	Reset();
}

void CAPU::ChangeMachine(int Machine)
{
	// Allow to change speed on the fly
	//
	// // //
	/*
	switch (Machine) {
		case MACHINE_NTSC:
			m_pNoise->PERIOD_TABLE = CNoise::NOISE_PERIODS_NTSC;
			// // //
			m_pMixer->SetClockRate(BASE_FREQ_NTSC);
			break;
		case MACHINE_PAL:
			m_pNoise->PERIOD_TABLE = CNoise::NOISE_PERIODS_PAL;
			// // //
			m_pMixer->SetClockRate(BASE_FREQ_PAL);
			break;
	}
	*/
}

bool CAPU::SetupSound(int SampleRate, int NrChannels, int Machine)
{
	// Allocate a sound buffer
	//
	// Returns false if a buffer couldn't be allocated
	//
	
	uint32 BaseFreq = (Machine == MACHINE_NTSC) ? BASE_FREQ_NTSC : BASE_FREQ_PAL;
	uint8 FrameRate = (Machine == MACHINE_NTSC) ? FRAME_RATE_NTSC : FRAME_RATE_PAL;

	m_iSoundBufferSamples = uint32(SampleRate / FRAME_RATE_PAL);	// Samples / frame. Allocate for PAL, since it's more
	m_bStereoEnabled	  = (NrChannels == 2);	
	m_iSoundBufferSize	  = m_iSoundBufferSamples * NrChannels;		// Total amount of samples to allocate
	m_iSampleSizeShift	  = (NrChannels == 2) ? 1 : 0;
	m_iBufferPointer	  = 0;

	if (!m_pMixer->AllocateBuffer(m_iSoundBufferSamples, SampleRate, NrChannels))
		return false;

	m_pMixer->SetClockRate(BaseFreq);

	SAFE_RELEASE_ARRAY(m_pSoundBuffer);

	m_pSoundBuffer = new int16[m_iSoundBufferSize << 1];

	if (m_pSoundBuffer == NULL)
		return false;

	ChangeMachine(Machine);

	// // //

	// Numbers of cycles/audio frame
	m_iFrameCycleCount = BaseFreq / FrameRate;

	return true;
}

void CAPU::AddTime(int32 Cycles)
{
	if (Cycles < 0)
		return;
	m_iCyclesToRun += Cycles;
}

void CAPU::Write(uint16 Address, uint8 Value)
{
	// Data was written to an APU register
	//

	Process();

	if (Address == CSN76489::STEREO_PORT || Address <= 0x10 || Address == (uint16)-1) {		// // //
		m_pSN76489->Write(Address, Value);
	}

	// // //m_iRegs[Address & 0x1F] = Value;

#ifdef LOGGING
	m_iRegs[Address & 0x1F] = Value;
#endif
}

// // //

void CAPU::ExternalWrite(uint16 Address, uint8 Value)
{
	// Data was written to an external sound chip 
	// (this doesn't really belong in the APU but are here for convenience)
	//

	Process();

	// // //
}

uint8 CAPU::ExternalRead(uint16 Address)
{
	// Data read from an external chip
	//

	uint8 Value(0);
	bool Mapped(false);

	Process();

	// // //

	if (!Mapped)
		Value = Address >> 8;	// open bus

	return Value;
}

// Expansion for famitracker

int32 CAPU::GetVol(uint8 Chan) const	
{
	return m_pMixer->GetChanOutput(Chan);
}

// // //

#ifdef LOGGING
void CAPU::Log()
{
	CString str;
	str.Format("Frame %08i: ", m_iFrame);
	for (int i = 0; i < 0x14; ++i)
		str.AppendFormat("%02X ", m_iRegs[i]);
	str.Append("\n");
	m_pLog->Write(str, str.GetLength());
}
#endif

void CAPU::SetChipLevel(chip_level_t Chip, float Level) const
{
	float fLevel = powf(10, Level / 20.0f);		// Convert dB to linear

	// // //
	m_pMixer->SetChipLevel(Chip, fLevel);
}

void CAPU::SetStereoSeparation(float Sep) const		// // //
{
	m_pMixer->SetChipLevel(CHIP_LEVEL_SN7Sep, Sep);
}

void CAPU::SetVGMWriter(VGMChip Chip, const CVGMWriterBase *pWrite)		// // //
{
	switch (Chip) {
	case VGMChip::SN76489:
		m_pSN76489->SetVGMWriter(pWrite);
		break;
	}
}

uint8 CAPU::GetReg(int Chip, int Reg) const 
{
	switch (Chip) {
		case SNDCHIP_NONE:
			return m_iRegs[Reg & 0x1F]; 
		// // //
	}

	return 0;
}
