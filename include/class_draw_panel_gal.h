/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2018 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#ifndef  PANELGAL_WXSTRUCT_H
#define  PANELGAL_WXSTRUCT_H

#include <wx/window.h>
#include <wx/timer.h>
#include <wx/grid.h> // needed for MSVC to see wxScrolledCanvas indirectly exported
#include <math/box2.h>
#include <math/vector2d.h>
#include <widgets/msgpanel.h>
#include <memory>
#include <mutex>

#include <gal/cursors.h>

class BOARD;
class EDA_DRAW_FRAME;
class TOOL_DISPATCHER;
class PROF_COUNTER;

namespace KIGFX
{
class GAL;
class VIEW;
class WX_VIEW_CONTROLS;
class VIEW_CONTROLS;
class PAINTER;
class GAL_DISPLAY_OPTIONS;
class VIEW_OVERLAY;
struct VC_SETTINGS;
}


class EDA_DRAW_PANEL_GAL : public wxScrolledCanvas
{
public:
    enum GAL_TYPE {
        GAL_TYPE_UNKNOWN = -1,  ///< not specified: a GAL engine must be set by the client
        GAL_TYPE_NONE = 0,      ///< GAL not used (the legacy wxDC engine is used)
        GAL_TYPE_OPENGL,        ///< OpenGL implementation
        GAL_TYPE_CAIRO,         ///< Cairo implementation
        GAL_TYPE_LAST           ///< Sentinel, do not use as a parameter
    };

#ifdef __WXMAC__
    // Cairo doesn't work on OSX so we really have no fallback available.
    static constexpr GAL_TYPE GAL_FALLBACK = GAL_TYPE_OPENGL;
#else
    static constexpr GAL_TYPE GAL_FALLBACK = GAL_TYPE_CAIRO;
#endif

    /**
     * Create a drawing panel that is contained inside \p aParentWindow.
     *
     * If \p aParentWindow is not an EDA frame, a search through all the parents
     * of the parent window will be done to find the frame.
     *
     * @param aParentWindow is the window immediately containing this panel
     */
    EDA_DRAW_PANEL_GAL( wxWindow* aParentWindow, wxWindowID aWindowId,
                        const wxPoint& aPosition, const wxSize& aSize,
                        KIGFX::GAL_DISPLAY_OPTIONS& aOptions,
                        GAL_TYPE aGalType = GAL_TYPE_OPENGL );
    ~EDA_DRAW_PANEL_GAL();

    void SetFocus() override;

    bool StatusPopupHasFocus()
    {
        return m_statusPopup && m_statusPopup->HasFocus();
    }

    void SetStatusPopup( wxWindow* aPopup )
    {
        m_statusPopup = aPopup;
    }

    /**
     * Switch method of rendering graphics.
     *
     * @param aGalType is a type of rendering engine that you want to use.
     */
    virtual bool SwitchBackend( GAL_TYPE aGalType );

    /**
     * Return the type of backend currently used by GAL canvas.
     */
    inline GAL_TYPE GetBackend() const { return m_backend; }

    /**
     * Return a pointer to the GAL instance used in the panel.
     *
     * @return The instance of GAL.
     */
    KIGFX::GAL* GetGAL() const { return m_gal; }

    /**
     * Return a pointer to the #VIEW instance used in the panel.
     *
     * @return The instance of #VIEW.
     */
    virtual KIGFX::VIEW* GetView() const { return m_view; }

    /**
     * Return a pointer to the #VIEW_CONTROLS instance used in the panel.
     *
     * @return The instance of #VIEW_CONTROLS.
     */
    KIGFX::VIEW_CONTROLS* GetViewControls() const
    {
        return (KIGFX::VIEW_CONTROLS*)( m_viewControls );
    }

    /// @copydoc wxWindow::Refresh()
    virtual void Refresh( bool aEraseBackground = true, const wxRect* aRect = nullptr ) override;

    /**
     * Force a redraw.
     */
    void ForceRefresh();

    /**
     * Make sure a refresh gets done on the next idle event if it hasn't already.
     */
    void RequestRefresh();

    /**
     * Set a dispatcher that processes events and forwards them to tools.
     *
     * #DRAW_PANEL_GAL does not take over the ownership. Passing NULL disconnects all event
     * handlers from the #DRAW_PANEL_GAL and parent frame.
     *
     * @param aEventDispatcher is the object that will be used for dispatching events.
     */
    void SetEventDispatcher( TOOL_DISPATCHER* aEventDispatcher );

    /**
     * Begin drawing if it was stopped previously.
     */
    void StartDrawing();

    /**
     * Prevent the GAL canvas from further drawing until it is recreated or #StartDrawing()
     * is called.
     */
    void StopDrawing();

    /**
     * Take care of display settings for the given layer to be displayed in high contrast mode.
     */
    virtual void SetHighContrastLayer( int aLayer );

    /**
     * Move the selected layer to the top, so it is displayed above all others.
     */
    virtual void SetTopLayer( int aLayer );

    virtual void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
    {
        wxASSERT( false );
    }

    /**
     * Returns parent #EDA_DRAW_FRAME, if available or NULL otherwise.
     */
    EDA_DRAW_FRAME* GetParentEDAFrame() const { return m_edaFrame; }

    bool IsDialogPreview() const { return m_parent != (wxWindow*) m_edaFrame; }

    /**
     * Called when the window is shown for the first time.
     */
    virtual void OnShow() {}

    /**
     * Set whether focus is taken on certain events (mouseover, keys, etc).
     *
     * This should be true (and is by default) for any primary canvas, but can be false to make
     * well behaved preview panes and the like.
     */
    void SetStealsFocus( bool aStealsFocus ) { m_stealsFocus = aStealsFocus; }

    /**
     * Set the current cursor shape for this panel.
     */
    void SetCurrentCursor( KICURSOR aCursor );

    /**
     * Return the bounding box of the view that should be used if model is not valid.
     * For example, the drawing sheet bounding box for an empty PCB
     *
     * @return the default bounding box for the panel.
     */
    virtual BOX2I GetDefaultViewBBox() const { return BOX2I(); }

    /**
     * Used to forward events to the canvas from popups, etc.
     */
    void OnEvent( wxEvent& aEvent );

    /**
     * Repaint the canvas, and fix scrollbar cursors
     *
     * Usually called by a OnPaint event.
     *
     * Because it does not use a wxPaintDC, it can be called outside a wxPaintEvent.
     *
     * @return true if the repaint attempt was successful.
     */
    bool DoRePaint();

    /**
     * Create an overlay for rendering debug graphics.
     */
    std::shared_ptr<KIGFX::VIEW_OVERLAY> DebugOverlay();


    /**
     * Clear the contents of the debug overlay and removes it from the VIEW.
     */
    void ClearDebugOverlay();


    /**
     * Gets a populated View Controls settings object dervived from our program settings
     */
    static KIGFX::VC_SETTINGS GetVcSettings();

    /**
     * used on wxMSW: true after a wxEVT_MOUSE_CAPTURE_LOST was received
     * false after the mouse is recaptured.
     * Used to avoid calling twice a CaptureMouse(), not accepted by wxMSW
     */
    bool m_MouseCapturedLost;

    std::unique_ptr<PROF_COUNTER> m_PaintEventCounter;

protected:
    virtual void onPaint( wxPaintEvent& WXUNUSED( aEvent ) );
    void onSize( wxSizeEvent& aEvent );
    void onEnter( wxMouseEvent& aEvent );
    void onLostFocus( wxFocusEvent& aEvent );
    void onIdle( wxIdleEvent& aEvent );
    void onRefreshTimer( wxTimerEvent& aEvent );
    void onShowEvent( wxShowEvent& aEvent );

    wxWindow*                m_parent;           ///< Pointer to the parent window
    EDA_DRAW_FRAME*          m_edaFrame;         ///< Parent EDA_DRAW_FRAME (if available)

    wxLongLong               m_lastRepaintStart; ///< Timestamp of the last repaint start
    wxLongLong               m_lastRepaintEnd;   ///< Timestamp of the last repaint end
    wxTimer                  m_refreshTimer;     ///< Timer to prevent too-frequent refreshing

    std::mutex               m_refreshMutex;     ///< Blocks multiple calls to the draw

    /// True if GAL is currently redrawing the view
    bool                     m_drawing;

    /// Flag that determines if VIEW may use GAL for redrawing the screen.
    bool                     m_drawingEnabled;

    /// True when canvas needs to be refreshed from idle handler
    bool                     m_needIdleRefresh;

    /// Interface for drawing objects on a 2D-surface
    KIGFX::GAL*              m_gal;

    /// Stores view settings (scale, center, etc.) and items to be drawn
    KIGFX::VIEW*             m_view;

    /// Contains information about how to draw items using GAL
    std::unique_ptr<KIGFX::PAINTER> m_painter;

    /// Control for VIEW (moving, zooming, etc.)
    KIGFX::WX_VIEW_CONTROLS* m_viewControls;

    /// Currently used GAL
    GAL_TYPE                    m_backend;
    KIGFX::GAL_DISPLAY_OPTIONS& m_options;

    /// Processes and forwards events to tools
    TOOL_DISPATCHER*         m_eventDispatcher;

    /// Flag to indicate that focus should be regained on the next mouse event. It is a workaround
    /// for cases when the panel loses keyboard focus, so it does not react to hotkeys anymore.
    bool                     m_lostFocus;

    /// Flag to indicate whether the panel should take focus at certain times (when moused over,
    /// and on various mouse/key events)
    bool                     m_stealsFocus;

    wxWindow*                m_statusPopup;

    /// Optional overlay for drawing transient debug objects
    std::shared_ptr<KIGFX::VIEW_OVERLAY> m_debugOverlay;
};

#endif
