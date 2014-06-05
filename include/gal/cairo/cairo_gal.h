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
#include <boost/smart_ptr/shared_ptr.hpp>
#include <wx/dcbuffer.h>

#if defined(__WXMSW__)
#define SCREEN_DEPTH 24
#else
#if wxCHECK_VERSION( 2, 9, 0 )
#define SCREEN_DEPTH    wxBITMAP_SCREEN_DEPTH
#else
#define SCREEN_DEPTH    32
#endif
#endif

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
               wxEvtHandler* aPaintListener = NULL, const wxString& aName = wxT( "CairoCanvas" ) );

    virtual ~CAIRO_GAL();

    // ---------------
    // Drawing methods
    // ---------------

    /// @copydoc GAL::BeginDrawing()
    virtual void BeginDrawing();

    /// @copydoc GAL::EndDrawing()
    virtual void EndDrawing();

    /// @copydoc GAL::DrawLine()
    virtual void DrawLine( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint );

    /// @copydoc GAL::DrawSegment()
    virtual void DrawSegment( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint, double aWidth );

    /// @copydoc GAL::DrawCircle()
    virtual void DrawCircle( const VECTOR2D& aCenterPoint, double aRadius );

    /// @copydoc GAL::DrawArc()
    virtual void DrawArc( const VECTOR2D& aCenterPoint, double aRadius,
                          double aStartAngle, double aEndAngle );

    /// @copydoc GAL::DrawRectangle()
    virtual void DrawRectangle( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint );

    /// @copydoc GAL::DrawPolyline()
    virtual void DrawPolyline( std::deque<VECTOR2D>& aPointList );

    /// @copydoc GAL::DrawPolygon()
    virtual void DrawPolygon( const std::deque<VECTOR2D>& aPointList );

    /// @copydoc GAL::DrawCurve()
    virtual void DrawCurve( const VECTOR2D& startPoint, const VECTOR2D& controlPointA,
                            const VECTOR2D& controlPointB, const VECTOR2D& endPoint );

    // --------------
    // Screen methods
    // --------------

    /// @brief Resizes the canvas.
    virtual void ResizeScreen( int aWidth, int aHeight );

    /// @brief Shows/hides the GAL canvas
    virtual bool Show( bool aShow );

    /// @copydoc GAL::Flush()
    virtual void Flush();

    /// @copydoc GAL::ClearScreen()
    virtual void ClearScreen( const COLOR4D& aColor );

    // -----------------
    // Attribute setting
    // -----------------

    /// @copydoc GAL::SetIsFill()
    virtual void SetIsFill( bool aIsFillEnabled );

    /// @copydoc GAL::SetIsStroke()
    virtual void SetIsStroke( bool aIsStrokeEnabled );

    /// @copydoc GAL::SetStrokeColor()
    virtual void SetStrokeColor( const COLOR4D& aColor );

    /// @copydoc GAL::SetFillColor()
    virtual void SetFillColor( const COLOR4D& aColor );

    /// @copydoc GAL::SetLineWidth()
    virtual void SetLineWidth( double aLineWidth );

    /// @copydoc GAL::SetLayerDepth()
    virtual void SetLayerDepth( double aLayerDepth );

    // --------------
    // Transformation
    // --------------

    /// @copydoc GAL::Transform()
    virtual void Transform( const MATRIX3x3D& aTransformation );

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

    /// @copydoc GAL::ChangeGroupColor()
    virtual void ChangeGroupColor( int aGroupNumber, const COLOR4D& aNewColor );

    /// @copydoc GAL::ChangeGroupDepth()
    virtual void ChangeGroupDepth( int aGroupNumber, int aDepth );

    /// @copydoc GAL::DeleteGroup()
    virtual void DeleteGroup( int aGroupNumber );

    /// @copydoc GAL::ClearCache()
    virtual void ClearCache();

    // --------------------------------------------------------
    // Handling the world <-> screen transformation
    // --------------------------------------------------------

    /// @copydoc GAL::SaveScreen()
    virtual void SaveScreen();

    /// @copydoc GAL::RestoreScreen()
    virtual void RestoreScreen();

    /// @copydoc GAL::SetTarget()
    virtual void SetTarget( RENDER_TARGET aTarget );

    /// @copydoc GAL::GetTarget()
    virtual RENDER_TARGET GetTarget() const;

    /// @copydoc GAL::ClearTarget()
    virtual void ClearTarget( RENDER_TARGET aTarget );

    // -------
    // Cursor
    // -------

    /// @copydoc GAL::DrawCursor()
    virtual void DrawCursor( const VECTOR2D& aCursorPosition );

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
    virtual void drawGridLine( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint );

private:
    /// Super class definition
    typedef GAL super;

    // Compositing variables
    boost::shared_ptr<CAIRO_COMPOSITOR> compositor; ///< Object for layers compositing
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

    // Cursor variables
    std::deque<wxColour>    savedCursorPixels;      ///< Saved pixels of the cursor
    bool                    isDeleteSavedPixels;    ///< True, if the saved pixels can be discarded
    wxPoint                 savedCursorPosition;    ///< The last cursor position
    wxBitmap*               cursorPixels;           ///< Cursor pixels
    wxBitmap*               cursorPixelsSaved;      ///< Saved cursor pixels

    /// Maximum number of arguments for one command
    static const int MAX_CAIRO_ARGUMENTS = 6;

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
        GRAPHICS_COMMAND command;                   ///< Command to execute
        double arguments[MAX_CAIRO_ARGUMENTS];      ///< Arguments for Cairo commands
        bool boolArgument;                          ///< A bool argument
        int intArgument;                            ///< An int argument
        cairo_path_t* cairoPath;                    ///< Pointer to a Cairo path
    } GROUP_ELEMENT;

    // Variables for the grouping function
    bool                        isGrouping;         ///< Is grouping enabled ?
    bool                        isElementAdded;     ///< Was an graphic element added ?
    typedef std::deque<GROUP_ELEMENT> GROUP;        ///< A graphic group type definition
    std::map<int, GROUP>        groups;             ///< List of graphic groups
    unsigned int                groupCounter;       ///< Counter used for generating keys for groups
    GROUP*                      currentGroup;       ///< Currently used group

    // Variables related to Cairo <-> wxWidgets
    cairo_matrix_t      cairoWorldScreenMatrix; ///< Cairo world to screen transformation matrix
    cairo_t*            currentContext;         ///< Currently used Cairo context for drawing
    cairo_t*            context;                ///< Cairo image
    cairo_surface_t*    surface;                ///< Cairo surface
    unsigned int*       bitmapBuffer;           ///< Storage of the cairo image
    unsigned int*       bitmapBufferBackup;     ///< Backup storage of the cairo image
    int                 stride;                 ///< Stride value for Cairo
    bool                isInitialized;          ///< Are Cairo image & surface ready to use
    COLOR4D             backgroundColor;        ///< Background color

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
     * @brief Prepares cursor bitmap.
     */
    virtual void initCursor();

    /**
     * @brief Blits cursor into the current screen.
     */
    virtual void blitCursor( wxBufferedDC& clientDC );

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

    /**
     * @brief Returns a valid key that can be used as a new group number.
     *
     * @return An unique group number that is not used by any other group.
     */
    unsigned int getNewGroupNumber();

    /// Format used to store pixels
    static const cairo_format_t GAL_FORMAT = CAIRO_FORMAT_RGB24;

    ///> Opacity of a single layer
    static const float LAYER_ALPHA;
};
} // namespace KIGFX

#endif  // CAIROGAL_H_
