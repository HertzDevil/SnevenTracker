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

#include "SN76489_new.h"

const uint16 CSN76489::VOLUME_TABLE[] = {
	1516, 1205, 957, 760, 603, 479, 381, 303, 240, 191, 152, 120, 96, 76, 60, 0
};
const uint16 CSNSquare::CUTOFF_PERIOD = /* 0x01 */ 0x06;



CSNSquare::CSNSquare(CMixer *pMixer, int ID) :
	CExChannel(pMixer, 0, ID)
{
}

CSNSquare::~CSNSquare()
{
}

void CSNSquare::Reset()
{
	m_iSquarePeriod = 0;
	m_iAttenuation = 0xF;

	m_iSquareCounter = 0;
	m_bSqaureActive = true;
}

void CSNSquare::Write(uint16 Address, uint8 Value)
{
	// they don't correspond to real addressable registers
	// but neither do the built-in 2A03 channel classes
	switch (Address) {
	case 0:
		m_iSquarePeriod = (m_iSquarePeriod & 0x3F0) | (Value & 0x00F); break;
	case 1:
		m_iSquarePeriod = (m_iSquarePeriod & 0x00F) | ((Value & 0x3F) << 4); break;
	case 2:
		m_iAttenuation = Value & 0x0F; break;
	}
}

void CSNSquare::Process(uint32 Time)
{
	uint16 Period = m_iSquarePeriod ? m_iSquarePeriod : 1;

	while (Time >= m_iSquareCounter) {
		Time -= m_iSquareCounter;
		m_iTime += m_iSquareCounter;
		m_iSquareCounter = Period << 4;
		m_bSqaureActive = Period > CUTOFF_PERIOD ? !m_bSqaureActive : 1;
		Mix(m_bSqaureActive ? CSN76489::VOLUME_TABLE[m_iAttenuation] : 0);
	}

	m_iSquareCounter -= Time;
	m_iTime += Time;
}

uint16 CSNSquare::GetPeriod() const
{
	return m_iSquarePeriod;
}



const uint16 CSNNoise::LFSR_INIT = 0x8000;

CSNNoise::CSNNoise(CMixer *pMixer) :
	CExChannel(pMixer, 0, CHANID_NOISE), m_iCH3Period(0)
{
}

CSNNoise::~CSNNoise()
{
}

void CSNNoise::Reset()
{
	m_iFeedbackMode = SN_NOI_DIV_512;
	m_bShortNoise = false;
	m_iAttenuation = 0xF;

	m_iLFSRState = LFSR_INIT;
	m_iSquareCounter = 0;
	m_bSqaureActive = true;
}

void CSNNoise::Write(uint16 Address, uint8 Value)
{
	switch (Address) {
	case 0:
		m_iFeedbackMode = Value & 0x03;
		m_bShortNoise = (Value & 0x04) != 0;
		m_iLFSRState = LFSR_INIT; break;
	case 2:
		m_iAttenuation = Value & 0x0F; break;
	}
}

void CSNNoise::Process(uint32 Time)
{
	uint16 Period;
	switch (m_iFeedbackMode) {
	case SN_NOI_DIV_512:  Period = 0x10; break;
	case SN_NOI_DIV_1024: Period = 0x20; break;
	case SN_NOI_DIV_2048: Period = 0x40; break;
	case SN_NOI_DIV_CH3:  Period = m_iCH3Period ? m_iCH3Period : 1; break;
	}
	
	while (Time >= m_iSquareCounter) {
		Time -= m_iSquareCounter;
		m_iTime += m_iSquareCounter;
		m_iSquareCounter = Period << 4;
		if ((m_bSqaureActive = !m_bSqaureActive)) {
			int Feedback = m_iLFSRState;
			if (m_bShortNoise)
				Feedback &= 1;
			else
				Feedback = ((Feedback & 0x0009) && ((Feedback & 0x0009) ^ 0x0009));
			m_iLFSRState = (m_iLFSRState >> 1) | (Feedback << (16 - 1));
			Mix(CSN76489::VOLUME_TABLE[m_iAttenuation] * (m_iLFSRState & 1));
		}
	}

	m_iSquareCounter -= Time;
	m_iTime += Time;
}

void CSNNoise::CachePeriod(uint16 Period)
{
	m_iCH3Period = Period;
}



CSN76489::CSN76489(CMixer *pMixer) : CExternal(pMixer)
{
	for (int i = 0; i < 3; ++i)
		m_SquareChannel[i] = new CSNSquare(pMixer, i);
	m_NoiseChannel = new CSNNoise(pMixer);
}

CSN76489::~CSN76489()
{
	for (int i = 0; i < 3; ++i)
		delete m_SquareChannel[i];
	delete m_NoiseChannel;
}

void CSN76489::Reset()
{
	for (int i = 0; i < 3; ++i)
		m_SquareChannel[i]->Reset();
	m_NoiseChannel->Reset();

	m_iAddressLatch = 0;
}

void CSN76489::Process(uint32 Time)
{
	for (int i = 0; i < 3; ++i)
		m_SquareChannel[i]->Process(Time);
	m_NoiseChannel->Process(Time);
}

void CSN76489::EndFrame()
{
	for (int i = 0; i < 3; ++i)
		m_SquareChannel[i]->EndFrame();
	m_NoiseChannel->EndFrame();
}

void CSN76489::Write(uint16 Address, uint8 Value)
{
	switch (Address) {
	case 0: case 2: case 4:
		m_iAddressLatch = (uint8)Address;
		m_SquareChannel[Address / 2]->Write(0, Value & 0x0F);
		break;
	case 1: case 3: case 5:
		m_SquareChannel[Address / 2]->Write(2, Value & 0x0F);
		break;
	case 6:
		m_NoiseChannel->Write(0, Value & 0x07);
		break;
	case 7:
		m_NoiseChannel->Write(2, Value & 0x0F);
		break;
	default:
		m_SquareChannel[m_iAddressLatch / 2]->Write(1, Value & 0x3F);
		if (m_iAddressLatch / 2 == CHANID_TRIANGLE)
			m_NoiseChannel->CachePeriod(m_SquareChannel[CHANID_TRIANGLE]->GetPeriod());
	}

	if (Address == 4 || Address == 5)
		m_NoiseChannel->CachePeriod(m_SquareChannel[CHANID_TRIANGLE]->GetPeriod());
}

uint8 CSN76489::Read(uint16 Address, bool &Mapped)
{
	return 0;
}
