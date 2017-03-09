/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright (C) 2016-2017 Kicad Developers, see change_log.txt for contributors.
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

namespace KIGFX
{

/**
 * @brief Class GAL is the abstract interface for drawing on a 2D-surface.
 *
 * The functions are optimized for drawing shapes of an EDA-program such as KiCad. Most methods
 * are abstract and need to be implemented by a lower layer, for example by a cairo or OpenGL implementation.
 * <br>
 * Almost all methods use world coordinates as arguments. The board design is defined in world space units;
 * for drawing purposes these are transformed to screen units with this layer. So zooming is handled here as well.
 *
 */
class GAL: GAL_DISPLAY_OPTIONS_OBSERVER
{
public:
    // Constructor / Destructor
    GAL( GAL_DISPLAY_OPTIONS& aOptions );
    virtual ~GAL();

    /// @brief Returns the initalization status for the canvas.
    virtual bool IsInitialized() const { return true; }

    /// @brief Returns true if the GAL canvas is visible on the screen.
    virtual bool IsVisible() const { return true; }

    // ---------------
    // Drawing methods
    // ---------------

    /// @brief Begin the drawing, needs to be called for every new frame.
    virtual void BeginDrawing() {};

    /// @brief End the drawing, needs to be called for every new frame.
    virtual void EndDrawing() {};

    /// @brief Enables item update mode.
    virtual void BeginUpdate() {}

    /// @brief Disables item update mode.
    virtual void EndUpdate() {}

    /**
     * @brief Draw a line.
     *
     * Start and end points are defined as 2D-Vectors.
     *
     * @param aStartPoint   is the start point of the line.
     * @param aEndPoint     is the end point of the line.
     */
    virtual void DrawLine( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint ) {};

    /**
     * @brief Draw a rounded segment.
     *
     * Start and end points are defined as 2D-Vectors.
     *
     * @param aStartPoint   is the start point of the segment.
     * @param aEndPoint     is the end point of the segment.
     * @param aWidth        is a width of the segment
     */
    virtual void DrawSegment( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint, double aWidth ) {};

    /**
     * @brief Draw a polyline
     *
     * @param aPointList is a list of 2D-Vectors containing the polyline points.
     */
    virtual void DrawPolyline( const std::deque<VECTOR2D>& aPointList ) {};
    virtual void DrawPolyline( const VECTOR2D aPointList[], int aListSize ) {};
    virtual void DrawPolyline( const SHAPE_LINE_CHAIN& aLineChain ) {};

    /**
     * @brief Draw a circle using world coordinates.
     *
     * @param aCenterPoint is the center point of the circle.
     * @param aRadius is the radius of the circle.
     */
    virtual void DrawCircle( const VECTOR2D& aCenterPoint, double aRadius ) {};

    /**
     * @brief Draw an arc.
     *
     * @param aCenterPoint  is the center point of the arc.
     * @param aRadius       is the arc radius.
     * @param aStartAngle   is the start angle of the arc.
     * @param aEndAngle     is the end angle of the arc.
     */
    virtual void
    DrawArc( const VECTOR2D& aCenterPoint, double aRadius, double aStartAngle, double aEndAngle ) {};

    /**
     * @brief Draw an arc segment.
     *
     * This method differs from DrawArc() in what happens when fill/stroke are on or off.
     * DrawArc() draws a "pie piece" when fill is turned on, and a thick stroke when fill is off.
     * DrawArcSegment() with fill *on* behaves like DrawArc() with fill *off*.
     * DrawArcSegment() with fill *off* draws the outline of what it would have drawn with fill on.
     *
     * @param aCenterPoint  is the center point of the arc.
     * @param aRadius       is the arc radius.
     * @param aStartAngle   is the start angle of the arc.
     * @param aEndAngle     is the end angle of the arc.
     */
    virtual void
    DrawArcSegment( const VECTOR2D& aCenterPoint, double aRadius, double aStartAngle,
                   double aEndAngle, double aWidth ) {};

    /**
     * @brief Draw a rectangle.
     *
     * @param aStartPoint   is the start point of the rectangle.
     * @param aEndPoint     is the end point of the rectangle.
     */
    virtual void DrawRectangle( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint ) {};

    /**
     * @brief Draw a polygon.
     *
     * @param aPointList is the list of the polygon points.
     */
    virtual void DrawPolygon( const std::deque<VECTOR2D>& aPointList ) {};
    virtual void DrawPolygon( const VECTOR2D aPointList[], int aListSize ) {};
    virtual void DrawPolygon( const SHAPE_POLY_SET& aPolySet ) {};

    /**
     * @brief Draw a cubic bezier spline.
     *
     * @param startPoint    is the start point of the spline.
     * @param controlPointA is the first control point.
     * @param controlPointB is the second control point.
     * @param endPoint      is the end point of the spline.
     */
    virtual void DrawCurve( const VECTOR2D& startPoint,    const VECTOR2D& controlPointA,
                            const VECTOR2D& controlPointB, const VECTOR2D& endPoint ) {};

    // --------------
    // Screen methods
    // --------------

    /// @brief Resizes the canvas.
    virtual void ResizeScreen( int aWidth, int aHeight ) {};

    /// @brief Shows/hides the GAL canvas
    virtual bool Show( bool aShow ) { return true; };

    /// @brief Returns GAL canvas size in pixels
    const VECTOR2I& GetScreenPixelSize() const
    {
        return screenSize;
    }

    /// @brief Force all remaining objects to be drawn.
    virtual void Flush() {};

    /**
     * @brief Clear the screen.
     * @param aColor is the color used for clearing.
     */
    virtual void ClearScreen( const COLOR4D& aColor ) {};

    // -----------------
    // Attribute setting
    // -----------------

    /**
     * @brief Enable/disable fill.
     *
     * @param aIsFillEnabled is true, when the graphics objects should be filled, else false.
     */
    virtual void SetIsFill( bool aIsFillEnabled )
    {
        isFillEnabled = aIsFillEnabled;
    }

    /**
     * @brief Enable/disable stroked outlines.
     *
     * @param aIsStrokeEnabled is true, if the outline of an object should be stroked.
     */
    virtual void SetIsStroke( bool aIsStrokeEnabled )
    {
        isStrokeEnabled = aIsStrokeEnabled;
    }

    /**
     * @brief Set the fill color.
     *
     * @param aColor is the color for filling.
     */
    virtual void SetFillColor( const COLOR4D& aColor )
    {
        fillColor = aColor;
    }

    /**
     * @brief Set the stroke color.
     *
     * @param aColor is the color for stroking the outline.
     */
    virtual void SetStrokeColor( const COLOR4D& aColor )
    {
        strokeColor = aColor;
    }

    /**
     * @brief Get the stroke color.
     *
     * @return the color for stroking the outline.
     */
    inline const COLOR4D& GetStrokeColor() const
    {
        return strokeColor;
    }

    /**
     * @brief Set the line width.
     *
     * @param aLineWidth is the line width.
     */
    virtual void SetLineWidth( double aLineWidth )
    {
        lineWidth = aLineWidth;
    }

    /**
     * @brief Get the line width.
     *
     * @return the actual line width.
     */
    inline double GetLineWidth() const
    {
        return lineWidth;
    }

    /**
     * @brief Set the depth of the layer (position on the z-axis)
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
     * @brief Draws a vector type text using preloaded Newstroke font.
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
     * @brief Draws a text using a bitmap font. It should be faster than StrokeText(),
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

        StrokeText( aText, aPosition, aRotationAngle );

        if( globalFlipX )
            textProperties.m_mirrored = !textProperties.m_mirrored;
    }

    /**
     * @brief Compute the X and Y size of a given text. The text is expected to be
     * a only one line text.
     *
     * @param aText is the text string (one line).
     * @return is the text size.
     */
    VECTOR2D GetTextLineSize( const UTF8& aText ) const;

    /**
     * Compute the vertical position of an overbar, sometimes used in texts.
     * This is the distance between the text base line and the overbar.
     * @return the relative position of the overbar axis.
     */
    double GetOverbarVerticalPosition() const
    {
        return strokeFont.computeOverbarVerticalPosition();
    }

    /**
     * @brief Loads attributes of the given text (bold/italic/underline/mirrored and so on).
     *
     * @param aText is the text item.
     */
    virtual void SetTextAttributes( const EDA_TEXT* aText );

    /**
     * @brief Set the font glyph size.
     *
     * @param aGlyphSize is the new font glyph size.
     */
    inline void SetGlyphSize( const VECTOR2D aGlyphSize )
    {
        textProperties.m_glyphSize = aGlyphSize;
    }

    /**
     * @return the current font glyph size.
     */
    const VECTOR2D& GetGlyphSize() const
    {
        return textProperties.m_glyphSize;
    }

    /**
     * @brief Set bold property of current font.
     *
     * @param aBold tells if the font should be bold or not.
     */
    inline void SetFontBold( const bool aBold )
    {
        textProperties.m_bold = aBold;
    }

    /**
     * @brief Returns true if current font has 'bold' attribute enabled.
     */
    inline bool IsFontBold() const
    {
        return textProperties.m_bold;
    }

    /**
     * @brief Set italic property of current font.
     *
     * @param aItalic tells if the font should be italic or not.
     */
    inline void SetFontItalic( const bool aItalic )
    {
        textProperties.m_italic = aItalic;
    }

    /**
     * @brief Returns true if current font has 'italic' attribute enabled.
     */
    inline bool IsFontItalic() const
    {
        return textProperties.m_italic;
    }

    /**
     * @brief Set a mirrored property of text.
     *
     * @param aMirrored tells if the text should be mirrored or not.
     */
    inline void SetTextMirrored( const bool aMirrored )
    {
        textProperties.m_mirrored = aMirrored;
    }

    /**
     * @brief Returns true if text should displayed mirrored.
     */
    inline bool IsTextMirrored() const
    {
        return textProperties.m_mirrored;
    }

    /**
     * @brief Set the horizontal justify for text drawing.
     *
     * @param aHorizontalJustify is the horizontal justify value.
     */
    inline void SetHorizontalJustify( const EDA_TEXT_HJUSTIFY_T aHorizontalJustify )
    {
        textProperties.m_horizontalJustify = aHorizontalJustify;
    }

    /**
     * @brief Returns current text horizontal justification setting.
     */
    inline EDA_TEXT_HJUSTIFY_T GetHorizontalJustify() const
    {
        return textProperties.m_horizontalJustify;
    }

    /**
     * @brief Set the vertical justify for text drawing.
     *
     * @param aVerticalJustify is the vertical justify value.
     */
    inline void SetVerticalJustify( const EDA_TEXT_VJUSTIFY_T aVerticalJustify )
    {
        textProperties.m_verticalJustify = aVerticalJustify;
    }

    /**
     * @brief Returns current text vertical justification setting.
     */
    inline EDA_TEXT_VJUSTIFY_T GetVerticalJustify() const
    {
        return textProperties.m_verticalJustify;
    }


    // --------------
    // Transformation
    // --------------

    /**
     * @brief Transform the context.
     *
     * @param aTransformation is the ransformation matrix.
     */
    virtual void Transform( const MATRIX3x3D& aTransformation ) {};

    /**
     * @brief Rotate the context.
     *
     * @param aAngle is the rotation angle in radians.
     */
    virtual void Rotate( double aAngle ) {};

    /**
     * @brief Translate the context.
     *
     * @param aTranslation is the translation vector.
     */
    virtual void Translate( const VECTOR2D& aTranslation ) {};

    /**
     * @brief Scale the context.
     *
     * @param aScale is the scale factor for the x- and y-axis.
     */
    virtual void Scale( const VECTOR2D& aScale ) {};

    /// @brief Save the context.
    virtual void Save() {};

    /// @brief Restore the context.
    virtual void Restore() {};

    // --------------------------------------------
    // Group methods
    // ---------------------------------------------

    /**
     * @brief Begin a group.
     *
     * A group is a collection of graphic items.
     * Hierarchical groups are possible, attributes and transformations can be used.
     *
     * @return the number of the group.
     */
    virtual int BeginGroup() { return 0; };

    /// @brief End the group.
    virtual void EndGroup() {};

    /**
     * @brief Draw the stored group.
     *
     * @param aGroupNumber is the group number.
     */
    virtual void DrawGroup( int aGroupNumber ) {};

    /**
     * @brief Changes the color used to draw the group.
     *
     * @param aGroupNumber is the group number.
     * @param aNewColor is the new color.
     */
    virtual void ChangeGroupColor( int aGroupNumber, const COLOR4D& aNewColor ) {};

    /**
     * @brief Changes the depth (Z-axis position) of the group.
     *
     * @param aGroupNumber is the group number.
     * @param aDepth is the new depth.
     */
    virtual void ChangeGroupDepth( int aGroupNumber, int aDepth ) {};

    /**
     * @brief Delete the group from the memory.
     *
     * @param aGroupNumber is the group number.
     */
    virtual void DeleteGroup( int aGroupNumber ) {};

    /**
     * @brief Delete all data created during caching of graphic items.
     */
    virtual void ClearCache() {};

    // --------------------------------------------------------
    // Handling the world <-> screen transformation
    // --------------------------------------------------------

    /// @brief Compute the world <-> screen transformation matrix
    virtual void ComputeWorldScreenMatrix();

    /**
     * @brief Get the world <-> screen transformation matrix.
     *
     * @return the transformation matrix.
     */
    const MATRIX3x3D& GetWorldScreenMatrix() const
    {
        return worldScreenMatrix;
    }

    /**
     * @brief Get the screen <-> world transformation matrix.
     *
     * @return the transformation matrix.
     */
    const MATRIX3x3D& GetScreenWorldMatrix() const
    {
        return screenWorldMatrix;
    }

    /**
     * @brief Set the world <-> screen transformation matrix.
     *
     * @param aMatrix is the 3x3 world <-> screen transformation matrix.
     */
    inline void SetWorldScreenMatrix( const MATRIX3x3D& aMatrix )
    {
        worldScreenMatrix = aMatrix;
    }

    /**
     * @brief Set the unit length.
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

    /**
     * @brief Set the dots per inch of the screen.
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
     * @brief Set the Point in world space to look at.
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
     * @brief Get the look at point.
     *
     * @return the look at point.
     */
    inline const VECTOR2D& GetLookAtPoint() const
    {
        return lookAtPoint;
    }

    /**
     * @brief Set the zoom factor of the scene.
     *
     * @param aZoomFactor is the zoom factor.
     */
    inline void SetZoomFactor( double aZoomFactor )
    {
        zoomFactor = aZoomFactor;
    }

    /**
     * @brief Get the zoom factor
     *
     * @return the zoom factor.
     */
    inline double GetZoomFactor() const
    {
        return zoomFactor;
    }

    /**
     * @brief Set the range of the layer depth.
     *
     * Usually required for the OpenGL implementation, any object outside this range is not drawn.
     *
     * @param aDepthRange is the depth range where component x is the near clipping plane and y
     *        is the far clipping plane.
     */
    inline void SetDepthRange( const VECTOR2D& aDepthRange )
    {
        depthRange = aDepthRange;
    }

    /**
     * @brief Returns the minimum depth in the currently used range (the top).
     */
    inline double GetMinDepth() const
    {
        return depthRange.x;
    }

    /**
     * @brief Returns the maximum depth in the currently used range (the bottom).
     */
    inline double GetMaxDepth() const
    {
        return depthRange.y;
    }

    /**
     * @brief Get the world scale.
     *
     * @return the actual world scale factor.
     */
    inline double GetWorldScale() const
    {
        return worldScale;
    }

    /**
     * @brief Sets flipping of the screen.
     *
     * @param xAxis is the flip flag for the X axis.
     * @param yAxis is the flip flag for the Y axis.
     */
    inline void SetFlip( bool xAxis, bool yAxis )
    {
        globalFlipX = xAxis;
        globalFlipY = yAxis;
    }


    // ---------------------------
    // Buffer manipulation methods
    // ---------------------------

    /**
     * @brief Save the screen contents.
     */
    virtual void SaveScreen() {};

    /**
     * @brief Restore the screen contents.
     */
    virtual void RestoreScreen() {};

    /**
     * @brief Sets the target for rendering.
     *
     * @param aTarget is the new target for rendering.
     */
    virtual void SetTarget( RENDER_TARGET aTarget ) {};

    /**
     * @brief Gets the currently used target for rendering.
     *
     * @return The current rendering target.
     */
    virtual RENDER_TARGET GetTarget() const { return TARGET_CACHED; };

    /**
     * @brief Clears the target for rendering.
     *
     * @param aTarget is the target to be cleared.
     */
    virtual void ClearTarget( RENDER_TARGET aTarget ) {};

    // -------------
    // Grid methods
    // -------------

    /**
     * @brief Sets the visibility setting of the grid.
     *
     * @param aVisibility is the new visibility setting of the grid.
     */
    inline void SetGridVisibility( bool aVisibility )
    {
        gridVisibility = aVisibility;
    }

    /**
     * @brief Set the origin point for the grid.
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

    /**
     * @brief Set the grid size.
     *
     * @param aGridSize is a vector containing the grid size in x and y direction.
     */
    inline void SetGridSize( const VECTOR2D& aGridSize )
    {
        gridSize = aGridSize;

        gridOffset = VECTOR2D( (long) gridOrigin.x % (long) gridSize.x,
                               (long) gridOrigin.y % (long) gridSize.y );
    }

    /**
     * @brief Returns the grid size.
     *
     * @return A vector containing the grid size in x and y direction.
     */
    inline const VECTOR2D& GetGridSize() const
    {
        return gridSize;
    }

    /**
     * @brief Set the grid color.
     *
     * @param aGridColor is the grid color, it should have a low alpha value for the best effect.
     */
    inline void SetGridColor( const COLOR4D& aGridColor )
    {
        gridColor = aGridColor;
    }

    /**
     * @brief Set the axes color.
     *
     * @param aAxesColor is the color to draw the axes if enabled.
     */
    inline void SetAxesColor( const COLOR4D& aAxesColor )
    {
        axesColor = aAxesColor;
    }

    /**
     * @brief Enables drawing the axes.
     */
    inline void SetAxesEnabled( bool aAxesEnabled )
    {
        axesEnabled = aAxesEnabled;
    }

    /**
     * @brief Draw every tick line wider.
     *
     * @param aInterval increase the width of every aInterval line, if 0 do not use this feature.
     */
    inline void SetCoarseGrid( int aInterval )
    {
        gridTick = aInterval;
    }

    /**
     * @brief Get the grid line width.
     *
     * @return the grid line width
     */
    inline double GetGridLineWidth() const
    {
        return gridLineWidth;
    }

    ///> @brief Draw the grid
    virtual void DrawGrid();

    /**
     * Function GetGridPoint()
     * For a given point it returns the nearest point belonging to the grid in world coordinates.
     *
     * @param aPoint is the point for which the grid point is searched.
     * @return The nearest grid point in world coordinates.
     */
    VECTOR2D GetGridPoint( const VECTOR2D& aPoint ) const;

    /**
     * @brief Compute the point position in world coordinates from given screen coordinates.
     *
     * @param aPoint the pointposition in screen coordinates.
     * @return the point position in world coordinates.
     */
    inline VECTOR2D ToWorld( const VECTOR2D& aPoint ) const
    {
        return VECTOR2D( screenWorldMatrix * aPoint );
    }

    /**
     * @brief Compute the point position in screen coordinates from given world coordinates.
     *
     * @param aPoint the pointposition in world coordinates.
     * @return the point position in screen coordinates.
     */
    inline VECTOR2D ToScreen( const VECTOR2D& aPoint ) const
    {
        return VECTOR2D( worldScreenMatrix * aPoint );
    }

    /**
     * @brief Enable/disable cursor.
     *
     * @param aCursorEnabled is true if the cursor should be drawn, else false.
     */
    inline void SetCursorEnabled( bool aCursorEnabled )
    {
        isCursorEnabled = aCursorEnabled;
    }

    /**
     * @brief Returns information about cursor visibility.
     * @return True if cursor is visible.
     */
    bool IsCursorEnabled() const
    {
        return isCursorEnabled;
    }

    /**
     * @brief Set the cursor color.
     *
     * @param aCursorColor is the color of the cursor.
     */
    inline void SetCursorColor( const COLOR4D& aCursorColor )
    {
        cursorColor = aCursorColor;
    }

    /**
     * @brief Returns the cursor size.
     *
     * @return The current cursor size (in pixels).
     */
    inline unsigned int GetCursorSize() const
    {
        return cursorSize;
    }

    /**
     * @brief Set the cursor size.
     *
     * @param aCursorSize is the size of the cursor expressed in pixels.
     */
    virtual inline void SetCursorSize( unsigned int aCursorSize )
    {
        cursorSize = aCursorSize;
    }

    /**
     * @brief Draw the cursor.
     *
     * @param aCursorPosition is the cursor position in screen coordinates.
     */
    virtual void DrawCursor( const VECTOR2D& aCursorPosition ) {};

    /**
     * @brief Changes the current depth to deeper, so it is possible to draw objects right beneath
     * other.
     */
    inline void AdvanceDepth()
    {
        layerDepth -= 0.05;
    }

    /**
     * @brief Stores current drawing depth on the depth stack.
     */
    inline void PushDepth()
    {
        depthStack.push( layerDepth );
    }

    /**
     * @brief Restores previously stored drawing depth for the depth stack.
     */
    inline void PopDepth()
    {
        layerDepth = depthStack.top();
        depthStack.pop();
    }

    static const double METRIC_UNIT_LENGTH;

protected:

    GAL_DISPLAY_OPTIONS&    options;
    UTIL::LINK              observerLink;

    std::stack<double> depthStack;             ///< Stored depth values
    VECTOR2I           screenSize;             ///< Screen size in screen coordinates

    double             worldUnitLength;        ///< The unit length of the world coordinates [inch]
    double             screenDPI;              ///< The dots per inch of the screen
    VECTOR2D           lookAtPoint;            ///< Point to be looked at in world space

    double             zoomFactor;             ///< The zoom factor
    MATRIX3x3D         worldScreenMatrix;      ///< World transformation
    MATRIX3x3D         screenWorldMatrix;      ///< Screen transformation
    double             worldScale;             ///< The scale factor world->screen

    bool globalFlipX;                          ///< Flag for X axis flipping
    bool globalFlipY;                          ///< Flag for Y axis flipping

    double             lineWidth;              ///< The line width

    bool               isFillEnabled;          ///< Is filling of graphic objects enabled ?
    bool               isStrokeEnabled;        ///< Are the outlines stroked ?

    COLOR4D            fillColor;              ///< The fill color
    COLOR4D            strokeColor;            ///< The color of the outlines

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
    double             gridLineWidth;          ///< Line width of the grid
    int                gridMinSpacing;         ///< Minimum screen size of the grid (pixels)
                                               ///< below which the grid is not drawn

    // Cursor settings
    bool               isCursorEnabled;        ///< Is the cursor enabled?
    COLOR4D            cursorColor;            ///< Cursor color
    unsigned int       cursorSize;             ///< Size of the cursor in pixels
    VECTOR2D           cursorPosition;         ///< Current cursor position (world coordinates)

    /// Instance of object that stores information about how to draw texts
    STROKE_FONT        strokeFont;

    /// Compute the scaling factor for the world->screen matrix
    inline void computeWorldScale()
    {
        worldScale = screenDPI * worldUnitLength * zoomFactor;
    }

    /**
     * @brief compute minimum grid spacing from the grid settings
     *
     * @return the minimum spacing to use for drawing the grid
     */
    double computeMinGridSpacing() const;

    /**
     * @brief Draw a grid line (usually a simplified line function).
     *
     * @param aStartPoint is the start point of the line.
     * @param aEndPoint is the end point of the line.
     */
    virtual void drawGridLine( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint ) {};

    /// Possible depth range
    static const int MIN_DEPTH;
    static const int MAX_DEPTH;

    /// Depth level on which the grid is drawn
    static const int GRID_DEPTH;

    // ---------------
    // Settings observer interface
    // ---------------
    /**
     * Handler for observer settings changes
     */
    void OnGalDisplayOptionsChanged( const GAL_DISPLAY_OPTIONS& aOptions ) override;

    /**
     * Function updatedGalDisplayOptions
     *
     * @brief handler for updated display options. Derived classes
     * should call up to this to set base-class methods.
     *
     * @return true if the new settings changed something. Derived classes
     * can use this information to refresh themselves
     */
    virtual bool updatedGalDisplayOptions( const GAL_DISPLAY_OPTIONS& aOptions );

private:
    struct TEXT_PROPERTIES
    {
        VECTOR2D            m_glyphSize;            ///< Size of the glyphs
        EDA_TEXT_HJUSTIFY_T m_horizontalJustify;    ///< Horizontal justification
        EDA_TEXT_VJUSTIFY_T m_verticalJustify;      ///< Vertical justification
        bool                m_bold;
        bool                m_italic;
        bool                m_mirrored;
    } textProperties;
};
}    // namespace KIGFX

#endif /* GRAPHICSABSTRACTIONLAYER_H_ */
