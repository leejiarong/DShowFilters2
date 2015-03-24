//-----------------------------------------------------------------------------
//
//	MONOGRAM AMR Filter Pack
//
//	Author : Igor Janos
//
//-----------------------------------------------------------------------------
#include "stdafx.h"


// minimalny a maximalny cas, ktory je mozne navolit v property pagese
#define ITEM(x)					GetDlgItem(m_Dlg, x)

void MakeNiceSpeed(__int64 bps, CString &v)
{
	int r=0;
	__int64	c=bps;
	LPCTSTR		rady[] = {
		_T("bps"),
		_T("kbps"),
		_T("mbps"),
		_T("gbps"),
		_T("tbps")
	};

	// spocitame rad
	while (c > 1000 && r<4) {
		r++;
		c = c / 1000;
	}

	c=bps;
	for (int i=1; i<r; i++) { c = c/1000; }
	double d=c / 1000.0;

	v.Format(_T("%5.3f %s"), (float)d, rady[r]);
}

//-----------------------------------------------------------------------------
//
//	CAMRDecoderPage class
//
//-----------------------------------------------------------------------------

CUnknown *CAMRDecoderPage::CreateInstance(LPUNKNOWN lpUnk, HRESULT *phr)
{
	return new CAMRDecoderPage(lpUnk, phr);
}

CAMRDecoderPage::CAMRDecoderPage(LPUNKNOWN lpUnk, HRESULT *phr) :
	CBasePropertyPage(NAME("AMR Decoder Page"), lpUnk, IDD_PAGE_AMR_DEC, IDS_PAGE_AMR_DEC),
	decoder(NULL)
{
}

CAMRDecoderPage::~CAMRDecoderPage()
{
}

HRESULT CAMRDecoderPage::OnConnect(IUnknown *pUnknown)
{
	// hold a pointer
	ASSERT(!decoder);

	HRESULT hr = pUnknown->QueryInterface(IID_IMonogramAMRDecoder, (void**)&decoder);
	if (FAILED(hr)) return hr;

	// okej
	return NOERROR;
}

HRESULT CAMRDecoderPage::OnDisconnect()
{
	if (decoder) {
		decoder->Release(); decoder = NULL;
	}

	return NOERROR;
}

HRESULT CAMRDecoderPage::OnActivate()
{
	Static_SetText(ITEM(IDC_STATIC_VERSION), MM_AMR_VERSION);
	UpdateData();
	SetTimer(m_Dlg, 0, 500, NULL);
	return NOERROR;
}

HRESULT CAMRDecoderPage::OnDeactivate()
{
	KillTimer(m_Dlg, 0);
	return NOERROR;
}

void CAMRDecoderPage::UpdateData()
{
	AMR_DECODER_INFO	info;
	memset(&info, 0, sizeof(info));

	if (decoder) {
		decoder->GetInfo(&info);

		// write out the info
		CString	t, t2;
		t.Format(_T("%d"), info.frame_type);
		if (info.frame_type < 0) {
			t = _T("Unknown");
		}

		if (info.indicated_bitrate == 0) {
		} else {
			__int64 bps = info.indicated_bitrate;
			MakeNiceSpeed(bps, t2);
			t = t + _T(" (") + t2 + _T(")");
		}
		Static_SetText(ITEM(IDC_STATIC_FRAMETYPE), t);

		__int64 b = info.avg_bitrate;
		MakeNiceSpeed(b, t);
		Static_SetText(ITEM(IDC_STATIC_BITRATE), t);

		t.Format(_T("%d Hz"), info.samplerate);
		Static_SetText(ITEM(IDC_STATIC_SAMPLERATE), t);
		t.Format(_T("%d"), info.channels);
		Static_SetText(ITEM(IDC_STATIC_CHANNELS), t);
		t.Format(_T("%I64d"), info.frames);
		Static_SetText(ITEM(IDC_STATIC_FRAMES), t);
	}
}


INT_PTR CAMRDecoderPage::OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{	
	switch (uMsg) {
	case WM_TIMER:	UpdateData(); break;
	}
	return __super::OnReceiveMessage(hwnd, uMsg, wParam, lParam);
}

