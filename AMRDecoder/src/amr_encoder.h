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
//	CAMREncoder
//
//-----------------------------------------------------------------------------

class CAMREncoder : 
	public CTransformFilter,
	public ISpecifyPropertyPages,
	public IMonogramAMREncoder
{
protected:

	CCritSec			lock_info;
	AMR_ENCODER_INFO	info;
	Resample			resample;
	void				*encoder;
	int					dtx;
	int					mode;

	// temporary buffer
	REFERENCE_TIME		rtStart;
	REFERENCE_TIME		rtInput;
	bool				has_start;
	int16				*resampled_samples;
	int					resampled_count;

public:

	// constructor & destructor
	CAMREncoder(LPUNKNOWN pUnk, HRESULT *phr);
	virtual ~CAMREncoder();
	static CUnknown *WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);

    // ISpecifyPropertyPages interface
    STDMETHODIMP GetPages(CAUUID *pPages);

	// Pure virtual methods
	virtual HRESULT CheckInputType(const CMediaType *mtIn);
	virtual HRESULT CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut);
	virtual HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProp);
	virtual HRESULT GetMediaType(int iPosition, CMediaType *pmt);
	virtual HRESULT SetMediaType(PIN_DIRECTION direction, const CMediaType *pmt);
	virtual HRESULT BreakConnect(PIN_DIRECTION dir);

	// iny proces transformovania samplov
	virtual HRESULT Receive(IMediaSample *pSample);
	virtual HRESULT GetDeliveryBuffer(IMediaSample **sample);
	HRESULT Encode(int16 *samples, int count);

	// filter states
    virtual HRESULT StartStreaming();
	virtual HRESULT EndFlush();

	// IMonogramAMREncoder
	STDMETHODIMP GetInfo(AMR_ENCODER_INFO *info);
	STDMETHODIMP GetMode(int *mode);
	STDMETHODIMP SetMode(int mode);

	// open/close encoder
	int OpenEncoder();
	int CloseEncoder();
};

