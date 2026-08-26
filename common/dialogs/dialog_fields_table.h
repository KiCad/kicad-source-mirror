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
#include <set>
#include <vector>

#include <dialog_fields_table_base.h>
#include <grid_tricks.h>
#include <kiid.h>
#include <settings/bom_settings.h>

struct FIELDS_TABLE_SETTINGS;
enum class FIELD_T : int;

class FIELDS_TABLE_DATA_MODEL_BASE;
class JOB_EXPORT_BOM;
class VIEW_CONTROLS_GRID_DATA_MODEL;
class wxGridCellEditor;


class DIALOG_FIELDS_TABLE;


class FIELDS_TABLE_GRID_TRICKS : public GRID_TRICKS
{
public:
    static constexpr int FIRST_CLIENT_ID = GRIDTRICKS_FIRST_CLIENT_ID + 2;

    FIELDS_TABLE_GRID_TRICKS( DIALOG_FIELDS_TABLE* aDialog, WX_GRID* aGrid,
                              FIELDS_TABLE_DATA_MODEL_BASE* aDataModel );

protected:
    void showPopupMenu( wxMenu& aMenu, wxGridEvent& aEvent ) override;
    void doPopupSelection( wxCommandEvent& aEvent ) override;

    virtual void showFieldsTablePopupMenu( wxMenu& aMenu, wxGridEvent& aEvent );
    virtual void doFieldsTablePopupSelection( wxCommandEvent& aEvent );

private:
    DIALOG_FIELDS_TABLE*          m_dialog;
    FIELDS_TABLE_DATA_MODEL_BASE* m_dataModel;
};


class DIALOG_FIELDS_TABLE : public DIALOG_FIELDS_TABLE_BASE
{
public:
    DIALOG_FIELDS_TABLE( wxWindow* aParent, FIELDS_TABLE_SETTINGS& aPanelSettings,
                         FIELDS_TABLE_BOM_SETTINGS& aBomSettings, JOB_EXPORT_BOM* aJob );
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

    void EnableSelectionEvents();
    void DisableSelectionEvents();
    void OnTableRangeSelectionChanged( wxGridRangeSelectEvent& aEvent );

    /// Return the explicitly selected grid rows, or the cursor row if the selection is empty.
    std::set<int> GetSelectedGridRows() const;

    /// Save the grid selection by stable item identity so it can survive a row rebuild.
    std::set<KIID_PATH> SaveGridSelection();

    /// Restore a selection previously returned by SaveGridSelection().
    void                RestoreGridSelection( const std::set<KIID_PATH>& aItemKeys );

    void AddField( const wxString& aFieldName, const wxString& aLabelValue, bool aShow, bool aGroupBy,
                   bool aAddedByUser = false );

    void SetReadOnly( bool aReadOnly );

    static void loadJobBomPreset( const JOB_EXPORT_BOM& aJob, BOM_PRESET& aPreset );
    static void loadJobBomFmtPreset( const JOB_EXPORT_BOM& aJob, BOM_FMT_PRESET& aPreset );
    void        saveJobSettings( JOB_EXPORT_BOM& aJob );

    bool savePresets( bool aSaveCurrentSettings );


    void RestorePanelLayout();
    void SavePanelLayout();
    void SaveColumnWidths();

    void SetupColumnProperties( int aCol );
    void SetupAllColumnProperties();

    virtual wxGridCellEditor* createFootprintEditor();
    virtual wxGridCellEditor* createDatasheetEditor() = 0;
    virtual void              onBomSettingsChanged();

    // Set bitmap and tooltip according to left panel visibility
    void setSideBarButtonLook( bool aIsLeftPanelCollapsed );

    void OnTableCellClick( wxGridEvent& event ) override;
    void OnTableValueChanged( wxGridEvent& event ) override;
    void OnTableColSize( wxGridSizeEvent& event ) override;
    virtual void OnTableSelectionChanged( const std::set<int>& aRows ) = 0;
    void OnSizeViewControlsGrid( wxSizeEvent& event ) override;
    void OnViewControlsCellChanged( wxGridEvent& aEvent ) override;

    void OnAddField( wxCommandEvent& aEvent ) override;
    void OnRemoveField( wxCommandEvent& aEvent ) override;
    void OnRenameField( wxCommandEvent& aEvent ) override;

    void OnFilterMouseMoved( wxMouseEvent& event ) override;
    void OnFilterText( wxCommandEvent& aEvent ) override;
    void OnFilterScope( wxCommandEvent& aEvent ) override;
    void OnGroupSymbolsToggled( wxCommandEvent& aEvent ) override;
    void OnRegroupSymbols( wxCommandEvent& aEvent ) override;
    void OnColSort( wxGridEvent& aEvent );
    void OnColMove( wxGridEvent& aEvent );
    void OnGridMouseMove( wxMouseEvent& aEvent );
    void OnGridMouseWheel( wxMouseEvent& aEvent );

    void OnPageChanged( wxNotebookEvent& aEvent ) override;
    void OnPreviewRefresh( wxCommandEvent& aEvent ) override;
    void OnOutputFileBrowseClicked( wxCommandEvent& aEvent ) override;
    void OnExport( wxCommandEvent& aEvent ) override;
    void OnSidebarToggle( wxCommandEvent& event ) override;

    void PreviewRefresh();

    wxString getSelectedVariant() const;

    void syncBomPresetSelection();
    void rebuildBomPresetsWidget();
    int  presetDashDashDashIndex( int aPresetCount ) const;
    int  presetSavePresetIndex( int aPresetCount ) const;
    int  presetDeletePresetIndex( int aPresetCount ) const;
    void updateBomPresetSelection( const wxString& aName );
    void onBomPresetChanged( wxCommandEvent& aEvent );
    void loadDefaultBomPresets();
    virtual std::vector<BOM_PRESET> getBuiltInBomPresets() const;
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
    JOB_EXPORT_BOM*            m_job;

    VIEW_CONTROLS_GRID_DATA_MODEL* m_viewControlsDataModel = nullptr;
    int                            m_gridWheelRotation = 0;

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
