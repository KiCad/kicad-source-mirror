/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <class_module.h>

class BOARD_DESIGN_SETTINGS;
class EDA_3D_FRAME;
class CPOLYGONS_LIST;

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
    GL_ID_3DSHAPES_SOLID_BACK,  // List id for 3D shapes, non transparent entities
    GL_ID_3DSHAPES_TRANSP_BACK, // List id for 3D shapes, transparent entities
    GL_ID_SHADOW_FRONT,
    GL_ID_SHADOW_BACK,
    GL_ID_SHADOW_BOARD,
    GL_ID_BODY,                 // Body only list
    GL_ID_END
};

class EDA_3D_CANVAS : public wxGLCanvas
{
private:
    bool            m_init;
    bool            m_reportWarnings;       // true to report all wranings when build the 3D scene
                                            // false to report errors only
    GLuint          m_glLists[GL_ID_END];   // GL lists
    wxGLContext*    m_glRC;
    wxRealPoint     m_draw3dOffset;     // offset to draw the 3D mesh.
    double          m_ZBottom;          // position of the back layer
    double          m_ZTop;             // position of the front layer

    GLuint          m_text_pcb;     // an index to the texture generated for pcb texts
    GLuint          m_text_silk;    // an index to the texture generated for silk layers

    // Index to the textures generated for shadows
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
     * creates the OpenGL draw list items (board, grid ...)
     * @param aErrorMessages = a wxString which will filled with error messages,
     * if any
     * @param aShowWarnings = true to show all messages, false to show errors only
     */
    void   CreateDrawGL_List( wxString* aErrorMessages, bool aShowWarnings );
    void   InitGL();

    void ReportWarnings( bool aReport ) { m_reportWarnings = aReport; }

    void   SetLights();

    void   SetOffset(double aPosX, double aPosY)
    {
        m_draw3dOffset.x = aPosX;
        m_draw3dOffset.y = aPosY;
    }

    /** @return the INFO3D_VISU which contains the current parameters
     * to draw the 3D view og the board
     */
    INFO3D_VISU& GetPrm3DVisu() const;


private:

    /**
     * return true if we are in realistic mode render
     */
    bool isRealisticMode() const;

    /**
     * @return true if aItem should be displayed
     * @param aItem = an item of DISPLAY3D_FLG enum
     */
    bool isEnabled( DISPLAY3D_FLG aItem ) const;

    /** Helper function
     * @return true if aLayer should be displayed, false otherwise
     */
    bool is3DLayerEnabled( LAYER_ID aLayer ) const;

    /**
     * @return the size of the board in pcb units
     */
    wxSize getBoardSize() const;

    /**
     * @return the position of the board center in pcb units
     */
    wxPoint getBoardCenter() const;

    /**
     * Helper function SetGLTechLayersColor
     * Initialize the color to draw the non copper layers
     * in realistic mode and normal mode.
     */
    void setGLTechLayersColor( LAYER_NUM aLayer );

    /**
     * Helper function SetGLCopperColor
     * Initialize the copper color to draw the board
     * in realistic mode (a golden yellow color )
     */
    void setGLCopperColor();

    /**
     * Helper function SetGLEpoxyColor
     * Initialize the color to draw the epoxy body board in realistic mode.
     */
    void setGLEpoxyColor( float aTransparency = 1.0 );

    /**
     * Helper function SetGLSolderMaskColor
     * Initialize the color to draw the solder mask layers in realistic mode.
     */
    void setGLSolderMaskColor( float aTransparency = 1.0 );

    /**
     * Function BuildBoard3DView
     * Called by CreateDrawGL_List()
     * Populates the OpenGL GL_ID_BOARD draw list with board items only on copper layers.
     * 3D footprint shapes, tech layers and aux layers are not on this list
     * Fills aErrorMessages with error messages created by some calculation function
     * @param aBoardList =
     * @param aBodyOnlyList =
     * @param aErrorMessages = a wxString to add error and warning messages
     * created by the build process (can be NULL)
     * @param aShowWarnings = true to show all messages, false to show errors only
     */
    void   BuildBoard3DView( GLuint aBoardList, GLuint aBodyOnlyList,
                             wxString* aErrorMessages, bool aShowWarnings );

    /**
     * Function BuildTechLayers3DView
     * Called by CreateDrawGL_List()
     * Populates the OpenGL GL_ID_TECH_LAYERS draw list with items on tech layers
     * @param aErrorMessages = a wxString to add error and warning messages
     * created by the build process (can be NULL)
     * @param aShowWarnings = true to show all messages, false to show errors only
     */
    void   BuildTechLayers3DView( wxString* aErrorMessages, bool aShowWarnings );

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

    /**
     * Helper function BuildPadShapeThickOutlineAsPolygon:
     * Build a pad outline as non filled polygon, to draw pads on silkscreen layer
     * with a line thickness = aWidth
     * Used only to draw pads outlines on silkscreen layers.
     */
    void BuildPadShapeThickOutlineAsPolygon( const D_PAD*          aPad,
                                             CPOLYGONS_LIST& aCornerBuffer,
                                             int             aWidth,
                                             int             aCircleToSegmentsCount,
                                             double          aCorrectionFactor );


    /**
     * Helper function Draw3DViaHole:
     * Draw the via hole:
     * Build a vertical hole (a cylinder) between the first and the last via layers
     */
    void   Draw3DViaHole( const VIA * aVia );

    /**
     * Helper function Draw3DPadHole:
     * Draw the pad hole:
     * Build a vertical hole (round or oblong) between the front and back layers
     */
    void   Draw3DPadHole( const D_PAD * aPad );

    /**
     * function Render3DComponentShape
     * insert mesh in gl list
     * @param module
     * @param  aIsRenderingJustNonTransparentObjects = true to load non transparent objects
     * @param  aIsRenderingJustTransparentObjects = true to load non transparent objects
     * @param  aSideToLoad = false will load not fliped, true will load fliped objects
     * in openGL, transparent objects should be drawn *after* non transparent objects
     */
    void Render3DComponentShape( MODULE* module,
                                 bool aIsRenderingJustNonTransparentObjects,
                                 bool aIsRenderingJustTransparentObjects,
                                 bool aSideToLoad );

    /**
     * function Read3DComponentShape
     * read the 3D component shape(s) of the footprint (physical shape).
     * @param module
     * @param model_parsers_list = list of each new model loaded
     * @param model_filename_list = list of each new filename model loaded
     * @return true if load was succeeded, false otherwise
     */
    bool Read3DComponentShape( MODULE* module,
                               std::vector<S3D_MODEL_PARSER *>& model_parsers_list,
                               std::vector<wxString>& model_filename_list );

    /**
     * function GenerateFakeShadowsTextures
     * creates shadows of the board an footprints
     * for aesthetical purpose
     * @param aErrorMessages = a wxString to add error and warning messages
     * created by the build process (can be NULL)
     * @param aShowWarnings = true to show all messages, false to show errors only
     */
    void   GenerateFakeShadowsTextures( wxString* aErrorMessages, bool aShowWarnings );

    DECLARE_EVENT_TABLE()
};

void CheckGLError(const char *aFileName, int aLineNumber);

#endif  /*  _3D_CANVAS_H_ */
