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
#include "../VGM/Writer/Base.h"

const uint16 CSN76489::STEREO_PORT = 0x4F;
const uint16 CSN76489::VOLUME_TABLE[] = {
	1516, 1205, 957, 760, 603, 479, 381, 303, 240, 191, 152, 120, 96, 76, 60, 0
};
const uint16 CSNSquare::CUTOFF_PERIOD = /* 0x01 */ 0x06;



void CSN76489Channel::SetStereo(bool Left, bool Right)
{
	m_bLeft = Left;
	m_bRight = Right;
}

bool CSN76489Channel::GetLeftOutput() const
{
	return m_bLeft;
}

bool CSN76489Channel::GetRightOutput() const
{
	return m_bRight;
}



CSNSquare::CSNSquare(CMixer *pMixer, int ID) :
	CSN76489Channel(pMixer, 0, ID)
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

	SetStereo(true, true);
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
		m_bSqaureActive = Period > CUTOFF_PERIOD ? !m_bSqaureActive : 0;
		int32 Vol = m_bSqaureActive ? CSN76489::VOLUME_TABLE[m_iAttenuation] : 0;
		Mix(GetLeftOutput() ? Vol : 0, GetRightOutput() ? Vol : 0);
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
	CSN76489Channel(pMixer, 0, CHANID_NOISE), m_iCH3Period(0)
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

	SetStereo(true, true);
}

void CSNNoise::Write(uint16 Address, uint8 Value)
{
	switch (Address) {
	case 0:
		m_iFeedbackMode = Value & 0x03;
		m_bShortNoise = (Value & 0x04) == 0;
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
			int32 Vol = (m_iLFSRState & 1) ? CSN76489::VOLUME_TABLE[m_iAttenuation] : 0;
			Mix(GetLeftOutput() ? Vol : 0, GetRightOutput() ? Vol : 0);
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
		m_pChannels[i] = new CSNSquare(pMixer, i);
	m_pChannels[3] = new CSNNoise(pMixer);
}

CSN76489::~CSN76489()
{
	for (auto &x : m_pChannels)
		delete x;
}

void CSN76489::Reset()
{
	for (auto &x : m_pChannels)
		x->Reset();

	m_iAddressLatch = 0;
}

void CSN76489::Process(uint32 Time)
{
	for (auto &x : m_pChannels)
		x->Process(Time);
}

void CSN76489::EndFrame()
{
	for (auto &x : m_pChannels)
		x->EndFrame();
}

void CSN76489::Write(uint16 Address, uint8 Value)
{
	switch (Address) {
	case 0: case 2: case 4:
		Value &= 0x0F;
		m_iAddressLatch = (uint8)Address;
		m_pChannels[Address / 2]->Write(0, Value);
		break;
	case 1: case 3: case 5: case 7:
		Value &= 0x0F;
		m_pChannels[Address / 2]->Write(2, Value);
		break;
	case 6:
		Value &= 0x07;
		m_pChannels[CHANID_NOISE]->Write(0, Value);
		break;
	case STEREO_PORT:
		if (m_pVGMWriter != nullptr)
			m_pVGMWriter->WriteReg(0, Value, 0x06);
		for (int i = 0; i < CHANNEL_COUNT; ++i) {
			m_pChannels[i]->SetStereo((Value & 0x10) != 0, (Value & 0x01) != 0);
			Value >>= 1;
		}
		return;
	default:
		Value &= 0x3F;
		m_pChannels[m_iAddressLatch / 2]->Write(1, Value & 0x3F);
		if (m_iAddressLatch / 2 == CHANID_SQUARE3)
			UpdateNoisePeriod();
		if (m_pVGMWriter != nullptr)
			m_pVGMWriter->WriteReg(0, Value);
	}

	if (Address != -1)
		if (m_pVGMWriter != nullptr)
			m_pVGMWriter->WriteReg(0, 0x80 | (Address << 4) | Value);

	if (Address == 4 || Address == 5)
		UpdateNoisePeriod();
}

uint8 CSN76489::Read(uint16 Address, bool &Mapped)
{
	return 0;
}

void CSN76489::SetVGMWriter(const CVGMWriterBase *pWrite)
{
	m_pVGMWriter = pWrite; // do not propagate to channel classes i guess
}

void CSN76489::UpdateNoisePeriod() const
{
	auto Square = static_cast<CSNSquare*>(m_pChannels[CHANID_SQUARE3]);
	auto Noise = static_cast<CSNNoise*>(m_pChannels[CHANID_NOISE]);
	Noise->CachePeriod(Square->GetPeriod());
}
