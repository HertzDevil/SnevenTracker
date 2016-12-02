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

// // //
#include <map>
#include <vector>
#include "stdafx.h"
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "PatternCompiler.h"
#include "Compiler.h"
#include "Chunk.h"
#include "ChunkRenderText.h"
#include "ChunkRenderBinary.h"
#include "Driver.h"
#include "SoundGen.h"
#include "APU/APU.h"

//
// This is the new NSF data compiler, music is compiled to an object list instead of a binary chunk
//
// The list can be translated to both a binary chunk and an assembly file
// 

/*
 * TODO:
 *  - Remove duplicated FDS waves
 *  - Remove the bank value in CHUNK_SONG??
 *  - Derive classes for each output format instead of separate functions
 *  - Create a config file for NSF driver optimizations
 *  - Pattern hash collisions prevents detecting similar patterns, fix that
 *  - Add bankswitching schemes for other memory mappers
 *
 */

/*
 * Notes:
 *
 *  // // //
 *  - Instrument data is non bankswitched, it might be possible to create
 *    instrument data of a size that makes export impossible.
 *
 */

/*
 * Bankswitched file layout:
 *
 * - $8000 - $AFFF: Music driver and song data (instruments, frames & patterns, unpaged)
 * - $B000 - $BFFF: Swichted part of song data (frames + patterns, 1 page only)
 * - $C000 - $EFFF: Samples (3 pages)
 * - $F000 - $FFFF: Fixed to last bank for compatibility with TNS HFC carts
 *
 * Non-bankswitched, compressed layout:
 *
 * - Music data, driver		// // //
 * 
 * Non-bankswitched + bankswitched, default layout:
 *
 * - Driver, music data,		// // //
 *
 */

// Note: Each CCompiler object may only be used once (fix this)

// Remove duplicated patterns (default on)
#define REMOVE_DUPLICATE_PATTERNS

// Don't remove patterns across different tracks (default off)
//#define LOCAL_DUPLICATE_PATTERN_REMOVAL

// Enable bankswitching on all songs (default off)
//#define FORCE_BANKSWITCH

const int CCompiler::PATTERN_CHUNK_INDEX		= 0;		// Fixed at 0 for the moment

const int CCompiler::PAGE_SIZE					= 0x1000;
const int CCompiler::PAGE_START					= 0x8000;
const int CCompiler::PAGE_BANKED				= 0xB000;	// 0xB000 -> 0xBFFF
const int CCompiler::PAGE_SAMPLES				= 0xC000;

const int CCompiler::PATTERN_SWITCH_BANK		= 3;		// 0xB000 -> 0xBFFF

// // //

const bool CCompiler::LAST_BANK_FIXED			= true;		// Fix for TNS carts

// Assembly labels
const char CCompiler::LABEL_SONG_LIST[]			= "ft_song_list";
const char CCompiler::LABEL_INSTRUMENT_LIST[]	= "ft_instrument_list";
// // //
const char CCompiler::LABEL_SEQ_2A03[]			= "ft_seq_2a03_%i";			// one argument
// // //
const char CCompiler::LABEL_INSTRUMENT[]		= "ft_inst_%i";				// one argument
const char CCompiler::LABEL_SONG[]				= "ft_song_%i";				// one argument
const char CCompiler::LABEL_SONG_FRAMES[]		= "ft_s%i_frames";			// one argument
const char CCompiler::LABEL_SONG_FRAME[]		= "ft_s%if%i";				// two arguments
const char CCompiler::LABEL_PATTERN[]			= "ft_s%ip%ic%i";			// three arguments

// Flag byte flags
const int CCompiler::FLAG_BANKSWITCHED	= 1 << 0;
const int CCompiler::FLAG_VIBRATO		= 1 << 1;

CCompiler *CCompiler::pCompiler = NULL;

CCompiler *CCompiler::GetCompiler()
{
	return pCompiler;
}

// // //

// CCompiler

CCompiler::CCompiler(CFamiTrackerDoc *pDoc, CCompilerLog *pLogger) : 
	m_pDocument(pDoc), 
	m_pLogger(pLogger),
	// // //
	m_pHeaderChunk(NULL),
	m_pDriverData(NULL),
	// // //
	m_iHashCollisions(0)
{
	ASSERT(CCompiler::pCompiler == NULL);
	CCompiler::pCompiler = this;
}

CCompiler::~CCompiler()
{
	CCompiler::pCompiler = NULL;

	Cleanup();

	SAFE_RELEASE(m_pLogger);
}

void CCompiler::Print(LPCTSTR text, ...) const
{
 	static TCHAR buf[256];

	if (m_pLogger == NULL)
		return;

	va_list argp;
    va_start(argp, text);

    if (!text)
		return;

	_vsntprintf_s(buf, sizeof(buf), _TRUNCATE, text, argp);

	size_t len = _tcslen(buf);

	if (buf[len - 1] == '\n' && len < (sizeof(buf) - 1)) {
		buf[len - 1] = '\r';
		buf[len] = '\n';
		buf[len + 1] = 0;
	}

	m_pLogger->WriteLog(buf);
}

void CCompiler::ClearLog() const
{
	if (m_pLogger != NULL)
		m_pLogger->Clear();
}

bool CCompiler::OpenFile(LPCTSTR lpszFileName, CFile &file) const
{
	CFileException ex;

	if (!file.Open(lpszFileName, CFile::modeWrite | CFile::modeCreate, &ex)) {
		// Display formatted file exception message
		TCHAR szCause[255];
		CString strFormatted;
		ex.GetErrorMessage(szCause, 255);
		AfxFormatString1(strFormatted, IDS_OPEN_FILE_ERROR, szCause);
		AfxMessageBox(strFormatted, MB_OK | MB_ICONERROR);
		return false;
	}

	return true;
}

void CCompiler::ExportNSF(LPCTSTR lpszFileName, int MachineType)
{
	ClearLog();

	// Build the music data
	if (!CompileData()) {
		// Failed
		Cleanup();
		return;
	}

	if (m_bBankSwitched) {
		// Expand and allocate label addresses
		AddBankswitching();
		if (!ResolveLabelsBankswitched()) {
			Cleanup();
			return;
		}
		// Write bank data
		UpdateFrameBanks();
		UpdateSongBanks();
		// Make driver aware of bankswitching
		EnableBankswitching();
	}
	else {
		ResolveLabels();
		ClearSongBanks();
	}

	// // //

	// Compressed mode means that driver and music is located just below the sample space, no space is lost even when samples are used
	bool bCompressedMode;
	unsigned short MusicDataAddress;

	// Find out load address
	if ((PAGE_SAMPLES - m_iDriverSize - m_iMusicDataSize) < 0x8000 || m_bBankSwitched)
		bCompressedMode = false;
	else
		bCompressedMode = true;
	
	if (bCompressedMode) {
		// Locate driver at $C000 - (driver size)
		m_iLoadAddress = PAGE_SAMPLES - m_iDriverSize - m_iMusicDataSize;
		m_iDriverAddress = PAGE_SAMPLES - m_iDriverSize;
		MusicDataAddress = m_iLoadAddress;
	}
	else {
		// Locate driver at $8000
		m_iLoadAddress = PAGE_START;
		m_iDriverAddress = PAGE_START;
		MusicDataAddress = m_iLoadAddress + m_iDriverSize;
	}

	// Init is located first at the driver
	m_iInitAddress = m_iDriverAddress + 8;

	// Load driver
	std::unique_ptr<char[]> pDriverPtr(LoadDriver(m_pDriverData, m_iDriverAddress));		// // //
	char *pDriver = pDriverPtr.get();

	// Patch driver binary
	PatchVibratoTable(pDriver);

	// // //

	// Write music data address
	SetDriverSongAddress(pDriver, MusicDataAddress);

	// Open output file
	CFile OutputFile;
	if (!OpenFile(lpszFileName, OutputFile)) {
		Print(_T("Error: Could not open output file\n"));
		Cleanup();
		return;
	}

	// Create NSF header
	stNSFHeader Header;
	CreateHeader(&Header, MachineType);

	// Write header
	OutputFile.Write(&Header, sizeof(stNSFHeader));

	// Write NSF data
	CChunkRenderNSF Render(&OutputFile, m_iLoadAddress);

	if (m_bBankSwitched) {
		Render.StoreDriver(pDriver, m_iDriverSize);
		Render.StoreChunksBankswitched(m_vChunks);
		// // //
	}
	else {
		if (bCompressedMode) {
			Render.StoreChunks(m_vChunks);
			Render.StoreDriver(pDriver, m_iDriverSize);
			// // //
		}
		else {
			Render.StoreDriver(pDriver, m_iDriverSize);
			Render.StoreChunks(m_vChunks);
			// // //
		}
	}

	// Writing done, print some stats
	Print(_T(" * NSF load address: $%04X\n"), m_iLoadAddress);
	Print(_T("Writing output file...\n"));
	Print(_T(" * Driver size: %i bytes\n"), m_iDriverSize);

	if (m_bBankSwitched) {
		int Percent = (100 * m_iMusicDataSize) / (0x80000 - m_iDriverSize);		// // //
		int Banks = Render.GetBankCount();
		Print(_T(" * Song data size: %i bytes (%i%%)\n"), m_iMusicDataSize, Percent);
		Print(_T(" * NSF type: Bankswitched (%i banks)\n"), Banks - 1);
	}
	else {
		int Percent = (100 * m_iMusicDataSize) / (0x8000 - m_iDriverSize);		// // //
		Print(_T(" * Song data size: %i bytes (%i%%)\n"), m_iMusicDataSize, Percent);
		Print(_T(" * NSF type: Linear (driver @ $%04X)\n"), m_iDriverAddress);
	}

	Print(_T("Done, total file size: %i bytes\n"), OutputFile.GetLength());

	// Done
	OutputFile.Close();

	Cleanup();
}

void CCompiler::ExportNES(LPCTSTR lpszFileName, bool EnablePAL)
{
	// 32kb NROM, no CHR
	const char NES_HEADER[] = {
		0x4E, 0x45, 0x53, 0x1A, 0x02, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};

	ClearLog();

	if (m_pDocument->GetExpansionChip() != SNDCHIP_NONE) {
		Print(_T("Error: Expansion chips not supported.\n"));
		AfxMessageBox(_T("Expansion chips are currently not supported when exporting to .NES!"), 0, 0);
		return;
	}

	CFile OutputFile;
	if (!OpenFile(lpszFileName, OutputFile)) {
		return;
	}

	// Build the music data
	if (!CompileData()) {
		Cleanup();
		return;
	}

	if (m_bBankSwitched) {
		// Abort if larger than 32kb
		Print(_T("Error: Song is too large, aborted.\n"));
		AfxMessageBox(_T("Song is too big to fit into 32kB!"), 0, 0);
		Cleanup();
		return;
	}

	ResolveLabels();

	// // //

	// Locate driver at $8000
	m_iLoadAddress = PAGE_START;
	m_iDriverAddress = PAGE_START;
	unsigned short MusicDataAddress = m_iLoadAddress + m_iDriverSize;

	// Init is located first at the driver
	m_iInitAddress = m_iDriverAddress + 8;

	// Load driver
	std::unique_ptr<char[]>pDriverPtr(LoadDriver(m_pDriverData, m_iDriverAddress));		// // //
	char *pDriver = pDriverPtr.get();

	// Patch driver binary
	PatchVibratoTable(pDriver);

	// Write music data address
	SetDriverSongAddress(pDriver, MusicDataAddress);

	int Percent = (100 * m_iMusicDataSize) / (0x8000 - m_iDriverSize);		// // //

	Print(_T("Writing file...\n"));
	Print(_T(" * Driver size: %i bytes\n"), m_iDriverSize);
	Print(_T(" * Song data size: %i bytes (%i%%)\n"), m_iMusicDataSize, Percent);

	// Write header
	OutputFile.Write(NES_HEADER, 0x10);

	// Write NES data
	CChunkRenderNES Render(&OutputFile, m_iLoadAddress);
	Render.StoreDriver(pDriver, m_iDriverSize);
	Render.StoreChunks(m_vChunks);
	// // //
	Render.StoreCaller(NSF_CALLER_BIN, NSF_CALLER_SIZE);

	Print(_T("Done, total file size: %i bytes\n"), 0x8000 + 0x10);

	// Done
	OutputFile.Close();

	Cleanup();
}

void CCompiler::ExportBIN(LPCTSTR lpszBIN_File)		// // //
{
	ClearLog();

	// Build the music data
	if (!CompileData())
		return;

	if (m_bBankSwitched) {
		Print(_T("Error: Can't write bankswitched songs!\n"));
		return;
	}

	// Convert to binary
	ResolveLabels();

	CFile OutputFileBIN;
	if (!OpenFile(lpszBIN_File, OutputFileBIN)) {
		return;
	}

	// // //

	Print(_T("Writing output files...\n"));

	WriteBinary(&OutputFileBIN);

	// // //

	Print(_T("Done\n"));

	// Done
	OutputFileBIN.Close();

	// // //

	Cleanup();
}

void CCompiler::ExportPRG(LPCTSTR lpszFileName, bool EnablePAL)
{
	// Same as export to .NES but without the header

	ClearLog();

	if (m_pDocument->GetExpansionChip() != SNDCHIP_NONE) {
		Print(_T("Expansion chips not supported.\n"));
		AfxMessageBox(_T("Error: Expansion chips is currently not supported when exporting to PRG!"), 0, 0);
		return;
	}

	CFile OutputFile;
	if (!OpenFile(lpszFileName, OutputFile)) {
		return;
	}

	// Build the music data
	if (!CompileData())
		return;

	if (m_bBankSwitched) {
		// Abort if larger than 32kb
		Print(_T("Song is too big, aborted.\n"));
		AfxMessageBox(_T("Error: Song is too big to fit!"), 0, 0);
		return;
	}

	ResolveLabels();

	// // //

	// Locate driver at $8000
	m_iLoadAddress = PAGE_START;
	m_iDriverAddress = PAGE_START;
	unsigned short MusicDataAddress = m_iLoadAddress + m_iDriverSize;

	// Init is located first at the driver
	m_iInitAddress = m_iDriverAddress + 8;

	// Load driver
	std::unique_ptr<char[]> pDriverPtr(LoadDriver(m_pDriverData, m_iDriverAddress));		// // //
	char *pDriver = pDriverPtr.get();

	// Patch driver binary
	PatchVibratoTable(pDriver);

	// Write music data address
	SetDriverSongAddress(pDriver, MusicDataAddress);

	int Percent = (100 * m_iMusicDataSize) / (0x8000 - m_iDriverSize);		// // //

	Print(_T("Writing file...\n"));
	Print(_T(" * Driver size: %i bytes\n"), m_iDriverSize);
	Print(_T(" * Song data size: %i bytes (%i%%)\n"), m_iMusicDataSize, Percent);

	// Write NES data
	CChunkRenderNES Render(&OutputFile, m_iLoadAddress);
	Render.StoreDriver(pDriver, m_iDriverSize);
	Render.StoreChunks(m_vChunks);
	// // //
	Render.StoreCaller(NSF_CALLER_BIN, NSF_CALLER_SIZE);

	// Done
	OutputFile.Close();

	Cleanup();
}

void CCompiler::ExportASM(LPCTSTR lpszFileName)
{
	ClearLog();

	// Build the music data
	if (!CompileData())
		return;

	if (m_bBankSwitched) {
		// TODO: bankswitching is still unsupported when exporting to ASM
		AddBankswitching();
		ResolveLabelsBankswitched();
		EnableBankswitching();
		UpdateFrameBanks();
		UpdateSongBanks();
	}
	else {
		ResolveLabels();
		ClearSongBanks();
	}

	// // //

	Print(_T("Writing output files...\n"));

	CFile OutputFile;
	if (!OpenFile(lpszFileName, OutputFile)) {
		return;
	}

	// Write output file
	WriteAssembly(&OutputFile);

	// Done
	OutputFile.Close();

	Print(_T("Done\n"));

	Cleanup();
}

char* CCompiler::LoadDriver(const driver_t *pDriver, unsigned short Origin) const
{
	// Copy embedded driver
	unsigned char* pData = new unsigned char[pDriver->driver_size];
	memcpy(pData, pDriver->driver, pDriver->driver_size);

	// Relocate driver
	for (size_t i = 0; i < (pDriver->word_reloc_size / sizeof(int)); ++i) {
		// Words
		unsigned short value = pData[pDriver->word_reloc[i]] + (pData[pDriver->word_reloc[i] + 1] << 8);
		value += Origin;
		pData[pDriver->word_reloc[i]] = value & 0xFF;
		pData[pDriver->word_reloc[i] + 1] = value >> 8;
	}

	for (size_t i = 0; i < (pDriver->byte_reloc_size / sizeof(int)); i += 2) {	// each item is a pair
		// Low bytes
		int value = pDriver->byte_reloc_low[i + 1];
		if (value < 0)
			value = 65536 + value;
		value += Origin;
		pData[pDriver->byte_reloc_low[i]] = value & 0xFF;
		// High bytes
		value = pDriver->byte_reloc_high[i + 1];
		if (value < 0)
			value = 65536 + value;
		value += Origin;
		pData[pDriver->byte_reloc_high[i]] = value >> 8;
	}

	return (char*)pData;
}

void CCompiler::SetDriverSongAddress(char *pDriver, unsigned short Address) const
{
	// Write start address of music data
	pDriver[m_iDriverSize - 2] = Address & 0xFF;
	pDriver[m_iDriverSize - 1] = Address >> 8;
}

void CCompiler::PatchVibratoTable(char *pDriver) const
{
	// Copy the vibrato table, the stock one only works for new vibrato mode
	const CSoundGen *pSoundGen = theApp.GetSoundGenerator();

	for (int i = 0; i < 256; ++i) {
		*(pDriver + m_iVibratoTableLocation + i) = (char)pSoundGen->ReadVibratoTable(i);
	}
}

void CCompiler::CreateHeader(stNSFHeader *pHeader, int MachineType) const
{
	// Fill the NSF header
	//
	// Speed will be the same for NTSC/PAL
	//

	int Speed = m_pDocument->GetEngineSpeed();

	// If speed is default, write correct NTSC/PAL speed periods
	// else, set the same custom speed for both
	int SpeedNTSC = (Speed == 0) ? 1000000 / 60 : 1000000 / Speed;
	int SpeedPAL = (Speed == 0) ? 1000000 / 50 : 1000000 / Speed; 

	memset(pHeader, 0, 0x80);

	pHeader->Ident[0]	= 0x4E;
	pHeader->Ident[1]	= 0x45;
	pHeader->Ident[2]	= 0x53;
	pHeader->Ident[3]	= 0x4D;
	pHeader->Ident[4]	= 0x1A;

	pHeader->Version	= 0x01;
	pHeader->TotalSongs	= m_pDocument->GetTrackCount();
	pHeader->StartSong	= 1;
	pHeader->LoadAddr	= m_iLoadAddress;
	pHeader->InitAddr	= m_iInitAddress;
	pHeader->PlayAddr	= m_iInitAddress + 3;

	memset(pHeader->SongName, 0x00, 32);
	memset(pHeader->ArtistName, 0x00, 32);
	memset(pHeader->Copyright, 0x00, 32);

	strcpy_s((char*)pHeader->SongName,	 32, m_pDocument->GetSongName());
	strcpy_s((char*)pHeader->ArtistName, 32, m_pDocument->GetSongArtist());
	strcpy_s((char*)pHeader->Copyright,  32, m_pDocument->GetSongCopyright());

	pHeader->Speed_NTSC = SpeedNTSC; //0x411A; // default ntsc speed

	if (m_bBankSwitched) {
		for (int i = 0; i < 4; ++i) {
			pHeader->BankValues[i] = i;
			pHeader->BankValues[i + 4] = i + 4;
			// // //
		}
		if (LAST_BANK_FIXED) {
			// Bind last page to last bank
			pHeader->BankValues[7] = m_iLastBank;
		}
	}
	else {
		for (int i = 0; i < 8; ++i) {
			pHeader->BankValues[i] = 0;
		}
	}

	pHeader->Speed_PAL = SpeedPAL; //0x4E20; // default pal speed

	// Allow PAL or dual tunes only if no expansion chip is selected
	// Expansion chips weren't available in PAL areas
	if (m_pDocument->GetExpansionChip() == SNDCHIP_NONE) {
		switch (MachineType) {
			case 0:	// NTSC
				pHeader->Flags = 0x00;
				break;
			case 1:	// PAL
				pHeader->Flags = 0x01;
				break;
			case 2:	// Dual
				pHeader->Flags = 0x02;
				break;
		}
	}
	else {
		pHeader->Flags = 0x00;
	}

	// Expansion chip
	pHeader->SoundChip = m_pDocument->GetExpansionChip();

	pHeader->Reserved[0] = 0x00;
	pHeader->Reserved[1] = 0x00;
	pHeader->Reserved[2] = 0x00;
	pHeader->Reserved[3] = 0x00;
}

// // //

void CCompiler::UpdateFrameBanks()
{
	// Write bank numbers to frame lists (can only be used when bankswitching is used)

	int Channels = m_pDocument->GetAvailableChannels();

	for (std::vector<CChunk*>::iterator it = m_vFrameChunks.begin(); it != m_vFrameChunks.end(); ++it) {
		CChunk *pChunk = *it;
		if (pChunk->GetType() == CHUNK_FRAME) {
			// Add bank data
			for (int j = 0; j < Channels; ++j) {
				unsigned char bank = GetObjectByRef(pChunk->GetDataRefName(j))->GetBank();
				if (bank < PATTERN_SWITCH_BANK)
					bank = PATTERN_SWITCH_BANK;
				pChunk->SetupBankData(j + Channels, bank);
			}
		}
	}
}

void CCompiler::UpdateSongBanks()
{
	// Write bank numbers to song lists (can only be used when bankswitching is used)
	for (std::vector<CChunk*>::iterator it = m_vSongChunks.begin(); it != m_vSongChunks.end(); ++it) {
		CChunk *pChunk = *it;
		int bank = GetObjectByRef(pChunk->GetDataRefName(0))->GetBank();
		if (bank < PATTERN_SWITCH_BANK)
			bank = PATTERN_SWITCH_BANK;
		pChunk->SetupBankData(m_iSongBankReference, bank);
	}
}

void CCompiler::ClearSongBanks()
{
	// Clear bank data in song chunks
	for (std::vector<CChunk*>::iterator it = m_vSongChunks.begin(); it != m_vSongChunks.end(); ++it) {
		(*it)->SetupBankData(m_iSongBankReference, 0);
	}
}

void CCompiler::EnableBankswitching()
{
	// Set bankswitching flag in the song header
	ASSERT(m_pHeaderChunk != NULL);
	unsigned char flags = (unsigned char)m_pHeaderChunk->GetData(m_iHeaderFlagOffset);
	flags |= FLAG_BANKSWITCHED;
	m_pHeaderChunk->ChangeByte(m_iHeaderFlagOffset, flags);
}

void CCompiler::ResolveLabels()
{
	// Resolve label addresses, no banks since bankswitching is disabled
	CMap<CStringA, LPCSTR, int, int> labelMap;

	// Pass 1, collect labels
	CollectLabels(labelMap);

	// Pass 2
	AssignLabels(labelMap);
}

bool CCompiler::ResolveLabelsBankswitched()
{
	// Resolve label addresses and banks
	CMap<CStringA, LPCSTR, int, int> labelMap;

	// Pass 1, collect labels
	if (!CollectLabelsBankswitched(labelMap))
		return false;

	// Pass 2
	AssignLabels(labelMap);

	return true;
}

void CCompiler::CollectLabels(CMap<CStringA, LPCSTR, int, int> &labelMap) const
{
	// Collect labels and assign offsets
	int Offset = 0;
	for (std::vector<CChunk*>::const_iterator it = m_vChunks.begin(); it != m_vChunks.end(); ++it) {
		CChunk *pChunk = *it;
		labelMap[pChunk->GetLabel()] = Offset;
		Offset += pChunk->CountDataSize();
	}
}

bool CCompiler::CollectLabelsBankswitched(CMap<CStringA, LPCSTR, int, int> &labelMap)
{
	int Offset = 0;
	int Bank = PATTERN_SWITCH_BANK;

	// Instruments and stuff
	for (std::vector<CChunk*>::iterator it = m_vChunks.begin(); it != m_vChunks.end(); ++it) {
		CChunk *pChunk = *it;
		int Size = pChunk->CountDataSize();

		switch (pChunk->GetType()) {
			case CHUNK_FRAME_LIST:
			case CHUNK_FRAME:
			case CHUNK_PATTERN:
				break;
			default:
				labelMap[pChunk->GetLabel()] = Offset;
				Offset += Size;
		}
	}

	if (Offset + m_iDriverSize > 0x3000) {
		// Instrument data did not fit within the limit, display an error and abort?
		Print(_T("Error: Instrument data overflow, can't export file!\n"));
		return false;
	}

	unsigned int Track = 0;

	// The switchable area is $B000-$C000
	for (std::vector<CChunk*>::iterator it = m_vChunks.begin(); it != m_vChunks.end(); ++it) {
		CChunk *pChunk = *it;
		int Size = pChunk->CountDataSize();

		switch (pChunk->GetType()) {
			case CHUNK_FRAME_LIST:
				// Make sure the entire frame list will fit, if not then allocate a new bank
				if (Offset + m_iDriverSize + m_iTrackFrameSize[Track++] > 0x4000) {
					Offset = 0x3000 - m_iDriverSize;
					++Bank;
				}
			case CHUNK_FRAME:
				labelMap[pChunk->GetLabel()] = Offset;
				pChunk->SetBank(Bank < 4 ? ((Offset + m_iDriverSize) >> 12) : Bank);
				Offset += Size;
				break;
			case CHUNK_PATTERN:
				// Make sure entire pattern will fit
				if (Offset + m_iDriverSize + Size > 0x4000) {
					Offset = 0x3000 - m_iDriverSize;
					++Bank;
				}
				labelMap[pChunk->GetLabel()] = Offset;
				pChunk->SetBank(Bank < 4 ? ((Offset + m_iDriverSize) >> 12) : Bank);
				Offset += Size;
			default:
				break;
		}
	}

	m_iLastBank = ((Bank < 4) ? ((Offset + m_iDriverSize) >> 12) : Bank) + 1;		// // //

	return true;
}

void CCompiler::AssignLabels(CMap<CStringA, LPCSTR, int, int> &labelMap)
{
	// Pass 2: assign addresses to labels
	for (std::vector<CChunk*>::iterator it = m_vChunks.begin(); it != m_vChunks.end(); ++it) {
		(*it)->AssignLabels(labelMap);
	}
}

bool CCompiler::CompileData()
{
	// Compile music data to an object tree
	//

	const int Channels = m_pDocument->GetAvailableChannels();

	// Setup channel order list
	int Channel = 0;
	for (int i = 0; i < Channels - 1; ++i)
		m_vChanOrder.push_back(Channel++);
	// // //

	// Select driver and channel order
	switch (m_pDocument->GetExpansionChip()) {
		case SNDCHIP_NONE:
			m_pDriverData = &DRIVER_PACK_2A03;
			m_iVibratoTableLocation = VIBRATO_TABLE_LOCATION_NONE;
			Print(_T(" * No expansion chip\n"));
			break;
		// // //
		default:
			Print(_T("Error: Selected expansion chip is unsupported\n"));
			return false;
	}

	// Driver size
	m_iDriverSize = m_pDriverData->driver_size;

	// Scan and optimize song
	ScanSong();

	Print(_T("Building music data...\n"));

	// Build music data
	CreateMainHeader();
	CreateSequenceList();
	CreateInstrumentList();
	// // //
	StoreSongs();

	// Determine if bankswitching is needed
	m_bBankSwitched = false;
	m_iMusicDataSize = CountData();

	// // //
	if ((m_iMusicDataSize + m_iDriverSize) > 0x8000)
		m_bBankSwitched = true;

	// Compiling done
	// // //

#ifdef FORCE_BANKSWITCH
	m_bBankSwitched = true;
#endif /* FORCE_BANKSWITCH */

	return true;
}

void CCompiler::Cleanup()
{
	// Delete objects

	for (std::vector<CChunk*>::iterator it = m_vChunks.begin(); it != m_vChunks.end(); ++it) {
		delete *it;
	}

	m_vChunks.clear();
	m_vSequenceChunks.clear();
	m_vInstrumentChunks.clear();
	m_vSongChunks.clear();
	m_vFrameChunks.clear();
	m_vPatternChunks.clear();

	// // //
	m_pHeaderChunk = NULL;
}

void CCompiler::AddBankswitching()
{
	// Add bankswitching data

	for (std::vector<CChunk*>::iterator it = m_vChunks.begin(); it != m_vChunks.end(); ++it) {
		CChunk *pChunk = *it;
		// Frame chunks
		if (pChunk->GetType() == CHUNK_FRAME) {
			int Length = pChunk->GetLength();
			// Bank data is located at end
			for (int j = 0; j < Length; ++j) {
				pChunk->StoreBankReference(pChunk->GetDataRefName(j), 0);
			}
		}
	}

	// Frame lists sizes has changed
	const int TrackCount = m_pDocument->GetTrackCount();
	for (int i = 0; i < TrackCount; ++i) {
		m_iTrackFrameSize[i] += m_pDocument->GetChannelCount() * m_pDocument->GetFrameCount(i);
	}

	// Data size has changed
	m_iMusicDataSize = CountData();
}

void CCompiler::ScanSong()
{
	// Scan and optimize song
	//

	// Re-assign instruments
	m_iInstruments = 0;

	memset(m_iAssignedInstruments, 0, sizeof(int) * MAX_INSTRUMENTS);
	memset(m_bSequencesUsed2A03, false, sizeof(bool) * MAX_SEQUENCES * SEQ_COUNT);
	// // //

	for (int i = 0; i < MAX_INSTRUMENTS; ++i) {
		if (m_pDocument->IsInstrumentUsed(i) && IsInstrumentInPattern(i)) {
			
			// List of used instruments
			m_iAssignedInstruments[m_iInstruments++] = i;
			
			// Create a list of used sequences
			switch (m_pDocument->GetInstrumentType(i)) {
				case INST_2A03: 
					{
						CInstrumentContainer<CInstrument2A03> instContainer(m_pDocument, i);
						CInstrument2A03 *pInstrument = instContainer();
						for (int j = 0; j < CInstrument2A03::SEQUENCE_COUNT; ++j) {
							if (pInstrument->GetSeqEnable(j))
								m_bSequencesUsed2A03[pInstrument->GetSeqIndex(j)][j] = true;
						}
					}
					break;
				// // //
			}
		}
	}

	// // //
}

bool CCompiler::IsInstrumentInPattern(int index) const
{
	// Returns true if the instrument is used in a pattern

	const int TrackCount = m_pDocument->GetTrackCount();
	const int Channels = m_pDocument->GetAvailableChannels();

	// Scan patterns in entire module
	for (int i = 0; i < TrackCount; ++i) {
		int PatternLength = m_pDocument->GetPatternLength(i);
		for (int j = 0; j < Channels; ++j) {
			for (int k = 0; k < MAX_PATTERN; ++k) {
				for (int l = 0; l < PatternLength; ++l) {
					stChanNote Note;
					m_pDocument->GetDataAtPattern(i, k, j, l, &Note);
					if (Note.Instrument == index)
						return true;
				}
			}
		}
	}	

	return false;
}

void CCompiler::CreateMainHeader()
{
	// Writes the music header
	int TicksPerSec = m_pDocument->GetEngineSpeed();

	unsigned short DividerNTSC, DividerPAL;

	CChunk *pChunk = CreateChunk(CHUNK_HEADER, "");

	if (TicksPerSec == 0) {
		// Default
		DividerNTSC = CAPU::FRAME_RATE_NTSC * 60;
		DividerPAL	= CAPU::FRAME_RATE_PAL * 60;
	}
	else {
		// Custom
		DividerNTSC = TicksPerSec * 60;
		DividerPAL = TicksPerSec * 60;
	}

	unsigned char Flags = ((m_pDocument->GetVibratoStyle() == VIBRATO_OLD) ? FLAG_VIBRATO : 0);	// bankswitch flag is set later

	// Write header

	pChunk->StoreReference(LABEL_SONG_LIST);
	pChunk->StoreReference(LABEL_INSTRUMENT_LIST);
	// // //
	
	m_iHeaderFlagOffset = pChunk->GetLength();		// Save the flags offset
	pChunk->StoreByte(Flags);

	// // //

	pChunk->StoreWord(DividerNTSC);
	pChunk->StoreWord(DividerPAL);

	// // //

	m_pHeaderChunk = pChunk;
}

// Sequences

void CCompiler::CreateSequenceList()
{
	// Create sequence lists
	//

	unsigned int Size = 0, StoredCount = 0;

	for (int i = 0; i < MAX_SEQUENCES; ++i) {
		for (int j = 0; j < CInstrument2A03::SEQUENCE_COUNT; ++j) {
			CSequence* pSeq = m_pDocument->GetSequence((unsigned)i, j);

			if (m_bSequencesUsed2A03[i][j] && pSeq->GetItemCount() > 0) {
				int Index = i * SEQ_COUNT + j;
				CStringA label;
				label.Format(LABEL_SEQ_2A03, Index);
				Size += StoreSequence(pSeq, label);
				++StoredCount;
			}
		}
	}

	// // //

	Print(_T(" * Sequences used: %i (%i bytes)\n"), StoredCount, Size);
}

int CCompiler::StoreSequence(CSequence *pSeq, CStringA &label)
{
	CChunk *pChunk = CreateChunk(CHUNK_SEQUENCE, label);
	m_vSequenceChunks.push_back(pChunk);

	// Store the sequence
	int iItemCount	  = pSeq->GetItemCount();
	int iLoopPoint	  = pSeq->GetLoopPoint();
	int iReleasePoint = pSeq->GetReleasePoint();
	int iSetting	  = pSeq->GetSetting();

	if (iReleasePoint != -1)
		iReleasePoint += 1;
	else
		iReleasePoint = 0;

	if (iLoopPoint > iItemCount)
		iLoopPoint = -1;

	pChunk->StoreByte((unsigned char)iItemCount);
	pChunk->StoreByte((unsigned char)iLoopPoint);
	pChunk->StoreByte((unsigned char)iReleasePoint);
	pChunk->StoreByte((unsigned char)iSetting);

	for (int i = 0; i < iItemCount; ++i) {
		pChunk->StoreByte(pSeq->GetItem(i));
	}

	// Return size of this chunk
	return iItemCount + 4;
}

// Instruments

void CCompiler::CreateInstrumentList()
{
	/*
	 * Create the instrument list
	 *
	 * The format of instruments depends on the type
	 *
	 */

	unsigned int iTotalSize = 0;	
	CChunk *pWavetableChunk = NULL;	// FDS
	CChunk *pWavesChunk = NULL;		// N163
	int iWaveSize = 0;				// N163 waves size

	CChunk *pInstListChunk = CreateChunk(CHUNK_INSTRUMENT_LIST, LABEL_INSTRUMENT_LIST);
	
	// // //

	// Store instruments
	for (unsigned int i = 0; i < m_iInstruments; ++i) {
		// Add reference to instrument list
		CStringA label;
		label.Format(LABEL_INSTRUMENT, i);
		pInstListChunk->StoreReference(label);
		iTotalSize += 2;

		// Actual instrument
		CChunk *pChunk = CreateChunk(CHUNK_INSTRUMENT, label);
		m_vInstrumentChunks.push_back(pChunk);

		int iIndex = m_iAssignedInstruments[i];
		CInstrumentContainer<CInstrument> instContainer(m_pDocument, iIndex);
		CInstrument *pInstrument = instContainer();

		// // //

		// Returns number of bytes 
		iTotalSize += pInstrument->Compile(m_pDocument, pChunk, iIndex);
	}

	Print(_T(" * Instruments used: %i (%i bytes)\n"), m_iInstruments, iTotalSize);

	if (iWaveSize > 0)
		Print(_T(" * N163 waves size: %i bytes\n"), iWaveSize);
}

// // //

// Songs

void CCompiler::StoreSongs()
{
	/*
	 * Store patterns and frames for each song
	 * 
	 */

	const int TrackCount = m_pDocument->GetTrackCount();

	CChunk *pSongListChunk = CreateChunk(CHUNK_SONG_LIST, LABEL_SONG_LIST);

	m_iDuplicatePatterns = 0;

	// Store song info
	for (int i = 0; i < TrackCount; ++i) {
		// Add reference to song list
		CStringA label;
		label.Format(LABEL_SONG, i);
		pSongListChunk->StoreReference(label);

		// Create song
		CChunk *pChunk = CreateChunk(CHUNK_SONG, label);
		m_vSongChunks.push_back(pChunk);

		// Store reference to song
		label.Format(LABEL_SONG_FRAMES, i);
		pChunk->StoreReference(label);
		pChunk->StoreByte(m_pDocument->GetFrameCount(i));
		pChunk->StoreByte(m_pDocument->GetPatternLength(i));
		pChunk->StoreByte(m_pDocument->GetSongSpeed(i));
		pChunk->StoreByte(m_pDocument->GetSongTempo(i));
		pChunk->StoreBankReference(label, 0);
	}

	m_iSongBankReference = m_vSongChunks[0]->GetLength() - 1;	// Save bank value position (all songs are equal)

	// Store actual songs
	for (int i = 0; i < TrackCount; ++i) {
		Print(_T(" * Song %i: "), i);
		// Store frames
		CreateFrameList(i);
		// Store pattern data
		StorePatterns(i);
	}

	if (m_iDuplicatePatterns > 0)
		Print(_T(" * %i duplicated pattern(s) removed\n"), m_iDuplicatePatterns);
	
#ifdef _DEBUG
	Print(_T("Hash collisions: %i (of %i items)\r\n"), m_iHashCollisions, m_PatternMap.GetCount());
#endif
}

// Frames

void CCompiler::CreateFrameList(unsigned int Track)
{
	/*
	 * Creates a frame list
	 *
	 * The pointer list is just pointing to each item in the frame list
	 * and the frame list holds the offset addresses for the patterns for all channels
	 *
	 * ---------------------
	 *  Frame entry pointers
	 *  $XXXX (2 bytes, offset to a frame entry)
	 *  ...
	 * ---------------------
	 *
	 * ---------------------
	 *  Frame entries
	 *  $XXXX * 4 (2 * 2 bytes, each pair is an offset to the pattern)
	 * ---------------------
	 *
	 */
	
	const int FrameCount   = m_pDocument->GetFrameCount(Track);
	const int ChannelCount = m_pDocument->GetAvailableChannels();

	// Create frame list
	CStringA label;
	label.Format(LABEL_SONG_FRAMES, Track);
	CChunk *pFrameListChunk = CreateChunk(CHUNK_FRAME_LIST, label);

	unsigned int TotalSize = 0;

	// Store addresses to patterns
	for (int i = 0; i < FrameCount; ++i) {
		// Add reference to frame list
		label.Format(LABEL_SONG_FRAME, Track, i);
		pFrameListChunk->StoreReference(label);
		TotalSize += 2;

		// Store frame item
		CChunk *pChunk = CreateChunk(CHUNK_FRAME, label);
		m_vFrameChunks.push_back(pChunk);

		// Pattern pointers
		for (int j = 0; j < ChannelCount; ++j) {
			int Chan = m_vChanOrder[j];
			int Pattern = m_pDocument->GetPatternAtFrame(Track, i, Chan);
			label.Format(LABEL_PATTERN, Track, Pattern, Chan);
			pChunk->StoreReference(label);
			TotalSize += 2;
		}
	}

	m_iTrackFrameSize[Track] = TotalSize;

	Print(_T("%i frames (%i bytes), "), FrameCount, TotalSize);
}

// Patterns

void CCompiler::StorePatterns(unsigned int Track)
{
	/* 
	 * Store patterns and save references to them for the frame list
	 * 
	 */

	const int iChannels = m_pDocument->GetAvailableChannels();

	CPatternCompiler PatternCompiler(m_pDocument, m_iAssignedInstruments, m_pLogger);		// // //

	int PatternCount = 0;
	int PatternSize = 0;

	// Iterate through all patterns
	for (int i = 0; i < MAX_PATTERN; ++i) {
		for (int j = 0; j < iChannels; ++j) {
			// And store only used ones
			if (IsPatternAddressed(Track, i, j)) {

				// Compile pattern data
				PatternCompiler.CompileData(Track, i, j);

				CStringA label;
				label.Format(LABEL_PATTERN, Track, i, j);

				bool StoreNew = true;

#ifdef REMOVE_DUPLICATE_PATTERNS
				unsigned int Hash = PatternCompiler.GetHash();
				
				// Check for duplicate patterns
				CChunk *pDuplicate = m_PatternMap[Hash];

				if (pDuplicate != NULL) {
					// Hash only indicates that patterns may be equal, check exact data
					if (PatternCompiler.CompareData(pDuplicate->GetStringData(PATTERN_CHUNK_INDEX))) {
						// Duplicate was found, store a reference to existing pattern
						m_DuplicateMap[label] = pDuplicate->GetLabel();
						++m_iDuplicatePatterns;
						StoreNew = false;
					}
				}
#endif /* REMOVE_DUPLICATE_PATTERNS */

				if (StoreNew) {
					// Store new pattern
					CChunk *pChunk = CreateChunk(CHUNK_PATTERN, label);
					m_vPatternChunks.push_back(pChunk);

#ifdef REMOVE_DUPLICATE_PATTERNS
					if (m_PatternMap[Hash] != NULL)
						m_iHashCollisions++;
					m_PatternMap[Hash] = pChunk;
#endif /* REMOVE_DUPLICATE_PATTERNS */
					
					// Store pattern data as string
					pChunk->StoreString(PatternCompiler.GetData());

					PatternSize += PatternCompiler.GetDataSize();
					++PatternCount;
				}
			}
		}
	}

#ifdef REMOVE_DUPLICATE_PATTERNS
	// Update references to duplicates
	for (std::vector<CChunk*>::const_iterator it = m_vFrameChunks.begin(); it != m_vFrameChunks.end(); ++it) {
		for (int j = 0; j < (*it)->GetLength(); ++j) {
			CStringA str = m_DuplicateMap[(*it)->GetDataRefName(j)];
			if (str.GetLength() != 0) {
				// Update reference
				(*it)->UpdateDataRefName(j, str);
			}
		}
	}
#endif /* REMOVE_DUPLICATE_PATTERNS */

#ifdef LOCAL_DUPLICATE_PATTERN_REMOVAL
	// Forget patterns when one whole track is stored
	m_PatternMap.RemoveAll();
	m_DuplicateMap.RemoveAll();
#endif /* LOCAL_DUPLICATE_PATTERN_REMOVAL */

	Print(_T("%i patterns (%i bytes)\r\n"), PatternCount, PatternSize);
}

bool CCompiler::IsPatternAddressed(unsigned int Track, int Pattern, int Channel) const
{
	// Scan the frame list to see if a pattern is accessed for that frame
	const int FrameCount = m_pDocument->GetFrameCount(Track);
	
	for (int i = 0; i < FrameCount; ++i) {
		if (m_pDocument->GetPatternAtFrame(Track, i, Channel) == Pattern)
			return true;
	}

	return false;
}

// // //

void CCompiler::WriteAssembly(CFile *pFile)
{
	// Dump all chunks and samples as assembly text
	CChunkRenderText Render(pFile);
	Render.StoreChunks(m_vChunks);
	Print(_T(" * Music data size: %i bytes\n"), m_iMusicDataSize);
	// // //
}

void CCompiler::WriteBinary(CFile *pFile)
{
	// Dump all chunks as binary
	CChunkRenderBinary Render(pFile);
	Render.StoreChunks(m_vChunks);
	Print(_T(" * Music data size: %i bytes\n"), m_iMusicDataSize);
}

// // //

// Object list functions

CChunk *CCompiler::CreateChunk(chunk_type_t Type, CStringA label)
{
	CChunk *pChunk = new CChunk(Type, label);
	m_vChunks.push_back(pChunk);
	return pChunk;
}

int CCompiler::CountData() const
{
	// Only count data
	int Offset = 0;

	for (std::vector<CChunk*>::const_iterator it = m_vChunks.begin(); it != m_vChunks.end(); ++it) {
		Offset += (*it)->CountDataSize();
	}

	return Offset;
}

CChunk *CCompiler::GetObjectByRef(CStringA label) const
{
	for (std::vector<CChunk*>::const_iterator it = m_vChunks.begin(); it != m_vChunks.end(); ++it) {
		CChunk *pChunk = *it;
		if (!label.Compare(pChunk->GetLabel()))
			return pChunk;
	}

	return NULL;
}

#if 0

void CCompiler::WriteChannelMap()
{
	CChunk *pChunk = CreateChunk(CHUNK_CHANNEL_MAP, "");
	
	pChunk->StoreByte(CHANID_SQUARE1 + 1);
	pChunk->StoreByte(CHANID_SQUARE2 + 1);
	pChunk->StoreByte(CHANID_TRIANGLE + 1);
	pChunk->StoreByte(CHANID_NOISE + 1);

	if (m_pDocument->ExpansionEnabled(SNDCHIP_VRC6)) {
		pChunk->StoreByte(CHANID_VRC6_PULSE1 + 1);
		pChunk->StoreByte(CHANID_VRC6_PULSE2 + 1);
		pChunk->StoreByte(CHANID_VRC6_SAWTOOTH + 1);
	}

	if (m_pDocument->ExpansionEnabled(SNDCHIP_VRC7)) {
		pChunk->StoreByte(CHANID_VRC7_CH1 + 1);
		pChunk->StoreByte(CHANID_VRC7_CH2 + 1);
		pChunk->StoreByte(CHANID_VRC7_CH3 + 1);
		pChunk->StoreByte(CHANID_VRC7_CH4 + 1);
		pChunk->StoreByte(CHANID_VRC7_CH5 + 1);
		pChunk->StoreByte(CHANID_VRC7_CH6 + 1);
	}

	if (m_pDocument->ExpansionEnabled(SNDCHIP_FDS)) {
		pChunk->StoreByte(CHANID_FDS + 1);
	}

	if (m_pDocument->ExpansionEnabled(SNDCHIP_MMC5)) {
		pChunk->StoreByte(CHANID_MMC5_SQUARE1 + 1);
		pChunk->StoreByte(CHANID_MMC5_SQUARE2 + 1);
	}

	if (m_pDocument->ExpansionEnabled(SNDCHIP_N163)) {
		for (unsigned int i = 0; i < m_pDocument->GetNamcoChannels(); ++i) {
			pChunk->StoreByte(CHANID_N163_CHAN1 + i + 1);
		}
	}

	pChunk->StoreByte(CHANID_DPCM + 1);
}

void CCompiler::WriteChannelTypes()
{
	const int TYPE_2A03 = 0;
	const int TYPE_VRC6 = 2;
	const int TYPE_VRC7 = 4;
	const int TYPE_FDS	= 6;
	const int TYPE_MMC5 = 8;
	const int TYPE_N163 = 10;
	const int TYPE_S5B	= 12;

	CChunk *pChunk = CreateChunk(CHUNK_CHANNEL_TYPES, "");
	
	for (int i = 0; i < 4; ++i)
		pChunk->StoreByte(TYPE_2A03);

	if (m_pDocument->ExpansionEnabled(SNDCHIP_VRC6)) {
		for (int i = 0; i < 3; ++i)
			pChunk->StoreByte(TYPE_VRC6);
	}

	if (m_pDocument->ExpansionEnabled(SNDCHIP_VRC7)) {
		for (int i = 0; i < 3; ++i)
			pChunk->StoreByte(TYPE_VRC7);
	}

	if (m_pDocument->ExpansionEnabled(SNDCHIP_FDS)) {
		pChunk->StoreByte(TYPE_FDS);
	}

	if (m_pDocument->ExpansionEnabled(SNDCHIP_MMC5)) {
		for (int i = 0; i < 2; ++i)
			pChunk->StoreByte(TYPE_MMC5);
	}

	if (m_pDocument->ExpansionEnabled(SNDCHIP_N163)) {
		for (unsigned int i = 0; i < m_pDocument->GetNamcoChannels(); ++i)
			pChunk->StoreByte(TYPE_N163);
	}

	pChunk->StoreByte(TYPE_2A03);
}

#endif
