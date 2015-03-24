//-----------------------------------------------------------------------------
//
//	MONOGRAM AMR Filter Pack
//
//	Author : Igor Janos
//
//-----------------------------------------------------------------------------
#pragma once

const CString ACER_AMR_DECODER = L"Acer AMR Decoder";
//-----------------------------------------------------------------------------
//
//	CAMRDecoder
//
//-----------------------------------------------------------------------------

class CAMRDecoder :
	public CTransformFilter,
	public ISpecifyPropertyPages,
	public IMonogramAMRDecoder
{
public:
	CString			m_AppName;
	BOOL			m_bEnable;

	// decoder instance
	void			*decoder;

	// encoded buffer
	BYTE			*buffer;
	int				buffer_size;		// total buffer size
	int				data_size;			// count of bytes inside

	// decoded buffer
	CCritSec		lock_info;
	short			*dec_buffer;		// we keep the decoded samples here
	bool			discontinuity;
	__int64			dec_frames;
	int				frame_type;
	int				indicated_bitrate;

	// let's do some bitrate math
	double			bitrate_blur;
	double			bitrate_cnt;
	double			bitrate_decay;


	// timestamps
	REFERENCE_TIME	rtInput;
	REFERENCE_TIME	rtOutput;

public:
	// constructor & destructor
	CAMRDecoder(LPUNKNOWN pUnk, HRESULT *phr);
	virtual ~CAMRDecoder();
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
	virtual HRESULT Receive(IMediaSample *pSample);

	// buffer handling
	int DecodeFrame(BYTE *src, int len, int *bytes_processed);
	HRESULT	DecodeBuffer();
	HRESULT	AddEncodedData(BYTE *buf, int len);	

	// decoded data delivery
	HRESULT DeliverData();

	virtual HRESULT StartStreaming();
	virtual HRESULT StopStreaming();

	// IMonogramAMRDecoder
	STDMETHODIMP GetInfo(AMR_DECODER_INFO *info);

};

