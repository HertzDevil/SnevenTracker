/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** SnevenTracker is (C) HertzDevil 2016
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

// // // VGM logger class

#include <cstdint>
#include <vector>
#include <fstream>
#include "Constants.h"		// // //

#include <stdexcept>
#include <cstring>		// // // memcpy

class CVGMWriterBase;

class CVGMLogger
{
public:
	struct HEADER_POS {
		enum : size_t {
			Tag				= 0x00,
			EofOffset		= 0x04,
			Version			= 0x08,
			GD3Offset		= 0x14,
			TotalSamples	= 0x18,
			LoopOffset		= 0x1C,
			LoopSamples		= 0x20,
			RefreshRate		= 0x24,
			DataOffset		= 0x34,
			VolumeModifier	= 0x7C,
			LoopBase		= 0x7E,
			LoopModifier	= 0x7F,
			ExtraOffset		= 0xBC,
		};
		HEADER_POS() = delete;
	};

public:
	class Header {
	public:
		Header();

		// returns the underlying data vector
		const std::vector<char> &GetData() const;
		// writes an object at a specified position
		template <typename T>
		void WriteAt(size_t pos, T obj)
		{
			const size_t N = sizeof(T);
			if (pos + N > m_cData.size())
				throw std::out_of_range {"Attempt to write beyond VGM header"};
			memcpy(&m_cData[pos], &obj, N);
		}
	private:
		std::vector<char> m_cData;

		static const char IDENT[4];
		static const int VER_MAJ, VER_MIN, VER_REV;
	};

public:
	explicit CVGMLogger(const char *fname);
	~CVGMLogger();

	// delays for a number of samples
	void DelaySamples(uint64_t count);
	// delays for a number of ticks
	void DelayTicks(uint64_t count);
	// sets the refresh rate of a tick
	void SetFrequency(double hz);

	// adds an external writer to the vgm, does not check for duplicates!!
	void RegisterWriter(const CVGMWriterBase &pWrite);
	// removes an external writer
	void UnregisterWriter(const CVGMWriterBase &pWrite);

	// inserts a single byte
	void InsertByte(char b);
	// inserts a number of bytes
	void InsertByte(const std::vector<char> &b);
	// inserts a loop point at the current time
	void Loop();
	// sets the GD3 tag
	void SetGD3Tag(std::vector<char> tag);

	// writes to the file stream
	// returns true if no errors occurred
	bool Commit();

private:
	void FlushDelay();
	bool WriteToFile(std::ofstream &f);

private:
	double m_fCurrentTime = 0.;
	double m_fDelayTime = 0.;
	double m_fRefreshInterval = 0.;
	uint32_t m_iRefreshRate = (uint32_t)DEFAULT_FREQUENCY;
	uint32_t m_iCurrentSamples = 0;
	uint32_t m_iIntroSamples = 0;

	std::vector<char> m_cCommands;
	std::vector<char> m_cCommandsIntro;
	std::vector<char> m_cGD3Tag;
	Header m_Header;

	std::ofstream m_File;
	const char *m_pFileName = nullptr;

	std::vector<const CVGMWriterBase*> m_pWriters;

	bool m_bLooped = false;

	static const uint64_t SAMPLE_RATE;
	static const double DEFAULT_FREQUENCY;
};
