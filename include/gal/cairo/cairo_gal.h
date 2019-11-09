/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright (C) 2012-2019 Kicad Developers, see change_log.txt for contributors.
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

#include <gal/graphics_abstraction_layer.h>
#include <wx/dcbuffer.h>

#include <memory>

/**
 * @brief Class CAIRO_GAL is the cairo implementation of the graphics abstraction layer.
 *
 * Quote from Wikipedia:
 * " Cairo is a software library used to provide a vector graphics-based, device-independent
 *   API for software developers. It is designed to provide primitives for 2-dimensional
 *   drawing across a number of different backends. "
 * <br>
 * Cairo offers also backends for Postscript and PDF surfaces. So it can be used for printing
 * of KiCad graphics surfaces as well.
 *
 */
namespace KIGFX
{
class CAIRO_COMPOSITOR;

class CAIRO_GAL_BASE : public GAL
{
public:
    CAIRO_GAL_BASE( GAL_DISPLAY_OPTIONS& aDisplayOptions );

    virtual ~CAIRO_GAL_BASE();

    virtual bool IsCairoEngine() override { return true; }

    // ---------------
    // Drawing methods
    // ---------------

    /// @copydoc GAL::DrawLine()
    virtual void DrawLine( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint ) override;

    /// @copydoc GAL::DrawSegment()
    virtual void DrawSegment( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint, double aWidth ) override;

    /// @copydoc GAL::DrawCircle()
    virtual void DrawCircle( const VECTOR2D& aCenterPoint, double aRadius ) override;

    /// @copydoc GAL::DrawArc()
    virtual void DrawArc( const VECTOR2D& aCenterPoint, double aRadius,
                          double aStartAngle, double aEndAngle ) override;

    /// @copydoc GAL::DrawArcSegment()
    virtual void DrawArcSegment( const VECTOR2D& aCenterPoint, double aRadius,
                                 double aStartAngle, double aEndAngle, double aWidth ) override;

    /// @copydoc GAL::DrawRectangle()
    virtual void DrawRectangle( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint ) override;

    /// @copydoc GAL::DrawPolyline()
    virtual void DrawPolyline( const std::deque<VECTOR2D>& aPointList ) override { drawPoly( aPointList ); }
    virtual void DrawPolyline( const VECTOR2D aPointList[], int aListSize ) override { drawPoly( aPointList, aListSize ); }
    virtual void DrawPolyline( const SHAPE_LINE_CHAIN& aLineChain ) override { drawPoly( aLineChain ); }

    /// @copydoc GAL::DrawPolygon()
    virtual void DrawPolygon( const std::deque<VECTOR2D>& aPointList ) override { drawPoly( aPointList ); }
    virtual void DrawPolygon( const VECTOR2D aPointList[], int aListSize ) override { drawPoly( aPointList, aListSize ); }
    virtual void DrawPolygon( const SHAPE_POLY_SET& aPolySet ) override;
    virtual void DrawPolygon( const SHAPE_LINE_CHAIN& aPolySet ) override;

    /// @copydoc GAL::DrawCurve()
    virtual void DrawCurve( const VECTOR2D& startPoint, const VECTOR2D& controlPointA,
                            const VECTOR2D& controlPointB, const VECTOR2D& endPoint,
                            double aFilterValue = 0.0 ) override;

    /// @copydoc GAL::DrawBitmap()
    virtual void DrawBitmap( const BITMAP_BASE& aBitmap ) override;

    // --------------
    // Screen methods
    // --------------

    /// @brief Resizes the canvas.
    virtual void ResizeScreen( int aWidth, int aHeight ) override;

    /// @copydoc GAL::Flush()
    virtual void Flush() override;

    /// @copydoc GAL::ClearScreen()
    virtual void ClearScreen() override;

    // -----------------
    // Attribute setting
    // -----------------

    /// @copydoc GAL::SetIsFill()
    virtual void SetIsFill( bool aIsFillEnabled ) override;

    /// @copydoc GAL::SetIsStroke()
    virtual void SetIsStroke( bool aIsStrokeEnabled ) override;

    /// @copydoc GAL::SetStrokeColor()
    virtual void SetStrokeColor( const COLOR4D& aColor ) override;

    /// @copydoc GAL::SetFillColor()
    virtual void SetFillColor( const COLOR4D& aColor ) override;

    /// @copydoc GAL::SetLineWidth()
    virtual void SetLineWidth( float aLineWidth ) override;

    /// @copydoc GAL::SetLayerDepth()
    virtual void SetLayerDepth( double aLayerDepth ) override;

    // --------------
    // Transformation
    // --------------

    /// @copydoc GAL::Transform()
    virtual void Transform( const MATRIX3x3D& aTransformation ) override;

    /// @copydoc GAL::Rotate()
    virtual void Rotate( double aAngle ) override;

    /// @copydoc GAL::Translate()
    virtual void Translate( const VECTOR2D& aTranslation ) override;

    /// @copydoc GAL::Scale()
    virtual void Scale( const VECTOR2D& aScale ) override;

    /// @copydoc GAL::Save()
    virtual void Save() override;

    /// @copydoc GAL::Restore()
    virtual void Restore() override;

    // --------------------------------------------
    // Group methods
    // ---------------------------------------------

    /// @copydoc GAL::BeginGroup()
    virtual int BeginGroup() override;

    /// @copydoc GAL::EndGroup()
    virtual void EndGroup() override;

    /// @copydoc GAL::DrawGroup()
    virtual void DrawGroup( int aGroupNumber ) override;

    /// @copydoc GAL::ChangeGroupColor()
    virtual void ChangeGroupColor( int aGroupNumber, const COLOR4D& aNewColor ) override;

    /// @copydoc GAL::ChangeGroupDepth()
    virtual void ChangeGroupDepth( int aGroupNumber, int aDepth ) override;

    /// @copydoc GAL::DeleteGroup()
    virtual void DeleteGroup( int aGroupNumber ) override;

    /// @copydoc GAL::ClearCache()
    virtual void ClearCache() override;

    // --------------------------------------------------------
    // Handling the world <-> screen transformation
    // --------------------------------------------------------

    /// @copydoc GAL::SetNegativeDrawMode()
    virtual void SetNegativeDrawMode( bool aSetting ) override;

    // -------
    // Cursor
    // -------

    /// @copydoc GAL::DrawCursor()
    virtual void DrawCursor( const VECTOR2D& aCursorPosition ) override;

    virtual void EnableDepthTest( bool aEnabled = false ) override;

    ///> @copydoc GAL::DrawGrid()
    virtual void DrawGrid() override;


protected:
    // Geometric transforms according to the currentWorld2Screen transform matrix:
    const double xform( double x );             // scale
    const VECTOR2D xform( double x, double y ); // rotation, scale and offset
    const VECTOR2D xform( const VECTOR2D& aP ); // rotation, scale and offset

    /** Transform according to the rotation from currentWorld2Screen transform matrix:
     * @param aAngle is the angle in radians to transform
     * @return the modified angle
     */
    const double angle_xform( const double aAngle );

    /** Transform according to the rotation from currentWorld2Screen transform matrix
     * for the start angle and the end angle of an arc
     * @param aStartAngle is the arc starting point in radians to transform
     * @param aEndAngle is the arc ending point in radians to transform
     */
    void arc_angles_xform_and_normalize( double& aStartAngle, double& aEndAngle );

    /// @copydoc GAL::BeginDrawing()
    virtual void beginDrawing() override;

    /// @copydoc GAL::EndDrawing()
    virtual void endDrawing() override;

    void resetContext();

    /**
     * @brief Draw a grid line (usually a simplified line function).
     *
     * @param aStartPoint is the start point of the line.
     * @param aEndPoint is the end point of the line.
     */
    void drawGridLine( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint );
    void drawGridCross( const VECTOR2D& aPoint );
    void drawGridPoint( const VECTOR2D& aPoint, double aSize );
    void drawAxes( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint );

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
    typedef struct
    {
        GRAPHICS_COMMAND command;                   ///< Command to execute
        union {
            double dblArg[MAX_CAIRO_ARGUMENTS];     ///< Arguments for Cairo commands
            bool boolArg;                           ///< A bool argument
            int intArg;                             ///< An int argument
        } argument;
        cairo_path_t* cairoPath;                    ///< Pointer to a Cairo path
    } GROUP_ELEMENT;

    // Variables for the grouping function
    bool                        isGrouping;         ///< Is grouping enabled ?
    bool                        isElementAdded;     ///< Was an graphic element added ?
    typedef std::deque<GROUP_ELEMENT> GROUP;        ///< A graphic group type definition
    std::map<int, GROUP>        groups;             ///< List of graphic groups
    unsigned int                groupCounter;       ///< Counter used for generating keys for groups
    GROUP*                      currentGroup;       ///< Currently used group

    double lineWidth;
    double linePixelWidth;
    double lineWidthInPixels;
    bool lineWidthIsOdd;

    cairo_matrix_t      cairoWorldScreenMatrix; ///< Cairo world to screen transformation matrix
    cairo_matrix_t      currentXform;
    cairo_matrix_t      currentWorld2Screen;
    cairo_t*            currentContext;         ///< Currently used Cairo context for drawing
    cairo_t*            context;                ///< Cairo image
    cairo_surface_t*    surface;                ///< Cairo surface

    std::vector<cairo_matrix_t> xformStack;

    void flushPath();
    void storePath();                           ///< Store the actual path

    /**
     * @brief Blits cursor into the current screen.
     */
    virtual void blitCursor( wxMemoryDC& clientDC );

    /// Drawing polygons & polylines is the same in cairo, so here is the common code
    void drawPoly( const std::deque<VECTOR2D>& aPointList );
    void drawPoly( const VECTOR2D aPointList[], int aListSize );
    void drawPoly( const SHAPE_LINE_CHAIN& aLineChain );

    /**
     * @brief Returns a valid key that can be used as a new group number.
     *
     * @return An unique group number that is not used by any other group.
     */
    unsigned int getNewGroupNumber();

    void syncLineWidth( bool aForceWidth = false, double aWidth = 0.0 );
    void updateWorldScreenMatrix();
    const VECTOR2D roundp( const VECTOR2D& v );


    /// Format used to store pixels
    static constexpr cairo_format_t GAL_FORMAT = CAIRO_FORMAT_ARGB32;
};


class CAIRO_GAL : public CAIRO_GAL_BASE, public wxWindow
{
public:
    /**
     * Constructor CAIRO_GAL_BASE
     *
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
    CAIRO_GAL( GAL_DISPLAY_OPTIONS& aDisplayOptions,
               wxWindow* aParent, wxEvtHandler* aMouseListener = NULL,
               wxEvtHandler* aPaintListener = NULL, const wxString& aName = wxT( "CairoCanvas" ) );

    virtual ~CAIRO_GAL();

    ///> @copydoc GAL::IsVisible()
    bool IsVisible() const override
    {
        return IsShownOnScreen() && !GetClientRect().IsEmpty();
    }

    virtual void ResizeScreen( int aWidth, int aHeight ) override;

    virtual bool Show( bool aShow ) override;

    virtual int BeginGroup() override;

    virtual void EndGroup() override;

    virtual void SetTarget( RENDER_TARGET aTarget ) override;

    virtual RENDER_TARGET GetTarget() const override;

    virtual void ClearTarget( RENDER_TARGET aTarget ) override;

    /**
     * Function PostPaint
     * posts an event to m_paint_listener.  A post is used so that the actual drawing
     * function can use a device context type that is not specific to the wxEVT_PAINT event.
     */
    void PostPaint()
    {
        if( paintListener )
        {
            wxPaintEvent redrawEvent;
            wxPostEvent( paintListener, redrawEvent );
        }
    }

    void SetMouseListener( wxEvtHandler* aMouseListener )
    {
        mouseListener = aMouseListener;
    }

    void SetPaintListener( wxEvtHandler* aPaintListener )
    {
        paintListener = aPaintListener;
    }

protected:
    // Compositor related variables
    std::shared_ptr<CAIRO_COMPOSITOR> compositor;   ///< Object for layers compositing
    unsigned int            mainBuffer;             ///< Handle to the main buffer
    unsigned int            overlayBuffer;          ///< Handle to the overlay buffer
    RENDER_TARGET           currentTarget;          ///< Current rendering target
    bool                    validCompositor;        ///< Compositor initialization flag

    // Variables related to wxWidgets
    wxWindow*               parentWindow;           ///< Parent window
    wxEvtHandler*           mouseListener;          ///< Mouse listener
    wxEvtHandler*           paintListener;          ///< Paint listener
    unsigned int            bufferSize;             ///< Size of buffers cairoOutput, bitmapBuffers
    unsigned char*          wxOutput;               ///< wxImage comaptible buffer

    // Variables related to Cairo <-> wxWidgets
    unsigned char*      bitmapBuffer;           ///< Storage of the cairo image
    int                 stride;                 ///< Stride value for Cairo
    int                 wxBufferWidth;
    bool                isInitialized;          ///< Are Cairo image & surface ready to use
    COLOR4D             backgroundColor;        ///< Background color

    /// @copydoc GAL::BeginDrawing()
    virtual void beginDrawing() override;

    /// @copydoc GAL::EndDrawing()
    virtual void endDrawing() override;

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
     * @brief Paint event handler.
     *
     * @param aEvent is the paint event.
     */
    void onPaint( wxPaintEvent& aEvent );

    /**
     * @brief Mouse event handler, forwards the event to the child.
     *
     * @param aEvent is the mouse event to be forwarded.
     */
    void skipMouseEvent( wxMouseEvent& aEvent );

    ///> Cairo-specific update handlers
    bool updatedGalDisplayOptions( const GAL_DISPLAY_OPTIONS& aOptions ) override;
};

} // namespace KIGFX

#endif  // CAIROGAL_H_
