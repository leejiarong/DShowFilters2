/* 
 *	Copyright (C) 2003-2006 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "MpaDecSettingsWnd.h"
#include "..\..\..\dsutil\dsutil.h"

//
// CMpaDecSettingsWnd
//

CMpaDecSettingsWnd::CMpaDecSettingsWnd()
{
}

bool CMpaDecSettingsWnd::OnConnect(const CInterfaceList<IUnknown, &IID_IUnknown>& pUnks)
{
	ASSERT(!m_pMDF);

	m_pMDF.Release();

	POSITION pos = pUnks.GetHeadPosition();
	while(pos && !(m_pMDF = pUnks.GetNext(pos)));
	
	if(!m_pMDF) return false;

	m_outputformat = m_pMDF->GetSampleFormat();
	m_ac3spkcfg = m_pMDF->GetSpeakerConfig(IMpaDecFilter::ac3);
	m_ac3drc = m_pMDF->GetDynamicRangeControl(IMpaDecFilter::ac3);
	m_dtsspkcfg = m_pMDF->GetSpeakerConfig(IMpaDecFilter::dts);
	m_dtsdrc = m_pMDF->GetDynamicRangeControl(IMpaDecFilter::dts);
	m_aacdownmix = !!m_pMDF->GetSpeakerConfig(IMpaDecFilter::aac);

	return true;
}

void CMpaDecSettingsWnd::OnDisconnect()
{
	m_pMDF.Release();
}

bool CMpaDecSettingsWnd::OnActivate()
{
	DWORD dwStyle = WS_VISIBLE|WS_CHILD|WS_TABSTOP;

	CRect r;

	CPoint p(10, 10);

	m_outputformat_static.Create(_T("Output sample format:"), dwStyle, CRect(p, CSize(120, m_fontheight)), this);

	p.y += m_fontheight + 5;

	m_outputformat_combo.Create(dwStyle|CBS_DROPDOWNLIST, CRect(p + CSize(10, 0), CSize(100, 200)), this, IDC_PP_COMBO1);
	m_outputformat_combo.SetItemData(m_outputformat_combo.AddString(_T("PCM 16 Bit")), SF_PCM16);
	m_outputformat_combo.SetItemData(m_outputformat_combo.AddString(_T("PCM 24 Bit")), SF_PCM24);
	m_outputformat_combo.SetItemData(m_outputformat_combo.AddString(_T("PCM 32 Bit")), SF_PCM32);
	m_outputformat_combo.SetItemData(m_outputformat_combo.AddString(_T("IEEE Float")), SF_FLOAT32);
	m_outputformat_combo.SetCurSel(0);

	for(int i = 0; i < m_outputformat_combo.GetCount(); i++)
		if((int)m_outputformat_combo.GetItemData(i) == m_outputformat)
			m_outputformat_combo.SetCurSel(i);

	p.y += 30;

	m_ac3spkcfg_static.Create(_T("AC3 Decoder Settings"), dwStyle, CRect(p, CSize(120, m_fontheight)), this);

	p.y += m_fontheight + 5;

	m_ac3spkcfg_combo.Create(dwStyle|CBS_DROPDOWNLIST, CRect(p + CSize(150, 0), CSize(100, 200)), this, IDC_PP_COMBO2);
	m_ac3spkcfg_combo.SetItemData(m_ac3spkcfg_combo.AddString(_T("Mono")), A52_MONO);
	m_ac3spkcfg_combo.SetItemData(m_ac3spkcfg_combo.AddString(_T("Dual Mono")), A52_CHANNEL);
	m_ac3spkcfg_combo.SetItemData(m_ac3spkcfg_combo.AddString(_T("Stereo")), A52_STEREO);
	m_ac3spkcfg_combo.SetItemData(m_ac3spkcfg_combo.AddString(_T("Dolby Stereo")), A52_DOLBY);
	m_ac3spkcfg_combo.SetItemData(m_ac3spkcfg_combo.AddString(_T("3 Front")), A52_3F);
	m_ac3spkcfg_combo.SetItemData(m_ac3spkcfg_combo.AddString(_T("2 Front + 1 Rear")), A52_2F1R);
	m_ac3spkcfg_combo.SetItemData(m_ac3spkcfg_combo.AddString(_T("3 Front + 1 Rear")), A52_3F1R);
	m_ac3spkcfg_combo.SetItemData(m_ac3spkcfg_combo.AddString(_T("2 Front + 2 Rear")), A52_2F2R);
	m_ac3spkcfg_combo.SetItemData(m_ac3spkcfg_combo.AddString(_T("3 Front + 2 Rear")), A52_3F2R);
	m_ac3spkcfg_combo.SetItemData(m_ac3spkcfg_combo.AddString(_T("Channel 1")), A52_CHANNEL1);
	m_ac3spkcfg_combo.SetItemData(m_ac3spkcfg_combo.AddString(_T("Channel 2")), A52_CHANNEL2);

	for(int i = 0, sel = abs(m_ac3spkcfg) & A52_CHANNEL_MASK; i < m_ac3spkcfg_combo.GetCount(); i++)
		if((int)m_ac3spkcfg_combo.GetItemData(i) == sel)
			m_ac3spkcfg_combo.SetCurSel(i);

	m_ac3spkcfg_combo.GetWindowRect(r);
	ScreenToClient(r);

	m_ac3lfe_check.Create(_T("LFE"), dwStyle|BS_AUTOCHECKBOX, CRect(CPoint(r.left, r.bottom + 3), CSize(50, m_fontheight)), this, IDC_PP_CHECK4);
	m_ac3lfe_check.SetCheck(!!(abs(m_ac3spkcfg) & A52_LFE));

	for(int i = 0, h = max(20, m_fontheight)+1; i < countof(m_ac3spkcfg_radio); i++, p.y += h)
	{
		static const TCHAR* labels[] = {_T("Decode to speakers"), _T("SPDIF")};
		m_ac3spkcfg_radio[i].Create(labels[i], dwStyle|BS_AUTORADIOBUTTON|(i == 0 ? WS_GROUP : 0), CRect(p + CPoint(10, 0), CSize(140, h)), this, IDC_PP_RADIO1+i);
	}

	CheckRadioButton(IDC_PP_RADIO1, IDC_PP_RADIO2, m_ac3spkcfg >= 0 ? IDC_PP_RADIO1 : IDC_PP_RADIO2);

	p.y += 5;

	m_ac3spkcfg_check.Create(_T("Dynamic Range Control"), dwStyle|BS_AUTOCHECKBOX, CRect(p + CPoint(10, 0), CSize(150, m_fontheight)), this, IDC_PP_CHECK1);
	m_ac3spkcfg_check.SetCheck(m_ac3drc);

	p.y += m_fontheight + 10;

	m_dtsspkcfg_static.Create(_T("DTS Decoder Settings"), dwStyle, CRect(p, CSize(120, m_fontheight)), this);

	p.y += m_fontheight + 5;

	m_dtsspkcfg_combo.Create(dwStyle|CBS_DROPDOWNLIST, CRect(p + CSize(150, 0), CSize(100, 200)), this, IDC_PP_COMBO3);
	m_dtsspkcfg_combo.SetItemData(m_dtsspkcfg_combo.AddString(_T("Mono")), DTS_MONO);
	m_dtsspkcfg_combo.SetItemData(m_dtsspkcfg_combo.AddString(_T("Dual Mono")), DTS_CHANNEL);
	m_dtsspkcfg_combo.SetItemData(m_dtsspkcfg_combo.AddString(_T("Stereo")), DTS_STEREO);
	//m_dtsspkcfg_combo.SetItemData(m_dtsspkcfg_combo.AddString(_T("Stereo ..")), DTS_STEREO_SUMDIFF);
	//m_dtsspkcfg_combo.SetItemData(m_dtsspkcfg_combo.AddString(_T("Stereo ..")), DTS_STEREO_TOTAL);
	m_dtsspkcfg_combo.SetItemData(m_dtsspkcfg_combo.AddString(_T("3 Front")), DTS_3F);
	m_dtsspkcfg_combo.SetItemData(m_dtsspkcfg_combo.AddString(_T("2 Front + 1 Rear")), DTS_2F1R);
	m_dtsspkcfg_combo.SetItemData(m_dtsspkcfg_combo.AddString(_T("3 Front + 1 Rear")), DTS_3F1R);
	m_dtsspkcfg_combo.SetItemData(m_dtsspkcfg_combo.AddString(_T("2 Front + 2 Rear")), DTS_2F2R);
	m_dtsspkcfg_combo.SetItemData(m_dtsspkcfg_combo.AddString(_T("3 Front + 2 Rear")), DTS_3F2R);

	for(int i = 0, sel = abs(m_dtsspkcfg) & DTS_CHANNEL_MASK; i < m_dtsspkcfg_combo.GetCount(); i++)
		if((int)m_dtsspkcfg_combo.GetItemData(i) == sel)
			m_dtsspkcfg_combo.SetCurSel(i);

	m_dtsspkcfg_combo.GetWindowRect(r);
	ScreenToClient(r);

	m_dtslfe_check.Create(_T("LFE"), dwStyle|BS_AUTOCHECKBOX, CRect(CPoint(r.left, r.bottom + 3), CSize(50, m_fontheight)), this, IDC_PP_CHECK5);
	m_dtslfe_check.SetCheck(!!(abs(m_dtsspkcfg) & DTS_LFE));

	for(int i = 0, h = max(20, m_fontheight)+1; i < countof(m_dtsspkcfg_radio); i++, p.y += h)
	{
		static const TCHAR* labels[] = {_T("Decode to speakers"), _T("SPDIF")};
		m_dtsspkcfg_radio[i].Create(labels[i], dwStyle|BS_AUTORADIOBUTTON|(i == 0 ? WS_GROUP : 0), CRect(p + CPoint(10, 0), CSize(140, h)), this, IDC_PP_RADIO3+i);
	}

	CheckRadioButton(IDC_PP_RADIO3, IDC_PP_RADIO4, m_dtsspkcfg >= 0 ? IDC_PP_RADIO3 : IDC_PP_RADIO4);

	p.y += 5;

	m_dtsspkcfg_check.Create(_T("Dynamic Range Control"), dwStyle|WS_DISABLED|BS_AUTOCHECKBOX, CRect(p + CPoint(10, 0), CSize(150, m_fontheight)), this, IDC_PP_CHECK2);
	m_dtsspkcfg_check.SetCheck(m_dtsdrc);

	p.y += m_fontheight + 10;

	m_aacspkcfg_static.Create(_T("AAC Decoder Settings"), dwStyle, CRect(p, CSize(120, m_fontheight)), this);

	p.y += m_fontheight + 5;

	m_aacdownmix_check.Create(_T("Downmix to stereo"), dwStyle|BS_AUTOCHECKBOX, CRect(p + CPoint(10, 0), CSize(150, m_fontheight)), this, IDC_PP_CHECK3);
	m_aacdownmix_check.SetCheck(m_aacdownmix);

	for(CWnd* pWnd = GetWindow(GW_CHILD); pWnd; pWnd = pWnd->GetNextWindow())
		pWnd->SetFont(&m_font, FALSE);

	return true;
}

void CMpaDecSettingsWnd::OnDeactivate()
{
	m_outputformat = m_outputformat_combo.GetItemData(m_outputformat_combo.GetCurSel());
	m_ac3spkcfg = m_ac3spkcfg_combo.GetItemData(m_ac3spkcfg_combo.GetCurSel());
	if(!!m_ac3lfe_check.GetCheck()) m_ac3spkcfg |= A52_LFE;
	if(IsDlgButtonChecked(IDC_PP_RADIO2)) m_ac3spkcfg = -m_ac3spkcfg;
	m_ac3drc = !!m_ac3spkcfg_check.GetCheck();
	m_dtsspkcfg = m_dtsspkcfg_combo.GetItemData(m_dtsspkcfg_combo.GetCurSel());
	if(!!m_dtslfe_check.GetCheck()) m_dtsspkcfg |= DTS_LFE;
	if(IsDlgButtonChecked(IDC_PP_RADIO4)) m_dtsspkcfg = -m_dtsspkcfg;
	m_dtsdrc = !!m_dtsspkcfg_check.GetCheck();
	m_aacdownmix = !!m_aacdownmix_check.GetCheck();
}

bool CMpaDecSettingsWnd::OnApply()
{
	OnDeactivate();

	if(m_pMDF)
	{
		m_pMDF->SetSampleFormat((SampleFormat)m_outputformat);
		m_pMDF->SetSpeakerConfig(IMpaDecFilter::ac3, m_ac3spkcfg);
		m_pMDF->SetDynamicRangeControl(IMpaDecFilter::ac3, m_ac3drc);
		m_pMDF->SetSpeakerConfig(IMpaDecFilter::dts, m_dtsspkcfg);
		m_pMDF->SetDynamicRangeControl(IMpaDecFilter::dts, m_dtsdrc);
		m_pMDF->SetSpeakerConfig(IMpaDecFilter::aac, m_aacdownmix);
	}

	return true;
}

BEGIN_MESSAGE_MAP(CMpaDecSettingsWnd, CInternalPropertyPageWnd)
END_MESSAGE_MAP()
