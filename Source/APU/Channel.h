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

#ifndef CHANNEL_H
#define CHANNEL_H

class CMixer;
class CVGMWriterBase;		// // //

//
// This class is used to derive the audio channels
//

// // //

class CExChannel {
public:
	CExChannel(CMixer *pMixer, uint8 Chip, uint8 ID) :
		m_pMixer(pMixer),
		m_iChip(Chip),
		m_iChanId(ID),
		m_iTime(0),
		m_iLastValueLeft(0),		// // //
		m_iLastValueRight(0)
	{
	}

	virtual inline void EndFrame() {
		m_iTime = 0;
	}

	virtual inline void SetVGMWriter(const CVGMWriterBase *pWrite) {		// // //
		m_pVGMWriter = pWrite;
	}

protected:
	inline void Mix(int32 Value) {
		MixLeft(Value);		// // //
		MixRight(Value);
	}
	inline void Mix(int32 Left, int32 Right) {
		MixLeft(Left);
		MixRight(Right);
	}

private:
	inline void MixSingle(int32 Value, int32 &Last, bool Right) {		// // //
		int32 Delta = Value - Last;
		if (Delta)
			m_pMixer->AddValue(m_iChanId, m_iChip, Delta, Value, m_iTime, Right);
		Last = Value;
	}
	inline void MixLeft(int32 Value) {		// // //
		MixSingle(Value, m_iLastValueLeft, false);
	}
	inline void MixRight(int32 Value) {		// // //
		MixSingle(Value, m_iLastValueRight, true);
	}

protected:
	CMixer	*m_pMixer;
	const CVGMWriterBase *m_pVGMWriter = nullptr;

	uint32	m_iTime;			// Cycle counter, resets every new frame
	uint8	m_iChip;
	uint8	m_iChanId;
	int32	m_iLastValueLeft;		// Last value sent to mixer
	int32	m_iLastValueRight;		// // // stereo support
};

#endif /* CHANNEL_H */
