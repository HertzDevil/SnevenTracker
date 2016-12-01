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

#include <map>
#include <vector>
#include <functional>
#include <algorithm>
#include "stdafx.h"
#include "FamiTrackerDoc.h"
#include "Compiler.h"
#include "Chunk.h"
#include "ChunkRenderBinary.h"

/**
 * Binary file writer, base class binary renderers
 */

CBinaryFileWriter::CBinaryFileWriter(CFile *pFile) : m_pFile(pFile), m_iDataWritten(0)
{
}

void CBinaryFileWriter::Store(const void *pData, unsigned int Size)
{
	m_pFile->Write(pData, Size);
	m_iDataWritten += Size;
}

void CBinaryFileWriter::Fill(unsigned int Size)
{
	char d = 0;
	for (unsigned int i = 0; i < Size; ++i)
		m_pFile->Write(&d, 1);
	m_iDataWritten += Size;
}

unsigned int CBinaryFileWriter::GetWritten() const
{
	return m_iDataWritten;
}

/**
 * Binary chunk render, used to write binary files
 *
 */

CChunkRenderBinary::CChunkRenderBinary(CFile *pFile) : CBinaryFileWriter(pFile)		// // //
{
}

void CChunkRenderBinary::StoreChunks(const std::vector<CChunk*> &Chunks) 
{
	std::for_each(Chunks.begin(), Chunks.end(), std::bind1st(std::mem_fun(&CChunkRenderBinary::StoreChunk), this));
}

// // //

void CChunkRenderBinary::StoreChunk(CChunk *pChunk)
{
	for (int i = 0; i < pChunk->GetLength(); ++i) {
		if (pChunk->GetType() == CHUNK_PATTERN) {
			const std::vector<char> &vec = pChunk->GetStringData(CCompiler::PATTERN_CHUNK_INDEX);
			Store(&vec.front(), vec.size());
		}
		else {
			unsigned short data = pChunk->GetData(i);
			unsigned short size = pChunk->GetDataSize(i);
			Store(&data, size);
		}
	}
}

// // //


/**
 * NSF chunk render, used to write NSF files
 *
 */

CChunkRenderNSF::CChunkRenderNSF(CFile *pFile, unsigned int StartAddr) : 
	CBinaryFileWriter(pFile),
	m_iStartAddr(StartAddr)		// // //
{
}

void CChunkRenderNSF::StoreDriver(const char *pDriver, unsigned int Size)
{
	// Store NSF driver
	Store(pDriver, Size);
}

void CChunkRenderNSF::StoreChunks(const std::vector<CChunk*> &Chunks)
{
	// Store chunks into NSF banks
	std::for_each(Chunks.begin(), Chunks.end(), std::bind1st(std::mem_fun(&CChunkRenderNSF::StoreChunk), this));
}

void CChunkRenderNSF::StoreChunksBankswitched(const std::vector<CChunk*> &Chunks)
{
	// Store chunks into NSF banks with bankswitching
	std::for_each(Chunks.begin(), Chunks.end(), std::bind1st(std::mem_fun(&CChunkRenderNSF::StoreChunkBankswitched), this));
}

// // //

int CChunkRenderNSF::GetBankCount() const
{
	return GetBank() + 1;
}

void CChunkRenderNSF::StoreChunkBankswitched(const CChunk *pChunk)
{
	switch (pChunk->GetType()) {			
		case CHUNK_FRAME_LIST:
		case CHUNK_FRAME:
		case CHUNK_PATTERN:
			// Switchable data
			while ((GetBank() + 1) <= pChunk->GetBank() && pChunk->GetBank() > CCompiler::PATTERN_SWITCH_BANK)
				AllocateNewBank();
	}

	// Write chunk
	StoreChunk(pChunk);
}

void CChunkRenderNSF::StoreChunk(const CChunk *pChunk)
{
	for (int i = 0; i < pChunk->GetLength(); ++i) {
		if (pChunk->GetType() == CHUNK_PATTERN) {
			const std::vector<char> &vec = pChunk->GetStringData(CCompiler::PATTERN_CHUNK_INDEX);
			Store(&vec.front(), vec.size());			
		}
		else {
			unsigned short data = pChunk->GetData(i);
			unsigned short size = pChunk->GetDataSize(i);
			Store(&data, size);
		}
	}
}

int CChunkRenderNSF::GetRemainingSize() const
{
	// Return remaining bank size
	return 0x1000 - (GetWritten() & 0xFFF);
}

void CChunkRenderNSF::AllocateNewBank()
{
	// Get new NSF bank
	int Remaining = GetRemainingSize();
	Fill(Remaining);
}

int CChunkRenderNSF::GetBank() const
{
	return GetWritten() >> 12;
}

int CChunkRenderNSF::GetAbsoluteAddr() const
{
	// Return NSF address
	return m_iStartAddr + GetWritten();
}


/**
 * NES chunk render, borrows from NSF render
 *
 */

CChunkRenderNES::CChunkRenderNES(CFile *pFile, unsigned int StartAddr) : CChunkRenderNSF(pFile, StartAddr)
{
}

void CChunkRenderNES::StoreCaller(const void *pData, unsigned int Size)
{
	while (GetBank() < 7)
		AllocateNewBank();

	int FillSize = (0x10000 - GetAbsoluteAddr()) - Size;
	Fill(FillSize);
	Store(pData, Size);
}
