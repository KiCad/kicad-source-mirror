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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <layer_ids.h>
#include <lseq.h>
#include <dialog_map_layers.h>

#include <wx/msgdlg.h>


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
        if( LayerName( ToLAYER_ID( layer ) ) == selectedKiCadLayerName )
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
        wxString kiName            = LayerName( selectedKiCadLayerID );

        // add layer pair to the GUI list and also to the map
        long newItemIndex = m_matched_layers_list->InsertItem( 0, selectedLayerName );
        m_matched_layers_list->SetItem( newItemIndex, 1, kiName );

        m_matched_layers_map.insert(
                { UnwrapRequired( selectedLayerName ), selectedKiCadLayerID } );

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

    // Auto select the first item to improve ease-of-use
    m_unmatched_layers_list->SetItemState( 0, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );
}


void DIALOG_MAP_LAYERS::RemoveMappings( int aStatus )
{
    wxArrayInt rowsToDelete;
    int        itemIndex = -1;

    while( ( itemIndex = m_matched_layers_list->GetNextItem( itemIndex, wxLIST_NEXT_ALL, aStatus ) ) != wxNOT_FOUND )
    {
        wxString selectedLayerName     = m_matched_layers_list->GetItemText( itemIndex, 0 );
        wxString pureSelectedLayerName = UnwrapRequired( selectedLayerName );

        wxCHECK( m_matched_layers_map.find( pureSelectedLayerName ) != m_matched_layers_map.end(),
                /*void*/ );

        m_matched_layers_map.erase( pureSelectedLayerName );
        rowsToDelete.Add( itemIndex );

        m_unmatched_layers_list->InsertItem( 0, selectedLayerName );
        m_unmatched_layer_names.push_back( selectedLayerName );
    }

    DeleteListItems( rowsToDelete, m_matched_layers_list );
}


void DIALOG_MAP_LAYERS::DeleteListItems( const wxArrayInt& aRowsToDelete, wxListCtrl* aListCtrl )
{
    for( long n = (long) aRowsToDelete.GetCount() - 1; 0 <= n; n-- )
        aListCtrl->DeleteItem( aRowsToDelete[n] );
}


void DIALOG_MAP_LAYERS::OnAutoMatchLayersClicked( wxCommandEvent& event )
{
    // Iterate through each selected layer in the unmatched layers list
    int        itemIndex = -1;
    wxArrayInt rowsToDelete;

    while( ( itemIndex = m_unmatched_layers_list->GetNextItem( itemIndex, wxLIST_NEXT_ALL,
                                                               wxLIST_STATE_DONTCARE ) ) != wxNOT_FOUND )
    {
        wxString     layerName      = m_unmatched_layers_list->GetItemText( itemIndex );
        PCB_LAYER_ID autoMatchLayer = GetAutoMatchLayerID( layerName );

        if( autoMatchLayer == PCB_LAYER_ID::UNDEFINED_LAYER )
            continue;

        wxString kiName = LayerName( autoMatchLayer );

        // add layer pair to the GUI list and also to the map
        long newItemIndex = m_matched_layers_list->InsertItem( 0, layerName );
        m_matched_layers_list->SetItem( newItemIndex, 1, kiName );

        m_matched_layers_map.insert( { UnwrapRequired( layerName ), autoMatchLayer } );

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
}


DIALOG_MAP_LAYERS::DIALOG_MAP_LAYERS( wxWindow* aParent, const std::vector<INPUT_LAYER_DESC>& aLayerDesc ) :
        DIALOG_IMPORTED_LAYERS_BASE( aParent )
{
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
    kicadLayersHeader.SetWidth( kicadMaxTextWidth + 5 );
    m_matched_layers_list->InsertColumn( 0, importedLayersHeader );
    m_matched_layers_list->InsertColumn( 1, kicadLayersHeader );

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
        wxString kiName = LayerName( layer );

        wxListItem item;
        item.SetId( row );
        item.SetText( kiName );
        m_kicad_layers_list->InsertItem( item );
        ++row;
    }

    // Auto select the first item to improve ease-of-use
    m_kicad_layers_list->SetItemState( 0, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );

    SetupStandardButtons();

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


std::map<wxString, PCB_LAYER_ID>
DIALOG_MAP_LAYERS::RunModal( wxWindow* aParent, const std::vector<INPUT_LAYER_DESC>& aLayerDesc )
{
    DIALOG_MAP_LAYERS dlg( aParent, aLayerDesc );
    bool              dataOk = false;

    while( !dataOk )
    {
        dlg.ShowModal();

        if( dlg.GetUnmappedRequiredLayers().size() > 0 )
        {
            wxMessageBox( _( "All required layers (marked with '*') must be matched. Please click "
                             "'Auto-Match Layers' to automatically match the remaining layers" ),
                          _( "Unmatched Layers" ), wxICON_ERROR | wxOK );
        }
        else
        {
            dataOk = true;
        }
    }

    return dlg.m_matched_layers_map;
}
