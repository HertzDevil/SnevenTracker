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



void CSN76489Channel::Reset()
{
	SetStereo(true, true);
	SetAttenuation(0xF);
}

void CSN76489Channel::SetVGMWriter(const CVGMWriterBase *pWrite)
{
	m_pVGMWriter = pWrite;
}

void CSN76489Channel::SetStereo(bool Left, bool Right)
{
	m_bLeft = Left;
	m_bRight = Right;
}

void CSN76489Channel::SetAttenuation(uint8 Value)
{
	Value &= 0xF;
	if (Value != m_iAttenuation) {
		if (m_pVGMWriter != nullptr)
			m_pVGMWriter->WriteReg(0, 0x90 | (m_iChanId << 5) | Value);
		m_iAttenuation = Value;
	}
}

int32 CSN76489Channel::GetVolume() const
{
	return CSN76489::VOLUME_TABLE[m_iAttenuation];
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
	CSN76489Channel::Reset();
	
	SetPeriodLo(0);
	SetPeriodHi(0);

	m_iSquareCounter = 0;
	m_bSqaureActive = true;
}

void CSNSquare::Process(uint32 Time)
{
	uint16 Period = GetPeriod();
	if (!Period)
		Period = 1; // or 0x400 according to one of the vgm flags

	while (Time >= m_iSquareCounter) {
		Time -= m_iSquareCounter;
		m_iTime += m_iSquareCounter;
		m_iSquareCounter = Period << 4;
		m_bSqaureActive = Period > CUTOFF_PERIOD ? !m_bSqaureActive : 0;
		int32 Vol = m_bSqaureActive ? GetVolume() : 0;
		Mix(m_bLeft ? Vol : 0, m_bRight ? Vol : 0);
	}

	m_iSquareCounter -= Time;
	m_iTime += Time;
}

void CSNSquare::SetPeriodLo(uint8 Value)
{
	m_iPrevPeriod = GetPeriod();
	m_iSquarePeriodLo = Value & 0xF;
}

void CSNSquare::SetPeriodHi(uint8 Value)
{
	m_iSquarePeriodHi = Value & 0x3F;
	FlushPeriod();
}

void CSNSquare::FlushPeriod() const
{
	if (m_pVGMWriter == nullptr)
		return;
	uint16 NewPeriod = GetPeriod();
	if (NewPeriod == m_iPrevPeriod)
		return;
	m_pVGMWriter->WriteReg(0, 0x80 | (m_iChanId << 5) | m_iSquarePeriodLo);
	if (m_iSquarePeriodHi != (m_iPrevPeriod >> 4))
		m_pVGMWriter->WriteReg(0, m_iSquarePeriodHi);
}

uint16 CSNSquare::GetPeriod() const
{
	return m_iSquarePeriodLo | (m_iSquarePeriodHi << 4);
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
	CSN76489Channel::Reset();

	SetControlMode(SN_NOI_DIV_512 | (SN_NOI_FB_LONG << 2));

	m_iSquareCounter = 0;
	m_bSqaureActive = true;
}

void CSNNoise::Process(uint32 Time)
{
	uint16 Period;
	switch (m_iNoiseFreq) {
	case SN_NOI_DIV_512:  Period = 0x10; break;
	case SN_NOI_DIV_1024: Period = 0x20; break;
	case SN_NOI_DIV_2048: Period = 0x40; break;
	case SN_NOI_DIV_CH3:  Period = m_iCH3Period ? m_iCH3Period : 1; break;
	default: assert(false);
	}
	
	while (Time >= m_iSquareCounter) {
		Time -= m_iSquareCounter;
		m_iTime += m_iSquareCounter;
		m_iSquareCounter = Period << 4;
		if ((m_bSqaureActive = !m_bSqaureActive)) {
			int Feedback = m_iLFSRState;
			switch (m_iNoiseFeedback) {
			case SN_NOI_FB_SHORT: Feedback &= 1; break;
			case SN_NOI_FB_LONG:  Feedback = ((Feedback & 0x0009) && ((Feedback & 0x0009) ^ 0x0009)); break;
			default: assert(false);
			}
			m_iLFSRState = (m_iLFSRState >> 1) | (Feedback << (16 - 1));
			int32 Vol = (m_iLFSRState & 1) ? GetVolume() : 0;
			Mix(m_bLeft ? Vol : 0, m_bRight ? Vol : 0);
		}
	}

	m_iSquareCounter -= Time;
	m_iTime += Time;
}

void CSNNoise::SetControlMode(uint8 Value)
{
	Value &= 0x7;
	m_iNoiseFreq = static_cast<SN_noise_div_t>(Value & 0x03);
	m_iNoiseFeedback = static_cast<SN_noise_fb_t>(Value & 0x04);
	m_iLFSRState = LFSR_INIT;
	if (m_pVGMWriter != nullptr)
		m_pVGMWriter->WriteReg(0, 0xE0 | Value);
}

void CSNNoise::CachePeriod(uint16 Period)
{
	m_iCH3Period = Period;
}



CSN76489::CSN76489(CMixer *pMixer) : CExternal(pMixer)
{
	for (int i = CHANID_SQUARE1; i <= CHANID_SQUARE3; ++i)
		m_pChannels[i] = new CSNSquare(pMixer, i);
	m_pChannels[CHANID_NOISE] = new CSNNoise(pMixer);
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
	for (int i = CHANID_SQUARE1; i <= CHANID_SQUARE3; ++i)
		GetSquare(i)->FlushPeriod();
	for (auto &x : m_pChannels)
		x->EndFrame();
}

void CSN76489::Write(uint16 Address, uint8 Value)
{
	switch (Address) {
	case 0: case 2: case 4:
		m_iAddressLatch = (uint8)Address;
		GetSquare(Address / 2)->SetPeriodLo(Value);
		break;
	case 1: case 3: case 5: case 7:
		GetSquare(m_iAddressLatch / 2)->FlushPeriod();
		m_pChannels[Address / 2]->SetAttenuation(Value);
		break;
	case 6:
		GetSquare(m_iAddressLatch / 2)->FlushPeriod();
		GetNoise()->SetControlMode(Value);
		break;
	case STEREO_PORT:
		if (m_pVGMWriter != nullptr)
			m_pVGMWriter->WriteReg(0, Value, 0x06);
		for (const auto &x : m_pChannels) {
			x->SetStereo((Value & 0x10) != 0, (Value & 0x01) != 0);
			Value >>= 1;
		}
		break;
	default:
		GetSquare(m_iAddressLatch / 2)->SetPeriodHi(Value);
		if (m_iAddressLatch / 2 == CHANID_SQUARE3)
			UpdateNoisePeriod();
	}

	if (Address / 2 == CHANID_SQUARE3)
		UpdateNoisePeriod();
}

uint8 CSN76489::Read(uint16 Address, bool &Mapped)
{
	return 0;
}

void CSN76489::SetVGMWriter(const CVGMWriterBase *pWrite)
{
	for (const auto &x : m_pChannels)
		x->SetVGMWriter(pWrite);
	m_pVGMWriter = pWrite;
}

CSNSquare *CSN76489::GetSquare(uint8 ID) const
{
	assert(ID <= 2u && m_pChannels[ID] != nullptr);
	return static_cast<CSNSquare*>(m_pChannels[ID]);
}

CSNNoise *CSN76489::GetNoise() const
{
	assert(m_pChannels[CHANID_NOISE] != nullptr);
	return static_cast<CSNNoise*>(m_pChannels[CHANID_NOISE]);
}

void CSN76489::UpdateNoisePeriod() const
{
	GetNoise()->CachePeriod(GetSquare(CHANID_SQUARE3)->GetPeriod());
}
