/******************************
 *  drawpanel.h:
 *  define class WinEDA_DrawPanel
 *************************************/

#ifndef  PANEL_WXSTRUCT_H
#define  PANEL_WXSTRUCT_H

#include "colors.h"
#include "base_struct.h"

class WinEDA_DrawFrame;
class BASE_SCREEN;
class PCB_SCREEN;


class WinEDA_DrawPanel : public wxScrolledWindow
{
private:
    WinEDA_DrawFrame* m_Parent;

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


    bool m_AutoPAN_Enable;                  // TRUE to allow auto pan
    bool m_AutoPAN_Request;                 // TRUE to request an auto pan
                                            // (will be made only if
                                            // m_AutoPAN_Enable = true)

    int  m_IgnoreMouseEvents;               //  when non-zero (true), then
                                            // ignore mouse events

    bool m_Block_Enable;                    // TRUE to accept Block Commands
    int  m_CanStartBlock;                   // >= 0 (or >= n) if a block can
                                            // start
    bool m_PrintIsMirrored;                 // True when drawing in mirror
                                            // mode. Used in draw arc function,
                                            // because arcs are oriented, and
                                            // in mirror mode, orientations are
                                            // reversed
    // useful to avoid false start block in certain cases (like switch from a
    // sheet to an other sheet
    int  m_PanelDefaultCursor;              // Current mouse cursor default
                                            // shape id for this window
    int  m_PanelCursor;                     // Current mouse cursor shape id
                                            // for this window
    int  m_CursorLevel;                     // Index for cursor redraw in XOR
                                            // mode

    /* Cursor management (used in editing functions) */

    /* Mouse capture move callback function prototype. */
    void (*ManageCurseur)( WinEDA_DrawPanel* panel, wxDC* DC, bool erase );

    /* Abort managed cursor callback function prototype. */
    void (*ForceCloseManageCurseur)( WinEDA_DrawPanel* panel, wxDC* DC );

public:

    WinEDA_DrawPanel( WinEDA_DrawFrame* parent, int id, const wxPoint& pos,
                      const wxSize& size );
    ~WinEDA_DrawPanel();

    BASE_SCREEN* GetScreen();

    WinEDA_DrawFrame* GetParent()
    {
        return m_Parent;
    }

    void         OnPaint( wxPaintEvent& event );
    void         OnSize( wxSizeEvent& event );

    /**  Function DrawBackGround
     * @param DC = current Device Context
     * Draws (if allowed) :
     * the grid
     * X and Y axis
     * X and Y auxiliary axis
     */
    void         DrawBackGround( wxDC* DC );

    /**  Function DrawGrid
     * @param DC = current Device Context
     * draws the grid
     *  - the grid is drawn only if the zoom level allows a good visibility
     *  - the grid is always centered on the screen center
     */
    void         DrawGrid( wxDC* DC );

    /** function DrawAuxiliaryAxis
     * Draw the Auxiliary Axis, used in pcbnew which as origin coordinates
     * for gerber and excellon files
     * @param DC = current Device Context
     */
    void         DrawAuxiliaryAxis( wxDC* DC, int drawmode );

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
     * connects everything together to acheive the appropiate coordinate
     * manipulation using wxDC LogicalToDeviceXXX and DeviceToLogixalXXX
     * methods.  This gets called automatically for a paint event.  If you do
     * any drawing outside the paint event, you must call DoPrepareDC manually.
     *
     * @param dc - The device context to prepare.
     */
    virtual void DoPrepareDC(wxDC& dc);

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
    void         SetBoundaryBox( wxDC* dc );
    void         ReDraw( wxDC* DC, bool erasebg = TRUE );

    /** Function CursorRealPosition
     * @return the position in user units of location ScreenPos
     * @param ScreenPos = the screen (in pixel) position to convert
     */
    wxPoint      CursorRealPosition( const wxPoint& ScreenPos );

    /** Function CursorScreenPosition
     * @return the curseur current position in pixels in the screen draw area
     */
    wxPoint      CursorScreenPosition();

    /**
     * Function PostDirtyRect
     * appends the given rectangle in pcb units to the DrawPanel's invalid
     * region list so that very soon (but not immediately), this rectangle
     * along with any other recently posted rectangles is redrawn.  Conversion
     * to pixels is done in here.
     * @param aRect The rectangle to append, it must be orthogonal
     *   (vertical and horizontal edges only), and it must be [,) in nature,
     *   i.e. [pos, dim) == [inclusive, exclusive)
     */
    void         PostDirtyRect( EDA_Rect aRect );

    /**
     * Function ConvertPcbUnitsToPixelsUnits
     * converts pos and size of the given EDA_Rect to pos and size in pixels,
     * relative to the current draw area (origin 0,0 is the left top visible
     * corner of draw area) according to the current scroll and zoom.
     * @param aRect = the rectangle to convert
     */
    void         ConvertPcbUnitsToPixelsUnits( EDA_Rect* aRect );

    /**
     * Function ConvertPcbUnitsToPixelsUnits
     * converts a given wxPoint position (in internal units) to units of
     * pixels, relative to the current draw area (origin 0,0 is the left
     * top visible
     * corner of draw area) according to the current scroll and zoom.
     * @param aPosition = the position to convert
     */
    void         ConvertPcbUnitsToPixelsUnits( wxPoint* aPosition );

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
