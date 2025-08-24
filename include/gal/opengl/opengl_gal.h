/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2013-2017 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * Graphics Abstraction Layer (GAL) for OpenGL
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

#ifndef OPENGLGAL_H_
#define OPENGLGAL_H_

// GAL imports
#include <gal/gal.h>
#include <gal/graphics_abstraction_layer.h>
#include <gal/gal_display_options.h>
#include <gal/opengl/shader.h>
#include <gal/opengl/vertex_manager.h>
#include <gal/opengl/vertex_item.h>
#include <gal/opengl/cached_container.h>
#include <gal/opengl/noncached_container.h>
#include <gal/opengl/opengl_compositor.h>
#include <gal/hidpi_gl_canvas.h>

#include <unordered_map>
#include <memory>
#include <wx/event.h>

#ifndef CALLBACK
#define CALLBACK
#endif

///< The default number of points for circle approximation
#define SEG_PER_CIRCLE_COUNT  64

struct bitmap_glyph;

namespace KIGFX
{
class SHADER;
class GL_BITMAP_CACHE;


/**
 * OpenGL implementation of the Graphics Abstraction Layer.
 *
 * This is a direct OpenGL-implementation and uses low-level graphics primitives like triangles
 * and quads. The purpose is to provide a fast graphics interface, that takes advantage of modern
 * graphics card GPUs. All methods here benefit thus from the hardware acceleration.
 */
class GAL_API OPENGL_GAL : public GAL, public HIDPI_GL_CANVAS
{
public:
    /**
     * @param aParent is the wxWidgets immediate wxWindow parent of this object.
     *
     * @param aMouseListener is the wxEvtHandler that should receive the mouse events,
     *  this can be can be any wxWindow, but is often a wxFrame container.
     *
     * @param aPaintListener is the wxEvtHandler that should receive the paint
     *  event.  This can be any wxWindow, but is often a derived instance
     *  of this class or a containing wxFrame.  The "paint event" here is
     *  a wxCommandEvent holding EVT_GAL_REDRAW, as sent by PostPaint().
     *
     * @param aName is the name of this window for use by wxWindow::FindWindowByName()
     */
    OPENGL_GAL( const KIGFX::VC_SETTINGS& aVcSettings, GAL_DISPLAY_OPTIONS& aDisplayOptions,
                wxWindow* aParent,
                wxEvtHandler* aMouseListener = nullptr, wxEvtHandler* aPaintListener = nullptr,
                const wxString& aName = wxT( "GLCanvas" ) );

    ~OPENGL_GAL();

    /**
     * Checks OpenGL features.
     *
     * @param aOptions
     * @return wxEmptyString if OpenGL 2.1 or greater is available, otherwise returns error message
     */
    static wxString CheckFeatures( GAL_DISPLAY_OPTIONS& aOptions );

    bool IsOpenGlEngine() override { return true; }

    /// @copydoc GAL::IsInitialized()
    bool IsInitialized() const override
    {
        // is*Initialized flags, but it is enough for OpenGL to show up
        return IsShownOnScreen() && !GetClientRect().IsEmpty();
    }

    ///< @copydoc GAL::IsVisible()
    bool IsVisible() const override
    {
        return IsShownOnScreen() && !GetClientRect().IsEmpty();
    }

    void SetMinLineWidth( float aLineWidth ) override;

    // ---------------
    // Drawing methods
    // ---------------

    /// @copydoc GAL::DrawLine()
    void DrawLine( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint ) override;

    /// @copydoc GAL::DrawSegment()
    void DrawSegment( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint,
                              double aWidth ) override;

    /// @copydoc GAL::DrawSegmentChain()
    void DrawSegmentChain( const std::vector<VECTOR2D>& aPointList, double aWidth ) override;
    void DrawSegmentChain( const SHAPE_LINE_CHAIN& aLineChain, double aWidth ) override;

    /// @copydoc GAL::DrawCircle()
    void DrawCircle( const VECTOR2D& aCenterPoint, double aRadius ) override;

    /// @copydoc GAL::DrawHoleWall()
    void DrawHoleWall( const VECTOR2D& aCenterPoint, double aHoleRadius,
                       double aWallWidth ) override;

    /// @copydoc GAL::DrawArc()
    void DrawArc( const VECTOR2D& aCenterPoint, double aRadius, const EDA_ANGLE& aStartAngle,
                  const EDA_ANGLE& aAngle ) override;

    /// @copydoc GAL::DrawArcSegment()
    void DrawArcSegment( const VECTOR2D& aCenterPoint, double aRadius, const EDA_ANGLE& aStartAngle,
                         const EDA_ANGLE& aAngle, double aWidth, double aMaxError ) override;

    /// @copydoc GAL::DrawRectangle()
    void DrawRectangle( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint ) override;

    /// @copydoc GAL::DrawPolyline()
    void DrawPolyline( const std::deque<VECTOR2D>& aPointList ) override;
    void DrawPolyline( const std::vector<VECTOR2D>& aPointList ) override;
    void DrawPolyline( const VECTOR2D aPointList[], int aListSize ) override;
    void DrawPolyline( const SHAPE_LINE_CHAIN& aLineChain ) override;

    /// @copydoc GAL::DrawPolylines()
    void DrawPolylines( const std::vector<std::vector<VECTOR2D>>& aPointLists ) override;

    /// @copydoc GAL::DrawPolygon()
    void DrawPolygon( const std::deque<VECTOR2D>& aPointList ) override;
    void DrawPolygon( const VECTOR2D aPointList[], int aListSize ) override;
    void DrawPolygon( const SHAPE_POLY_SET& aPolySet, bool aStrokeTriangulation = false ) override;
    void DrawPolygon( const SHAPE_LINE_CHAIN& aPolySet ) override;

    /// @copydoc GAL::DrawGlyph()
    virtual void DrawGlyph( const KIFONT::GLYPH& aGlyph, int aNth, int aTotal ) override;

    /// @copydoc GAL::DrawGlyphs()
    virtual void DrawGlyphs( const std::vector<std::unique_ptr<KIFONT::GLYPH>>& aGlyphs ) override;

    /// @copydoc GAL::DrawCurve()
    void DrawCurve( const VECTOR2D& startPoint, const VECTOR2D& controlPointA,
                            const VECTOR2D& controlPointB, const VECTOR2D& endPoint,
                            double aFilterValue = 0.0 ) override;

    /// @copydoc GAL::DrawBitmap()
    void DrawBitmap( const BITMAP_BASE& aBitmap, double alphaBlend = 1.0 ) override;

    /// @copydoc GAL::BitmapText()
    void BitmapText( const wxString& aText, const VECTOR2I& aPosition,
                     const EDA_ANGLE& aAngle ) override;

    /// @copydoc GAL::DrawGrid()
    void DrawGrid() override;

    // --------------
    // Screen methods
    // --------------

    /// @brief Resizes the canvas.
    void ResizeScreen( int aWidth, int aHeight ) override;

    /// @brief Shows/hides the GAL canvas
    bool Show( bool aShow ) override;

    /// @copydoc GAL::GetSwapInterval()
    int GetSwapInterval() const override { return m_swapInterval; };

    /// @copydoc GAL::Flush()
    void Flush() override;

    /// @copydoc GAL::ClearScreen()
    void ClearScreen( ) override;

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

    /// @copydoc GAL::SetTarget()
    void SetTarget( RENDER_TARGET aTarget ) override;

    /// @copydoc GAL::GetTarget()
    RENDER_TARGET GetTarget() const override;

    /// @copydoc GAL::ClearTarget()
    void ClearTarget( RENDER_TARGET aTarget ) override;

    /// @copydoc GAL::HasTarget()
    virtual bool HasTarget( RENDER_TARGET aTarget ) override;

    /// @copydoc GAL::SetNegativeDrawMode()
    void SetNegativeDrawMode( bool aSetting ) override {}

    /// @copydoc GAL::StartDiffLayer()
    void StartDiffLayer() override;
    //
    /// @copydoc GAL::EndDiffLayer()
    void EndDiffLayer() override;

    void ComputeWorldScreenMatrix() override;

    // -------
    // Cursor
    // -------

    /// @copydoc GAL::SetNativeCursorStyle()
    bool SetNativeCursorStyle( KICURSOR aCursor, bool aHiDPI ) override;

    /// @copydoc GAL::DrawCursor()
    void DrawCursor( const VECTOR2D& aCursorPosition ) override;

    /**
     * Post an event to #m_paint_listener.
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

    void EnableDepthTest( bool aEnabled = false ) override;

    bool IsContextLocked() override
    {
        return m_isContextLocked;
    }

    void LockContext( int aClientCookie ) override;

    void UnlockContext( int aClientCookie ) override;

    /// @copydoc GAL::BeginDrawing()
    void BeginDrawing() override;

    /// @copydoc GAL::EndDrawing()
    void EndDrawing() override;

    ///< Parameters passed to the GLU tesselator
    struct TessParams
    {
        /// Manager used for storing new vertices
        VERTEX_MANAGER* vboManager;

        /// Intersect points, that have to be freed after tessellation
        std::deque<std::shared_ptr<GLdouble>>& intersectPoints;
    };

private:
    /// Super class definition
    typedef GAL super;

    static wxGLContext*     m_glMainContext;    ///< Parent OpenGL context
    wxGLContext*            m_glPrivContext;    ///< Canvas-specific OpenGL context
    int                     m_swapInterval;     ///< Used to store swap interval information
    static int              m_instanceCounter;  ///< GL GAL instance counter
    wxEvtHandler*           m_mouseListener;
    wxEvtHandler*           m_paintListener;

    static GLuint           g_fontTexture;      ///< Bitmap font texture handle (shared)

    // Vertex buffer objects related fields
    typedef std::unordered_map< unsigned int, std::shared_ptr<VERTEX_ITEM> > GROUPS_MAP;

    GROUPS_MAP              m_groups;           ///< Stores information about VBO objects (groups)
    unsigned int            m_groupCounter;     ///< Counter used for generating keys for groups
    VERTEX_MANAGER*         m_currentManager;   ///< Currently used VERTEX_MANAGER (for storing
                                                ///< VERTEX_ITEMs).
    VERTEX_MANAGER*         m_cachedManager;    ///< Container for storing cached VERTEX_ITEMs
    VERTEX_MANAGER*         m_nonCachedManager; ///< Container for storing non-cached VERTEX_ITEMs
    VERTEX_MANAGER*         m_overlayManager;   ///< Container for storing overlaid VERTEX_ITEMs

    /// Container for storing temp (diff mode) VERTEX_ITEMs
    VERTEX_MANAGER*         m_tempManager;

    // Framebuffer & compositing
    OPENGL_COMPOSITOR*      m_compositor;       ///< Handles multiple rendering targets
    unsigned int            m_mainBuffer;       ///< Main rendering target
    unsigned int            m_overlayBuffer;    ///< Auxiliary rendering target (for menus etc.)
    unsigned int            m_tempBuffer;       ///< Temporary rendering target (for diffing etc.)
    RENDER_TARGET           m_currentTarget;    ///< Current rendering target

    // Shader
    /// There is only one shader used for different objects.
    SHADER*                 m_shader;

    // Internal flags
    bool                    m_isFramebufferInitialized; ///< Are the framebuffers initialized?
    static bool             m_isBitmapFontLoaded;       ///< Is the bitmap font texture loaded?
    bool                    m_isBitmapFontInitialized;  ///< Is the shader set to use bitmap fonts?
    bool                    m_isInitialized;            ///< Basic initialization flag, has to be
                                                        ///< done when the window is visible
    bool                    m_isGrouping;               ///< Was a group started?
    bool                    m_isContextLocked;          ///< Used for assertion checking
    int                     m_lockClientCookie;
    GLint                   ufm_worldPixelSize;
    GLint                   ufm_screenPixelSize;
    GLint                   ufm_pixelSizeMultiplier;
    GLint                   ufm_antialiasingOffset;
    GLint                   ufm_minLinePixelWidth;
    GLint                   ufm_fontTexture;
    GLint                   ufm_fontTextureWidth;

    /// wx cursor showing the current native cursor.
    WX_CURSOR_TYPE          m_currentwxCursor;

    std::unique_ptr<GL_BITMAP_CACHE>            m_bitmapCache;

    // Polygon tesselation
    GLUtesselator*                        m_tesselator;
    std::deque<std::shared_ptr<GLdouble>> m_tessIntersects;

    /// @copydoc GAL::BeginUpdate()
    void beginUpdate() override;

    /// @copydoc GAL::EndUpdate()
    void endUpdate() override;

    ///< Update handler for OpenGL settings
    bool updatedGalDisplayOptions( const GAL_DISPLAY_OPTIONS& aOptions ) override;

    /**
     * Draw a quad for the line.
     *
     * @param aStartPoint is the start point of the line.
     * @param aEndPoint is the end point of the line.
     * @param aReserve if set to false, call reserveLineQuads beforehand
     * to reserve the right amount of vertices.
     */
    void drawLineQuad( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint,
                       bool aReserve = true );

    /**
     * Reserve specified number of line quads.
     *
     * @param aLineCount the number of line quads to reserve.
     */
    void reserveLineQuads( const int aLineCount );

    /**
     * Draw a semicircle.
     *
     * Depending on settings (m_isStrokeEnabled & isFilledEnabled) it runs the proper function
     * (drawStrokedSemiCircle or drawFilledSemiCircle).
     *
     * @param aCenterPoint is the center point.
     * @param aRadius is the radius of the semicircle.
     * @param aAngle is the angle of the semicircle.
     *
     */
    void drawSemiCircle( const VECTOR2D& aCenterPoint, double aRadius, double aAngle );

    /**
     *Draw a filled semicircle.
     *
     * @param aCenterPoint is the center point.
     * @param aRadius is the radius of the semicircle.
     * @param aAngle is the angle of the semicircle.
     *
     */
    void drawFilledSemiCircle( const VECTOR2D& aCenterPoint, double aRadius, double aAngle );

    /**
     * Draw a stroked semicircle.
     *
     * @param aCenterPoint is the center point.
     * @param aRadius is the radius of the semicircle.
     * @param aAngle is the angle of the semicircle.
     * @param aReserve if set to false, reserve 3 vertices for each semicircle.
     *
     */
    void drawStrokedSemiCircle( const VECTOR2D& aCenterPoint, double aRadius, double aAngle,
                                bool aReserve = true );

    /**
     * Internal method for circle drawing.
     *
     * @param aReserve if set to false, reserve 3 vertices for each circle.
     */
    void drawCircle( const VECTOR2D& aCenterPoint, double aRadius, bool aReserve = true );

    /**
     * Generic way of drawing a polyline stored in different containers.
     *
     * @param aPointGetter is a function to obtain coordinates of n-th vertex.
     * @param aPointCount is the number of points to be drawn.
     * @param aReserve if set to false, reserve aPointCount - 1 line quads.
     */
    void drawPolyline( const std::function<VECTOR2D( int )>& aPointGetter, int aPointCount,
                       bool aReserve = true );

    /**
     * Generic way of drawing a chain of segments stored in different containers.
     *
     * @param aPointGetter is a function to obtain coordinates of n-th vertex.
     * @param aPointCount is the number of points to be drawn.
     * @param aReserve if set to false, do not reserve vertices internally.
     */
    void drawSegmentChain( const std::function<VECTOR2D( int )>& aPointGetter, int aPointCount,
                           double aWidth, bool aReserve = true );

    /**
     * Internal method for segment drawing
     */
    void drawSegment( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint, double aWidth,
                      bool aReserve = true );

    /**
     * Draw a filled polygon. It does not need the last point to have the same coordinates
     * as the first one.
     *
     * @param aPoints is the vertices data (3 coordinates: x, y, z).
     * @param aPointCount is the number of points.
     */
    void drawPolygon( GLdouble* aPoints, int aPointCount );

    /**
     * Draw a set of polygons with a cached triangulation. Way faster than drawPolygon.
     *
     * @param aStrokeTriangulation indicates the triangulation should be stroked rather than
     *                             filled.  Used for debugging.
     */
    void drawTriangulatedPolyset( const SHAPE_POLY_SET& aPoly, bool aStrokeTriangulation );

    /**
     * Draw a single character using bitmap font.
     *
     * Its main purpose is to be used in BitmapText() function.
     *
     * @param aChar is the character to be drawn.
     * @return Width of the drawn glyph.
     * @param aReserve if set to false, reserve 6 vertices for each character.
     */
    int drawBitmapChar( unsigned long aChar, bool aReserve = true );

    /**
     * Draw an overbar over the currently drawn text.
     *
     * Its main purpose is to be used in BitmapText() function.
     * This method requires appropriate scaling to be applied (as is done in BitmapText() function).
     * The current X coordinate will be the overbar ending.
     *
     * @param aLength is the width of the overbar.
     * @param aHeight is the height for the overbar.
     * @param aReserve if set to false, reserve 6 vertices for each overbar.
     */
    void drawBitmapOverbar( double aLength, double aHeight, bool aReserve = true );

    /**
     * Compute a size of text drawn using bitmap font with current text setting applied.
     *
     * @param aText is the text to be drawn.
     * @return Pair containing text bounding box and common Y axis offset. The values are expressed
     * as a number of pixels on the bitmap font texture and need to be scaled before drawing.
     */
    std::pair<VECTOR2D, float> computeBitmapTextSize( const UTF8& aText ) const;

    // Event handling
    /**
     * This is the OnPaint event handler.
     *
     * @param aEvent is the OnPaint event.
     */
    void onPaint( wxPaintEvent& aEvent );

    /**
     * Skip the mouse event to the parent.
     *
     * @param aEvent is the mouse event.
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

    /**
     * Blit cursor into the current screen.
     */
    void blitCursor();

    /**
     * Return a valid key that can be used as a new group number.
     *
     * @return An unique group number that is not used by any other group.
     */
    unsigned int getNewGroupNumber();

    /**
     * Compute the angle step when drawing arcs/circles approximated with lines.
     */
    double calcAngleStep( double aRadius ) const
    {
        // Bigger arcs need smaller alpha increment to make them look smooth
        return std::min( 1e6 / aRadius, 2.0 * M_PI / SEG_PER_CIRCLE_COUNT );
    }

    double getWorldPixelSize() const;

    VECTOR2D getScreenPixelSize() const;

    /**
     * Set up the shader parameters for OpenGL rendering.
     * This method initializes all the uniform parameter locations
     * after the shader has been linked.
     */
    void setupShaderParameters();

    /**
     * Basic OpenGL initialization and feature checks.
     *
     * @throw std::runtime_error if any of the OpenGL feature checks failed
     */
    void init();
};
} // namespace KIGFX

#endif  // OPENGLGAL_H_
