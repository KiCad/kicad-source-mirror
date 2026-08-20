/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Oliver Walters
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

#include <set>

#include <dialogs/dialog_fields_table.h>
#include <fields_view_controls_grid_data_model.h>
#include <sch_reference_list.h>
#include <schematic.h>
#include <symbol_fields_data_model.h>

wxDECLARE_EVENT( EDA_EVT_CLOSE_DIALOG_SYMBOL_FIELDS_TABLE, wxCommandEvent );

class SCHEMATIC_SETTINGS;
class SCH_EDIT_FRAME;
class JOB_EXPORT_BOM;


class DIALOG_SYMBOL_FIELDS_TABLE : public DIALOG_FIELDS_TABLE, public SCHEMATIC_LISTENER
{
public:
    DIALOG_SYMBOL_FIELDS_TABLE( SCH_EDIT_FRAME* parent, JOB_EXPORT_BOM* aJob = nullptr );
    ~DIALOG_SYMBOL_FIELDS_TABLE() override;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

private:
    wxGridCellEditor* createDatasheetEditor() override;

    void setScope( SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL::SCOPE aScope );
    void updateSelectionItems();

    /**
     * Construct the rows of m_fieldsCtrl and the columns of m_dataModel from a union of all
     * field names in use.
     */
    void LoadFieldNames();

    void OnTableRangeSelected( wxGridRangeSelectEvent& aEvent ) override;

    void OnScope( wxCommandEvent& event ) override;
    void OnMenu( wxCommandEvent& event ) override;

    void OnSaveAndContinue( wxCommandEvent& aEvent ) override;
    void OnCancel( wxCommandEvent& aEvent ) override;
    void OnOk( wxCommandEvent& aEvent ) override;
    void OnClose( wxCloseEvent& aEvent ) override;

    // Schematic listener event handlers
    void OnSchItemsAdded( SCHEMATIC& aSch, std::vector<SCH_ITEM*>& aSchItem ) override;
    void OnSchItemsRemoved( SCHEMATIC& aSch, std::vector<SCH_ITEM*>& aSchItem ) override;
    void OnSchItemsChanged( SCHEMATIC& aSch, std::vector<SCH_ITEM*>& aSchItem ) override;
    void OnSchSheetChanged( SCHEMATIC& aSch ) override;
    void OnSchSelectionChanged( SCHEMATIC& aSch ) override;

    /**
     * Saves the current grid selection as a set of symbol full paths for later restoration.
     */
    std::set<wxString> SaveGridSelection();

    /**
     * Restores the grid selection from a previously saved set of symbol full paths.
     */
    void RestoreGridSelection( const std::set<wxString>& aFullPaths );

    void rebuildRowsPreservingSelection();
    void rebuildRowsPreservingSelection( const std::set<wxString>& aSavedSelection );

    FIELDS_TABLE_DATA_MODEL_BASE* getDataModel() const override { return m_dataModel; }

private:
    SCH_REFERENCE_LIST getSymbolReferences( SCH_SYMBOL* aSymbol, SCH_REFERENCE_LIST& aCachedRefs );
    SCH_REFERENCE_LIST getSheetSymbolReferences( SCH_SHEET& aSheet );

    void onAddVariant( wxCommandEvent& aEvent ) override;
    void onDeleteVariant( wxCommandEvent& aEvent ) override;
    void onRenameVariant( wxCommandEvent& aEvent ) override;
    void onCopyVariant( wxCommandEvent& aEvent ) override;
    void onEditVariantDescription( wxCommandEvent& aEvent ) override;
    void onVariantSelectionChange( wxCommandEvent& aEvent ) override;

    void updateVariantButtonStates();

    wxString resolveVariant() const override;
    bool     resolveTextVar( wxString* aToken ) const override;

private:
    SCH_EDIT_FRAME*                    m_parent;

    SCH_REFERENCE_LIST                 m_symbolsList;
    SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL* m_dataModel = nullptr;

    SCHEMATIC_SETTINGS&                m_schSettings;

    bool m_aborted = false;

public:
    bool WasAborted() const { return m_aborted; }
};
