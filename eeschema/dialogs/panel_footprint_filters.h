/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <functional>
#include <memory>
#include <vector>

#include <wx/arrstr.h>
#include <wx/timer.h>

#include <dialog_lib_symbol_properties_base.h>

class KIWAY;
class FP_FILTER_MATCHER;
class LISTBOX_TRICKS;


/**
 * The footprint filters/matches panel of the symbol properties dialog.
 */
class PANEL_FOOTPRINT_FILTERS : public PANEL_FOOTPRINT_FILTERS_BASE
{
public:
    PANEL_FOOTPRINT_FILTERS( wxWindow* aParent, KIWAY& aKiway );

    ~PANEL_FOOTPRINT_FILTERS() override;

    /// Replace the filters in the list with \a aFilters.
    void SetFilters( const wxArrayString& aFilters );

    /// @return the filters in the list.
    wxArrayString GetFilters() const;


    using FOOTPRINT_ASSIGN_QUERY = std::function<bool()>;
    using FOOTPRINT_ASSIGNER = std::function<void( const wxString& )>;

    /**
     * Injection for interfaces that reach outside the panel to interact with
     * host dialog
     *
     * @param aCanAssign reports whether the FP field can be written at all
     * @param aAssign pushes a footprint name into the field.
     */
    void SetFootprintFieldAccessors( FOOTPRINT_ASSIGN_QUERY aCanAssign, FOOTPRINT_ASSIGNER aAssign );

protected:
    void OnFpFilterDClick( wxMouseEvent& aEvent ) override;
    void OnEditFootprintFilter( wxCommandEvent& aEvent ) override;
    void OnAddFootprintFilter( wxCommandEvent& aEvent ) override;

private:
    /// Mark the host dialog (the top level window) as modified.
    void onModify();

    /// Refresh the matches list for the current filters, or for the filter being typed in
    /// the Add/Edit dialog.
    void updateMatchingFootprints();

    void showFootprintMatches( const std::vector<wxString>& aPatterns );

    /**
     * Return the "library:footprint" name of the given item in the matches list, or an
     * empty string when the item is not a footprint name.
     */
    wxString matchingFootprintName( long aItem ) const;

    void openFootprintInEditor( const wxString& aFootprintName );
    void copyFootprintName( const wxString& aFootprintName );

    /// Return true once the footprint libraries can be queried.  While they load, show the
    /// loading indication, keep polling and return false.
    bool matchLibrariesReady();

    /**
     * Show or hide the footprint filter match list's library load indicator.
     *
     * @param aShow Whether to show the load indicator.
     * @param aProgress The progress of the library loading, between 0.0 and 1.0.
     */
    void showFpFilterMatchLoadIndication( bool aShow, float aProgress = 0.0f );
    void onFpFilterMatchLoadTimer( wxTimerEvent& aEvent );
    void onMatchingFootprintDoubleClick( wxListEvent& aEvent );
    void onMatchingFootprintContextMenu( wxListEvent& aEvent );
    void onMatchingFootprintKeyDown( wxKeyEvent& aEvent );

    /// Query the matches when the page is selected; the libraries are not loaded before
    /// that, and native notebooks do not reliably report page visibility with show events.
    void onNotebookPageChanged( wxBookCtrlEvent& aEvent );

    KIWAY& m_kiway;

    std::unique_ptr<LISTBOX_TRICKS> m_fpFilterTricks;

    /// Supplies the matches for the footprint filter patterns.
    std::unique_ptr<FP_FILTER_MATCHER> m_fpMatcher;

    /// The sanitised filter text being typed in the Add/Edit filter dialog. If it is
    /// set, the match list previews this instead of the filters in the list pane.
    wxString m_fpFilterPreview;

    /// Access to panel-external interfaces
    FOOTPRINT_ASSIGN_QUERY m_canAssignFootprint;
    FOOTPRINT_ASSIGNER     m_assignFootprint;

    bool    m_matchLibrariesReady = false;
    wxTimer m_matchLoadTimer;
};
