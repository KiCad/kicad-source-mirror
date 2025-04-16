/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <eda_units.h>
#include <gal/color4d.h>

class FOOTPRINT_PREVIEW_PANEL_BASE;
class BOARD;
class FOOTPRINT;
class KIWAY;
class TOOL_DISPATCHER;
class EDA_DRAW_PANEL_GAL;
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
     * Return whether the widget initialized properly. This could return false if Kiway is
     * not available. If this returns false, no other methods should be called.
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
     * Set the units for the preview.
     */
    void SetUserUnits( EDA_UNITS aUnits );

    /**
     * Set the pin functions from the symbol's netlist.  This allows us to display them in
     * the corresponding pads.
     * @param aPinFunctions a map from pin_number to pin_function
     */
    void SetPinFunctions( const std::map<wxString, wxString>& aPinFunctions );

    /**
     * Set the currently displayed footprint. Any footprint passed in here *MUST* have been
     * passed to CacheFootprint before.
     */
    void DisplayFootprint( const LIB_ID& aFPID );

    /**
     * Display a pair of footprints.  (Normally used for diff'ing.)
     */
    void DisplayFootprints( std::shared_ptr<FOOTPRINT> aFootprintA,
                            std::shared_ptr<FOOTPRINT> aFootprintB );

    /**
     * Force the redrawing of all contents.
     */
    void RefreshAll();

    FOOTPRINT_PREVIEW_PANEL_BASE* GetPreviewPanel() { return m_prev_panel; }

protected:
    FOOTPRINT_PREVIEW_PANEL_BASE* m_prev_panel;

    wxStaticText*                 m_status;
    wxPanel*                      m_statusPanel;
    wxSizer*                      m_statusSizer;
    wxSizer*                      m_outerSizer;
    LIB_ID                        m_libid;
};


/**
 * Base class for the actual viewer panel. The implementation is in
 * pcbnew/footprint_preview_panel.cpp, accessed via kiface.
 */
class APIEXPORT FOOTPRINT_PREVIEW_PANEL_BASE
{
public:
    virtual ~FOOTPRINT_PREVIEW_PANEL_BASE() {}

    virtual void SetUserUnits( EDA_UNITS aUnits ) = 0;

    /**
     * Set the pin functions from the symbol's netlist.  This allows us to display them in
     * the corresponding pads.
     * @param aPinFunctions a map from pin_number to pin_function
     */
    virtual void SetPinFunctions( const std::map<wxString, wxString>& aPinFunctions ) = 0;

    /**
     * Set the currently displayed footprint. Any footprint passed in here *MUST* have been
     * passed to CacheFootprint before.
     */
    virtual bool DisplayFootprint( LIB_ID const& aFPID ) = 0;

    /**
     * Display a pair of footprints.  (Normally used for diff'ing.)
     */
    virtual void DisplayFootprints( std::shared_ptr<FOOTPRINT> aFootprintA,
                                    std::shared_ptr<FOOTPRINT> aFootprintB ) = 0;

    /**
     * Force the redrawing of all contents.
     */
    virtual void RefreshAll() = 0;

    /**
     * Get the GAL canvas.
     */
    virtual EDA_DRAW_PANEL_GAL* GetCanvas() = 0;

    virtual BOARD* GetBoard() = 0;

    /**
     * Get the colors to use in a preview widget to match the preview panel.
     */
    virtual const KIGFX::COLOR4D& GetBackgroundColor() const = 0;
    virtual const KIGFX::COLOR4D& GetForegroundColor() const = 0;

    /**
     * Return a footprint preview panel instance via Kiface. May return null if Kiway is not
     * available or there is any error on load.
     */
    static FOOTPRINT_PREVIEW_PANEL_BASE* Create( wxWindow* aParent, KIWAY& aKiway );
};


#endif // FOOTPRINT_PREVIEW_WIDGET_H
