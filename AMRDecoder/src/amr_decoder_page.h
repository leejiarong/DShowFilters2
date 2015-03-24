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
//	CAMRDecoderPage class
//
//-----------------------------------------------------------------------------
class CAMRDecoderPage : public CBasePropertyPage
{
public:

	IMonogramAMRDecoder		*decoder;

	void UpdateData();
public:
	CAMRDecoderPage(LPUNKNOWN lpUnk, HRESULT *phr);
	virtual ~CAMRDecoderPage();
	static CUnknown *WINAPI CreateInstance(LPUNKNOWN lpUnk, HRESULT *phr);

	// CBasePropertyPage
	HRESULT OnConnect(IUnknown *pUnknown);
	HRESULT OnDisconnect();
	HRESULT OnActivate();
	HRESULT OnDeactivate();

	// message
	INT_PTR OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};


