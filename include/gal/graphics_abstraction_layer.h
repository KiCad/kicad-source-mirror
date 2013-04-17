/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright (C) 2012 Kicad Developers, see change_log.txt for contributors.
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

#include <wx/event.h>

#include <math/matrix3x3.h>
#include <gal/color4d.h>


namespace KiGfx
{
// Event declaration
extern const wxEventType EVT_GAL_REDRAW;

/**
 * LineCap: Type definition of the line end point style
 */
enum LineCap
{
    LINE_CAP_BUTT,      ///< Stop line at the end point
    LINE_CAP_ROUND,     ///< Draw a circle at the end point
    LINE_CAP_SQUARED    ///< Draw a square at the end point
};

/**
 * LineJoin: Type definition of the line joint style
 */
enum LineJoin
{
    LINE_JOIN_MITER,    ///< Use sharp corners
    LINE_JOIN_ROUND,    ///< Insert a circle at the joints
    LINE_JOIN_BEVEL     ///< Diagonal corner
};

/**
 * GridStyle: Type definition of the grid style
 */
enum GridStyle
{
    GRID_STYLE_LINES,   ///< Use lines for the grid
    GRID_STYLE_DOTS     ///< Use dots for the grid
};

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
class GAL
{
public:
    // Constructor / Destructor
    GAL();
    virtual ~GAL();

    // ---------------
    // Drawing methods
    // ---------------

    /// @brief Begin the drawing, needs to be called for every new frame.
    virtual void BeginDrawing() = 0;

    /// @brief End the drawing, needs to be called for every new frame.
    virtual void EndDrawing() = 0;

    /**
     * @brief Draw a line.
     *
     * Start and end points are defined as 2D-Vectors.
     *
     * @param aStartPoint   is the start point of the line.
     * @param aEndPoint     is the end point of the line.
     */
    virtual void DrawLine( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint ) = 0;

    /**
     * @brief Draw a rounded segment.
     *
     * Start and end points are defined as 2D-Vectors.
     *
     * @param aStartPoint   is the start point of the segment.
     * @param aEndPoint     is the end point of the segment.
     * @param aWidth        is a width of the segment
     */
    virtual void DrawSegment( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint, double aWidth ) = 0;

    /**
     * @brief Draw a polyline
     *
     * @param aPointList is a list of 2D-Vectors containing the polyline points.
     */
    virtual void DrawPolyline( std::deque<VECTOR2D>& aPointList ) = 0;

    /**
     * @brief Draw a circle using world coordinates.
     *
     * @param aCenterPoint is the center point of the circle.
     * @param aRadius is the radius of the circle.
     */
    virtual void DrawCircle( const VECTOR2D& aCenterPoint, double aRadius ) = 0;

    /**
     * @brief Draw an arc.
     *
     * @param aCenterPoint  is the center point of the arc.
     * @param aRadius       is the arc radius.
     * @param aStartAngle   is the start angle of the arc.
     * @param aEndAngle     is the end angle of the arc.
     */
    virtual void
    DrawArc( const VECTOR2D& aCenterPoint, double aRadius, double aStartAngle, double aEndAngle ) = 0;

    /**
     * @brief Draw a rectangle.
     *
     * @param aStartPoint   is the start point of the rectangle.
     * @param aEndPoint     is the end point of the rectangle.
     */
    virtual void DrawRectangle( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint ) = 0;

    /**
     * @brief Draw a polygon.
     *
     * @param aPointList is the list of the polygon points.
     */
    virtual void DrawPolygon( const std::deque<VECTOR2D>& aPointList ) = 0;

    /**
     * @brief Draw a cubic bezier spline.
     *
     * @param startPoint    is the start point of the spline.
     * @param controlPointA is the first control point.
     * @param controlPointB is the second control point.
     * @param endPoint      is the end point of the spline.
     */
    virtual void DrawCurve( const VECTOR2D& startPoint,    const VECTOR2D& controlPointA,
                            const VECTOR2D& controlPointB, const VECTOR2D& endPoint ) = 0;

    // --------------
    // Screen methods
    // --------------

    /// @brief Resizes the canvas.
    virtual void ResizeScreen( int aWidth, int aHeight ) = 0;

    /// @brief Shows/hides the GAL canvas
    virtual bool Show( bool aShow ) = 0;

    /// @brief Returns GAL canvas size in pixels
    VECTOR2D GetScreenPixelSize() const
    {
        return screenSize;
    }

    /// @brief Force all remaining objects to be drawn.
    virtual void Flush() = 0;

    /// @brief Clear the screen.
    virtual void ClearScreen() = 0;

    // -----------------
    // Attribute setting
    // -----------------

    /**
     * @brief Enable/disable fill.
     *
     * @param aIsFillEnabled is true, when the graphics objects should be filled, else false.
     */
    inline virtual void SetIsFill( bool aIsFillEnabled )
    {
        isFillEnabled = aIsFillEnabled;
    }

    /**
     * @brief Enable/disable stroked outlines.
     *
     * @param aIsStrokeEnabled is true, if the outline of an object should be stroked.
     */
    inline virtual void SetIsStroke( bool aIsStrokeEnabled )
    {
        isStrokeEnabled = aIsStrokeEnabled;
    }

    /**
     * @brief Set the fill color.
     *
     * @param aColor is the color for filling.
     */
    inline virtual void SetFillColor( COLOR4D aColor )
    {
        fillColor = aColor;
    }

    /**
     * @brief Set the stroke color.
     *
     * @param aColor is the color for stroking the outline.
     */
    inline virtual void SetStrokeColor( COLOR4D aColor )
    {
        strokeColor = aColor;
    }

    /**
     * @brief Get the stroke color.
     *
     * @return the color for stroking the outline.
     */
    inline COLOR4D GetStrokeColor()
    {
        return strokeColor;
    }

    /**
     * @brief Set the background color.
     *
     * @param aColor is the color for background filling.
     */
    virtual void SetBackgroundColor( COLOR4D aColor ) = 0;

    /**
     * @brief Set the style of the line caps.
     *
     * @param aLineCap is the line cap style.
     */
    inline virtual void SetLineCap( LineCap aLineCap )
    {
        lineCap = aLineCap;
    }

    /**
     * @brief Set the line join style.
     *
     * @param aLineJoin is the line join style.
     */
    inline virtual void SetLineJoin( LineJoin aLineJoin )
    {
        lineJoin = aLineJoin;
    }

    /**
     * @brief Set the line width.
     *
     * @param aLineWidth is the line width.
     */
    inline virtual void SetLineWidth( double aLineWidth )
    {
        lineWidth = aLineWidth;
    }

    /**
     * @brief Get the line width.
     *
     * @return the actual line width.
     */
    inline double GetLineWidth()
    {
        return lineWidth;
    }

    /**
     * @brief Set the depth of the layer (position on the z-axis)
     *
     * @param aLayerDepth the layer depth for the objects.
     */
    inline virtual void SetLayerDepth( double aLayerDepth )
    {
        layerDepth = aLayerDepth;
    }

    // --------------
    // Transformation
    // --------------

    /**
     * @brief Transform the context.
     *
     * @param aTransformation is the ransformation matrix.
     */
    virtual void Transform( MATRIX3x3D aTransformation ) = 0;

    /**
     * @brief Rotate the context.
     *
     * @param aAngle is the rotation angle in radians.
     */
    virtual void Rotate( double aAngle ) = 0;

    /**
     * @brief Translate the context.
     *
     * @param aTranslation is the translation vector.
     */
    virtual void Translate( const VECTOR2D& aTranslation ) = 0;

    /**
     * @brief Scale the context.
     *
     * @param aScale is the scale factor for the x- and y-axis.
     */
    virtual void Scale( const VECTOR2D& aScale ) = 0;

    /// @brief Save the context.
    virtual void Save() = 0;

    /// @brief Restore the context.
    virtual void Restore() = 0;

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
    virtual int BeginGroup() = 0;

    /// @brief End the group.
    virtual void EndGroup() = 0;

    /**
     * @brief Draw the stored group.
     *
     * @param aGroupNumber is the group number.
     */
    virtual void DrawGroup( int aGroupNumber ) = 0;

    /**
     * @brief Delete the group from the memory.
     *
     * @param aGroupNumber is the group number.
     */
    virtual void DeleteGroup( int aGroupNumber ) = 0;

    // --------------------------------------------------------
    // Handling the world <-> screen transformation
    // --------------------------------------------------------

    /// @brief Compute the world <-> screen transformation matrix
    virtual void ComputeWorldScreenMatrix() = 0;

    /**
     * @brief Get the world <-> screen transformation matrix.
     *
     * @return the transformation matrix.
     */
    MATRIX3x3D GetWorldScreenMatrix()
    {
        return worldScreenMatrix;
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
    inline VECTOR2D GetLookAtPoint()
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
    inline double GetZoomFactor()
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
    inline double GetMinDepth()
    {
        return depthRange.x;
    }

    /**
     * @brief Returns the maximum depth in the currently used range (the bottom).
     */
    inline double GetMaxDepth()
    {
        return depthRange.y;
    }

    /**
     * @brief Get the world scale.
     *
     * @return the actual world scale factor.
     */
    inline double GetWorldScale()
    {
        return worldScale;
    }

    /**
     * @brief Save the screen contents.
     */
    virtual void SaveScreen() = 0;

    /**
     * @brief Save the screen contents.
     */
    virtual void RestoreScreen() = 0;

    // -------------
    // Grid methods
    // -------------

    /**
     * @brief Set the origin point for the grid.
     *
     * @param aGridOrigin is a vector containing the grid origin point, in world coordinates.
     */
    inline void SetGridOrigin( const VECTOR2D& aGridOrigin )
    {
        gridOrigin = aGridOrigin;
    }

    /**
     * @brief Sets the screen size of the grid origin marker
     *
     * @param aSize is the radius of the origin marker, in pixels.
     */
    inline void SetGridOriginMarkerSize( int aSize )
    {
        gridOriginMarkerSize = aSize;
    }

    /**
     * @brief Set the threshold for grid drawing.
     *
     * @param aThreshold is the minimum grid cell size (in pixels) for which the grid is drawn.
     */
    inline void SetGridDrawThreshold( int aThreshold )
    {
        gridDrawThreshold = aThreshold;
    }

    /**
     * @brief Set the grid size.
     *
     * @param aGridSize is a vector containing the grid size in x- and y direction.
     */
    inline void SetGridSize( const VECTOR2D& aGridSize )
    {
        gridSize = aGridSize;
    }

    /**
     * @brief Set the grid color.
     *
     * @param aGridColor is the grid color, it should have a low alpha value for the best effect.
     */
    inline void SetGridColor( COLOR4D aGridColor )
    {
        gridColor = aGridColor;
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
    inline double GetGridLineWidth()
    {
        return gridLineWidth;
    }

    /**
     * @brief Set the grid line width.
     *
     * @param aGridLineWidth is the rid line width.
     */
    inline void SetGridLineWidth( double aGridLineWidth )
    {
        gridLineWidth = aGridLineWidth;
    }

    /// @brief Draw the grid
    void DrawGrid();

    // TODO Not yet implemented
    // virtual void SetGridStyle(GridStyle gridStyle);

    // -------
    // Cursor
    // -------

    /**
     * @brief Compute the cursor position in world coordinates from given screen coordinates.
     *
     * @param aCursorPosition is the cursor position in screen coordinates.
     * @return the cursor position in world coordinates.
     */
    virtual VECTOR2D ComputeCursorToWorld( const VECTOR2D& aCursorPosition ) = 0;

    /**
     * @brief Enable/Disable cursor.
     *
     * @param aIsCursorEnabled is true if the cursor should be enabled, else false.
     */
    inline void SetIsCursorEnabled( bool aIsCursorEnabled )
    {
        isCursorEnabled = aIsCursorEnabled;
    }

    /**
     * @brief Set the cursor color.
     *
     * @param aCursorColor is the color of the cursor.
     */
    inline void SetCursorColor( COLOR4D aCursorColor )
    {
        cursorColor = aCursorColor;
    }

    /**
     * @brief Draw the cursor.
     *
     * @param aCursorPosition is the cursor position in screen coordinates.
     */
    virtual void DrawCursor( VECTOR2D aCursorPosition ) = 0;

    void AdvanceDepth()
    {
        layerDepth -= 0.1;    // fixme: there should be a minimum step
    }

    /**
     * @brief Stores current drawing depth on the depth stack.
     */
    void PushDepth()
    {
        depthStack.push( layerDepth );
    }

    /**
     * @brief Restores previously stored drawing depth for the depth stack.
     */
    void PopDepth()
    {
        layerDepth = depthStack.top();
        depthStack.pop();
    }

protected:
    std::stack<double> depthStack;             ///< Stored depth values
    VECTOR2D           screenSize;             ///< Screen size in screen coordinates

    double             worldUnitLength;        ///< The unit length of the world coordinates [inch]
    double             screenDPI;              ///< The dots per inch of the screen
    VECTOR2D           lookAtPoint;            ///< Point to be looked at in world space

    double             zoomFactor;             ///< The zoom factor
    MATRIX3x3D         worldScreenMatrix;      ///< World transformation
    double             worldScale;             ///< The scale factor world->screen

    double             lineWidth;              ///< The line width
    LineCap            lineCap;                ///< Line end style
    LineJoin           lineJoin;               ///< Style of the line joints

    bool               isFillEnabled;          ///< Is filling of graphic objects enabled ?
    bool               isStrokeEnabled;        ///< Are the outlines stroked ?
    bool               isSetAttributes;        ///< True, if the attributes have been set

    COLOR4D            backgroundColor;        ///< The background color
    COLOR4D            fillColor;              ///< The fill color
    COLOR4D            strokeColor;            ///< The color of the outlines

    double             layerDepth;             ///< The actual layer depth
    VECTOR2D           depthRange;             ///< Range of the depth

    VECTOR2D           gridSize;               ///< The grid size
    VECTOR2D           gridOrigin;             ///< The grid origin
    COLOR4D            gridColor;              ///< Color of the grid
    int                gridTick;               ///< Every tick line gets the double width
    double             gridLineWidth;          ///< Line width of the grid
    int                gridDrawThreshold;      ///< Minimum screen size of the grid (pixels)
                                                ///< below which the grid is not drawn
    int                gridOriginMarkerSize;   ///< Grid origin indicator size (pixels)

    bool               isCursorEnabled;        ///< Is the cursor enabled?
    VECTOR2D           cursorPosition;         ///< The cursor position
    COLOR4D            cursorColor;            ///< Cursor color

    /// Compute the scaling factor for the world->screen matrix
    inline void ComputeWorldScale()
    {
        worldScale = screenDPI * worldUnitLength * zoomFactor;
    }

    /**
     * @brief Draw a grid line (usually a simplified line function).
     *
     * @param aStartPoint is the start point of the line.
     * @param aEndPoint is the end point of the line.
     */
    virtual void DrawGridLine( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint ) = 0;
};
} // namespace KiGfx

#endif /* GRAPHICSABSTRACTIONLAYER_H_ */
