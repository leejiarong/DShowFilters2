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
//	CAMRInputPin class
//
//-----------------------------------------------------------------------------

CAMRInputPin::CAMRInputPin(TCHAR *pObjectName, CAMRDemux *pDemux, HRESULT *phr, LPCWSTR pName) :
	CBaseInputPin(pObjectName, pDemux, &pDemux->lock_filter, phr, pName),
	reader(NULL)
{
	// assign the splitter filter
	demux = pDemux;
}

CAMRInputPin::~CAMRInputPin()
{
	if (reader) { reader->Release(); reader = NULL; }
}

// this is a pull mode pin - these can not happen

STDMETHODIMP CAMRInputPin::EndOfStream()		{ return E_UNEXPECTED; }
STDMETHODIMP CAMRInputPin::BeginFlush()			{ return E_UNEXPECTED; }
STDMETHODIMP CAMRInputPin::EndFlush()			{ return E_UNEXPECTED; }
STDMETHODIMP CAMRInputPin::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double rate)	{ return E_UNEXPECTED; }

// we don't support this kind of transport
STDMETHODIMP CAMRInputPin::Receive(IMediaSample * pSample) { return E_UNEXPECTED; }

// check for input type
HRESULT CAMRInputPin::CheckConnect(IPin *pPin)
{
	HRESULT hr = demux->CheckConnect(PINDIR_INPUT, pPin);
	if (FAILED(hr)) return hr;

	return CBaseInputPin::CheckConnect(pPin);
}

HRESULT CAMRInputPin::BreakConnect()
{
	// Can't disconnect unless stopped
	ASSERT(IsStopped());

	demux->BreakConnect(PINDIR_INPUT, this);

	// release the reader
	if (reader) reader->Release(); reader = NULL;
	return CBaseInputPin::BreakConnect();
}

HRESULT CAMRInputPin::CompleteConnect(IPin *pReceivePin)
{
	// make sure there is a pin
	ASSERT(pReceivePin);

	if (reader) reader->Release(); reader = NULL;

	// and make sure it supports IAsyncReader
	HRESULT hr = pReceivePin->QueryInterface(IID_IAsyncReader, (void **)&reader);
	if (FAILED(hr)) return hr;

	// we're done here
	hr = demux->CompleteConnect(PINDIR_INPUT, this, pReceivePin);
	if (FAILED(hr)) return hr;

	return CBaseInputPin::CompleteConnect(pReceivePin);
}

HRESULT CAMRInputPin::CheckMediaType(const CMediaType* pmt)
{
	// make sure we have a type
	ASSERT(pmt);

	// ask the splitter
	return demux->CheckInputType(pmt);
}

HRESULT CAMRInputPin::SetMediaType(const CMediaType* pmt)
{
	// let the baseclass know
	if (FAILED(CheckMediaType(pmt))) return E_FAIL;

	HRESULT hr = CBasePin::SetMediaType(pmt);
	if (FAILED(hr)) return hr;

	return NOERROR;
}

HRESULT CAMRInputPin::Inactive()
{
	HRESULT hr = CBaseInputPin::Inactive();
	if (hr == VFW_E_NO_ALLOCATOR) hr = NOERROR;
	if (FAILED(hr)) return hr;

	return hr;
}

//-----------------------------------------------------------------------------
//
//	CAMROutputPin class
//
//-----------------------------------------------------------------------------

CAMROutputPin::CAMROutputPin(TCHAR *pObjectName, CAMRDemux *pDemux, HRESULT *phr, LPCWSTR pName, int iBuffers) :
	CBaseOutputPin(pObjectName, pDemux, &pDemux->lock_filter, phr, pName),
	CAMThread(),
	demux(pDemux),
	buffers(iBuffers),
	active(false),
	rtStart(0),
	rtStop(0xffffffffffff),
	rate(1.0),
	ev_can_read(TRUE),
	ev_can_write(TRUE),
	ev_abort(TRUE)
{
	discontinuity = true;
	eos_delivered = false;

	ev_can_read.Reset();
	ev_can_write.Set();
	ev_abort.Reset();

	// up to 5 seconds
	buffer_time_ms = 5000;
}

CAMROutputPin::~CAMROutputPin()
{
	// nothing yet
}

STDMETHODIMP CAMROutputPin::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	CheckPointer(ppv, E_POINTER);
	if (riid == IID_IMediaSeeking) {
		return GetInterface((IMediaSeeking*)this, ppv);
	} else {
		return CBaseOutputPin::NonDelegatingQueryInterface(riid, ppv);
	}
}

HRESULT CAMROutputPin::CheckMediaType(const CMediaType *mtOut)
{
	for (int i=0; i<mt_types.GetCount(); i++) {
		if (mt_types[i] == *mtOut) return NOERROR;
	}

	// reject type if it's not among ours
	return E_FAIL;
}

HRESULT CAMROutputPin::SetMediaType(const CMediaType *pmt)
{
	// just set internal media type
	if (FAILED(CheckMediaType(pmt))) return E_FAIL;
	return CBaseOutputPin::SetMediaType(pmt);
}

HRESULT CAMROutputPin::GetMediaType(int iPosition, CMediaType *pmt)
{
	// enumerate the list
	if (iPosition < 0) return E_INVALIDARG;
	if (iPosition >= mt_types.GetCount()) return VFW_S_NO_MORE_ITEMS;

	*pmt = mt_types[iPosition];
	return NOERROR;
}

HRESULT CAMROutputPin::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProp)
{
	ASSERT(pAlloc);
	ASSERT(pProp);
	HRESULT hr = NOERROR;

	pProp->cBuffers = max(buffers, 1);
	pProp->cbBuffer = max(m_mt.lSampleSize, 1);
	ALLOCATOR_PROPERTIES	Actual;
	hr = pAlloc->SetProperties(pProp, &Actual);
	if (FAILED(hr)) return hr;

	ASSERT(Actual.cBuffers >= pProp->cBuffers);
	return NOERROR;
}

HRESULT CAMROutputPin::BreakConnect()
{
	ASSERT(IsStopped());
	demux->BreakConnect(PINDIR_OUTPUT, this);
	discontinuity = true;
	return CBaseOutputPin::BreakConnect();
}

HRESULT CAMROutputPin::CompleteConnect(IPin *pReceivePin)
{
	// let the parent know
	HRESULT hr = demux->CompleteConnect(PINDIR_OUTPUT, this, pReceivePin);
	if (FAILED(hr)) return hr;

	discontinuity = true;
	// okey
	return CBaseOutputPin::CompleteConnect(pReceivePin);
}

STDMETHODIMP CAMROutputPin::Notify(IBaseFilter *pSender, Quality q)
{
	return S_FALSE;
}

HRESULT CAMROutputPin::Inactive()
{
	if (!active) return NOERROR;
	active = FALSE;

	// destroy everything
	ev_abort.Set();
	HRESULT hr = CBaseOutputPin::Inactive();
	ASSERT(SUCCEEDED(hr));

	if (ThreadExists()) {
		CallWorker(CMD_EXIT);
		Close();
	}
	FlushQueue();
	ev_abort.Reset();

	return NOERROR;
}


HRESULT CAMROutputPin::Active()
{
	if (active) return NOERROR;
	
	FlushQueue();
	if (!IsConnected()) return VFW_E_NOT_CONNECTED;

	ev_abort.Reset();

	HRESULT hr = CBaseOutputPin::Active();
	if (FAILED(hr)) {
		active = FALSE;
		return hr;
	}

	// new segment
	DoNewSegment(rtStart, rtStop, rate);

	// vytvorime novu queue
	if (!ThreadExists()) {
		Create();
		CallWorker(CMD_RUN);
	}

	active = TRUE;
	return hr;
}


HRESULT CAMROutputPin::DoNewSegment(REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, double dRate)
{
	// queue the EOS packet
	this->rtStart = rtStart;
	this->rtStop = rtStop;
	this->rate = dRate;
	eos_delivered = false;

	if (1) {
		discontinuity = true;
		return DeliverNewSegment(rtStart, rtStop, rate);
	} else {
		DataPacket	*packet = new DataPacket();
		{
			CAutoLock	lck(&lock_queue);
			packet->type = DataPacket::PACKET_TYPE_NEW_SEGMENT;
			packet->rtStart = rtStart;
			packet->rtStop = rtStop;
			packet->rate = rate;
			queue.AddTail(packet);
			ev_can_read.Set();
			discontinuity = true;
		}
	}
	return NOERROR;
}

// IMediaSeeking
STDMETHODIMP CAMROutputPin::GetCapabilities(DWORD* pCapabilities)					{ return demux->GetCapabilities(pCapabilities); }
STDMETHODIMP CAMROutputPin::CheckCapabilities(DWORD* pCapabilities)					{ return demux->CheckCapabilities(pCapabilities); }
STDMETHODIMP CAMROutputPin::IsFormatSupported(const GUID* pFormat)					{ return demux->IsFormatSupported(pFormat); }
STDMETHODIMP CAMROutputPin::QueryPreferredFormat(GUID* pFormat)						{ return demux->QueryPreferredFormat(pFormat); }
STDMETHODIMP CAMROutputPin::GetTimeFormat(GUID* pFormat)							{ return demux->GetTimeFormat(pFormat); }
STDMETHODIMP CAMROutputPin::IsUsingTimeFormat(const GUID* pFormat)					{ return demux->IsUsingTimeFormat(pFormat); }
STDMETHODIMP CAMROutputPin::SetTimeFormat(const GUID* pFormat)						{ return demux->SetTimeFormat(pFormat); }
STDMETHODIMP CAMROutputPin::GetDuration(LONGLONG* pDuration)						{ return demux->GetDuration(pDuration); }
STDMETHODIMP CAMROutputPin::GetStopPosition(LONGLONG* pStop)						{ return demux->GetStopPosition(pStop); }
STDMETHODIMP CAMROutputPin::GetCurrentPosition(LONGLONG* pCurrent)					{ return demux->GetCurrentPosition(pCurrent); }
STDMETHODIMP CAMROutputPin::GetPositions(LONGLONG* pCurrent, LONGLONG* pStop)		{ return demux->GetPositions(pCurrent, pStop); }
STDMETHODIMP CAMROutputPin::GetAvailable(LONGLONG* pEarliest, LONGLONG* pLatest)	{ return demux->GetAvailable(pEarliest, pLatest); }
STDMETHODIMP CAMROutputPin::SetRate(double dRate)									{ return demux->SetRate(dRate); }
STDMETHODIMP CAMROutputPin::GetRate(double* pdRate)									{ return demux->GetRate(pdRate); }
STDMETHODIMP CAMROutputPin::GetPreroll(LONGLONG* pllPreroll)						{ return demux->GetPreroll(pllPreroll); }
STDMETHODIMP CAMROutputPin::ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat)
{
	return demux->ConvertTimeFormat(pTarget, pTargetFormat, Source, pSourceFormat);
}
STDMETHODIMP CAMROutputPin::SetPositions(LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags)
{
	return demux->SetPositionsInternal(0, pCurrent, dwCurrentFlags, pStop, dwStopFlags);
}

int CAMROutputPin::GetDataPacket(DataPacket **packet)
{
	// this method may blokc
	HANDLE	events[] = { ev_can_write, ev_abort };
	DWORD	ret;

	do {
		// if the abort event is set, quit
		if (ev_abort.Check()) return -1;

		ret = WaitForMultipleObjects(2, events, FALSE, 10);
		if (ret == WAIT_OBJECT_0) {
			// return new packet
			*packet = new DataPacket();
			return 0;
		}

	} while (1);

	// unexpected
	return -1;
}

HRESULT CAMROutputPin::DeliverPacket(CAMRPacket &packet)
{
	// we don't accept packets while aborting...
	if (ev_abort.Check()) {
		return E_FAIL;
	}

	// ziskame novy packet na vystup
	DataPacket	*outp = NULL;
	int ret = GetDataPacket(&outp);
	if (ret < 0 || !outp) return E_FAIL;

	outp->type	  = DataPacket::PACKET_TYPE_DATA;

	// spocitame casy
	outp->rtStart = packet.tStart;
	outp->rtStop  = packet.tStop;
	outp->has_time = true;
	
	outp->size	  = packet.packet_size;
	outp->buf	  = (BYTE*)malloc(outp->size);
	memcpy(outp->buf, packet.packet, packet.packet_size);

	// each packet is sync point
	outp->sync_point = TRUE;

	// discontinuity ?
	if (discontinuity) {
		outp->discontinuity = TRUE;
		discontinuity = false;
	} else {
		outp->discontinuity = FALSE;
	}

	{
		// insert into queue
		CAutoLock		lck(&lock_queue);
		queue.AddTail(outp);
		ev_can_read.Set();

		// we allow buffering for a few seconds (might be usefull for network playback)
		if (GetBufferTime_MS() >= buffer_time_ms) {
			ev_can_write.Reset();
		}
	}

	return NOERROR;
}

HRESULT CAMROutputPin::DoEndOfStream()
{
	DataPacket	*packet = new DataPacket();

	// naqueueujeme EOS
	{
		CAutoLock	lck(&lock_queue);
		packet->type = DataPacket::PACKET_TYPE_EOS;
		queue.AddTail(packet);
		ev_can_read.Set();
	}
	eos_delivered = true;

	return NOERROR;
}

void CAMROutputPin::FlushQueue()
{
	CAutoLock	lck(&lock_queue);
	while (queue.GetCount() > 0) {
		DataPacket *packet = queue.RemoveHead();
		if (packet) delete packet;
	}
	ev_can_read.Reset();
	ev_can_write.Set();
}

HRESULT CAMROutputPin::DeliverDataPacket(DataPacket &packet)
{
	IMediaSample	*sample;
	HRESULT hr = GetDeliveryBuffer(&sample, NULL, NULL, 0);
	if (FAILED(hr)) {
		return E_FAIL;
	}

	// we should have enough space in there
	long lsize = sample->GetSize();
	ASSERT(lsize >= packet.size);

	BYTE			*buf;
	sample->GetPointer(&buf);

	//*************************************************************************
	//
	//	data
	//
	//*************************************************************************

	memcpy(buf, packet.buf, packet.size);
	sample->SetActualDataLength(packet.size);

	// sync point, discontinuity ?
	if (packet.sync_point) { sample->SetSyncPoint(TRUE); } else { sample->SetSyncPoint(FALSE); }
	if (packet.discontinuity) { 
		sample->SetDiscontinuity(TRUE); 
	} else { 
		sample->SetDiscontinuity(FALSE); 
	}

	// do we have a time stamp ?
	if (packet.has_time) { 
		sample->SetTime(&packet.rtStart, &packet.rtStop); 
	}

	// dorucime
	hr = Deliver(sample);	
	sample->Release();
	return hr;
}

__int64 CAMROutputPin::GetBufferTime_MS()
{
	CAutoLock	lck(&lock_queue);
	if (queue.IsEmpty()) return 0;

	DataPacket	*pfirst;
	DataPacket	*plast;
	__int64		tstart, tstop;
	POSITION	posf, posl;

	tstart = -1;
	tstop = -1;

	posf = queue.GetHeadPosition();
	while (posf) {
		pfirst = queue.GetNext(posf);
		if (pfirst->type == DataPacket::PACKET_TYPE_DATA && pfirst->rtStart != -1) {
			tstart = pfirst->rtStart;
			break;
		}
	}

	posl = queue.GetTailPosition();
	while (posl) {
		plast = queue.GetPrev(posl);
		if (plast->type == DataPacket::PACKET_TYPE_DATA && plast->rtStart != -1) {
			tstop = plast->rtStart;
			break;
		}
	}

	if (tstart == -1 || tstop == -1) return 0;

	// calculate time in milliseconds
	return (tstop - tstart) / 10000;
}

// vlakno na output
DWORD CAMROutputPin::ThreadProc()
{
	while (true) {
		DWORD cmd = GetRequest();
		switch (cmd) {
		case CMD_EXIT:		Reply(0); return 0; break;
		case CMD_STOP:
			{
				Reply(0); 
			}
			break;
		case CMD_RUN:
			{
				Reply(0);
				if (!IsConnected()) break;

				// deliver packets
				DWORD		cmd2;
				BOOL		is_first = true;
				DataPacket	*packet;
				HRESULT		hr;

				do {
					if (ev_abort.Check()) break;
					hr = NOERROR;
					
					HANDLE	events[] = { ev_can_read, ev_abort};
					DWORD	ret = WaitForMultipleObjects(2, events, FALSE, 10);

					if (ret == WAIT_OBJECT_0) {
						// look for new packet in queue
						{
							CAutoLock	lck(&lock_queue);
							packet = queue.RemoveHead();
							if (queue.IsEmpty()) ev_can_read.Reset();

							// allow buffering
							if (GetBufferTime_MS() < buffer_time_ms) ev_can_write.Set();
						}

						// bud dorucime End Of Stream, alebo packet
						if (packet->type == DataPacket::PACKET_TYPE_EOS) {
							DeliverEndOfStream();
						} else 
						if (packet->type == DataPacket::PACKET_TYPE_NEW_SEGMENT) {
							hr = DeliverNewSegment(packet->rtStart, packet->rtStop, packet->rate);
						} else
						if (packet->type == DataPacket::PACKET_TYPE_DATA) {
							hr = DeliverDataPacket(*packet);
						}					

						delete packet;

						if (FAILED(hr)) {
							break;
						}
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


//-----------------------------------------------------------------------------
//
//	CAMRReader class
//
//-----------------------------------------------------------------------------
CAMRReader::CAMRReader(IAsyncReader *rd)
{
	ASSERT(rd);

	reader = rd;
	reader->AddRef();
	position = 0;
}

CAMRReader::~CAMRReader()
{
	if (reader) {
		reader->Release();
		reader = NULL;
	}
}

int CAMRReader::GetSize(__int64 *avail, __int64 *total)
{
	// vraciame velkost
	HRESULT hr = reader->Length(total, avail);
	if (FAILED(hr)) return -1;
	return 0;
}

int CAMRReader::GetPosition(__int64 *pos, __int64 *avail)
{
	HRESULT hr;
	__int64	total;
	hr = reader->Length(&total, avail);
	if (FAILED(hr)) return -1;

	// aktualna pozicia
	if (pos) *pos = position;
	return 0;
}

int CAMRReader::Seek(__int64 pos)
{
	__int64	avail, total;
	GetSize(&avail, &total);
	if (pos < 0 || pos >= total) return -1;

	// seekneme
	position = pos;
	return 0;
}

int CAMRReader::Read(void *buf, int size)
{
	__int64	avail, total;
	GetSize(&avail, &total);
	if (position + size > avail) {
		return -1;
	}

	// TODO: Caching here !!!!
	//TRACE("    - read, %I64d, %d\n", position, size);

	HRESULT hr = reader->SyncRead(position, size, (BYTE*)buf);
	if (FAILED(hr)) {
		return -1;
	}

	// update position
	position += size;
	return 0;
}


DataPacket::DataPacket() :
	type(PACKET_TYPE_EOS),
	rtStart(0),
	rtStop(0),
	sync_point(FALSE),
	discontinuity(FALSE),
	buf(NULL),
	size(0)
{
}

DataPacket::~DataPacket()
{
	if (buf) {
		free(buf);
		buf = NULL;
	}
}











