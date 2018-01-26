/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2017 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef _DIALOG_SYM_LIB_TABLE_H_
#define _DIALOG_SYM_LIB_TABLE_H_

#include <dialog_sym_lib_table_base.h>

class SYMBOL_LIB_TABLE;
class SYMBOL_LIB_TABLE_GRID;


/**
 * Dialog to show and edit symbol library tables.
 */
class DIALOG_SYMBOL_LIB_TABLE : public DIALOG_SYMBOL_LIB_TABLE_BASE
{

public:
    DIALOG_SYMBOL_LIB_TABLE( wxTopLevelWindow* aParent, SYMBOL_LIB_TABLE* aGlobal,
                             SYMBOL_LIB_TABLE* aProject );
    virtual ~DIALOG_SYMBOL_LIB_TABLE();


private:
    /// If the cursor is not on a valid cell, because there are no rows at all, return -1,
    /// else return a 0 based column index.
    int getCursorCol() const;

    /// If the cursor is not on a valid cell, because there are no rows at all, return -1,
    /// else return a 0 based row index.
    int getCursorRow() const;

    /**
     * Trim important fields, removes blank row entries, and checks for duplicates.
     *
     * @return bool - true if tables are OK, else false.
     */
    bool verifyTables();

    void pageChangedHandler( wxAuiNotebookEvent& event ) override;

    void browseLibrariesHandler( wxCommandEvent& event ) override;

    void appendRowHandler( wxCommandEvent& event ) override;

    void deleteRowHandler( wxCommandEvent& event ) override;

    void moveUpHandler( wxCommandEvent& event ) override;

    void moveDownHandler( wxCommandEvent& event ) override;

    bool TransferDataFromWindow() override;

    /// Populate the readonly environment variable table with names and values
    /// by examining all the full_uri columns.
    void populateEnvironReadOnlyTable();

    /// Makes a specific row visible
    void scrollToRow( int aRowNumber );

    // Caller's tables are modified only on OK button and successful verification.
    SYMBOL_LIB_TABLE*    m_global;
    SYMBOL_LIB_TABLE*    m_project;

    SYMBOL_LIB_TABLE_GRID* global_model() const;

    SYMBOL_LIB_TABLE_GRID* project_model() const;

    SYMBOL_LIB_TABLE_GRID* cur_model() const;

    wxGrid*          m_cur_grid;     ///< changed based on tab choice
    static int       m_pageNdx;      ///< Remember the last notebook page selected during a session

    wxString         m_lastBrowseDir; ///< last browsed directory
};

#endif    // _DIALOG_SYM_LIB_TABLE_H_
