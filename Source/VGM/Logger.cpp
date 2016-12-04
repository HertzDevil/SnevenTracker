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

#include "Logger.h"
#include "Writer/Base.h"
#include "../vgmtools/vgm_cmp.h"
#include "../stdafx.h" // _tcsdup

const char CVGMLogger::Header::IDENT[4] = {'V', 'g', 'm', ' '};
const int CVGMLogger::Header::VER_MAJ = 1;
const int CVGMLogger::Header::VER_MIN = 6;
const int CVGMLogger::Header::VER_REV = 1;

CVGMLogger::Header::Header() : m_cData(0x100)
{
	for (int i = 0; i < sizeof(IDENT); ++i)
		WriteAt<char>((size_t)CVGMLogger::HEADER_POS::Tag + i, IDENT[i]);
	WriteAt<uint32_t>(CVGMLogger::HEADER_POS::Version,
		(VER_MAJ << 8) | (VER_MIN << 4) | VER_REV);
	WriteAt<uint8_t>(CVGMLogger::HEADER_POS::VolumeModifier, 0);
	WriteAt<uint8_t>(CVGMLogger::HEADER_POS::LoopBase, 0);
	WriteAt<uint8_t>(CVGMLogger::HEADER_POS::LoopModifier, 0);
	WriteAt<uint32_t>(CVGMLogger::HEADER_POS::ExtraOffset, 0);
}

const std::vector<char> &CVGMLogger::Header::GetData() const
{
	return m_cData;
}



const uint64_t CVGMLogger::SAMPLE_RATE = 44100u;
const double CVGMLogger::DEFAULT_FREQUENCY = 60.;

CVGMLogger::CVGMLogger(const char *fname) :
	m_File(fname, std::ios::out | std::ios::binary),
	m_pFileName(_tcsdup(fname))
{
	if (!m_File)
		throw std::runtime_error {"Cannot open VGM file"};
	SetFrequency(DEFAULT_FREQUENCY);
}

CVGMLogger::~CVGMLogger()
{
	delete[] m_pFileName;
	if (m_File.is_open())
		m_File.close();
}

void CVGMLogger::DelaySamples(uint64_t count)
{
	m_fDelayTime += count;
}

void CVGMLogger::DelayTicks(uint64_t count)
{
	m_fDelayTime += count * m_fRefreshInterval;
}

void CVGMLogger::SetFrequency(double hz)
{
	if (hz <= 0)
		throw std::out_of_range {"Bad VGM tick frequency"};
	m_Header.WriteAt<uint32_t>(CVGMLogger::HEADER_POS::RefreshRate,
					 m_iRefreshRate = (uint32_t)hz);
	m_fRefreshInterval = SAMPLE_RATE / hz;
}

void CVGMLogger::RegisterWriter(const CVGMWriterBase &pWrite)
{
	m_pWriters.push_back(&pWrite);
}

void CVGMLogger::UnregisterWriter(const CVGMWriterBase &pWrite)
{
	auto it = m_pWriters.begin();
	while (it != m_pWriters.end())
		if (*it == &pWrite)
			it = m_pWriters.erase(it);
		else
			++it;
}

void CVGMLogger::InsertByte(char b)
{
	FlushDelay();
	m_cCommands.push_back(b);
}

void CVGMLogger::InsertByte(const std::vector<char> &b)
{
	FlushDelay();
	m_cCommands.insert(m_cCommands.end(), b.begin(), b.end());
}

void CVGMLogger::Loop()
{
	FlushDelay();
	m_iIntroSamples = (uint32_t)m_fCurrentTime;

	if (m_bLooped) {
		uint64_t NewTime = (uint64_t)m_fCurrentTime;
		m_cCommandsIntro.insert(m_cCommandsIntro.end(),
								m_cCommands.begin(), m_cCommands.end());
		m_cCommands.clear();
	}
	else {
		m_cCommandsIntro.swap(m_cCommands);
		m_bLooped = true;
	}
}

void CVGMLogger::SetGD3Tag(std::vector<char> tag)
{
	m_cGD3Tag = tag;
}

bool CVGMLogger::Commit()
{
	FlushDelay();

	uint64_t Size = m_Header.GetData().size() + m_cGD3Tag.size() + m_cCommandsIntro.size()
				  + m_cCommands.size() + 1;
	if (Size > 0xFFFFFFFFull)
		throw std::out_of_range {"VGM file is too large"};
	m_Header.WriteAt<uint32_t>(CVGMLogger::HEADER_POS::EofOffset,
		(uint32_t)Size - CVGMLogger::HEADER_POS::EofOffset);
	m_Header.WriteAt<uint32_t>(CVGMLogger::HEADER_POS::GD3Offset, 
		m_cGD3Tag.empty() ? 0x00000000 :
			m_Header.GetData().size() - (size_t)CVGMLogger::HEADER_POS::GD3Offset);
	m_Header.WriteAt<uint32_t>(CVGMLogger::HEADER_POS::TotalSamples, (uint32_t)m_fCurrentTime);
	m_Header.WriteAt<uint32_t>(CVGMLogger::HEADER_POS::LoopOffset,
		!m_bLooped ? 0x00000000 :
			((uint32_t)Size - m_cCommands.size() - 1 - (size_t)CVGMLogger::HEADER_POS::LoopOffset));
	m_Header.WriteAt<uint32_t>(CVGMLogger::HEADER_POS::LoopSamples,
		!m_bLooped ? 0x00000000 : ((uint32_t)m_fCurrentTime - m_iIntroSamples));
	m_Header.WriteAt<uint32_t>(CVGMLogger::HEADER_POS::DataOffset,
		m_Header.GetData().size() + m_cGD3Tag.size() - (size_t)CVGMLogger::HEADER_POS::DataOffset);
	for (const auto &x : m_pWriters)
		x->UpdateHeader(m_Header);

	bool Status = WriteToFile(m_File);
	m_File.close();
	Status = Status && CompressVGM(m_pFileName) == 0;
	return Status;
}

void CVGMLogger::FlushDelay()
{
	if (m_fDelayTime == 0.)
		return;
	uint64_t Samples = (uint64_t)(m_fCurrentTime + m_fDelayTime)
					 - (uint64_t)(m_fCurrentTime);
	m_fCurrentTime += m_fDelayTime;
	m_fDelayTime = 0;
	if (m_fCurrentTime > 0xFFFFFFFFull)
		throw std::out_of_range {"VGM file contains too many samples"};

	while (Samples) {
		uint16_t t = Samples > 0xFFFF ? 0xFFFF : (uint16_t)Samples;
		Samples -= t;
		switch (t) {
		case 2 * 44100 / 50:
			m_cCommands.push_back(0x63);
			// [[fallthrough]]
		case 44100 / 50:
			m_cCommands.push_back(0x63); break;
		case 2 * 44100 / 60:
			m_cCommands.push_back(0x62);
			// [[fallthrough]]
		case 44100 / 60:
			m_cCommands.push_back(0x62); break;
		default:
			if (t <= 16)
				m_cCommands.push_back(0x6F + (char)t);
			else if (t <= 32) {
				m_cCommands.push_back(0x7F);
				m_cCommands.push_back(0x5F + (char)t);
			}
			else {
				m_cCommands.push_back(0x61);
				m_cCommands.push_back(t & 0xFF);
				m_cCommands.push_back(t >> 8);
			}
		}
	}
}

bool CVGMLogger::WriteToFile(std::ofstream &f)
{
	auto h = m_Header.GetData();
	f.write(h.data(), h.size());
	f.write(m_cGD3Tag.data(), m_cGD3Tag.size());
	f.write(m_cCommandsIntro.data(), m_cCommandsIntro.size());
	f.write(m_cCommands.data(), m_cCommands.size());
	f.put(0x66); // end of data

	return (bool)f;
}
