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
//	CAMRMux
//
//-----------------------------------------------------------------------------
CAMRMux::CAMRMux(LPUNKNOWN pUnk, HRESULT *phr) :
	CTransformFilter(_T("MONOGRAM AMR Mux"), pUnk, CLSID_MonogramAMRMux)
{
	if (phr) *phr = NOERROR;

	// create some output pins
	m_pInput = new CTransformInputPin(NAME("Input"), this, phr, L"In");
	m_pOutput = new CTransformOutputPin(NAME("Output"), this, phr, L"Out");

	bytes_written = 0;
}

CAMRMux::~CAMRMux()
{
}

CUnknown *WINAPI CAMRMux::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
	CAMRMux *enc = new CAMRMux(pUnk, phr);
	if (!enc) {
		if (phr) *phr = E_OUTOFMEMORY;
	}
	return enc;
}

STDMETHODIMP CAMRMux::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	if (riid == IID_ISpecifyPropertyPages) {
		return GetInterface((ISpecifyPropertyPages*)this, ppv);
	} else
		return __super::NonDelegatingQueryInterface(riid, ppv);
}

STDMETHODIMP CAMRMux::GetPages(CAUUID *pPages)
{
    CheckPointer(pPages,E_POINTER);

    pPages->cElems = 1;
    pPages->pElems = (GUID *) CoTaskMemAlloc(pPages->cElems * sizeof(GUID));
    if (pPages->pElems == NULL) return E_OUTOFMEMORY;

	*(pPages->pElems) = CLSID_MonogramAMRMuxPage;
    return NOERROR;
}

HRESULT CAMRMux::CheckInputType(const CMediaType *mtIn)
{
	if (mtIn->majortype != MEDIATYPE_Audio) return E_FAIL;
	if (mtIn->subtype != MEDIASUBTYPE_AMR) return E_FAIL;
	if (mtIn->formattype != FORMAT_WaveFormatEx) return E_FAIL;

	return NOERROR;
}

HRESULT CAMRMux::CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut)
{
	HRESULT hr = CheckInputType(mtIn);
	if (FAILED(hr)) return hr;

	// aj vystup by mal byt slusny
	if (mtOut->majortype != MEDIATYPE_Stream) return E_FAIL;
	if (mtOut->subtype != MEDIASUBTYPE_AMR &&
		mtOut->subtype != MEDIASUBTYPE_None) return E_FAIL;

	return NOERROR;
}

HRESULT CAMRMux::GetMediaType(int iPosition, CMediaType *pmt)
{
	if (iPosition < 0) return E_INVALIDARG;
	if (iPosition > 0) return VFW_S_NO_MORE_ITEMS;

	// there is only one type we can produce
	pmt->majortype	= MEDIATYPE_Stream;
	pmt->subtype	= MEDIASUBTYPE_AMR;
	pmt->formattype = FORMAT_None;

	return NOERROR;
}

HRESULT CAMRMux::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProp)
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

HRESULT CAMRMux::StartStreaming()
{
	HRESULT		hr = __super::StartStreaming();
	if (FAILED(hr)) return hr;

    CComPtr<IStream>	stream;
	IPin				*recvpin;

    if (m_pOutput->IsConnected() == FALSE) return E_FAIL;
    recvpin = m_pOutput->GetConnected();
    if (!recvpin) return E_FAIL;

    hr = ((IMemInputPin *)recvpin)->QueryInterface(IID_IStream, (void **)&stream);
    if (SUCCEEDED(hr)) {

		// AMR file magic startcode
		BYTE		magic[6] = { 0x23, 0x21, 0x41, 0x4D, 0x52, 0x0A };

        LARGE_INTEGER li;
		memset(&li, 0, sizeof(li));

        hr = stream->Seek(li, STREAM_SEEK_SET, 0);
        if (SUCCEEDED(hr)) {
            hr = stream->Write(magic, 6, NULL);
        }        

		stream = NULL;
    }

	// we start writing at position 6 (the length of magic)
	bytes_written = 6;

	return NOERROR;
}


HRESULT CAMRMux::Copy(IMediaSample *src, IMediaSample *dest)
{
    BYTE		*src_buf = NULL;
	BYTE		*dst_buf = NULL;
	long		src_len  = src->GetActualDataLength();
	long		dst_len  = src->GetSize();

	ASSERT(src_len < dst_len);

	src->GetPointer(&src_buf);
	dest->GetPointer(&dst_buf);

	memcpy(dst_buf, src_buf, src_len);
	dest->SetActualDataLength(src_len);

    REFERENCE_TIME	tstart, tend;
	if(src->GetTime(&tstart, &tend) == NOERROR) {
        dest->SetTime(&tstart, &tend);
    }

    return NOERROR;
}

HRESULT CAMRMux::Receive(IMediaSample *pSample)
{
    __int64 written_old = bytes_written;
    HRESULT hr = CTransformFilter::Receive(pSample);

    if (hr != S_OK) {
        bytes_written = written_old;
    }

    return hr;
}

HRESULT CAMRMux::Transform(IMediaSample *pIn, IMediaSample *pOut)
{
	HRESULT	hr;

	// make an identical copy
	hr = Copy(pIn, pOut);

	// and update the timestamps to be file positions
	long		len = pIn->GetActualDataLength();

	REFERENCE_TIME		tstart = bytes_written;
	REFERENCE_TIME		tstop  = tstart + len;
	bytes_written += len;

	pOut->SetTime(&tstart, &tstop);
	return NOERROR;
}
