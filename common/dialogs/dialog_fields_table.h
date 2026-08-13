/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) Mike Williams
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <map>
#include <vector>

#include <dialog_fields_table_base.h>
#include <settings/bom_settings.h>

struct FIELDS_TABLE_SETTINGS;
enum class FIELD_T : int;

class FIELDS_TABLE_DATA_MODEL_BASE;
class JOB_EXPORT_BOM;
class VIEW_CONTROLS_GRID_DATA_MODEL;
class wxGridCellEditor;

class DIALOG_FIELDS_TABLE : public DIALOG_FIELDS_TABLE_BASE
{
public:
    DIALOG_FIELDS_TABLE( wxWindow* aParent, FIELDS_TABLE_SETTINGS& aPanelSettings,
                         FIELDS_TABLE_BOM_SETTINGS& aBomSettings );
    ~DIALOG_FIELDS_TABLE() override;

    void ShowEditTab();
    void ShowExportTab();
    void ShowHideColumn( int aCol, bool aShow );

    /**
     * Derive the default BOM output file name from the input file name by swapping the
     * extension to CSV. Returns an empty string when the input has no name (unsaved), so
     * callers can distinguish "use the default" from "no destination is available".
     */
    static wxString GetDefaultBomFileName( const wxString& aInputFileName );

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


protected:
    wxSize GetDefaultDialogSize() const;

    void AddField( const wxString& aFieldName, const wxString& aLabelValue, bool aShow, bool aGroupBy,
                   bool aAddedByUser = false );

    static void loadJobBomPreset( const JOB_EXPORT_BOM& aJob, BOM_PRESET& aPreset );
    static void loadJobBomFmtPreset( const JOB_EXPORT_BOM& aJob, BOM_FMT_PRESET& aPreset );
    void        saveJobSettings( JOB_EXPORT_BOM& aJob );

    bool savePresets( bool aSaveCurrentSettings );


    void RestorePanelLayout();
    void SavePanelLayout();
    void SaveColumnWidths();

    void SetupColumnProperties( int aCol );
    void SetupAllColumnProperties();

    virtual wxGridCellEditor* createDatasheetEditor() = 0;

    // Set bitmap and tooltip according to left panel visibility
    void setSideBarButtonLook( bool aIsLeftPanelCollapsed );

    void OnTableValueChanged( wxGridEvent& event ) override;
    void OnTableColSize( wxGridSizeEvent& event ) override;
    void OnSizeViewControlsGrid( wxSizeEvent& event ) override;
    void OnViewControlsCellChanged( wxGridEvent& aEvent ) override;

    void OnAddField( wxCommandEvent& aEvent ) override;
    void OnRemoveField( wxCommandEvent& aEvent ) override;
    void OnRenameField( wxCommandEvent& aEvent ) override;

    void OnFilterMouseMoved( wxMouseEvent& event ) override;
    void OnFilterText( wxCommandEvent& aEvent ) override;
    void OnGroupSymbolsToggled( wxCommandEvent& aEvent ) override;
    void OnRegroupSymbols( wxCommandEvent& aEvent ) override;
    void OnColSort( wxGridEvent& aEvent );
    void OnColMove( wxGridEvent& aEvent );
    void OnGridMouseMove( wxMouseEvent& aEvent );

    void OnPageChanged( wxNotebookEvent& aEvent ) override;
    void OnPreviewRefresh( wxCommandEvent& aEvent ) override;
    void OnOutputFileBrowseClicked( wxCommandEvent& aEvent ) override;
    void OnExport( wxCommandEvent& aEvent ) override;
    void OnSidebarToggle( wxCommandEvent& event ) override;

    void PreviewRefresh();

    wxString getSelectedVariant() const;

    void syncBomPresetSelection();
    void rebuildBomPresetsWidget();
    void updateBomPresetSelection( const wxString& aName );
    void onBomPresetChanged( wxCommandEvent& aEvent );
    void loadDefaultBomPresets();
    void       doApplyBomPreset( const BOM_PRESET& aPreset );
    BOM_PRESET getDataModelBomPreset();

    void syncBomFmtPresetSelection();
    void rebuildBomFmtPresetsWidget();
    void updateBomFmtPresetSelection( const wxString& aName );
    void onBomFmtPresetChanged( wxCommandEvent& aEvent );
    void loadDefaultBomFmtPresets();
    void doApplyBomFmtPreset( const BOM_FMT_PRESET& aPreset );

    virtual FIELDS_TABLE_DATA_MODEL_BASE* getDataModel() const = 0;
    virtual wxString                      resolveVariant() const = 0;
    virtual bool                          resolveTextVar( wxString* aToken ) const = 0;

protected:
    FIELDS_TABLE_SETTINGS&     m_cfgDialogSettings;
    FIELDS_TABLE_BOM_SETTINGS& m_cfgBomSettings;

    VIEW_CONTROLS_GRID_DATA_MODEL* m_viewControlsDataModel = nullptr;

    // Index in the fields list control for each MANDATORY_FIELD type
    std::map<FIELD_T, int> m_mandatoryFieldListIndexes;

    std::map<wxString, BOM_PRESET>     m_bomPresets;
    BOM_PRESET*                        m_currentBomPreset = nullptr;
    BOM_PRESET*                        m_lastSelectedBomPreset = nullptr;
    wxArrayString                      m_bomPresetMRU;

    std::map<wxString, BOM_FMT_PRESET> m_bomFmtPresets;
    BOM_FMT_PRESET*                    m_currentBomFmtPreset = nullptr;
    BOM_FMT_PRESET*                    m_lastSelectedBomFmtPreset = nullptr;
    wxArrayString                      m_bomFmtPresetMRU;

};
