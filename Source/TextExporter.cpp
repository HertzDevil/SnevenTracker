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

#include "stdafx.h"
#include "TextExporter.h"
#include "FamiTrackerDoc.h"

#define DEBUG_OUT(...) { CString s__; s__.Format(__VA_ARGS__); OutputDebugString(s__); }

// command tokens
enum
{
	CT_COMMENTLINE,    // anything may follow
	// song info
	CT_TITLE,          // string
	CT_AUTHOR,         // string
	CT_COPYRIGHT,      // string
	CT_COMMENT,        // string (concatenates line)
	// global settings
	CT_MACHINE,        // uint (0=NTSC, 1=PAL)
	CT_FRAMERATE,      // uint (0=default)
	CT_EXPANSION,      // uint (0=none, 1=VRC6, 2=VRC7, 4=FDS, 8=MMC5, 16=N163, 32=S5B)
	CT_VIBRATO,        // uint (0=old, 1=new)
	CT_SPLIT,          // uint (32=default)
	// // //
	// macros
	CT_MACRO,          // uint (type) uint (index) int (loop) int (release) int (setting) : int_list
	// // //
	// instruments
	CT_INST2A03,       // uint (index) int int int int int string (name)
	// // //
	// track info
	CT_TRACK,          // uint (pat length) uint (speed) uint (tempo) string (name)
	CT_COLUMNS,        // : uint_list (effect columns)
	CT_ORDER,          // hex (frame) : hex_list
	// pattern data
	CT_PATTERN,        // hex (pattern)
	CT_ROW,            // row data
	// end of command list
	CT_COUNT
};

static const TCHAR* CT[CT_COUNT] =
{
	// comment
	_T("#"),
	// song info
	_T("TITLE"),
	_T("AUTHOR"),
	_T("COPYRIGHT"),
	_T("COMMENT"),
	// global settings
	_T("MACHINE"),
	_T("FRAMERATE"),
	_T("EXPANSION"),
	_T("VIBRATO"),
	_T("SPLIT"),
	// // //
	// macros
	_T("MACRO"),
	// // //
	// instruments
	_T("INST2A03"),
	// // //
	// track info
	_T("TRACK"),
	_T("COLUMNS"),
	_T("ORDER"),
	// pattern data
	_T("PATTERN"),
	_T("ROW"),
};

// =============================================================================

class Tokenizer
{
public:
	Tokenizer(const CString* text_)
		: text(text_), pos(0), line(1), linestart(0)
	{}

	~Tokenizer()
	{}

	void Reset()
	{
		pos = 0;
		line = 1;
		linestart = 0;
	}

	void ConsumeSpace()
	{
		if (!text) return;
		while (pos < text->GetLength())
		{
			TCHAR c = text->GetAt(pos);
			if (c != TCHAR(' ') &&
				c != TCHAR('\t'))
			{
				return;
			}
			++pos;
		}
	}

	void FinishLine()
	{
		if (!text) return;
		while (pos < text->GetLength() && text->GetAt(pos) != TCHAR('\n'))
		{
			++pos;
		}
		if (pos < text->GetLength()) ++pos; // skip newline
		++line;
		linestart = pos;
	}

	int GetColumn() const
	{
		return 1 + pos - linestart;
	}

	bool Finished() const
	{
		if (!text) return true;
		return pos >= text->GetLength();
	}

	CString ReadToken()
	{
		if (!text) return _T("");

		ConsumeSpace();
		CString t = _T("");

		bool inQuote = false;
		bool lastQuote = false; // for finding double-quotes
		do
		{
			if (pos >= text->GetLength()) break;
			TCHAR c = text->GetAt(pos);
			if ((c == TCHAR(' ') && !inQuote) ||
				c == TCHAR('\t') ||
				c == TCHAR('\r') ||
				c == TCHAR('\n'))
			{
				break;
			}

			// quotes suppress space ending the token
			if (c == TCHAR('\"'))
			{
				if (!inQuote && t.GetLength() == 0) // first quote begins a quoted string
				{
					inQuote = true;
				}
				else
				{
					if (lastQuote) // convert "" to "
					{
						t += c;
						lastQuote = false;
					}
					else
					{
						lastQuote = true;
					}
				}
			}
			else
			{
				lastQuote = false;
				t += c;
			}

			++pos;
		}
		while (true);

		//DEBUG_OUT("ReadToken(%d,%d): '%s'\n", line, GetColumn(), t);
		return t;
	}

	bool ReadInt(int& i, int range_min, int range_max, CString* err)
	{
		CString t = ReadToken();
		int c = GetColumn();
		if (t.GetLength() < 1)
		{
			if (err) err->Format(_T("Line %d column %d: expected integer, no token found."), line, c);
			return false;
		}

		int result = ::sscanf(t, "%d", &i);
		if(result == EOF || result == 0)
		{
			if (err) err->Format(_T("Line %d column %d: expected integer, '%s' found."), line, c, t);
			return false;
		}

		if (i < range_min || i > range_max)
		{
			if (err) err->Format(_T("Line %d column %d: expected integer in range [%d,%d], %d found."), line, c, range_min, range_max, i);
			return false;
		}

		return true;
	}

	bool ReadHex(int& i, int range_min, int range_max, CString* err)
	{
		CString t = ReadToken();
		int c = GetColumn();
		if (t.GetLength() < 1)
		{
			if (err) err->Format(_T("Line %d column %d: expected hexadecimal, no token found."), line, c);
			return false;
		}

		int result = ::sscanf(t, "%x", &i);
		if(result == EOF || result == 0)
		{
			if (err) err->Format(_T("Line %d column %d: expected hexadecimal, '%s' found."), line, c, t);
			return false;
		}

		if (i < range_min || i > range_max)
		{
			if (err) err->Format(_T("Line %d column %d: expected hexidecmal in range [%X,%X], %X found."), line, c, range_min, range_max, i);
			return false;
		}
		return true;
	}

	// note: finishes line if found
	bool ReadEOL(CString* err)
	{
		int c = GetColumn();
		ConsumeSpace();
		CString s = ReadToken();
		if (s.GetLength() > 0)
		{
			if (err) err->Format(_T("Line %d column %d: expected end of line, '%s' found."), line, c, s);
			return false;
		}

		if (Finished()) return true;

		TCHAR eol = text->GetAt(pos);
		if (eol != TCHAR('\r') && eol != TCHAR('\n'))
		{
			if (err) err->Format(_T("Line %d column %d: expected end of line, '%c' found."), line, c, eol);
			return false;
		}

		FinishLine();
		return true;
	}

	// note: finishes line if found
	bool IsEOL()
	{
		ConsumeSpace();
		if (Finished()) return true;

		TCHAR eol = text->GetAt(pos);
		if (eol == TCHAR('\r') || eol == TCHAR('\n'))
		{
			FinishLine();
			return true;
		}

		return false;
	}

	const CString* text;
	int pos;
	int line;
	int linestart;
};

// =============================================================================

static bool ImportHex(CString& sToken, int& i, int line, int column, CString& sResult)
{
	i = 0;
	for (int d=0; d < sToken.GetLength(); ++d)
	{
		const TCHAR* HEX_TEXT[16] = {
			_T("0"), _T("1"), _T("2"), _T("3"), _T("4"), _T("5"), _T("6"), _T("7"),
			_T("8"), _T("9"), _T("A"), _T("B"), _T("C"), _T("D"), _T("E"), _T("F") };

		i <<= 4;
		CString t = sToken.Mid(d,1);
		int h = 0;
		for (h=0; h < 16; ++h)
			if (0 == t.CompareNoCase(HEX_TEXT[h])) break;
		if (h >= 16)
		{
			sResult.Format(_T("Line %d column %d: hexadecimal number expected, '%s' found."), line, column, sToken);
			return false;
		}
		i += h;
	}
	return true;
}

CString ExportString(const CString& s)
{
	// puts " at beginning and end of string, replace " with ""
	CString r = _T("\"");
	for (int i=0; i < s.GetLength(); ++i)
	{
		TCHAR c = s.GetAt(i);
		if (c == TCHAR('\"'))
			r += c;
		r += c;
	}	
	r += _T("\"");
	return r;
}

// =============================================================================

static bool ImportCellText(
	CFamiTrackerDoc* pDoc,
	Tokenizer &t,
	unsigned int track,
	unsigned int pattern,
	unsigned int channel,
	unsigned int row,
	CString& sResult)
{
	stChanNote Cell;

	// empty Cell
	::memset(&Cell, 0, sizeof(Cell));
	Cell.Instrument = MAX_INSTRUMENTS;
	Cell.Vol = 0x10;

	CString sNote = t.ReadToken();
	if      (sNote == _T("...")) { Cell.Note = 0; }
	else if (sNote == _T("---")) { Cell.Note = HALT; }
	else if (sNote == _T("===")) { Cell.Note = RELEASE; }
	else
	{
		if (sNote.GetLength() != 3)
		{
			sResult.Format(_T("Line %d column %d: note column should be 3 characters wide, '%s' found."), t.line, t.GetColumn(), sNote);
			return false;
		}

		if (channel == 3) // noise
		{
			int h;
			if (!ImportHex(sNote.Left(1), h, t.line, t.GetColumn(), sResult))
				return false;
			Cell.Note = (h % 12) + 1;
			Cell.Octave = h / 12;

			// importer is very tolerant about the second and third characters
			// in a noise note, they can be anything
		}
		else
		{
			int n = 0;
			switch (sNote.GetAt(0))
			{
				case TCHAR('c'): case TCHAR('C'): n = 0; break;
				case TCHAR('d'): case TCHAR('D'): n = 2; break;
				case TCHAR('e'): case TCHAR('E'): n = 4; break;
				case TCHAR('f'): case TCHAR('F'): n = 5; break;
				case TCHAR('g'): case TCHAR('G'): n = 7; break;
				case TCHAR('a'): case TCHAR('A'): n = 9; break;
				case TCHAR('b'): case TCHAR('B'): n = 11; break;
				default:
					sResult.Format(_T("Line %d column %d: unrecognized note '%s'."), t.line, t.GetColumn(), sNote);
					return false;
			}
			switch (sNote.GetAt(1))
			{
				case TCHAR('-'): case TCHAR('.'): break;
				case TCHAR('#'): case TCHAR('+'): n += 1; break;
				case TCHAR('b'): case TCHAR('f'): n -= 1; break;
				default:
					sResult.Format(_T("Line %d column %d: unrecognized note '%s'."), t.line, t.GetColumn(), sNote);
					return false;
			}
			while (n <   0) n += 12;
			while (n >= 12) n -= 12;
			Cell.Note = n + 1;

			int o = sNote.GetAt(2) - TCHAR('0');
			if (o < 0 || o >= OCTAVE_RANGE)
			{
				sResult.Format(_T("Line %d column %d: unrecognized octave '%s'."), t.line, t.GetColumn(), sNote);
				return false;
			}
			Cell.Octave = o;
		}
	}

	CString sInst = t.ReadToken();
	if (sInst == _T("..")) { Cell.Instrument = MAX_INSTRUMENTS; }
	else
	{
		if (sInst.GetLength() != 2)
		{
			sResult.Format(_T("Line %d column %d: instrument column should be 2 characters wide, '%s' found."), t.line, t.GetColumn(), sInst);
			return false;
		}
		int h;
		if (!ImportHex(sInst, h, t.line, t.GetColumn(), sResult))
			return false;
		if (h >= MAX_INSTRUMENTS)
		{
			sResult.Format(_T("Line %d column %d: instrument '%s' is out of bounds."), t.line, t.GetColumn(), sInst);
			return false;
		}
		Cell.Instrument = h;
	}

	CString sVol  = t.ReadToken();
	const TCHAR* VOL_TEXT[17] = {
		_T("0"), _T("1"), _T("2"), _T("3"), _T("4"), _T("5"), _T("6"), _T("7"),
		_T("8"), _T("9"), _T("A"), _T("B"), _T("C"), _T("D"), _T("E"), _T("F"),
		_T(".") };
	int v = 0;
	for (; v <= 17; ++v)
		if (0 == sVol.CompareNoCase(VOL_TEXT[v])) break;
	if (v > 17)
	{
		sResult.Format(_T("Line %d column %d: unrecognized volume token '%s'."), t.line, t.GetColumn(), sVol);
		return false;
	}
	Cell.Vol = v;

	for (unsigned int e=0; e <= pDoc->GetEffColumns(track, channel); ++e)
	{
		CString sEff = t.ReadToken();
		if (sEff != _T("..."))
		{
			if (sEff.GetLength() != 3)
			{
				sResult.Format(_T("Line %d column %d: effect column should be 3 characters wide, '%s' found."), t.line, t.GetColumn(), sEff);
				return false;
			}

			int p=0;
			TCHAR pC = sEff.GetAt(0);
			if (pC >= TCHAR('a') && pC <= TCHAR('z')) pC += TCHAR('A') - TCHAR('a');
			for (;p < EF_COUNT; ++p)
				if (EFF_CHAR[p] == pC) break;
			if (p >= EF_COUNT)
			{
				sResult.Format(_T("Line %d column %d: unrecognized effect '%s'."), t.line, t.GetColumn(), sEff);
				return false;
			}
			Cell.EffNumber[e] = p+1;

			int h;
			if (!ImportHex(sEff.Right(2), h, t.line, t.GetColumn(), sResult))
				return false;
			Cell.EffParam[e] = h;
		}
	}

	pDoc->SetDataAtPattern(track,pattern,channel,row,&Cell);
	return true;
}

static const CString& ExportCellText(const stChanNote& stCell, unsigned int nEffects, bool bNoise)
{
	static CString s;
	CString tmp;

	static const char* TEXT_NOTE[HALT+1] = {
		_T("..."),
		_T("C-?"), _T("C#?"), _T("D-?"), _T("D#?"), _T("E-?"), _T("F-?"),
		_T("F#?"), _T("G-?"), _T("G#?"), _T("A-?"), _T("A#?"), _T("B-?"),
		_T("==="), _T("---") };

	s = (stCell.Note <= HALT) ? TEXT_NOTE[stCell.Note] : "...";
	if (stCell.Note >= C && stCell.Note <= B)
	{
		if (bNoise)
		{
			char nNoiseFreq = (stCell.Note - 1 + stCell.Octave * 12) & 0x0F;
			s.Format(_T("%01X-#"), nNoiseFreq);
		}
		else
		{
			s = s.Left(2);
			tmp.Format(_T("%01d"), stCell.Octave);
			s += tmp;
		}
	}

	tmp.Format(_T(" %02X"), stCell.Instrument);
	s += (stCell.Instrument == MAX_INSTRUMENTS) ? _T(" ..") : tmp;

	tmp.Format(_T(" %01X"), stCell.Vol);
	s += (stCell.Vol == 0x10) ? _T(" .") : tmp;

	for (unsigned int e=0; e < nEffects; ++e)
	{
		if (stCell.EffNumber[e] == 0)
		{
			s += _T(" ...");
		}
		else
		{
			tmp.Format(_T(" %c%02X"), EFF_CHAR[stCell.EffNumber[e]-1], stCell.EffParam[e]);
			s += tmp;
		}
	}

	return s;
}

// =============================================================================

CTextExport::CTextExport()
{
}

CTextExport::~CTextExport()
{
}

// =============================================================================

#define CHECK(x) { if (!(x)) { return sResult; } }

#define CHECK_SYMBOL(x) \
	{ \
		CString symbol_ = t.ReadToken(); \
		if (symbol_ != _T(x)) \
		{ \
			sResult.Format(_T("Line %d column %d: expected '%s', '%s' found."), t.line, t.GetColumn(), _T(x), symbol_); \
			return sResult; \
		} \
	}

#define CHECK_COLON() CHECK_SYMBOL(":")

const char* Charify(CString& s)
{
	// NOTE if Famitracker is switched to unicode, need to do a conversion here
	return s.GetString();
}

const CString& CTextExport::ImportFile(LPCTSTR FileName, CFamiTrackerDoc *pDoc)
{
	static CString sResult;
	sResult = _T("");

	// read file into "text" CString
	CString text = _T("");
	CStdioFile f;
	CFileException oFileException;
	if (!f.Open(FileName, CFile::modeRead | CFile::typeText, &oFileException))
	{
		TCHAR szError[256];
		oFileException.GetErrorMessage(szError, 256);
		
		sResult.Format(_T("Unable to open file:\n%s"), szError);
		return sResult;
	}
	CString line;
	while (f.ReadString(line))
		text += line + TCHAR('\n');
	f.Close();

	// begin a new document
	if (!pDoc->OnNewDocument())
	{
		sResult = _T("Unable to create new SnevenTracker document.");		// // //
		return sResult;
	}

	// parse the file
	Tokenizer t(&text);
	int i; // generic integer for reading
	unsigned int dpcm_index = 0;
	unsigned int dpcm_pos = 0;
	unsigned int track = 0;
	unsigned int pattern = 0;
	while (!t.Finished())
	{
		// read first token on line
		if (t.IsEOL()) continue; // blank line
		CString command = t.ReadToken();

		int c = 0;
		for (; c < CT_COUNT; ++c)
			if (0 == command.CompareNoCase(CT[c])) break;

		//DEBUG_OUT("Command read: %s\n", command);
		switch (c)
		{
			case CT_COMMENTLINE:
				t.FinishLine();
				break;
			case CT_TITLE:
				pDoc->SetSongName(Charify(t.ReadToken()));
				CHECK(t.ReadEOL(&sResult));
				break;
			case CT_AUTHOR:
				pDoc->SetSongArtist(Charify(t.ReadToken()));
				CHECK(t.ReadEOL(&sResult));
				break;
			case CT_COPYRIGHT:
				pDoc->SetSongCopyright(Charify(t.ReadToken()));
				CHECK(t.ReadEOL(&sResult));
				break;
			case CT_COMMENT:
				{
					CString sComment = pDoc->GetComment();
					if (sComment.GetLength() > 0)
						sComment = sComment + _T("\r\n");
					sComment += t.ReadToken();
					pDoc->SetComment(sComment, pDoc->ShowCommentOnOpen());
					CHECK(t.ReadEOL(&sResult));
				}
				break;
			case CT_MACHINE:
				CHECK(t.ReadInt(i,0,PAL,&sResult));
				pDoc->SetMachine(i);
				CHECK(t.ReadEOL(&sResult));
				break;
			case CT_FRAMERATE:
				CHECK(t.ReadInt(i,0,800,&sResult));
				pDoc->SetEngineSpeed(i);
				CHECK(t.ReadEOL(&sResult));
				break;
			case CT_EXPANSION:
				CHECK(t.ReadInt(i,0,255,&sResult));
				pDoc->SelectExpansionChip(i);
				CHECK(t.ReadEOL(&sResult));
				break;
			case CT_VIBRATO:
				CHECK(t.ReadInt(i,0,VIBRATO_NEW,&sResult));
				pDoc->SetVibratoStyle((vibrato_t)i);
				CHECK(t.ReadEOL(&sResult));
				break;
			case CT_SPLIT:
				CHECK(t.ReadInt(i,0,255,&sResult));
				pDoc->SetSpeedSplitPoint(i);
				CHECK(t.ReadEOL(&sResult));
				break;
			// // //
			case CT_MACRO:
				{
					// // //
					int chip = c - CT_MACRO;

					int mt;
					CHECK(t.ReadInt(mt,0,SEQ_COUNT-1,&sResult));
					CHECK(t.ReadInt(i,0,MAX_SEQUENCES-1,&sResult));
					CSequence* pSeq = pDoc->GetSequence(SNDCHIP_NONE, i, mt);		// // //

					CHECK(t.ReadInt(i,-1,MAX_SEQUENCE_ITEMS,&sResult));
					pSeq->SetLoopPoint(i);
					CHECK(t.ReadInt(i,-1,MAX_SEQUENCE_ITEMS,&sResult));
					pSeq->SetReleasePoint(i);
					CHECK(t.ReadInt(i,0,255,&sResult));
					pSeq->SetSetting(i);

					CHECK_COLON();

					int count = 0;
					while (!t.IsEOL())
					{
						CHECK(t.ReadInt(i,-128,127,&sResult));
						if (count >= MAX_SEQUENCE_ITEMS)
						{
							sResult.Format(_T("Line %d column %d: macro overflow, max size: %d."), t.line, t.GetColumn(), MAX_SEQUENCE_ITEMS);
							return sResult;
						}
						pSeq->SetItem(count, i);
						++count;
					}
					pSeq->SetItemCount(count);
				}
				break;
			// // //
			case CT_INST2A03:
				{
					CHECK(t.ReadInt(i,0,MAX_INSTRUMENTS-1,&sResult));
					CInstrument2A03* pInst = (CInstrument2A03*)pDoc->CreateInstrument(INST_2A03);
					pDoc->AddInstrument(pInst, i);
					for (int s=0; s < SEQ_COUNT; ++s)
					{
						CHECK(t.ReadInt(i,-1,MAX_SEQUENCES-1,&sResult));
						pInst->SetSeqEnable(s, (i == -1) ? 0 : 1);
						pInst->SetSeqIndex(s, (i == -1) ? 0 : i);
					}
					pInst->SetName(Charify(t.ReadToken()));
					CHECK(t.ReadEOL(&sResult));
				}
				break;
			// // //
			case CT_TRACK:
				{
					if (track != 0)
					{
						if(pDoc->AddTrack() == -1)
						{
							sResult.Format(_T("Line %d column %d: unable to add new track."), t.line, t.GetColumn());
							return sResult;
						}
					}

					CHECK(t.ReadInt(i,0,MAX_PATTERN_LENGTH,&sResult));
					pDoc->SetPatternLength(track, i);
					CHECK(t.ReadInt(i,0,MAX_TEMPO,&sResult));
					pDoc->SetSongSpeed(track, i);
					CHECK(t.ReadInt(i,0,MAX_TEMPO,&sResult));
					pDoc->SetSongTempo(track, i);
					pDoc->SetTrackTitle(track, t.ReadToken());

					CHECK(t.ReadEOL(&sResult));
					++track;
				}
				break;
			case CT_COLUMNS:
				{
					CHECK_COLON();
					for (int c=0; c < pDoc->GetChannelCount(); ++c)
					{
						CHECK(t.ReadInt(i,1,MAX_EFFECT_COLUMNS,&sResult));
						pDoc->SetEffColumns(track-1,c,i-1);
					}
					CHECK(t.ReadEOL(&sResult));
				}
				break;
			case CT_ORDER:
				{
					int ifr;
					CHECK(t.ReadHex(ifr,0,MAX_FRAMES-1,&sResult));
					if (ifr >= (int)pDoc->GetFrameCount(track-1)) // expand to accept frames
					{
						pDoc->SetFrameCount(track-1,ifr+1);
					}
					CHECK_COLON();
					for (int c=0; c < pDoc->GetChannelCount(); ++c)
					{
						CHECK(t.ReadHex(i,0,MAX_PATTERN-1,&sResult));
						pDoc->SetPatternAtFrame(track-1,ifr, c, i);
					}
					CHECK(t.ReadEOL(&sResult));
				}
				break;
			case CT_PATTERN:
				CHECK(t.ReadHex(i,0,MAX_PATTERN-1,&sResult));
				pattern = i;
				CHECK(t.ReadEOL(&sResult));
				break;
			case CT_ROW:
				{
					if (track == 0)
					{
						sResult.Format(_T("Line %d column %d: no TRACK defined, cannot add ROW data."), t.line, t.GetColumn());
						return sResult;
					}

					CHECK(t.ReadHex(i,0,MAX_PATTERN_LENGTH-1,&sResult));
					for (int c=0; c < pDoc->GetChannelCount(); ++c)
					{
    					CHECK_COLON();
						if (!ImportCellText(pDoc, t, track-1, pattern, c, i, sResult))
						{
							return sResult;
						}
					}
					CHECK(t.ReadEOL(&sResult));
				}
				break;
			case CT_COUNT:
			default:
				sResult.Format(_T("Unrecognized command at line %d: '%s'."), t.line, command);
				return sResult;
		}
	}

	return sResult;
}

// =============================================================================

const CString& CTextExport::ExportFile(LPCTSTR FileName, CFamiTrackerDoc *pDoc)
{
	static CString sResult;
	sResult = _T("");

	CStdioFile f;
	CFileException oFileException;
	if (!f.Open(FileName, CFile::modeCreate | CFile::modeWrite | CFile::typeText, &oFileException))
	{
		TCHAR szError[256];
		oFileException.GetErrorMessage(szError, 256);
		
		sResult.Format(_T("Unable to open file:\n%s"), szError);
		return sResult;
	}

	CString s;

	f.WriteString(_T("# SnevenTracker text export 0.1.0\n\n"));		// // //

	s.Format(_T("# Song information\n"
	            "%-15s %s\n"
	            "%-15s %s\n"
	            "%-15s %s\n"
	            "\n"),
	            CT[CT_TITLE],     ExportString(pDoc->GetSongName()),
	            CT[CT_AUTHOR],    ExportString(pDoc->GetSongArtist()),
	            CT[CT_COPYRIGHT], ExportString(pDoc->GetSongCopyright()));
	f.WriteString(s);

	f.WriteString(_T("# Song comment\n"));
	CString sComment = pDoc->GetComment();
	bool bCommentLines = false;
	do
	{
		int nPos = sComment.Find(TCHAR('\r'));
		bCommentLines = (nPos >= 0);
		if (bCommentLines)
		{
			CString sLine = sComment.Left(nPos);
			s.Format(_T("%s %s\n"), CT[CT_COMMENT], ExportString(sLine));
			f.WriteString(s);
			sComment = sComment.Mid(nPos+2); // +2 skips \r\n
		}
		else
		{
			s.Format(_T("%s %s\n"), CT[CT_COMMENT], ExportString(sComment));
			f.WriteString(s);
		}
	} while (bCommentLines);
	f.WriteString(_T("\n"));

	s.Format(_T("# Global settings\n"
	            "%-15s %d\n"
	            "%-15s %d\n"
	            "%-15s %d\n"
	            "%-15s %d\n"
	            "%-15s %d\n"
	            "\n"),
	            CT[CT_MACHINE],   pDoc->GetMachine(),
	            CT[CT_FRAMERATE], pDoc->GetEngineSpeed(),
	            CT[CT_EXPANSION], pDoc->GetExpansionChip(),
	            CT[CT_VIBRATO],   pDoc->GetVibratoStyle(),
	            CT[CT_SPLIT],     pDoc->GetSpeedSplitPoint() );
	f.WriteString(s);

	// // //

	f.WriteString(_T("# Macros\n"));
	for (int c=0; c<4; ++c)
	{
		int chip = SNDCHIP_NONE;		// // //

		for (int st=0; st < SEQ_COUNT; ++st)
		for (int seq=0; seq < MAX_SEQUENCES; ++seq)
		{
			CSequence* pSequence = pDoc->GetSequence(chip, seq, st);
			if (pSequence && pSequence->GetItemCount() > 0)
			{
				s.Format(_T("%-9s %3d %3d %3d %3d %3d :"),
					CT[CT_MACRO+c],
					st,
					seq,
					pSequence->GetLoopPoint(),
					pSequence->GetReleasePoint(),
					pSequence->GetSetting());
				f.WriteString(s);
				for (unsigned int i=0; i < pSequence->GetItemCount(); ++i)
				{
					s.Format(_T(" %d"), pSequence->GetItem(i));
					f.WriteString(s);
				}
				f.WriteString(_T("\n"));
			}
		}
	}
	f.WriteString(_T("\n"));

	// // //
	
	f.WriteString(_T("# Instruments\n"));
	for (unsigned int i=0; i<MAX_INSTRUMENTS; ++i)
	{
		CInstrument* pInst = pDoc->GetInstrument(i);
		if (!pInst) continue;

		switch (pInst->GetType())
		{
			default:
			case INST_NONE:
				break;
			case INST_2A03:
				{
					CInstrument2A03* pDI = (CInstrument2A03*)pInst;
					s.Format(_T("%-8s %3d   %3d %3d %3d %3d %3d %s\n"),
						CT[CT_INST2A03],
						i,
						pDI->GetSeqEnable(0) ? pDI->GetSeqIndex(0) : -1,
						pDI->GetSeqEnable(1) ? pDI->GetSeqIndex(1) : -1,
						pDI->GetSeqEnable(2) ? pDI->GetSeqIndex(2) : -1,
						pDI->GetSeqEnable(3) ? pDI->GetSeqIndex(3) : -1,
						pDI->GetSeqEnable(4) ? pDI->GetSeqIndex(4) : -1,
						ExportString(pInst->GetName()));
					f.WriteString(s);

					// // //
				}
				break;
			// // //
		}
	}
	f.WriteString(_T("\n"));

	f.WriteString(_T("# Tracks\n\n"));

	for (unsigned int t=0; t < pDoc->GetTrackCount(); ++t)
	{
		const char* zpTitle = pDoc->GetTrackTitle(t).GetString();
		if (zpTitle == NULL) zpTitle = "";

		s.Format(_T("%s %3d %3d %3d %s\n"),
			CT[CT_TRACK],
			pDoc->GetPatternLength(t),
			pDoc->GetSongSpeed(t),
			pDoc->GetSongTempo(t),
			ExportString(zpTitle));
		f.WriteString(s);

		s.Format(_T("%s :"), CT[CT_COLUMNS]);
		f.WriteString(s);
		for (int c=0; c < pDoc->GetChannelCount(); ++c)
		{
			s.Format(_T(" %d"), pDoc->GetEffColumns(t, c)+1);
			f.WriteString(s);
		}
		f.WriteString(_T("\n\n"));

		for (unsigned int o=0; o < pDoc->GetFrameCount(t); ++o)
		{
			s.Format(_T("%s %02X :"), CT[CT_ORDER], o);
			f.WriteString(s);
			for (int c=0; c < pDoc->GetChannelCount(); ++c)
			{
				s.Format(_T(" %02X"), pDoc->GetPatternAtFrame(t, o, c));
				f.WriteString(s);
			}
			f.WriteString(_T("\n"));
		}
		f.WriteString(_T("\n"));

		for (int p=0; p < MAX_PATTERN; ++p)
		{
			// detect and skip empty patterns
			bool bUsed = false;
			for (int c=0; c < pDoc->GetChannelCount(); ++c)
			{
				if (!pDoc->IsPatternEmpty(t, c, p))
				{
					bUsed = true;
					break;
				}
			}
			if (!bUsed) continue;

			s.Format(_T("%s %02X\n"), CT[CT_PATTERN], p);
			f.WriteString(s);

			for (unsigned int r=0; r < pDoc->GetPatternLength(t); ++r)
			{
				s.Format(_T("%s %02X"), CT[CT_ROW], r);
				f.WriteString(s);
				for (int c=0; c < pDoc->GetChannelCount(); ++c)
				{
					f.WriteString(_T(" : "));
					stChanNote stCell;
					pDoc->GetDataAtPattern(t,p,c,r,&stCell);
					f.WriteString(ExportCellText(stCell, pDoc->GetEffColumns(t, c)+1, c==3));
				}
				f.WriteString(_T("\n"));
			}
			f.WriteString(_T("\n"));
		}
	}

	f.WriteString(_T("# End of export\n"));
	return sResult;
}

// end of file
