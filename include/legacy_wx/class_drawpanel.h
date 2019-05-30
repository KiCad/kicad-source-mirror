/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file class_drawpanel.h:
 * @brief EDA_DRAW_PANEL class definition.
 */

#ifndef  PANEL_WXSTRUCT_H
#define  PANEL_WXSTRUCT_H

#include <base_struct.h>
#include <gr_basic.h>
#include <eda_rect.h>


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
    int     m_currentCursor;                ///< Current mouse cursor shape id.
    int     m_defaultCursor;                ///< The default mouse cursor shape id.
    bool    m_showCrossHair;                ///< Indicate if cross hair is to be shown.
    int     m_cursorLevel;                  ///< Index for cursor redraw in XOR mode.
    int     m_scrollIncrementX;             ///< X axis scroll increment in pixels per unit.
    int     m_scrollIncrementY;             ///< Y axis scroll increment in pixels per unit.

    wxPoint m_CursorStartPos;               ///< Used for testing the cursor movement.
    wxPoint m_PanStartCenter;               ///< Initial scroll center position when pan started
    wxPoint m_PanStartEventPosition;        ///< Initial position of mouse event when pan started

    wxPoint m_CursorClickPos;               ///< Used for maintaining click position
    wxTimer *m_ClickTimer;

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

    /// useful to avoid false start block in certain cases
    /// (like switch from a sheet to another sheet
    /// >= 0 (or >= n) if a block can start
    int     m_canStartBlock;

    int     m_doubleClickInterval;

public:

    EDA_DRAW_PANEL( EDA_DRAW_FRAME* parent, int id, const wxPoint& pos, const wxSize& size );
    ~EDA_DRAW_PANEL();

    /**
     * Function GetDisplayOptions
     * A way to pass info to draw functions.
     * this is just an accessor to the GetDisplayOptions() parent frame function.
     */
    void* GetDisplayOptions();

    BASE_SCREEN* GetScreen();

    EDA_DRAW_FRAME* GetParent() const;

    void OnPaint( wxPaintEvent& event );

    EDA_RECT* GetClipBox() { return &m_ClipBox; }

    void SetClipBox( const EDA_RECT& aRect ) { m_ClipBox = aRect; }

    void SetIgnoreMouseEvents( bool aIgnore ) { m_ignoreMouseEvents = aIgnore; }

    void SetIgnoreLeftButtonReleaseEvent( bool aIgnore ) { m_ignoreNextLeftButtonRelease = aIgnore; }

    bool GetPrintMirrored() const               { return m_PrintIsMirrored; }
    void SetPrintMirrored( bool aMirror )       { m_PrintIsMirrored = aMirror; }

    void OnEraseBackground( wxEraseEvent& event ) { }

    /**
     * Function OnActivate
     * handles window activation events.
     * <p>
     * The member m_canStartBlock is initialize to avoid a block start command on activation
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
    virtual void DoPrepareDC( wxDC& aDC ) override;

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
#if wxCHECK_VERSION( 3, 1, 0 ) || defined( USE_OSX_MAGNIFY_EVENT )
    void OnMagnify( wxMouseEvent& event );
#endif
    void OnMouseEntering( wxMouseEvent& aEvent );
    void OnMouseLeaving( wxMouseEvent& event );
    void OnCharHook( wxKeyEvent& event );

    void OnPan( wxCommandEvent& event );

    void OnScrollWin( wxCommandEvent& event );
    void OnScroll( wxScrollWinEvent& event );

    void SetGrid( const wxRealPoint& size );
    wxRealPoint GetGrid();

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

    /**
     * Function RefreshDrawingRect
     * redraws the contents of \a aRect in drawing units.  \a aRect is converted to
     * screen coordinates and wxWindow::RefreshRect() is called to repaint the region.
     * @param aRect The rectangle to repaint.
     * @param aEraseBackground Erases the background if true.
     */
    void RefreshDrawingRect( const EDA_RECT& aRect, bool aEraseBackground = true );

    /// @copydoc wxWindow::Refresh()
    virtual void Refresh( bool eraseBackground = true, const wxRect* rect = NULL ) override;

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
     * Function ToDeviceXY
     * transforms logical to device coordinates
     */
    wxPoint ToDeviceXY( const wxPoint& pos );

    /**
     * Function ToLogicalXY
     * transforms device to logical coordinates
     */
    wxPoint ToLogicalXY( const wxPoint& pos );

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
    void DrawCrossHair( wxDC* aDC, COLOR4D aColor = COLOR4D::WHITE );

    // Hide the cross hair.
    void CrossHairOff( wxDC* DC );

    // Show the cross hair.
    void CrossHairOn( wxDC* DC );

    /**
     * Function SetCurrentCursor
     * Set the current cursor shape for drawpanel
     */
    void SetCurrentCursor( int aCursor )
    {
        m_currentCursor = aCursor;
        SetCursor( (wxStockCursor) m_currentCursor );
    }

    /**
     * Function GetDefaultCursor
     * @return the default cursor shape
     */
    int GetDefaultCursor() const { return m_defaultCursor; }

    /**
     * Function GetCurrentCursor
     * @return the current cursor shape, depending on the current selected tool
     */
    int GetCurrentCursor() const { return m_currentCursor; }


    DECLARE_EVENT_TABLE()
};


#endif  /* #ifndef PANEL_WXSTRUCT_H */
