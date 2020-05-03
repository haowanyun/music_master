#include "views.h"
#include "canvas.h"
#include "fft.h"
#include "recorder.h"

#define	MIN_PEAK	8192			// Spectrum intensity higher than this: energy density confirmed

COLORREF MapColor (int s)			// Convert spectrum intensity to colored temperature
{
    if ( s < 8 )
        return RGB(128,	255, 255);	// Light Cyan
    else if ( s < 16)
        return RGB(0,	255, 255);	// Cyan
    else if ( s < 32 )
        return RGB(0,	255, 0);	// Green
    else if ( s < 64 )
        return RGB(128, 255, 0);	// Yellow Green
    else if ( s < 128)
        return RGB(255, 255, 0);    // Yellow
    else if ( s < 256 )
        return RGB(255, 128, 0);    // Orange
    else
        return RGB(255, 0, 0);      // Red
}

void ViewFreq::Clear ()
{
    if (Hwnd ())
    {
        UpdateCanvas canvas (Hwnd ());
        ClientRect rect (Hwnd ());
        canvas.ClearWhite(rect);
    }
}

void ViewFreq::Update (Fft const &fftTransformer)
{
    UpdateCanvas canvas (Hwnd ());
    ClientRect rect (Hwnd ());

    {
        // Erase background for current spectrum
        WhitePen pen(canvas);
        canvas.Line (_xRecord, 0, _xRecord, rect.bottom);
        canvas.Line (_xRecord + 1, 0, _xRecord + 1, rect.bottom);
    }

    for (int i = 0; 
             i < fftTransformer.Points() / 2 && i < rect.bottom; 
             i++ )
    {
        int s = int (fftTransformer.GetIntensity(i) / 256);
        
        COLORREF color;
        if (s > 8)
        {
            color = MapColor (s);

            canvas.Point (_xRecord, rect.bottom - i - 1, color);
            canvas.Point (_xRecord + 1, rect.bottom - i - 1, color);
        }
    }

    _xRecord += 2;
    if (_xRecord >= rect.right)
        _xRecord = 0;

    {
        // Draw white vertical mark
        BlackPen pen(canvas);
        canvas.Line (_xRecord, 0, _xRecord, rect.bottom);
    }
}

void ViewFreq::Fake ()
{
    UpdateCanvas canvas (Hwnd ());
    ClientRect rect (Hwnd ());

    {
        // Erase background for current spectrum
        WhitePen pen(canvas);
        canvas.Line (_xRecord, 0, _xRecord, rect.bottom);
        canvas.Line (_xRecord + 1, 0, _xRecord + 1, rect.bottom);
    }


    _xRecord += 2;
    if (_xRecord >= rect.right)
        _xRecord = 0;

    {
        // Draw vertical mark: current update
        BlackPen pen(canvas);
        canvas.Line (_xRecord, 0, _xRecord, rect.bottom);
    }
}

void ViewWave::Update (Fft const &fftTransformer)
{
    UpdateCanvas canvas (Hwnd ());
    ClientRect rect (Hwnd ());
    canvas.ClearWhite(rect);
    int cMaxPoints = min (fftTransformer.Points(), _poly.Points());
    for ( int i = 0; i < cMaxPoints; i++ )
    {
        int x, y = fftTransformer.Tape(i) / 512 + (rect.bottom - 1) / 2;
		
        if (i >= rect.right)
			x = rect.right - 1;
        else
			x = i;

        if ( y < 0 )
			y = 0;
		else if (y >= rect.bottom)
			y = rect.bottom - 1;
		
		_poly.Add (i, x, y);				
    }

    PenHolder pen (canvas, _penBlack);	
    _poly.Paint (canvas, cMaxPoints);	
}

double ViewSpec::Update(Fft const& fftTransformer)
{
    UpdateCanvas canvas(Hwnd());
    ClientRect rect(Hwnd());
    canvas.ClearWhite(rect);

    int i, cMaxPoints = min(fftTransformer.Points() / 2, _poly.Points()), peak = 0;
    for (i = 0; i < cMaxPoints; i++)
	{
		int y = (int) fftTransformer.GetIntensity(i);
		fftTransformer._aIntensity[i] = y;
		if(peak < y) peak = y;
	}

    int freq = 0;
	for (i = 0; i < cMaxPoints; i++)
    {
        int x, y = fftTransformer._aIntensity[i];
		int j = i * 5 / 7, bandLimit = i * 7 / 5;

		for(; j < bandLimit; j++)
		{
			if(y < fftTransformer._aIntensity[j])		// Not peak in the frequency band
				break;									// Stop comparing in the range
		}
		
		if (i < rect.right)
			x = i;
		else
			x = rect.right - 1;
		
		if(y >= MIN_PEAK && j >= bandLimit)				// A peak value found
		{
			fftTransformer._aPeak[i] = y;
			if(y >= peak)
			{
				freq = i;
				_spectrum.Add(i, x, rect.top + 30);		// Major peak
			}
			else _spectrum.Add(i, x, rect.top + 15);	// Minor peak
		}
		else
			_spectrum.Add(i, x, rect.top);				// Not a peak

		y >>= 9;										// y /= 512
		if (y < 0)										// Limit spectrum 
			y = 0;
		else if (y >= rect.bottom)
			y = rect.bottom - 1;

		y++;
		y = rect.bottom - y;
		_poly.Add(i, x, y);

    }

    PenHolder penSpec(canvas, _penBlue);
    _poly.Paint(canvas, cMaxPoints);
    PenHolder penPeak(canvas, _penRed);
    _spectrum.Paint(canvas, cMaxPoints);

	return (double (freq) * fftTransformer.MaxFreq() / fftTransformer.Points());
}
