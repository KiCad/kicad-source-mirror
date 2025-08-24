/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2017-2018 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * CairoGal - Graphics Abstraction Layer for Cairo
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

#ifndef CAIROGAL_H_
#define CAIROGAL_H_

#include <map>
#include <iterator>

#include <cairo.h>

#include <gal/gal.h>
#include <gal/graphics_abstraction_layer.h>
#include <wx/dcbuffer.h>

#include <memory>

/**
 * The Cairo implementation of the graphics abstraction layer.
 *
 * Quote from Wikipedia:
 * " Cairo is a software library used to provide a vector graphics-based, device-independent
 *   API for software developers. It is designed to provide primitives for 2-dimensional
 *   drawing across a number of different backends. "
 *
 * Cairo offers also backends for PostScript and PDF surfaces. So it can be used for printing
 * of KiCad graphics surfaces as well.
 */
namespace KIGFX
{
class CAIRO_COMPOSITOR;

class GAL_API CAIRO_GAL_BASE : public GAL
{
public:
    CAIRO_GAL_BASE( GAL_DISPLAY_OPTIONS& aDisplayOptions );

    ~CAIRO_GAL_BASE();

    bool IsCairoEngine() override { return true; }

    // ---------------
    // Drawing methods
    // ---------------

    /// @copydoc GAL::DrawLine()
    void DrawLine( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint ) override;

    /// @copydoc GAL::DrawSegment()
    void DrawSegment( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint,
                      double aWidth ) override;

    /// @copydoc GAL::DrawCircle()
    void DrawCircle( const VECTOR2D& aCenterPoint, double aRadius ) override;

    /// @copydoc GAL::DrawHoleWall()
    void DrawHoleWall( const VECTOR2D& aCenterPoint, double aHoleRadius,
                       double aWallWidth ) override;

    /// @copydoc GAL::DrawArc()
    void DrawArc( const VECTOR2D& aCenterPoint, double aRadius, const EDA_ANGLE& aStartAngle,
                  const EDA_ANGLE& aAngle ) override;

    /// @copydoc GAL::DrawArcSegment()
    /// Note: aMaxError is not used in Cairo, because Cairo can draw true arcs
    void DrawArcSegment( const VECTOR2D& aCenterPoint, double aRadius, const EDA_ANGLE& aStartAngle,
                         const EDA_ANGLE& aAngle, double aWidth, double aMaxError ) override;

    /// @copydoc GAL::DrawRectangle()
    void DrawRectangle( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint ) override;

    /// @copydoc GAL::DrawSegmentChain()
    void DrawSegmentChain( const std::vector<VECTOR2D>& aPointList, double aWidth ) override;
    void DrawSegmentChain( const SHAPE_LINE_CHAIN& aLineChain, double aWidth ) override;

    /// @copydoc GAL::DrawPolyline()
    void DrawPolyline( const std::deque<VECTOR2D>& aPointList ) override { drawPoly( aPointList ); }
    void DrawPolyline( const VECTOR2D aPointList[], int aListSize ) override
    {
        drawPoly( aPointList, aListSize );
    }

    void DrawPolyline( const SHAPE_LINE_CHAIN& aLineChain ) override { drawPoly( aLineChain ); }

    /// @copydoc GAL::DrawPolylines()
    void DrawPolylines( const std::vector<std::vector<VECTOR2D>>& aPointLists ) override
    {
        for( const std::vector<VECTOR2D>& points : aPointLists )
            drawPoly( points );
    }

    /// @copydoc GAL::DrawPolygon()
    void DrawPolygon( const std::deque<VECTOR2D>& aPointList ) override { drawPoly( aPointList ); }
    void DrawPolygon( const VECTOR2D aPointList[], int aListSize ) override
    {
        drawPoly( aPointList, aListSize );
    }

    void DrawPolygon( const SHAPE_POLY_SET& aPolySet, bool aStrokeTriangulation = false ) override;
    void DrawPolygon( const SHAPE_LINE_CHAIN& aPolySet ) override;

    /// @copydoc GAL::DrawGlyph()
    void DrawGlyph( const KIFONT::GLYPH& aPolySet, int aNth, int aTotal ) override;

    /// @copydoc GAL::DrawGlyphs()
    void DrawGlyphs( const std::vector<std::unique_ptr<KIFONT::GLYPH>>& aGlyphs ) override
    {
        for( size_t i = 0; i < aGlyphs.size(); i++ )
            DrawGlyph( *aGlyphs[i], i, aGlyphs.size() );
    }

    /// @copydoc GAL::DrawCurve()
    void DrawCurve( const VECTOR2D& startPoint, const VECTOR2D& controlPointA,
                    const VECTOR2D& controlPointB, const VECTOR2D& endPoint,
                    double aFilterValue = 0.0 ) override;

    /// @copydoc GAL::DrawBitmap()
    void DrawBitmap( const BITMAP_BASE& aBitmap, double alphaBlend = 1.0 ) override;

    // --------------
    // Screen methods
    // --------------

    /// Resizes the canvas.
    void ResizeScreen( int aWidth, int aHeight ) override;

    /// @copydoc GAL::Flush()
    void Flush() override;

    /// @copydoc GAL::ClearScreen()
    void ClearScreen() override;

    // -----------------
    // Attribute setting
    // -----------------

    /// @copydoc GAL::SetIsFill()
    void SetIsFill( bool aIsFillEnabled ) override;

    /// @copydoc GAL::SetIsStroke()
    void SetIsStroke( bool aIsStrokeEnabled ) override;

    /// @copydoc GAL::SetStrokeColor()
    void SetStrokeColor( const COLOR4D& aColor ) override;

    /// @copydoc GAL::SetFillColor()
    void SetFillColor( const COLOR4D& aColor ) override;

    /// @copydoc GAL::SetLineWidth()
    void SetLineWidth( float aLineWidth ) override;

    /// @copydoc GAL::SetLayerDepth()
    void SetLayerDepth( double aLayerDepth ) override;

    // --------------
    // Transformation
    // --------------

    /// @copydoc GAL::Transform()
    void Transform( const MATRIX3x3D& aTransformation ) override;

    /// @copydoc GAL::Rotate()
    void Rotate( double aAngle ) override;

    /// @copydoc GAL::Translate()
    void Translate( const VECTOR2D& aTranslation ) override;

    /// @copydoc GAL::Scale()
    void Scale( const VECTOR2D& aScale ) override;

    /// @copydoc GAL::Save()
    void Save() override;

    /// @copydoc GAL::Restore()
    void Restore() override;

    // --------------------------------------------
    // Group methods
    // ---------------------------------------------

    /// @copydoc GAL::BeginGroup()
    int BeginGroup() override;

    /// @copydoc GAL::EndGroup()
    void EndGroup() override;

    /// @copydoc GAL::DrawGroup()
    void DrawGroup( int aGroupNumber ) override;

    /// @copydoc GAL::ChangeGroupColor()
    void ChangeGroupColor( int aGroupNumber, const COLOR4D& aNewColor ) override;

    /// @copydoc GAL::ChangeGroupDepth()
    void ChangeGroupDepth( int aGroupNumber, int aDepth ) override;

    /// @copydoc GAL::DeleteGroup()
    void DeleteGroup( int aGroupNumber ) override;

    /// @copydoc GAL::ClearCache()
    void ClearCache() override;

    // --------------------------------------------------------
    // Handling the world <-> screen transformation
    // --------------------------------------------------------

    /// @copydoc GAL::SetNegativeDrawMode()
    void SetNegativeDrawMode( bool aSetting ) override;

    // -------
    // Cursor
    // -------

    /// @copydoc GAL::DrawCursor()
    void DrawCursor( const VECTOR2D& aCursorPosition ) override;

    void EnableDepthTest( bool aEnabled = false ) override;

    ///< @copydoc GAL::DrawGrid()
    void DrawGrid() override;

    /// @copydoc GAL::BeginDrawing()
    void BeginDrawing() override;

    /// @copydoc GAL::EndDrawing()
    void EndDrawing() override;


protected:
    // Geometric transforms according to the m_currentWorld2Screen transform matrix:
    double xform( double x );             // scale
    const VECTOR2D xform( double x, double y ); // rotation, scale and offset
    const VECTOR2D xform( const VECTOR2D& aP ); // rotation, scale and offset

    /**
     * Transform according to the rotation from m_currentWorld2Screen transform matrix.
     *
     * @param aAngle is the angle in radians to transform.
     * @return the modified angle.
     */
    double angle_xform( double aAngle );

    /**
     * Transform according to the rotation from m_currentWorld2Screen transform matrix
     * for the start angle and the end angle of an arc.
     *
     * @param aStartAngle is the arc starting point in radians to transform
     * @param aEndAngle is the arc ending point in radians to transform
     */
    void arc_angles_xform_and_normalize( double& aStartAngle, double& aEndAngle );

    void resetContext();

    /**
     * Draw a grid line (usually a simplified line function).
     *
     * @param aStartPoint is the start point of the line.
     * @param aEndPoint is the end point of the line.
     */
    void drawGridLine( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint );
    void drawGridCross( const VECTOR2D& aPoint );
    void drawGridPoint( const VECTOR2D& aPoint, double aWidth, double aHeight );
    void drawAxes( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint );


    void flushPath();
    void storePath();                           ///< Store the actual path

    /**
     * Blit cursor into the current screen.
     */
    void blitCursor( wxMemoryDC& clientDC );

    /// Drawing polygons & polylines is the same in Cairo, so here is the common code
    void drawPoly( const std::deque<VECTOR2D>& aPointList );
    void drawPoly( const std::vector<VECTOR2D>& aPointList );
    void drawPoly( const VECTOR2D aPointList[], int aListSize );
    void drawPoly( const SHAPE_LINE_CHAIN& aLineChain );

    /**
     * Return a valid key that can be used as a new group number.
     *
     * @return An unique group number that is not used by any other group.
     */
    unsigned int getNewGroupNumber();

    void syncLineWidth( bool aForceWidth = false, double aWidth = 0.0 );
    void updateWorldScreenMatrix();
    const VECTOR2D roundp( const VECTOR2D& v );

    /// Super class definition
    typedef GAL super;

    /// Maximum number of arguments for one command
    static const int MAX_CAIRO_ARGUMENTS = 4;

    /// Definitions for the command recorder
    enum GRAPHICS_COMMAND
    {
        CMD_SET_FILL,                               ///< Enable/disable filling
        CMD_SET_STROKE,                             ///< Enable/disable stroking
        CMD_SET_FILLCOLOR,                          ///< Set the fill color
        CMD_SET_STROKECOLOR,                        ///< Set the stroke color
        CMD_SET_LINE_WIDTH,                         ///< Set the line width
        CMD_STROKE_PATH,                            ///< Set the stroke path
        CMD_FILL_PATH,                              ///< Set the fill path
        //CMD_TRANSFORM,                              ///< Transform the actual context
        CMD_ROTATE,                                 ///< Rotate the context
        CMD_TRANSLATE,                              ///< Translate the context
        CMD_SCALE,                                  ///< Scale the context
        CMD_SAVE,                                   ///< Save the transformation matrix
        CMD_RESTORE,                                ///< Restore the transformation matrix
        CMD_CALL_GROUP                              ///< Call a group
    };

    /// Type definition for an graphics group element
    struct GROUP_ELEMENT
    {
        GRAPHICS_COMMAND m_Command;                 ///< Command to execute
        union {
            double DblArg[MAX_CAIRO_ARGUMENTS];     ///< Arguments for Cairo commands
            bool   BoolArg;                         ///< A bool argument
            int    IntArg = 0;                      ///< An int argument
        }                m_Argument;
        cairo_path_t*    m_CairoPath = nullptr;     ///< Pointer to a Cairo path
    };

    typedef std::deque<GROUP_ELEMENT> GROUP;        ///< A graphic group type definition

    // Variables for the grouping function
    bool                  m_isGrouping;             ///< Is grouping enabled ?
    bool                  m_isElementAdded;         ///< Was an graphic element added ?
    std::map<int, GROUP>  m_groups;                 ///< List of graphic groups
    unsigned int          m_groupCounter;           ///< Counter used for generating group keys
    GROUP*                m_currentGroup;           ///< Currently used group

    double                m_lineWidthInPixels;
    bool                  m_lineWidthIsOdd;

    cairo_matrix_t        m_cairoWorldScreenMatrix; ///< Cairo world to screen transform matrix
    cairo_matrix_t        m_currentXform;
    cairo_matrix_t        m_currentWorld2Screen;
    cairo_t*              m_currentContext;         ///< Currently used Cairo context for drawing
    cairo_t*              m_context;                ///< Cairo image
    cairo_surface_t*      m_surface;                ///< Cairo surface

    /// List of surfaces that were created by painting images, to be cleaned up later
    std::vector<cairo_surface_t*> m_imageSurfaces;

    std::vector<cairo_matrix_t>   m_xformStack;
    /// Format used to store pixels
    static constexpr cairo_format_t GAL_FORMAT = CAIRO_FORMAT_ARGB32;
};


class GAL_API CAIRO_GAL : public CAIRO_GAL_BASE, public wxWindow
{
public:
    /**
     * @param aParent is the wxWidgets immediate wxWindow parent of this object.
     * @param aMouseListener is the wxEvtHandler that should receive the mouse events, this
     *                       can be can be any wxWindow, but is often a wxFrame container.
     * @param aPaintListener is the wxEvtHandler that should receive the paint event.  This
     *                       can be any wxWindow, but is often a derived instance of this
     *                       class or a containing wxFrame.  The "paint event" here is a
     *                       wxCommandEvent holding EVT_GAL_REDRAW, as sent by PostPaint().
     *
     * @param aName is the name of this window for use by wxWindow::FindWindowByName().
     */
    CAIRO_GAL( GAL_DISPLAY_OPTIONS& aDisplayOptions, wxWindow* aParent,
               wxEvtHandler* aMouseListener = nullptr, wxEvtHandler* aPaintListener = nullptr,
               const wxString& aName = wxT( "CairoCanvas" ) );

    ~CAIRO_GAL();

    ///< @copydoc GAL::IsVisible()
    bool IsVisible() const override
    {
        return IsShownOnScreen() && !GetClientRect().IsEmpty();
    }

    void ResizeScreen( int aWidth, int aHeight ) override;

    bool Show( bool aShow ) override;

    int BeginGroup() override;

    void EndGroup() override;

    void SetTarget( RENDER_TARGET aTarget ) override;

    RENDER_TARGET GetTarget() const override;

    void ClearTarget( RENDER_TARGET aTarget ) override;

    /// @copydoc GAL::StartDiffLayer()
    void StartDiffLayer() override;

    /// @copydoc GAL::EndDiffLayer()
    void EndDiffLayer() override;

    /// @copydoc GAL::StartNegativesLayer()
    void StartNegativesLayer() override;

    /// @copydoc GAL::EndNegativesLayer()
    void EndNegativesLayer() override;

    /**
     * Post an event to m_paint_listener.
     *
     * A post is used so that the actual drawing function can use a device context type that
     * is not specific to the wxEVT_PAINT event, just by changing the PostPaint code.
     */
    void PostPaint( wxPaintEvent& aEvent );

    void SetMouseListener( wxEvtHandler* aMouseListener )
    {
        m_mouseListener = aMouseListener;
    }

    void SetPaintListener( wxEvtHandler* aPaintListener )
    {
        m_paintListener = aPaintListener;
    }

    /// @copydoc GAL::SetNativeCursorStyle()
    bool SetNativeCursorStyle( KICURSOR aCursor, bool aHiDPI ) override;

    /// @copydoc GAL::BeginDrawing()
    void BeginDrawing() override;

    /// @copydoc GAL::EndDrawing()
    void EndDrawing() override;

    /// Prepare Cairo surfaces for drawing
    void initSurface();

    /// Destroy Cairo surfaces when are not needed anymore
    void deinitSurface();

    /// Allocate the bitmaps for drawing
    void allocateBitmaps();

    /// Allocate the bitmaps for drawing
    void deleteBitmaps();

    /// Prepare the compositor
    void setCompositor();

    // Event handlers
    /**
     * Paint event handler.
     *
     * @param aEvent is the paint event.
     */
    void onPaint( wxPaintEvent& aEvent );

    /**
     * Mouse event handler, forwards the event to the child.
     *
     * @param aEvent is the mouse event to be forwarded.
     */
    void skipMouseEvent( wxMouseEvent& aEvent );

    /**
     * Skip the gesture event to the parent.
     *
     * @param aEvent is the gesture event.
     */
    void skipGestureEvent( wxGestureEvent& aEvent );

    /**
     * Give the correct cursor image when the native widget asks for it.
     *
     * @param aEvent is the cursor event to plac the cursor into.
     */
    void onSetNativeCursor( wxSetCursorEvent& aEvent );

    ///< Cairo-specific update handlers
    bool updatedGalDisplayOptions( const GAL_DISPLAY_OPTIONS& aOptions ) override;

protected:
    // Compositor related variables
    std::shared_ptr<CAIRO_COMPOSITOR> m_compositor;  ///< Object for layers compositing
    unsigned int        m_mainBuffer;          ///< Handle to the main buffer
    unsigned int        m_overlayBuffer;       ///< Handle to the overlay buffer
    unsigned int        m_tempBuffer;          ///< Handle to the temp buffer
    unsigned int        m_savedBuffer;         ///< Handle to buffer to restore after rendering to temp buffer
    RENDER_TARGET       m_currentTarget;       ///< Current rendering target
    bool                m_validCompositor;     ///< Compositor initialization flag

    // Variables related to wxWidgets
    wxWindow*           m_parentWindow;        ///< Parent window
    wxEvtHandler*       m_mouseListener;       ///< Mouse listener
    wxEvtHandler*       m_paintListener;       ///< Paint listener
    unsigned int        m_bufferSize;          ///< Size of buffers cairoOutput, bitmapBuffers
    unsigned char*      m_wxOutput;            ///< wxImage compatible buffer

    // Variables related to Cairo <-> wxWidgets
    unsigned char*      m_bitmapBuffer;        ///< Storage of the Cairo image
    int                 m_stride;              ///< Stride value for Cairo
    int                 m_wxBufferWidth;
    bool                m_isInitialized;       ///< Are Cairo image & surface ready to use
    COLOR4D             m_backgroundColor;     ///< Background color

    WX_CURSOR_TYPE      m_currentwxCursor;     ///< wx cursor showing the current native cursor
};

} // namespace KIGFX

#endif  // CAIROGAL_H_
