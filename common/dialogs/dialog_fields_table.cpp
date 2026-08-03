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

#include "dialog_fields_table.h"
#include "fields_view_controls_grid_data_model.h"

#include <algorithm>
#include <bitmaps.h>
#include <confirm.h>
#include <eda_list_dialog.h>
#include <wildcards_and_files_ext.h>
#include <widgets/std_bitmap_button.h>
#include <widgets/ui_common.h>
#include <wx/filename.h>

#ifdef __WXMAC__
#define COLUMN_MARGIN 4
#else
#define COLUMN_MARGIN 15
#endif


void DIALOG_FIELDS_TABLE::ShowEditTab()
{
    m_nbPages->SetSelection( 0 );
}


void DIALOG_FIELDS_TABLE::ShowExportTab()
{
    m_nbPages->SetSelection( 1 );
}


wxString DIALOG_FIELDS_TABLE::GetDefaultBomFileName( const wxString& aInputFileName )
{
    if( aInputFileName.IsEmpty() )
        return wxEmptyString;

    wxFileName fn( aInputFileName );
    fn.SetExt( FILEEXT::CsvFileExtension );

    return fn.GetFullPath();
}


void DIALOG_FIELDS_TABLE::setSideBarButtonLook( bool aIsLeftPanelCollapsed )
{
    // Set bitmap and tooltip according to left panel visibility

    if( aIsLeftPanelCollapsed )
    {
        m_sidebarButton->SetBitmap( KiBitmapBundle( BITMAPS::right ) );
        m_sidebarButton->SetToolTip( _( "Expand left panel" ) );
    }
    else
    {
        m_sidebarButton->SetBitmap( KiBitmapBundle( BITMAPS::left ) );
        m_sidebarButton->SetToolTip( _( "Collapse left panel" ) );
    }
}


void DIALOG_FIELDS_TABLE::OnTableValueChanged( wxGridEvent& aEvent )
{
    m_grid->ForceRefresh();
}


void DIALOG_FIELDS_TABLE::OnTableColSize( wxGridSizeEvent& aEvent )
{
    aEvent.Skip();

    m_grid->ForceRefresh();
}


void DIALOG_FIELDS_TABLE::OnFilterMouseMoved( wxMouseEvent& aEvent )
{
#if defined( __WXOSX__ ) // Doesn't work properly on other ports
    wxPoint pos = aEvent.GetPosition();
    wxRect  ctrlRect = m_filter->GetScreenRect();
    int     buttonWidth = ctrlRect.GetHeight(); // Presume buttons are square

    // TODO: restore cursor when mouse leaves the filter field (or is it a MSW bug?)
    if( m_filter->IsSearchButtonVisible() && pos.x < buttonWidth )
        SetCursor( wxCURSOR_ARROW );
    else if( m_filter->IsCancelButtonVisible() && pos.x > ctrlRect.GetWidth() - buttonWidth )
        SetCursor( wxCURSOR_ARROW );
    else
        SetCursor( wxCURSOR_IBEAM );
#endif
}


void DIALOG_FIELDS_TABLE::OnSizeViewControlsGrid( wxSizeEvent& event )
{
    const wxString& showColLabel = m_viewControlsGrid->GetColLabelValue( SHOW_FIELD_COLUMN );
    const wxString& groupByColLabel = m_viewControlsGrid->GetColLabelValue( GROUP_BY_COLUMN );
    int             showColWidth = KIUI::GetTextSize( showColLabel, m_viewControlsGrid ).x + COLUMN_MARGIN;
    int             groupByColWidth = KIUI::GetTextSize( groupByColLabel, m_viewControlsGrid ).x + COLUMN_MARGIN;
    int             remainingWidth = m_viewControlsGrid->GetSize().GetX() - showColWidth - groupByColWidth;

    m_viewControlsGrid->SetColSize( showColWidth, SHOW_FIELD_COLUMN );
    m_viewControlsGrid->SetColSize( groupByColWidth, GROUP_BY_COLUMN );

    if( m_viewControlsGrid->IsColShown( DISPLAY_NAME_COLUMN ) && m_viewControlsGrid->IsColShown( LABEL_COLUMN ) )
    {
        m_viewControlsGrid->SetColSize( DISPLAY_NAME_COLUMN, std::max( remainingWidth / 2, 60 ) );
        m_viewControlsGrid->SetColSize( LABEL_COLUMN, std::max( remainingWidth - ( remainingWidth / 2 ), 60 ) );
    }
    else if( m_viewControlsGrid->IsColShown( DISPLAY_NAME_COLUMN ) )
    {
        m_viewControlsGrid->SetColSize( DISPLAY_NAME_COLUMN, std::max( remainingWidth, 60 ) );
    }
    else if( m_viewControlsGrid->IsColShown( LABEL_COLUMN ) )
    {
        m_viewControlsGrid->SetColSize( LABEL_COLUMN, std::max( remainingWidth, 60 ) );
    }

    event.Skip();
}


std::vector<BOM_PRESET> DIALOG_FIELDS_TABLE::GetUserBomPresets() const
{
    std::vector<BOM_PRESET> ret;

    for( const std::pair<const wxString, BOM_PRESET>& pair : m_bomPresets )
    {
        if( !pair.second.readOnly )
            ret.emplace_back( pair.second );
    }

    return ret;
}


void DIALOG_FIELDS_TABLE::SetUserBomPresets( std::vector<BOM_PRESET>& aPresetList )
{
    // Reset to defaults
    loadDefaultBomPresets();

    for( const BOM_PRESET& preset : aPresetList )
    {
        if( m_bomPresets.count( preset.name ) )
            continue;

        m_bomPresets[preset.name] = preset;

        m_bomPresetMRU.Add( preset.name );
    }

    rebuildBomPresetsWidget();
}


void DIALOG_FIELDS_TABLE::ApplyBomPreset( const wxString& aPresetName )
{
    updateBomPresetSelection( aPresetName );

    wxCommandEvent dummy;
    onBomPresetChanged( dummy );
}


void DIALOG_FIELDS_TABLE::ApplyBomPreset( const BOM_PRESET& aPreset )
{
    if( m_bomPresets.count( aPreset.name ) )
        m_currentBomPreset = &m_bomPresets[aPreset.name];
    else
        m_currentBomPreset = nullptr;

    if( m_currentBomPreset && !m_currentBomPreset->readOnly )
        m_lastSelectedBomPreset = m_currentBomPreset;
    else
        m_lastSelectedBomPreset = nullptr;

    updateBomPresetSelection( aPreset.name );
    doApplyBomPreset( aPreset );
}


void DIALOG_FIELDS_TABLE::loadDefaultBomPresets()
{
    m_bomPresets.clear();
    m_bomPresetMRU.clear();

    // Load the read-only defaults
    for( const BOM_PRESET& preset : BOM_PRESET::BuiltInPresets() )
    {
        m_bomPresets[preset.name] = preset;
        m_bomPresets[preset.name].readOnly = true;

        m_bomPresetMRU.Add( preset.name );
    }
}


void DIALOG_FIELDS_TABLE::rebuildBomPresetsWidget()
{
    m_cbBomPresets->Clear();

    int idx = 0;
    int default_idx = 0;

    for( const auto& [presetName, preset] : m_bomPresets )
    {
        m_cbBomPresets->Append( wxGetTranslation( presetName ), (void*) &preset );

        if( presetName == BOM_PRESET::DefaultEditing().name )
            default_idx = idx;

        idx++;
    }

    m_cbBomPresets->Append( wxT( "---" ) );
    m_cbBomPresets->Append( _( "Save preset..." ) );
    m_cbBomPresets->Append( _( "Delete preset..." ) );

    // At least the built-in presets should always be present
    wxASSERT( !m_bomPresets.empty() );

    m_cbBomPresets->SetSelection( default_idx );
    m_currentBomPreset = static_cast<BOM_PRESET*>( m_cbBomPresets->GetClientData( default_idx ) );
}


void DIALOG_FIELDS_TABLE::syncBomPresetSelection()
{
    BOM_PRESET current = getDataModelBomPreset();

    auto it = std::find_if( m_bomPresets.begin(), m_bomPresets.end(),
            [&]( const std::pair<const wxString, BOM_PRESET>& aPair )
            {
                const BOM_PRESET& preset = aPair.second;

                // Check the simple settings first
                if( !( preset.sortAsc == current.sortAsc
                       && preset.filterString == current.filterString
                       && preset.groupSymbols == current.groupSymbols
                       && preset.excludeDNP == current.excludeDNP
                       && preset.includeExcludedFromBOM == current.includeExcludedFromBOM ) )
                {
                    return false;
                }

                // We should compare preset.name and current.name.  Unfortunately current.name is
                // empty because m_dataModel->GetBomSettings() does not store the .name member.
                // So use sortField member as a (not very efficient) auxiliary filter.
                // As a further complication, sortField can be translated in m_bomPresets list, so
                // current.sortField needs to be translated.
                // Probably this not efficient and error prone test should be removed (JPC).
                if( preset.sortField != wxGetTranslation( current.sortField ) )
                    return false;

                // Only compare shown or grouped fields
                std::vector<BOM_FIELD> A, B;

                for( const BOM_FIELD& field : preset.fieldsOrdered )
                {
                    if( field.show || field.groupBy )
                        A.emplace_back( field );
                }

                for( const BOM_FIELD& field : current.fieldsOrdered )
                {
                    if( field.show || field.groupBy )
                        B.emplace_back( field );
                }

                return A == B;
            } );

    if( it != m_bomPresets.end() )
    {
        // Select the right m_cbBomPresets item.
        // but these items are translated if they are predefined items.
        bool     do_translate = it->second.readOnly;
        wxString text = do_translate ? wxGetTranslation( it->first ) : it->first;
        m_cbBomPresets->SetStringSelection( text );
    }
    else
    {
        m_cbBomPresets->SetSelection( m_cbBomPresets->GetCount() - 3 ); // separator
    }

    m_currentBomPreset = static_cast<BOM_PRESET*>( m_cbBomPresets->GetClientData( m_cbBomPresets->GetSelection() ) );
}


void DIALOG_FIELDS_TABLE::updateBomPresetSelection( const wxString& aName )
{
    // Look at m_userBomPresets to know if aName is a read only preset, or a user preset.
    // Read-only presets have translated names in UI, so we have to use a translated name
    // in UI selection.  But for a user preset name we search for the untranslated aName.
    wxString ui_label = aName;

    for( const auto& [presetName, preset] : m_bomPresets )
    {
        if( presetName == aName )
        {
            if( preset.readOnly == true )
                ui_label = wxGetTranslation( aName );

            break;
        }
    }

    int idx = m_cbBomPresets->FindString( ui_label );

    if( idx >= 0 && m_cbBomPresets->GetSelection() != idx )
    {
        m_cbBomPresets->SetSelection( idx );
        m_currentBomPreset = static_cast<BOM_PRESET*>( m_cbBomPresets->GetClientData( idx ) );
    }
    else if( idx < 0 )
    {
        m_cbBomPresets->SetSelection( m_cbBomPresets->GetCount() - 3 ); // separator
    }
}


void DIALOG_FIELDS_TABLE::onBomPresetChanged( wxCommandEvent& aEvent )
{
    int count = m_cbBomPresets->GetCount();
    int index = m_cbBomPresets->GetSelection();

    auto resetSelection =
            [&]()
            {
                if( m_currentBomPreset )
                    m_cbBomPresets->SetStringSelection( m_currentBomPreset->name );
                else
                    m_cbBomPresets->SetSelection( m_cbBomPresets->GetCount() - 3 );
            };

    if( index == count - 3 )
    {
        // Separator: reject the selection
        resetSelection();
        return;
    }
    else if( index == count - 2 )
    {
        // Save current state to new preset
        wxString name;

        if( m_lastSelectedBomPreset )
            name = m_lastSelectedBomPreset->name;

        wxTextEntryDialog dlg( this, _( "BOM preset name:" ), _( "Save BOM Preset" ), name );

        if( dlg.ShowModal() != wxID_OK )
        {
            resetSelection();
            return;
        }

        name = dlg.GetValue();
        bool exists = m_bomPresets.count( name );

        if( !exists )
        {
            m_bomPresets[name] = getDataModelBomPreset();
            m_bomPresets[name].readOnly = false;
            m_bomPresets[name].name = name;
        }

        BOM_PRESET* preset = &m_bomPresets[name];

        if( !exists )
        {
            index = m_cbBomPresets->Insert( name, index - 1, static_cast<void*>( preset ) );
        }
        else if( preset->readOnly )
        {
            wxMessageBox( _( "Default presets cannot be modified.\nPlease use a different name." ),
                          _( "Error" ), wxOK | wxICON_ERROR, this );
            resetSelection();
            return;
        }
        else
        {
            // Ask the user if they want to overwrite the existing preset
            if( !IsOK( this, _( "Overwrite existing preset?" ) ) )
            {
                resetSelection();
                return;
            }

            *preset = getDataModelBomPreset();
            preset->name = name;

            index = m_cbBomPresets->FindString( name );

            if( m_bomPresetMRU.Index( name ) != wxNOT_FOUND )
                m_bomPresetMRU.Remove( name );
        }

        m_currentBomPreset = preset;
        m_cbBomPresets->SetSelection( index );
        m_bomPresetMRU.Insert( name, 0 );

        return;
    }
    else if( index == count - 1 )
    {
        // Delete a preset
        wxArrayString              headers;
        std::vector<wxArrayString> items;

        headers.Add( _( "Presets" ) );

        for( const auto& [name, preset] : m_bomPresets )
        {
            if( !preset.readOnly )
            {
                wxArrayString item;
                item.Add( name );
                items.emplace_back( item );
            }
        }

        EDA_LIST_DIALOG dlg( this, _( "Delete Preset" ), headers, items );
        dlg.SetListLabel( _( "Select preset:" ) );

        if( dlg.ShowModal() == wxID_OK )
        {
            wxString presetName = dlg.GetTextSelection();
            int      idx = m_cbBomPresets->FindString( presetName );

            if( idx != wxNOT_FOUND )
            {
                m_bomPresets.erase( presetName );

                m_cbBomPresets->Delete( idx );
                m_currentBomPreset = nullptr;
            }

            if( m_bomPresetMRU.Index( presetName ) != wxNOT_FOUND )
                m_bomPresetMRU.Remove( presetName );
        }

        resetSelection();
        return;
    }

    BOM_PRESET* preset = static_cast<BOM_PRESET*>( m_cbBomPresets->GetClientData( index ) );
    m_currentBomPreset = preset;

    m_lastSelectedBomPreset = ( !preset || preset->readOnly ) ? nullptr : preset;

    if( preset )
    {
        doApplyBomPreset( *preset );
        syncBomPresetSelection();
        m_currentBomPreset = preset;

        if( !m_currentBomPreset->name.IsEmpty() )
        {
            if( m_bomPresetMRU.Index( preset->name ) != wxNOT_FOUND )
                m_bomPresetMRU.Remove( preset->name );

            m_bomPresetMRU.Insert( preset->name, 0 );
        }
    }
}


BOM_FMT_PRESET DIALOG_FIELDS_TABLE::GetCurrentBomFmtSettings()
{
    BOM_FMT_PRESET current;

    current.name = m_cbBomFmtPresets->GetStringSelection();
    current.fieldDelimiter = m_textFieldDelimiter->GetValue();
    current.stringDelimiter = m_textStringDelimiter->GetValue();
    current.refDelimiter = m_textRefDelimiter->GetValue();
    current.refRangeDelimiter = m_textRefRangeDelimiter->GetValue();
    current.keepTabs = m_checkKeepTabs->GetValue();
    current.keepLineBreaks = m_checkKeepLineBreaks->GetValue();
    current.includeByteOrderMark = m_checkIncludeByteOrderMark->GetValue();

    return current;
}


std::vector<BOM_FMT_PRESET> DIALOG_FIELDS_TABLE::GetUserBomFmtPresets() const
{
    std::vector<BOM_FMT_PRESET> ret;

    for( const auto& [name, preset] : m_bomFmtPresets )
    {
        if( !preset.readOnly )
            ret.emplace_back( preset );
    }

    return ret;
}


void DIALOG_FIELDS_TABLE::SetUserBomFmtPresets( std::vector<BOM_FMT_PRESET>& aPresetList )
{
    // Reset to defaults
    loadDefaultBomFmtPresets();

    for( const BOM_FMT_PRESET& preset : aPresetList )
    {
        if( m_bomFmtPresets.count( preset.name ) )
            continue;

        m_bomFmtPresets[preset.name] = preset;

        m_bomFmtPresetMRU.Add( preset.name );
    }

    rebuildBomFmtPresetsWidget();
}


void DIALOG_FIELDS_TABLE::ApplyBomFmtPreset( const wxString& aPresetName )
{
    updateBomFmtPresetSelection( aPresetName );

    wxCommandEvent dummy;
    onBomFmtPresetChanged( dummy );
}


void DIALOG_FIELDS_TABLE::ApplyBomFmtPreset( const BOM_FMT_PRESET& aPreset )
{
    m_currentBomFmtPreset = nullptr;
    m_lastSelectedBomFmtPreset = nullptr;

    if( m_bomFmtPresets.count( aPreset.name ) )
        m_currentBomFmtPreset = &m_bomFmtPresets[aPreset.name];

    if( m_currentBomFmtPreset && !m_currentBomFmtPreset->readOnly )
        m_lastSelectedBomFmtPreset =m_currentBomFmtPreset;

    updateBomFmtPresetSelection( aPreset.name );
    doApplyBomFmtPreset( aPreset );
}


void DIALOG_FIELDS_TABLE::loadDefaultBomFmtPresets()
{
    m_bomFmtPresets.clear();
    m_bomFmtPresetMRU.clear();

    // Load the read-only defaults
    for( const BOM_FMT_PRESET& preset : BOM_FMT_PRESET::BuiltInPresets() )
    {
        m_bomFmtPresets[preset.name] = preset;
        m_bomFmtPresets[preset.name].readOnly = true;

        m_bomFmtPresetMRU.Add( preset.name );
    }
}


void DIALOG_FIELDS_TABLE::rebuildBomFmtPresetsWidget()
{
    m_cbBomFmtPresets->Clear();

    int idx = 0;
    int default_idx = 0;

    for( const auto& [presetName, preset] : m_bomFmtPresets )
    {
        m_cbBomFmtPresets->Append( wxGetTranslation( presetName ), (void*) &preset );

        if( presetName == BOM_FMT_PRESET::CSV().name )
            default_idx = idx;

        idx++;
    }

    m_cbBomFmtPresets->Append( wxT( "---" ) );
    m_cbBomFmtPresets->Append( _( "Save preset..." ) );
    m_cbBomFmtPresets->Append( _( "Delete preset..." ) );

    // At least the built-in presets should always be present
    wxASSERT( !m_bomFmtPresets.empty() );

    m_cbBomFmtPresets->SetSelection( default_idx );
    m_currentBomFmtPreset = static_cast<BOM_FMT_PRESET*>( m_cbBomFmtPresets->GetClientData( default_idx ) );
}


void DIALOG_FIELDS_TABLE::syncBomFmtPresetSelection()
{
    BOM_FMT_PRESET current = GetCurrentBomFmtSettings();

    auto it = std::find_if( m_bomFmtPresets.begin(), m_bomFmtPresets.end(),
                            [&]( const std::pair<const wxString, BOM_FMT_PRESET>& aPair )
                            {
                                return ( aPair.second.fieldDelimiter == current.fieldDelimiter
                                         && aPair.second.stringDelimiter == current.stringDelimiter
                                         && aPair.second.refDelimiter == current.refDelimiter
                                         && aPair.second.refRangeDelimiter == current.refRangeDelimiter
                                         && aPair.second.keepTabs == current.keepTabs
                                         && aPair.second.keepLineBreaks == current.keepLineBreaks
                                         && aPair.second.includeByteOrderMark == current.includeByteOrderMark );
                            } );

    if( it != m_bomFmtPresets.end() )
    {
        // Select the right m_cbBomFmtPresets item.
        // but these items are translated if they are predefined items.
        bool     do_translate = it->second.readOnly;
        wxString text = do_translate ? wxGetTranslation( it->first ) : it->first;

        m_cbBomFmtPresets->SetStringSelection( text );
    }
    else
    {
        m_cbBomFmtPresets->SetSelection( m_cbBomFmtPresets->GetCount() - 3 ); // separator
    }

    int idx = m_cbBomFmtPresets->GetSelection();
    m_currentBomFmtPreset = static_cast<BOM_FMT_PRESET*>( m_cbBomFmtPresets->GetClientData( idx ) );
}


void DIALOG_FIELDS_TABLE::updateBomFmtPresetSelection( const wxString& aName )
{
    // look at m_userBomFmtPresets to know if aName is a read only preset, or a user preset.
    // Read only presets have translated names in UI, so we have to use a translated name in UI selection.
    // But for a user preset name we should search for aName (not translated)
    wxString ui_label = aName;

    for( const auto& [presetName, preset] : m_bomFmtPresets )
    {
        if( presetName == aName )
        {
            if( preset.readOnly )
                ui_label = wxGetTranslation( aName );

            break;
        }
    }

    int idx = m_cbBomFmtPresets->FindString( ui_label );

    if( idx >= 0 && m_cbBomFmtPresets->GetSelection() != idx )
    {
        m_cbBomFmtPresets->SetSelection( idx );
        m_currentBomFmtPreset = static_cast<BOM_FMT_PRESET*>( m_cbBomFmtPresets->GetClientData( idx ) );
    }
    else if( idx < 0 )
    {
        m_cbBomFmtPresets->SetSelection( m_cbBomFmtPresets->GetCount() - 3 ); // separator
    }
}


void DIALOG_FIELDS_TABLE::onBomFmtPresetChanged( wxCommandEvent& aEvent )
{
    int count = m_cbBomFmtPresets->GetCount();
    int index = m_cbBomFmtPresets->GetSelection();

    auto resetSelection =
            [&]()
            {
                if( m_currentBomFmtPreset )
                    m_cbBomFmtPresets->SetStringSelection( m_currentBomFmtPreset->name );
                else
                    m_cbBomFmtPresets->SetSelection( m_cbBomFmtPresets->GetCount() - 3 );
            };

    if( index == count - 3 )
    {
        // Separator: reject the selection
        resetSelection();
        return;
    }
    else if( index == count - 2 )
    {
        // Save current state to new preset
        wxString name;

        if( m_lastSelectedBomFmtPreset )
            name = m_lastSelectedBomFmtPreset->name;

        wxTextEntryDialog dlg( this, _( "BOM preset name:" ), _( "Save BOM Preset" ), name );

        if( dlg.ShowModal() != wxID_OK )
        {
            resetSelection();
            return;
        }

        name = dlg.GetValue();
        bool exists = m_bomFmtPresets.count( name );

        if( !exists )
        {
            m_bomFmtPresets[name] = GetCurrentBomFmtSettings();
            m_bomFmtPresets[name].readOnly = false;
            m_bomFmtPresets[name].name = name;
        }

        BOM_FMT_PRESET* preset = &m_bomFmtPresets[name];

        if( !exists )
        {
            index = m_cbBomFmtPresets->Insert( name, index - 1, static_cast<void*>( preset ) );
        }
        else if( preset->readOnly )
        {
            wxMessageBox( _( "Default presets cannot be modified.\nPlease use a different name." ),
                          _( "Error" ), wxOK | wxICON_ERROR, this );
            resetSelection();
            return;
        }
        else
        {
            // Ask the user if they want to overwrite the existing preset
            if( !IsOK( this, _( "Overwrite existing preset?" ) ) )
            {
                resetSelection();
                return;
            }

            *preset = GetCurrentBomFmtSettings();
            preset->name = name;

            index = m_cbBomFmtPresets->FindString( name );

            if( m_bomFmtPresetMRU.Index( name ) != wxNOT_FOUND )
                m_bomFmtPresetMRU.Remove( name );
        }

        m_currentBomFmtPreset = preset;
        m_cbBomFmtPresets->SetSelection( index );
        m_bomFmtPresetMRU.Insert( name, 0 );

        return;
    }
    else if( index == count - 1 )
    {
        // Delete a preset
        wxArrayString              headers;
        std::vector<wxArrayString> items;

        headers.Add( _( "Presets" ) );

        for( std::pair<const wxString, BOM_FMT_PRESET>& pair : m_bomFmtPresets )
        {
            if( !pair.second.readOnly )
            {
                wxArrayString item;
                item.Add( pair.first );
                items.emplace_back( item );
            }
        }

        EDA_LIST_DIALOG dlg( this, _( "Delete Preset" ), headers, items );
        dlg.SetListLabel( _( "Select preset:" ) );

        if( dlg.ShowModal() == wxID_OK )
        {
            wxString presetName = dlg.GetTextSelection();
            int      idx = m_cbBomFmtPresets->FindString( presetName );

            if( idx != wxNOT_FOUND )
            {
                m_bomFmtPresets.erase( presetName );

                m_cbBomFmtPresets->Delete( idx );
                m_currentBomFmtPreset = nullptr;
            }

            if( m_bomFmtPresetMRU.Index( presetName ) != wxNOT_FOUND )
                m_bomFmtPresetMRU.Remove( presetName );
        }

        resetSelection();
        return;
    }

    auto* preset = static_cast<BOM_FMT_PRESET*>( m_cbBomFmtPresets->GetClientData( index ) );
    m_currentBomFmtPreset = preset;

    m_lastSelectedBomFmtPreset = ( !preset || preset->readOnly ) ? nullptr : preset;

    if( preset )
    {
        doApplyBomFmtPreset( *preset );
        syncBomFmtPresetSelection();
        m_currentBomFmtPreset = preset;

        if( !m_currentBomFmtPreset->name.IsEmpty() )
        {
            if( m_bomFmtPresetMRU.Index( preset->name ) != wxNOT_FOUND )
                m_bomFmtPresetMRU.Remove( preset->name );

            m_bomFmtPresetMRU.Insert( preset->name, 0 );
        }
    }
}
