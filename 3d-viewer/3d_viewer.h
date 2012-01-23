/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file 3d_viewer.h
 */

#ifndef __3D_VIEWER_H__
#define __3D_VIEWER_H__

#include <wxBasePcbFrame.h>        // m_auimanager member.


#if !wxUSE_GLCANVAS
#error Please set wxUSE_GLCANVAS to 1 in setup.h.
#endif

#include <wx/glcanvas.h>

#ifdef __WXMAC__
#  ifdef __DARWIN__
#    include <OpenGL/glu.h>
#  else
#    include <glu.h>
#  endif
#else
#  include <GL/glu.h>
#endif

#include <3d_struct.h>
#include <id.h>


class BOARD_DESIGN_SETTINGS;
class TRACK;
class TEXTE_PCB;
class DRAWSEGMENT;
class ZONE_CONTAINER;


#define KICAD_DEFAULT_3D_DRAWFRAME_STYLE wxDEFAULT_FRAME_STYLE | wxWANTS_CHARS


#define LIB3D_PATH wxT( "packages3d" )

/**
 * Command IDs for the 3D viewer.
 *
 * Please add IDs that are unique to the 3D viewer here and not in the global
 * id.h file.  This will prevent the entire project from being rebuilt when
 * adding new commands to the 3D viewer.
 */
enum id_3dview_frm
{
    ID_START_COMMAND_3D = ID_END_LIST,
    ID_ROTATE3D_X_NEG,
    ID_ROTATE3D_X_POS,
    ID_ROTATE3D_Y_NEG,
    ID_ROTATE3D_Y_POS,
    ID_ROTATE3D_Z_NEG,
    ID_ROTATE3D_Z_POS,
    ID_RELOAD3D_BOARD,
    ID_TOOL_SCREENCOPY_TOCLIBBOARD,
    ID_MOVE3D_LEFT,
    ID_MOVE3D_RIGHT,
    ID_MOVE3D_UP,
    ID_MOVE3D_DOWN,
    ID_ORTHO,
    ID_MENU3D_BGCOLOR_SELECTION,
    ID_MENU3D_AXIS_ONOFF,
    ID_MENU3D_MODULE_ONOFF,
    ID_MENU3D_UNUSED,
    ID_MENU3D_ZONE_ONOFF,
    ID_MENU3D_DRAWINGS_ONOFF,
    ID_MENU3D_COMMENTS_ONOFF,
    ID_MENU3D_ECO1_ONOFF,
    ID_MENU3D_ECO2_ONOFF,
    ID_END_COMMAND_3D,

    ID_MENU_SCREENCOPY_PNG,
    ID_MENU_SCREENCOPY_JPEG,
    ID_MENU_SCREENCOPY_TOCLIBBOARD,

    ID_POPUP_3D_VIEW_START,
    ID_POPUP_ZOOMIN,
    ID_POPUP_ZOOMOUT,
    ID_POPUP_VIEW_XPOS,
    ID_POPUP_VIEW_XNEG,
    ID_POPUP_VIEW_YPOS,
    ID_POPUP_VIEW_YNEG,
    ID_POPUP_VIEW_ZPOS,
    ID_POPUP_VIEW_ZNEG,
    ID_POPUP_MOVE3D_LEFT,
    ID_POPUP_MOVE3D_RIGHT,
    ID_POPUP_MOVE3D_UP,
    ID_POPUP_MOVE3D_DOWN,
    ID_POPUP_3D_VIEW_END
};


class EDA_3D_CANVAS;
class EDA_3D_FRAME;
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
    double    m_Beginx, m_Beginy;   /* position of mouse */
    double    m_Quat[4];            /* orientation of object */
    double    m_Rot[4];             /* man rotation of object */
    double    m_Zoom;               /* field of view in degrees */
    S3D_Color m_BgColor;
    bool      m_Draw3DAxis;
    bool      m_Draw3DModule;
    bool      m_Draw3DZone;
    bool      m_Draw3DComments;
    bool      m_Draw3DDrawings;
    bool      m_Draw3DEco1;
    bool      m_Draw3DEco2;
    wxPoint   m_BoardPos;
    wxSize    m_BoardSize;
    int       m_Layers;

    const BOARD_DESIGN_SETTINGS*    m_BoardSettings;   // Link to current board design settings

    double    m_Epoxy_Width;    // Epoxy thickness (normalized)

    double    m_BoardScale;     /* Normalization scale for coordinates:
                                 * when scaled between -1.0 and +1.0 */
    double    m_LayerZcoord[32];
    double    m_ActZpos;

public: Info_3D_Visu();
    ~Info_3D_Visu();
};


class EDA_3D_CANVAS : public wxGLCanvas
{
public:
    EDA_3D_FRAME* m_Parent;

private:
    bool         m_init;
    GLuint       m_gllist;
    /// Tracks whether to use Orthographic or Perspective projection
    //TODO: Does this belong here, or in  EDA_3D_FRAME ???
    bool         m_ortho;

#if wxCHECK_VERSION( 2, 7, 0 )
    wxGLContext* m_glRC;
#endif

public:
    EDA_3D_CANVAS( EDA_3D_FRAME* parent, int* attribList = 0 );
    ~EDA_3D_CANVAS();

    void   ClearLists();

    void   OnPaint( wxPaintEvent& event );
    void   OnEraseBackground( wxEraseEvent& event );
    void   OnChar( wxKeyEvent& event );
    void   OnMouseWheel( wxMouseEvent& event );
    void   OnMouseMove( wxMouseEvent& event );
    void   OnRightClick( wxMouseEvent& event );
    void   OnPopUpMenu( wxCommandEvent& event );
    void   TakeScreenshot( wxCommandEvent& event );
    void   SetView3D( int keycode );
    void   DisplayStatus();
    void   Redraw( bool finish = false );
    GLuint DisplayCubeforTest();

    void   OnEnterWindow( wxMouseEvent& event );

    void   Render();

    /**
     * Function CreateDrawGL_List
     * creates the OpenGL draw list items.
     */
    GLuint CreateDrawGL_List();
    void   InitGL();
    void   SetLights();
    void   Draw3D_Track( TRACK* track );

    /**
     * Function Draw3D_SolidPolygonsInZones
     * draw all solid polygons used as filled areas in a zone
     * @param aZone = the zone to draw
    */
    void   Draw3D_SolidPolygonsInZones( ZONE_CONTAINER* aZone );

    /**
     * Function Draw3D_Polygon
     * draw one solid polygon
     * @param aCornersList = a std::vector<wxPoint> list of corners, in physical coordinates
     * @param aZpos = the z position in 3D units
    */
    void   Draw3D_Polygon( std::vector<wxPoint>& aCornersList, double aZpos );

    /**
     * Function Draw3D_Via
     * draws 3D via as a cylinder and filled circles.
     */
    void   Draw3D_Via( SEGVIA* via );
    void   Draw3D_DrawSegment( DRAWSEGMENT* segment );

    /**
     * Function Draw3D_DrawText
     * draws 3D segments to create text objects.
     * When DrawGraphicText is called to draw a text to an OpenGL DC
     * it calls Draw3dTextSegm to each segment to draw.
     * 2 parameters used by Draw3D_FilledSegment are not handled by DrawGraphicText
     * but are used in Draw3D_FilledSegment().
     * they are 2 local variables. This is an ugly, but trivial code.
     * Using DrawGraphicText to draw all texts ensure texts have the same shape
     * in all contexts
     */
    void   Draw3D_DrawText( TEXTE_PCB* text );

    /// Toggles orthographic projection on and off
    void ToggleOrtho(){ m_ortho = !m_ortho ; Refresh(true);};

    /// Returns the orthographic projection flag
    bool ModeIsOrtho() { return m_ortho ;};


    //int Get3DLayerEnable(int act_layer);

    DECLARE_EVENT_TABLE()
};


class EDA_3D_FRAME : public wxFrame
{
public:
    PCB_BASE_FRAME* m_Parent;
private:
    wxString        m_FrameName;      // name used for writing and reading setup. It is "Frame3D"
    EDA_3D_CANVAS*  m_Canvas;
    wxAuiToolBar*   m_HToolBar;
    wxAuiToolBar*   m_VToolBar;
    int             m_InternalUnits;
    wxPoint         m_FramePos;
    wxSize          m_FrameSize;
    wxAuiManager    m_auimgr;
    bool            m_reloadRequest;

public:
    EDA_3D_FRAME( PCB_BASE_FRAME* parent, const wxString& title,
                  long style = KICAD_DEFAULT_3D_DRAWFRAME_STYLE );
    ~EDA_3D_FRAME()
    {
        m_auimgr.UnInit();
    };

    void Exit3DFrame( wxCommandEvent& event );
    void OnCloseWindow( wxCloseEvent& Event );
    void ReCreateMenuBar();
    void ReCreateHToolbar();
    void ReCreateVToolbar();
    void SetToolbars();
    void GetSettings();
    void SaveSettings();

    /**
     * Function ReloadRequest
     * must be called when reloading data from Pcbnew is needed
     * mainly after edition of the board or footprint being displayed.
     * mainly for the module editor.
     */
    void ReloadRequest( )
    {
        m_reloadRequest = true;
    }

    void OnLeftClick( wxDC* DC, const wxPoint& MousePos );
    void OnRightClick( const wxPoint& MousePos, wxMenu* PopMenu );
    void OnKeyEvent( wxKeyEvent& event );
    double BestZoom();
    void RedrawActiveWindow( wxDC* DC, bool EraseBg );
    void Process_Special_Functions( wxCommandEvent& event );
    void Process_Zoom( wxCommandEvent& event );
    void OnActivate( wxActivateEvent& event );

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

void SetGLColor( int color );
void Set_Object_Data( std::vector< S3D_Vertex >& aVertices );

extern Info_3D_Visu g_Parm_3D_Visu;
extern double       g_Draw3d_dx, g_Draw3d_dy;
extern double       ZBottom, ZTop;
extern double       DataScale3D; // 3D scale units.
extern int          gl_attrib[];

#endif  /*  __3D_VIEWER_H__ */
