#if !defined DISPLAY_H
#define DISPLAY_H

#include "active.h"
#include "fft.h"
#include "smartptrs.h"

class ViewWave;
class ViewFreq;
class ViewSpec;
class EditReadOnly;

class Painter: public ActiveObject
{
public:
    Painter (
        HWND hwnd,
        ViewWave& viewWave,
        ViewSpec& viewSpec,
        ViewFreq& viewFreq,
		EditReadOnly& dispFreq,
		EditReadOnly& dispNote,
        int samplesPerBuf, 
        int samplesPerSec,
        int fftPoints);

    bool ReInit (
        int samplesPerBuf, 
        int samplesPerSec, 
        int fftPoints,
        int bitsPerSample);

    bool Start ();
    void Stop ();

    int HzToPoint (int hz)
    { 
        Lock lock (_mutex);
        return _pFftTransformer->HzToPoint (hz); 
    }

    int Points () 
    { 
        Lock lock (_mutex);
        return _pFftTransformer->Points (); 
    }

private:
    void InitThread () {}
    void Run ();
    void FlushThread ();

    void LokWaveInData ();

    ViewWave   &_viewWave;
    ViewSpec   &_viewSpec;
    ViewFreq   &_viewFreq;
	EditReadOnly	&_dispFreq;
	EditReadOnly	&_dispNote;

    int         _samplesPerBuf;
    int         _samplesPerSecond;
    int         _fftPoints;
    int         _bitsPerSample;

    HWND        _hwnd;

    Mutex       _mutex;
    Event       _event;

    PtrRecorder _pRecorder;
    PtrFft      _pFftTransformer;
};

#endif
