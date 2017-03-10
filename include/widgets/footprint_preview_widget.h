/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2017 Chris Pavlina <pavlina.chris@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __FOOTPRINT_PREVIEW_WIDGET_H
#define __FOOTPRINT_PREVIEW_WIDGET_H

#include <wx/panel.h>
#include <functional>
#include <import_export.h>

class FOOTPRINT_LOAD_EVENT;
class FOOTPRINT_PREVIEW_PANEL_BASE;
class LIB_ID;
class KIWAY;
class wxStaticText;
class wxSizer;


enum FOOTPRINT_STATUS {
    FPS_NOT_FOUND = 0,
    FPS_READY = 1,
    FPS_LOADING = 2
};


class FOOTPRINT_PREVIEW_WIDGET: public wxPanel
{
public:

    /**
     * Construct a footprint preview widget.
     *
     * @param aParent - parent window
     * @param aKiway - an active Kiway instance
     */
    FOOTPRINT_PREVIEW_WIDGET( wxWindow* aParent, KIWAY& aKiway );

    /**
     * Return whether the widget initialized properly. This could return false
     * if Kiway is not available. If this returns false, no other methods should
     * be called.
     */
    bool IsInitialized() const { return !! m_prev_panel; }

    /**
     * Set the contents of the status label and display it.
     */
    void SetStatusText( wxString const& aText );

    /**
     * Clear the contents of the status label and hide it.
     */
    void ClearStatus();

    /**
     * Preload a footprint into the cache. This must be called prior to
     * DisplayFootprint, and may be called early.
     */
    void CacheFootprint( const LIB_ID& aFPID );

    /**
     * Set the currently displayed footprint. Any footprint passed in here
     * must have been passed to CacheFootprint before.
     */
    void DisplayFootprint( const LIB_ID& aFPID );

private:

    /**
     * Callback from the FOOTPRINT_PREVIEW_PANEL
     */
    void OnStatusChange( FOOTPRINT_STATUS aStatus );

    FOOTPRINT_PREVIEW_PANEL_BASE* m_prev_panel;
    wxStaticText*   m_status_label;
    wxSizer*        m_sizer;
};


typedef std::function<void( FOOTPRINT_STATUS )> FOOTPRINT_STATUS_HANDLER;


/**
 * Base class for the actual viewer panel. The implementation is in
 * pcbnew/footprint_preview_panel.cpp, accessed via kiface.
 */
class APIEXPORT FOOTPRINT_PREVIEW_PANEL_BASE
{
public:
    virtual ~FOOTPRINT_PREVIEW_PANEL_BASE() {}

    /**
     * Preload a footprint into the cache. This must be called prior to
     * DisplayFootprint, and may be called early.
     */
    virtual void CacheFootprint( LIB_ID const& aFPID ) = 0;

    /**
     * Set the currently displayed footprint. Any footprint passed in here
     * must have been passed to CacheFootprint before.
     */
    virtual void DisplayFootprint( LIB_ID const& aFPID ) = 0;

    /**
     * Set the callback to receive status updates.
     */
    virtual void SetStatusHandler( FOOTPRINT_STATUS_HANDLER aHandler ) = 0;

    /**
     * Get the underlying wxWindow.
     */
    virtual wxWindow* GetWindow() = 0;

    /**
     * Return a footprint preview panel instance via Kiface. May return null
     * if Kiway is not available or there is any error on load.
     */
    static FOOTPRINT_PREVIEW_PANEL_BASE* Create( wxWindow* aParent, KIWAY& aKiway );
};


#endif // __FOOTPRINT_PREVIEW_WIDGET_H
