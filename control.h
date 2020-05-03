#if !defined control_h
#define control_h

#include <windows.h>
#include "wassert.h"
#include "control.h"
#include "views.h"
#include "display.h"
#include "metactrl.h"

// private message processed _after_ WM_CREATE is done

extern HINSTANCE TheInstance;

class Controller
{
public:
    Controller(HWND hwnd);
    ~Controller ();
    void Command (HWND hwnd, int id, int code);
    void Scroll (HWND hwnd, int cmd, int pos); 
    void Paint (HWND hwnd);
    void ReInit (HWND hwnd);
    void Stop (HWND hwnd);

private:
    void InitScrollPositions ();
    void PaintScale ();

    bool                _isStopped;

    int                 _bitsPerSample;
    int                 _samplesPerSecond;
    int                 _fftPoints;
    int                 _samplesPerBuf;

    EditReadOnly        _edit, _dispFreq, _dispNote;
    Combo               _comboFreq;
    Combo               _comboPoints;
    RadioButton         _radio8;
    RadioButton         _radio16;
    ScrollBarMap        _scroll;

    StaticEnhMetafileControl    _scaleFreq;

    ViewWave            _viewWave;
    ViewFreq            _viewFreq;
    ViewSpec            _viewSpec;

    Painter             _display;
};

#endif
