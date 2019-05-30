#ifndef __EDA_DRAW_PANEL_H
#define __EDA_DRAW_PANEL_H

#include <wx/wx.h>
#include <base_struct.h>
#include <gr_basic.h>
#include <eda_rect.h>

class BASE_SCREEN;


class EDA_DRAW_PANEL
{
protected:
    bool    m_showCrossHair;                ///< Indicate if cross hair is to be shown.
    int     m_cursorLevel;                  ///< Index for cursor redraw in XOR mode.
    int     m_scrollIncrementX;             ///< X axis scroll increment in pixels per unit.
    int     m_scrollIncrementY;             ///< Y axis scroll increment in pixels per unit.

    /// The drawing area used to redraw the screen which is usually the visible area
    /// of the drawing in internal units.
    EDA_RECT    m_ClipBox;

    bool    m_ignoreMouseEvents;            ///< Ignore mouse events when true.

    /* Used to inhibit a response to a mouse left button release, after a double click
     * (when releasing the left button at the end of the second click.  Used in Eeschema
     * to inhibit a mouse left release command when switching between hierarchical sheets
     * on a double click.
     */
    bool    m_ignoreNextLeftButtonRelease;  ///< Ignore the next mouse left button release when true.

    /// True when drawing in mirror mode. Used by the draw arc function, because arcs
    /// are oriented, and in mirror mode, orientations are reversed.
    bool    m_PrintIsMirrored;

    int     m_doubleClickInterval;

public:

    EDA_DRAW_PANEL() :
        m_showCrossHair( true ),
        m_cursorLevel( 0 ),
        m_scrollIncrementX( 1 ),
        m_scrollIncrementY( 1 ),
        m_ignoreMouseEvents( false ),
        m_ignoreNextLeftButtonRelease( false ),
        m_PrintIsMirrored( false ),
        m_doubleClickInterval( 0 )
    {};

    virtual ~EDA_DRAW_PANEL(){};

    /**
     * Function GetDisplayOptions
     * A way to pass info to draw functions.
     * this is just an accessor to the GetDisplayOptions() parent frame function.
     */
    virtual void* GetDisplayOptions() { printf("EDA_DRAW_PANEL:Unimplemented\n"); wxASSERT(false); return nullptr; };

    virtual BASE_SCREEN* GetScreen() = 0;

    virtual EDA_DRAW_FRAME* GetParent() const = 0;

    //virtual void OnPaint( wxPaintEvent& event );

    virtual EDA_RECT* GetClipBox() { return &m_ClipBox; }

    void SetClipBox( const EDA_RECT& aRect ) { m_ClipBox = aRect; }

    void SetIgnoreMouseEvents( bool aIgnore ) { m_ignoreMouseEvents = aIgnore; }

    void SetIgnoreLeftButtonReleaseEvent( bool aIgnore ) { m_ignoreNextLeftButtonRelease = aIgnore; }

    bool GetPrintMirrored() const               { return m_PrintIsMirrored; }
    void SetPrintMirrored( bool aMirror )       { m_PrintIsMirrored = aMirror; }

    /* Mouse and keys events */

    /**
     * Function OnMouseWheel
     * handles mouse wheel events.
     * <p>
     * The mouse wheel is used to provide support for zooming and panning.  This
     * is accomplished by converting mouse wheel events in pseudo menu command
     * events and sending them to the appropriate parent window event handler.
     *</p>
     */

    virtual void SetZoom( double mode ) { printf("EDA_DRAW_PANEL:Unimplemented7\n");  };;
    virtual double GetZoom() { return 1.0; };;

    //virtual void SetGrid( const wxRealPoint& size ) { printf("EDA_DRAW_PANEL:Unimplemented\n");  };;
    //virtual wxRealPoint GetGrid() { printf("EDA_DRAW_PANEL:Unimplemented\n"); return wxRealPoint(1.0, 1.0); };;


    /**
     * Function SetClipBox
     * sets the clip box in drawing (logical) units from \a aRect in device units.
     * <p>
     * If \a aRect is NULL, then the entire visible area of the screen is used as the clip
     * area.  The clip box is used when drawing to determine which objects are not visible
     * and do not need to be drawn.  Note that this is not the same as setting the device
     * context clipping with wxDC::SetClippingRegion().  This is the rectangle used by the
     * drawing functions in gr_basic.cpp used to determine if the item to draw is off screen
     * and therefore not drawn.
     * </p>
     * @param aDC The device context use for drawing with the correct scale and
     *            offsets already configured.  See DoPrepareDC().
     * @param aRect The clip rectangle in device units or NULL for the entire visible area
     *              of the screen.
     */
    virtual void SetClipBox( wxDC& aDC, const wxRect* aRect = NULL ) { printf("EDA_DRAW_PANEL:Unimplemented10\n"); };;

    /**
     * Function RefreshDrawingRect
     * redraws the contents of \a aRect in drawing units.  \a aRect is converted to
     * screen coordinates and wxWindow::RefreshRect() is called to repaint the region.
     * @param aRect The rectangle to repaint.
     * @param aEraseBackground Erases the background if true.
     */
    virtual void RefreshDrawingRect( const EDA_RECT& aRect, bool aEraseBackground = true ) { printf("EDA_DRAW_PANEL:Unimplemented12\n"); };;

    /// @copydoc wxWindow::Refresh()
    //virtual void Refresh( bool eraseBackground = true, const wxRect* rect = NULL );

    /**
     * Function GetScreenCenterLogicalPosition
     * @return The current screen center position in logical (drawing) units.
     */
    virtual wxPoint GetScreenCenterLogicalPosition() { return wxPoint(0, 0); };;

    /**
     * Function MoveCursorToCrossHair
     * warps the cursor to the current cross hair position.
     */
    virtual void MoveCursorToCrossHair() { printf("EDA_DRAW_PANEL:Unimplemented14\n"); };;

    /**
     * Function ToDeviceXY
     * transforms logical to device coordinates
     */
    virtual wxPoint ToDeviceXY( const wxPoint& pos ) { printf("EDA_DRAW_PANEL:Unimplemented15\n"); return wxPoint(0, 0); };;

    /**
     * Function ToLogicalXY
     * transforms device to logical coordinates
     */
    virtual wxPoint ToLogicalXY( const wxPoint& pos ) { printf("EDA_DRAW_PANEL:Unimplemented16\n"); return wxPoint(0, 0); };;

    /**
     * Function MoveCursor
     * moves the mouse pointer to \a aPosition in logical (drawing) units.
     * @param aPosition The position in logical units to move the cursor.
     */
    virtual void MoveCursor( const wxPoint& aPosition ) { printf("EDA_DRAW_PANEL:Unimplemented17\n");  };;

    virtual void Refresh( bool eraseBackground = true, const wxRect* rect = NULL ) {}

    virtual wxWindow* GetWindow() = 0;
};

#endif
