/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Roberto Fernandez Bautista <roberto.fer.bau@gmail.com>
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


#include <layer_ids.h>
#include <lseq.h>
#include <dialog_map_layers.h>
#include <board.h>
#include <pcb_base_frame.h>
#include <pcbnew_settings.h>
#include <ki_exception.h>
#include <widgets/wx_infobar.h>

#include <wx/checkbox.h>
#include <wx/filename.h>
#include <wx/msgdlg.h>


static wxString LayerNameError( const wxString& aName )
{
    if( aName.IsEmpty() )
        return _( "Layer names cannot be blank." );

    wxString badChars = wxFileName::GetForbiddenChars( wxPATH_DOS );
    badChars.Append( '%' );

    size_t pos = aName.find_first_of( badChars );

    if( pos != wxString::npos )
    {
        return wxString::Format( _( "'%s' contains '%s'. Forbidden characters are: %s" ), aName, aName.Mid( pos, 1 ),
                                 badChars );
    }

    if( aName == wxT( "signal" ) )
        return _( "Layer name 'signal' is reserved." );

    return wxEmptyString;
}


wxString DIALOG_MAP_LAYERS::WrapRequired( const wxString& aLayerName )
{
    return aLayerName + wxT( " *" );
}


wxString DIALOG_MAP_LAYERS::UnwrapRequired( const wxString& aLayerName )
{
    if( !aLayerName.EndsWith( wxT( " *" ) ) )
        return aLayerName;

    return aLayerName.Left( aLayerName.Length() - 2 );
}


wxString DIALOG_MAP_LAYERS::KiCadLayerName( PCB_LAYER_ID aLayer ) const
{
    if( m_board )
        return m_board->GetLayerName( aLayer );

    return LayerName( aLayer );
}


const INPUT_LAYER_DESC* DIALOG_MAP_LAYERS::GetLayerDescription( const wxString& aLayerName ) const
{
    wxString layerName = UnwrapRequired( aLayerName );

    for( const INPUT_LAYER_DESC& layerDescription : m_input_layers )
    {
        if( layerDescription.Name == layerName )
            return &layerDescription;
    }

    return nullptr;
}


PCB_LAYER_ID DIALOG_MAP_LAYERS::GetSelectedLayerID()
{
    // First check if there is a KiCad element selected
    wxString selectedKiCadLayerName;
    long     itemIndex = -1;

    if( ( itemIndex = m_kicad_layers_list->GetNextItem( itemIndex, wxLIST_NEXT_ALL,
                                                        wxLIST_STATE_SELECTED ) ) != wxNOT_FOUND )
    {
        selectedKiCadLayerName = m_kicad_layers_list->GetItemText( itemIndex );
    }
    else
    {
        return PCB_LAYER_ID::UNDEFINED_LAYER;
    }

    // There should only be one selected (or none) as the list is set with wxLC_SINGLE_SEL style
    wxASSERT_MSG( ( m_kicad_layers_list->GetNextItem( itemIndex, wxLIST_NEXT_ALL,
                                                      wxLIST_STATE_SELECTED ) ) == wxNOT_FOUND,
                  wxT( "There are more than one KiCad layer selected (unexpected)" ) );

    for( int layer = 0; layer < PCB_LAYER_ID_COUNT; ++layer )
    {
        if( KiCadLayerName( ToLAYER_ID( layer ) ) == selectedKiCadLayerName )
            return ToLAYER_ID( layer );
    }

    return PCB_LAYER_ID::UNDEFINED_LAYER;
}


PCB_LAYER_ID DIALOG_MAP_LAYERS::GetAutoMatchLayerID( const wxString& aInputLayerName )
{
    wxString pureInputLayerName = UnwrapRequired( aInputLayerName );

    for( const INPUT_LAYER_DESC& inputLayerDesc : m_input_layers )
    {
        if( inputLayerDesc.Name == pureInputLayerName
                && inputLayerDesc.AutoMapLayer != PCB_LAYER_ID::UNSELECTED_LAYER )
        {
            return inputLayerDesc.AutoMapLayer;
        }
    }

    return PCB_LAYER_ID::UNDEFINED_LAYER;
}


void DIALOG_MAP_LAYERS::AddMappings()
{
    PCB_LAYER_ID selectedKiCadLayerID = GetSelectedLayerID();

    if( selectedKiCadLayerID == PCB_LAYER_ID::UNDEFINED_LAYER )
        return;

    // Now iterate through each selected layer in the unmatched layers list
    int        itemIndex = -1;
    wxArrayInt rowsToDelete;

    while( ( itemIndex = m_unmatched_layers_list->GetNextItem( itemIndex, wxLIST_NEXT_ALL,
                                                               wxLIST_STATE_SELECTED ) )
            != wxNOT_FOUND )
    {
        wxString selectedLayerName = m_unmatched_layers_list->GetItemText( itemIndex );
        wxString kiName = KiCadLayerName( selectedKiCadLayerID );

        // add layer pair to the GUI list and also to the map
        wxString sourceName = UnwrapRequired( selectedLayerName );
        long     newItemIndex = m_matched_layers_list->InsertItem( 0, ImportedLayerName( sourceName ) );
        m_matched_layers_list->SetItem( newItemIndex, 1, kiName );
        m_matched_layers_list->SetItemData( newItemIndex, m_sourceNames.size() );
        m_sourceNames.push_back( sourceName );

        m_matched_layers_map.insert( { sourceName, selectedKiCadLayerID } );

        // remove selected layer from vector and also GUI list
        for( auto iter = m_unmatched_layer_names.begin(); iter != m_unmatched_layer_names.end();
                ++iter )
        {
            if( *iter == selectedLayerName )
            {
                m_unmatched_layer_names.erase( iter );
                break;
            }
        }

        rowsToDelete.Add( itemIndex );
    }

    DeleteListItems( rowsToDelete, m_unmatched_layers_list );

    UpdateNameWarning();

    // Auto select the first item to improve ease-of-use
    if( m_unmatched_layers_list->GetItemCount() > 0 )
        m_unmatched_layers_list->SetItemState( 0, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );
}


void DIALOG_MAP_LAYERS::RemoveMappings( int aStatus )
{
    wxArrayInt rowsToDelete;
    int        itemIndex = -1;

    while( ( itemIndex = m_matched_layers_list->GetNextItem( itemIndex, wxLIST_NEXT_ALL, aStatus ) ) != wxNOT_FOUND )
    {
        wxString                pureSelectedLayerName = SourceNameForRow( itemIndex );
        const INPUT_LAYER_DESC* layerDesc = GetLayerDescription( pureSelectedLayerName );
        wxString                selectedLayerName =
                layerDesc && layerDesc->Required ? WrapRequired( pureSelectedLayerName ) : pureSelectedLayerName;

        wxCHECK( m_matched_layers_map.find( pureSelectedLayerName ) != m_matched_layers_map.end(),
                /*void*/ );

        m_matched_layers_map.erase( pureSelectedLayerName );
        rowsToDelete.Add( itemIndex );

        m_unmatched_layers_list->InsertItem( 0, selectedLayerName );
        m_unmatched_layer_names.push_back( selectedLayerName );
    }

    DeleteListItems( rowsToDelete, m_matched_layers_list );

    UpdateNameWarning();
}


void DIALOG_MAP_LAYERS::DeleteListItems( const wxArrayInt& aRowsToDelete, wxListCtrl* aListCtrl )
{
    for( long n = (long) aRowsToDelete.GetCount() - 1; 0 <= n; n-- )
        aListCtrl->DeleteItem( aRowsToDelete[n] );
}


void DIALOG_MAP_LAYERS::AutoMatchLayers()
{
    int        itemIndex = -1;
    wxArrayInt rowsToDelete;

    while( ( itemIndex = m_unmatched_layers_list->GetNextItem( itemIndex, wxLIST_NEXT_ALL,
                                                               wxLIST_STATE_DONTCARE ) ) != wxNOT_FOUND )
    {
        wxString     layerName = m_unmatched_layers_list->GetItemText( itemIndex );
        PCB_LAYER_ID autoMatchLayer = GetAutoMatchLayerID( layerName );

        if( autoMatchLayer == PCB_LAYER_ID::UNDEFINED_LAYER )
            continue;

        wxString kiName = KiCadLayerName( autoMatchLayer );

        // add layer pair to the GUI list and also to the map
        wxString sourceName = UnwrapRequired( layerName );
        long     newItemIndex = m_matched_layers_list->InsertItem( 0, ImportedLayerName( sourceName ) );
        m_matched_layers_list->SetItem( newItemIndex, 1, kiName );
        m_matched_layers_list->SetItemData( newItemIndex, m_sourceNames.size() );
        m_sourceNames.push_back( sourceName );

        m_matched_layers_map.insert( { sourceName, autoMatchLayer } );

        // remove selected layer from vector and also GUI list
        for( auto iter = m_unmatched_layer_names.begin(); iter != m_unmatched_layer_names.end(); ++iter )
        {
            if( *iter == layerName )
            {
                m_unmatched_layer_names.erase( iter );
                break;
            }
        }

        rowsToDelete.Add( itemIndex );
    }

    DeleteListItems( rowsToDelete, m_unmatched_layers_list );

    UpdateNameWarning();
}


void DIALOG_MAP_LAYERS::OnAutoMatchLayersClicked( wxCommandEvent& event )
{
    AutoMatchLayers();
}


DIALOG_MAP_LAYERS::DIALOG_MAP_LAYERS( wxWindow* aParent, const std::vector<INPUT_LAYER_DESC>& aLayerDesc,
                                      const BOARD* aValidationBoard, bool aImportNames ) :
        DIALOG_IMPORTED_LAYERS_BASE( aParent ),
        m_validationBoard( aValidationBoard ),
        m_importNames( aImportNames )
{
    if( PCB_BASE_FRAME* frame = dynamic_cast<PCB_BASE_FRAME*>( aParent ) )
        m_board = frame->GetBoard();

    LSET kiCadLayers;

    // Read in the input layers
    for( const INPUT_LAYER_DESC& inLayer : aLayerDesc )
    {
        m_input_layers.push_back( inLayer );
        wxString layerName = inLayer.Required ? WrapRequired( inLayer.Name ) : inLayer.Name;
        m_unmatched_layer_names.push_back( layerName );
        kiCadLayers |= inLayer.PermittedLayers;
    }

    int maxTextWidth = GetTextExtent( _( "Imported Layer" ) ).x;

    for( const INPUT_LAYER_DESC& layer : m_input_layers )
        maxTextWidth = std::max( maxTextWidth, GetTextExtent( layer.Name ).x );

    // Initialize columns in the wxListCtrl elements:
    wxListItem importedLayersHeader;
    importedLayersHeader.SetId( 0 );
    importedLayersHeader.SetText( _( "Imported Layer" ) );
    importedLayersHeader.SetWidth( maxTextWidth + 15 );
    m_unmatched_layers_list->InsertColumn( 0, importedLayersHeader );

    int kicadMaxTextWidth = GetTextExtent( wxT( "User.Drawings" ) ).x;

    wxListItem kicadLayersHeader;
    kicadLayersHeader.SetId( 0 );
    kicadLayersHeader.SetText( _( "KiCad Layer" ) );
    kicadLayersHeader.SetWidth( kicadMaxTextWidth + 5 );
    m_kicad_layers_list->InsertColumn( 0, kicadLayersHeader );

    kicadLayersHeader.SetId( 1 );
    importedLayersHeader.SetWidth( maxTextWidth + 15 );
    importedLayersHeader.SetText( _( "Layer name" ) );
    kicadLayersHeader.SetWidth( kicadMaxTextWidth + 5 );
    m_matched_layers_list->InsertColumn( 0, importedLayersHeader );
    m_matched_layers_list->InsertColumn( 1, kicadLayersHeader );

    if( aImportNames )
    {
        m_matched_layers_list->SetWindowStyleFlag( m_matched_layers_list->GetWindowStyleFlag() | wxLC_EDIT_LABELS );
        m_matched_layers_list->Bind( wxEVT_LIST_END_LABEL_EDIT, &DIALOG_MAP_LAYERS::OnNameEdited, this );
        m_matched_layers_list->Bind( wxEVT_LIST_ITEM_SELECTED,
                                     [this]( wxListEvent& )
                                     {
                                         UpdateNameWarning();
                                     } );

        wxSizer* bottomSizer = bSizerMain->GetItem( 1 )->GetSizer();
        m_renameButton = new wxButton( this, wxID_ANY, _( "Rename Selected..." ) );
        bottomSizer->Insert( 1, m_renameButton, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5 );
        m_renameButton->Bind( wxEVT_BUTTON, &DIALOG_MAP_LAYERS::OnRenameClicked, this );

        m_nameWarning = new WX_INFOBAR( this );
        m_nameWarning->Hide();
        bSizerMain->Insert( 0, m_nameWarning, 0, wxEXPAND | wxALL, 5 );

        m_cbKeepKiCadLayerNames->Bind( wxEVT_CHECKBOX,
                                       [this]( wxCommandEvent& )
                                       {
                                           UpdateNameWarning();
                                       } );
    }

    // Load the input layer list to unmatched layers
    int row = 0;

    for( const wxString& importedLayerName : m_unmatched_layer_names )
    {
        wxListItem item;
        item.SetId( row );
        item.SetText( importedLayerName );
        m_unmatched_layers_list->InsertItem( item );
        ++row;
    }

    // Auto select the first item to improve ease-of-use
    m_unmatched_layers_list->SetItemState( 0, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );

    // Load the KiCad Layer names
    row = 0;
    LSEQ kicadLayersSeq = kiCadLayers.UIOrder();

    for( PCB_LAYER_ID layer : kicadLayersSeq )
    {
        wxString kiName = KiCadLayerName( layer );

        wxListItem item;
        item.SetId( row );
        item.SetText( kiName );
        m_kicad_layers_list->InsertItem( item );
        ++row;
    }

    // Auto select the first item to improve ease-of-use
    m_kicad_layers_list->SetItemState( 0, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );

    if( auto* frame = dynamic_cast<PCB_BASE_FRAME*>( m_parentFrame ) )
        m_cbKeepKiCadLayerNames->SetValue( frame->GetPcbNewSettings()->m_ImportKeepKiCadLayerNames );

    UpdateNameWarning();

    SetupStandardButtons();
    m_sdbSizerOK->SetDefault();
    m_sdbSizerOK->SetFocus();

    Fit();
    finishDialogSettings();
}


std::vector<wxString> DIALOG_MAP_LAYERS::GetUnmappedRequiredLayers() const
{
    std::vector<wxString> unmappedLayers;

    for( const wxString& layerName : m_unmatched_layer_names )
    {
        const INPUT_LAYER_DESC* layerDesc = GetLayerDescription( layerName );
        wxASSERT_MSG( layerDesc != nullptr, wxT( "Expected to find layer description" ) );

        if( layerDesc->Required )
            unmappedLayers.push_back( layerDesc->Name );
    }

    return unmappedLayers;
}


std::map<wxString, wxString> DIALOG_MAP_LAYERS::GetInvalidLayerNames() const
{
    std::map<wxString, wxString>                          invalidNames;
    std::map<wxString, std::pair<wxString, PCB_LAYER_ID>> usedNames;

    if( m_matched_layers_map.empty() )
    {
        for( const INPUT_LAYER_DESC& layer : m_input_layers )
        {
            wxString error = LayerNameError( layer.Name );

            if( !error.IsEmpty() )
                invalidNames[layer.Name] = error;
        }

        return invalidNames;
    }

    if( m_validationBoard )
    {
        for( PCB_LAYER_ID layer : m_validationBoard->GetEnabledLayers() )
        {
            bool mapped = false;

            for( const auto& [sourceName, target] : m_matched_layers_map )
            {
                if( target == layer )
                {
                    mapped = true;
                    break;
                }
            }

            if( !mapped )
                usedNames.emplace( m_validationBoard->GetLayerName( layer ), std::make_pair( wxString(), layer ) );
        }
    }

    for( const auto& [sourceName, layer] : m_matched_layers_map )
    {
        wxString name = ImportedLayerName( sourceName );

        if( layer == UNDEFINED_LAYER )
            continue;

        wxString error = LayerNameError( name );

        if( !error.IsEmpty() )
            invalidNames[sourceName] = error;

        auto [it, inserted] = usedNames.emplace( name, std::make_pair( sourceName, layer ) );

        if( !inserted && it->second.second != layer )
        {
            wxString message = wxString::Format( _( "Layer name '%s' is used more than once." ), name );
            invalidNames[sourceName] = message;

            if( !it->second.first.IsEmpty() )
                invalidNames[it->second.first] = message;
        }
    }

    return invalidNames;
}


wxString DIALOG_MAP_LAYERS::SourceNameForRow( long aRow ) const
{
    size_t index = static_cast<size_t>( m_matched_layers_list->GetItemData( aRow ) );
    return index < m_sourceNames.size() ? m_sourceNames[index] : wxString();
}


wxString DIALOG_MAP_LAYERS::ImportedLayerName( const wxString& aSourceName ) const
{
    auto it = m_renamedLayerNames.find( aSourceName );
    return it != m_renamedLayerNames.end() ? it->second : aSourceName;
}


void DIALOG_MAP_LAYERS::UpdateNameWarning()
{
    if( !m_nameWarning )
        return;

    std::map<wxString, wxString> invalidNames = GetInvalidLayerNames();

    bool keepKiCadNames = m_cbKeepKiCadLayerNames->GetValue();
    m_renameButton->Enable( !keepKiCadNames && m_matched_layers_list->GetItemCount() > 0 );
    bool     darkMode = wxSystemSettings::GetAppearance().IsDark();
    wxColour invalidColour = darkMode ? wxColour( 255, 145, 145 ) : wxColour( 165, 0, 0 );
    wxColour normalColour = m_matched_layers_list->GetForegroundColour();

    for( long row = 0; row < m_matched_layers_list->GetItemCount(); ++row )
    {
        bool invalid = !keepKiCadNames && invalidNames.contains( SourceNameForRow( row ) );
        m_matched_layers_list->SetItemTextColour( row, invalid ? invalidColour : normalColour );
    }

    if( keepKiCadNames || invalidNames.empty() )
    {
        m_nameWarning->Dismiss();
        return;
    }

    if( m_matched_layers_map.empty() )
    {
        m_nameWarning->ShowMessage(
                wxString::Format( _( "%d imported layer names would be invalid in KiCad. %s Click Auto-Match "
                                     "Layers or map the layers, then rename the invalid names." ),
                                  static_cast<int>( invalidNames.size() ), invalidNames.begin()->second ),
                wxICON_WARNING );
        return;
    }

    auto selectedError = invalidNames.end();
    long selectedRow = m_matched_layers_list->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );

    if( selectedRow != wxNOT_FOUND )
        selectedError = invalidNames.find( SourceNameForRow( selectedRow ) );

    if( selectedError == invalidNames.end() )
        selectedError = invalidNames.begin();

    m_nameWarning->ShowMessage( wxString::Format( _( "%d invalid layer names. %s Select a red name and click "
                                                     "Rename Selected, or select 'Keep KiCad layer names'." ),
                                                  static_cast<int>( invalidNames.size() ), selectedError->second ),
                                wxICON_WARNING );
}


void DIALOG_MAP_LAYERS::OnRenameClicked( wxCommandEvent& event )
{
    long row = m_matched_layers_list->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );

    if( row == wxNOT_FOUND )
    {
        std::map<wxString, wxString> invalidNames = GetInvalidLayerNames();

        for( long candidate = 0; candidate < m_matched_layers_list->GetItemCount(); ++candidate )
        {
            if( invalidNames.contains( SourceNameForRow( candidate ) ) )
            {
                row = candidate;
                break;
            }
        }
    }

    if( row != wxNOT_FOUND )
        m_matched_layers_list->EditLabel( row );
}


void DIALOG_MAP_LAYERS::OnNameEdited( wxListEvent& event )
{
    if( event.IsEditCancelled() )
        return;

    wxString     sourceName = SourceNameForRow( event.GetIndex() );
    PCB_LAYER_ID layer = m_matched_layers_map.at( sourceName );

    for( long row = 0; row < m_matched_layers_list->GetItemCount(); ++row )
    {
        wxString rowSourceName = SourceNameForRow( row );

        if( ( layer == UNDEFINED_LAYER && row != event.GetIndex() )
            || m_matched_layers_map.at( rowSourceName ) != layer )
            continue;

        m_renamedLayerNames[rowSourceName] = event.GetLabel();

        if( row != event.GetIndex() )
            m_matched_layers_list->SetItemText( row, event.GetLabel() );
    }

    UpdateNameWarning();
}


bool DIALOG_MAP_LAYERS::TransferDataFromWindow()
{
    // If the user accepted with no mappings, apply suggested AutoMapLayer values.
    if( m_matched_layers_map.empty() )
        AutoMatchLayers();

    if( !GetUnmappedRequiredLayers().empty() )
    {
        wxMessageBox( _( "All required layers (marked with '*') must be matched. Please click "
                         "'Auto-Match Layers' to automatically match the remaining layers" ),
                      _( "Unmatched Layers" ), wxICON_ERROR | wxOK );
        return false;
    }

    if( m_importNames && !m_cbKeepKiCadLayerNames->GetValue() && !GetInvalidLayerNames().empty() )
    {
        UpdateNameWarning();
        return false;
    }

    return true;
}


std::map<wxString, PCB_LAYER_ID> DIALOG_MAP_LAYERS::RunModal( wxWindow*                            aParent,
                                                              const std::vector<INPUT_LAYER_DESC>& aLayerDesc,
                                                              std::map<PCB_LAYER_ID, wxString>*    aRenamedLayers,
                                                              const BOARD*                         aValidationBoard )
{
    DIALOG_MAP_LAYERS dlg( aParent, aLayerDesc, aValidationBoard, aRenamedLayers != nullptr );

    // Validation lives in TransferDataFromWindow(). wxWidgets 3.3.4 cannot re-show a closed dialog.
    if( dlg.ShowModal() != wxID_OK )
    {
        if( aRenamedLayers )
            THROW_IO_CANCELLED();

        return {};
    }

    if( auto* frame = dynamic_cast<PCB_BASE_FRAME*>( dlg.m_parentFrame ) )
        frame->GetPcbNewSettings()->m_ImportKeepKiCadLayerNames = dlg.m_cbKeepKiCadLayerNames->GetValue();

    if( aRenamedLayers && !dlg.m_cbKeepKiCadLayerNames->GetValue() )
    {
        for( const auto& [sourceName, layer] : dlg.m_matched_layers_map )
        {
            auto it = dlg.m_renamedLayerNames.find( sourceName );

            if( it != dlg.m_renamedLayerNames.end() && layer != UNDEFINED_LAYER )
                ( *aRenamedLayers )[layer] = it->second;
        }
    }

    return dlg.m_matched_layers_map;
}
