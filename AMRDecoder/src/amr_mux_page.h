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
//	CAMRMuxPage class
//
//-----------------------------------------------------------------------------
class CAMRMuxPage : public CBasePropertyPage
{
public:
	CAMRMuxPage(LPUNKNOWN lpUnk, HRESULT *phr);
	virtual ~CAMRMuxPage();
	static CUnknown *WINAPI CreateInstance(LPUNKNOWN lpUnk, HRESULT *phr);

	// CBasePropertyPage
	HRESULT OnActivate();
};


