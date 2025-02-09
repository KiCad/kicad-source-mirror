/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Wayne Stambaugh <stambaughw@gmail.com>
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

#ifndef PANEL_SYM_LIB_TABLE_H
#define PANEL_SYM_LIB_TABLE_H

#include <grid_tricks.h>
#include <dialogs/dialog_edit_library_tables.h>
#include <panel_sym_lib_table_base.h>
#include <lib_table_grid.h>

class SYMBOL_LIB_TABLE;
class SYMBOL_LIB_TABLE_GRID;

/**
 * Dialog to show and edit symbol library tables.
 */
class PANEL_SYM_LIB_TABLE : public PANEL_SYM_LIB_TABLE_BASE
{

public:
    PANEL_SYM_LIB_TABLE( DIALOG_EDIT_LIBRARY_TABLES* aParent, PROJECT* m_project );
    virtual ~PANEL_SYM_LIB_TABLE();

private:
    /**
     * Trim important fields, removes blank row entries, and checks for duplicates.
     *
     * @return bool - true if tables are OK, else false.
     */
    bool verifyTables();

    void OnUpdateUI( wxUpdateUIEvent& event ) override;
    void browseLibrariesHandler( wxCommandEvent& event ) override;
    void appendRowHandler( wxCommandEvent& event ) override;
    void deleteRowHandler( wxCommandEvent& event ) override;
    void moveUpHandler( wxCommandEvent& event ) override;
    void moveDownHandler( wxCommandEvent& event ) override;
    void onSizeGrid( wxSizeEvent& event ) override;
    void adjustPathSubsGridColumns( int aWidth );
    void onConvertLegacyLibraries( wxCommandEvent& event ) override;

    void onPageChange( wxBookCtrlEvent& event ) override;
    void onReset( wxCommandEvent& event ) override;

    void setupGrid( WX_GRID* aGrid );

    bool TransferDataFromWindow() override;

    /// Populate the readonly environment variable table with names and values
    /// by examining all the full_uri columns.
    void populateEnvironReadOnlyTable();

    SYMBOL_LIB_TABLE_GRID* global_model() const;

    SYMBOL_LIB_TABLE_GRID* project_model() const;

    SYMBOL_LIB_TABLE_GRID* cur_model() const;

    /**
     * @return true if the plugin type can be selected from the library path only
     * (i.e. only from its extension)
     * if the type needs an access to the file itself, return false because
     * the file can be not (at least temporary) available
     */
    bool allowAutomaticPluginTypeSelection( wxString& aLibraryPath );

    PROJECT*                    m_project;

    DIALOG_EDIT_LIBRARY_TABLES* m_parent;
    wxArrayString               m_pluginChoices;

    WX_GRID*                    m_cur_grid;     ///< changed based on tab choice
    static size_t               m_pageNdx;      ///< Remember the last notebook page selected

    /// Transient (unsaved) last browsed folder when adding a project level library.
    wxString                    m_lastProjectLibDir;
};


void InvokeSchEditSymbolLibTable( KIWAY* aKiway, wxWindow *aParent );


#endif    // PANEL_SYM_LIB_TABLE_H
