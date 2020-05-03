//------------------------------------
//  fft.cpp
//  The implementation of the 
//  Fast Fourier Transform algorithm
//  (c) Reliable Software, 1996
//------------------------------------
#include "fft.h"
#include "recorder.h"

// log (1) = 0, log(2) = 1, log(3) = 2, log(4) = 2 ...

#define PI (2.0 * asin(1.0))			// sin(PI/2) = 1

// Points must be a power of 2

Fft::Fft (int Points, long sampleRate)
:	_Points (Points),					// Audio buffer size
	_sampleRate (sampleRate)			// Samples per second
{
    int i;
	_aTape = new double [_Points];		// Audio data buffer
	_aIntensity = new int [_Points];	// Audio specturm intensity
	_aPeak = new int [_Points];			// Audio specturm peaks

#ifdef CALIB_PROGRAM
    // 1 kHz calibration wave
    for (i = 0; i < _Points; i++)
        _aTape[i] = 16384/*1600*/ * sin (2 * PI * 1000. * i / _sampleRate);
#else
    for (i = 0; i < _Points; i++)
        _aTape[i] = 0;
#endif

    _sqrtPoints = sqrt((double)_Points);
    
	// calculate binary log
    _logPoints = 0;
    Points--;
    while (Points != 0)
    {
        Points >>= 1;
        _logPoints++;
    }

    _aBitRev = new int [_Points];
    _X = new Complex[_Points];
    _W = new Complex* [_logPoints+1];
    
	// Precompute complex exponentials
    for (int l = 1, _2_l = 2; l <= _logPoints; l++)
    {
        _W[l] = new Complex [_Points];

        for (i = 0; i < _Points; i++ )
        {
            double re =  cos (2. * PI * i / _2_l);
            double im = -sin (2. * PI * i / _2_l);
            _W[l][i] = Complex (re, im);
        }
        _2_l *= 2;
    }

    // set up bit reverse mapping
    int rev = 0;
    int halfPoints = _Points/2;
    for (i = 0; i < _Points - 1; i++)
    {
        _aBitRev[i] = rev;
        int mask = halfPoints;
        // add 1 backwards
        while (rev >= mask)
        {
            rev -= mask; // turn off this bit
            mask >>= 1;
        }
        rev += mask;
    }
    _aBitRev [_Points-1] = _Points-1;
}

Fft::~Fft()
{
    delete []_aTape;
    delete []_aIntensity;
    delete []_aPeak;
    delete []_aBitRev;
    for (int l = 1; l <= _logPoints; l++)
    {
        delete []_W[l];
    }
    delete []_W;
    delete []_X;
}

void Fft::CopyIn (SampleIter& iter)
{
    int i, cSample = iter.Count();
    if (cSample > _Points)
        return;

#ifndef CALIB_PROGRAM
    // make space for cSample samples at the end of tape
    // shifting previous samples towards the beginning
    memmove (_aTape, &_aTape[cSample], 
              (_Points - cSample) * sizeof(double));
    // copy samples from iterator to tail end of tape
    int iTail  = _Points - cSample;
    for (i = 0; i < cSample; i++, iter.Advance())
    {
        _aTape [i + iTail] = (double) iter.GetSample();
    }
#endif

    // Initialize the FFT buffer
    for (i = 0; i < _Points; i++)
        PutAt (i, _aTape[i]);		// _X [_aBitRev[i]] = Complex (_aTape[i]);
}

//
//               0   1   2   3   4   5   6   7
//  level   1
//  step    1                                     0
//  increm  2                                   W 
//  j = 0        <--->   <--->   <--->   <--->   1
//  level   2
//  step    2
//  increm  4                                     0
//  j = 0        <------->       <------->      W      1
//  j = 1            <------->       <------->   2   W
//  level   3                                         2
//  step    4
//  increm  8                                     0
//  j = 0        <--------------->              W      1
//  j = 1            <--------------->           3   W      2
//  j = 2                <--------------->            3   W      3
//  j = 3                    <--------------->             3   W
//                                                              3
//

void Fft::Transform ()
{
    // step = 2 ^ (level-1)
    // increm = 2 ^ level;
    for (int step = 1, level = 1; level <= _logPoints; level++)
    {
        int increm = step * 2;
        for (int j = 0; j < step; j++)
        {
            // U = exp ( - 2 PI j / 2 ^ level )
            Complex U = _W [level][j];
            for (int i = j; i < _Points; i += increm)
            {
                // butterfly
                Complex T = U;
                T *= _X [i+step];
                _X [i+step] = _X[i];
                _X [i+step] -= T;
                _X [i] += T;
            }
        }
        step *= 2;
    }
}

