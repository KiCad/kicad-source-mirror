/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef FOOTPRINT_SELECT_WIDGET_H
#define FOOTPRINT_SELECT_WIDGET_H

#include <wx/arrstr.h>
#include <wx/panel.h>

class KIWAY;
class PROJECT;
class FOOTPRINT_CHOICE;
class wxWindow;
class EDA_DRAW_FRAME;

/**
 * This event is fired when a footprint is selected. The string data of the
 * event will contain the footprint name.
 */
wxDECLARE_EVENT( EVT_FOOTPRINT_SELECTED, wxCommandEvent );

class FOOTPRINT_SELECT_WIDGET : public wxPanel
{
public:
    /**
     * Construct a footprint selector widget.
     *
     * @param aFrame - parent frame for context
     * @param aParent - parent window
     * @param aMaxItems - maximum number of filter items to display
     */
    FOOTPRINT_SELECT_WIDGET( EDA_DRAW_FRAME* aFrame, wxWindow* aParent, int aMaxItems = 400 );

    virtual ~FOOTPRINT_SELECT_WIDGET()
    {
    }

    /**
     * Initialize the widget with a KIWAY for cross-module communication.
     *
     * @param aKiway - active kiway instance used to fetch filtered footprints from pcbnew
     * @param aProject - current project (unused, kept for API compatibility)
     */
    void Load( KIWAY& aKiway, PROJECT& aProject );

    /**
     * Clear all filters. Does not update the list.
     */
    void ClearFilters();

    /**
     * Filter by pin count. Does not update the list.
     */
    void FilterByPinCount( int aPinCount );

    /**
     * Filter by footprint filter list. Does not update the list.
     *
     * @param aFilters is a wxArrayString of strings used to filter the list of available
     * footprints (can be empty).
     * The final fp list is the list of footprint names matching at least one filtering string.
     * A filtering string is something like sm0402 or DIP*
     * @param aZeroFilters - if true, zero filters = zero footprints. If false, zero filters =
     *  not filtering.
     */
    void FilterByFootprintFilters( const wxArrayString& aFilters, bool aZeroFilters );

    /**
     * Set the default footprint for a part. This will be listed at the
     * top. May be an empty string.
     */
    void SetDefaultFootprint( const wxString& aFp );

    /**
     * Update the contents of the list by fetching filtered footprints from pcbnew.
     * The "default" footprint will be selected.
     *
     * @return true if the list was updated successfully
     */
    bool UpdateList();

    /**
     * Set current selection to the default footprint
     */
    void SelectDefault();

    /**
     * Enable or disable the control for input
     */
    virtual bool Enable( bool aEnable = true ) override;

private:
    FOOTPRINT_CHOICE* m_fp_sel_ctrl;
    wxSizer*          m_sizer;

    int               m_max_items;
    wxString          m_default_footprint;

    // Filter parameters
    int               m_pin_count;
    wxArrayString     m_filters;
    bool              m_zero_filter;

    KIWAY*            m_kiway;

    void OnComboBox( wxCommandEvent& aEvent );
};


#endif // FOOTPRINT_SELECT_WIDGET
