//-----------------------------------------------------------------------------
//
//	MONOGRAM AMR Filter Pack
//
//	Author : Igor Janos
//
//-----------------------------------------------------------------------------
#pragma once


//-----------------------------------------------------------------------------
//
//	CAMRDemuxPage class
//
//-----------------------------------------------------------------------------
class CAMRDemuxPage : public CBasePropertyPage
{
public:

	IMonogramAMRDemux		*demux;

	void UpdateData();
public:
	CAMRDemuxPage(LPUNKNOWN lpUnk, HRESULT *phr);
	virtual ~CAMRDemuxPage();
	static CUnknown *WINAPI CreateInstance(LPUNKNOWN lpUnk, HRESULT *phr);

	// CBasePropertyPage
	HRESULT OnConnect(IUnknown *pUnknown);
	HRESULT OnDisconnect();
	HRESULT OnActivate();
	HRESULT OnDeactivate();

	// message
	INT_PTR OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};


