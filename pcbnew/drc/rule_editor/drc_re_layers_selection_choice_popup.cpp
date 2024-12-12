/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <wx/tokenzr.h>

#include "drc_re_layers_selection_choice_popup.h"


void DRC_RE_LAYER_SELECTION_CHOICE_POPUP::Init()
{
    m_checkListBox = nullptr;
}


bool DRC_RE_LAYER_SELECTION_CHOICE_POPUP::Create( wxWindow* aParent )
{
    m_checkListBox = new wxCheckListBox( aParent, wxID_ANY );
    return true;
}


wxWindow* DRC_RE_LAYER_SELECTION_CHOICE_POPUP::GetControl()
{
    return m_checkListBox;
}


void DRC_RE_LAYER_SELECTION_CHOICE_POPUP::SetStringValue( const wxString& aValue )
{
    m_selectedItemsString = aValue;
}


wxString DRC_RE_LAYER_SELECTION_CHOICE_POPUP::GetStringValue() const
{
    return m_selectedItemsString;
}


void DRC_RE_LAYER_SELECTION_CHOICE_POPUP::populate( const wxArrayString& aItems )
{
    m_checkListBox->Clear();
    m_checkListBox->Append( aItems );
}


wxString DRC_RE_LAYER_SELECTION_CHOICE_POPUP::GetSelectedItemsString()
{
    wxArrayInt checkedItems;
    wxString   selectedItems;

    m_checkListBox->GetCheckedItems( checkedItems );

    for( size_t i = 0; i < checkedItems.GetCount(); ++i )
    {
        if( !selectedItems.IsEmpty() )
            selectedItems += ", ";

        selectedItems += m_checkListBox->GetString( checkedItems[i] );
    }

    SetStringValue( selectedItems );

    return selectedItems;
}


std::vector<PCB_LAYER_ID> DRC_RE_LAYER_SELECTION_CHOICE_POPUP::GetSelectedLayers(
        const std::vector<PCB_LAYER_ID>& aAllLayerIds,
        const std::function<wxString( PCB_LAYER_ID )>& aNameGetter )
{
    wxString input = GetSelectedItemsString();
    input.Replace( ", ", "," );

    std::vector<PCB_LAYER_ID> selectedLayerIds;

    for( const auto& layerID : aAllLayerIds )
    {
        wxString searchString = aNameGetter( layerID );
        bool     found = ( "," + input + "," ).Contains( "," + searchString + "," );

        if( found )
        {
            selectedLayerIds.push_back( layerID );
        }
    }

    return selectedLayerIds;
}


void DRC_RE_LAYER_SELECTION_CHOICE_POPUP::SetSelections(
        const std::vector<PCB_LAYER_ID>& aLayerIDs,
        const std::function<wxString( PCB_LAYER_ID )>& aNameGetter )
{
    wxArrayString names;

    for( const auto& layerID : aLayerIDs )
    {
        names.Add( aNameGetter( layerID ) );
    }

    if( m_checkListBox )
    {
        for( const auto& item : names )
        {
            int index = m_checkListBox->FindString( item );

            if( index != wxNOT_FOUND )
            {
                m_checkListBox->Check( index );
            }
        }
    }
}


void DRC_RE_LAYER_SELECTION_CHOICE_POPUP::PopulateWithLayerIDs(
        const std::vector<PCB_LAYER_ID>& aLayerIDs,
        const std::function<wxString( PCB_LAYER_ID )>& aNameGetter )
{
    wxArrayString names;

    for( const auto& layerID : aLayerIDs )
    {
        names.Add( aNameGetter( layerID ) );
    }

    populate( names );
}