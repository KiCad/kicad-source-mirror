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

#ifndef  PANEL_WXSTRUCT_H
#define  PANEL_WXSTRUCT_H

#include <base_struct.h>
#include <gr_basic.h>
#include <eda_rect.h>


class BASE_SCREEN;
class PCB_SCREEN;


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

    /// The drawing area used to redraw the screen which is usually the visible area
    /// of the drawing in internal units.
    EDA_RECT    m_ClipBox;

    /* Used to inhibit a response to a mouse left button release, after a double click
     * (when releasing the left button at the end of the second click.  Used in Eeschema
     * to inhibit a mouse left release command when switching between hierarchical sheets
     * on a double click.
     */
    bool    m_ignoreNextLeftButtonRelease;  ///< Ignore the next mouse left button release when true.

    /// True when drawing in mirror mode. Used by the draw arc function, because arcs
    /// are oriented, and in mirror mode, orientations are reversed.
    bool    m_PrintIsMirrored;

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

    EDA_RECT* GetClipBox() { return &m_ClipBox; }

    void SetClipBox( const EDA_RECT& aRect ) { m_ClipBox = aRect; }

    bool GetPrintMirrored() const               { return m_PrintIsMirrored; }
    void SetPrintMirrored( bool aMirror )       { m_PrintIsMirrored = aMirror; }

    void OnEraseBackground( wxEraseEvent& event ) { }

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

    /* Mouse and keys events */

    void OnCharHook( wxKeyEvent& event );

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

    /// @copydoc wxWindow::Refresh()
    virtual void Refresh( bool eraseBackground = true, const wxRect* rect = NULL ) override;

    /**
     * Function GetScreenCenterLogicalPosition
     * @return The current screen center position in logical (drawing) units.
     */
    wxPoint GetScreenCenterLogicalPosition();

    /* Cursor functions */

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
};


#endif  /* #ifndef PANEL_WXSTRUCT_H */
