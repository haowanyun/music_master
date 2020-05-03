#include "display.h"
#include "views.h"
#include "canvas.h"
#include "recorder.h"
#include "fft.h"
#include "winex.h"
#include "wassert.h"

Painter::Painter (
        HWND hwnd,
        ViewWave& viewWave,
        ViewSpec& viewSpec,
        ViewFreq& viewFreq,
		EditReadOnly& dispFreq,
		EditReadOnly& dispNote,
        int samplesPerBuf, 
        int samplesPerSec, 
        int fftPoints)
:   _hwnd (hwnd), 
    _viewWave (viewWave),
    _viewSpec (viewSpec),
    _viewFreq (viewFreq),
	_dispFreq (dispFreq),
	_dispNote (dispNote),
    _samplesPerBuf (samplesPerBuf),
    _samplesPerSecond (samplesPerSec),
    _fftPoints (fftPoints),
    _bitsPerSample (16),
    _pFftTransformer (fftPoints, samplesPerSec),
    _pRecorder (samplesPerBuf, samplesPerSec)
{
    // Let the caller Resume explicitly
    //_thread.Resume ();
}

void Painter::Run ()
{
    for (;;)
    {
        _event.Wait ();
        if (_isDying)
            return;

        Lock lock (_mutex);
        if (_pRecorder->IsBufferDone ())
            LokWaveInData ();
    }
}

void Painter::FlushThread ()
{
    _event.Release ();
}

bool Painter::ReInit (
        int samplesPerBuf, 
        int samplesPerSec, 
        int fftPoints,
        int bitsPerSample)
{
    Lock lock (_mutex);

    if (_pRecorder->BitsPerSample() == bitsPerSample &&
        _pRecorder->SamplesPerSecond() == samplesPerSec &&
        _pFftTransformer->Points() == fftPoints &&
        _pRecorder->SampleCount() == samplesPerBuf)
    {
        return true;
    }

    _samplesPerBuf = samplesPerBuf;
    _samplesPerSecond = samplesPerSec;
    _fftPoints = fftPoints;
    _bitsPerSample = bitsPerSample;

    bool isStarted = _pRecorder->IsStarted();
    if (isStarted)
        _pRecorder->Stop();

    bool isError = false;

    try
    {
        _pFftTransformer.ReInit ( _fftPoints, _samplesPerSecond);
        _pRecorder.ReInit (_bitsPerSample, _samplesPerBuf, _samplesPerSecond);
    }
    catch ( WinException e )
    {
        char buf[50];
        wsprintf ( buf, "%s, Error %d", e.GetMessage(), e.GetError() );
        MessageBox (0, buf, "Exception", MB_ICONEXCLAMATION | MB_OK);
        isError = true;
    }
    catch (...)
    {
        MessageBox (0, "Unknown", "Exception", MB_ICONEXCLAMATION | MB_OK);
        isError = true;
    }

    if (isStarted)
    {
        isError = !_pRecorder->Start(_event);
    }

    return !isError;
}

bool Painter::Start ()
{
    Lock lock (_mutex);
    return _pRecorder->Start (_event);
}

void Painter::Stop ()
{
    Lock lock (_mutex);
    _pRecorder->Stop ();
}

void Painter::LokWaveInData ()
{
	SampleIter iter (_pRecorder.GetAccess());
    // Quickly release the buffer
    if (!_pRecorder->BufferDone ())
        return;

    _pFftTransformer->CopyIn (iter);
    _pFftTransformer->Transform();

    _viewFreq.Update (_pFftTransformer.GetAccess());
    _viewWave.Update (_pFftTransformer.GetAccess());

    double freq = _viewSpec.Update (_pFftTransformer.GetAccess());
	if(freq < 27.0 || freq > 24000.0)		// Frequency out of range
		return;								// Do not display frequency / note
	_dispFreq.SetDouble (freq);

	char note[9] = "A 0 + 0";
	const char notationChar[] = "AaBCcDdEFfGg";
	int ratio = log(freq / 27.5) * 1200.0 / log(2.0);

	if(ratio < -44)
		_dispNote.SetString (" < A0");
	else if(ratio > 8754)
		_dispNote.SetString (" > C8");
	else
	{
		if((ratio % 10) >= 5)
			ratio = ratio / 10 + 1;
		else
			ratio /= 10;

		int deviation = (ratio + 10) % 10;
		if(deviation <= 5)
		{
			ratio -= deviation;
			note[4] = '+';
			note[6] = '0' + deviation;
		}
		else
		{
			ratio += 10 - deviation;
			note[4] = '-';
			note[6] = '9' + 1 - deviation;
		}
		note[0] = notationChar[(ratio /10) % 12];
		if(islower(note[0]))
		{
			note[0] = toupper(note[0]);
			note[1] = '#';
		}
		else note[1] = ' ';
		note[2] = '0' + (ratio + 90) / 120;
		_dispNote.SetString (note);
	}
}


