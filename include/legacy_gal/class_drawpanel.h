#ifndef __EDA_DRAW_PANEL_H
#define __EDA_DRAW_PANEL_H

#include <wx/wx.h>
#include <base_struct.h>
#include <gr_basic.h>
#include <eda_rect.h>

/**
 * Mouse capture callback function prototype.
 */

class BASE_SCREEN;

class EDA_DRAW_PANEL;

typedef void ( *MOUSE_CAPTURE_CALLBACK )( EDA_DRAW_PANEL* aPanel, wxDC* aDC,
                                          const wxPoint& aPosition, bool aErase );

/**
 * End mouse capture callback function prototype.
 */
typedef void ( *END_MOUSE_CAPTURE_CALLBACK )( EDA_DRAW_PANEL* aPanel, wxDC* aDC );


class EDA_DRAW_PANEL
{
protected:
    bool    m_showCrossHair;                ///< Indicate if cross hair is to be shown.
    int     m_cursorLevel;                  ///< Index for cursor redraw in XOR mode.
    int     m_scrollIncrementX;             ///< X axis scroll increment in pixels per unit.
    int     m_scrollIncrementY;             ///< Y axis scroll increment in pixels per unit.

    wxPoint m_CursorStartPos;               ///< Used for testing the cursor movement.
    wxPoint m_PanStartCenter;               ///< Initial scroll center position when pan started
    wxPoint m_PanStartEventPosition;        ///< Initial position of mouse event when pan started

    /// The drawing area used to redraw the screen which is usually the visible area
    /// of the drawing in internal units.
    EDA_RECT    m_ClipBox;

    bool    m_abortRequest;                 ///< Flag used to abort long commands.

    bool    m_enableZoomNoCenter;           ///< True to enable zooming around the crosshair instead of the center
    bool    m_enableMousewheelPan;          ///< True to enable mousewheel panning by default.

    bool    m_enableAutoPan;                ///< True to enable automatic panning.

    bool    m_ignoreMouseEvents;            ///< Ignore mouse events when true.

    /* Used to inhibit a response to a mouse left button release, after a double click
     * (when releasing the left button at the end of the second click.  Used in Eeschema
     * to inhibit a mouse left release command when switching between hierarchical sheets
     * on a double click.
     */
    bool    m_ignoreNextLeftButtonRelease;  ///< Ignore the next mouse left button release when true.

    bool    m_enableBlockCommands;          ///< True enables block commands.

    /**
     * Count the drag events. Used to filter mouse moves before starting a
     * block command.  A block command can be started only if
     * MinDragEventCount > MIN_DRAG_COUNT_FOR_START_BLOCK_COMMAND in order to avoid
     * spurious block commands.
     */
    int     m_minDragEventCount;

    /// True when drawing in mirror mode. Used by the draw arc function, because arcs
    /// are oriented, and in mirror mode, orientations are reversed.
    bool    m_PrintIsMirrored;

    /// Mouse capture move callback function.
    MOUSE_CAPTURE_CALLBACK m_mouseCaptureCallback;

    /// Abort mouse capture callback function.
    END_MOUSE_CAPTURE_CALLBACK m_endMouseCaptureCallback;

    /// useful to avoid false start block in certain cases
    /// (like switch from a sheet to another sheet
    /// >= 0 (or >= n) if a block can start
    int     m_canStartBlock;

    int     m_doubleClickInterval;

public:

    EDA_DRAW_PANEL() :
        m_showCrossHair( true ),
        m_cursorLevel( 0 ),
        m_scrollIncrementX( 1 ),
        m_scrollIncrementY( 1 ),
        m_abortRequest( false ),
        m_enableZoomNoCenter( false ),
        m_enableMousewheelPan( false ),
        m_enableAutoPan( false ),
        m_ignoreMouseEvents( false ),
        m_ignoreNextLeftButtonRelease( false ),
        m_enableBlockCommands( true ),
        m_minDragEventCount( 5 ),
        m_PrintIsMirrored( false ),
        m_mouseCaptureCallback( nullptr ),
        m_endMouseCaptureCallback( nullptr ),
        m_canStartBlock( true ),
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

    bool GetAbortRequest() const { return m_abortRequest; }

    void SetAbortRequest( bool aAbortRequest ) { m_abortRequest = aAbortRequest; }

    bool GetEnableMousewheelPan() const { return m_enableMousewheelPan; }

    virtual void SetEnableMousewheelPan( bool aEnable ) { m_enableMousewheelPan = aEnable; };

    bool GetEnableZoomNoCenter() const { return m_enableZoomNoCenter; }

    virtual void SetEnableZoomNoCenter( bool aEnable ) { m_enableZoomNoCenter = aEnable; };

    bool GetEnableAutoPan() const { return m_enableAutoPan; }

    virtual void SetEnableAutoPan( bool aEnable ) { m_enableAutoPan = aEnable; };

    virtual void SetAutoPanRequest( bool aEnable ) = 0;

    void SetIgnoreMouseEvents( bool aIgnore ) { m_ignoreMouseEvents = aIgnore; }

    void SetIgnoreLeftButtonReleaseEvent( bool aIgnore ) { m_ignoreNextLeftButtonRelease = aIgnore; }

    void SetEnableBlockCommands( bool aEnable ) { m_enableBlockCommands = aEnable; }

    bool GetPrintMirrored() const               { return m_PrintIsMirrored; }
    void SetPrintMirrored( bool aMirror )       { m_PrintIsMirrored = aMirror; }

    void SetCanStartBlock( int aStartBlock ) { m_canStartBlock = aStartBlock; }

    /**
     * Function DrawBackGround
     * @param DC = current Device Context
     * Draws (if allowed) :
     * the grid
     * X and Y axis
     * X and Y auxiliary axis
     */
    virtual void DrawBackGround( wxDC* DC ) { printf("EDA_DRAW_PANEL:Unimplemented1\n"); };

    /**
     * Function DrawGrid
     * draws a grid to \a aDC.
     * @see m_ClipBox to determine the damaged area of the drawing to draw the grid.
     * @see EDA_DRAW_FRAME::IsGridVisible() to determine if grid is shown.
     * @see EDA_DRAW_FRAME::GetGridColor() for the color of the grid.
     * @param aDC The device context to draw the grid.
     */
    virtual void DrawGrid( wxDC* aDC ) { printf("EDA_DRAW_PANEL:Unimplemented2\n"); };

    /**
     * Function DrawAuxiliaryAxis
     * Draw the Auxiliary Axis, used in Pcbnew which as origin coordinates
     * for gerber and excellon files
     * @param aDC = current Device Context
     * @param aDrawMode = draw mode (GR_COPY, GR_OR ..)
     */
    virtual void DrawAuxiliaryAxis( wxDC* aDC, GR_DRAWMODE aDrawMode ) { printf("EDA_DRAW_PANEL:Unimplemented2\n");};

    /**
     * Function DrawGridAxis
     * Draw on auxiliary axis, used in Pcbnew to show grid origin, when
     * the grid origin is set by user, and is not (0,0)
     * @param aDC = current Device Context
     * @param aDrawMode = draw mode (GR_COPY, GR_OR ..)
     * @param aGridOrigin = the absolute coordinate of grid origin for snap.
     */
    virtual void DrawGridAxis( wxDC* aDC, GR_DRAWMODE aDrawMode, const wxPoint& aGridOrigin ) { printf("EDA_DRAW_PANEL:Unimplemented4\n");  };

        /**
     * Function DeviceToLogical
     * converts \a aRect from device to drawing (logical) coordinates.
     * <p>
     * \a aRect must be in scrolled device units.
     * </p>
     * @param aRect The rectangle to convert.
     * @param aDC The device context used for the conversion.
     * @return A rectangle converted to drawing units.
     */
    virtual wxRect DeviceToLogical( const wxRect& aRect, wxDC& aDC ) { printf("EDA_DRAW_PANEL:Unimplemented5\n");wxASSERT(false); return wxRect(); };

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

     virtual void EraseScreen( wxDC* DC ) { printf("EDA_DRAW_PANEL:Unimplemented6\n");  };;

    virtual void SetZoom( double mode ) { printf("EDA_DRAW_PANEL:Unimplemented7\n");  };;
    virtual double GetZoom() { printf("EDA_DRAW_PANEL:Unimplemented8\n"); return 1.0; };;

    //virtual void SetGrid( const wxRealPoint& size ) { printf("EDA_DRAW_PANEL:Unimplemented\n");  };;
    //virtual wxRealPoint GetGrid() { printf("EDA_DRAW_PANEL:Unimplemented\n"); return wxRealPoint(1.0, 1.0); };;


    /**
     * Function IsPointOnDisplay
     * @param aPosition The position to test in logical (drawing) units.
     * @return true if \a aPosition is visible on the screen.
     *         false if \a aPosition is not visible on the screen.
     */
    virtual bool IsPointOnDisplay( const wxPoint& aPosition ) { printf("EDA_DRAW_PANEL:Unimplemented9\n"); return false; };;

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

    virtual void ReDraw( wxDC* aDC, bool aEraseBackground = true ) { printf("EDA_DRAW_PANEL:Unimplemented11\n");  };;

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
    virtual wxPoint GetScreenCenterLogicalPosition() { printf("EDA_DRAW_PANEL:Unimplemented13\n"); return wxPoint(0, 0); };;

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

    /* Cursor functions */
    /**
     * Function DrawCrossHair
     * draws the user cross hair.
     * <p>
     * The user cross hair is not the mouse cursor although they may be at the same screen
     * position.  The mouse cursor is still render by the OS.  This is a drawn cross hair
     * that is used to snap to grid when grid snapping is enabled.  This is as an indicator
     * to where the next user action will take place.
     * </p>
     * @param aDC - the device context to draw the cursor
     * @param aColor - the color to draw the cursor
     */
    virtual void DrawCrossHair( wxDC* aDC=nullptr, COLOR4D aColor = COLOR4D::WHITE ) { printf("EDA_DRAW_PANEL:Unimplemented18\n"); };;

    // Hide the cross hair.
    virtual void CrossHairOff( wxDC* DC=nullptr ) { printf("EDA_DRAW_PANEL:Unimplemented19\n");  };;

    // Show the cross hair.
    virtual void CrossHairOn( wxDC* DC=nullptr ) { printf("EDA_DRAW_PANEL:Unimplemented20\n");  };;

    /**
     * Function SetMouseCapture
     * sets the mouse capture and end mouse capture callbacks to \a aMouseCaptureCallback
     * and \a aEndMouseCaptureCallback respectively.
     */
    virtual void SetMouseCapture( MOUSE_CAPTURE_CALLBACK aMouseCaptureCallback,
                          END_MOUSE_CAPTURE_CALLBACK aEndMouseCaptureCallback )
    {
        m_mouseCaptureCallback = aMouseCaptureCallback;
        m_endMouseCaptureCallback = aEndMouseCaptureCallback;
    }


    virtual void SetMouseCaptureCallback( MOUSE_CAPTURE_CALLBACK aMouseCaptureCallback )
    {
        m_mouseCaptureCallback = aMouseCaptureCallback;
    }


    /**
     * Function EndMouseCapture
     * ends mouse a capture.
     *
     * Check to see if the cursor is being managed for block or editing commands and release it.
     * @param aId The command ID to restore or -1 to keep the current command ID.
     * @param aCursorId The wxWidgets stock cursor ID to set the cursor to or -1 to keep the
     *                  current cursor.
     * @param aTitle The tool message to display in the status bar or wxEmptyString to clear
     *               the message.
     * @param aCallEndFunc Call the abort mouse capture callback if true.
     */
    virtual void EndMouseCapture( int aId = -1, int aCursorId = -1,
                          const wxString& aTitle = wxEmptyString,
                          bool aCallEndFunc = true ) { printf("EDA_DRAW_PANEL:Unimplemented21\n"); wxASSERT(false); };;

    inline bool IsMouseCaptured() const { return m_mouseCaptureCallback != NULL; }

    /**
     * Function CallMouseCapture
     * calls the mouse capture callback.
     *
     * @param aDC A point to a wxDC object to perform any drawing upon.
     * @param aPosition A referecnce to a wxPoint object containing the current cursor
     *                  position.
     * @param aErase True indicates the item being drawn should be erase before drawing
     *               it a \a aPosition.
     */
    virtual void CallMouseCapture( wxDC* aDC, const wxPoint& aPosition, bool aErase ) { printf("EDA_DRAW_PANEL:Unimplemented22\n"); wxASSERT(false); };;

    /**
     * Function CallEndMouseCapture
     * calls the end mouse capture callback.
     *
     * @param aDC A point to a wxDC object to perform any drawing upon.
     */
    virtual void CallEndMouseCapture( wxDC* aDC ) { printf("EDA_DRAW_PANEL:Unimplemented23\n"); wxASSERT(false); };;

    virtual void Refresh( bool eraseBackground = true, const wxRect* rect = NULL ) {}

    virtual wxWindow* GetWindow() = 0;
};

#endif
