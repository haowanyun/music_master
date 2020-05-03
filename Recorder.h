#if !defined RECORDER_H
#define RECORDER_H

#include "wavein.h"
#include "wassert.h"

class Event;

class Recorder
{
    friend class SampleIter;
    enum { NUM_BUF = 8 };
public:
    Recorder(
        int cSamples, 
        int cSamplePerSec, 
        int nChannels, 
        int bitsPerSecond);

    ~Recorder();
    bool    Start (Event& event);
    void    Stop ();
    DWORD   GetSamplePos () 
    { 
        return _waveInDevice.GetPosSample(); 
    }

    bool    BufferDone ();

    bool    IsBufferDone () const 
    { 
        return _header [_iBuf].IsDone (); 
    }

    bool    IsStarted () const { return _isStarted; }
    int     SampleCount () const { return _cSamples; }
    int     BitsPerSample () const { return _bitsPerSample; }
    int     SamplesPerSecond () const { return _cSamplePerSec; }
protected:
    virtual int GetSample (char *pBuf, int i) const = 0;
    char * GetData () const { return _header [_iBuf].lpData; }

    bool            _isStarted;

    WaveInDevice    _waveInDevice;
    int             _cSamplePerSec;     // sampling frequency
    int             _cSamples;          // samples per buffer
    int             _nChannels;
    int             _bitsPerSample;
    int             _cbSampleSize;      // bytes per sample

    int             _cbBuf;             // bytes per buffer
    int             _iBuf;              // current buffer #
    char		   *_pBuf;              // pool of buffers
    WaveHeader      _header [NUM_BUF];  // pool of headers
};

class RecorderM8: public Recorder  // 8 bit mono
{
public:
    RecorderM8 (int cSamples, int cSamplesPerSec)
    : Recorder (cSamples, cSamplesPerSec, 1, 8) {}
protected:
    int GetSample (char *pBuf, int i) const
    {
        return ((unsigned char)pBuf[i] - 128) * 64;
    }
};

class RecorderM16: public Recorder  // 16 bit mono
{
public:
    RecorderM16 (int cSamples, int cSamplesPerSec)
    : Recorder (cSamples, cSamplesPerSec,  1, 16) {}
protected:
    int GetSample (char *pBuf, int i) const
    {
        return ((short *) pBuf)[i];
    }
};


// for 8 bit stereo return (pBuf[2*i] + pBuf[2*i+1] - 2 * 128) * 128;
// for 16 bit stereo 
// return ( ((short *) pBuf)[2*i] +  ((short *) pBuf)[2*i+1] ) / 2

// This iterator works for any Recorder

class SampleIter
{
public:
    SampleIter (Recorder const &recorder);
    bool AtEnd () const { return _iCur == _iEnd;}
    void Advance () { _iCur++; }
    void Rewind () { _iCur = _iEnd - _recorder.SampleCount(); }
    int  GetSample () const { return _recorder.GetSample(_pBuffer, _iCur);}
    int  Count () const { return _recorder.SampleCount(); }
private:
    char	   *_pBuffer;
    Recorder const &_recorder;
    int         _iCur;
    int         _iEnd;
};

#endif
