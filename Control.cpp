#include "control.h"
#include "resource.h"
#include "display.h"
#include "winex.h"
#include "params.h"
#include "views.h"
#include <string.h>

// Helper functions

bool Is16BitSampling (DWORD format);
bool Is11_025kHz (DWORD format);
bool Is22_05kHz (DWORD format);
bool Is44_1kHz (DWORD format);

// Main dialog handler

BOOL CALLBACK DialogProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static Controller* pControl = 0;
    switch (message)
    {
    case WM_INITDIALOG:
        try
        {
            pControl = new Controller (hwnd);
        }
        catch (WinException e)
        {
            MessageBox (0, e.GetMessage (), "Exception", MB_ICONEXCLAMATION | MB_OK);
			return -1;
        }
        catch (...)
        {
            MessageBox (0, "Unknown", "Exception", MB_ICONEXCLAMATION | MB_OK);
            return -1;
        }
        return TRUE;        
    case WM_COMMAND:
        pControl->Command(hwnd, LOWORD (wParam), HIWORD (wParam));
        return TRUE;
    case WM_HSCROLL:
        pControl->Scroll (hwnd, LOWORD (wParam), HIWORD (wParam));
        return 0;
    case WM_CLOSE:
        delete pControl;
        DestroyWindow (hwnd);
        return TRUE;
    }
    return FALSE;
}

BOOL CALLBACK AboutDlgProc
   (HWND hwnd, UINT message, UINT wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD (wParam))
        {
        case IDOK:
        case IDCANCEL:
            EndDialog (hwnd, 0);
            return TRUE;
		case IDC_WWW:
			ShellExecute (hwnd, "open", "https://gitee.com/digital_twin/music_master", 0, 0, SW_SHOWNORMAL);			
			return TRUE;
        }
        break;
    }
    return FALSE;
}

BOOL CALLBACK OptionsDlgProc
   (HWND hwnd, UINT message, UINT wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD (wParam))
        {
        case IDOK:
        case IDCANCEL:
            EndDialog (hwnd, 0);
            return TRUE;
		case IDC_WWW:
			ShellExecute (hwnd, "open", "https://gitee.com/digital_twin/music_master", 0, 0, SW_SHOWNORMAL);			
			return TRUE;
        }
        break;
    }
    return FALSE;
}

Controller::Controller (HWND hwnd) 
   :_isStopped (true),
    _bitsPerSample (16),
    _samplesPerSecond (SAMPLES_SEC),
    _fftPoints (FFT_POINTS * 4),
    _samplesPerBuf (FFT_POINTS * 2),
    _radio8 (hwnd, IDC_8_BITS),
    _radio16 (hwnd, IDC_16_BITS),
    _scroll (hwnd, IDC_SCROLLBAR),
    _edit (hwnd, IDC_EDIT),
	_dispFreq (hwnd, IDC_CENTER_FREQ),
	_dispNote (hwnd, IDC_BASE_NOTE),
    _comboFreq (hwnd, IDC_SAMPLING),
    _comboPoints (hwnd, IDC_POINTS),
    _viewWave (hwnd, IDS_WAVE_PANE, FFT_POINTS * 8),
    _viewSpec (hwnd, IDS_SPEC_PANE, FFT_POINTS * 8),
    _viewFreq (hwnd, IDS_FREQ_PANE),
    _scaleFreq (hwnd, IDC_FREQ_SCALE),
    _display (hwnd, 
            _viewWave, 
            _viewSpec,
            _viewFreq,
			_dispFreq,
			_dispNote,
            _samplesPerBuf,
            _samplesPerSecond,
            _fftPoints)
{
    if (waveInGetNumDevs() == 0)
        throw WinException ("No sound card installed !");

    WAVEINCAPS waveInCaps;
    if (waveInGetDevCaps (0, &waveInCaps, sizeof(WAVEINCAPS)) != MMSYSERR_NOERROR)
        throw WinException ("Cannot determine sound card capabilities !");

    // Attach icon to main dialog
    HICON hIcon = LoadIcon (TheInstance, MAKEINTRESOURCE (ICO_FFT));
    SendMessage (hwnd, WM_SETICON, WPARAM (ICON_SMALL), LPARAM (hIcon));

    // Display RS icon over the button
    hIcon = LoadIcon (TheInstance, MAKEINTRESOURCE (ICO_RS));
    HWND hwndButton = GetDlgItem (hwnd, IDC_RELIBUTTON);
    SendMessage (hwndButton, BM_SETIMAGE, (WPARAM)IMAGE_ICON,
           (LPARAM)(HANDLE)hIcon);
 
    // Initialize radio buttons (bits per sample)
    if (Is16BitSampling (waveInCaps.dwFormats))
    {
        _radio16.Check();
    }
    else
    {
        _radio8.Check();
        _bitsPerSample = 8;
        _radio16.Disable();
    }

    // Initialize scroll bar (fft's per second)
    InitScrollPositions ();
    // Initialize the combo box with sampling frequencies
    if (Is11_025kHz (waveInCaps.dwFormats))
        _comboFreq.AddValue (SAMPLES_SEC);       
    if (Is22_05kHz (waveInCaps.dwFormats))
        _comboFreq.AddValue (2 * SAMPLES_SEC);  
    if (Is44_1kHz (waveInCaps.dwFormats))
        _comboFreq.AddValue (4 * SAMPLES_SEC); 
    Assert (_samplesPerSecond == SAMPLES_SEC);
    // Select lowest available sampling frequency
    _comboFreq.SelectPos (0); 
    _samplesPerSecond = _comboFreq.GetValue();

    // Initialize combo with FFT points
    _comboPoints.AddValue (FFT_POINTS);         // 0
    _comboPoints.AddValue (2 * FFT_POINTS);     // 1
    _comboPoints.AddValue (4 * FFT_POINTS);     // 2
    _comboPoints.AddValue (8 * FFT_POINTS);     // 3
    Assert (_fftPoints == 4 * FFT_POINTS);
    _comboPoints.SelectPos (2);  // 4 * FFT_POINTS

    PaintScale ();
    _display.Resume ();
    PostMessage (hwnd, WM_COMMAND, IDB_START, 0);
}

Controller::~Controller ()
{
    _display.Kill ();
    PostQuitMessage (0);
}

// called when _samplesPerSec, _fftPoints or _bitsPerSample changed
void Controller::ReInit (HWND hwnd)
{
    if (!_display.ReInit (
            _samplesPerBuf,
            _samplesPerSecond,
            _fftPoints,
            _bitsPerSample))
    {
        PostQuitMessage (1);
    }

    PaintScale ();
}

void Controller::InitScrollPositions ()
{
    // This scrollbar controls fft's per second
    // by storing and returning samples per buffer
    _scroll.Clear ();
    // We perform one fft per buffer
    // so start with fftPoints large buffer
    // and shrink it down to MIN_SAMP_BUF
    int sampPerBuf = _fftPoints, i;
    for (i = 0; i < MAX_POS; i++)
    {
        _scroll.AddValue (sampPerBuf);
        sampPerBuf -= MIN_SAMP_BUF;
        if (sampPerBuf <= 0)
            break;
    }

    _scroll.Init (i + 1, 1);

    // let's start with one buffer per fft
    _samplesPerBuf = _fftPoints;
    _scroll.SetPos (0);
    // fft's per second
    _edit.SetNumber (_samplesPerSecond / _samplesPerBuf);
}

void Controller::Scroll (HWND hwnd, int code, int pos) 
{ 
    _scroll.Command (code, pos);
    _samplesPerBuf = _scroll.GetValue(); 
    Assert (_samplesPerBuf <= _fftPoints);
    Assert (_samplesPerBuf >= MIN_SAMP_BUF);
    if (!_display.ReInit (
            _samplesPerBuf,
            _samplesPerSecond,
            _fftPoints,
            _bitsPerSample))
    {
        PostQuitMessage (1);
    }
    _edit.SetNumber (_samplesPerSecond / _samplesPerBuf);
}

void Controller::Stop (HWND hwnd)
{
    _display.Stop ();
    EnableWindow (GetDlgItem(hwnd, IDB_STOP), FALSE);
    EnableWindow (GetDlgItem(hwnd, IDB_START), TRUE);
}

// Child control command processing

void Controller::Command ( HWND hwnd, int ctrlid, int code)
{
    int i = 0;
    switch (ctrlid)
    {
    case IDC_RELIBUTTON:
        DialogBox (TheInstance, MAKEINTRESOURCE (DLG_ABOUT), hwnd, DLGPROC(AboutDlgProc));
        break;
    case IDC_OPTIONSBUTTON:
        DialogBox (TheInstance, MAKEINTRESOURCE (DLG_OPTIONS), hwnd, DLGPROC(OptionsDlgProc));
        break;
    case IDB_START:
        if (_isStopped)
        {
            _isStopped = !_display.Start();
            if (_isStopped)
            {
                SendMessage (GetDlgItem (hwnd, IDB_START), WM_SETTEXT, 
                             0, (LPARAM)"Start");
            }
            else
            {
                SendMessage (GetDlgItem (hwnd, IDB_START), WM_SETTEXT, 
                             0, (LPARAM)"Stop");
            }
        }
        else
        {
            Stop (hwnd);
            SendMessage (GetDlgItem (hwnd, IDB_START), WM_SETTEXT, 
                         0, (LPARAM)"Start");
            _isStopped = true;
        }
        break;
    case IDB_STOP:
        break;
    case IDC_8_BITS:
        if (_radio8.IsClicked (code))
        {
            _bitsPerSample = 8;
            ReInit (hwnd);
        }
        break;
    case IDC_16_BITS:
        if (_radio16.IsClicked (code))
        {
            _bitsPerSample = 16;
            ReInit (hwnd);
        }
        break;
    case IDC_SAMPLING:
        if (_comboFreq.IsChanged (code))
        {
            _samplesPerSecond = _comboFreq.GetValue ();
            InitScrollPositions ();
            ReInit (hwnd);
        }    
        break;
    case IDC_POINTS:
        if (_comboPoints.IsChanged(code))
        {
            _fftPoints = _comboPoints.GetValue ();
            InitScrollPositions ();
            ReInit (hwnd);
        }    
        break;
    }
}

void Controller::PaintScale ()
{
    // Get the rectangle (in pixels)
    RECT &rect = _scaleFreq.Rect ();
    // Get the reference canvas
    UpdateCanvas canvRef (_scaleFreq.Hwnd ());
    // Translate rectangle to hundredths of millimiters
    RectHmm rectHmm (canvRef, rect);
    // Create Enhanced Metafile control canvas
    CanvasEMFCtrl canvas (rectHmm, canvRef);

    canvas.ClearWhite (rect);
    int x0 = rect.left;
    int y0 = rect.bottom - 1;

    // draw the vertical line
//	canvas.Line (x0, rect.top, x0, rect.bottom);

    // draw a notch every 100 Hz
    int s1000 = _display.HzToPoint(1000);
    int s100 = s1000 / 10;
    int maxS = _display.Points() / 2;
    int count = 0;

    if ( s100 < 3)
    {
        for (int s = 0; s < maxS; s += s1000 )
        {
            int y = y0 - s;
            if (y <= 0)
                break;
            int c = 6;
            canvas.Line (x0, y, x0 + c, y);
            count++;
        }
    }
    else
    {
        for (int s = 0; s < maxS; s += s100)
        {
            int c = (count % 10) == 0? 6: 3;
            int y = y0 - s;
            if ( y <= 0 )
                break;
            canvas.Line (x0 - c, y, x0, y);
            count++;
        }
    }
    // Attach the Enhanced Metafile to the control
    _scaleFreq.Attach (canvas);
}

bool Is16BitSampling (DWORD format)
{
    return (format & WAVE_FORMAT_1M16) ||
           (format & WAVE_FORMAT_1S16) ||
           (format & WAVE_FORMAT_2M16) ||
           (format & WAVE_FORMAT_2S16) ||
           (format & WAVE_FORMAT_4M16) ||
           (format & WAVE_FORMAT_4S16);
}

bool Is11_025kHz (DWORD format)
{
    return (format & WAVE_FORMAT_1M08) ||
           (format & WAVE_FORMAT_1M16) ||
           (format & WAVE_FORMAT_1S08) ||
           (format & WAVE_FORMAT_1S16);
}

bool Is22_05kHz (DWORD format)
{
    return (format & WAVE_FORMAT_2M08) ||
           (format & WAVE_FORMAT_2M16) ||
           (format & WAVE_FORMAT_2S08) ||
           (format & WAVE_FORMAT_2S16);
}

bool Is44_1kHz (DWORD format)
{
    return (format & WAVE_FORMAT_4M08) ||
           (format & WAVE_FORMAT_4M16) ||
           (format & WAVE_FORMAT_4S08) ||
           (format & WAVE_FORMAT_4S16);
}
