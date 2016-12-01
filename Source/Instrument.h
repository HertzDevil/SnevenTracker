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

// // //

// Instrument types
enum inst_type_t {
	INST_NONE = 0,
	INST_2A03 = 1,
	// // //
};

// External classes
class CCompiler;
class CDocumentFile;
class CSequence;
class CFamiTrackerDoc;

class CChunk;

class CRefCounter {
public:
	CRefCounter();
	virtual ~CRefCounter();
	void Retain();
	void Release();
private:
	volatile int m_iRefCounter;
};

// Instrument file load/store
class CInstrumentFile : public CFile
{
public:
	CInstrumentFile(LPCTSTR lpszFileName, UINT nOpenFlags) : CFile(lpszFileName, nOpenFlags) {};
	void WriteInt(unsigned int Value);
	void WriteChar(unsigned char Value);
	unsigned int ReadInt();
	unsigned char ReadChar();
};

// Instrument base class
class CInstrument : public CRefCounter {
public:
	CInstrument();
	virtual ~CInstrument();
	void SetName(const char *Name);
	void GetName(char *Name) const;
	const char* GetName() const;
public:
	virtual inst_type_t GetType() const = 0;										// Returns instrument type
	virtual CInstrument* CreateNew() const = 0;										// Creates a new object
	virtual CInstrument* Clone() const = 0;											// Creates a copy
	virtual void Setup() = 0;														// Setup some initial values
	virtual void Store(CDocumentFile *pDocFile) = 0;								// Saves the instrument to the module
	virtual bool Load(CDocumentFile *pDocFile) = 0;									// Loads the instrument from a module
	virtual void SaveFile(CInstrumentFile *pFile, const CFamiTrackerDoc *pDoc) = 0;	// Saves to an FTI file
	virtual bool LoadFile(CInstrumentFile *pFile, int iVersion, CFamiTrackerDoc *pDoc) = 0;	// Loads from an FTI file
	virtual int Compile(CFamiTrackerDoc *pDoc, CChunk *pChunk, int Index) = 0;		// Compiles the instrument for NSF generation
	virtual bool CanRelease() const = 0;
protected:
	void InstrumentChanged() const;
public:
	static const int INST_NAME_MAX = 128;
private:
	char m_cName[INST_NAME_MAX];
	int	 m_iType;
};

class CInstrument2A03 : public CInstrument {		// // //
public:
	CInstrument2A03();
	virtual inst_type_t	GetType() const { return INST_2A03; };
	virtual CInstrument* CreateNew() const { return new CInstrument2A03(); };
	virtual CInstrument* Clone() const;
	virtual void Setup();
	virtual void Store(CDocumentFile *pFile);
	virtual bool Load(CDocumentFile *pDocFile);
	virtual void SaveFile(CInstrumentFile *pFile, const CFamiTrackerDoc *pDoc);
	virtual bool LoadFile(CInstrumentFile *pFile, int iVersion, CFamiTrackerDoc *pDoc);
	virtual int Compile(CFamiTrackerDoc *pDoc, CChunk *pChunk, int Index);
	virtual bool CanRelease() const;

public:
	// Sequences
	int		GetSeqEnable(int Index) const;
	int		GetSeqIndex(int Index) const;
	void	SetSeqIndex(int Index, int Value);
	void	SetSeqEnable(int Index, int Value);
	// // //

public:
	static const int SEQUENCE_COUNT = 5;
	static const int SEQUENCE_TYPES[];

private:
	int		m_iSeqEnable[SEQ_COUNT];
	int		m_iSeqIndex[SEQ_COUNT];
	// // //
};

// // //

// This takes care of reference counting
// TODO replace this with boost shared_ptr
template <class T>
class CInstrumentContainer {
public:
	CInstrumentContainer(CFamiTrackerDoc *pDoc, int Index) {
		ASSERT(Index < MAX_INSTRUMENTS);
		m_pInstrument = pDoc->GetInstrument(Index);
	}
	~CInstrumentContainer() {
		if (m_pInstrument != NULL)
			m_pInstrument->Release();
	}
	T* operator()() const {
		return dynamic_cast<T*>(m_pInstrument);
	}
private:
	CInstrument *m_pInstrument;
};
