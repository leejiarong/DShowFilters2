//-----------------------------------------------------------------------------
//
//	MONOGRAM AMR Filter Pack
//
//	Author : Igor Janos
//
//-----------------------------------------------------------------------------
#pragma once


class CAMRDemux;


//-----------------------------------------------------------------------------
//
//	CAMRInputPin class
//
//-----------------------------------------------------------------------------
class CAMRInputPin : public CBaseInputPin
{
public:

	// parent splitter
	CAMRDemux		*demux;
	IAsyncReader	*reader;

public:
	CAMRInputPin(TCHAR *pObjectName, CAMRDemux *pDemux, HRESULT *phr, LPCWSTR pName);
	virtual ~CAMRInputPin();

	// Get hold of IAsyncReader interface
	HRESULT CompleteConnect(IPin *pReceivePin);
	HRESULT CheckConnect(IPin *pPin);
	HRESULT BreakConnect();
	HRESULT CheckMediaType(const CMediaType *pmt);
	HRESULT SetMediaType(const CMediaType *pmt);

	// IMemInputPIn
	STDMETHODIMP Receive(IMediaSample *pSample);
	STDMETHODIMP EndOfStream(void);
	STDMETHODIMP BeginFlush();
	STDMETHODIMP EndFlush();
	STDMETHODIMP NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double rate);

	HRESULT Inactive();	
public:
	// Helpers
	CMediaType &CurrentMediaType() { return m_mt; }
	IAsyncReader *Reader() { return reader; }
};

//-----------------------------------------------------------------------------
//
//	DataPacket class
//
//-----------------------------------------------------------------------------
class DataPacket
{
public:
	int		type;

	enum {
		PACKET_TYPE_EOS	= 0,
		PACKET_TYPE_DATA = 1,
		PACKET_TYPE_NEW_SEGMENT = 2
	};

	REFERENCE_TIME	rtStart, rtStop;
	double			rate;
	bool			has_time;
	BOOL			sync_point;
	BOOL			discontinuity;
	BYTE			*buf;
	int				size;

public:
	DataPacket();
	virtual ~DataPacket();

};

typedef CArray<CMediaType> CMediaTypes;

//-----------------------------------------------------------------------------
//
//	CAMROutputPin class
//
//-----------------------------------------------------------------------------
class CAMROutputPin : 
	public CBaseOutputPin, 
	public IMediaSeeking, 
	public CAMThread
{
public:

	// parser
	CAMRDemux			*demux;
	int					stream_index;
	CMediaTypes			mt_types;

	// buffer queue
	int					buffers;
	CList<DataPacket*>	queue;
	CCritSec			lock_queue;
	CAMEvent			ev_can_read;
	CAMEvent			ev_can_write;
	CAMEvent			ev_abort;
	int					buffer_time_ms;

	// time stamps
	REFERENCE_TIME		rtStart;
	REFERENCE_TIME		rtStop;
	double				rate;
	bool				active;
	bool				discontinuity;
	bool				eos_delivered;

public:
	CAMROutputPin(TCHAR *pObjectName, CAMRDemux *pDemux, HRESULT *phr, LPCWSTR pName, int iBuffers);
	virtual ~CAMROutputPin();

	// IUNKNOWN
	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);

	// MediaType stuff
	HRESULT CheckMediaType(const CMediaType *pmt);
	HRESULT SetMediaType(const CMediaType *pmt);
	HRESULT GetMediaType(int iPosition, CMediaType *pmt);
	HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProp);
	HRESULT BreakConnect();
	HRESULT CompleteConnect(IPin *pReceivePin);

	// Activation/Deactivation
	HRESULT Active();
	HRESULT Inactive();
	HRESULT DoNewSegment(REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, double dRate);

	STDMETHODIMP Notify(IBaseFilter *pSender, Quality q);

	// packet delivery mechanism
	HRESULT DeliverPacket(CAMRPacket &packet);
	HRESULT DeliverDataPacket(DataPacket &packet);
	HRESULT DoEndOfStream();

	// delivery thread
	virtual DWORD ThreadProc();
	void FlushQueue();
	int GetDataPacket(DataPacket **packet);

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

public:
	CMediaType &CurrentMediaType() { return m_mt; }
	IMemAllocator *Allocator() { return m_pAllocator; }
	REFERENCE_TIME CurrentStart() { return rtStart; }
	REFERENCE_TIME CurrentStop() { return rtStop; }
	double CurrentRate() { return rate; }
	__int64 GetBufferTime_MS();
};

//-----------------------------------------------------------------------------
//
//	CAMRReader class
//
//-----------------------------------------------------------------------------

class CAMRReader
{
protected:
	IAsyncReader	*reader;
	__int64			position;

public:
	CAMRReader(IAsyncReader *rd);
	virtual ~CAMRReader();

	virtual int GetSize(__int64 *avail, __int64 *total);
	virtual int GetPosition(__int64 *pos, __int64 *avail);
	virtual int Seek(__int64 pos);
	virtual int Read(void *buf, int size);

	void BeginFlush() { if (reader) reader->BeginFlush(); }
	void EndFlush() { if (reader) reader->EndFlush(); }
};


