//-----------------------------------------------------------------------------
//
//	MONOGRAM AMR Filter Pack
//
//	Author : Igor Janos
//
//-----------------------------------------------------------------------------
#include "stdafx.h"

#define ITEM(x)					GetDlgItem(m_Dlg, x)

//-----------------------------------------------------------------------------
//
//	CAMRMuxPage class
//
//-----------------------------------------------------------------------------

CUnknown *CAMRMuxPage::CreateInstance(LPUNKNOWN lpUnk, HRESULT *phr)
{
	return new CAMRMuxPage(lpUnk, phr);
}

CAMRMuxPage::CAMRMuxPage(LPUNKNOWN lpUnk, HRESULT *phr) :
	CBasePropertyPage(NAME("AMR Mux Page"), lpUnk, IDD_PAGE_AMR_MUX, IDS_PAGE_AMR_MUX)
{
}

CAMRMuxPage::~CAMRMuxPage()
{
}

HRESULT CAMRMuxPage::OnActivate()
{
	Static_SetText(ITEM(IDC_STATIC_VERSION), MM_AMR_VERSION);
	return NOERROR;
}

