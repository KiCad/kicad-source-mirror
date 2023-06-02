/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Oliver Walters
 * Copyright (C) 2017-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef DIALOG_SYMBOL_FIELDS_TABLE_H
#define DIALOG_SYMBOL_FIELDS_TABLE_H


#include <dialog_symbol_fields_table_base.h>
#include <sch_reference_list.h>


class SCHEMATIC_SETTINGS;
struct BOM_PRESET;
struct BOM_FMT_PRESET;
class SCH_EDIT_FRAME;
class FIELDS_EDITOR_GRID_DATA_MODEL;


class DIALOG_SYMBOL_FIELDS_TABLE : public DIALOG_SYMBOL_FIELDS_TABLE_BASE
{
public:
    DIALOG_SYMBOL_FIELDS_TABLE( SCH_EDIT_FRAME* parent );
    virtual ~DIALOG_SYMBOL_FIELDS_TABLE();

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

private:
    void SetupColumnProperties();
    void AddField( const wxString& displayName, const wxString& aCanonicalName, bool show,
                   bool groupBy, bool addedByUser = false );

    /**
     * Construct the rows of m_fieldsCtrl and the columns of m_dataModel from a union of all
     * field names in use.
     */
    void LoadFieldNames();

    void OnColSort( wxGridEvent& aEvent );
    void OnColMove( wxGridEvent& aEvent );
    void OnColLabelChange( wxDataViewEvent& aEvent );
    void OnTableRangeSelected( wxGridEvent& event );

    void OnColumnItemToggled( wxDataViewEvent& event ) override;
    void OnGroupSymbolsToggled( wxCommandEvent& event ) override;
    void OnExcludeDNPToggled( wxCommandEvent& event ) override;
    void OnRegroupSymbols( wxCommandEvent& aEvent ) override;
    void OnTableValueChanged( wxGridEvent& event ) override;
    void OnTableCellClick( wxGridEvent& event ) override;
    void OnTableItemContextMenu( wxGridEvent& event ) override;
    void OnTableColSize( wxGridSizeEvent& event ) override;
    void OnSizeFieldList( wxSizeEvent& event ) override;
    void OnAddField( wxCommandEvent& event ) override;
    void OnRemoveField( wxCommandEvent& event ) override;
    void OnRenameField( wxCommandEvent& event ) override;
    void OnExport( wxCommandEvent& aEvent ) override;
    void OnSaveAndContinue( wxCommandEvent& aEvent ) override;
    void OnCancel( wxCommandEvent& aEvent ) override;
    void OnClose( wxCloseEvent& aEvent ) override;
    void OnFilterText( wxCommandEvent& aEvent ) override;
    void OnFilterMouseMoved( wxMouseEvent& event ) override;
    void OnFieldsCtrlSelectionChanged( wxDataViewEvent& event ) override;

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

private:
    void syncBomPresetSelection();
    void rebuildBomPresetsWidget();
    void updateBomPresetSelection( const wxString& aName );
    void onBomPresetChanged( wxCommandEvent& aEvent );
    void doApplyBomPreset( const BOM_PRESET& aPreset );
    void loadDefaultBomPresets();

    std::map<wxString, BOM_PRESET> m_bomPresets;
    BOM_PRESET*                    m_currentBomPreset;
    BOM_PRESET*                    m_lastSelectedBomPreset;
    wxArrayString                  m_bomPresetMRU;

    void syncBomFmtPresetSelection();
    void rebuildBomFmtPresetsWidget();
    void updateBomFmtPresetSelection( const wxString& aName );
    void onBomFmtPresetChanged( wxCommandEvent& aEvent );
    void doApplyBomFmtPreset( const BOM_FMT_PRESET& aPreset );
    void loadDefaultBomFmtPresets();

    std::map<wxString, BOM_FMT_PRESET> m_bomFmtPresets;
    BOM_FMT_PRESET*                    m_currentBomFmtPreset;
    BOM_FMT_PRESET*                    m_lastSelectedBomFmtPreset;
    wxArrayString                      m_bomFmtPresetMRU;

    void savePresetsToSchematic();

    SCH_EDIT_FRAME*                m_parent;
    int                            m_fieldNameColWidth;
    int                            m_labelColWidth;
    int                            m_showColWidth;
    int                            m_groupByColWidth;

    SCH_REFERENCE_LIST             m_symbolsList;
    FIELDS_EDITOR_GRID_DATA_MODEL* m_dataModel;

    SCHEMATIC_SETTINGS&            m_schSettings;
};

#endif /* DIALOG_SYMBOL_FIELDS_TABLE_H */
