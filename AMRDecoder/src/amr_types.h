//-----------------------------------------------------------------------------
//
//	MONOGRAM AMR Filter Pack
//
//	Author : Igor Janos
//
//-----------------------------------------------------------------------------

#define MM_AMR_VERSION		L"1.0.1.0"

//-----------------------------------------------------------------------------
//	Demux
//-----------------------------------------------------------------------------

// {24FA7933-FE18-46a9-914A-C2AA0DBACE93}
static const GUID CLSID_MonogramAMRDemux = 
{ 0x24fa7933, 0xfe18, 0x46a9, { 0x91, 0x4a, 0xc2, 0xaa, 0xd, 0xba, 0xce, 0x93 } };

// {D7AF1F00-A702-4d1b-8490-8B7E0CDC3DEF}
static const GUID CLSID_MonogramAMRDemuxPage = 
{ 0xd7af1f00, 0xa702, 0x4d1b, { 0x84, 0x90, 0x8b, 0x7e, 0xc, 0xdc, 0x3d, 0xef } };

// {61E14FFB-FD77-4d31-AE7C-52BE908FC846}
static const GUID IID_IMonogramAMRDemux = 
{ 0x61e14ffb, 0xfd77, 0x4d31, { 0xae, 0x7c, 0x52, 0xbe, 0x90, 0x8f, 0xc8, 0x46 } };

//-----------------------------------------------------------------------------
//	Mux
//-----------------------------------------------------------------------------

// {AAA4AACD-FD95-4240-9C45-9EB98E5DAC52}
static const GUID CLSID_MonogramAMRMux = 
{ 0xaaa4aacd, 0xfd95, 0x4240, { 0x9c, 0x45, 0x9e, 0xb9, 0x8e, 0x5d, 0xac, 0x52 } };

// {B6EAE677-074B-43ea-9239-5E509F87C652}
static const GUID CLSID_MonogramAMRMuxPage = 
{ 0xb6eae677, 0x74b, 0x43ea, { 0x92, 0x39, 0x5e, 0x50, 0x9f, 0x87, 0xc6, 0x52 } };


//-----------------------------------------------------------------------------
//	Decoder
//-----------------------------------------------------------------------------

// {50DDA33E-C529-4343-9689-338ADC793BB5}
//static const GUID CLSID_MonogramAMRDecoder = 
//{ 0x50dda33e, 0xc529, 0x4343, { 0x96, 0x89, 0x33, 0x8a, 0xdc, 0x79, 0x3b, 0xb5 } };
// {CC0B18E2-7533-412a-861B-A152E9BB7650}
static const GUID CLSID_MonogramAMRDecoder = 
{ 0xcc0b18e2, 0x7533, 0x412a, { 0x86, 0x1b, 0xa1, 0x52, 0xe9, 0xbb, 0x76, 0x50 } };


// {BA327E17-6AE9-430b-8246-1A90208AD1D7}
static const GUID CLSID_MonogramAMRDecoderPage = 
{ 0xba327e17, 0x6ae9, 0x430b, { 0x82, 0x46, 0x1a, 0x90, 0x20, 0x8a, 0xd1, 0xd7 } };

// {B60CDD28-480E-4f20-8C7F-E83B147CE196}
static const GUID IID_IMonogramAMRDecoder = 
{ 0xb60cdd28, 0x480e, 0x4f20, { 0x8c, 0x7f, 0xe8, 0x3b, 0x14, 0x7c, 0xe1, 0x96 } };

//-----------------------------------------------------------------------------
//	Encoder
//-----------------------------------------------------------------------------

// {99735894-CAF4-488b-8275-B8CB1998216E}
static const GUID CLSID_MonogramAMREncoder = 
{ 0x99735894, 0xcaf4, 0x488b, { 0x82, 0x75, 0xb8, 0xcb, 0x19, 0x98, 0x21, 0x6e } };

// {9B2DBA95-39D2-4537-8BBF-CED535E8DE56}
static const GUID CLSID_MonogramAMREncoderPage = 
{ 0x9b2dba95, 0x39d2, 0x4537, { 0x8b, 0xbf, 0xce, 0xd5, 0x35, 0xe8, 0xde, 0x56 } };

// {6A5A4D78-1856-4720-A83D-4220605D81A8}
static const GUID IID_IMonogramAMREncoder = 
{ 0x6a5a4d78, 0x1856, 0x4720, { 0xa8, 0x3d, 0x42, 0x20, 0x60, 0x5d, 0x81, 0xa8 } };





// {726D6173-0000-0010-8000-00AA00389B71}
static const GUID MEDIASUBTYPE_AMR =
{ 0x726D6173, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } };



struct AMR_DEMUX_INFO
{
	__int64	total_frames;
	__int64	duration_10mhz;
};


DECLARE_INTERFACE_(IMonogramAMRDemux, IUnknown)
{
	STDMETHOD(GetInfo)(AMR_DEMUX_INFO *info);	
};



struct AMR_ENCODER_INFO
{
	int			samplerate;
	int			channels;
	__int64		frames;
};

DECLARE_INTERFACE_(IMonogramAMREncoder, IUnknown)
{
	STDMETHOD(GetInfo)(AMR_ENCODER_INFO *info);
	STDMETHOD(GetMode)(int *mode);
	STDMETHOD(SetMode)(int mode);
};

struct AMR_DECODER_INFO
{
	int			frame_type;
	int			indicated_bitrate;
	double		avg_bitrate;
	int			samplerate;
	int			channels;
	__int64		frames;
};

DECLARE_INTERFACE_(IMonogramAMRDecoder, IUnknown)
{
	STDMETHOD(GetInfo)(AMR_DECODER_INFO *info);
};

