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
//	CAMREncoderPage class
//
//-----------------------------------------------------------------------------
class CAMREncoderPage : public CBasePropertyPage
{
public:

	// enkoder
	IMonogramAMREncoder		*encoder;

	void UpdateStats();
public:
	CAMREncoderPage(LPUNKNOWN lpUnk, HRESULT *phr);
	virtual ~CAMREncoderPage();
	static CUnknown *WINAPI CreateInstance(LPUNKNOWN lpUnk, HRESULT *phr);

	// CBasePropertyPage
	HRESULT OnConnect(IUnknown *pUnknown);
	HRESULT OnDisconnect();
	HRESULT OnActivate();
	HRESULT OnDeactivate();
	HRESULT OnApplyChanges();

	// message
	INT_PTR OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void SetDirty();
};

extern const	int	ModeTab[];
extern const	int	ModeCount;
