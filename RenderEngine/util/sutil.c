
/*
* Copyright (c) 2008 - 2009 NVIDIA Corporation.  All rights reserved.
*
* NVIDIA Corporation and its licensors retain all intellectual property and proprietary
* rights in and to this software, related documentation and any modifications thereto.
* Any use, reproduction, disclosure or distribution of this software and related
* documentation without an express license agreement from NVIDIA Corporation is strictly
* prohibited.
*
* TO THE MAXIMUM EXTENT PERMITTED BY APPLICABLE LAW, THIS SOFTWARE IS PROVIDED *AS IS*
* AND NVIDIA AND ITS SUPPLIERS DISCLAIM ALL WARRANTIES, EITHER EXPRESS OR IMPLIED,
* INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE.  IN NO EVENT SHALL NVIDIA OR ITS SUPPLIERS BE LIABLE FOR ANY
* SPECIAL, INCIDENTAL, INDIRECT, OR CONSEQUENTIAL DAMAGES WHATSOEVER (INCLUDING, WITHOUT
* LIMITATION, DAMAGES FOR LOSS OF BUSINESS PROFITS, BUSINESS INTERRUPTION, LOSS OF
* BUSINESS INFORMATION, OR ANY OTHER PECUNIARY LOSS) ARISING OUT OF THE USE OF OR
* INABILITY TO USE THIS SOFTWARE, EVEN IF NVIDIA HAS BEEN ADVISED OF THE POSSIBILITY OF
* SUCH DAMAGES
*/

#include "sutil.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#if defined(_WIN32)
#    define WIN32_LEAN_AND_MEAN
#    include<windows.h>
#    include<mmsystem.h>
#else /*Apple and Linux both use this */
#    include<sys/time.h>
#    include <unistd.h>
#    include <dirent.h>
#endif

void sutilReportError(const char* message)
{
	fprintf( stderr, "OptiX Error: %s\n", message );
	//#if defined(_WIN32) && defined(RELEASE_PUBLIC)
	{
		char s[2048];
		sprintf( s, "OptiX Error: %s", message );
		MessageBox( 0, s, "OptiX Error", MB_OK|MB_ICONWARNING|MB_SYSTEMMODAL );
	}
	//#endif
}

#if defined(_WIN32)

// inv_freq is 1 over the number of ticks per second.
static double inv_freq;
static int freq_initialized = 0;
static int use_high_res_timer = 0;

double sutilCurrentTime()
{
	if(!freq_initialized) {
		LARGE_INTEGER freq;
		use_high_res_timer = QueryPerformanceFrequency(&freq);
		inv_freq = 1.0/freq.QuadPart;
		freq_initialized = 1;
	}

	if (use_high_res_timer) {
		LARGE_INTEGER c_time;
		if(QueryPerformanceCounter(&c_time)) {
			return c_time.QuadPart*inv_freq;
		} else {
			return -1;
		}
	} else {
		return ( (double)timeGetTime() ) * 1.0e-3;
	}
}

#else

RTresult sutilCurrentTime( double* current_time )
{
	struct timeval tv;
	if( gettimeofday( &tv, 0 ) ) {
		fprintf( stderr, "sutilCurrentTime(): gettimeofday failed!\n" );
		return RT_ERROR_UNKNOWN;
	}

	*current_time = tv.tv_sec+ tv.tv_usec * 1.0e-6;
	return RT_SUCCESS;
}

#endif

