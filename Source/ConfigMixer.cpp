/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2010  Jonathan Liss
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
#include "FamiTracker.h"
#include "ConfigMixer.h"
#include "Settings.h"


// CConfigLevels dialog

IMPLEMENT_DYNAMIC(CConfigMixer, CPropertyPage)
CConfigMixer::CConfigMixer()
	: CPropertyPage(CConfigMixer::IDD)
{
}

CConfigMixer::~CConfigMixer()
{
}

void CConfigMixer::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);

	DDX_Slider(pDX, IDC_SLIDER_APU1, m_iLevelSN7L);
	DDX_Slider(pDX, IDC_SLIDER_APU2, m_iLevelSN7R);
	DDX_Slider(pDX, IDC_SLIDER_STEREO, m_iLevelSN7Sep);		// // //

	UpdateLevels();
}

BEGIN_MESSAGE_MAP(CConfigMixer, CPropertyPage)
	ON_WM_VSCROLL()
END_MESSAGE_MAP()


// CConfigMixer message handlers

const int CConfigMixer::LEVEL_RANGE = 12;		// +/- 12 dB range
const int CConfigMixer::LEVEL_SCALE = 10;		// 0.1 dB resolution

BOOL CConfigMixer::OnInitDialog()
{
	const CSettings *pSettings = theApp.GetSettings();

	m_iLevelSN7L = -pSettings->ChipLevels.iLevelSN7L;
	m_iLevelSN7R = -pSettings->ChipLevels.iLevelSN7R;
	m_iLevelSN7Sep = 100 - pSettings->ChipLevels.iLevelSN7Sep;		// // //

	SetupSlider(IDC_SLIDER_APU1);
	SetupSlider(IDC_SLIDER_APU2);

	CSliderCtrl *pSlider = static_cast<CSliderCtrl*>(GetDlgItem(IDC_SLIDER_STEREO));		// // //
	pSlider->SetRange(0, 100);
	pSlider->SetTicFreq(10);
	pSlider->SetPageSize(5);

	CPropertyPage::OnInitDialog();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CConfigMixer::OnApply()
{
	CSettings *pSettings = theApp.GetSettings();

	pSettings->ChipLevels.iLevelSN7L = -m_iLevelSN7L;
	pSettings->ChipLevels.iLevelSN7R = -m_iLevelSN7R;
	pSettings->ChipLevels.iLevelSN7Sep = 100 - m_iLevelSN7Sep;		// // //

	theApp.LoadSoundConfig();

	return CPropertyPage::OnApply();
}

void CConfigMixer::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	UpdateData(TRUE);
	SetModified();
	CPropertyPage::OnVScroll(nSBCode, nPos, pScrollBar);
}

void CConfigMixer::SetupSlider(int nID) const
{
	CSliderCtrl *pSlider = static_cast<CSliderCtrl*>(GetDlgItem(nID));
	pSlider->SetRange(-LEVEL_RANGE * LEVEL_SCALE, LEVEL_RANGE * LEVEL_SCALE);
	pSlider->SetTicFreq(LEVEL_SCALE * 2);
	pSlider->SetPageSize(5);
}

void CConfigMixer::UpdateLevels()
{
	UpdateLevel(IDC_LEVEL_APU1, m_iLevelSN7L);
	UpdateLevel(IDC_LEVEL_APU2, m_iLevelSN7R);
	// // //
	CString str, str2;
	str.Format(_T("%d%%"), 100 - m_iLevelSN7Sep);
	GetDlgItemText(IDC_SLIDER_STEREO, str2);
	if (str.Compare(str2) != 0)
		SetDlgItemText(IDC_LEVEL_APU3, str);
}

void CConfigMixer::UpdateLevel(int nID, int Level)
{
	CString str, str2;
	str.Format(_T("%+.1f dB"), float(-Level) / float(LEVEL_SCALE));
	GetDlgItemText(nID, str2);
	if (str.Compare(str2) != 0)
		SetDlgItemText(nID, str);
}
