//-----------------------------------------------------------------------------
//
//	MONOGRAM AMR Filter Pack
//
//	Author : Igor Janos
//
//-----------------------------------------------------------------------------
#include "stdafx.h"


#define MAX_TEXT_LENGTH			1024
#define WM_UPDATE_VISUALS		(WM_USER + 1003)
#define ITEM(x)					(GetDlgItem(m_Dlg, x))

const	int	ModeTab[] = {
		4750,
		5150,
		5900,
		6700,
		7400,
		7950,
		10200,
		12200
};
const	int	ModeCount = sizeof(ModeTab) / sizeof(ModeTab[0]);

//-----------------------------------------------------------------------------
//
//	CAMREncoderPage class
//
//-----------------------------------------------------------------------------

CUnknown *CAMREncoderPage::CreateInstance(LPUNKNOWN lpUnk, HRESULT *phr)
{
	return new CAMREncoderPage(lpUnk, phr);
}

CAMREncoderPage::CAMREncoderPage(LPUNKNOWN lpUnk, HRESULT *phr) :
	CBasePropertyPage(NAME("AMR Encoder Page"), lpUnk, IDD_PAGE_AMR_ENC, IDS_PAGE_AMR_ENC),
	encoder(NULL)
{
}

CAMREncoderPage::~CAMREncoderPage()
{
}


HRESULT CAMREncoderPage::OnConnect(IUnknown *pUnknown)
{
	ASSERT(!encoder);
	HRESULT hr = pUnknown->QueryInterface(IID_IMonogramAMREncoder, (void**)&encoder);
	if (FAILED(hr)) return hr;

	// okej
	return NOERROR;
}

HRESULT CAMREncoderPage::OnDisconnect()
{
	if (encoder) {
		encoder->Release();
		encoder = NULL;
	}
	return NOERROR;
}

HRESULT CAMREncoderPage::OnActivate()
{
	Static_SetText(ITEM(IDC_STATIC_VERSION), MM_AMR_VERSION);
	for (int i=0; i<ModeCount; i++) {
		CString	t;
		t.Format(_T("%5.03f kbps"), ModeTab[i] / 1000.0);
		ComboBox_AddString(ITEM(IDC_COMBO_MODE), t);
	}

	int	 mode;
	if (encoder) {
		encoder->GetMode(&mode);
		ComboBox_SetCurSel(ITEM(IDC_COMBO_MODE), mode);
	}

	// update timer
	SetTimer(m_Dlg, 0, 200, NULL);
	UpdateStats();
	return NOERROR;
}

HRESULT CAMREncoderPage::OnDeactivate()
{
	KillTimer(m_Dlg, 0);
	return NOERROR;
}

HRESULT CAMREncoderPage::OnApplyChanges()
{
	int mode = ComboBox_GetCurSel(ITEM(IDC_COMBO_MODE));
	if (encoder) encoder->SetMode(mode);
	return NOERROR;
}


INT_PTR CAMREncoderPage::OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_COMMAND:
		{
			int	cmd = HIWORD(wParam);
			if (cmd == CBN_SELCHANGE) {
				SetDirty();
			}
		}
		break;
	case WM_TIMER:
		{
			// updatujeme rychlost stahovania
			UpdateStats();
		}
		break;
	}
	return __super::OnReceiveMessage(hwnd, uMsg, wParam, lParam);
}

void CAMREncoderPage::UpdateStats()
{
	if (!encoder) return ;

	AMR_ENCODER_INFO	info;
	encoder->GetInfo(&info);

	CString	t;
	t.Format(_T("%d Hz"), info.samplerate);
	Static_SetText(ITEM(IDC_STATIC_SAMPLERATE), t);
	t.Format(_T("%d"), info.channels);
	Static_SetText(ITEM(IDC_STATIC_CHANNELS), t);
	t.Format(_T("%I64d"), info.frames);
	Static_SetText(ITEM(IDC_STATIC_FRAMES), t);
}

void CAMREncoderPage::SetDirty()
{
	m_bDirty = TRUE;
    if (m_pPageSite) {
        m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
    }
}


