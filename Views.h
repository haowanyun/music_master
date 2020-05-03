#if !defined VIEWS_H
#define VIEWS_H

#include "controls.h"
#include "canvas.h"

class Fft;
class SampleIter;

class ViewFreq: public SimpleControl
{
public:
    ViewFreq (HWND hwndParent, int id) 
        : SimpleControl (hwndParent, id),
          _xRecord(0) {}
    void Update (Fft const & fftTransformer);
    void Clear ();
    void Fake ();
private:
    int         _xRecord;
};

class ViewWave: public SimpleControl
{
public:
    ViewWave (HWND hwndParent, int id, int cSamples) 
        : SimpleControl (hwndParent, id),
          _poly (cSamples),		  
          _penBlack (RGB(0, 0, 0))		  
    {}
    void Update (Fft const & fftTransformer);
private:
    PolyLine    _poly;
    Pen         _penBlack;	
};

class ViewSpec : public SimpleControl
{
public:
    ViewSpec(HWND hwndParent, int id, int cSamples)
        : SimpleControl(hwndParent, id),
        _poly(cSamples),
        _spectrum(cSamples),
        _penBlue (RGB(0, 0, 128)),
        _penRed (RGB(255, 0, 0))
    {}
    double Update(Fft const& fftTransformer);
private:
    PolyLine    _poly, _spectrum;
    Pen         _penBlue, _penRed;
};

#endif