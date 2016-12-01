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

#ifndef MIXER_H
#define MIXER_H

#include "Types.h"
#include "../Common.h"
#include "../Blip_Buffer/blip_buffer.h"

enum chip_level_t {
	CHIP_LEVEL_APU1,
	CHIP_LEVEL_APU2,
	// // //
};

class CMixer
{
public:
	CMixer();
	~CMixer();

	void	ExternalSound(int Chip);
	void	AddValue(int ChanID, int Chip, int Value, int AbsValue, int FrameCycles);
	void	UpdateSettings(int LowCut,	int HighCut, int HighDamp, float OverallVol);

	bool	AllocateBuffer(unsigned int Size, uint32 SampleRate, uint8 NrChannels);
	void	SetClockRate(uint32 Rate);
	void	ClearBuffer();
	int		FinishBuffer(int t);
	int		SamplesAvail() const;
	void	MixSamples(blip_sample_t *pBuffer, uint32 Count);
	uint32	GetMixSampleCount(int t) const;

	void	AddSample(int ChanID, int Value);
	int		ReadBuffer(int Size, void *Buffer, bool Stereo);

	int32	GetChanOutput(uint8 Chan) const;
	void	SetChipLevel(chip_level_t Chip, float Level);
	uint32	ResampleDuration(uint32 Time) const;

private:
	// // //

	void StoreChannelLevel(int Channel, int Value);
	void ClearChannelLevels();

	float GetAttenuation() const;

private:
	// Blip buffer synths
	Blip_Synth<blip_good_quality, -500>		Synth2A03SS;
	Blip_Synth<blip_good_quality, -500>		Synth2A03TND;
	Blip_Synth<blip_good_quality, -3000>		SynthSN76489Left;		// // //
	Blip_Synth<blip_good_quality, -3000>		SynthSN76489Right;
	// // //
	
	// Blip buffer object
	Blip_Buffer	BlipBuffer;

	double		m_dSumSS;
	double		m_dSumTND;

	int32		m_iChannels[CHANNELS];
	uint8		m_iExternalChip;
	uint32		m_iSampleRate;

	float		m_fChannelLevels[CHANNELS];
	uint32		m_iChanLevelFallOff[CHANNELS];

	int			m_iLowCut;
	int			m_iHighCut;
	int			m_iHighDamp;
	float		m_fOverallVol;

	float		m_fLevelAPU1;
	float		m_fLevelAPU2;
	// // //
};

#endif /* MIXER_H */
