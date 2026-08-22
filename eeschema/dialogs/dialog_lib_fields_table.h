/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <vector>

#include <dialogs/dialog_fields_table.h>
#include <lib_fields_data_model.h>

#include <wx/arrstr.h>

class LIB_SYMBOL;
class SYMBOL_EDIT_FRAME;


class DIALOG_LIB_FIELDS_TABLE : public DIALOG_FIELDS_TABLE
{
public:
    enum SCOPE : int
    {
        SCOPE_LIBRARY = 0,
        SCOPE_RELATED_SYMBOLS
    };

    DIALOG_LIB_FIELDS_TABLE( SYMBOL_EDIT_FRAME* aParent, SCOPE aScope );
    ~DIALOG_LIB_FIELDS_TABLE() override;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

private:
    wxGridCellEditor* createDatasheetEditor() override;
    wxGridCellEditor* createFootprintEditor() override;
    void              onBomSettingsChanged() override {}

    void setScope( SCOPE aScope );
    void loadSymbols( const wxArrayString& aSymbolNames );

    /**
     * Construct the rows of m_fieldsCtrl and the columns of m_dataModel from a union of all
     * field names in use.
     */
    void LoadFieldNames();

    void OnTableRangeSelected( wxGridRangeSelectEvent& aEvent ) override {}

    void OnScope( wxCommandEvent& aEvent ) override;
    void OnMenu( wxCommandEvent& aEvent ) override;

    void OnSaveAndContinue( wxCommandEvent& aEvent ) override;
    void OnCancel( wxCommandEvent& aEvent ) override;
    void OnOk( wxCommandEvent& aEvent ) override;
    void OnClose( wxCloseEvent& aEvent ) override;

    FIELDS_TABLE_DATA_MODEL_BASE* getDataModel() const override { return m_dataModel; }

private:
    std::vector<BOM_PRESET> getBuiltInBomPresets() const override;

    wxString resolveVariant() const override;
    bool     resolveTextVar( wxString* aToken ) const override;

private:
    SYMBOL_EDIT_FRAME*                 m_parent;
    std::vector<LIB_SYMBOL*>           m_symbolsList;
    LIB_FIELDS_EDITOR_GRID_DATA_MODEL* m_dataModel = nullptr;

    SCOPE m_symbolScope;
};
