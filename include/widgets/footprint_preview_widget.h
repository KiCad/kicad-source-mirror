/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef FOOTPRINT_PREVIEW_WIDGET_H
#define FOOTPRINT_PREVIEW_WIDGET_H

#include <wx/panel.h>
#include <functional>
#include <import_export.h>
#include <lib_id.h>

#include <gal/color4d.h>

class FOOTPRINT_LOAD_EVENT;
class FOOTPRINT_PREVIEW_PANEL_BASE;
class KIWAY;
class wxStaticText;
class wxSizer;


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
    bool IsInitialized() const { return m_prev_panel != nullptr; }

    /**
     * Set the contents of the status label and display it.
     */
    void SetStatusText( const wxString& aText );

    /**
     * Clear the contents of the status label and hide it.
     */
    void ClearStatus();

    /**
     * Set the currently displayed footprint. Any footprint passed in here
     * must have been passed to CacheFootprint before.
     */
    void DisplayFootprint( const LIB_ID& aFPID );

private:
    FOOTPRINT_PREVIEW_PANEL_BASE* m_prev_panel;

    wxStaticText* m_status;
    wxPanel*      m_statusPanel;
    wxSizer*      m_statusSizer;
    wxSizer*      m_outerSizer;
    LIB_ID        m_libid;

};


/**
 * Base class for the actual viewer panel. The implementation is in
 * pcbnew/footprint_preview_panel.cpp, accessed via kiface.
 */
class APIEXPORT FOOTPRINT_PREVIEW_PANEL_BASE
{
public:
    virtual ~FOOTPRINT_PREVIEW_PANEL_BASE() {}

    /**
     * Set the currently displayed footprint. Any footprint passed in here
     * must have been passed to CacheFootprint before.
     */
    virtual bool DisplayFootprint( LIB_ID const& aFPID ) = 0;

    /**
     * Get the underlying wxWindow.
     */
    virtual wxWindow* GetWindow() = 0;

    /**
     * Get the colors to use in a preview widget to match the preview panel.
     */
    virtual const KIGFX::COLOR4D& GetBackgroundColor() = 0;
    virtual const KIGFX::COLOR4D& GetForegroundColor() = 0;

    /**
     * Return a footprint preview panel instance via Kiface. May return null
     * if Kiway is not available or there is any error on load.
     */
    static FOOTPRINT_PREVIEW_PANEL_BASE* Create( wxWindow* aParent, KIWAY& aKiway );
};


#endif // FOOTPRINT_PREVIEW_WIDGET_H
