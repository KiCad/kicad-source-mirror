/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright (C) 2012 Kicad Developers, see change_log.txt for contributors.
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


#if defined(__WXMSW__)
#define SCREEN_DEPTH 24
#else
#if wxCHECK_VERSION( 2, 9, 0 )
#define SCREEN_DEPTH wxBITMAP_SCREEN_DEPTH
#else
#define SCREEN_DEPTH 32
#endif
#endif

#define EXCEPTION_ZERO_CLIENT_RECTANGLE 0
#define EXCEPTION_ZERO_CONTEXT          1

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
namespace KiGfx
{
class CAIRO_GAL : public GAL, public wxWindow
{
public:
    /**
     * Constructor CAIRO_GAL
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
    CAIRO_GAL( wxWindow* aParent, wxEvtHandler* aMouseListener = NULL,
               wxEvtHandler* aPaintListener = NULL, const wxString& aName = wxT("CairoCanvas") );

    virtual ~CAIRO_GAL();

    // ---------------
    // Drawing methods
    // ---------------

    /// @copydoc GAL::BeginDrawing()
    virtual void BeginDrawing() throw (int);

    /// @copydoc GAL::EndDrawing()
    virtual void EndDrawing();

    /// @copydoc GAL::DrawLine()
    virtual void DrawLine( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint );

    /// @copydoc GAL::DrawSegment()
    virtual void DrawSegment( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint, double aWidth );

    /// @copydoc GAL::DrawPolyline()
    virtual void DrawPolyline( std::deque<VECTOR2D>& aPointList );

    /// @copydoc GAL::DrawCircle()
    virtual void DrawCircle( const VECTOR2D& aCenterPoint, double aRadius );

    /// @copydoc GAL::DrawArc()
    virtual void
    DrawArc( const VECTOR2D& aCenterPoint, double aRadius, double aStartAngle, double aEndAngle );

    /// @copydoc GAL::DrawRectangle()
    virtual void DrawRectangle( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint );

    /// @copydoc GAL::DrawPolygon()
    virtual void DrawPolygon( const std::deque<VECTOR2D>& aPointList );

    /// @copydoc GAL::DrawCurve()
    virtual void DrawCurve( const VECTOR2D& startPoint, const VECTOR2D& controlPointA,
                            const VECTOR2D& controlPointB, const VECTOR2D& endPoint );

    // --------------
    // Screen methods
    // --------------

    /// @brief Resizes the canvas.
    virtual void ResizeScreen ( int aWidth, int aHeight );

    /// @brief Shows/hides the GAL canvas
    virtual bool Show( bool aShow );

    /// @copydoc GAL::Flush()
    virtual void Flush();

    /// @copydoc GAL::ClearScreen()
    virtual void ClearScreen();

    // -----------------
    // Attribute setting
    // -----------------

    /// @copydoc GAL::SetIsFill()
    virtual void SetIsFill( bool aIsFillEnabled );

    /// @copydoc GAL::SetIsStroke()
    virtual void SetIsStroke( bool aIsStrokeEnabled );

    /// @copydoc GAL::SetFillColor()
    virtual void SetFillColor( const COLOR4D& aColor );

    /// @copydoc GAL::SetStrokeColor()
    virtual void SetStrokeColor( const COLOR4D& aColor );

    /// @copydoc GAL::GetStrokeColor()
    COLOR4D      GetStrokeColor();

    /// @copydoc GAL::SetBackgroundColor()
    virtual void SetBackgroundColor( const COLOR4D& aColor );

    /// @copydoc GAL::SetLineCap()
    virtual void SetLineCap( LineCap aLineCap );

    /// @copydoc GAL::SetLineJoin()
    virtual void SetLineJoin( LineJoin aLineJoin );

    /// @copydoc GAL::SetLineWidth()
    virtual void SetLineWidth( double aLineWidth );

    /// @copydoc GAL::GetLineWidth()
    double       GetLineWidth();

    /// @copydoc GAL::SetLayerDepth()
    virtual void SetLayerDepth( double aLayerDepth );

    // --------------
    // Transformation
    // --------------

    /// @copydoc GAL::Transform()
    virtual void Transform( MATRIX3x3D aTransformation );

    /// @copydoc GAL::Rotate()
    virtual void Rotate( double aAngle );

    /// @copydoc GAL::Translate()
    virtual void Translate( const VECTOR2D& aTranslation );

    /// @copydoc GAL::Scale()
    virtual void Scale( const VECTOR2D& aScale );

    /// @copydoc GAL::Save()
    virtual void Save();

    /// @copydoc GAL::Restore()
    virtual void Restore();

    // --------------------------------------------
    // Group methods
    // ---------------------------------------------

    /// @copydoc GAL::BeginGroup()
    virtual int BeginGroup();

    /// @copydoc GAL::EndGroup()
    virtual void EndGroup();

    /// @copydoc GAL::DrawGroup()
    virtual void DrawGroup( int aGroupNumber );

    /// @copydoc GAL::DeleteGroup()
    virtual void DeleteGroup( int aGroupNumber );

    /// @copydoc GAL::ClearCache()
    virtual void ClearCache();

    // --------------------------------------------------------
    // Handling the world <-> screen transformation
    // --------------------------------------------------------

    /// @copydoc GAL::ComputeWorldScreenMatrix()
    virtual void ComputeWorldScreenMatrix();

    /// @copydoc GAL::GetWorldScreenMatrix()
    MATRIX3x3D GetWorldScreenMatrix();

    /// @copydoc GAL::SetWorldScreenMatrix()
    void SetWorldScreenMatrix( MATRIX3x3D aMatrix );

    /// @copydoc GAL::SetWorldUnitLength()
    void SetWorldUnitLength( double aWorldUnitLength );

    /// @copydoc GAL::SetScreenDPI()
    void SetScreenDPI( double aScreenDPI );

    /// @copydoc GAL::SetLookAtPoint()
    void SetLookAtPoint( const VECTOR2D& aPoint );

    /// @copydoc GAL::GetLookAtPoint()
    VECTOR2D GetLookAtPoint();

    /// @copydoc GAL::SetZoomFactor()
    void SetZoomFactor( double aZoomFactor );

    /// @copydoc GAL::GetZoomFactor()
    double GetZoomFactor();

    /// @copydoc GAL::SaveScreen()
    virtual void SaveScreen();

    /// @copydoc GAL::RestoreScreen()
    virtual void RestoreScreen();

    // -------
    // Cursor
    // -------

    /// @copydoc GAL::ComputeCursorToWorld()
    virtual VECTOR2D ComputeCursorToWorld( const VECTOR2D& aCursorPosition );

    /// @copydoc GAL::SetIsCursorEnabled()
    void SetIsCursorEnabled( bool aIsCursorEnabled );

    /// @copydoc GAL::DrawCursor()
    virtual void DrawCursor( VECTOR2D aCursorPosition );

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
    virtual void DrawGridLine( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint );

private:
    /// Super class definition
    typedef GAL super;

    // Variables related to wxWidgets
    wxWindow*                   parentWindow;       ///< Parent window
    wxEvtHandler*               mouseListener;      ///< Mouse listener
    wxEvtHandler*               paintListener;      ///< Paint listener
    wxRect                      clientRectangle;    ///< Area definition of the surface
    unsigned int                bufferSize;         ///< Size of buffers cairoOutput, bitmapBuffers
    unsigned char*              wxOutput;           ///< wxImage comaptible buffer

    // Cursor variables
    std::deque<wxColour>        savedCursorPixels;      ///< Saved pixels of the cursor
    bool                        isDeleteSavedPixels;    ///< True, if the saved pixels can be discarded
    wxPoint                     savedCursorPosition;    ///< The last cursor position
    wxBitmap*                   cursorPixels;           ///< Cursor pixels
    wxBitmap*                   cursorPixelsSaved;      ///< Saved cursor pixels
    int                         cursorSize;             ///< Cursor size

    // Variables for the grouping function
    int                         actualGroupIndex;   ///< The index of the actual group
    bool                        isGrouping;         ///< Is grouping enabled ?
    bool                        isElementAdded;     ///< Was an graphic element added ?
    std::deque<cairo_path_t*>   pathList;           ///< List of stored paths

    /// Maximum number of arguments for one command
    static const int MAX_CAIRO_ARGUMENTS = 6;

    /// Definitions for the command recorder
    enum GraphicsCommand
    {
        CMD_SET_FILL,                               ///< Enable/disable filling
        CMD_SET_STROKE,                             ///< Enable/disable stroking
        CMD_SET_FILLCOLOR,                          ///< Set the fill color
        CMD_SET_STROKECOLOR,                        ///< Set the stroke color
        CMD_SET_LINE_WIDTH,                         ///< Set the line width
        CMD_SET_LINE_CAP,                           ///< Set the line cap style
        CMD_SET_LINE_JOIN,                          ///< Set the line join style
        CMD_STROKE_PATH,                            ///< Set the stroke path
        CMD_FILL_PATH,                              ///< Set the fill path
        CMD_TRANSFORM,                              ///< Transform the actual context
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
        GraphicsCommand command;                    ///< Command to execute
        double arguments[MAX_CAIRO_ARGUMENTS];      ///< Arguments for Cairo commands
        bool boolArgument;                          ///< A bool argument
        int intArgument;                            ///< An int argument
        cairo_path_t* cairoPath;                    ///< Pointer to a Cairo path
    } GroupElement;

    typedef std::deque<GroupElement> Group;         ///< A graphic group type definition
    std::deque<Group> groups;                       ///< List of graphic groups

    // Variables related to Cairo <-> wxWidgets
    cairo_matrix_t      cairoWorldScreenMatrix; ///< Cairo world to screen transformation matrix
    cairo_t*            cairoImage;             ///< Cairo image
    cairo_surface_t*    cairoSurface;           ///< Cairo surface
    unsigned int*       bitmapBuffer;           ///< Storage of the cairo image
    unsigned int*       bitmapBufferBackup;     ///< Backup storage of the cairo image
    int                 stride;                 ///< Stride value for Cairo
    bool                isInitialized;          ///< Are Cairo image & surface ready to use

    // Mapping between Cairo and GAL line attributes
    std::map<LineCap, cairo_line_cap_t>     lineCapMap;     ///< Line cap style mapping
    std::map<LineJoin, cairo_line_join_t>   lineJoinMap;    ///< Line join style mapping

    // Methods
    void storePath();                           ///< Store the actual path

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

    /**
     * @brief Initialize the cursor.
     *
     * @param aCursorSize is the size of the cursor.
     */
    void initCursor( int aCursorSize );

    /// Allocate the bitmaps for drawing
    void allocateBitmaps();

    /// Allocate the bitmaps for drawing
    void deleteBitmaps();

    /// Prepare Cairo surfaces for drawing
    void initSurface();

    // Destroy Cairo surfaces when are not needed anymore
    void deinitSurface();
};
} // namespace KiGfx

#endif // CAIROGAL_H_
