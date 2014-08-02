/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef _3D_CANVAS_H_
#define _3D_CANVAS_H_

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

class BOARD_DESIGN_SETTINGS;
class EDA_3D_FRAME;
class S3D_VERTEX;
class SEGVIA;
class D_PAD;


class EDA_3D_CANVAS : public wxGLCanvas
{
private:
    bool            m_init;
    GLuint          m_gllist;
    wxGLContext*    m_glRC;
    wxRealPoint     m_draw3dOffset;     // offset to draw the 3 mesh.
    double          m_ZBottom;          // position of the back layer
    double          m_ZTop;             // position of the front layer

public:
    EDA_3D_CANVAS( EDA_3D_FRAME* parent, int* attribList = 0 );
    ~EDA_3D_CANVAS();

    EDA_3D_FRAME*   Parent() { return (EDA_3D_FRAME*)GetParent(); }

    void   ClearLists();

    // Event functions:
    void   OnPaint( wxPaintEvent& event );
    void   OnEraseBackground( wxEraseEvent& event );
    void   OnChar( wxKeyEvent& event );
    void   OnMouseWheel( wxMouseEvent& event );
    void   OnMouseMove( wxMouseEvent& event );
    void   OnRightClick( wxMouseEvent& event );
    void   OnPopUpMenu( wxCommandEvent& event );
    void   TakeScreenshot( wxCommandEvent& event );
    void   OnEnterWindow( wxMouseEvent& event );

    // Display functions
    GLuint DisplayCubeforTest();        // Just a test function
    void   SetView3D( int keycode );
    void   DisplayStatus();
    void   Redraw();
    void   Render();

    /**
     * Function CreateDrawGL_List
     * Prepares the parameters of the OpenGL draw list
     * creates the OpenGL draw list items (board, grid ...
     */
    GLuint CreateDrawGL_List();
    void   InitGL();
    void   SetLights();
    void   SetOffset(double aPosX, double aPosY)
    {
        m_draw3dOffset.x = aPosX;
        m_draw3dOffset.y = aPosY;
    }

    /**
     * Function BuildBoard3DView
     * Called by CreateDrawGL_List()
     * Fills the OpenGL draw list with board items draw list.
     */
    void   BuildBoard3DView();

    void   DrawGrid( double aGriSizeMM );
    void   Draw3DViaHole( SEGVIA * aVia );
    void   Draw3DPadHole( D_PAD * aPad );

    DECLARE_EVENT_TABLE()
};

#endif  /*  _3D_CANVAS_H_ */
