/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2026 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <memory>
#include <vector>

#include <dialogs/dialog_fields_table.h>
#include <footprint_fields_data_model.h>

class FOOTPRINT_EDIT_FRAME;


class DIALOG_LIB_FOOTPRINT_FIELDS_TABLE : public DIALOG_FIELDS_TABLE
{
public:
    DIALOG_LIB_FOOTPRINT_FIELDS_TABLE( FOOTPRINT_EDIT_FRAME*                          aParent,
                                       FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::SCOPE aScope );
    ~DIALOG_LIB_FOOTPRINT_FIELDS_TABLE() override;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

private:
    void loadFootprints();

    wxGridCellEditor* createDatasheetEditor() override;

    // Don't mark the frame modified for lib tables, we're not modifying the library,
    // just the application settings for the library editor.
    void onBomSettingsChanged() override {}

    void setScope( FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::SCOPE aScope );

    /**
     * Construct the rows of m_fieldsCtrl and the columns of m_dataModel from a union of all
     * field names in use.
     */
    void LoadFieldNames();

    void OnTableSelectionChanged( const std::set<int>& aRows ) override {}

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
    FOOTPRINT_EDIT_FRAME*                        m_parent;
    std::vector<std::unique_ptr<FOOTPRINT>>      m_loadedFootprints;
    std::vector<FOOTPRINT_REF>                   m_footprintsList;
    LIB_FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL* m_dataModel = nullptr;
};
