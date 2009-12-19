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
public:
    WinEDA_DrawFrame* m_Parent;
    EDA_Rect          m_ClipBox;            // the clipbox used in screen
                                            // redraw (usually gives the
                                            // visible area in internal units)
    wxPoint           m_CursorStartPos;     // useful in testing the cursor
                                            // movement

    int  m_ScrollButt_unit;                 // scroll bar pixels per unit value


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


    bool         IsPointOnDisplay( wxPoint ref_pos );
    void         OnPaint( wxPaintEvent& event );
    void         OnSize( wxSizeEvent& event );
    void         SetBoundaryBox();
    void         ReDraw( wxDC* DC, bool erasebg = TRUE );
    void         PrintPage( wxDC* DC,
                            bool  Print_Sheet_Ref,
                            int   PrintMask,
                            bool  aPrintMirrorMode );
    void         DrawBackGround( wxDC* DC );
    void         DrawAuxiliaryAxis( wxDC* DC, int drawmode );
    void         OnEraseBackground( wxEraseEvent& event );
    void         OnActivate( wxActivateEvent& event );

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

    void         AddMenuZoom( wxMenu* MasterMenu );
    bool         OnRightClick( wxMouseEvent& event );
    void         Process_Special_Functions( wxCommandEvent& event );

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
    void         Trace_Curseur( wxDC* DC, int color = WHITE );  // Draw the
                                                                // user cursor
                                                                // (grid
                                                                // cursor)
    void         CursorOff( wxDC* DC );                         // remove the
                                                                // grid cursor
                                                                // from the
                                                                // display
    void         CursorOn( wxDC* DC );                          // display the
                                                                // grid cursor

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

// Helper class to handle the client Device Context
class KicadGraphicContext : public wxClientDC
{
public:
    KicadGraphicContext( WinEDA_DrawPanel * aDrawPanel );
    ~KicadGraphicContext();
};

#endif  /* #ifndef PANEL_WXSTRUCT_H */
