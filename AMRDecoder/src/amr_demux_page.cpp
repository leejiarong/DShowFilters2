//-----------------------------------------------------------------------------
//
//	MONOGRAM AMR Filter Pack
//
//	Author : Igor Janos
//
//-----------------------------------------------------------------------------
#include "stdafx.h"


// minimalny a maximalny cas, ktory je mozne navolit v property pagese
#define	MIN_TIME				1
#define MAX_TIME				10
#define ITEM(x)					GetDlgItem(m_Dlg, x)

void MakeNiceTime(int time_ms, CString &v)
{
	int		ms = time_ms%1000;	
	time_ms -= ms;
	time_ms /= 1000;

	int		h, m, s;
	h = time_ms / 3600;		time_ms -= h*3600;
	m = time_ms / 60;		time_ms -= m*60;
	s = time_ms;

	v.Format(_T("%.2d:%.2d:%.2d,%.3d"), h, m, s, ms);
}


//-----------------------------------------------------------------------------
//
//	CAMRDemuxPage class
//
//-----------------------------------------------------------------------------

CUnknown *CAMRDemuxPage::CreateInstance(LPUNKNOWN lpUnk, HRESULT *phr)
{
	return new CAMRDemuxPage(lpUnk, phr);
}

CAMRDemuxPage::CAMRDemuxPage(LPUNKNOWN lpUnk, HRESULT *phr) :
	CBasePropertyPage(NAME("AMR Demux Page"), lpUnk, IDD_PAGE_AMR_DMX, IDS_PAGE_AMR_DMX),
	demux(NULL)
{
}

CAMRDemuxPage::~CAMRDemuxPage()
{
}

HRESULT CAMRDemuxPage::OnConnect(IUnknown *pUnknown)
{
	// hold a pointer
	ASSERT(!demux);

	HRESULT hr = pUnknown->QueryInterface(IID_IMonogramAMRDemux, (void**)&demux);
	if (FAILED(hr)) return hr;

	// okej
	return NOERROR;
}

HRESULT CAMRDemuxPage::OnDisconnect()
{
	if (demux) {
		demux->Release(); demux = NULL;
	}

	return NOERROR;
}

HRESULT CAMRDemuxPage::OnActivate()
{
	Static_SetText(ITEM(IDC_STATIC_VERSION), MM_AMR_VERSION);
	UpdateData();
	SetTimer(m_Dlg, 0, 500, NULL);
	return NOERROR;
}

HRESULT CAMRDemuxPage::OnDeactivate()
{
	KillTimer(m_Dlg, 0);
	return NOERROR;
}

void CAMRDemuxPage::UpdateData()
{
	AMR_DEMUX_INFO	info;
	memset(&info, 0, sizeof(info));

	if (demux) {
		if (demux->GetInfo(&info) != NOERROR) {
			info.duration_10mhz = 0;
			info.total_frames = 0;
		}

		// write out the info
		CString	t;
		MakeNiceTime(info.duration_10mhz / 10000, t);
		Static_SetText(ITEM(IDC_STATIC_DURATION), t);
		t.Format(_T("%I64d"), info.total_frames);
		Static_SetText(ITEM(IDC_STATIC_FRAMES), t);
	}
}


INT_PTR CAMRDemuxPage::OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{	
	switch (uMsg) {
	case WM_TIMER:	UpdateData(); break;
	}
	return __super::OnReceiveMessage(hwnd, uMsg, wParam, lParam);
}

