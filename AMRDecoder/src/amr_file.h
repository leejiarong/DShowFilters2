//-----------------------------------------------------------------------------
//
//	MONOGRAM AMR Filter Pack
//
//	Author : Igor Janos
//
//-----------------------------------------------------------------------------
#pragma once

class CAMRReader;

//-----------------------------------------------------------------------------
//
//	CAMRPacket
//
//-----------------------------------------------------------------------------

class CAMRPacket
{
public:
	__int64			file_position;		// absolute file position (in bytes)
	BYTE			packet[64];			// we own this one
	int				packet_size;		// whole packet size	
	REFERENCE_TIME	tStart, tStop;

public:
	CAMRPacket();
	virtual ~CAMRPacket();

	// loading packets
	int Load(CAMRReader *reader);
};


//-----------------------------------------------------------------------------
//
//	CAMRFile class
//
//-----------------------------------------------------------------------------

class CAMRFile
{
public:

	__int64			duration_10mhz;			// total file duration
	__int64			total_frames;			// total number of AMR frames

	// internals
	CAMRReader		*reader;				// file reader interface

	// current position
	__int64			total_samples;
	__int64			current_sample;
	CArray<__int64>	seek_table;

public:
	CAMRFile();
	virtual ~CAMRFile();

	// I/O for AMR file
	int Open(CAMRReader *reader);

	// parsing out packets
	int ReadAudioPacket(CAMRPacket *packet, __int64 *cur_sample);
	int Seek(__int64 seek_sample);

};

extern const int AMR_Frame_Size[];
