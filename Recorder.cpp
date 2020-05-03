#include "recorder.h"
#include "wassert.h"

// Call BufferDone after creating the iterator
SampleIter::SampleIter (Recorder const &recorder)
: _recorder (recorder), _iCur(0)
{
    _pBuffer = recorder.GetData ();
    _iEnd = recorder.SampleCount();
}

Recorder::Recorder (
    int cSamples, 
    int cSamplePerSec, 
    int nChannels, 
    int bitsPerSample)
: _iBuf(0),
  _cSamplePerSec (cSamplePerSec), 
  _cSamples (cSamples), 
  _cbSampleSize (nChannels * bitsPerSample/8), 
  _cbBuf (cSamples * nChannels * bitsPerSample/8),
  _nChannels (nChannels),
  _bitsPerSample (bitsPerSample),
  _isStarted (false)
{
    _pBuf = new char [_cbBuf * NUM_BUF];
}

Recorder::~Recorder()
{
    Stop();
    delete []_pBuf;
}

bool Recorder::Start (Event& event)
{
    WaveFormat format ( 
        _nChannels,      
        _cSamplePerSec,
        _bitsPerSample );

    if (!format.isInSupported(0))
    {
        MessageBox (0, "Format not supported", "Recorder", MB_OK);
        return false;
    }

    _waveInDevice.Open (0, format, event);
    if (!_waveInDevice.Ok())
    {
        char buf[164];
        if (_waveInDevice.isInUse())
            strcpy (buf, "Another application is recording audio. Stop recording with this other application and then try to record with Sound Frequency Analyzer again.");
        else
            _waveInDevice.GetErrorText (buf, sizeof (buf));
        MessageBox (0, buf, "Recorder", MB_OK);
        return false;
    }

    // Don't initialize the last buffer
    // It will be initialized in the
    // first call to BufferDone
    for ( int i = 0; i < NUM_BUF - 1; i++ )
    {
        _header[i].lpData = &_pBuf [i * _cbBuf];
        _header[i].dwBufferLength = _cbBuf;
        _header[i].dwFlags = 0;
        _header[i].dwLoops = 0;

        _waveInDevice.Prepare (&_header[i]);

        _waveInDevice.SendBuffer (&_header [i]);
    }
    _isStarted = true;
    _iBuf = 0;
    _waveInDevice.Start();
    return true;
}

bool Recorder::BufferDone ()
{
    Assert (IsBufferDone ());
    _waveInDevice.UnPrepare (&_header [_iBuf]);
    int prevBuf = _iBuf - 1;
    if (prevBuf < 0)
        prevBuf = NUM_BUF - 1;

    // Next buffer to be filled
    _iBuf++;
    if ( _iBuf == NUM_BUF )
        _iBuf = 0;

    _header[prevBuf].lpData = &_pBuf [prevBuf * _cbBuf];
    _header[prevBuf].dwBufferLength = _cbBuf;
    _header[prevBuf].dwFlags = 0;
    _header[prevBuf].dwLoops = 0;
    _waveInDevice.Prepare (&_header [prevBuf]);

    _waveInDevice.SendBuffer (&_header [prevBuf]);
    return true;
}

void Recorder::Stop()
{
    _isStarted = false;
    _waveInDevice.Reset ();
    _waveInDevice.Close ();
}

