#if !defined SMART_PTRS_H
#define SMART_PTRS_H
//------------------------------------
//  smartptrs.h
//  Smart pointers to objects
//  (c) Bartosz Milewski, 1996
//------------------------------------

#include "recorder.h"
#include "fft.h"
#include "xptr.h"

class PtrRecorder : public XPtr<Recorder>
{
public:
    PtrRecorder (int cSamples, 
                 int cSamplePerSec)
        : XPtr<Recorder>(new RecorderM16 (cSamples, cSamplePerSec)) 
    {}

    void ReInit (int bitsPerSample,
                 int cSamples, 
                 int cSamplePerSec);
};

class PtrFft : public XPtr<Fft>
{
public:
    PtrFft (int points, long sampleRate)
        : XPtr<Fft> (new Fft (points, sampleRate))
    {}

    void ReInit (int points, long sampleRate);
};

#endif