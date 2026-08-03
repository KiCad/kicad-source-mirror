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
#include <fields_data_model.h>

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

    void ShowHideColumn( int aCol, bool aShow );

private:
    void SetupColumnProperties( int aCol );
    void SetupAllColumnProperties();
    void AddField( const wxString& displayName, const wxString& aCanonicalName, bool show,
                   bool groupBy, bool addedByUser = false );
    void setScope( FIELDS_EDITOR_GRID_DATA_MODEL::SCOPE aScope );

    /**
     * Construct the rows of m_fieldsCtrl and the columns of m_dataModel from a union of all
     * field names in use.
     */
    void LoadFieldNames();

    void OnViewControlsCellChanged( wxGridEvent& aEvent ) override;
    void OnAddField( wxCommandEvent& event ) override;
    void OnRemoveField( wxCommandEvent& event ) override;
    void OnRenameField( wxCommandEvent& event ) override;

    void OnColSort( wxGridEvent& aEvent );
    void OnColMove( wxGridEvent& aEvent );
    void OnTableRangeSelected( wxGridRangeSelectEvent& aEvent );

    void OnFilterText( wxCommandEvent& aEvent ) override;
    void OnScope( wxCommandEvent& event ) override;
    void OnGroupSymbolsToggled( wxCommandEvent& event ) override;
    void OnRegroupSymbols( wxCommandEvent& aEvent ) override;
    void OnMenu( wxCommandEvent& event ) override;

    void OnTableCellClick( wxGridEvent& event ) override;
    void OnGridMouseMove( wxMouseEvent& aEvent );

    void OnSidebarToggle( wxCommandEvent& event ) override;
    void OnExport( wxCommandEvent& aEvent ) override;
    void OnSaveAndContinue( wxCommandEvent& aEvent ) override;
    void OnCancel( wxCommandEvent& aEvent ) override;
    void OnOk( wxCommandEvent& aEvent ) override;
    void OnClose( wxCloseEvent& aEvent ) override;

    void OnOutputFileBrowseClicked( wxCommandEvent& event ) override;
    void OnPageChanged( wxNotebookEvent& event ) override;
    void OnPreviewRefresh( wxCommandEvent& event ) override;
    void PreviewRefresh();


    // Schematic listener event handlers
    void OnSchItemsAdded( SCHEMATIC& aSch, std::vector<SCH_ITEM*>& aSchItem ) override;
    void OnSchItemsRemoved( SCHEMATIC& aSch, std::vector<SCH_ITEM*>& aSchItem ) override;
    void OnSchItemsChanged( SCHEMATIC& aSch, std::vector<SCH_ITEM*>& aSchItem ) override;
    void OnSchSheetChanged( SCHEMATIC& aSch ) override;

    void EnableSelectionEvents();
    void DisableSelectionEvents();

    /**
     * Saves the current grid selection as a set of symbol full paths for later restoration.
     */
    std::set<wxString> SaveGridSelection();

    /**
     * Restores the grid selection from a previously saved set of symbol full paths.
     */
    void RestoreGridSelection( const std::set<wxString>& aFullPaths );

private:
    SCH_REFERENCE_LIST getSymbolReferences( SCH_SYMBOL* aSymbol, SCH_REFERENCE_LIST& aCachedRefs );
    SCH_REFERENCE_LIST getSheetSymbolReferences( SCH_SHEET& aSheet );

    void doApplyBomPreset( const BOM_PRESET& aPreset ) override;
    void doApplyBomFmtPreset( const BOM_FMT_PRESET& aPreset ) override;
    BOM_PRESET getDataModelBomPreset() override;
    void savePresetsToSchematic();

    void onAddVariant( wxCommandEvent& aEvent ) override;
    void onDeleteVariant( wxCommandEvent& aEvent ) override;
    void onRenameVariant( wxCommandEvent& aEvent ) override;
    void onCopyVariant( wxCommandEvent& aEvent ) override;
    void onEditVariantDescription( wxCommandEvent& aEvent ) override;
    void onVariantSelectionChange( wxCommandEvent& aEvent ) override;

    void updateVariantButtonStates();

    wxString getSelectedVariant() const;

    wxString resolveVariant() const;

private:
    SCH_EDIT_FRAME*                    m_parent;

    // Index in the fields list control for each MANDATORY_FIELD type
    std::map<FIELD_T, int>             m_mandatoryFieldListIndexes;

    VIEW_CONTROLS_GRID_DATA_MODEL*     m_viewControlsDataModel = nullptr;

    SCH_REFERENCE_LIST                 m_symbolsList;
    FIELDS_EDITOR_GRID_DATA_MODEL*     m_dataModel = nullptr;

    SCHEMATIC_SETTINGS&                m_schSettings;

    JOB_EXPORT_BOM* m_job;

    bool m_aborted = false;

public:
    bool WasAborted() const { return m_aborted; }
};
