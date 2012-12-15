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

#include <wxBasePcbFrame.h>         // for m_auimanager member.
#include <layers_id_colors_and_visibility.h>    // Layers id definitions
#include <PolyLine.h>               // for CPolyPt

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

class BOARD_DESIGN_SETTINGS;
class TRACK;
class TEXTE_PCB;
class DRAWSEGMENT;
class ZONE_CONTAINER;
class EDA_3D_FRAME;
class S3D_VERTEX;
class SEGVIA;


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
    void   Redraw( bool finish = false );
    void   Render();

    /**
     * Function CreateDrawGL_List
     * creates the OpenGL draw list items.
     */
    GLuint CreateDrawGL_List();
    void   InitGL();
    void   SetLights();
    void   SetOffset(double aPosX, double aPosY)
    {
        m_draw3dOffset.x = aPosX;
        m_draw3dOffset.y = aPosY;
    }

    void   DrawGrid( double aGriSizeMM );

    /**
     * Function Draw3D_Track
     * @param aTrack = the aTrack to draw
    */
    void   Draw3D_Track( TRACK* aTrack );

    /**
     * Function Draw3D_Via
     * draws 3D via as a cylinder and filled circles.
     */
    void   Draw3D_Via( SEGVIA* via );

    /**
     * Function Draw3D_DrawSegment
     * draws a 3D segment (line, arc or circle).
     */
    void   Draw3D_DrawSegment( DRAWSEGMENT* segment );

    /**
     * Function Draw3D_Zone
     * draw all solid areas in aZone
     * @param aZone = the zone to draw
    */
    void Draw3D_Zone( ZONE_CONTAINER* aZone );

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

    //int Get3DLayerEnable(int act_layer);

    DECLARE_EVENT_TABLE()
};

#endif  /*  _3D_CANVAS_H_ */
