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

#pragma once

#include <dialog_import_symbol_select_base.h>
#include <symbol_import_manager.h>
#include <sch_io/sch_io_mgr.h>
#include <sch_io/sch_io.h>

#include <map>
#include <vector>

class LIB_SYMBOL;
class SYMBOL_PREVIEW_WIDGET;
class SYMBOL_EDIT_FRAME;
class SYMBOL_LIBRARY_MANAGER;


/**
 * Dialog to select symbols for import from an external library file.
 */
class DIALOG_IMPORT_SYMBOL_SELECT : public DIALOG_IMPORT_SYMBOL_SELECT_BASE
{
public:
    /**
     * @param aParent Symbol editor frame
     * @param aFilePath Path to the library file to import from
     * @param aDestLibrary Name of the destination library
     * @param aPluginType The file type/plugin to use for reading
     */
    DIALOG_IMPORT_SYMBOL_SELECT( SYMBOL_EDIT_FRAME* aParent,
                                 const wxString& aFilePath,
                                 const wxString& aDestLibrary,
                                 SCH_IO_MGR::SCH_FILE_T aPluginType );

    ~DIALOG_IMPORT_SYMBOL_SELECT() override;

    /**
     * Get the list of symbols selected for import.
     * Includes both manually selected symbols and auto-selected dependencies.
     * @return Vector of symbol names to import
     */
    std::vector<wxString> GetSelectedSymbols() const;

    /**
     * Get the conflict resolutions chosen by the user.
     * @return Map of symbol name to resolution choice
     */
    const std::map<wxString, CONFLICT_RESOLUTION>& GetConflictResolutions() const
    {
        return m_conflictResolutions;
    }

protected:
    void OnFilterTextChanged( wxCommandEvent& event ) override;
    void OnSymbolSelected( wxDataViewEvent& event ) override;
    void OnSelectAll( wxCommandEvent& event ) override;
    void OnSelectNone( wxCommandEvent& event ) override;
    void OnUnitChanged( wxCommandEvent& event ) override;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

private:
    /**
     * Load symbols from the source file and populate the manager.
     * @return true on success
     */
    bool loadSymbols();

    /**
     * Refresh the list control based on current filter and selections.
     */
    void refreshList();

    /**
     * Update the preview for the currently selected symbol.
     */
    void updatePreview();

    /**
     * Update the status line with selection counts.
     */
    void updateStatusLine();

    /**
     * Update import button enabled state based on selection.
     */
    void updateImportButton();

    /**
     * Handle checkbox toggle in the list.
     */
    void onItemChecked( wxDataViewEvent& event );

    /**
     * Toggle selection state for a symbol.
     * Handles dependency auto-selection via the manager.
     * @param aSymbolName Name of the symbol to toggle
     * @param aChecked New checked state
     * @return true if change was made (may be blocked by confirmation dialog)
     */
    bool toggleSymbolSelection( const wxString& aSymbolName, bool aChecked );

    /**
     * Check if a symbol matches the current filter.
     */
    bool matchesFilter( const wxString& aSymbolName ) const;

    /**
     * Show conflict resolution dialog.
     * @return true if user wants to proceed, false to cancel
     */
    bool resolveConflicts();

private:
    SYMBOL_EDIT_FRAME*    m_frame;
    wxString              m_filePath;
    wxString              m_destLibrary;
    SCH_IO_MGR::SCH_FILE_T m_pluginType;

    /// Plugin kept alive for symbol access during dialog lifetime
    IO_RELEASER<SCH_IO>   m_plugin;

    SYMBOL_PREVIEW_WIDGET* m_preview;

    /// Manager for symbol selection logic
    SYMBOL_IMPORT_MANAGER m_manager;

    /// Map from symbol name to list index (UI-only, -1 if filtered out)
    std::map<wxString, int> m_listIndices;

    /// Current filter string
    wxString m_filterString;

    /// Currently selected symbol for preview
    wxString m_selectedSymbol;

    /// Conflict resolutions chosen by user
    std::map<wxString, CONFLICT_RESOLUTION> m_conflictResolutions;

    /// Column indices for the data view
    enum
    {
        COL_CHECKBOX = 0,
        COL_ICON,
        COL_NAME
    };
};
