/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2017-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <dialogs/dialog_edit_library_tables.h>
#include <panel_sym_lib_table_base.h>


class SYMBOL_LIB_TABLE;
class SYMBOL_LIB_TABLE_GRID;


/**
 * Dialog to show and edit symbol library tables.
 */
class PANEL_SYM_LIB_TABLE : public PANEL_SYM_LIB_TABLE_BASE
{

public:
    PANEL_SYM_LIB_TABLE( DIALOG_EDIT_LIBRARY_TABLES* aParent, PROJECT* m_project,
                         SYMBOL_LIB_TABLE* aGlobal, const wxString& aGlobalTablePath,
                         SYMBOL_LIB_TABLE* aProject, const wxString& aProjectTablePath );
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

    bool TransferDataFromWindow() override;

    /// Populate the readonly environment variable table with names and values
    /// by examining all the full_uri columns.
    void populateEnvironReadOnlyTable();

    bool convertLibrary( const wxString& aLibrary, const wxString& legacyFilepath,
                         const wxString& newFilepath );

    SYMBOL_LIB_TABLE_GRID* global_model() const;

    SYMBOL_LIB_TABLE_GRID* project_model() const;

    SYMBOL_LIB_TABLE_GRID* cur_model() const;

private:
    // Caller's tables are modified only on OK button and successful verification.
    SYMBOL_LIB_TABLE*           m_globalTable;
    SYMBOL_LIB_TABLE*           m_projectTable;
    PROJECT*                    m_project;

    DIALOG_EDIT_LIBRARY_TABLES* m_parent;

    WX_GRID*                    m_cur_grid;     ///< changed based on tab choice
    static size_t               m_pageNdx;      ///< Remember the last notebook page selected
    wxString                    m_lastProjectLibDir;    //< Transient (unsaved) last browsed folder when adding a project level library
};


void InvokeSchEditSymbolLibTable( KIWAY* aKiway, wxWindow *aParent );


#endif    // PANEL_SYM_LIB_TABLE_H
