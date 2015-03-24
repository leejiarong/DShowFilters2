//-----------------------------------------------------------------------------
//
//	MONOGRAM AMR Filter Pack
//
//	Author : Igor Janos
//
//-----------------------------------------------------------------------------
#pragma once

#define CMD_EXIT		0
#define CMD_STOP		1
#define CMD_RUN			2


//-----------------------------------------------------------------------------
//
//	CMPCDemux class
//
//-----------------------------------------------------------------------------
class CAMRDemux : 
	public CBaseFilter,
	public CAMThread,
	public IMediaSeeking,
	public ISpecifyPropertyPages,
	public IMonogramAMRDemux
{
public:

	CCritSec					lock_filter;
	CAMRInputPin				*input;
	CArray<CAMROutputPin*>		output;
	CArray<CAMROutputPin*>		retired;
	CAMRReader					*reader;
	CAMRFile					*file;

	CAMEvent					ev_abort;

	// times
	REFERENCE_TIME				rtCurrent;
	REFERENCE_TIME				rtStop;
	double						rate;


public:
	// constructor
	CAMRDemux(LPUNKNOWN pUnk, HRESULT *phr);
	virtual ~CAMRDemux();
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);

    // ISpecifyPropertyPages interface
    STDMETHODIMP GetPages(CAUUID *pPages);

	// CBaseFilter
	virtual int GetPinCount();
    virtual CBasePin *GetPin(int n);

	// Output pins
	HRESULT AddOutputPin(CAMROutputPin *pPin);
	virtual HRESULT RemoveOutputPins();
	CAMROutputPin *FindStream(int stream_no);

    // check that we can support this input type
    virtual HRESULT CheckInputType(const CMediaType* mtIn);
	virtual HRESULT CheckConnect(PIN_DIRECTION Dir, IPin *pPin);
	virtual HRESULT BreakConnect(PIN_DIRECTION Dir, CBasePin *pCaller);
	virtual HRESULT CompleteConnect(PIN_DIRECTION Dir, CBasePin *pCaller, IPin *pReceivePin);
	virtual HRESULT ConfigureMediaType(CAMROutputPin *pin);

	// Demuxing thread
	virtual DWORD ThreadProc();

	// IMonogramAMRDemux
	STDMETHODIMP GetInfo(AMR_DEMUX_INFO *info);

	// activate / deactivate filter
	STDMETHODIMP Pause();
	STDMETHODIMP Stop();

	// IMediaSeeking
	STDMETHODIMP GetCapabilities(DWORD* pCapabilities);
	STDMETHODIMP CheckCapabilities(DWORD* pCapabilities);
	STDMETHODIMP IsFormatSupported(const GUID* pFormat);
	STDMETHODIMP QueryPreferredFormat(GUID* pFormat);
	STDMETHODIMP GetTimeFormat(GUID* pFormat);
	STDMETHODIMP IsUsingTimeFormat(const GUID* pFormat);
	STDMETHODIMP SetTimeFormat(const GUID* pFormat);
	STDMETHODIMP GetDuration(LONGLONG* pDuration);
	STDMETHODIMP GetStopPosition(LONGLONG* pStop);
	STDMETHODIMP GetCurrentPosition(LONGLONG* pCurrent);
	STDMETHODIMP ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat);
	STDMETHODIMP SetPositions(LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags);
	STDMETHODIMP GetPositions(LONGLONG* pCurrent, LONGLONG* pStop);
	STDMETHODIMP GetAvailable(LONGLONG* pEarliest, LONGLONG* pLatest);
	STDMETHODIMP SetRate(double dRate);
	STDMETHODIMP GetRate(double* pdRate);
	STDMETHODIMP GetPreroll(LONGLONG* pllPreroll);

	STDMETHODIMP SetPositionsInternal(int iD, LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags);

	virtual HRESULT DoNewSeek();

};


