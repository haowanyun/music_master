//------------------------------------
//  SmartPtrs.cpp
//  Smart pointers to objects
//  (c) Bartosz Milewski, 1996
//------------------------------------

#include "smartptrs.h"

void PtrRecorder::ReInit (int bitsPerSample,
                          int cSamples, 
                          int cSamplePerSec)
{
    delete _p;
    _p = 0;
    if (bitsPerSample == 8)
    {
        _p = new RecorderM8 (cSamples, cSamplePerSec);
    }
    else
    {
	    Assert (bitsPerSample == 16);
        _p = new RecorderM16 (cSamples, cSamplePerSec);
     }
}

void PtrFft::ReInit(int points, long sampleRate)
{
    delete _p;
    _p = 0;
    _p = new Fft(points, sampleRate);
}