/////////////////////////////////////////////////////////////////////////////
// Name:        3d_viewer.h
/////////////////////////////////////////////////////////////////////////////
#if !wxUSE_GLCANVAS
#error Please set wxUSE_GLCANVAS to 1 in setup.h.
#endif

#include "wx/glcanvas.h"

#ifdef __WXMAC__
#  ifdef __DARWIN__
#    include <OpenGL/gl.h>
#    include <OpenGL/glu.h>
#  else
#    include <gl.h>
#    include <glu.h>
#  endif
#else
#  include <GL/gl.h>
#  include <GL/glu.h>
#endif

#include "pcbstruct.h"
#include "3d_struct.h"

#define KICAD_DEFAULT_3D_DRAWFRAME_STYLE wxDEFAULT_FRAME_STYLE|wxWANTS_CHARS


#define LIB3D_PATH wxT("packages3d")

class Pcb3D_GLCanvas;
class WinEDA3D_DrawFrame;
class Info_3D_Visu;
class S3D_Vertex;
class SEGVIA;


#define m_ROTX m_Rot[0]
#define m_ROTY m_Rot[1]
#define m_ROTZ m_Rot[2]

/* information needed to display 3D board */
class Info_3D_Visu
{
public:
    double m_Beginx, m_Beginy;  /* position of mouse */
    double m_Quat[4];           /* orientation of object */
    double m_Rot[4];                /* man rotation of object */
    double m_Zoom;              /* field of view in degrees */
    S3D_Color m_BgColor;
    bool m_Draw3DAxis;
    bool m_Draw3DModule;
    bool m_Draw3DZone;
    bool m_Draw3DComments;
    bool m_Draw3DDrawings;
    bool m_Draw3DEco1;
    bool m_Draw3DEco2;
    wxPoint m_BoardPos;
    wxSize m_BoardSize;
    int m_Layers;
    EDA_BoardDesignSettings * m_BoardSettings;  // Link to current board design settings
    double m_Epoxy_Width;       /* Epoxy tickness (normalized) */

    double m_BoardScale;            /* Normalisation scale for coordinates:
                                when scaled tey are between -1.0 and +1.0 */
    double m_LayerZcoord[32];
public:
    Info_3D_Visu();
    ~Info_3D_Visu();
};


class Pcb3D_GLCanvas: public wxGLCanvas
{
public:
    WinEDA3D_DrawFrame * m_Parent;

private:
    bool   m_init;
    GLuint m_gllist;

public:
    Pcb3D_GLCanvas( WinEDA3D_DrawFrame *parent );
    ~Pcb3D_GLCanvas();

    void ClearLists();

    void OnPaint(wxPaintEvent& event);
    void OnEraseBackground(wxEraseEvent& event);
    void OnChar(wxKeyEvent& event);
    void OnMouseEvent(wxMouseEvent& event);
    void OnRightClick(wxMouseEvent& event);
    void OnPopUpMenu(wxCommandEvent & event);
    void TakeScreenshot(wxCommandEvent & event);
    void SetView3D(int keycode);
    void DisplayStatus();
    void Redraw(bool finish = false);
    GLuint DisplayCubeforTest();

    void OnEnterWindow( wxMouseEvent& event );

    void Render();
    GLuint CreateDrawGL_List();
    void InitGL();
    void SetLights();
    void Draw3D_Track(TRACK * track);
    void Draw3D_Via(SEGVIA * via);
    void Draw3D_DrawSegment(DRAWSEGMENT * segment);
    void Draw3D_DrawText(TEXTE_PCB * text);
    //int Get3DLayerEnable(int act_layer);

DECLARE_EVENT_TABLE()
};


class WinEDA3D_DrawFrame: public wxFrame
{
public:
    WinEDA_BasePcbFrame * m_Parent;
    Pcb3D_GLCanvas * m_Canvas;
    wxToolBar * m_HToolBar;
    wxToolBar * m_VToolBar;
    int m_InternalUnits;
    wxPoint m_FramePos;
    wxSize m_FrameSize;

private:
    wxString m_FrameName;       // name used for writting and reading setup
                                // It is "Frame3D"

public:
    WinEDA3D_DrawFrame( WinEDA_BasePcbFrame * parent,
                        const wxString& title,
                        long style = KICAD_DEFAULT_3D_DRAWFRAME_STYLE );

    void Exit3DFrame(wxCommandEvent& event);
    void OnCloseWindow(wxCloseEvent & Event);
    void ReCreateMenuBar();
    void ReCreateHToolbar();
    void ReCreateVToolbar();
    void SetToolbars();
    void GetSettings();
    void SaveSettings();

    void OnLeftClick(wxDC * DC, const wxPoint& MousePos);
    void OnRightClick(const wxPoint& MousePos, wxMenu * PopMenu);
    void OnKeyEvent(wxKeyEvent& event);
    int BestZoom(); // Retourne le meilleur zoom
    void RedrawActiveWindow(wxDC * DC, bool EraseBg);
    void Process_Special_Functions(wxCommandEvent& event);
    void Process_Zoom(wxCommandEvent& event);

    void NewDisplay();
    void Set3DBgColor();
    void Set3DAxisOnOff();
    void Set3DModuleOnOff();
    void Set3DPlaceOnOff();
    void Set3DZoneOnOff();
    void Set3DCommentsOnOff();
    void Set3DDrawingsOnOff();
    void Set3DEco1OnOff();
    void Set3DEco2OnOff();

    DECLARE_EVENT_TABLE()
};

void SetGLColor(int color);
void Set_Object_Data(const S3D_Vertex * coord, int nbcoord );

extern Info_3D_Visu g_Parm_3D_Visu;
extern double g_Draw3d_dx, g_Draw3d_dy;
extern double ZBottom, ZTop;
extern double DataScale3D;  // coeff de conversion unites utilsateut -> unites 3D
extern int gl_attrib[];
