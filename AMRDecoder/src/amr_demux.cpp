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
//	CAMRDemux class
//
//-----------------------------------------------------------------------------

CUnknown *CAMRDemux::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
	return new CAMRDemux(pUnk, phr);
}

CAMRDemux::CAMRDemux(LPUNKNOWN pUnk, HRESULT *phr) :
	CBaseFilter(_T("AMR Splitter"), pUnk, &lock_filter, CLSID_MonogramAMRDemux, phr),
	CAMThread(),
	reader(NULL),
	file(NULL),
	rtCurrent(0),
	rtStop(0xFFFFFFFFFFFFFF),
	rate(1.0),
	ev_abort(TRUE)
{
	input = new CAMRInputPin(NAME("AMR Input Pin"), this, phr, L"In");
	output.RemoveAll();
	retired.RemoveAll();

	ev_abort.Reset();
}

CAMRDemux::~CAMRDemux()
{
	// just to be sure
	if (reader) { delete reader; reader = NULL; }
	for (int i=0; i<output.GetCount(); i++) {
		CAMROutputPin	*pin = output[i];
		if (pin) delete pin;
	}
	output.RemoveAll();
	for (int i=0; i<retired.GetCount(); i++) {
		CAMROutputPin	*pin = retired[i];
		if (pin) delete pin;
	}
	retired.RemoveAll();
	if (input) { delete input; input = NULL; }
}

STDMETHODIMP CAMRDemux::NonDelegatingQueryInterface(REFIID riid,void **ppv)
{
    CheckPointer(ppv,E_POINTER);
	if (riid == IID_ISpecifyPropertyPages) {
		return GetInterface((ISpecifyPropertyPages*)this, ppv);
	} else
	if (riid == IID_IMonogramAMRDemux) {
		return GetInterface((IMonogramAMRDemux*)this, ppv);
	} else
    if (riid == IID_IMediaSeeking) {
        return GetInterface((IMediaSeeking*)this, ppv);
	} else {
		return CBaseFilter::NonDelegatingQueryInterface(riid,ppv);
	}
}

STDMETHODIMP CAMRDemux::GetPages(CAUUID *pPages)
{
    CheckPointer(pPages,E_POINTER);

    pPages->cElems = 1;
    pPages->pElems = (GUID *) CoTaskMemAlloc(sizeof(GUID));
    if (pPages->pElems == NULL) {
        return E_OUTOFMEMORY;
    }

	*(pPages->pElems) = CLSID_MonogramAMRDemuxPage;
    return NOERROR;

} // GetPages

int CAMRDemux::GetPinCount()
{
	// return pin count
	CAutoLock	Lock(&lock_filter);
	return ((input ? 1 : 0) + output.GetCount());
}

CBasePin *CAMRDemux::GetPin(int n)
{
	CAutoLock	Lock(&lock_filter);
	if (n == 0) return input;
	n -= 1;
	int l = output.GetCount();

	// return the requested output pin
	if (n >= l) return NULL;
	return output[n];
}

HRESULT CAMRDemux::CheckConnect(PIN_DIRECTION Dir, IPin *pPin)
{
	return NOERROR;
}

HRESULT CAMRDemux::CheckInputType(const CMediaType* mtIn)
{
	if (mtIn->majortype == MEDIATYPE_Stream) {
		// we are sure we can accept this type
		if (mtIn->subtype == MEDIASUBTYPE_AMR) return NOERROR;

		// and we may accept unknown type as well
		if (mtIn->subtype == MEDIASUBTYPE_None ||
			mtIn->subtype == MEDIASUBTYPE_NULL ||
			mtIn->subtype == GUID_NULL
			) return NOERROR;
	} else
	if (mtIn->majortype == GUID_NULL) {
		return NOERROR;
	}

	// sorry.. nothing else
	return E_FAIL;
}

HRESULT CAMRDemux::CompleteConnect(PIN_DIRECTION Dir, CBasePin *pCaller, IPin *pReceivePin)
{
	if (Dir == PINDIR_INPUT) {
		// when our input pin gets connected we have to scan
		// the input file if it is really musepack.
		ASSERT(input && input->Reader());
		ASSERT(!reader);
		ASSERT(!file);

		//---------------------------------------------------------------------
		//
		//	Analyse the source file
		//
		//---------------------------------------------------------------------
		reader = new CAMRReader(input->Reader());
		file = new CAMRFile();

		// try to open the file
		int ret = file->Open(reader);
		if (ret < 0) {
			delete file;	file = NULL;
			delete reader;	reader = NULL;
			return E_FAIL;
		}

		HRESULT			hr = NOERROR;
		CAMROutputPin	*opin = new CAMROutputPin(_T("Outpin"), this, &hr, L"Out", 5);
		ConfigureMediaType(opin);
		AddOutputPin(opin);
	} else {
	}
	return NOERROR;
}


HRESULT CAMRDemux::RemoveOutputPins()
{
	CAutoLock	Lck(&lock_filter);
	if (m_State != State_Stopped) return VFW_E_NOT_STOPPED;

	// we retire all current output pins
	for (int i=0; i<output.GetCount(); i++) {
		CAMROutputPin *pin = output[i];
		if (pin->IsConnected()) {
			pin->GetConnected()->Disconnect();
			pin->Disconnect();
		}
		retired.Add(pin);
	}
	output.RemoveAll();
	return NOERROR;
}


HRESULT CAMRDemux::ConfigureMediaType(CAMROutputPin *pin)
{
	CMediaType		mt;
	mt.majortype = MEDIATYPE_Audio;
	mt.subtype = MEDIASUBTYPE_AMR;
	mt.formattype = FORMAT_WaveFormatEx;
	mt.lSampleSize = 1*1024;				// should be way enough

	ASSERT(file);

	// let us fill the waveformatex structure
	WAVEFORMATEX *wfx = (WAVEFORMATEX*)mt.AllocFormatBuffer(sizeof(WAVEFORMATEX));
	memset(wfx, 0, sizeof(*wfx));
	wfx->wBitsPerSample = 0;
	wfx->nChannels = 1;
	wfx->nSamplesPerSec = 8000;
	wfx->nBlockAlign = 1;
	wfx->nAvgBytesPerSec = 0;
	wfx->wFormatTag = 0;

	// the one and only type
	pin->mt_types.Add(mt);
	return NOERROR;
}

HRESULT CAMRDemux::BreakConnect(PIN_DIRECTION Dir, CBasePin *pCaller)
{
	ASSERT(m_State == State_Stopped);

	if (Dir == PINDIR_INPUT) {
		// let's disconnect the output pins
		ev_abort.Set();
		//ev_ready.Set();

		HRESULT hr = RemoveOutputPins();
		if (FAILED(hr)) return hr;

		// destroy input file, reader and update property page
		if (file) { delete file; file = NULL; }
		if (reader) { delete reader; reader = NULL;	}

		ev_abort.Reset();
	} else 
	if (Dir == PINDIR_OUTPUT) {
		// nothing yet
	}
	return NOERROR;
}


// Output pins
HRESULT CAMRDemux::AddOutputPin(CAMROutputPin *pPin)
{
	CAutoLock	lck(&lock_filter);
	output.Add(pPin);
	return NOERROR;
}

STDMETHODIMP CAMRDemux::GetInfo(AMR_DEMUX_INFO *info)
{
	CAutoLock	lck(&lock_filter);
	if (!info) return E_POINTER;
	if (!file) return E_FAIL;
	
	// fill in the struct
	info->duration_10mhz = file->duration_10mhz;
	info->total_frames = file->total_frames;
	return NOERROR;
}


// IMediaSeeking

STDMETHODIMP CAMRDemux::GetCapabilities(DWORD* pCapabilities)
{
	return pCapabilities ? *pCapabilities =	
			AM_SEEKING_CanGetStopPos|AM_SEEKING_CanGetDuration|AM_SEEKING_CanSeekAbsolute|AM_SEEKING_CanSeekForwards|AM_SEEKING_CanSeekBackwards, 
			S_OK : E_POINTER;
}
STDMETHODIMP CAMRDemux::CheckCapabilities(DWORD* pCapabilities)
{
	CheckPointer(pCapabilities, E_POINTER);
	if (*pCapabilities == 0) return S_OK;
	DWORD caps;
	GetCapabilities(&caps);
	if ((caps&*pCapabilities) == 0) return E_FAIL;
	if (caps == *pCapabilities) return S_OK;
	return S_FALSE;
}
STDMETHODIMP CAMRDemux::IsFormatSupported(const GUID* pFormat) {return !pFormat ? E_POINTER : *pFormat == TIME_FORMAT_MEDIA_TIME ? S_OK : S_FALSE;}
STDMETHODIMP CAMRDemux::QueryPreferredFormat(GUID* pFormat) {return GetTimeFormat(pFormat);}
STDMETHODIMP CAMRDemux::GetTimeFormat(GUID* pFormat) {return pFormat ? *pFormat = TIME_FORMAT_MEDIA_TIME, S_OK : E_POINTER;}
STDMETHODIMP CAMRDemux::IsUsingTimeFormat(const GUID* pFormat) {return IsFormatSupported(pFormat);}
STDMETHODIMP CAMRDemux::SetTimeFormat(const GUID* pFormat) {return S_OK == IsFormatSupported(pFormat) ? S_OK : E_INVALIDARG;}
STDMETHODIMP CAMRDemux::GetStopPosition(LONGLONG* pStop) 
{
	if (pStop) *pStop = this->rtStop;
	return NOERROR; 
}
STDMETHODIMP CAMRDemux::GetCurrentPosition(LONGLONG* pCurrent) {return E_NOTIMPL;}
STDMETHODIMP CAMRDemux::ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat) {return E_NOTIMPL;}

STDMETHODIMP CAMRDemux::SetPositions(LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags)
{
	return SetPositionsInternal(0, pCurrent, dwCurrentFlags, pStop, dwStopFlags);
}

STDMETHODIMP CAMRDemux::GetPositions(LONGLONG* pCurrent, LONGLONG* pStop)
{
	if(pCurrent) *pCurrent = rtCurrent;
	if(pStop) *pStop = rtStop;
	return S_OK;
}
STDMETHODIMP CAMRDemux::GetAvailable(LONGLONG* pEarliest, LONGLONG* pLatest)
{
	if(pEarliest) *pEarliest = 0;
	return GetDuration(pLatest);
}
STDMETHODIMP CAMRDemux::SetRate(double dRate) {return dRate > 0 ? rate = dRate, S_OK : E_INVALIDARG;}
STDMETHODIMP CAMRDemux::GetRate(double* pdRate) {return pdRate ? *pdRate = rate, S_OK : E_POINTER;}
STDMETHODIMP CAMRDemux::GetPreroll(LONGLONG* pllPreroll) {return pllPreroll ? *pllPreroll = 0, S_OK : E_POINTER;}

STDMETHODIMP CAMRDemux::GetDuration(LONGLONG* pDuration) 
{	
	CheckPointer(pDuration, E_POINTER); 
	*pDuration = 0;

	if (file) {
		if (pDuration) *pDuration = file->duration_10mhz;
	}
	return S_OK;
}

STDMETHODIMP CAMRDemux::SetPositionsInternal(int iD, LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags)
{
	// only our first pin can seek
	if (iD != 0) return NOERROR;


	CAutoLock cAutoLock(&lock_filter);

	if (!pCurrent && !pStop || (dwCurrentFlags&AM_SEEKING_PositioningBitsMask) == AM_SEEKING_NoPositioning 
		&& (dwStopFlags&AM_SEEKING_PositioningBitsMask) == AM_SEEKING_NoPositioning)
		return S_OK;

	REFERENCE_TIME rtCurrent = this->rtCurrent, rtStop = this->rtStop;

	if (pCurrent) {
		switch(dwCurrentFlags&AM_SEEKING_PositioningBitsMask) {
		case AM_SEEKING_NoPositioning: break;
		case AM_SEEKING_AbsolutePositioning: rtCurrent = *pCurrent; break;
		case AM_SEEKING_RelativePositioning: rtCurrent = rtCurrent + *pCurrent; break;
		case AM_SEEKING_IncrementalPositioning: rtCurrent = rtCurrent + *pCurrent; break;
		}
	}

	if (pStop) {
		switch(dwStopFlags&AM_SEEKING_PositioningBitsMask) {
		case AM_SEEKING_NoPositioning: break;
		case AM_SEEKING_AbsolutePositioning: rtStop = *pStop; break;
		case AM_SEEKING_RelativePositioning: rtStop += *pStop; break;
		case AM_SEEKING_IncrementalPositioning: rtStop = rtCurrent + *pStop; break;
		}
	}

	if (this->rtCurrent == rtCurrent && this->rtStop == rtStop) {
		//return S_OK;
	}

	this->rtCurrent = rtCurrent;
	this->rtStop = rtStop;

	// now there are new valid Current and Stop positions
	HRESULT hr = DoNewSeek();
	return hr;
}


STDMETHODIMP CAMRDemux::Pause()
{
	CAutoLock	lck(&lock_filter);

	if (m_State == State_Stopped) {

		ev_abort.Reset();

		// activate pins
		for (int i=0; i<output.GetCount(); i++) output[i]->Active();
		if (input) input->Active();

		// seekneme na danu poziciu
		DoNewSeek();

		// pustime parser thread
		if (!ThreadExists()) {
			Create();
			CallWorker(CMD_RUN);
		}
	}

	m_State = State_Paused;
	return NOERROR;
}

STDMETHODIMP CAMRDemux::Stop()
{
	CAutoLock	lock(&lock_filter);
	HRESULT		hr = NOERROR;

	if (m_State != State_Stopped) {

		// set abort
		ev_abort.Set();
		if (reader) reader->BeginFlush();

		// deaktivujeme piny
		if (input) input->Inactive();
		for (int i=0; i<output.GetCount(); i++) output[i]->Inactive();

		// zrusime parser thread
		if (ThreadExists()) {
			CallWorker(CMD_EXIT);
			Close();
		}

		if (reader) reader->EndFlush();
		ev_abort.Reset();
	}


	m_State = State_Stopped;
	return hr;
}


HRESULT CAMRDemux::DoNewSeek()
{
	CAMROutputPin	*pin = output[0];
	HRESULT			hr;

	if (!pin->IsConnected()) return NOERROR;

	// stop first
	ev_abort.Set();
	if (reader) reader->BeginFlush();

	FILTER_STATE	state = m_State;

	// abort
	if (state != State_Stopped) {
		if (pin->ThreadExists()) {
			pin->ev_abort.Set();
			pin->DeliverBeginFlush();
			if (ThreadExists()) {
				CallWorker(CMD_STOP);
			}
			pin->CallWorker(CMD_STOP);

			pin->DeliverEndFlush();
			pin->FlushQueue();
		}
	}

	pin->DoNewSegment(rtCurrent, rtStop, rate);
	if (reader) reader->EndFlush();

	// seek the file
	if (file) {
		__int64 sample_pos = (rtCurrent * 8000) / 10000000;
		file->Seek(sample_pos);
	}

	ev_abort.Reset();

	if (state != State_Stopped) {
		// spustime aj jeho thread
		pin->FlushQueue();
		pin->ev_abort.Reset();
		if (pin->ThreadExists()) {
			pin->CallWorker(CMD_RUN);
		}
		if (ThreadExists()) {
			CallWorker(CMD_RUN);
		}
	}

	return NOERROR;
}

DWORD CAMRDemux::ThreadProc()
{
	DWORD	cmd, cmd2;
	while (true) {
		cmd = GetRequest();
		switch (cmd) {
		case CMD_EXIT:	Reply(NOERROR); return 0;
		case CMD_STOP:	
			{
				Reply(NOERROR); 
			}
			break;
		case CMD_RUN:
			{
				Reply(NOERROR);
				if (!file) break;

				CAMRPacket		packet;
				int				ret=0;
				bool			eos=false;
				HRESULT			hr;
				__int64			current_sample=0;

				/*
					With a more complex demultiplexer we would need a mechanism
					to identify streams. Now we have only one output stream
					so it's easy.
				*/

				if (output.GetCount() <= 0) break;
				if (output[0]->IsConnected() == FALSE) break;
				int	delivered = 0;

				do {

					// are we supposed to abort ?
					if (ev_abort.Check()) {
						break; 
					}

					ret = file->ReadAudioPacket(&packet, &current_sample);
					if (ret == -2) {
						// end of stream
						if (!ev_abort.Check()) {
							output[0]->DoEndOfStream();
						}
						break;
					} else
					if (ret < 0) {
						break;
					} else {
						// compute time stamp
						REFERENCE_TIME	tStart = (current_sample * 10000000) / 8000;
						REFERENCE_TIME	tStop  = ((current_sample + 160) * 10000000) / 8000;

						packet.tStart = tStart - rtCurrent;
						packet.tStop  = tStop  - rtCurrent;

						// deliver packet
						hr = output[0]->DeliverPacket(packet);
						if (FAILED(hr)) {
							break;
						}

						delivered++;
					}

				} while (!CheckRequest(&cmd2));
			}
			break;
		default:
			Reply(E_UNEXPECTED);
			break;
		}
	}
	return 0;
}

