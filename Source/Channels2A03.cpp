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

// This file handles playing of 2A03 channels

#include <cmath>
#include "stdafx.h"
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "ChannelHandler.h"
#include "Channels2A03.h"
#include "Settings.h"
#include "SoundGen.h"

//#define NOISE_PITCH_SCALE

CChannelHandler2A03::CChannelHandler2A03() : 
	CChannelHandler(0x3FF, 0x0F),		// // //
	m_bManualVolume(0),
	m_iInitVolume(0),
	// // //
	m_iPostEffect(0),
	m_iPostEffectParam(0)
{
}

void CChannelHandler2A03::HandleNoteData(stChanNote *pNoteData, int EffColumns)
{
	m_iPostEffect = 0;
	m_iPostEffectParam = 0;
	// // //
	m_iInitVolume = 0x0F;
	m_bManualVolume = false;

	CChannelHandler::HandleNoteData(pNoteData, EffColumns);

	if (pNoteData->Note != NONE && pNoteData->Note != HALT && pNoteData->Note != RELEASE) {
		if (m_iPostEffect && (m_iEffect == EF_SLIDE_UP || m_iEffect == EF_SLIDE_DOWN))
			SetupSlide(m_iPostEffect, m_iPostEffectParam);
		else if (m_iEffect == EF_SLIDE_DOWN || m_iEffect == EF_SLIDE_UP)
			m_iEffect = EF_NONE;
	}
}

void CChannelHandler2A03::HandleCustomEffects(int EffNum, int EffParam)
{
	#define GET_SLIDE_SPEED(x) (((x & 0xF0) >> 3) + 1)

	if (!CheckCommonEffects(EffNum, EffParam)) {
		// Custom effects
		switch (EffNum) {
			// // //
			case EF_DUTY_CYCLE:
				m_iDefaultDuty = m_iDutyPeriod = EffParam;
				break;
			case EF_SLIDE_UP:
			case EF_SLIDE_DOWN:
				m_iPostEffect = EffNum;
				m_iPostEffectParam = EffParam;
				SetupSlide(EffNum, EffParam);
				break;
		}
	}
}

bool CChannelHandler2A03::HandleInstrument(int Instrument, bool Trigger, bool NewInstrument)
{
	CFamiTrackerDoc *pDocument = m_pSoundGen->GetDocument();
	CInstrumentContainer<CInstrument2A03> instContainer(pDocument, Instrument);
	CInstrument2A03 *pInstrument = instContainer();

	if (pInstrument == NULL)
		return false;

	for (int i = 0; i < CInstrument2A03::SEQUENCE_COUNT; ++i) {
		const CSequence *pSequence = pDocument->GetSequence(SNDCHIP_NONE, pInstrument->GetSeqIndex(i), i);
		if (Trigger || !IsSequenceEqual(i, pSequence) || pInstrument->GetSeqEnable(i) > GetSequenceState(i)) {
			if (pInstrument->GetSeqEnable(i) == 1)
				SetupSequence(i, pSequence);
			else
				ClearSequence(i);
		}
	}

	return true;
}

void CChannelHandler2A03::HandleEmptyNote()
{
	if (m_bManualVolume)
		m_iSeqVolume = m_iInitVolume;
	// // //
}

void CChannelHandler2A03::HandleCut()
{
	CutNote();
}

void CChannelHandler2A03::HandleRelease()
{
	if (!m_bRelease) {
		ReleaseNote();
		ReleaseSequences();
	}
	// // //
}

void CChannelHandler2A03::HandleNote(int Note, int Octave)
{
	m_iNote			= RunNote(Octave, Note);
	m_iDutyPeriod	= m_iDefaultDuty;
	m_iSeqVolume	= m_iInitVolume;

	m_iArpState = 0;

	// // //
}

void CChannelHandler2A03::ProcessChannel()
{
	// Default effects
	CChannelHandler::ProcessChannel();
	
	// // //

	// Sequences
	for (int i = 0; i < CInstrument2A03::SEQUENCE_COUNT; ++i)
		RunSequence(i);
}

void CChannelHandler2A03::ResetChannel()
{
	CChannelHandler::ResetChannel();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// Square 1 
///////////////////////////////////////////////////////////////////////////////////////////////////////////

void CSquare1Chan::RefreshChannel()
{
	int Period = CalculatePeriod();
	int Volume = CalculateVolume();
	// // //
	unsigned char HiFreq = (Period >> 4) & 0x3F;
	unsigned char LoFreq = (Period & 0xF);

	WriteRegister(0x01 + m_iChannelID * 2, 0xF ^ Volume);		// // //
	WriteRegister(0x00 + m_iChannelID * 2, LoFreq);
	WriteRegister(  -1, HiFreq); // double-byte
}

void CSquare1Chan::ClearRegisters()
{
	// // //
	WriteRegister(0x00 + m_iChannelID * 2, 0x00);
	WriteRegister(  -1, 0x00); // double-byte
	WriteRegister(0x01 + m_iChannelID * 2, 0xF);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// Square 2 
///////////////////////////////////////////////////////////////////////////////////////////////////////////

void CSquare2Chan::RefreshChannel()
{
	int Period = CalculatePeriod();
	int Volume = CalculateVolume();
	char DutyCycle = (m_iDutyPeriod & 0x03);

	unsigned char HiFreq		= (Period & 0xFF);
	unsigned char LoFreq		= (Period >> 8);
	unsigned char LastLoFreq	= (m_iLastPeriod >> 8);

	if (!m_bGate || !Volume) {
		//DutyCycle = 0;
		WriteRegister(0x4004, 0x30);
		m_iLastPeriod = 0xFFFF;
		return;
	}

	WriteRegister(0x4004, (DutyCycle << 6) | 0x30 | Volume);

	if (m_cSweep) {
		if (m_cSweep & 0x80) {
			WriteRegister(0x4005, m_cSweep);
			m_cSweep &= 0x7F;
			WriteRegister(0x4017, 0x80);		// Clear sweep unit
			WriteRegister(0x4017, 0x00);
			WriteRegister(0x4006, HiFreq);
			WriteRegister(0x4007, LoFreq);
			m_iLastPeriod = 0xFFFF;
		}
	}
	else {
		WriteRegister(0x4005, 0x08);
		WriteRegister(0x4017, 0x80);
		WriteRegister(0x4017, 0x00);
		WriteRegister(0x4006, HiFreq);
		
		if (LoFreq != LastLoFreq)
			WriteRegister(0x4007, LoFreq);
	}

	m_iLastPeriod = Period;
}

void CSquare2Chan::ClearRegisters()
{
	WriteRegister(0x4004, 0x30);
	WriteRegister(0x4005, 0x08);
	WriteRegister(0x4006, 0x00);
	WriteRegister(0x4007, 0x00);
	m_iLastPeriod = 0xFFFF;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// Triangle 
///////////////////////////////////////////////////////////////////////////////////////////////////////////

void CTriangleChan::RefreshChannel()
{
	int Freq = CalculatePeriod();

	unsigned char HiFreq = (Freq & 0xFF);
	unsigned char LoFreq = (Freq >> 8);
	
	if (m_iSeqVolume > 0 && m_iVolume > 0 && m_bGate) {
		WriteRegister(0x4008, 0x81);
		WriteRegister(0x400A, HiFreq);
		WriteRegister(0x400B, LoFreq);
	}
	else
		WriteRegister(0x4008, 0);
}

void CTriangleChan::ClearRegisters()
{
	WriteRegister(0x4008, 0);
	WriteRegister(0x4009, 0);
	WriteRegister(0x400A, 0);
	WriteRegister(0x400B, 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// Noise
///////////////////////////////////////////////////////////////////////////////////////////////////////////

CNoiseChan::CNoiseChan() : CChannelHandler2A03() 
{ 
	m_iDefaultDuty = 0; 
	/*
#ifdef NOISE_PITCH_SCALE
	SetMaxPeriod(0xFF); 
#else
	SetMaxPeriod(0x0F); 
#endif
	*/
}

void CNoiseChan::HandleNote(int Note, int Octave)
{
	int NewNote = MIDI_NOTE(Octave, Note);
	int NesFreq = TriggerNote(NewNote);

	NesFreq = (NesFreq & 0x03);		// // //
	if (!(NesFreq & 0x01))
		NesFreq ^= 0x02;

//	NewNote &= 0x0F;

	if (m_iPortaSpeed > 0 && m_iEffect == EF_PORTAMENTO) {
		if (m_iPeriod == 0)
			m_iPeriod = NesFreq;
		m_iPortaTo = NesFreq;
	}
	else
		m_iPeriod = NesFreq;

	m_bGate = true;

	m_iNote			= NewNote;
	m_iDutyPeriod	= m_iDefaultDuty;
	m_iSeqVolume	= m_iInitVolume;
}

void CNoiseChan::SetupSlide(int Type, int EffParam)
{
	CChannelHandler::SetupSlide(Type, EffParam);

	// Work-around for noise
	if (m_iEffect == EF_SLIDE_DOWN)
		m_iEffect = EF_SLIDE_UP;
	else
		m_iEffect = EF_SLIDE_DOWN;
}

/*
int CNoiseChan::CalculatePeriod() const
{
	return LimitPeriod(m_iPeriod - GetVibrato() + GetFinePitch() + GetPitch());
}
*/

void CNoiseChan::RefreshChannel()
{
	int Period = CalculatePeriod();
	int Volume = CalculateVolume();
	char NoiseMode = m_iDutyPeriod & 0x01;

	if (!m_bGate || !Volume) {
		WriteRegister(0x07, 0xF);		// // //
		return;
	}

#ifdef NOISE_PITCH_SCALE
	Period = (Period >> 4) & 0x0F;
#else
	Period = Period & 0x03;
#endif

	int newCtrl = (NoiseMode << 2) | Period;		// // //
	if (newCtrl != m_iLastCtrl) {
		WriteRegister(0x06, newCtrl);
		m_iLastCtrl = newCtrl;
	}
	WriteRegister(0x07, 0xF ^ Volume);
}

void CNoiseChan::ClearRegisters()
{
	m_iLastCtrl = 0;		// // //
	WriteRegister(0x06, 0);
	WriteRegister(0x07, 0xF);
}

int CNoiseChan::TriggerNote(int Note)
{
	// Clip range to 0-15
	/*
	if (Note > 0x0F)
		Note = 0x0F;
	if (Note < 0)
		Note = 0;
		*/

	RegisterKeyState(Note);

//	Note &= 0x0F;

#ifdef NOISE_PITCH_SCALE
	return (Note ^ 0x0F) << 4;
#else
	return Note;		// // //
#endif
}

// // //
