/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Wayne Stambaugh <stambaughw@gmail.com>
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

#include <dialog_change_symbols_base.h>

#include <lib_id.h>
#include <sch_sheet_path.h>
#include <template_fieldnames.h>


class SCH_SYMBOL;
class SCH_EDIT_FRAME;
class SCH_SCREEN;
class SCH_COMMIT;


struct SYMBOL_CHANGE_INFO
{
    std::vector<SCH_SHEET_PATH> m_Instances;
    LIB_ID                      m_LibId;
};


/**
 * Dialog to update or change schematic library symbols.
 */
class DIALOG_CHANGE_SYMBOLS : public DIALOG_CHANGE_SYMBOLS_BASE
{
public:
    enum class MODE { CHANGE = 0, UPDATE };

    DIALOG_CHANGE_SYMBOLS( SCH_EDIT_FRAME* aParent, SCH_SYMBOL* aSymbol, MODE aMode = MODE::UPDATE );
    ~DIALOG_CHANGE_SYMBOLS() override;

    bool TransferDataToWindow() override;

protected:
    void launchMatchIdSymbolBrowser( wxCommandEvent& aEvent ) override;
    void launchNewIdSymbolBrowser( wxCommandEvent& aEvent ) override;
    void onMatchTextKillFocus( wxFocusEvent& event ) override;
    void onMatchIDKillFocus( wxFocusEvent& event ) override;
    void onNewLibIDKillFocus( wxFocusEvent& event ) override;
    void onOkButtonClicked( wxCommandEvent& aEvent ) override;
    void onMatchByAll( wxCommandEvent& aEvent ) override;
    void onMatchBySelected( wxCommandEvent& aEvent ) override;
    void onMatchByReference( wxCommandEvent& aEvent ) override;
    void onMatchByValue( wxCommandEvent& aEvent ) override;
    void onMatchById( wxCommandEvent& aEvent ) override;

    void onSelectAll( wxCommandEvent& event ) override
    {
        selectAll( true );
    }

    void onSelectNone( wxCommandEvent& event ) override
    {
        selectAll( false );
    }

    void onCheckAll( wxCommandEvent& aEvent ) override
    {
        checkAll( true );
    }

    void onUncheckAll( wxCommandEvent& aEvent ) override
    {
        checkAll( false );
    }

    /// Select or deselect all fields in the listbox widget.
    void selectAll( bool aSelect );
    void checkAll( bool aCheck );

    void updateFieldsList();

    bool isMatch( SCH_SYMBOL* aSymbol, SCH_SHEET_PATH* aInstance );
    int processMatchingSymbols( SCH_COMMIT* aCommit );
    int processSymbols( SCH_COMMIT* aCommit, const std::map<SCH_SYMBOL*, SYMBOL_CHANGE_INFO>& aSymbols );
    wxString getSymbolReferences( SCH_SYMBOL& aSymbol, const LIB_ID& aNewId,
                                  const wxString* aOldLibLinkName = nullptr );

private:
    SCH_SYMBOL* m_symbol;
    MODE        m_mode;

    /// Set of field names that should have values updated.
    std::set<wxString>       m_updateFields;

    /// Index in the list control for each mandatory FIELD_T type
    std::map<FIELD_T, int>   m_mandatoryFieldListIndexes;
};
