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

class VIA;
class D_PAD;

// We are using GL lists to store layers and other items
// to draw or not
// GL_LIST_ID are the GL lists indexes in m_glLists
enum GL_LIST_ID
{
    GL_ID_BEGIN = 0,
    GL_ID_AXIS = GL_ID_BEGIN,   // list id for 3D axis
    GL_ID_GRID,                 // list id for 3D grid
    GL_ID_BOARD,                // List id for copper layers
    GL_ID_TECH_LAYERS,          // List id for non copper layers (masks...)
    GL_ID_AUX_LAYERS,           // List id for user layers (draw, eco, comment)
    GL_ID_3DSHAPES_SOLID_FRONT, // List id for 3D shapes, non transparent entities
    GL_ID_3DSHAPES_TRANSP_FRONT,// List id for 3D shapes, transparent entities
    GL_ID_3DSHAPES_SOLID_BACK, // List id for 3D shapes, non transparent entities
    GL_ID_3DSHAPES_TRANSP_BACK,// List id for 3D shapes, transparent entities
    GL_ID_SHADOW_FRONT,
    GL_ID_SHADOW_BACK,
    GL_ID_SHADOW_BOARD,
    GL_ID_BODY,                // Body only list
    GL_ID_END
};

class EDA_3D_CANVAS : public wxGLCanvas
{
private:
    bool            m_init;
    GLuint          m_glLists[GL_ID_END];    // GL lists
    wxGLContext*    m_glRC;
    wxRealPoint     m_draw3dOffset;     // offset to draw the 3D mesh.
    double          m_ZBottom;          // position of the back layer
    double          m_ZTop;             // position of the front layer

    GLuint          m_text_pcb;
    GLuint          m_text_silk;

    bool            m_shadow_init;
    GLuint          m_text_fake_shadow_front;
    GLuint          m_text_fake_shadow_back;
    GLuint          m_text_fake_shadow_board;

    void Create_and_Render_Shadow_Buffer( GLuint *aDst_gl_texture,
            GLuint aTexture_size, bool aDraw_body, int aBlurPasses );

public:
    EDA_3D_CANVAS( EDA_3D_FRAME* parent, int* attribList = 0 );
    ~EDA_3D_CANVAS();

    EDA_3D_FRAME* Parent() const { return static_cast<EDA_3D_FRAME*>( GetParent() ); }

    BOARD* GetBoard() { return Parent()->GetBoard(); }

    /**
     * Function ClearLists
     * Clear the display list.
     * @param aGlList = the list to clear.
     * if 0 (default) all lists are cleared
     */
    void   ClearLists( int aGlList = 0 );

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
    void   SetView3D( int keycode );
    void   DisplayStatus();
    void   Redraw();
    void   Render();

    /**
     * Function CreateDrawGL_List
     * Prepares the parameters of the OpenGL draw list
     * creates the OpenGL draw list items (board, grid ...
     */
    void   CreateDrawGL_List();
    void   InitGL();
    void   SetLights();
    void   SetOffset(double aPosX, double aPosY)
    {
        m_draw3dOffset.x = aPosX;
        m_draw3dOffset.y = aPosY;
    }
    void SetGLTechLayersColor( LAYER_NUM aLayer );
    void SetGLCopperColor();
    void SetGLEpoxyColor( double aTransparency = 1.0 );

    /**
     * Function BuildBoard3DView
     * Called by CreateDrawGL_List()
     * Populates the OpenGL GL_ID_BOARD draw list with board items only on copper layers.
     * 3D footprint shapes, tech layers and aux layers are not on this list
     */
    void   BuildBoard3DView(GLuint aBoardList, GLuint aBodyOnlyList);

    /**
     * Function BuildTechLayers3DView
     * Called by CreateDrawGL_List()
     * Populates the OpenGL GL_ID_TECH_LAYERS draw list with items on tech layers
     */
    void   BuildTechLayers3DView();

    /**
     * Function BuildShadowList
     * Called by CreateDrawGL_List()
     */
     void BuildShadowList( GLuint aFrontList, GLuint aBacklist, GLuint aBoardList );

    /**
     * Function BuildFootprintShape3DList
     * Called by CreateDrawGL_List()
     * Fills the OpenGL GL_ID_3DSHAPES_SOLID and GL_ID_3DSHAPES_TRANSP
     * draw lists with 3D footprint shapes
     * @param aOpaqueList is the gl list for non transparent items
     * @param aTransparentList is the gl list for non transparent items,
     * which need to be drawn after all other items
     */
    void   BuildFootprintShape3DList( GLuint aOpaqueList,
                                      GLuint aTransparentList,
                                      bool aSideToLoad );
    /**
     * Function BuildBoard3DAuxLayers
     * Called by CreateDrawGL_List()
     * Fills the OpenGL GL_ID_AUX_LAYERS draw list
     * with items on aux layers only
     */
    void   BuildBoard3DAuxLayers();

    void   Draw3DGrid( double aGriSizeMM );
    void   Draw3DAxis();

    void   Draw3DViaHole( const VIA * aVia );
    void   Draw3DPadHole( const D_PAD * aPad );

    void   GenerateFakeShadowsTextures();

    DECLARE_EVENT_TABLE()
};

void CheckGLError(const char *aFileName, int aLineNumber);

#endif  /*  _3D_CANVAS_H_ */
