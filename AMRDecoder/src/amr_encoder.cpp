//-----------------------------------------------------------------------------
//
//	MONOGRAM AMR Filter Pack
//
//	Author : Igor Janos
//
//-----------------------------------------------------------------------------
#include "stdafx.h"

//-----------------------------------------------------------------------------
//
//	CAMREncoder
//
//-----------------------------------------------------------------------------
CAMREncoder::CAMREncoder(LPUNKNOWN pUnk, HRESULT *phr) :
	CTransformFilter(_T("MONOGRAM AMR Encoder"), pUnk, CLSID_MonogramAMREncoder)
{
	if (phr) *phr = NOERROR;

	m_pInput = new CTransformInputPin(NAME("Input"), this, phr, L"In");
	m_pOutput = new CTransformOutputPin(NAME("Output"), this, phr, L"Out");

	encoder = NULL;
	dtx = 0;
	mode = MR122;

	memset(&info, 0, sizeof(info));

	// temporary buffer
	resampled_samples = (int16*)malloc(8*1024 * sizeof(int16));
	resampled_count = 0;
	rtStart = 0;
	has_start = false;
}

CAMREncoder::~CAMREncoder()
{
	if (resampled_samples) { free(resampled_samples); resampled_samples = NULL; }
}

CUnknown *WINAPI CAMREncoder::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
	CAMREncoder *enc = new CAMREncoder(pUnk, phr);
	if (!enc) {
		if (phr) *phr = E_OUTOFMEMORY;
	}
	return enc;
}

STDMETHODIMP CAMREncoder::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	if (riid == IID_ISpecifyPropertyPages) {
		return GetInterface((ISpecifyPropertyPages*)this, ppv);
	} else
	if (riid == IID_IMonogramAMREncoder) {
		return GetInterface((IMonogramAMREncoder*)this, ppv);
	} else
		return __super::NonDelegatingQueryInterface(riid, ppv);
}

STDMETHODIMP CAMREncoder::GetPages(CAUUID *pPages)
{
    CheckPointer(pPages,E_POINTER);

    pPages->cElems = 1;
    pPages->pElems = (GUID *) CoTaskMemAlloc(pPages->cElems * sizeof(GUID));
    if (pPages->pElems == NULL) {
        return E_OUTOFMEMORY;
    }

	*(pPages->pElems) = CLSID_MonogramAMREncoderPage;
    return NOERROR;

} // GetPages

STDMETHODIMP CAMREncoder::GetInfo(AMR_ENCODER_INFO *info)
{
	if (info) {
		memcpy(info, &this->info, sizeof(AMR_ENCODER_INFO));
	}
	return NOERROR;
}

STDMETHODIMP CAMREncoder::GetMode(int *mode)
{
	if (mode) *mode = this->mode;
	return NOERROR;
}

STDMETHODIMP CAMREncoder::SetMode(int mode)
{
	this->mode = mode;
	return NOERROR;
}


HRESULT CAMREncoder::CheckInputType(const CMediaType *mtIn)
{
	// Accept PCM sound
	if (mtIn->majortype != MEDIATYPE_Audio) return E_FAIL;
	if (mtIn->subtype != MEDIASUBTYPE_PCM) return E_FAIL;
	if (mtIn->formattype != FORMAT_WaveFormatEx) return E_FAIL;

	return NOERROR;
}

HRESULT CAMREncoder::CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut)
{
	HRESULT hr = CheckInputType(mtIn);
	if (FAILED(hr)) return hr;

	// aj vystup by mal byt slusny
	if (mtOut->majortype != MEDIATYPE_Audio) return E_FAIL;
	if (mtOut->subtype != MEDIASUBTYPE_AMR) return E_FAIL;

	return NOERROR;
}

HRESULT CAMREncoder::GetMediaType(int iPosition, CMediaType *pmt)
{
	if (iPosition < 0) return E_INVALIDARG;
	if (iPosition > 0) return VFW_S_NO_MORE_ITEMS;

	// there is only one type we can produce
	pmt->majortype = MEDIATYPE_Audio;
	pmt->subtype = MEDIASUBTYPE_AMR;
	pmt->formattype = FORMAT_WaveFormatEx;
	pmt->lSampleSize = 0;

	WAVEFORMATEX *wfx = (WAVEFORMATEX*)pmt->AllocFormatBuffer(sizeof(WAVEFORMATEX));
	memset(wfx, 0, sizeof(*wfx));
	wfx->wBitsPerSample = 16;
	wfx->nChannels = 1;
	wfx->nSamplesPerSec = 8000;
	wfx->nBlockAlign = 1;
	wfx->nAvgBytesPerSec = 0;
	wfx->wFormatTag = 0;

	return NOERROR;
}

HRESULT CAMREncoder::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProp)
{
	// decide the size
	pProp->cbBuffer		= 2 * 1024;			// must be enough for one AMR frame
	pProp->cBuffers		= 1;
	pProp->cbAlign		= 1;
	pProp->cbPrefix		= 0;

	ALLOCATOR_PROPERTIES	act;
	HRESULT hr = pAlloc->SetProperties(pProp, &act);
	if (FAILED(hr)) return hr;

	if (act.cbBuffer < pProp->cbBuffer) return E_FAIL;
	return NOERROR;
}


HRESULT CAMREncoder::SetMediaType(PIN_DIRECTION direction, const CMediaType *pmt)
{
	if (direction == PINDIR_INPUT) {
		// check for input type
		WAVEFORMATEX	*wfx = (WAVEFORMATEX*)pmt->pbFormat;
		if (!wfx) return E_FAIL;

		if (wfx->wBitsPerSample != 16) return E_FAIL;
		info.samplerate	= wfx->nSamplesPerSec;
		info.channels   = wfx->nChannels;

		if (encoder) {
			CloseEncoder();
		}

		int ret = OpenEncoder();
		if (ret < 0) {
			CloseEncoder();
			return E_FAIL;
		}
	}
	return NOERROR;
}

HRESULT CAMREncoder::BreakConnect(PIN_DIRECTION dir)
{
	if (dir == PINDIR_INPUT) {
		CloseEncoder();
		info.samplerate = 0;
		info.channels   = 0;

	} else
	if (dir == PINDIR_OUTPUT) {
	}
	return __super::BreakConnect(dir);
}


HRESULT CAMREncoder::Receive(IMediaSample *pSample)
{
	// not connected ?
	if (!m_pOutput->IsConnected()) return E_FAIL;

	BYTE	*ptr;
	HRESULT	hr;
	long	size;

	// get data pointers
	hr = pSample->GetPointer(&ptr);
	size = pSample->GetActualDataLength();

	REFERENCE_TIME	rtstart, rtstop;
	hr = pSample->GetTime(&rtstart, &rtstop);
	if (hr == NOERROR) {
		if (!has_start) {
			rtStart = rtstart;
			has_start = true;
		}

		// handle jitter
		__int64		jitter = (rtStart - rtInput);
		if (jitter < 0) jitter = 0-jitter;
		if (jitter > 1500000) {
			rtInput = rtstart;
			rtStart = rtstart;
		}

		// reset timestamps on discontinuity
		if (pSample->IsDiscontinuity() == NOERROR) {
			rtInput = rtstart;
			rtStart = rtstart;
		}
	}

	//-------------------------------------------------------------------------
	//
	//	Extract Samples, mix them into one channel and resmple to 8Khz
	//
	//-------------------------------------------------------------------------

	int		time_samples = (size / (info.channels * sizeof(int16)));
	int		chnls;
	int		acc;

	// update input timestamps
	double	dur = (time_samples * 10000000.0 / (double)info.samplerate);
	REFERENCE_TIME	tdur = (REFERENCE_TIME)dur;
	rtInput += tdur;

	int16	temp_samples[4*1024];
	int16	*cur = (int16*)ptr;
	int16	*dst = temp_samples;
	int16	*in[6] = { NULL, NULL, NULL, NULL, NULL, NULL };
	int16	*out[6]= { NULL, NULL, NULL, NULL, NULL, NULL };

	static __int64	 ins = 0;
	static __int64	 outs = 0;

	chnls = info.channels;
	while (time_samples > 0) {
		int to_read = min(time_samples, 2*1024);
		time_samples -= to_read;
	
		// mixing into one channel
		dst = temp_samples;
		for (int i=0; i<to_read; i++) {
			acc = 0;
			for (int ch=0; ch<chnls; ch++) acc += (*cur++);
			*dst++ = (int16)(acc / chnls);
		}

		// resample
		out[0] = resampled_samples + resampled_count;
		in[0]  = temp_samples;

		int ret = resample.Process(out, in, to_read);
		resampled_count += ret;

		// encode these
		int16 *r = resampled_samples;
		while (resampled_count >= 160) {
			hr = Encode(r, 160);
			if (FAILED(hr)) {
				return hr;
			}

			r += 160;
			resampled_count -= 160;
		}
		if (resampled_count > 0) memcpy(resampled_samples, r, resampled_count * sizeof(int16));
	}

	return NOERROR;
}

HRESULT CAMREncoder::Encode(int16 *samples, int count)
{
	BYTE	buf[1024];
	int	bytes = Encoder_Interface_Encode(encoder, (Mode)mode, samples, buf, 0);

	IMediaSample	*sample;
	HRESULT			hr;

	hr = GetDeliveryBuffer(&sample);
	if (FAILED(hr)) return hr;

	BYTE *ptr;
	sample->GetPointer(&ptr);
	memcpy(ptr, buf, bytes);
	sample->SetActualDataLength(bytes);

	if (has_start) {
		REFERENCE_TIME	rtstart, rtstop, rtdur;

		// update output timestamps
		rtdur = (160 * 10000 / 8);
		rtstart = rtStart;
		rtStart += rtdur;
		rtstop  = rtStart;

		sample->SetTime(&rtstart, &rtstop);
		info.frames ++;
	}

	sample->SetSyncPoint(TRUE);
	sample->SetDiscontinuity(FALSE);

	hr = m_pOutput->Deliver(sample);
	sample->Release();

	return hr;
}

HRESULT CAMREncoder::GetDeliveryBuffer(IMediaSample **sample)
{
    IMediaSample *pOutSample;
    HRESULT hr = m_pOutput->GetDeliveryBuffer(&pOutSample, NULL, NULL, 0);
    *sample = pOutSample;
    if (FAILED(hr)) {
        return hr;
    }

	// has the type changed ?
	AM_MEDIA_TYPE *mt;
	if (pOutSample->GetMediaType(&mt) == NOERROR) {
		CMediaType _mt(*mt);
		SetMediaType(PINDIR_OUTPUT, &_mt);
		DeleteMediaType(mt);
	}

	return NOERROR;
}

HRESULT CAMREncoder::StartStreaming()
{
	// zresetujeme veci
	resampled_count = 0;
	info.frames = 0;
	has_start = false;
	return __super::StartStreaming();
}

HRESULT CAMREncoder::EndFlush()
{
	info.frames = 0;
	resampled_count = 0;
	has_start = false;
	return __super::EndFlush();
}

int CAMREncoder::OpenEncoder()
{
	// open a new encoder instance
	resampled_count = 0;
	encoder = Encoder_Interface_init(dtx);
	if (!encoder) return -1;

	resample.Open(1, info.samplerate, 8000);
	info.frames = 0;
	return 0;
}

int CAMREncoder::CloseEncoder()
{
	// we close the encoder instance
	if (encoder) {
		Encoder_Interface_exit(encoder);
		encoder = NULL;
	}
	resample.Close();
	resampled_count = 0;
	info.frames = 0;
	return 0;
}


