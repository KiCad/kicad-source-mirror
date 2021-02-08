/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright (C) 2016-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <gal/color4d.h>
#include <gal/definitions.h>
#include <gal/stroke_font.h>
#include <gal/gal_display_options.h>
#include <newstroke_font.h>

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
class GAL : GAL_DISPLAY_OPTIONS_OBSERVER
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
                              double aWidth ) {};

    /**
     * Draw a polyline
     *
     * @param aPointList is a list of 2D-Vectors containing the polyline points.
     */
    virtual void DrawPolyline( const std::deque<VECTOR2D>& aPointList ) {};
    virtual void DrawPolyline( const VECTOR2D aPointList[], int aListSize ) {};
    virtual void DrawPolyline( const SHAPE_LINE_CHAIN& aLineChain ) {};

    /**
     * Draw a circle using world coordinates.
     *
     * @param aCenterPoint is the center point of the circle.
     * @param aRadius is the radius of the circle.
     */
    virtual void DrawCircle( const VECTOR2D& aCenterPoint, double aRadius ) {};

    /**
     * Draw an arc.
     *
     * @param aCenterPoint  is the center point of the arc.
     * @param aRadius       is the arc radius.
     * @param aStartAngle   is the start angle of the arc.
     * @param aEndAngle     is the end angle of the arc.
     */
    virtual void
    DrawArc( const VECTOR2D& aCenterPoint, double aRadius, double aStartAngle,
             double aEndAngle ) {};

    /**
     * Draw an arc segment.
     *
     * This method differs from DrawArc() in what happens when fill/stroke are on or off.
     * DrawArc() draws a "pie piece" when fill is turned on, and a thick stroke when fill is off.
     * DrawArcSegment() with fill *on* behaves like DrawArc() with fill *off*.
     * DrawArcSegment() with fill *off* draws the outline of what it would have drawn with fill on.
	 *
	 * TODO: Unify Arc routines
     *
     * @param aCenterPoint  is the center point of the arc.
     * @param aRadius       is the arc radius.
     * @param aStartAngle   is the start angle of the arc.
     * @param aEndAngle     is the end angle of the arc.
     * @param aWidth        is the thickness of the arc (pen size).
     */
    virtual void
    DrawArcSegment( const VECTOR2D& aCenterPoint, double aRadius, double aStartAngle,
                    double aEndAngle, double aWidth ) {};

    /**
     * Draw a rectangle.
     *
     * @param aStartPoint   is the start point of the rectangle.
     * @param aEndPoint     is the end point of the rectangle.
     */
    virtual void DrawRectangle( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint ) {};

    /**
     * Draw a polygon.
     *
     * @param aPointList is the list of the polygon points.
     */
    virtual void DrawPolygon( const std::deque<VECTOR2D>& aPointList ) {};
    virtual void DrawPolygon( const VECTOR2D aPointList[], int aListSize ) {};
    virtual void DrawPolygon( const SHAPE_POLY_SET& aPolySet ) {};
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
    virtual void DrawBitmap( const BITMAP_BASE& aBitmap ) {};

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
        return screenSize;
    }

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
        isFillEnabled = aIsFillEnabled;
    }

    /**
     * Enable/disable stroked outlines.
     *
     * @param aIsStrokeEnabled is true, if the outline of an object should be stroked.
     */
    virtual void SetIsStroke( bool aIsStrokeEnabled )
    {
        isStrokeEnabled = aIsStrokeEnabled;
    }

    /**
     * Set the fill color.
     *
     * @param aColor is the color for filling.
     */
    virtual void SetFillColor( const COLOR4D& aColor )
    {
        fillColor = aColor;
    }

    /**
     * Get the fill color.
     *
     * @return the color for filling a outline.
     */
    inline const COLOR4D& GetFillColor() const
    {
        return fillColor;
    }

    /**
     * Set the stroke color.
     *
     * @param aColor is the color for stroking the outline.
     */
    virtual void SetStrokeColor( const COLOR4D& aColor )
    {
        strokeColor = aColor;
    }

    /**
     * Get the stroke color.
     *
     * @return the color for stroking the outline.
     */
    inline const COLOR4D& GetStrokeColor() const
    {
        return strokeColor;
    }

    /**
     * Set the line width.
     *
     * @param aLineWidth is the line width.
     */
    virtual void SetLineWidth( float aLineWidth )
    {
        lineWidth = aLineWidth;
    }

    /**
     * Get the line width.
     *
     * @return the actual line width.
     */
    inline float GetLineWidth() const
    {
        return lineWidth;
    }

    /**
     * Set the depth of the layer (position on the z-axis)
     *
     * @param aLayerDepth the layer depth for the objects.
     */
    virtual void SetLayerDepth( double aLayerDepth )
    {
        assert( aLayerDepth <= depthRange.y );
        assert( aLayerDepth >= depthRange.x );

        layerDepth = aLayerDepth;
    }

    // ----
    // Text
    // ----

    const STROKE_FONT& GetStrokeFont() const
    {
        return strokeFont;
    }

    /**
     * Draw a vector type text using preloaded Newstroke font.
     *
     * @param aText is the text to be drawn.
     * @param aPosition is the text position in world coordinates.
     * @param aRotationAngle is the text rotation angle.
     */
    virtual void StrokeText( const wxString& aText, const VECTOR2D& aPosition,
                             double aRotationAngle )
    {
        strokeFont.Draw( aText, aPosition, aRotationAngle );
    }

    /**
     * Draw a text using a bitmap font. It should be faster than StrokeText(),
     * but can be used only for non-Gerber elements.
     *
     * @param aText is the text to be drawn.
     * @param aPosition is the text position in world coordinates.
     * @param aRotationAngle is the text rotation angle.
     */
    virtual void BitmapText( const wxString& aText, const VECTOR2D& aPosition,
                             double aRotationAngle )
    {
        // Fallback: use stroke font

        // Handle flipped view
        if( globalFlipX )
            textProperties.m_mirrored = !textProperties.m_mirrored;

        // Bitmap font is slightly smaller and slightly heavier than the stroke font so we
        // compensate a bit before stroking
        int saveLineWidth = lineWidth;
        VECTOR2D saveGlyphSize = textProperties.m_glyphSize;
        {
            lineWidth *= 1.2f;
            textProperties.m_glyphSize = textProperties.m_glyphSize * 0.8;

            StrokeText( aText, aPosition, aRotationAngle );
        }
        lineWidth = saveLineWidth;
        textProperties.m_glyphSize = saveGlyphSize;

        if( globalFlipX )
            textProperties.m_mirrored = !textProperties.m_mirrored;
    }

    /**
     * Compute the X and Y size of a given text. The text is expected to be a only one line text.
     *
     * @param aText is the text string (one line).
     * @return is the text size.
     */
    VECTOR2D GetTextLineSize( const UTF8& aText ) const;

    /**
     * Loads attributes of the given text (bold/italic/underline/mirrored and so on).
     *
     * @param aText is the text item.
     */
    virtual void SetTextAttributes( const EDA_TEXT* aText );

    /**
     * Reset text attributes to default styling.
     *
     * Normally, custom attributes will be set individually after this,
     * otherwise you can use SetTextAttributes()
     */
    void ResetTextAttributes();

    /**
     * Set the font glyph size.
     *
     * @param aGlyphSize is the new font glyph size.
     */
    inline void SetGlyphSize( const VECTOR2D aSize ) { textProperties.m_glyphSize = aSize; }
    const VECTOR2D& GetGlyphSize() const { return textProperties.m_glyphSize; }

    /**
     * Set bold property of current font.
     *
     * @param aBold tells if the font should be bold or not.
     */
    inline void SetFontBold( const bool aBold ) { textProperties.m_bold = aBold;  }
    inline bool IsFontBold() const { return textProperties.m_bold; }

    /**
     * Set italic property of current font.
     *
     * @param aItalic tells if the font should be italic or not.
     */
    inline void SetFontItalic( bool aItalic ) { textProperties.m_italic = aItalic; }
    inline bool IsFontItalic() const { return textProperties.m_italic; }

    inline void SetFontUnderlined( bool aUnderlined ) { textProperties.m_underlined = aUnderlined; }
    inline bool IsFontUnderlined() const { return textProperties.m_underlined; }

    /**
     * Set a mirrored property of text.
     *
     * @param aMirrored tells if the text should be mirrored or not.
     */
    inline void SetTextMirrored( const bool aMirrored ) { textProperties.m_mirrored = aMirrored; }
    inline bool IsTextMirrored() const { return textProperties.m_mirrored; }

    /**
     * Set the horizontal justify for text drawing.
     *
     * @param aHorizontalJustify is the horizontal justify value.
     */
    inline void SetHorizontalJustify( const EDA_TEXT_HJUSTIFY_T aHorizontalJustify )
    {
        textProperties.m_horizontalJustify = aHorizontalJustify;
    }

    /**
     * Return current text horizontal justification setting.
     */
    inline EDA_TEXT_HJUSTIFY_T GetHorizontalJustify() const
    {
        return textProperties.m_horizontalJustify;
    }

    /**
     * Set the vertical justify for text drawing.
     *
     * @param aVerticalJustify is the vertical justify value.
     */
    inline void SetVerticalJustify( const EDA_TEXT_VJUSTIFY_T aVerticalJustify )
    {
        textProperties.m_verticalJustify = aVerticalJustify;
    }

    /**
     * Returns current text vertical justification setting.
     */
    inline EDA_TEXT_VJUSTIFY_T GetVerticalJustify() const
    {
        return textProperties.m_verticalJustify;
    }


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
        return worldScreenMatrix;
    }

    /**
     * Get the screen <-> world transformation matrix.
     *
     * @return the transformation matrix.
     */
    const MATRIX3x3D& GetScreenWorldMatrix() const
    {
        return screenWorldMatrix;
    }

    /**
     * Set the world <-> screen transformation matrix.
     *
     * @param aMatrix is the 3x3 world <-> screen transformation matrix.
     */
    inline void SetWorldScreenMatrix( const MATRIX3x3D& aMatrix )
    {
        worldScreenMatrix = aMatrix;
    }

    /**
     * Set the unit length.
     *
     * This defines the length [inch] per one integer. For instance a value 0.001 means
     * that the coordinate [1000, 1000] corresponds with a point at (1 inch, 1 inch) or
     * 1 mil resolution per integer.
     *
     * @param aWorldUnitLength is the world Unit length.
     */
    inline void SetWorldUnitLength( double aWorldUnitLength )
    {
        worldUnitLength = aWorldUnitLength;
    }

    inline void SetScreenSize( const VECTOR2I& aSize )
    {
        screenSize = aSize;
    }

    /**
     * Set the dots per inch of the screen.
     *
     * This value depends on the user screen, it should be configurable by the application.
     * For instance a typical notebook with HD+ resolution (1600x900) has 106 DPI.
     *
     * @param aScreenDPI are the screen DPI.
     */
    inline void SetScreenDPI( double aScreenDPI )
    {
        screenDPI = aScreenDPI;
    }

    /**
     * Set the Point in world space to look at.
     *
     * This point corresponds with the center of the actual drawing area.
     *
     * @param aPoint is the look at point (center of the actual drawing area).
     */
    inline void SetLookAtPoint( const VECTOR2D& aPoint )
    {
        lookAtPoint = aPoint;
    }

    /**
     * Get the look at point.
     *
     * @return the look at point.
     */
    inline const VECTOR2D& GetLookAtPoint() const
    {
        return lookAtPoint;
    }

    /**
     * Set the zoom factor of the scene.
     *
     * @param aZoomFactor is the zoom factor.
     */
    inline void SetZoomFactor( double aZoomFactor )
    {
        zoomFactor = aZoomFactor;
    }

    /**
     * Get the zoom factor
     *
     * @return the zoom factor.
     */
    inline double GetZoomFactor() const
    {
        return zoomFactor;
    }

    /**
     * Set the rotation angle.
     *
     * @param aRotation is the new rotation angle (radians).
     */
    void SetRotation( double aRotation )
    {
        rotation = aRotation;
    }

    /**
     * Get the rotation angle.
     *
     * @return The rotation angle (radians).
     */
    double GetRotation() const
    {
        return rotation;
    }

    /**
     * Set the range of the layer depth.
     *
     * Usually required for the OpenGL implementation, any object outside this range is not drawn.
     *
     * @param aDepthRange is the depth range where component x is the near clipping plane and y
     *                    is the far clipping plane.
     */
    inline void SetDepthRange( const VECTOR2D& aDepthRange )
    {
        depthRange = aDepthRange;
    }

    /**
     * Return the minimum depth in the currently used range (the top).
     */
    inline double GetMinDepth() const
    {
        return depthRange.x;
    }

    /**
     * Return the maximum depth in the currently used range (the bottom).
     */
    inline double GetMaxDepth() const
    {
        return depthRange.y;
    }

    /**
     * Get the world scale.
     *
     * @return the actual world scale factor.
     */
    inline double GetWorldScale() const
    {
        return worldScale;
    }

    /**
     * Sets flipping of the screen.
     *
     * @param xAxis is the flip flag for the X axis.
     * @param yAxis is the flip flag for the Y axis.
     */
    inline void SetFlip( bool xAxis, bool yAxis )
    {
        globalFlipX = xAxis;
        globalFlipY = yAxis;
    }

    /**
     * Return true if flip flag for the X axis is set.
     */
    bool IsFlippedX() const
    {
        return globalFlipX;
    }

    /**
     * Return true if flip flag for the Y axis is set.
     */
    bool IsFlippedY() const
    {
        return globalFlipY;
    }

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

    // -------------
    // Grid methods
    // -------------

    /**
     * Set the visibility setting of the grid.
     *
     * @param aVisibility is the new visibility setting of the grid.
     */
    void SetGridVisibility( bool aVisibility ) { gridVisibility = aVisibility; }

    bool GetGridVisibility() const { return gridVisibility; }

    bool GetGridSnapping() const
    {
        return ( options.m_gridSnapping == KIGFX::GRID_SNAPPING::ALWAYS ||
                ( gridVisibility && options.m_gridSnapping == KIGFX::GRID_SNAPPING::WITH_GRID ) );
    }
    /**
     * Set the origin point for the grid.
     *
     * @param aGridOrigin is a vector containing the grid origin point, in world coordinates.
     */
    inline void SetGridOrigin( const VECTOR2D& aGridOrigin )
    {
        gridOrigin = aGridOrigin;

        if( gridSize.x == 0.0 || gridSize.y == 0.0 )
            gridOffset = VECTOR2D(0.0, 0.0);
        else
            gridOffset = VECTOR2D( (long) gridOrigin.x % (long) gridSize.x,
                                   (long) gridOrigin.y % (long) gridSize.y );
    }

    inline const VECTOR2D& GetGridOrigin() const
    {
        return gridOrigin;
    }

    /**
     * Set the grid size.
     *
     * @param aGridSize is a vector containing the grid size in x and y direction.
     */
    inline void SetGridSize( const VECTOR2D& aGridSize )
    {
        gridSize = aGridSize;

        // Avoid stupid grid size values: a grid size  should be >= 1 in internal units
        gridSize.x = std::max( 1.0, gridSize.x );
        gridSize.y = std::max( 1.0, gridSize.y );

        gridOffset = VECTOR2D( (long) gridOrigin.x % (long) gridSize.x,
                               (long) gridOrigin.y % (long) gridSize.y );
    }

    /**
     * Return the grid size.
     *
     * @return A vector containing the grid size in x and y direction.
     */
    inline const VECTOR2D& GetGridSize() const
    {
        return gridSize;
    }

    /**
     * Set the grid color.
     *
     * @param aGridColor is the grid color, it should have a low alpha value for the best effect.
     */
    inline void SetGridColor( const COLOR4D& aGridColor )
    {
        gridColor = aGridColor;
    }

    /**
     * Set the axes color.
     *
     * @param aAxesColor is the color to draw the axes if enabled.
     */
    inline void SetAxesColor( const COLOR4D& aAxesColor )
    {
        axesColor = aAxesColor;
    }

    /**
     * Enable drawing the axes.
     */
    inline void SetAxesEnabled( bool aAxesEnabled )
    {
        axesEnabled = aAxesEnabled;
    }

    /**
     * Draw every tick line wider.
     *
     * @param aInterval increase the width of every aInterval line, if 0 do not use this feature.
     */
    inline void SetCoarseGrid( int aInterval )
    {
        gridTick = aInterval;
    }

    /**
     * Get the grid line width.
     *
     * @return the grid line width
     */
    inline float GetGridLineWidth() const
    {
        return gridLineWidth;
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
     * @param aPoint the pointposition in screen coordinates.
     * @return the point position in world coordinates.
     */
    inline VECTOR2D ToWorld( const VECTOR2D& aPoint ) const
    {
        return VECTOR2D( screenWorldMatrix * aPoint );
    }

    /**
     * Compute the point position in screen coordinates from given world coordinates.
     *
     * @param aPoint the pointposition in world coordinates.
     * @return the point position in screen coordinates.
     */
    inline VECTOR2D ToScreen( const VECTOR2D& aPoint ) const
    {
        return VECTOR2D( worldScreenMatrix * aPoint );
    }

    /**
     * Enable/disable cursor.
     *
     * @param aCursorEnabled is true if the cursor should be drawn, else false.
     */
    inline void SetCursorEnabled( bool aCursorEnabled )
    {
        isCursorEnabled = aCursorEnabled;
    }

    /**
     * Return information about cursor visibility.
     *
     * @return True if cursor is visible.
     */
    bool IsCursorEnabled() const
    {
        return isCursorEnabled || forceDisplayCursor;
    }

    /**
     * Set the cursor color.
     *
     * @param aCursorColor is the color of the cursor.
     */
    inline void SetCursorColor( const COLOR4D& aCursorColor )
    {
        cursorColor = aCursorColor;
    }

    /**
     * Draw the cursor.
     *
     * @param aCursorPosition is the cursor position in screen coordinates.
     */
    virtual void DrawCursor( const VECTOR2D& aCursorPosition ) {};

    /**
     * Change the current depth to deeper, so it is possible to draw objects right beneath
     * other.
     */
    inline void AdvanceDepth()
    {
        layerDepth -= 0.05;
    }

    /**
     * Store current drawing depth on the depth stack.
     */
    inline void PushDepth()
    {
        depthStack.push( layerDepth );
    }

    /**
     * Restore previously stored drawing depth for the depth stack.
     */
    inline void PopDepth()
    {
        layerDepth = depthStack.top();
        depthStack.pop();
    }

    virtual void EnableDepthTest( bool aEnabled = false ) {};

    /**
     * Checks the state of the context lock
     * @return True if the context is currently locked
     */
    virtual bool IsContextLocked()
    {
        return false;
    }

protected:
    /// Use GAL_CONTEXT_LOCKER RAII object
    virtual void lockContext( int aClientCookie ) {}

    virtual void unlockContext( int aClientCookie ) {}

    /// Enable item update mode.
    /// Private: use GAL_UPDATE_CONTEXT RAII object
    virtual void beginUpdate() {}

    /// Disable item update mode.
    virtual void endUpdate() {}

    /// Begin the drawing, needs to be called for every new frame.
    /// Private: use GAL_DRAWING_CONTEXT RAII object
    virtual void beginDrawing() {};

    /// End the drawing, needs to be called for every new frame.
    /// Private: use GAL_DRAWING_CONTEXT RAII object
    virtual void endDrawing() {};

    /// Compute the scaling factor for the world->screen matrix
    inline void computeWorldScale()
    {
        worldScale = screenDPI * worldUnitLength * zoomFactor;
    }

    /**
     * compute minimum grid spacing from the grid settings
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
     * Get the actual cursor color to draw
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

    GAL_DISPLAY_OPTIONS&    options;
    UTIL::LINK              observerLink;

    std::stack<double> depthStack;             ///< Stored depth values
    VECTOR2I           screenSize;             ///< Screen size in screen coordinates

    double             worldUnitLength;        ///< The unit length of the world coordinates [inch]
    double             screenDPI;              ///< The dots per inch of the screen
    VECTOR2D           lookAtPoint;            ///< Point to be looked at in world space

    double             zoomFactor;             ///< The zoom factor
    double             rotation;               ///< Rotation transformation (radians)
    MATRIX3x3D         worldScreenMatrix;      ///< World transformation
    MATRIX3x3D         screenWorldMatrix;      ///< Screen transformation
    double             worldScale;             ///< The scale factor world->screen

    bool globalFlipX;                          ///< Flag for X axis flipping
    bool globalFlipY;                          ///< Flag for Y axis flipping

    float              lineWidth;              ///< The line width

    bool               isFillEnabled;          ///< Is filling of graphic objects enabled ?
    bool               isStrokeEnabled;        ///< Are the outlines stroked ?

    COLOR4D            fillColor;              ///< The fill color
    COLOR4D            strokeColor;            ///< The color of the outlines
    COLOR4D            m_clearColor;

    double             layerDepth;             ///< The actual layer depth
    VECTOR2D           depthRange;             ///< Range of the depth

    // Grid settings
    bool               gridVisibility;         ///< Should the grid be shown
    GRID_STYLE         gridStyle;              ///< Grid display style
    VECTOR2D           gridSize;               ///< The grid size
    VECTOR2D           gridOrigin;             ///< The grid origin
    VECTOR2D           gridOffset;             ///< The grid offset to compensate cursor position
    COLOR4D            gridColor;              ///< Color of the grid
    COLOR4D            axesColor;              ///< Color of the axes
    bool               axesEnabled;            ///< Should the axes be drawn
    int                gridTick;               ///< Every tick line gets the double width
    float              gridLineWidth;          ///< Line width of the grid
    int                gridMinSpacing;         ///< Minimum screen size of the grid (pixels)
                                               ///< below which the grid is not drawn

    // Cursor settings
    bool               isCursorEnabled;        ///< Is the cursor enabled?
    bool               forceDisplayCursor;     ///< Always show cursor
    COLOR4D            cursorColor;            ///< Cursor color
    bool               fullscreenCursor;       ///< Shape of the cursor (fullscreen or small cross)
    VECTOR2D           cursorPosition;         ///< Current cursor position (world coordinates)

    /// Instance of object that stores information about how to draw texts
    STROKE_FONT        strokeFont;

private:
    struct TEXT_PROPERTIES
    {
        VECTOR2D            m_glyphSize;            ///< Size of the glyphs
        EDA_TEXT_HJUSTIFY_T m_horizontalJustify;    ///< Horizontal justification
        EDA_TEXT_VJUSTIFY_T m_verticalJustify;      ///< Vertical justification
        bool                m_bold;
        bool                m_italic;
        bool                m_underlined;
        bool                m_mirrored;
    } textProperties;
};


class GAL_CONTEXT_LOCKER
{
public:
    GAL_CONTEXT_LOCKER( GAL* aGal ) :
        m_gal( aGal )
    {
        m_cookie = rand();
        m_gal->lockContext( m_cookie );
    }

    ~GAL_CONTEXT_LOCKER()
    {
        m_gal->unlockContext( m_cookie );
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
        m_gal->beginDrawing();
    }

    ~GAL_DRAWING_CONTEXT()
    {
        m_gal->endDrawing();
    }
};


};    // namespace KIGFX

#endif /* GRAPHICSABSTRACTIONLAYER_H_ */
