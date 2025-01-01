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

#include <footprint_filter.h>
#include <footprint_info.h>
#include <vector>
#include <wx/panel.h>

class KIWAY;
class PROJECT;
class FOOTPRINT_CHOICE;
class wxWindow;

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
     * This requires references to an external footprint loader, and an external
     * unique_ptr-to-FOOTPRINT_LIST. The latter will be populated with a
     * FOOTPRINT_LIST instance the first time Load() is called.
     *
     * The reason for this is that footprint loading tends to be very expensive,
     * especially when using online libraries. The caller is expected to keep
     * these objects around (e.g. they may be statics on the dialog this
     * FOOTPRINT_SELECT_WIDGET is created in) so footprints do not have to be
     * loaded more than once.
     *
     * @param aParent - parent window
     * @param aFpList - FOOTPRINT_LIST container
     * @param aUpdate - whether to call UpdateList() automatically when finished loading
     * @param aMaxItems - maximum number of filter items to display, in addition to
     *  Default and Other
     */
    FOOTPRINT_SELECT_WIDGET( EDA_DRAW_FRAME* aFrame, wxWindow* aParent, FOOTPRINT_LIST* aFpList,
                             bool aUpdate = true, int aMaxItems = 400 );

    virtual ~FOOTPRINT_SELECT_WIDGET()
    {
    }

    /**
     * Start loading. This function returns immediately; footprints will
     * continue to load in the background.
     *
     * @param aKiway - active kiway instance. This is cached for use when "Other"
     *      is selected.
     * @param aProject - current project
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
     * Update the contents of the list to match the filters. Has no effect if
     * the footprint list has not been loaded yet. The "default" footprint will be
     * selected.
     *
     * @return true if the footprint list has been loaded (and the list was updated)
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
    FOOTPRINT_CHOICE*        m_fp_sel_ctrl;
    wxSizer*                 m_sizer;

    bool                     m_update;
    int                      m_max_items;
    wxString                 m_default_footprint;

    FOOTPRINT_LIST*          m_fp_list;
    FOOTPRINT_FILTER         m_fp_filter;
    bool                     m_zero_filter;
    EDA_DRAW_FRAME*          m_frame;

    void OnComboBox( wxCommandEvent& aEvent );
};


#endif // FOOTPRINT_SELECT_WIDGET
