/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * Graphics Abstraction Layer (GAL) - base class
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

#ifndef GRAPHICSABSTRACTIONLAYER_H_
#define GRAPHICSABSTRACTIONLAYER_H_

#include <deque>
#include <stack>
#include <limits>

#include <math/matrix3x3.h>

#include <gal/gal.h>
#include <gal/color4d.h>
#include <gal/cursors.h>
#include <gal/definitions.h>
#include <gal/gal_display_options.h>
#include <font/stroke_font.h>
#include <geometry/eda_angle.h>
#include <pgm_base.h>
#include <settings/common_settings.h>

class SHAPE_LINE_CHAIN;
class SHAPE_POLY_SET;
class BITMAP_BASE;

namespace KIGFX
{
/**
 * Abstract interface for drawing on a 2D-surface.
 *
 * The functions are optimized for drawing shapes of an EDA-program such as KiCad. Most methods
 * are abstract and need to be implemented by a lower layer, for example by a Cairo or OpenGL
 * implementation.  Almost all methods use world coordinates as arguments. The board design is
 * defined in world space units for drawing purposes these are transformed to screen units with
 * this layer. So zooming is handled here as well.
 *
 */
class GAL_API GAL : GAL_DISPLAY_OPTIONS_OBSERVER
{
    // These friend declarations allow us to hide routines that should not be called.  The
    // corresponding RAII objects must be used instead.
    friend class GAL_CONTEXT_LOCKER;
    friend class GAL_UPDATE_CONTEXT;
    friend class GAL_DRAWING_CONTEXT;

public:
    // Constructor / Destructor
    GAL( GAL_DISPLAY_OPTIONS& aOptions );
    virtual ~GAL();

    /// Return the initialization status for the canvas.
    virtual bool IsInitialized() const { return true; }

    /// Return true if the GAL canvas is visible on the screen.
    virtual bool IsVisible() const { return true; }

    /// Return true if the GAL engine is a Cairo based type.
    virtual bool IsCairoEngine() { return false; }

    /// Return true if the GAL engine is a OpenGL based type.
    virtual bool IsOpenGlEngine() { return false; }

    // ---------------
    // Drawing methods
    // ---------------

    /**
     * Draw a line.
     *
     * Start and end points are defined as 2D-Vectors.
     *
     * @param aStartPoint   is the start point of the line.
     * @param aEndPoint     is the end point of the line.
     */
    virtual void DrawLine( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint ) {};

    /**
     * Draw a rounded segment.
     *
     * Start and end points are defined as 2D-Vectors.
     *
     * @param aStartPoint   is the start point of the segment.
     * @param aEndPoint     is the end point of the segment.
     * @param aWidth        is a width of the segment
     */
    virtual void DrawSegment( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint,
                              double aWidth ){};

    /**
     * Draw a chain of rounded segments.
     *
     * @param aPointList is a list of 2D-Vectors containing the chain points.
     * @param aWidth     is a width of the segments
     */
    virtual void DrawSegmentChain( const std::vector<VECTOR2D>& aPointList, double aWidth ){};
    virtual void DrawSegmentChain( const SHAPE_LINE_CHAIN& aLineChain, double aWidth ){};

    /**
     * Draw a polyline
     *
     * @param aPointList is a list of 2D-Vectors containing the polyline points.
     */
    virtual void DrawPolyline( const std::deque<VECTOR2D>& aPointList ) {};
    virtual void DrawPolyline( const std::vector<VECTOR2D>& aPointList ) {};
    virtual void DrawPolyline( const VECTOR2D aPointList[], int aListSize ) {};
    virtual void DrawPolyline( const SHAPE_LINE_CHAIN& aLineChain ) {};

    /**
     * Draw multiple polylines
     *
     * @param aPointLists are lists of 2D-Vectors containing the polyline points.
     */
    virtual void DrawPolylines( const std::vector<std::vector<VECTOR2D>>& aPointLists ){};

    /**
     * Draw a circle using world coordinates.
     *
     * @param aCenterPoint is the center point of the circle.
     * @param aRadius is the radius of the circle.
     */
    virtual void DrawCircle( const VECTOR2D& aCenterPoint, double aRadius ) {};

    /**
     * Draw a hole wall ring.
     *
     * @param aCenterPoint is the center point of the hole.
     * @param aHoleRadius is the radius of the hole.
     * @param aWallWidth is the wall thickness.
     */
    virtual void DrawHoleWall( const VECTOR2D& aCenterPoint, double aHoleRadius,
                               double aWallWidth ) {};

    /**
     * Draw an arc.
     *
     * @param aCenterPoint  is the center point of the arc.
     * @param aRadius       is the arc radius.
     * @param aStartAngle   is the start angle of the arc.
     * @param aAngle        is the angle of the arc.
     */
    virtual void DrawArc( const VECTOR2D& aCenterPoint, double aRadius,
                          const EDA_ANGLE& aStartAngle, const EDA_ANGLE& aAngle ){};

    /**
     * Draw an arc segment.
     *
     * This method differs from DrawArc() in what happens when fill/stroke are on or off.
     * DrawArc() draws a "pie piece" when fill is turned on, and a thick stroke when fill is off.
     * DrawArcSegment() with fill *on* behaves like DrawArc() with fill *off*.
     * DrawArcSegment() with fill *off* draws the outline of what it would have drawn with fill on.
     *
     * This has meaning only for back ends that can't draw a true arc, and use segments to
     * approximate.
     *
     * TODO: Unify Arc routines
     *
     * @param aCenterPoint  is the center point of the arc.
     * @param aRadius       is the arc radius.
     * @param aStartAngle   is the start angle of the arc.
     * @param aAngle        is the angle of the arc.
     * @param aWidth        is the thickness of the arc (pen size).
     * @param aMaxError     is the max allowed error to create segments to approximate a circle.
     */
    virtual void DrawArcSegment( const VECTOR2D& aCenterPoint, double aRadius,
                                 const EDA_ANGLE& aStartAngle, const EDA_ANGLE& aAngle,
                                 double aWidth, double aMaxError ){};

    /**
     * Draw a rectangle.
     *
     * @param aStartPoint   is the start point of the rectangle.
     * @param aEndPoint     is the end point of the rectangle.
     */
    virtual void DrawRectangle( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint ) {};

    void DrawRectangle( const BOX2I& aRect )
    {
        DrawRectangle( aRect.GetOrigin(), aRect.GetEnd() );
    }

    /**
     * Draw a polygon representing a font glyph.
     */
    virtual void DrawGlyph( const KIFONT::GLYPH& aGlyph, int aNth = 0, int aTotal = 1 ) {};

    /**
     * Draw polygons representing font glyphs.
     */
    virtual void DrawGlyphs( const std::vector<std::unique_ptr<KIFONT::GLYPH>>& aGlyphs )
    {
        COLOR4D fillColor = GetFillColor();
        COLOR4D strokeColor = GetStrokeColor();

        for( size_t i = 0; i < aGlyphs.size(); i++ )
        {
            if( aGlyphs[i]->IsHover() )
            {
                SetFillColor( m_hoverColor );
                SetStrokeColor( m_hoverColor );
            }

            DrawGlyph( *aGlyphs[i], i, aGlyphs.size() );

            if( aGlyphs[i]->IsHover() )
            {
                SetFillColor( fillColor );
                SetStrokeColor( strokeColor );
            }
        }
    }


    /**
     * Draw a polygon.
     *
     * @param aPointList is the list of the polygon points.
     */
    virtual void DrawPolygon( const std::deque<VECTOR2D>& aPointList ) {};
    virtual void DrawPolygon( const VECTOR2D aPointList[], int aListSize ) {};
    virtual void DrawPolygon( const SHAPE_POLY_SET& aPolySet,
                              bool aStrokeTriangulation = false ) {};
    virtual void DrawPolygon( const SHAPE_LINE_CHAIN& aPolySet ) {};

    /**
     * Draw a cubic bezier spline.
     *
     * @param startPoint    is the start point of the spline.
     * @param controlPointA is the first control point.
     * @param controlPointB is the second control point.
     * @param endPoint      is the end point of the spline.
     * @param aFilterValue  is used by Bezier to segments approximation, if
     * the Bezier curve is not supported and needs a curve to polyline conversion.
     * aFilterValue = 0 means no filtering.
     */
    virtual void DrawCurve( const VECTOR2D& startPoint,    const VECTOR2D& controlPointA,
                            const VECTOR2D& controlPointB, const VECTOR2D& endPoint,
                            double aFilterValue = 0.0 ) {};

    /**
     * Draw a bitmap image.
     */
    virtual void DrawBitmap( const BITMAP_BASE& aBitmap, double alphaBlend = 1.0 ) {};

    // --------------
    // Screen methods
    // --------------

    /// Resize the canvas.
    virtual void ResizeScreen( int aWidth, int aHeight ) {};

    /// Show/hide the GAL canvas
    virtual bool Show( bool aShow ) { return true; };

    /// Return GAL canvas size in pixels
    const VECTOR2I& GetScreenPixelSize() const
    {
        return m_screenSize;
    }

    /// Return the swap interval. -1 for adaptive, 0 for disabled/unknown
    virtual int GetSwapInterval() const { return 0; };

    /// Force all remaining objects to be drawn.
    virtual void Flush() {};

    void SetClearColor( const COLOR4D& aColor )
    {
        m_clearColor = aColor;
    }

    const COLOR4D& GetClearColor( ) const
    {
        return m_clearColor;
    }

    /**
     * Clear the screen.
     *
     * @param aColor is the color used for clearing.
     */
    virtual void ClearScreen() {};

    // -----------------
    // Attribute setting
    // -----------------

    /**
     * Enable/disable fill.
     *
     * @param aIsFillEnabled is true, when the graphics objects should be filled, else false.
     */
    virtual void SetIsFill( bool aIsFillEnabled )
    {
        m_isFillEnabled = aIsFillEnabled;
    }

    /**
     * Get the fill status.
     *
     * @return true if fill is enabled, false otherwise.
     */
    inline bool GetIsFill() const
    {
        return m_isFillEnabled;
    }

    /**
     * Enable/disable stroked outlines.
     *
     * @param aIsStrokeEnabled is true, if the outline of an object should be stroked.
     */
    virtual void SetIsStroke( bool aIsStrokeEnabled )
    {
        m_isStrokeEnabled = aIsStrokeEnabled;
    }

    /**
     * Get the stroke status.
     *
     * @return true if stroke is enabled, false otherwise.
     */
    inline bool GetIsStroke() const
    {
        return m_isStrokeEnabled;
    }

    /**
     * Set the fill color.
     *
     * @param aColor is the color for filling.
     */
    virtual void SetFillColor( const COLOR4D& aColor )
    {
        m_fillColor = aColor;
    }

    /**
     * Get the fill color.
     *
     * @return the color for filling a outline.
     */
    inline const COLOR4D& GetFillColor() const
    {
        return m_fillColor;
    }

    /**
     * Set the stroke color.
     *
     * @param aColor is the color for stroking the outline.
     */
    virtual void SetStrokeColor( const COLOR4D& aColor )
    {
        m_strokeColor = aColor;
    }

    virtual void SetHoverColor( const COLOR4D& aColor )
    {
        m_hoverColor = aColor;
    }

    /**
     * Get the stroke color.
     *
     * @return the color for stroking the outline.
     */
    inline const COLOR4D& GetStrokeColor() const
    {
        return m_strokeColor;
    }

    /**
     * Set the line width.
     *
     * @param aLineWidth is the line width.
     */
    virtual void SetLineWidth( float aLineWidth )
    {
        m_lineWidth = aLineWidth;
    }

    /**
     * Set the minimum line width in pixels.
     *
     * @param aLineWidth is the minimum line width.
     */
    virtual void SetMinLineWidth( float aLineWidth )
    {
        m_minLineWidth = aLineWidth;
    }

    /**
     * Get the line width.
     *
     * @return the actual line width.
     */
    inline float GetLineWidth() const
    {
        return m_lineWidth;
    }

    /**
     * Get the minimum line width in pixels.
     *
     * @return the minimum line width.
     */
    inline float GetMinLineWidth() const
    {
        return m_minLineWidth;
    }

    /**
     * Set the depth of the layer (position on the z-axis)
     *
     * If you do this, you should consider using a GAL_SCOPED_ATTR to ensure
     * the depth is reset to the original value.
     *
     * @param aLayerDepth the layer depth for the objects. Smaller is closer to the viewer.
     */
    virtual void SetLayerDepth( double aLayerDepth )
    {
        wxCHECK_MSG( aLayerDepth <= m_depthRange.y, /*void*/,
                     wxT( "SetLayerDepth: below minimum" ) );
        wxCHECK_MSG( aLayerDepth >= m_depthRange.x, /*void*/,
                     wxT( "SetLayerDepth: above maximum" ) );

        m_layerDepth = aLayerDepth;
    }

    /**
     * Change the current depth to deeper, so it is possible to draw objects right beneath
     * other.
     *
     * If you do this, you should consider using a GAL_SCOPED_ATTR to ensure the depth
     * is reset to the original value.
     */
    inline void AdvanceDepth()
    {
        SetLayerDepth( m_layerDepth - 0.1 );
    }

    // ----
    // Text
    // ----

    /**
     * Draw a text using a bitmap font. It should be faster than StrokeText(), but can be used
     * only for non-Gerber elements.
     *
     * @param aText is the text to be drawn.
     * @param aPosition is the text position in world coordinates.
     * @param aAngle is the text rotation angle.
     */
    virtual void BitmapText( const wxString& aText, const VECTOR2I& aPosition,
                             const EDA_ANGLE& aAngle );

    /**
     * Reset text attributes to default styling.  FONT TODO: do we need any of this in GAL anymore?
     *
     * Normally, custom attributes will be set individually after this, otherwise you can use
     * SetTextAttributes()
     */
    void ResetTextAttributes();

    void SetGlyphSize( const VECTOR2I aSize )         { m_attributes.m_Size = aSize; }
    const VECTOR2I& GetGlyphSize() const              { return m_attributes.m_Size; }

    inline void SetFontBold( const bool aBold )       { m_attributes.m_Bold = aBold; }
    inline bool IsFontBold() const                    { return m_attributes.m_Bold; }

    inline void SetFontItalic( bool aItalic )         { m_attributes.m_Italic = aItalic; }
    inline bool IsFontItalic() const                  { return m_attributes.m_Italic; }

    inline void SetFontUnderlined( bool aUnderlined ) { m_attributes.m_Underlined = aUnderlined; }
    inline bool IsFontUnderlined() const              { return m_attributes.m_Underlined; }

    void SetTextMirrored( const bool aMirrored )      { m_attributes.m_Mirrored = aMirrored; }
    bool IsTextMirrored() const                       { return m_attributes.m_Mirrored; }

    void SetHorizontalJustify( const GR_TEXT_H_ALIGN_T aHorizontalJustify )
    {
        m_attributes.m_Halign = aHorizontalJustify;
    }

    GR_TEXT_H_ALIGN_T GetHorizontalJustify() const { return m_attributes.m_Halign; }

    void SetVerticalJustify( const GR_TEXT_V_ALIGN_T aVerticalJustify )
    {
        m_attributes.m_Valign = aVerticalJustify;
    }

    GR_TEXT_V_ALIGN_T GetVerticalJustify() const { return m_attributes.m_Valign; }


    // --------------
    // Transformation
    // --------------

    /**
     * Transform the context.
     *
     * @param aTransformation is the transformation matrix.
     */
    virtual void Transform( const MATRIX3x3D& aTransformation ) {};

    /**
     * Rotate the context.
     *
     * @param aAngle is the rotation angle in radians.
     */
    virtual void Rotate( double aAngle ) {};

    /**
     * Translate the context.
     *
     * @param aTranslation is the translation vector.
     */
    virtual void Translate( const VECTOR2D& aTranslation ) {};

    /**
     * Scale the context.
     *
     * @param aScale is the scale factor for the x- and y-axis.
     */
    virtual void Scale( const VECTOR2D& aScale ) {};

    /// Save the context.
    virtual void Save() {};

    /// Restore the context.
    virtual void Restore() {};

    // --------------------------------------------
    // Group methods
    // ---------------------------------------------

    /**
     * Begin a group.
     *
     * A group is a collection of graphic items.
     * Hierarchical groups are possible, attributes and transformations can be used.
     *
     * @return the number of the group.
     */
    virtual int BeginGroup() { return 0; };

    /// End the group.
    virtual void EndGroup() {};

    /**
     * Draw the stored group.
     *
     * @param aGroupNumber is the group number.
     */
    virtual void DrawGroup( int aGroupNumber ) {};

    /**
     * Change the color used to draw the group.
     *
     * @param aGroupNumber is the group number.
     * @param aNewColor is the new color.
     */
    virtual void ChangeGroupColor( int aGroupNumber, const COLOR4D& aNewColor ) {};

    /**
     * Change the depth (Z-axis position) of the group.
     *
     * @param aGroupNumber is the group number.
     * @param aDepth is the new depth.
     */
    virtual void ChangeGroupDepth( int aGroupNumber, int aDepth ) {};

    /**
     * Delete the group from the memory.
     *
     * @param aGroupNumber is the group number.
     */
    virtual void DeleteGroup( int aGroupNumber ) {};

    /**
     * Delete all data created during caching of graphic items.
     */
    virtual void ClearCache() {};

    // --------------------------------------------------------
    // Handling the world <-> screen transformation
    // --------------------------------------------------------

    /// Compute the world <-> screen transformation matrix
    virtual void ComputeWorldScreenMatrix();

    /**
     * Get the world <-> screen transformation matrix.
     *
     * @return the transformation matrix.
     */
    const MATRIX3x3D& GetWorldScreenMatrix() const
    {
        return m_worldScreenMatrix;
    }

    /**
     * Get the screen <-> world transformation matrix.
     *
     * @return the transformation matrix.
     */
    const MATRIX3x3D& GetScreenWorldMatrix() const
    {
        return m_screenWorldMatrix;
    }

    /**
     * Set the world <-> screen transformation matrix.
     *
     * @param aMatrix is the 3x3 world <-> screen transformation matrix.
     */
    inline void SetWorldScreenMatrix( const MATRIX3x3D& aMatrix )
    {
        m_worldScreenMatrix = aMatrix;
    }

    /**
     * @return the bounding box of the world that is displayed on screen at the moment
     */
    BOX2D GetVisibleWorldExtents() const;

    /**
     * Set the unit length.
     *
     * This defines the length [inch] per one integer. For instance a value 0.001 means
     * that the coordinate [1000, 1000] corresponds with a point at (1 inch, 1 inch) or
     * 1 mil resolution per integer.
     */
    void SetWorldUnitLength( double aWorldUnitLength ) { m_worldUnitLength = aWorldUnitLength; }

    void SetScreenSize( const VECTOR2I& aSize ) { m_screenSize = aSize; }

    /**
     * Set the dots per inch of the screen.
     *
     * This value depends on the user screen, it should be configurable by the application.
     * For instance a typical notebook with HD+ resolution (1600x900) has 106 DPI.
     */
    void SetScreenDPI( double aScreenDPI ) { m_screenDPI = aScreenDPI; }
    double GetScreenDPI() const            { return m_screenDPI; }

    /**
     * Get/set the Point in world space to look at.
     *
     * This point corresponds with the center of the actual drawing area.
     */
    void SetLookAtPoint( const VECTOR2D& aPoint ) { m_lookAtPoint = aPoint; }
    const VECTOR2D& GetLookAtPoint() const        { return m_lookAtPoint; }

    void SetZoomFactor( double aZoomFactor )      { m_zoomFactor = aZoomFactor; }
    double GetZoomFactor() const                  { return m_zoomFactor; }

    /**
     * Get/set the rotation angle (in radians).
     */
    void SetRotation( double aRotation )          { m_rotation = aRotation; }
    double GetRotation() const                    { return m_rotation; }

    /**
     * Set the range of the layer depth.
     *
     * Usually required for the OpenGL implementation, any object outside this range is not drawn.
     *
     * The MinDepth (x) is closest to the clipping plane (top) while the MaxDepth (y) is farthest
     * from the clipping plane (bottom).
     */
    void SetDepthRange( const VECTOR2D& aDepthRange ) { m_depthRange = aDepthRange; }
    double GetMinDepth() const                        { return m_depthRange.x; }
    double GetMaxDepth() const                        { return m_depthRange.y; }

    /**
     * Get the world scale.
     *
     * @return the actual world scale factor.
     */
    double GetWorldScale() const { return m_worldScale; }

    /**
     * Sets flipping of the screen.
     *
     * @param xAxis is the flip flag for the X axis.
     * @param yAxis is the flip flag for the Y axis.
     */
    inline void SetFlip( bool xAxis, bool yAxis )
    {
        m_globalFlipX = xAxis;
        m_globalFlipY = yAxis;
    }

    bool IsFlippedX() const { return m_globalFlipX; }
    bool IsFlippedY() const { return m_globalFlipY; }

    // ---------------------------
    // Buffer manipulation methods
    // ---------------------------

    /**
     * Set the target for rendering.
     *
     * @param aTarget is the new target for rendering.
     */
    virtual void SetTarget( RENDER_TARGET aTarget ) {};

    /**
     * Get the currently used target for rendering.
     *
     * @return The current rendering target.
     */
    virtual RENDER_TARGET GetTarget() const { return TARGET_CACHED; };

    /**
     * Clear the target for rendering.
     *
     * @param aTarget is the target to be cleared.
     */
    virtual void ClearTarget( RENDER_TARGET aTarget ) {};

    /**
     * Return true if the target exists.
     *
     * @param aTarget is the target to be checked.
     */
    virtual bool HasTarget( RENDER_TARGET aTarget )
    {
        return true;
    };

    /**
     * Set negative draw mode in the renderer.
     *
     * When negative mode is enabled, drawn items will subtract from
     * previously drawn items.  This is mainly needed for Gerber
     * negative item support in Cairo, since unlike in OpenGL, objects
     * drawn with zero opacity on top of other objects would not normally
     * mask objects in Cairo.  This method is a no-op in OpenGL.
     *
     * @param aSetting is true if negative mode should be enabled
     */
    virtual void SetNegativeDrawMode( bool aSetting ) {};

    /**
     * Begins rendering of a differential layer. Used by gerbview's differential mode.
     *
     * Differential layers have their drawn objects blended onto the lower layers
     * differently so we need to end drawing of current objects and start a new
     * set to be completed with a different blend mode.
     */
    virtual void StartDiffLayer() {};

    /**
     * Ends rendering of a differential layer. Objects drawn after the StartDiffLayer()
     * will be drawn and composited with a differential blend mode, then drawing is
     * returned to normal.
     */
    virtual void EndDiffLayer() {};

    /**
     * Begins rendering in a new layer that will be copied to the main
     * layer in EndNegativesLayer().
     *
     * For Cairo, layers with negative items need a new layer so when
     * negative layers _CLEAR sections it doesn't delete drawings on layers
     * below them. No-op in OpenGL
     */
    virtual void StartNegativesLayer(){};

    /**
     * Ends rendering of a negatives layer and draws it to the main layer.
     * No-op in OpenGL.
     */
    virtual void EndNegativesLayer(){};

    // -------------
    // Grid methods
    // -------------

    /**
     * Set the visibility setting of the grid.
     *
     * @param aVisibility is the new visibility setting of the grid.
     */
    void SetGridVisibility( bool aVisibility ) { m_gridVisibility = aVisibility; }

    bool GetGridVisibility() const { return m_gridVisibility; }

    bool GetGridSnapping() const
    {
        return m_options.m_gridSnapping == KIGFX::GRID_SNAPPING::ALWAYS ||
                 ( m_gridVisibility && m_options.m_gridSnapping == KIGFX::GRID_SNAPPING::WITH_GRID );
    }

    /**
     * Set the origin point for the grid.
     *
     * @param aGridOrigin is a vector containing the grid origin point, in world coordinates.
     */
    inline void SetGridOrigin( const VECTOR2D& aGridOrigin )
    {
        m_gridOrigin = aGridOrigin;

        if( m_gridSize.x == 0.0 || m_gridSize.y == 0.0 )
        {
            m_gridOffset = VECTOR2D( 0.0, 0.0);
        }
        else
        {
            m_gridOffset = VECTOR2D( (long) m_gridOrigin.x % (long) m_gridSize.x,
                                     (long) m_gridOrigin.y % (long) m_gridSize.y );
        }
    }

    inline const VECTOR2D& GetGridOrigin() const
    {
        return m_gridOrigin;
    }

    /**
     * Set the grid size.
     *
     * @param aGridSize is a vector containing the grid size in x and y direction.
     */
    inline void SetGridSize( const VECTOR2D& aGridSize )
    {
        m_gridSize = aGridSize;

        // Avoid stupid grid size values: a grid size  should be >= 1 in internal units
        m_gridSize.x = std::max( 1.0, m_gridSize.x );
        m_gridSize.y = std::max( 1.0, m_gridSize.y );

        m_gridOffset = VECTOR2D( (long) m_gridOrigin.x % (long) m_gridSize.x,
                                 (long) m_gridOrigin.y % (long) m_gridSize.y );
    }

    /**
     * Return the grid size.
     *
     * @return A vector containing the grid size in x and y direction.
     */
    inline const VECTOR2D& GetGridSize() const
    {
        return m_gridSize;
    }

    /**
     * Return the visible grid size in x and y directions
     *
     * @return A vector containing the spacing of visible grid marks
     */
    inline VECTOR2D GetVisibleGridSize() const
    {
        VECTOR2D gridScreenSize( m_gridSize );
        gridScreenSize.x = std::max( 100.0, gridScreenSize.x );
        gridScreenSize.y = std::max( 100.0, gridScreenSize.y );

        double gridThreshold = computeMinGridSpacing() / m_worldScale;

        if( m_gridStyle == GRID_STYLE::SMALL_CROSS )
            gridThreshold *= 2.0;

        // If we cannot display the grid density, scale down by a tick size and
        // try again.  Eventually, we get some representation of the grid
        while( std::min( gridScreenSize.x, gridScreenSize.y ) <= gridThreshold )
        {
            gridScreenSize = gridScreenSize * static_cast<double>( m_gridTick );
        }

        return gridScreenSize;
    }

    /**
     * Set the grid color.
     *
     * @param aGridColor is the grid color, it should have a low alpha value for the best effect.
     */
    inline void SetGridColor( const COLOR4D& aGridColor )
    {
        m_gridColor = aGridColor;
    }

    /**
     * Set the axes color.
     *
     * @param aAxesColor is the color to draw the axes if enabled.
     */
    inline void SetAxesColor( const COLOR4D& aAxesColor )
    {
        m_axesColor = aAxesColor;
    }

    /**
     * Enable drawing the axes.
     */
    inline void SetAxesEnabled( bool aAxesEnabled )
    {
        m_axesEnabled = aAxesEnabled;
    }

    /**
     * Draw every tick line wider.
     *
     * @param aInterval increase the width of every aInterval line, if 0 do not use this feature.
     */
    inline void SetCoarseGrid( int aInterval )
    {
        m_gridTick = aInterval;
    }

    /**
     * Get the grid line width.
     *
     * @return the grid line width
     */
    inline float GetGridLineWidth() const
    {
        return m_gridLineWidth;
    }

    ///< Draw the grid
    virtual void DrawGrid() {};

    /**
     * For a given point it returns the nearest point belonging to the grid in world coordinates.
     *
     * @param aPoint is the point for which the grid point is searched.
     * @return The nearest grid point in world coordinates.
     */
    VECTOR2D GetGridPoint( const VECTOR2D& aPoint ) const;

    /**
     * Compute the point position in world coordinates from given screen coordinates.
     *
     * @param aPoint the point position in screen coordinates.
     * @return the point position in world coordinates.
     */
    inline VECTOR2D ToWorld( const VECTOR2D& aPoint ) const
    {
        return VECTOR2D( m_screenWorldMatrix * aPoint );
    }

    /**
     * Compute the point position in screen coordinates from given world coordinates.
     *
     * @param aPoint the point position in world coordinates.
     * @return the point position in screen coordinates.
     */
    inline VECTOR2D ToScreen( const VECTOR2D& aPoint ) const
    {
        return VECTOR2D( m_worldScreenMatrix * aPoint );
    }

    /**
     * Set the cursor in the native panel.
     *
     * @param aCursor is the cursor to use in the native panel
     * @return true if the cursor was updated, false if the cursor given was already set
     */
    virtual bool SetNativeCursorStyle( KICURSOR aCursor, bool aHiDPI );

    /**
     * Enable/disable cursor.
     *
     * @param aCursorEnabled is true if the cursor should be drawn, else false.
     */
    inline void SetCursorEnabled( bool aCursorEnabled )
    {
        m_isCursorEnabled = aCursorEnabled;
    }

    /**
     * Return information about cursor visibility.
     *
     * @return True if cursor is visible.
     */
    bool IsCursorEnabled() const
    {
        return m_isCursorEnabled || m_forceDisplayCursor;
    }

    /**
     * Set the cursor color.
     *
     * @param aCursorColor is the color of the cursor.
     */
    inline void SetCursorColor( const COLOR4D& aCursorColor )
    {
        m_cursorColor = aCursorColor;
    }

    /**
     * Draw the cursor.
     *
     * @param aCursorPosition is the cursor position in screen coordinates.
     */
    virtual void DrawCursor( const VECTOR2D& aCursorPosition ) {};

    virtual void EnableDepthTest( bool aEnabled = false ) {};

    /**
     * Checks the state of the context lock
     * @return True if the context is currently locked
     */
    virtual bool IsContextLocked()
    {
        return false;
    }


    /// Use GAL_CONTEXT_LOCKER RAII object unless you know what you're doing.
    virtual void LockContext( int aClientCookie ) {}

    virtual void UnlockContext( int aClientCookie ) {}

    /// Start/end drawing functions, draw calls can be only made in between the calls
    /// to BeginDrawing()/EndDrawing(). Normally you should create a GAL_DRAWING_CONTEXT RAII
    /// object, but I'm leaving these functions public for more precise (i.e. timing/profiling)
    /// control of the drawing process - Tom

    /// Begin the drawing, needs to be called for every new frame.
    /// Use GAL_DRAWING_CONTEXT RAII object unless you know what you're doing.
    virtual void BeginDrawing() {};

    /// End the drawing, needs to be called for every new frame.
    /// Use GAL_DRAWING_CONTEXT RAII object unless you know what you're doing.
    virtual void EndDrawing() {};
protected:

    /// Enable item update mode.
    /// Private: use GAL_UPDATE_CONTEXT RAII object
    virtual void beginUpdate() {}

    /// Disable item update mode.
    virtual void endUpdate() {}



    /// Compute the scaling factor for the world->screen matrix
    inline void computeWorldScale()
    {
        m_worldScale = m_screenDPI * m_worldUnitLength * m_zoomFactor;

        if( Pgm().GetCommonSettings() )
            m_worldScale *= Pgm().GetCommonSettings()->m_Appearance.zoom_correction_factor;
    }

    /**
     * Compute minimum grid spacing from the grid settings.
     *
     * @return the minimum spacing to use for drawing the grid
     */
    double computeMinGridSpacing() const;

    /// Possible depth range
    static const int MIN_DEPTH;
    static const int MAX_DEPTH;

    /// Depth level on which the grid is drawn
    static const int GRID_DEPTH;

    /**
     * Get the actual cursor color to draw.
     */
    COLOR4D getCursorColor() const;

    // ---------------
    // Settings observer interface
    // ---------------
    /**
     * Handler for observer settings changes.
     */
    void OnGalDisplayOptionsChanged( const GAL_DISPLAY_OPTIONS& aOptions ) override;

    /**
     * Handle updating display options.
     *
     * Derived classes should call up to this to set base-class methods.
     *
     * @return true if the new settings changed something. Derived classes can use this
     *         information to refresh themselves
     */
    virtual bool updatedGalDisplayOptions( const GAL_DISPLAY_OPTIONS& aOptions );

    /**
     * Ensure that the first element is smaller than the second.
     */
    template <typename T>
    void normalize( T &a, T &b )
    {
        if( a > b )
        {
            T tmp = a;
            a = b;
            b = tmp;
        }
    }

    GAL_DISPLAY_OPTIONS& m_options;
    UTIL::LINK           m_observerLink;

    std::stack<double>   m_depthStack;         ///< Stored depth values
    VECTOR2I             m_screenSize;         ///< Screen size in screen (wx logical) coordinates

    double               m_worldUnitLength;    ///< The unit length of the world coordinates [inch]
    double               m_screenDPI;          ///< The dots per inch of the screen
    VECTOR2D             m_lookAtPoint;        ///< Point to be looked at in world space

    double               m_zoomFactor;         ///< The zoom factor
    double               m_rotation;           ///< Rotation transformation (radians)
    MATRIX3x3D           m_worldScreenMatrix;  ///< World transformation
    MATRIX3x3D           m_screenWorldMatrix;  ///< Screen transformation
    double               m_worldScale;         ///< The scale factor world->screen

    bool                 m_globalFlipX;        ///< Flag for X axis flipping
    bool                 m_globalFlipY;        ///< Flag for Y axis flipping

    float                m_lineWidth;          ///< The line width
    float                m_minLineWidth;       ///< Minimum line width in pixels

    bool                 m_isFillEnabled;      ///< Is filling of graphic objects enabled ?
    bool                 m_isStrokeEnabled;    ///< Are the outlines stroked ?

    COLOR4D              m_fillColor;          ///< The fill color
    COLOR4D              m_strokeColor;        ///< The color of the outlines
    COLOR4D              m_clearColor;
    COLOR4D              m_hoverColor;         ///< Color for hovered (active) links

    double               m_layerDepth;         ///< The actual layer depth
    VECTOR2D             m_depthRange;         ///< Range of the depth

    // Grid settings
    bool                 m_gridVisibility;     ///< Should the grid be shown
    GRID_STYLE           m_gridStyle;          ///< Grid display style
    VECTOR2D             m_gridSize;           ///< The grid size
    VECTOR2D             m_gridOrigin;         ///< The grid origin
    VECTOR2D             m_gridOffset;         ///< The grid offset to compensate cursor position
    COLOR4D              m_gridColor;          ///< Color of the grid
    COLOR4D              m_axesColor;          ///< Color of the axes
    bool                 m_axesEnabled;        ///< Should the axes be drawn
    int                  m_gridTick;           ///< Every tick line gets the double width
    float                m_gridLineWidth;      ///< Line width of the grid
    int                  m_gridMinSpacing;     ///< Minimum screen size of the grid (pixels)
                                               ///< below which the grid is not drawn

    // Cursor settings
    bool                 m_isCursorEnabled;    ///< Is the cursor enabled?
    bool                 m_forceDisplayCursor; ///< Always show cursor
    COLOR4D              m_cursorColor;        ///< Cursor color
    KIGFX::CROSS_HAIR_MODE m_crossHairMode;    ///< Crosshair drawing mode
    VECTOR2D             m_cursorPosition;     ///< Current cursor position (world coordinates)

    KICURSOR             m_currentNativeCursor; ///< Current cursor

private:

    inline double getLayerDepth() const
    {
        return m_layerDepth;
    }

    TEXT_ATTRIBUTES      m_attributes;

    friend class GAL_SCOPED_ATTRS;
};


class GAL_CONTEXT_LOCKER
{
public:
    GAL_CONTEXT_LOCKER( GAL* aGal ) :
        m_gal( aGal )
    {
        m_cookie = rand();
        m_gal->LockContext( m_cookie );
    }

    ~GAL_CONTEXT_LOCKER()
    {
        m_gal->UnlockContext( m_cookie );
    }

protected:
    GAL* m_gal;
    int  m_cookie;
};


class GAL_UPDATE_CONTEXT : public GAL_CONTEXT_LOCKER
{
public:
    GAL_UPDATE_CONTEXT( GAL* aGal ) :
            GAL_CONTEXT_LOCKER( aGal )
    {
        m_gal->beginUpdate();
    }

    ~GAL_UPDATE_CONTEXT()
    {
        m_gal->endUpdate();
    }
};


class GAL_DRAWING_CONTEXT : public GAL_CONTEXT_LOCKER
{
public:
    GAL_DRAWING_CONTEXT( GAL* aGal ) :
            GAL_CONTEXT_LOCKER( aGal )
    {
        m_gal->BeginDrawing();
    }

    ~GAL_DRAWING_CONTEXT() noexcept( false )
    {
        m_gal->EndDrawing();
    }
};


/**
 * Attribute save/restore for GAL attributes.
 */
class GAL_SCOPED_ATTRS
{
public:
    enum FLAGS
    {
        STROKE_WIDTH = 1,
        STROKE_COLOR = 2,
        IS_STROKE = 4,
        FILL_COLOR = 8,
        IS_FILL = 16,
        LAYER_DEPTH = 32,

        // It is not clear to me that GAL needs to save text attributes.
        // Only BitmapText uses it, and maybe that should be passed in
        // explicitly (like for Draw) - every caller of BitmapText sets
        // the text attributes anyway.
        // TEXT_ATTRS = 64,

        // Convenience flags
        STROKE = STROKE_WIDTH | STROKE_COLOR | IS_STROKE,
        FILL = FILL_COLOR | IS_FILL,
        STROKE_FILL = STROKE | FILL,

        ALL_ATTRS = STROKE | FILL | LAYER_DEPTH,
    };

    /**
     * Instantiate a GAL_SCOPED_ATTRS object, saving the current attributes of the GAL.
     *
     * Specify the flags to save/restore in aFlags.
     */
    GAL_SCOPED_ATTRS( KIGFX::GAL& aGal, int aFlags )
        : m_gal( aGal ), m_flags( aFlags )
    {
        // Save what we need to restore later.
        // These are all so cheap to copy, it's likely not worth if'ing
        m_strokeWidth = aGal.GetLineWidth();
        m_strokeColor = aGal.GetStrokeColor();
        m_isStroke = aGal.GetIsStroke();
        m_fillColor = aGal.GetFillColor();
        m_isFill = aGal.GetIsFill();
        m_layerDepth = aGal.getLayerDepth();
    }

    ~GAL_SCOPED_ATTRS()
    {
        // Restore the attributes that were saved
        // based on the flags that were set.

        if( m_flags & STROKE_WIDTH )
            m_gal.SetLineWidth( m_strokeWidth );

        if( m_flags & STROKE_COLOR )
            m_gal.SetStrokeColor( m_strokeColor );

        if( m_flags & IS_STROKE )
            m_gal.SetIsStroke( m_isStroke );

        if( m_flags & FILL_COLOR )
            m_gal.SetFillColor( m_fillColor );

        if( m_flags & IS_FILL )
            m_gal.SetIsFill( m_isFill );

        if( m_flags & LAYER_DEPTH )
            m_gal.SetLayerDepth( m_layerDepth );
    }

private:
    GAL& m_gal;
    int  m_flags;

    COLOR4D m_strokeColor;
    double  m_strokeWidth;
    bool    m_isStroke;

    COLOR4D m_fillColor;
    bool    m_isFill;

    double m_layerDepth;
};


};    // namespace KIGFX

#endif /* GRAPHICSABSTRACTIONLAYER_H_ */
