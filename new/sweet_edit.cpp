/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2011 KiCad Developers, see change_log.txt for contributors.
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

// wxWidgets imports
#include <wx/wx.h>
#include <wx/graphics.h>
#include <wx/timer.h>
#include <wx/event.h>
#include <wx/wxprec.h>
#include <wx/cmdline.h>

// Graphics Abstraction Layer imports
#include <gal/common/definitions.h>
#include <gal/common/graphics_abstraction_layer.h>
#include <gal/opengl/opengl_gal.h>
#include <gal/common/stroke_font.h>
#include <gal/common/color4d.h>
#include <gal/font/newstroke_font.h>

#include <math/vector2d.h>
#include <math/matrix3x3.h>

#include <sweet_editor_panel.h>

#define screenSizeX 640
#define screenSizeY 480



/**
 * Class SWEET_EDIT
 * implements an editor for SWEET, including support for inheritance and
 * LIB_TABLE configuration.
 */
class SWEET_FRAME: public wxFrame
{
public:
    // @brief Constructor
    SWEET_FRAME( wxWindow* parent, wxWindowID id, const wxString& title,
        const wxPoint& pos, const wxSize& size );

    // @brief Destructor
    ~SWEET_FRAME();

private:
    bool m_isReady;
    bool m_isPanning;

    SCH::CANVAS*    m_gal;

    wxSize          m_screenSize;
    VECTOR2D        m_worldSize;
    VECTOR2D        m_startMousePoint;
    VECTOR2D        m_startLookAtPoint;

    /*
    double          m_alpha;

    MATRIX3x3D      m_startMatrix;

    STROKE_FONT     m_font;
    */

    // Event handlers
    void OnMotion( wxMouseEvent& event );
    void OnMouseWheel( wxMouseEvent& event );
    void OnRightDown( wxMouseEvent& event );
    void OnRightUp( wxMouseEvent& event );
//    void OnRedraw( wxCommandEvent& event );
};


SWEET_FRAME::SWEET_FRAME( wxWindow* parent, wxWindowID id, const wxString& title,
        const wxPoint& pos, const wxSize& size ) :
    wxFrame( parent, id, title, pos, size ),
    m_screenSize( size.x, size.y )
{
    m_isPanning = false;

    SWEET_EDITOR_PANEL* panel =
        new SWEET_EDITOR_PANEL( this, wxID_ANY, wxPoint( 0, 0 ), wxSize( -1, -1 ), 0 );

    m_gal = (SCH::CANVAS*) panel->FindWindowByName( wxT( "GLCanvas" ), panel );

    wxASSERT( m_gal );

    m_gal->SetMouseListener( this );

    Connect( wxEVT_MOTION, wxMouseEventHandler( SWEET_FRAME::OnMotion ) );
    Connect( wxEVT_MOUSEWHEEL, wxMouseEventHandler( SWEET_FRAME::OnMouseWheel ) );
    Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( SWEET_FRAME::OnRightDown ) );
    Connect( wxEVT_RIGHT_UP, wxMouseEventHandler( SWEET_FRAME::OnRightUp ) );
}


SWEET_FRAME::~SWEET_FRAME()
{
    delete m_gal;
}


void SWEET_FRAME::OnMotion( wxMouseEvent& event )
{
    VECTOR2D mousePoint( event.GetX(), event.GetY() );

    if( event.Dragging() )
    {
        if( m_isPanning )
        {
            MATRIX3x3D  matrix = m_gal->GetWorldScreenMatrix().Inverse();
            VECTOR2D    delta  = matrix.GetScale().x * ( m_startMousePoint - mousePoint );

            m_gal->SetLookAtPoint( m_startLookAtPoint + delta );

            m_gal->PaintScene();
        }
    }
    else
    {
        m_gal->DrawCursor( mousePoint );
    }
}


void SWEET_FRAME::OnMouseWheel( wxMouseEvent& event )
{
    double zoomScale = ( event.GetWheelRotation() > 0.0 ) ? 1.1 : 0.9;

    m_gal->SetZoomFactor( m_gal->GetZoomFactor() * zoomScale );
    m_gal->PaintScene();
}


void SWEET_FRAME::OnRightDown( wxMouseEvent& event )
{
    m_isPanning = true;
    m_startMousePoint = VECTOR2D( event.GetX(), event.GetY() );
    m_startLookAtPoint = m_gal->GetLookAtPoint();
}


void SWEET_FRAME::OnRightUp( wxMouseEvent& event )
{
    m_isPanning = false;
}


static const wxCmdLineEntryDesc g_cmdLineDesc[] = {
    {
        wxCMD_LINE_PARAM,
        NULL,
        NULL,
        "filename of libtable",
         wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_MULTIPLE
    },
    { wxCMD_LINE_NONE }
};


class GAL_TEST_APPLICATION: public wxApp
{
public:
    GAL_TEST_APPLICATION();

private:
    virtual bool OnInit();
    virtual int OnExit();
    virtual int OnRun();
    virtual void OnInitCmdLine( wxCmdLineParser& aParser );
    virtual bool OnCmdLineParsed( wxCmdLineParser& aParser );
};


GAL_TEST_APPLICATION::GAL_TEST_APPLICATION()
{
}


bool GAL_TEST_APPLICATION::OnInit()
{
    if ( !wxApp::OnInit() )
        return false;

    SWEET_FRAME* frame = new SWEET_FRAME( (wxFrame *) NULL,
        -1, wxT( "SWEET Editor" ),
        wxPoint( screenSizeX + 10, 0 ),
        wxSize( screenSizeX, screenSizeY ) );

    frame->Show( true );

    return true;
}


int GAL_TEST_APPLICATION::OnExit()
{
    return 0;
}


int GAL_TEST_APPLICATION::OnRun()
{
    int exitcode = wxApp::OnRun();
    return exitcode;
}


void GAL_TEST_APPLICATION::OnInitCmdLine( wxCmdLineParser& parser )
{
    parser.SetDesc( g_cmdLineDesc );
    parser.SetSwitchChars( wxT( "-" ) );
}


bool GAL_TEST_APPLICATION::OnCmdLineParsed( wxCmdLineParser& parser )
{
    return true;
}


DECLARE_APP( GAL_TEST_APPLICATION )

IMPLEMENT_APP( GAL_TEST_APPLICATION )
