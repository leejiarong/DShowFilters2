//-----------------------------------------------------------------------------
//
//	MONOGRAM AMR Filter Pack
//
//	Author : Igor Janos
//
//-----------------------------------------------------------------------------

#include <afxwin.h>
#include <afxtempl.h>
#include <atlbase.h>

#include <streams.h>
#include <initguid.h>

#include <mmsystem.h>
#include <mmreg.h>
#include <ks.h>
#include <ksmedia.h>
#include <dvdmedia.h>

#define _MATH_DEFINES
#include <math.h>

#include <windowsx.h>

#include "..\resource.h"

/*
#include "neaacdec.h"
#include "bits.h"
*/

#include "amr_types.h"
#include "amr_file.h"
#include "amr_pin.h"

#include "resample.h"
#include "libamr_nb.h"

#include "amr_demux.h"
#include "amr_demux_page.h"
#include "amr_mux.h"
#include "amr_mux_page.h"
#include "amr_encoder.h"
#include "amr_encoder_page.h"
#include "amr_decoder.h"
#include "amr_decoder_page.h"

/*
#include "aac_classes.h"
#include "aac_latm.h"
#include "audio_utils.h"
#include "audio_dsp.h"
#include "aac_filter.h"
#include "aac_prop.h"
*/