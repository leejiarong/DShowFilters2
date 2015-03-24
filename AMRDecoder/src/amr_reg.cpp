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
//	Registry Information
//
//-----------------------------------------------------------------------------


const AMOVIESETUP_MEDIATYPE sudPinTypes[] =   
		{ 
			{   &MEDIATYPE_Stream,	&MEDIASUBTYPE_None	},
			{	&MEDIATYPE_Stream,	&MEDIASUBTYPE_AMR	},
			{	&MEDIATYPE_Audio,   &MEDIASUBTYPE_AMR	},
			{	&MEDIATYPE_Audio, 	&MEDIASUBTYPE_PCM	}
		};

//-----------------------------------------------------------------------------
//	Demux
//-----------------------------------------------------------------------------

const AMOVIESETUP_PIN psudDemuxPins[] = 
		{ 
			{ L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, NULL, 2, &sudPinTypes[0] },
			{ L"Output", FALSE, TRUE, FALSE, TRUE,  &CLSID_NULL, NULL, 1, &sudPinTypes[2] } 
		};   

const AMOVIESETUP_FILTER sudAMRDemux = 
		{ 
			&CLSID_MonogramAMRDemux,		// clsID
            L"MONOGRAM AMR Splitter",		// strName
            MERIT_NORMAL,					// dwMerit
            2,								// nPins
            psudDemuxPins					// lpPin 
		};                     

//-----------------------------------------------------------------------------
//	Mux
//-----------------------------------------------------------------------------

const AMOVIESETUP_PIN psudMuxPins[] = 
		{ 
			{ L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, NULL, 1, &sudPinTypes[2] },
			{ L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, NULL, 2, &sudPinTypes[0] } 
		};   

const AMOVIESETUP_FILTER sudAMRMux = 
		{ 
			&CLSID_MonogramAMRMux,			// clsID
            L"MONOGRAM AMR Mux",			// strName
            MERIT_NORMAL,					// dwMerit
            2,								// nPins
            psudMuxPins						// lpPin 
		};                     

//-----------------------------------------------------------------------------
//	Decoder
//-----------------------------------------------------------------------------

const AMOVIESETUP_PIN psudDecoderPins[] = 
		{ 
			{ L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, NULL, 1, &sudPinTypes[2] },
			{ L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, NULL, 1, &sudPinTypes[3] } 
		};   

const AMOVIESETUP_FILTER sudAMRDecoder = 
		{ 
			&CLSID_MonogramAMRDecoder,		// clsID
            ACER_AMR_DECODER,		// strName
            MERIT_NORMAL,					// dwMerit
            2,								// nPins
            psudDecoderPins					// lpPin 
		};                     

//-----------------------------------------------------------------------------
//	Encoder
//-----------------------------------------------------------------------------

const AMOVIESETUP_PIN psudEncoderPins[] = 
		{ 
			{ L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, NULL, 1, &sudPinTypes[3] },
			{ L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, NULL, 1, &sudPinTypes[2] } 
		};   

const AMOVIESETUP_FILTER sudAMREncoder = 
		{ 
			&CLSID_MonogramAMREncoder,		// clsID
            L"MONOGRAM AMR Encoder",		// strName
            MERIT_NORMAL,					// dwMerit
            2,								// nPins
            psudEncoderPins					// lpPin 
		};                     

//-----------------------------------------------------------------------------
//	Templates
//-----------------------------------------------------------------------------

CFactoryTemplate g_Templates[]=
		{
			/*
			//-----------------------------------------------------------------
			//	Demux
			//-----------------------------------------------------------------
			{
				L"MONOGRAM AMR Splitter",
				&CLSID_MonogramAMRDemux,
				CAMRDemux::CreateInstance, 
				NULL,
				&sudAMRDemux 
			},
			{
				L"MONOGRAM AMR Splitter",
				&CLSID_MonogramAMRDemuxPage,
				CAMRDemuxPage::CreateInstance
			},
			//-----------------------------------------------------------------
			//	Mux
			//-----------------------------------------------------------------
			{
				L"MONOGRAM AMR Mux",
				&CLSID_MonogramAMRMux,
				CAMRMux::CreateInstance, 
				NULL,
				&sudAMRMux 
			},
			{
				L"MONOGRAM AMR Mux",
				&CLSID_MonogramAMRMuxPage,
				CAMRMuxPage::CreateInstance
			},
			*/
			//-----------------------------------------------------------------
			//	Decoder
			//-----------------------------------------------------------------
			{
				ACER_AMR_DECODER,
				&CLSID_MonogramAMRDecoder,
				CAMRDecoder::CreateInstance, 
				NULL,
				&sudAMRDecoder 
			},
			/*
			{
				L"MONOGRAM AMR Decoder",
				&CLSID_MonogramAMRDecoderPage,
				CAMRDecoderPage::CreateInstance
			},
			//-----------------------------------------------------------------
			//	Encoder
			//-----------------------------------------------------------------
			{
				L"MONOGRAM AMR Encoder",
				&CLSID_MonogramAMREncoder,
				CAMREncoder::CreateInstance, 
				NULL,
				&sudAMREncoder 
			},
			{
				L"MONOGRAM AMR Encoder",
				&CLSID_MonogramAMREncoderPage,
				CAMREncoderPage::CreateInstance
			}
			*/
		};

int g_cTemplates = sizeof(g_Templates)/sizeof(g_Templates[0]);


//-----------------------------------------------------------------------------
//
//	DLL Entry Points
//
//-----------------------------------------------------------------------------

STDAPI DllRegisterServer() 
{
	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	return AMovieDllRegisterServer2(FALSE);
}

