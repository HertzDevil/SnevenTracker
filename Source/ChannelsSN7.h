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

//
// Derived channels, SN76489
//

class CChannelHandlerSN7 : public CChannelHandler {		// // //
public:
	CChannelHandlerSN7();
	virtual void ProcessChannel();
	virtual void ResetChannel();

protected:
	virtual void HandleNoteData(stChanNote *pNoteData, int EffColumns);
	virtual void HandleCustomEffects(int EffNum, int EffParam);
	virtual bool HandleInstrument(int Instrument, bool Trigger, bool NewInstrument);
	virtual void HandleEmptyNote();
	virtual void HandleCut();
	virtual void HandleRelease();
	virtual void HandleNote(int Note, int Octave);

protected:
	unsigned char m_cSweep;			// Sweep, used by pulse channels

	bool	m_bManualVolume;		// Flag for Exx
	int		m_iInitVolume;			// Initial volume
	// // //
	int		m_iPostEffect;
	int		m_iPostEffectParam;

	static int m_iRegisterPos[3];		// // //
};

// Square 1
class CSquareChan : public CChannelHandlerSN7 {		// // //
public:
	CSquareChan() : CChannelHandlerSN7() { m_iDefaultDuty = 0; };
	virtual void RefreshChannel();
protected:
	virtual void ClearRegisters();
};

// Noise
class CNoiseChan : public CChannelHandlerSN7 {
public:
	CNoiseChan();
	virtual void RefreshChannel();
protected:
	virtual void ClearRegisters();
	virtual void HandleNote(int Note, int Octave);
	virtual void SetupSlide(int Type, int EffParam);

	int TriggerNote(int Note);

private:
	int m_iLastCtrl;		// // //
};

// // //
