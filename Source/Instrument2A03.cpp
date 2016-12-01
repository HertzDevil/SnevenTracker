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

#include <vector>
#include "stdafx.h"
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "Instrument.h"
#include "Compiler.h"
#include "Chunk.h"
#include "DocumentFile.h"

// 2A03 instruments

const int CInstrument2A03::SEQUENCE_TYPES[] = {
	SEQ_VOLUME, 
	SEQ_ARPEGGIO, 
	SEQ_PITCH, 
	SEQ_HIPITCH, 
	SEQ_DUTYCYCLE
};

CInstrument2A03::CInstrument2A03()
{
	for (int i = 0; i < SEQUENCE_COUNT; ++i) {
		m_iSeqEnable[i] = 0;
		m_iSeqIndex[i] = 0;
	}

	// // //
}

CInstrument *CInstrument2A03::Clone() const
{
	CInstrument2A03 *pNew = new CInstrument2A03();

	for (int i = 0; i < SEQUENCE_COUNT; ++i) {
		pNew->SetSeqEnable(i, GetSeqEnable(i));
		pNew->SetSeqIndex(i, GetSeqIndex(i));
	}

	// // //

	pNew->SetName(GetName());

	return pNew;
}

void CInstrument2A03::Setup()
{
	CFamiTrackerDoc *pDoc = CFamiTrackerDoc::GetDoc();

	// Select free sequences
	for (int i = 0; i < SEQ_COUNT; ++i) {
		SetSeqEnable(i, 0);
		int Slot = pDoc->GetFreeSequence(i);
		if (Slot != -1)
			SetSeqIndex(i, Slot);
	}
}

void CInstrument2A03::Store(CDocumentFile *pDocFile)
{
	pDocFile->WriteBlockInt(SEQUENCE_COUNT);

	for (int i = 0; i < SEQUENCE_COUNT; ++i) {
		pDocFile->WriteBlockChar(GetSeqEnable(i));
		pDocFile->WriteBlockChar(GetSeqIndex(i));
	}

	// // //
}

bool CInstrument2A03::Load(CDocumentFile *pDocFile)
{
	int Version = pDocFile->GetBlockVersion();

	int SeqCnt = pDocFile->GetBlockInt();
	ASSERT_FILE_DATA(SeqCnt < (SEQUENCE_COUNT + 1));

	for (int i = 0; i < SeqCnt; ++i) {
		SetSeqEnable(i, pDocFile->GetBlockChar());
		int Index = pDocFile->GetBlockChar();
		ASSERT_FILE_DATA(Index < MAX_SEQUENCES);
		SetSeqIndex(i, Index);
	}

	// // //

	return true;
}

void CInstrument2A03::SaveFile(CInstrumentFile *pFile, const CFamiTrackerDoc *pDoc)
{
	// Saves an 2A03 instrument
	// Current version 2.4

	// Sequences
	pFile->WriteChar(SEQUENCE_COUNT);

	for (int i = 0; i < SEQUENCE_COUNT; ++i) {
		unsigned int Sequence = GetSeqIndex(i);
		if (GetSeqEnable(i)) {
			const CSequence *pSeq = pDoc->GetSequence(Sequence, i);
			pFile->WriteChar(1);
			pFile->WriteInt(pSeq->GetItemCount());
			pFile->WriteInt(pSeq->GetLoopPoint());
			pFile->WriteInt(pSeq->GetReleasePoint());
			pFile->WriteInt(pSeq->GetSetting());
			for (unsigned int j = 0; j < pSeq->GetItemCount(); j++) {
				pFile->WriteChar(pSeq->GetItem(j));
			}
		}
		else {
			pFile->WriteChar(0);
		}
	}

	// // //
}

bool CInstrument2A03::LoadFile(CInstrumentFile *pFile, int iVersion, CFamiTrackerDoc *pDoc)
{
	// Reads an FTI file
	//

	// // //

	// Sequences
	unsigned char SeqCount = pFile->ReadChar();

	// Loop through all instrument effects
	for (unsigned int i = 0; i < SeqCount; ++i) {

		unsigned char Enabled = pFile->ReadChar();
		if (Enabled == 1) {
			// Read the sequence
			int Count = pFile->ReadInt();
			if (Count < 0 || Count > MAX_SEQUENCE_ITEMS)
				return false;

			// Find a free sequence
			int Index = pDoc->GetFreeSequence(i);
			if (Index != -1) {
				CSequence *pSeq = pDoc->GetSequence((unsigned)Index, i);

				if (iVersion < 20)
					return false;		// // //
				else {
					pSeq->SetItemCount(Count);
					int LoopPoint = pFile->ReadInt();
					pSeq->SetLoopPoint(LoopPoint);
					if (iVersion > 20) {
						int ReleasePoint = pFile->ReadInt();
						pSeq->SetReleasePoint(ReleasePoint);
					}
					if (iVersion >= 23) {
						int Setting = pFile->ReadInt();
						pSeq->SetSetting(Setting);
					}
					for (int j = 0; j < Count; ++j) {
						char Val = pFile->ReadChar();
						pSeq->SetItem(j, Val);
					}
				}
				SetSeqEnable(i, true);
				SetSeqIndex(i, Index);
			}
		}
		else {
			SetSeqEnable(i, false);
			SetSeqIndex(i, 0);
		}
	}

	// // //

	return true;
}

int CInstrument2A03::Compile(CFamiTrackerDoc *pDoc, CChunk *pChunk, int Index)
{
	int ModSwitch = 0;
	int StoredBytes = 0;

	for (unsigned int i = 0; i < SEQUENCE_COUNT; ++i) {
		const CSequence *pSequence = pDoc->GetSequence(unsigned(GetSeqIndex(i)), i);
		ModSwitch = (ModSwitch >> 1) | ((GetSeqEnable(i) && (pSequence->GetItemCount() > 0)) ? 0x10 : 0);
	}
	
	pChunk->StoreByte(ModSwitch);
	StoredBytes++;

	for (int i = 0; i < SEQUENCE_COUNT; ++i) {
		const CSequence *pSequence = pDoc->GetSequence(unsigned(GetSeqIndex(i)), i);
		if (GetSeqEnable(i) != 0 && (pSequence->GetItemCount() != 0)) {
			CStringA str;
			str.Format(CCompiler::LABEL_SEQ_2A03, GetSeqIndex(i) * SEQUENCE_COUNT + i);
			pChunk->StoreReference(str);
			StoredBytes += 2;
		}
	}

	return StoredBytes;
}

bool CInstrument2A03::CanRelease() const
{
	if (GetSeqEnable(0) != 0) {
		int index = GetSeqIndex(SEQ_VOLUME);
		return CFamiTrackerDoc::GetDoc()->GetSequence(SNDCHIP_NONE, index, SEQ_VOLUME)->GetReleasePoint() != -1;
	}

	return false;
}

int	CInstrument2A03::GetSeqEnable(int Index) const
{
	return m_iSeqEnable[Index];
}

int	CInstrument2A03::GetSeqIndex(int Index) const
{
	return m_iSeqIndex[Index];
}

void CInstrument2A03::SetSeqEnable(int Index, int Value)
{
	if (m_iSeqEnable[Index] != Value)
		InstrumentChanged();
	m_iSeqEnable[Index] = Value;
}

void CInstrument2A03::SetSeqIndex(int Index, int Value)
{
	if (m_iSeqIndex[Index] != Value)
		InstrumentChanged();
	m_iSeqIndex[Index] = Value;
}

// // //
