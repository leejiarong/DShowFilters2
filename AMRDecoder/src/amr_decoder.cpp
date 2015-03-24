//-----------------------------------------------------------------------------
//
//	MONOGRAM AMR Filter Pack
//
//	Author : Igor Janos
//
//-----------------------------------------------------------------------------
#include "stdafx.h"

const TCHAR V_KEYPATH[]		= _T("SoftWare\\OEM\\Acer\\Video2");
const TCHAR SUP_PLAYER[]	= _T("Player%d");
const TCHAR SUP_ALL[]		= _T("*");
const int MAX_SUP_PLAYERS	= 10;

//-----------------------------------------------------------------------------
//
//	CAMRDecoder
//
//-----------------------------------------------------------------------------
CAMRDecoder::CAMRDecoder(LPUNKNOWN pUnk, HRESULT *phr) :
	CTransformFilter(ACER_AMR_DECODER, pUnk, CLSID_MonogramAMRDecoder),
	decoder(NULL)
	,m_bEnable(FALSE)
{
	if (phr) *phr = NOERROR;

	// make some buffer
	buffer_size = 16*1024;
	data_size = 0;
	buffer = (BYTE*)malloc(buffer_size);

	// alloc some space for the decoded data
	dec_buffer = (short*)malloc(1024 * sizeof(short));

	rtInput = 0;
	rtOutput = 0;

	bitrate_decay = 0.995;
	bitrate_cnt = 1;
	bitrate_blur = 0;

	STARTUPINFO si;
	GetStartupInfo(&si);
	m_AppName = CString(si.lpTitle);
	m_AppName.Replace('\\', '/');
	m_AppName = m_AppName.Mid(m_AppName.ReverseFind('/')+1);
	m_AppName.MakeLower();

	//while list
	TCHAR KEYPATH[25];
	wsprintf(KEYPATH ,_T("%s"), V_KEYPATH);
	HKEY hKey;
	ULONG nError;
	nError = RegOpenKeyExW(HKEY_LOCAL_MACHINE, KEYPATH, 0, KEY_READ, &hKey);
	if(nError == ERROR_SUCCESS)
	{
		for(int i = 0; i < MAX_SUP_PLAYERS; i++)
		{
			TCHAR szBuffer[MAX_PATH];
			DWORD dwBufferSize = sizeof(szBuffer);

			CString csValueName, csValue;
			csValueName.Format(SUP_PLAYER, i);

			nError = RegQueryValueExW(hKey, csValueName, 0, NULL, (LPBYTE)szBuffer, &dwBufferSize);
			if(nError == ERROR_SUCCESS)
			{
				csValue = szBuffer;
				csValue.MakeLower();

				if(m_AppName == csValue ||
					csValue == SUP_ALL) //remove restrictions
				{
					m_bEnable = TRUE;
					break;
				}
			}
			else
				break;
		}
	}
	if(nError != ERROR_SUCCESS)
		TRACE(_T("CAMRDecoder::CAMRDecoder reg key error [%ld]\n"), nError);

	RegCloseKey(hKey);
}

CAMRDecoder::~CAMRDecoder()
{
	if (buffer) free(buffer); buffer = NULL;
	if (dec_buffer) free(dec_buffer); dec_buffer = NULL;
}

CUnknown *WINAPI CAMRDecoder::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
	CAMRDecoder *dec = new CAMRDecoder(pUnk, phr);
	if (!dec) {
		if (phr) *phr = E_OUTOFMEMORY;
	}
	return dec;
}

STDMETHODIMP CAMRDecoder::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	if (riid == IID_ISpecifyPropertyPages) {
		return GetInterface((ISpecifyPropertyPages*)this, ppv);
	} else
	if (riid == IID_IMonogramAMRDecoder) {
		return GetInterface((IMonogramAMRDecoder*)this, ppv);
	} else
		return __super::NonDelegatingQueryInterface(riid, ppv);
}

STDMETHODIMP CAMRDecoder::GetPages(CAUUID *pPages)
{
    CheckPointer(pPages,E_POINTER);

    pPages->cElems = 1;
    pPages->pElems = (GUID *) CoTaskMemAlloc(pPages->cElems * sizeof(GUID));
    if (pPages->pElems == NULL) {
        return E_OUTOFMEMORY;
    }

	*(pPages->pElems) = CLSID_MonogramAMRDecoderPage;
    return NOERROR;

}

HRESULT CAMRDecoder::CheckInputType(const CMediaType *mtIn)
{
	if(!m_bEnable)
		return E_FAIL;

	if (mtIn->majortype != MEDIATYPE_Audio) return E_FAIL;
	if (mtIn->subtype != MEDIASUBTYPE_AMR) return E_FAIL;
	if (mtIn->formattype != FORMAT_WaveFormatEx) return E_FAIL;

	return NOERROR;
}

HRESULT CAMRDecoder::CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut)
{
	HRESULT hr = CheckInputType(mtIn);
	if (FAILED(hr)) return hr;

	if (mtOut->majortype != MEDIATYPE_Audio) return E_FAIL;
	if (mtOut->subtype != MEDIASUBTYPE_PCM) return E_FAIL;

	return NOERROR;
}

HRESULT CAMRDecoder::GetMediaType(int iPosition, CMediaType *pmt)
{
	if (iPosition < 0) return E_INVALIDARG;
	if (iPosition > 0) return VFW_S_NO_MORE_ITEMS;

	// input must be connected
	if (m_pInput->IsConnected() == FALSE) return E_FAIL;

	pmt->SetType(&MEDIATYPE_Audio);
	pmt->SetSubtype(&MEDIASUBTYPE_PCM);
	pmt->SetFormatType(&FORMAT_WaveFormatEx);
	pmt->SetTemporalCompression(FALSE);

	WAVEFORMATEX			*pwf;

	// 8KHz, mono always
	pwf						= (WAVEFORMATEX *) pmt->AllocFormatBuffer(sizeof(WAVEFORMATEX));
	pwf->wFormatTag			= WAVE_FORMAT_PCM;
	pwf->cbSize				= 0;
	pwf->nChannels			= 1;
	pwf->wBitsPerSample		= 16;
	pwf->nSamplesPerSec		= 8000;
	pwf->nBlockAlign		= pwf->nChannels * (pwf->wBitsPerSample>>3);
	pwf->nAvgBytesPerSec	= pwf->nChannels * (pwf->wBitsPerSample>>3) * pwf->nSamplesPerSec;	

	return NOERROR;
}

HRESULT CAMRDecoder::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProp)
{
	// should be way enough
	pProp->cbBuffer		= 4 * 1024;
	pProp->cBuffers		= 3;
	pProp->cbAlign		= 2;		
	pProp->cbPrefix		= 0;

	ALLOCATOR_PROPERTIES	act;
	HRESULT hr = pAlloc->SetProperties(pProp, &act);
	if (FAILED(hr)) return hr;
	if (act.cbBuffer < pProp->cbBuffer) return E_FAIL;
	return NOERROR;
}

HRESULT CAMRDecoder::Receive(IMediaSample *pSample)
{
	// should not be happening...
	if (!decoder) return E_FAIL;

	//-------------------------------------------------------------------------
	// Handle timestamps and discontinuities
	//-------------------------------------------------------------------------

	REFERENCE_TIME	rtStart, rtStop;
	HRESULT hr = pSample->GetTime(&rtStart, &rtStop);
	if (hr == NOERROR) {

		// handle jitter
		int64	jitter = rtStart - rtInput;
		if (jitter < 0) jitter = -jitter;

		// if the jitter is larger than 150 milliseconds do a discontinuity
		if (jitter > 1500000) {
			data_size = 0;
			discontinuity = true;
			rtInput = rtStart;
			rtOutput = rtStart;
		}
	}

	// flush the buffer with discontinuity
	if (pSample->IsDiscontinuity()) {
		if (hr == NOERROR) {
			data_size = 0;
			discontinuity = true;
			rtInput = rtStart;
			rtOutput = rtStart;
		}
	}

	//-------------------------------------------------------------------------
	// Process the buffer
	//-------------------------------------------------------------------------

	BYTE		*data;
	long		size;

	pSample->GetPointer(&data);
	size = pSample->GetActualDataLength();

	int		left	= size;
	BYTE	*start	= data;

	hr = NOERROR;
	while (left > 0) {

		int		to_process = min(left, buffer_size - data_size);
		if (to_process > 0) {

			// add as much as possible
			AddEncodedData(start, to_process);
			start += to_process;
			left -= to_process;

			// and deal with it
			hr = DecodeBuffer();
			if (FAILED(hr)) break;
		}
	}

	return hr;
}

HRESULT	CAMRDecoder::DecodeBuffer()
{
	BYTE	*start = buffer;
	int		left   = data_size;
	int		proc   = 0;
	int		ret	   = 0;
	HRESULT	hr	   = NOERROR;

	do {
		ret = DecodeFrame(start, left, &proc);
		if (ret == 0) {

			// adjust the buffer
			start += proc;
			left  -= proc;

			// adjust decoding timestamps
			REFERENCE_TIME	rtdur = (160 * 10000 / 8);
			rtInput += rtdur;

			// deliver decoded samples
			hr = DeliverData();
			if (FAILED(hr)) break;
		}
	} while (ret == 0);

	// discard the used buffer
	if (left == 0) {
		data_size = 0;
	} else {
		memcpy(buffer, start, left);
		data_size = left;
	}

	return hr;
}

HRESULT	CAMRDecoder::AddEncodedData(BYTE *buf, int len)
{
	// append the new block of data

	memcpy(buffer + data_size, buf, len);
	data_size += len;

	return NOERROR;
}

int CAMRDecoder::DecodeFrame(BYTE *src, int len, int *bytes_processed)
{
	CAutoLock	lck(&lock_info);

	if (!bytes_processed) return -1;
	if (!src || len <= 0) return -1;

	int	type = (src[0] >> 3) & 0x0f;
	int frame_size = AMR_Frame_Size[type];
	
	// check the size
	*bytes_processed = 0;
	if (len < frame_size) return -1;

	// decode samples
	Decoder_Interface_Decode(decoder, src, dec_buffer, 0);
	*bytes_processed = frame_size;

	// update bitrate stuff
	bitrate_blur *= bitrate_decay;
	bitrate_cnt  *= bitrate_decay;
	bitrate_blur += ((frame_size-1)*8);		// first byte is mode... + several padding bits at the end
	bitrate_cnt  += 160;

	// increase counter
	dec_frames ++;
	frame_type = type;
	if (frame_type < ModeCount) {
		indicated_bitrate = ModeTab[frame_type];
	} else {
		indicated_bitrate = 0;
	}

	return 0;
}

HRESULT CAMRDecoder::DeliverData()
{
	HRESULT			hr		= NOERROR;
	IMediaSample	*sample = NULL;

	hr = m_pOutput->GetDeliveryBuffer(&sample, NULL, NULL, 0);
	if (FAILED(hr)) return hr;

	long out_size = 160 * 1 * sizeof(short);
	BYTE *out;
	sample->GetPointer(&out);
	memcpy(out, dec_buffer, out_size);
	sample->SetActualDataLength(out_size);

	// timestamps
	REFERENCE_TIME	rtdur = (160 * 10000 / 8);
	REFERENCE_TIME	start, stop;

	start		= rtOutput;
	rtOutput   += rtdur;
	stop		= rtOutput;
	
	sample->SetTime(&start, &stop);

	// deliver downstream
	hr = m_pOutput->Deliver(sample);

	sample->Release();
	return hr;
}

HRESULT CAMRDecoder::StartStreaming()
{
	CAutoLock	lck(&lock_info);
	HRESULT hr = __super::StartStreaming();
	if (FAILED(hr)) return hr;

	// open decoder
	if (decoder == NULL) {
		decoder = Decoder_Interface_init();
	}
	data_size = 0;
	dec_frames = 0;

	// init
	bitrate_cnt = 1;
	bitrate_blur = 0;
	return NOERROR;
}

HRESULT CAMRDecoder::StopStreaming()
{
	CAutoLock	lck(&lock_info);
	HRESULT hr = __super::StopStreaming();
	if (FAILED(hr)) return hr;

	// close decoder
	if (decoder) {
		Decoder_Interface_exit(decoder);
		decoder = NULL;
	}
	dec_frames = 0;

	// init
	bitrate_cnt = 1;
	bitrate_blur = 0;
	return NOERROR;
}

STDMETHODIMP CAMRDecoder::GetInfo(AMR_DECODER_INFO *info)
{
	CAutoLock	lck(&lock_info);
	if (!info) return E_INVALIDARG;

	if (decoder) {
		info->frame_type = frame_type;
		info->indicated_bitrate = indicated_bitrate;
		info->avg_bitrate = (8000.0 * bitrate_blur / bitrate_cnt);
	} else {
		info->frame_type = -1;
		info->indicated_bitrate = 0;
		info->avg_bitrate = 0;
	}

	info->samplerate = 8000;
	info->channels = 1;
	info->frames = dec_frames;

	return NOERROR;
}



