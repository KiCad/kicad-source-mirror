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

#include <dialogs/dialog_edit_library_tables.h>
#include <dialogs/panel_design_block_lib_table_base.h>
#include <widgets/wx_grid.h>
#include <design_block_io.h>


class LIBRARY_TABLE;
class DESIGN_BLOCK_LIB_TABLE;
class DESIGN_BLOCK_LIB_TABLE_GRID_DATA_MODEL;
class PROJECT;


/**
 * Dialog to show and edit symbol library tables.
 */
class PANEL_DESIGN_BLOCK_LIB_TABLE : public PANEL_DESIGN_BLOCK_LIB_TABLE_BASE
{
public:
    PANEL_DESIGN_BLOCK_LIB_TABLE( DIALOG_EDIT_LIBRARY_TABLES* aParent, PROJECT* aProject );
    ~PANEL_DESIGN_BLOCK_LIB_TABLE() override;

    bool TransferDataFromWindow() override;

    void AddTable( LIBRARY_TABLE* table, const wxString& aTitle, bool aClosable );

private:
    /**
     * Trim important fields, removes blank row entries, and checks for duplicates.
     *
     * @return bool - true if tables are OK, else false.
     */
    bool verifyTables();

    void appendRowHandler( wxCommandEvent& event ) override;
    void browseLibrariesHandler( wxCommandEvent& event );
    void deleteRowHandler( wxCommandEvent& event ) override;
    void moveUpHandler( wxCommandEvent& event ) override;
    void moveDownHandler( wxCommandEvent& event ) override;
    void onMigrateLibraries( wxCommandEvent& event ) override;
    void onSizeGrid( wxSizeEvent& event ) override;

    void onNotebookPageCloseRequest( wxAuiNotebookEvent& aEvent );

    void adjustPathSubsGridColumns( int aWidth );

    /// Populate the readonly environment variable table with names and values
    /// by examining all the full_uri columns.
    void populateEnvironReadOnlyTable();
    void populatePluginList();

    DESIGN_BLOCK_LIB_TABLE_GRID_DATA_MODEL* get_model( int aPage ) const;
    DESIGN_BLOCK_LIB_TABLE_GRID_DATA_MODEL* cur_model() const { return get_model( m_notebook->GetSelection() ); }

    WX_GRID* get_grid( int aPage ) const;
    WX_GRID* cur_grid() const { return get_grid( m_notebook->GetSelection() ); }

private:
    PROJECT*                    m_project;
    DIALOG_EDIT_LIBRARY_TABLES* m_parent;
    wxArrayString               m_pluginChoices;

    wxString                    m_lastProjectLibDir;   //< Transient (unsaved) last browsed folder when adding a
                                                       // project level library.

    std::map<DESIGN_BLOCK_IO_MGR::DESIGN_BLOCK_FILE_T, IO_BASE::IO_FILE_DESC> m_supportedDesignBlockFiles;
};


void InvokeEditDesignBlockLibTable( KIWAY* aKiway, wxWindow *aParent );
