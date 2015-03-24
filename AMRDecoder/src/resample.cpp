//-----------------------------------------------------------------------------
//
//	MONOGRAM AMR Filter Pack
//
//	Author : Igor Janos
//
//-----------------------------------------------------------------------------
#include "stdafx.h"
#define _USE_MATH_DEFINES
#include <math.h>


//-------------------------------------------------------------------------
//
//	Resample class
//
//-------------------------------------------------------------------------

/**
 * 0th order modified bessel function of the first kind.
 */
static double bessel(double x)
{
	double v=1;
	double t=1;
	int i;

	for(i=1; i<50; i++){
		t *= i;
		v += pow(x*x/4, i)/(t*t);
	}
	return v;
}

static inline int lrintf(float f)
{
	f += (3<<22);
	return *((int*)&f) - 0x4b400000;
}

#define MAX(a,b)			((a)>(b)?(a):(b))
#define MIN(a,b)			((a)<(b)?(a):(b))

inline int clip(int a,int b,int c)
{
	return (MIN(MAX(a,b), c));
}


/**
 * builds a polyphase filterbank.
 * @param factor resampling factor
 * @param scale wanted sum of coefficients for each filter
 * @param type 0->cubic, 1->blackman nuttall windowed sinc, 2->kaiser windowed sinc beta=16
 */
void build_filter(FELEM *filter, double factor, int tap_count, int phase_count, int scale, int type)
{
	int ph, i, v;
	double x, y, w;
	double *tab=(double*)malloc(tap_count*sizeof(double));
	const int center= (tap_count-1)/2;

	/* if upsampling, only need to interpolate, no filter */
	if (factor > 1.0) factor = 1.0;

	for (ph=0; ph<phase_count; ph++) {
		double norm = 0;
		double e= 0;
		for (i=0; i<tap_count; i++) {
			x = M_PI * ((double)(i - center) - (double)ph / phase_count) * factor;
			if (x == 0) y = 1.0; else y = sin(x) / x;
			switch(type){
			case 0:
				{
					const float d= -0.5; //first order derivative = -0.5
					x = fabs(((double)(i - center) - (double)ph / phase_count) * factor);
					if (x<1.0) {
						y= 1 - 3*x*x + 2*x*x*x + d*(            -x*x + x*x*x);
					} else {
						y=                       d*(-4 + 8*x - 5*x*x + x*x*x);
					}
				}
				break;
			case 1:
				{
					w = 2.0*x / (factor*tap_count) + M_PI;
					y *= 0.3635819 - 0.4891775 * cos(w) + 0.1365995 * cos(2*w) - 0.0106411 * cos(3*w);
				}
				break;
			case 2:
				{
					w = 2.0*x / (factor*tap_count*M_PI);
					y *= bessel(16*sqrt(MAX(1-w*w, 0)));
				}
				break;
			}

			tab[i] = y;
			norm += y;
		}

		/* normalize so that an uniform color remains the same */
		for (i=0; i<tap_count; i++) {
			v = clip(lrintf(tab[i] * scale / norm + e), FELEM_MIN, FELEM_MAX);
			filter[ph * tap_count + i] = v;
			e += tab[i] * scale / norm - v;
		}
	}
	free(tab);
}

Resample::Resample() 
{
	memset(temp, 0, sizeof(temp));
	temp_len = 0;
	filter_bank = NULL;
}

Resample::~Resample()
{
	Close();
}

void Resample::Close()
{
	int i;
	for (i=0; i<6; i++) {
		if (temp[i]) free(temp[i]);
	}
	memset(temp, 0, sizeof(temp));
	temp_len=0;
	if (filter_bank) {
		free(filter_bank);
		filter_bank = NULL;
	}
}

int Resample::Open(int channels, int input_rate, int output_rate)
{
	// oba dame rovnake
	input_channels = channels;
	output_channels = channels;
	filter_channels = channels;
	ratio = (float)output_rate / (float)input_rate;

	int	filter_size = 16;
	
	// cutoff
	double cutoff	= 1.0;
	double factor	= MIN(output_rate * cutoff / input_rate, 1.0);

	phase_shift = 10;
	int phase_count = 1 << phase_shift;
	phase_mask		= phase_count-1;
	linear			= 0.0;


	filter_length	= max( (int)ceil(filter_size/factor), 1);
	int fb_size		= filter_length * (phase_count+1);

	filter_bank		= (FELEM*)malloc(fb_size*sizeof(FELEM));
	memset(filter_bank, 0, fb_size*sizeof(FELEM));

	// build filter
	build_filter(filter_bank, factor, filter_length, phase_count, 1<<FILTER_SHIFT, 1);
	memcpy(&filter_bank[(filter_length*phase_count)+1], filter_bank, (filter_length-1)*sizeof(FELEM));
	filter_bank[filter_length*phase_count]= filter_bank[filter_length - 1];

	src_incr		= output_rate;
	ideal_dst_incr	= dst_incr = input_rate*phase_count;
	index			= -phase_count*((filter_length-1)/2);

	return 0;
}


int do_resample(Resample *c, short *dst, short *src, int *consumed, int src_size, int dst_size, int update_ctx)
{
	int dst_index, i;
	int index			= c->index;
	int frac			= c->frac;
	int dst_incr_frac	= c->dst_incr % c->src_incr;
	int dst_incr		= c->dst_incr / c->src_incr;
	int compensation_distance = c->compensation_distance;

	if (compensation_distance == 0 && c->filter_length == 1 && c->phase_shift==0) {
		int64 index2= ((int64)index)<<32;
		int64 incr= (((int64)1)<<32) * c->dst_incr / c->src_incr;
		dst_size= FFMIN(dst_size, (src_size-1-index) * (int64)c->src_incr / c->dst_incr);

		for (dst_index=0; dst_index < dst_size; dst_index++) {
			dst[dst_index] = src[index2>>32];
			index2 += incr;
		}

		frac += dst_index * dst_incr_frac;
		index += dst_index * dst_incr;
		index += frac / c->src_incr;
		frac %= c->src_incr;
	} else {
		for (dst_index=0; dst_index < dst_size; dst_index++) {
			FELEM *filter= c->filter_bank + c->filter_length*(index & c->phase_mask);
			int sample_index= index >> c->phase_shift;
			FELEM2 val=0;

			if (sample_index < 0) {
				for (i=0; i<c->filter_length; i++) val += src[abs(sample_index + i) % src_size] * filter[i];
			} else 
			if (sample_index + c->filter_length > src_size) { break; } else 
			if (c->linear) {
				int64 v=0;
				int sub_phase= (frac<<8) / c->src_incr;
				for (i=0; i<c->filter_length; i++) {
					int64 coeff = filter[i]*(256 - sub_phase) + filter[i + c->filter_length]*sub_phase;
					v += src[sample_index + i] * coeff;
				}
				val= v>>8;
			} else {
				for (i=0; i<c->filter_length; i++) {
					val += src[sample_index + i] * (FELEM2)filter[i];
				}
			}

			val = (val + (1<<(FILTER_SHIFT-1)))>>FILTER_SHIFT;
			dst[dst_index] = (unsigned)(val + 32768) > 65535 ? (val>>31) ^ 32767 : val;

			frac += dst_incr_frac;
			index += dst_incr;
			if (frac >= c->src_incr) {
				frac -= c->src_incr;
				index++;
			}

			if (dst_index + 1 == compensation_distance) {
				compensation_distance = 0;
				dst_incr_frac = c->ideal_dst_incr % c->src_incr;
				dst_incr =      c->ideal_dst_incr / c->src_incr;
			}
		}
	}
	*consumed = MAX(index, 0) >> c->phase_shift;
	if (index>=0) index &= c->phase_mask;

	if (compensation_distance) compensation_distance -= dst_index;
	if (update_ctx) {
		c->frac = frac;
		c->index = index;
		c->dst_incr = dst_incr_frac + c->src_incr*dst_incr;
		c->compensation_distance= compensation_distance;
	}
	return dst_index;
}


// resamplovanie
int Resample::Process(int16 *output[6], int16 *input[6], int nb_samples)
{
	int16	*bufin[6];
	int16	*bufout[6];
	int16	*buftmp2[6];
	int		nb_samples1, i, lenout;

	// ak neni co robit...
	if (ratio == 1.0) {
		// len skopirujeme
		for (i=0; i<filter_channels; i++) {
			memcpy(output[i], input[i], nb_samples * sizeof(int16) );
		}
		return nb_samples;
	}

	for (i=0; i<filter_channels; i++) {
		bufout[i]	= output[i];
		bufin[i]	= (int16*)malloc( (nb_samples + temp_len) * sizeof(int16) );
		buftmp2[i]	= bufin[i] + temp_len;

		// natlacime tam data
		memcpy(bufin[i],   temp[i],  temp_len   * sizeof(int16) );
		memcpy(buftmp2[i], input[i], nb_samples * sizeof(int16) );
	}
	lenout = (int)(nb_samples * ratio) + 16;
	nb_samples += temp_len;

	// resamplujeme kazdy kanal
	nb_samples1 = 0; 
	for (i=0; i<filter_channels; i++) {
		int consumed=0;
		int is_last = (i+1 == filter_channels);

		// resamplujeme
		nb_samples1 = do_resample(this, bufout[i], bufin[i], &consumed, nb_samples, lenout, is_last);
		temp_len = nb_samples - consumed;
		temp[i] = (int16*)realloc(temp[i], temp_len*sizeof(int16));
		memcpy(temp[i], bufin[i] + consumed, temp_len*sizeof(int16));
	}

	for (i=0; i<filter_channels; i++) {
		free(bufin[i]);
	}
	return nb_samples1;
}

void Resample::Compensate(int sample_delta, int comp_distance)
{
	compensation_distance = comp_distance;
	dst_incr = ideal_dst_incr - ideal_dst_incr * (int64)sample_delta / comp_distance;
}



