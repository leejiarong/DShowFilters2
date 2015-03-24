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
//	CAMRMux class
//
//-----------------------------------------------------------------------------
class CAMRMux : 
	public CTransformFilter,
	public ISpecifyPropertyPages
{
public:

	// file position counter
	__int64			bytes_written;

public:
	// constructor & destructor
	CAMRMux(LPUNKNOWN pUnk, HRESULT *phr);
	virtual ~CAMRMux();
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
    virtual HRESULT StartStreaming();

	virtual HRESULT Receive(IMediaSample *pSample);
    virtual HRESULT Transform(IMediaSample *pIn, IMediaSample *pOut);
	virtual HRESULT Copy(IMediaSample *src, IMediaSample *dest);
};
