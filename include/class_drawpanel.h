/******************************
 *  drawpanel.h:
 *  define class EDA_DRAW_PANEL
 *************************************/

#ifndef  PANEL_WXSTRUCT_H
#define  PANEL_WXSTRUCT_H

#include "colors.h"
#include "base_struct.h"
#include <wx/overlay.h>

class EDA_DRAW_FRAME;
class BASE_SCREEN;
class PCB_SCREEN;


/**
 * Mouse capture callback function prototype.
 */
typedef void ( *MOUSE_CAPTURE_CALLBACK )( EDA_DRAW_PANEL* aPanel, wxDC* aDC,
                                          const wxPoint& aPosition, bool aErase );

/**
 * End mouse capture callback function prototype.
 */
typedef void ( *END_MOUSE_CAPTURE_CALLBACK )( EDA_DRAW_PANEL* aPanel, wxDC* aDC );


class EDA_DRAW_PANEL : public wxScrolledWindow
{
private:
    int m_currentCursor;                 ///< Current mouse cursor shape id.
    int m_defaultCursor;          ///< The default mouse cursor shape id.
    bool m_showCrossHair;         ///< Indicate if cross hair is to be shown.
    int m_cursorLevel;            ///< Index for cursor redraw in XOR mode.

public:
    EDA_Rect m_ClipBox;           // the clipbox used in screen redraw (usually gives the
                                  // visible area in internal units)
    wxPoint m_CursorStartPos;     // useful in testing the cursor movement
    int m_scrollIncrementX;       // X axis scroll increment in pixels per unit.
    int m_scrollIncrementY;       // Y axis scroll increment in pixels per unit.

    bool m_AbortRequest;          // Flag to abort long commands
    bool m_AbortEnable;           // TRUE if abort button or menu to be displayed

    bool m_AutoPAN_Enable;        // TRUE to allow auto pan
    bool m_AutoPAN_Request;       // TRUE to request an auto pan (will be made only if
                                  // m_AutoPAN_Enable = true)
    int m_IgnoreMouseEvents;      // when non-zero (true), then ignore mouse events
    bool m_Block_Enable;          // TRUE to accept Block Commands

    // useful to avoid false start block in certain cases
    // (like switch from a sheet to an other sheet
    int m_CanStartBlock;          // >= 0 (or >= n) if a block can start
    bool m_PrintIsMirrored;       // True when drawing in mirror mode. Used in draw arc function,
                                  // because arcs are oriented, and in mirror mode, orientations
                                  // are reversed

#ifdef USE_WX_OVERLAY
    // MAC Uses overlay to workaround the wxINVERT and wxXOR miss
    wxOverlay m_overlay;
#endif

    /* Cursor management (used in editing functions) */

    /* Mouse capture move callback function. */
    MOUSE_CAPTURE_CALLBACK m_mouseCaptureCallback;

    /* Abort mouse capture callback function. */
    END_MOUSE_CAPTURE_CALLBACK m_endMouseCaptureCallback;

public:

    EDA_DRAW_PANEL( EDA_DRAW_FRAME* parent, int id, const wxPoint& pos, const wxSize& size );
    ~EDA_DRAW_PANEL();

    BASE_SCREEN* GetScreen();

    virtual EDA_DRAW_FRAME* GetParent();

    void OnPaint( wxPaintEvent& event );


    /**
     * Function DrawBackGround
     * @param DC = current Device Context
     * Draws (if allowed) :
     * the grid
     * X and Y axis
     * X and Y auxiliary axis
     */
    void DrawBackGround( wxDC* DC );

    /**
     * Function DrawGrid
     * draws a grid to \a aDC.
     * @see m_ClipBox to determine the damaged area of the drawing to draw the grid.
     * @see EDA_DRAW_FRAME::IsGridVisible() to determine if grid is shown.
     * @see EDA_DRAW_FRAME::GetGridColor() for the color of the grid.
     * @param aDC The device context to draw the grid.
     */
    void DrawGrid( wxDC* DC );

    /**
     * Function DrawAuxiliaryAxis
     * Draw the Auxiliary Axis, used in pcbnew which as origin coordinates
     * for gerber and excellon files
     * @param aDC = current Device Context
     * @param aDrawMode = draw mode (GR_COPY, GR_OR ..)
     */
    void DrawAuxiliaryAxis( wxDC* aDC, int aDrawMode );

    /**
     * Function DrawGridAxis
     * Draw on auxiliary axis, used in pcbnew to show grid origin, when
     * the grid origin is set by user, and is not (0,0)
     * @param aDC = current Device Context
     * @param aDrawMode = draw mode (GR_COPY, GR_OR ..)
     */
    void DrawGridAxis( wxDC* aDC, int aDrawMode );

    void OnEraseBackground( wxEraseEvent& event ) { }

    /**
     * Function OnActivate
     * handles window activation events.
     * <p>
     * The member m_CanStartBlock is initialize to avoid a block start command on activation
     * (because a left mouse button can be pressed and no block command wanted.  This happens
     * when enter on a hierarchy sheet on double click.
     *</p>
     */
    void OnActivate( wxActivateEvent& event );

    /**
     * Function DoPrepareDC
     * sets up the device context \a aDC for drawing.
     * <p>
     * This overrides wxScrolledWindow::DoPrepareDC() for setting up the the device context
     * used for drawing.   The scale factor and drawing logical offset are set and the base
     * method is called to set the DC device origin (scroll bar position).  This connects
     * everything together to achieve the appropriate coordinate manipulation using wxDC
     * LogicalToDeviceXXX and DeviceToLogixalXXX methods.  This gets called automatically
     * for a paint event.  If you do any drawing outside the paint event, you must call
     * DoPrepareDC manually.
     * </p>
     * @param aDC The device context to prepare.
     */
    virtual void DoPrepareDC( wxDC& aDC );

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
    wxRect DeviceToLogical( const wxRect& aRect, wxDC& aDC );

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
    void OnMouseWheel( wxMouseEvent& event );
    void OnMouseEvent( wxMouseEvent& event );
    void OnMouseLeaving( wxMouseEvent& event );
    void OnKeyEvent( wxKeyEvent& event );

    void OnPan( wxCommandEvent& event );

    void EraseScreen( wxDC* DC );
    void OnScrollWin( wxCommandEvent& event );
    void OnScroll( wxScrollWinEvent& event );

    void SetZoom( int mode );
    int GetZoom();
    void SetGrid( const wxRealPoint& size );
    wxRealPoint GetGrid();

    /**
     * Function OnRightClick
     * builds and displays a context menu on a right mouse button click.
     * @return true if the context menu is shown, or false
     */
    bool OnRightClick( wxMouseEvent& event );

    /**
     * Function IsPointOnDisplay
     * @param aPosition The position to test in logical (drawing) units.
     * @return true if \a aPosition is visible on the screen.
     *         false if \a aPosition is not visible on the screen.
     */
    bool IsPointOnDisplay( const wxPoint& aPosition );

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
    void SetClipBox( wxDC& aDC, const wxRect* aRect = NULL );

    void ReDraw( wxDC* aDC, bool aEraseBackground = true );

    /**
     * Function RefreshDrawingRect
     * redraws the contents of \a aRect in drawing units.  \a aRect is converted to
     * screen coordinates and wxWindow::RefreshRect() is called to repaint the region.
     * @param aRect The rectangle to repaint.
     * @param aEraseBackground Erases the background if true.
     */
    void RefreshDrawingRect( const EDA_Rect& aRect, bool aEraseBackground = true );

    /**
     * Function GetScreenCenterLogicalPosition
     * @return The current screen center position in logical (drawing) units.
     */
    wxPoint GetScreenCenterLogicalPosition();

    /**
     * Function MoveCursorToCrossHair
     * warps the cursor to the current cross hair position.
     */
    void MoveCursorToCrossHair();

    /**
     * Function MoveCursor
     * moves the mouse pointer to \a aPosition in logical (drawing) units.
     * @param aPosition The position in logical units to move the cursor.
     */
    void MoveCursor( const wxPoint& aPosition );

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
    void DrawCrossHair( wxDC* aDC, int aColor = WHITE );

    // Hide the cross hair.
    void CrossHairOff( wxDC* DC );

    // Show the cross hair.
    void CrossHairOn( wxDC* DC );

    /**
     * Function SetMouseCapture
     * sets the mouse capture and end mouse capture callbacks to \a aMouseCaptureCallback
     * and \a aEndMouseCaptureCallback respectively.
     */
    void SetMouseCapture( MOUSE_CAPTURE_CALLBACK aMouseCaptureCallback,
                          END_MOUSE_CAPTURE_CALLBACK aEndMouseCaptureCallback )
    {
        m_mouseCaptureCallback = aMouseCaptureCallback;
        m_endMouseCaptureCallback = aEndMouseCaptureCallback;
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
     */
    void EndMouseCapture( int aId = -1, int aCursorId = -1,
                          const wxString& aTitle = wxEmptyString );

    inline bool IsMouseCaptured() const { return m_mouseCaptureCallback != NULL; }

    /**
     * Function SetCurrentCursor
     * Set the current cursor shape for drawpanel
     */
    void SetCurrentCursor( int aCursor )
    {
        m_currentCursor = aCursor;
        SetCursor( m_currentCursor );
    }

    /**
     * Function GetDefaultCursor
     * return the default cursor shape
     */
    int GetDefaultCursor() const { return m_defaultCursor; }

    /**
     * Function GetCurrentCursor
     * return the current cursor shape, depending on the current selected tool
     */
    int GetCurrentCursor() const { return m_currentCursor; }


    DECLARE_EVENT_TABLE()
};


/**
 * Class EDA_CROSS_HAIR_MANAGER
 * is used to hide the cross hair and restore it when the class goes out of scope.
 */
class EDA_CROSS_HAIR_MANAGER
{
public:
    EDA_CROSS_HAIR_MANAGER( EDA_DRAW_PANEL* aPanel, wxDC* aDC ) :
        m_panel( aPanel ),
        m_dc( aDC )
    {
        if( aPanel && aDC )
            aPanel->CrossHairOff( aDC );
    }

    ~EDA_CROSS_HAIR_MANAGER()
    {
        if( m_panel && m_dc )
            m_panel->CrossHairOn( m_dc );
    }

private:
    EDA_DRAW_PANEL* m_panel;
    wxDC* m_dc;

    DECLARE_NO_COPY_CLASS( EDA_CROSS_HAIR_MANAGER )
};


#endif  /* #ifndef PANEL_WXSTRUCT_H */
