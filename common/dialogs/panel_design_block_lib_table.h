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

#ifndef PANEL_DESIGN_BLOCK_LIB_TABLE_H
#define PANEL_DESIGN_BLOCK_LIB_TABLE_H

#include <dialogs/dialog_edit_library_tables.h>
#include <dialogs/panel_design_block_lib_table_base.h>
#include <widgets/wx_grid.h>
#include <design_block_io.h>

class DESIGN_BLOCK_LIB_TABLE;
class DESIGN_BLOCK_LIB_TABLE_GRID;
class PROJECT;


/**
 * Dialog to show and edit symbol library tables.
 */
class PANEL_DESIGN_BLOCK_LIB_TABLE : public PANEL_DESIGN_BLOCK_LIB_TABLE_BASE
{
public:
    PANEL_DESIGN_BLOCK_LIB_TABLE( DIALOG_EDIT_LIBRARY_TABLES* aParent, PROJECT* aProject );
    ~PANEL_DESIGN_BLOCK_LIB_TABLE() override;

private:
    bool TransferDataFromWindow() override;

    /**
     * Trim important fields, removes blank row entries, and checks for duplicates.
     *
     * @return bool - true if tables are OK, else false.
     */
    bool verifyTables();

    void OnUpdateUI( wxUpdateUIEvent& event ) override;
    void appendRowHandler( wxCommandEvent& event ) override;
    void browseLibrariesHandler( wxCommandEvent& event );
    void deleteRowHandler( wxCommandEvent& event ) override;
    void moveUpHandler( wxCommandEvent& event ) override;
    void moveDownHandler( wxCommandEvent& event ) override;
    void onMigrateLibraries( wxCommandEvent& event ) override;
    void onSizeGrid( wxSizeEvent& event ) override;

    void adjustPathSubsGridColumns( int aWidth );

    /// Populate the readonly environment variable table with names and values
    /// by examining all the full_uri columns.
    void populateEnvironReadOnlyTable();
    void populatePluginList();

    DESIGN_BLOCK_LIB_TABLE_GRID* global_model() const
    {
        return (DESIGN_BLOCK_LIB_TABLE_GRID*) m_global_grid->GetTable();
    }

    DESIGN_BLOCK_LIB_TABLE_GRID* project_model() const
    {
        return m_project_grid ? (DESIGN_BLOCK_LIB_TABLE_GRID*) m_project_grid->GetTable() : nullptr;
    }

    DESIGN_BLOCK_LIB_TABLE_GRID* cur_model() const
    {
        return (DESIGN_BLOCK_LIB_TABLE_GRID*) m_cur_grid->GetTable();
    }

    PROJECT* m_project;

    DIALOG_EDIT_LIBRARY_TABLES* m_parent;

    WX_GRID*      m_cur_grid; // changed based on tab choice
    static size_t m_pageNdx;  // Remember last notebook page selected during a session

    //< Transient (unsaved) last browsed folder when adding a project level library.
    wxString m_lastProjectLibDir;

    std::map<DESIGN_BLOCK_IO_MGR::DESIGN_BLOCK_FILE_T, IO_BASE::IO_FILE_DESC>
            m_supportedDesignBlockFiles;
};


void InvokeEditDesignBlockLibTable( KIWAY* aKiway, wxWindow *aParent );

#endif // PANEL_DESIGN_BLOCK_LIB_TABLE_H
