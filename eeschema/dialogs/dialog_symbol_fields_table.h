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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#pragma once

#include <dialog_symbol_fields_table_base.h>
#include <sch_reference_list.h>
#include <schematic.h>
#include <fields_data_model.h>

wxDECLARE_EVENT( EDA_EVT_CLOSE_DIALOG_SYMBOL_FIELDS_TABLE, wxCommandEvent );

class SCHEMATIC_SETTINGS;
class SCH_EDIT_FRAME;
class JOB_EXPORT_SCH_BOM;


class DIALOG_SYMBOL_FIELDS_TABLE : public DIALOG_SYMBOL_FIELDS_TABLE_BASE, public SCHEMATIC_LISTENER
{
public:
    DIALOG_SYMBOL_FIELDS_TABLE( SCH_EDIT_FRAME* parent, JOB_EXPORT_SCH_BOM* aJob = nullptr );
    ~DIALOG_SYMBOL_FIELDS_TABLE() override;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void ShowEditTab();
    void ShowExportTab();
    void ShowHideColumn( int aCol, bool aShow );

private:
    void SetupColumnProperties( int aCol );
    void SetupAllColumnProperties();
    void AddField( const wxString& displayName, const wxString& aCanonicalName, bool show,
                   bool groupBy, bool addedByUser = false );
    void setScope( FIELDS_EDITOR_GRID_DATA_MODEL::SCOPE aScope );
    // Set bitmap and tooltip according to left panel visibility
    void setSideBarButtonLook( bool aIsLeftPanelCollapsed );

    /**
     * Construct the rows of m_fieldsCtrl and the columns of m_dataModel from a union of all
     * field names in use.
     */
    void LoadFieldNames();

    void OnViewControlsCellChanged( wxGridEvent& aEvent ) override;
    void OnSizeViewControlsGrid( wxSizeEvent& event ) override;
    void OnAddField( wxCommandEvent& event ) override;
    void OnRemoveField( wxCommandEvent& event ) override;
    void OnRenameField( wxCommandEvent& event ) override;

    void OnColSort( wxGridEvent& aEvent );
    void OnColMove( wxGridEvent& aEvent );
    void OnTableRangeSelected( wxGridRangeSelectEvent& aEvent );

    void OnFilterText( wxCommandEvent& aEvent ) override;
    void OnFilterMouseMoved( wxMouseEvent& event ) override;
    void OnScope( wxCommandEvent& event ) override;
    void OnGroupSymbolsToggled( wxCommandEvent& event ) override;
    void OnRegroupSymbols( wxCommandEvent& aEvent ) override;
    void OnMenu( wxCommandEvent& event ) override;

    void OnTableValueChanged( wxGridEvent& event ) override;
    void OnTableCellClick( wxGridEvent& event ) override;
    void OnTableColSize( wxGridSizeEvent& event ) override;

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

    std::vector<BOM_PRESET> GetUserBomPresets() const;
    void                    SetUserBomPresets( std::vector<BOM_PRESET>& aPresetList );
    void                    ApplyBomPreset( const wxString& aPresetName );
    void                    ApplyBomPreset( const BOM_PRESET& aPreset );

    /// Returns a formatting configuration corresponding to the values in the UI controls
    /// of the dialog.
    BOM_FMT_PRESET              GetCurrentBomFmtSettings();
    std::vector<BOM_FMT_PRESET> GetUserBomFmtPresets() const;
    void                        SetUserBomFmtPresets( std::vector<BOM_FMT_PRESET>& aPresetList );
    void                        ApplyBomFmtPreset( const wxString& aPresetName );
    void                        ApplyBomFmtPreset( const BOM_FMT_PRESET& aPreset );

    // Schematic listener event handlers
    void OnSchItemsAdded( SCHEMATIC& aSch, std::vector<SCH_ITEM*>& aSchItem ) override;
    void OnSchItemsRemoved( SCHEMATIC& aSch, std::vector<SCH_ITEM*>& aSchItem ) override;
    void OnSchItemsChanged( SCHEMATIC& aSch, std::vector<SCH_ITEM*>& aSchItem ) override;
    void OnSchSheetChanged( SCHEMATIC& aSch ) override;

    void EnableSelectionEvents();
    void DisableSelectionEvents();

private:
    SCH_REFERENCE_LIST getSymbolReferences( SCH_SYMBOL* aSymbol, SCH_REFERENCE_LIST& aCachedRefs );
    SCH_REFERENCE_LIST getSheetSymbolReferences( SCH_SHEET& aSheet );

    void syncBomPresetSelection();
    void rebuildBomPresetsWidget();
    void updateBomPresetSelection( const wxString& aName );
    void onBomPresetChanged( wxCommandEvent& aEvent );
    void doApplyBomPreset( const BOM_PRESET& aPreset );
    void loadDefaultBomPresets();

    void syncBomFmtPresetSelection();
    void rebuildBomFmtPresetsWidget();
    void updateBomFmtPresetSelection( const wxString& aName );
    void onBomFmtPresetChanged( wxCommandEvent& aEvent );
    void doApplyBomFmtPreset( const BOM_FMT_PRESET& aPreset );
    void loadDefaultBomFmtPresets();

    void savePresetsToSchematic();

    void onAddVariant( wxCommandEvent& aEvent ) override;
    void onDeleteVariant( wxCommandEvent& aEvent ) override;
    void onRenameVariant( wxCommandEvent& aEvent ) override;
    void onCopyVariant( wxCommandEvent& aEvent ) override;
    void onVariantSelectionChange( wxCommandEvent& aEvent ) override;

    void updateVariantButtonStates();

    wxString getSelectedVariant() const;

private:
    std::map<wxString, BOM_PRESET>     m_bomPresets;
    BOM_PRESET*                        m_currentBomPreset;
    BOM_PRESET*                        m_lastSelectedBomPreset;
    wxArrayString                      m_bomPresetMRU;

    std::map<wxString, BOM_FMT_PRESET> m_bomFmtPresets;
    BOM_FMT_PRESET*                    m_currentBomFmtPreset;
    BOM_FMT_PRESET*                    m_lastSelectedBomFmtPreset;
    wxArrayString                      m_bomFmtPresetMRU;

    SCH_EDIT_FRAME*                    m_parent;

    // Index in the fields list control for each MANDATORY_FIELD type
    std::map<FIELD_T, int>             m_mandatoryFieldListIndexes;

    VIEW_CONTROLS_GRID_DATA_MODEL*     m_viewControlsDataModel;

    SCH_REFERENCE_LIST                 m_symbolsList;
    FIELDS_EDITOR_GRID_DATA_MODEL*     m_dataModel;

    SCHEMATIC_SETTINGS&                m_schSettings;

    JOB_EXPORT_SCH_BOM* m_job;
};
