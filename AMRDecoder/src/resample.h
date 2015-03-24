//-----------------------------------------------------------------------------
//
//	MONOGRAM AMR Filter Pack
//
//	Author : Igor Janos
//
//-----------------------------------------------------------------------------
#pragma once

/*
	This code is based on libavcodec::resample
*/

#ifndef INT16_MAX
#define INT16_MAX		(0x7fff)
#endif

#ifndef INT16_MIN
#define INT16_MIN		(-0x7fff-1)
#endif

#define RSHIFT(a,b) ((a) > 0 ? ((a) + ((1<<(b))>>1))>>(b) : ((a) + ((1<<(b))>>1)-1)>>(b))
#define ROUNDED_DIV(a,b) (((a)>0 ? (a) + ((b)>>1) : (a) - ((b)>>1))/(b))
#define FFABS(a) ((a) >= 0 ? (a) : (-(a)))
#define FFSIGN(a) ((a) > 0 ? 1 : -1)
#define FFMAX(a,b) ((a) > (b) ? (a) : (b))
#define FFMIN(a,b) ((a) > (b) ? (b) : (a))
#define FFSWAP(type,a,b) do{type SWAP_tmp= b; b= a; a= SWAP_tmp;}while(0)

#if 1
	#define FILTER_SHIFT	15
	#define FELEM			int16
	#define FELEM2			int32
	#define FELEM_MAX		INT16_MAX
	#define FELEM_MIN		INT16_MIN
#else
	#define FILTER_SHIFT	22
	#define FELEM			int32
	#define FELEM2			int64
	#define FELEM_MAX		INT32_MAX
	#define FELEM_MIN		INT32_MIN
#endif

typedef __int16		int16;
typedef __int32		int32;
typedef __int64		int64;

//-----------------------------------------------------------------------------
//
//	Resample class
//
//-----------------------------------------------------------------------------
class Resample
{
public:
	int16		*temp[6];
	int			temp_len;

	float		ratio;

	int			input_channels;
	int			output_channels;
	int			filter_channels;

	FELEM		*filter_bank;
	int			filter_length;
	int			ideal_dst_incr;
	int			dst_incr;
	int			index;
	int			frac;
	int			src_incr;
	int			compensation_distance;
	int			phase_shift;
	int			phase_mask;
	int			linear;

	void Compensate(int sample_delta, int comp_distance);
public:
	Resample();
	virtual ~Resample();

	int Open(int channels, int input_rate, int output_rate);
	void Close();
	int Process(int16 *output[6], int16 *input[6], int nb_samples);
};
