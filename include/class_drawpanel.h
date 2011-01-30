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


class EDA_DRAW_PANEL : public wxScrolledWindow
{
private:
    EDA_DRAW_FRAME* m_Parent;

public:
    EDA_Rect          m_ClipBox;            // the clipbox used in screen
                                            // redraw (usually gives the
                                            // visible area in internal units)
    wxPoint           m_CursorStartPos;     // useful in testing the cursor
                                            // movement

    int  m_scrollIncrementX;                // X axis scroll increment in pixels per unit.
    int  m_scrollIncrementY;                // Y axis scroll increment in pixels per unit.


    bool m_AbortRequest;                    // Flag to abort long commands
    bool m_AbortEnable;                     // TRUE if abort button or menu to
                                            // be displayed

    bool m_DisableEraseBG;                  // if true: do not allow background erasure
                                            // (used to reduce flicker in Gerbview )


    bool m_AutoPAN_Enable;                  // TRUE to allow auto pan
    bool m_AutoPAN_Request;                 // TRUE to request an auto pan
                                            // (will be made only if
                                            // m_AutoPAN_Enable = true)

    int  m_IgnoreMouseEvents;               //  when non-zero (true), then
                                            // ignore mouse events

    bool m_Block_Enable;                    // TRUE to accept Block Commands
    // useful to avoid false start block in certain cases
    // (like switch from a sheet to an other sheet
    int  m_CanStartBlock;                   // >= 0 (or >= n) if a block can
                                            // start
    bool m_PrintIsMirrored;                 // True when drawing in mirror
                                            // mode. Used in draw arc function,
                                            // because arcs are oriented, and
                                            // in mirror mode, orientations are
                                            // reversed
    int  m_PanelDefaultCursor;              // Current mouse cursor default
                                            // shape id for this window
    int  m_PanelCursor;                     // Current mouse cursor shape id
                                            // for this window
    int  m_CursorLevel;                     // Index for cursor redraw in XOR
                                            // mode

#ifdef USE_WX_OVERLAY
    // MAC Uses overlay to workaround the wxINVERT and wxXOR miss
    wxOverlay               m_overlay;
#endif
    /* Cursor management (used in editing functions) */

    /* Mouse capture move callback function prototype. */
    void (*ManageCurseur)( EDA_DRAW_PANEL* panel, wxDC* DC, bool erase );

    /* Abort managed cursor callback function prototype. */
    void (*ForceCloseManageCurseur)( EDA_DRAW_PANEL* panel, wxDC* DC );

public:

    EDA_DRAW_PANEL( EDA_DRAW_FRAME* parent, int id, const wxPoint& pos, const wxSize& size );
    ~EDA_DRAW_PANEL();

    BASE_SCREEN* GetScreen();

    EDA_DRAW_FRAME* GetParent()
    {
        return m_Parent;
    }

    void         OnPaint( wxPaintEvent& event );

    /**
     * Function DrawBackGround
     * @param DC = current Device Context
     * Draws (if allowed) :
     * the grid
     * X and Y axis
     * X and Y auxiliary axis
     */
    void         DrawBackGround( wxDC* DC );

    /**
     * Function DrawGrid
     * @param DC = current Device Context
     * draws the grid
     *  - the grid is drawn only if the zoom level allows a good visibility
     *  - the grid is always centered on the screen center
     */
    void         DrawGrid( wxDC* DC );

    /**
     * Function DrawAuxiliaryAxis
     * Draw the Auxiliary Axis, used in pcbnew which as origin coordinates
     * for gerber and excellon files
     * @param aDC = current Device Context
     * @param aDrawMode = draw mode (GR_COPY, GR_OR ..)
     */
    void         DrawAuxiliaryAxis( wxDC* aDC, int aDrawMode );

    /**
     * Function DrawGridAxis
     * Draw on auxiliary axis, used in pcbnew to show grid origin, when
     * the grid origin is set by user, and is not (0,0)
     * @param aDC = current Device Context
     * @param aDrawMode = draw mode (GR_COPY, GR_OR ..)
     */
    void         DrawGridAxis( wxDC* aDC, int aDrawMode );

    void         OnEraseBackground( wxEraseEvent& event ) { }

    void         OnActivate( wxActivateEvent& event );

    /**
     * Prepare the device context for drawing.
     *
     * This overrides wxScrolledWindow::DoPrepareDC() for drawing depending
     * on the render mode selected a build time.  If the old kicad coordinate
     * scaling code is used then this code doesn't do any thing other than
     * update the boundary box.  If wxDC coordinate manipulation is used, then
     * the scale factor and drawing logical offset is set.  Then the base
     * method is called to set the DC device origin and user scale.  This
     * connects everything together to achieve the appropriate coordinate
     * manipulation using wxDC LogicalToDeviceXXX and DeviceToLogixalXXX
     * methods.  This gets called automatically for a paint event.  If you do
     * any drawing outside the paint event, you must call DoPrepareDC manually.
     *
     * @param dc - The device context to prepare.
     */
    virtual void DoPrepareDC(wxDC& dc);

    /**
     * Function DeviceToLogical
     * converts \a aRect from device to drawing (logical) coordinates.
     * <p>
     * \a aRect must be in scrolled device units.
     * <\p>
     * @param aRect The rectangle to convert.
     * @param aDC The device context used for the conversion.
     * @return A rectangle converted to drawing units.
     */
    wxRect DeviceToLogical( const wxRect& aRect, wxDC& aDC );

    /* Mouse and keys events */
    void         OnMouseWheel( wxMouseEvent& event );
    void         OnMouseEvent( wxMouseEvent& event );
    void         OnMouseLeaving( wxMouseEvent& event );
    void         OnKeyEvent( wxKeyEvent& event );

    void         OnPan( wxCommandEvent& event );

    void         EraseScreen( wxDC* DC );
    void         OnScrollWin( wxCommandEvent& event );
    void         OnScroll( wxScrollWinEvent& event );

    void         SetZoom( int mode );
    int          GetZoom();
    void         SetGrid( const wxRealPoint& size );
    wxRealPoint  GetGrid();

    bool         OnRightClick( wxMouseEvent& event );
    void         Process_Special_Functions( wxCommandEvent& event );

    bool         IsPointOnDisplay( wxPoint ref_pos );

    /**
     * Function SetClipBox
     * sets the clip box in drawing (logical) units from \a aRect in device units.
     * <p>
     * If \a aRect is NULL, then the entire visible area of the screen is used as the clip
     * area.  The clip box is used when drawing to determine which objects are not visible
     * and do not need to be drawn.
     * </p>
     * @param aDc The device context use for drawing with the correct scale and
     *            offsets already configured.  See DoPrepareDC().
     * @param aRect The clip rectangle in device units or NULL for the entire visible area
     *              of the screen.
     */
    void         SetClipBox( wxDC& dc, const wxRect* aRect = NULL );

    void         ReDraw( wxDC* DC, bool erasebg = TRUE );

    /**
     * Function CursorRealPosition
     * @return the position in user units of location ScreenPos
     * @param aPosition = the screen (in pixel) position to convert
     */
    wxPoint      CursorRealPosition( const wxPoint& aPosition );

    /**
     * Function CursorScreenPosition
     * @return the cursor current position in pixels in the screen draw area
     */
    wxPoint      CursorScreenPosition();

    /**
     * Function RefreshDrawingRect
     * redraws the contents of \a aRect in drawing units.  \a aRect is converted to
     * screen coordinates and wxWindow::RefreshRect() is called to repaint the region.
     * @param aRect The rectangle to repaint.
     * @param aEraseBackground Erases the background if true.
     */
    void         RefreshDrawingRect( const EDA_Rect& aRect, bool aEraseBackground = true );

    wxPoint      GetScreenCenterRealPosition( void );
    void         MouseToCursorSchema();
    void         MouseTo( const wxPoint& Mouse );

    /* Cursor functions */
    /**
     * Draw the user cursor.
     *
     * The user cursor is not the mouse cursor although they may be at the
     * same screen position.  The mouse cursor is still render by the OS.
     * This is a drawn cursor that is used to snap to grid when grid snapping
     * is enabled.  This is required because OSX does not allow moving the
     * cursor programmatically.
     *
     * @param aDC - the device context to draw the cursor
     * @param aColor - the color to draw the cursor
     */
    void         DrawCursor( wxDC* aDC, int aColor = WHITE );

    // remove the grid cursor from the display
    void         CursorOff( wxDC* DC );
    // display the grid cursor
    void         CursorOn( wxDC* DC );

    /**
     * Release managed cursor.
     *
     * Check to see if the cursor is being managed for block or editing
     * commands and release it.
     */
    void         UnManageCursor( int id = -1, int cursor = -1,
                                 const wxString& title = wxEmptyString );

    DECLARE_EVENT_TABLE()
};

#endif  /* #ifndef PANEL_WXSTRUCT_H */
