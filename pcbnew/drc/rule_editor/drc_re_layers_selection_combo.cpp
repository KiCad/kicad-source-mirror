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


#include "drc_re_layers_selection_combo.h"


DRC_RE_LAYER_SELECTION_COMBO::DRC_RE_LAYER_SELECTION_COMBO( wxWindow* aParent,
        const std::vector<PCB_LAYER_ID>& aLayerIDs,
        const std::function<wxString( PCB_LAYER_ID )>& aNameGetter ) :
        wxComboCtrl( aParent, wxID_ANY )
{
    m_popup = new DRC_RE_LAYER_SELECTION_CHOICE_POPUP();
    SetPopupControl( m_popup );

    m_layerIDs = aLayerIDs;
    m_nameGetter = aNameGetter;

    m_popup->PopulateWithLayerIDs( m_layerIDs, m_nameGetter );

    Bind( wxEVT_COMBOBOX_CLOSEUP, &DRC_RE_LAYER_SELECTION_COMBO::onPopupClose, this );
    Bind( wxEVT_KEY_DOWN, &DRC_RE_LAYER_SELECTION_COMBO::onKeyDown, this );
    Bind( wxEVT_LEFT_DOWN, &DRC_RE_LAYER_SELECTION_COMBO::onMouseClick, this );
}


DRC_RE_LAYER_SELECTION_COMBO::~DRC_RE_LAYER_SELECTION_COMBO()
{
    Unbind( wxEVT_COMBOBOX_CLOSEUP, &DRC_RE_LAYER_SELECTION_COMBO::onPopupClose, this );
    Unbind( wxEVT_KEY_DOWN, &DRC_RE_LAYER_SELECTION_COMBO::onKeyDown, this );
    Unbind( wxEVT_LEFT_DOWN, &DRC_RE_LAYER_SELECTION_COMBO::onMouseClick, this );
}


wxString DRC_RE_LAYER_SELECTION_COMBO::GetSelectedItemsString()
{
    return m_popup->GetSelectedItemsString();
}


std::vector<PCB_LAYER_ID> DRC_RE_LAYER_SELECTION_COMBO::GetSelectedLayers()
{
    return m_popup->GetSelectedLayers( m_layerIDs, m_nameGetter );
}


void DRC_RE_LAYER_SELECTION_COMBO::SetItemsSelected(
        const std::vector<PCB_LAYER_ID>& aSelectedLayers )
{
    m_popup->SetSelections( aSelectedLayers, m_nameGetter );

    SetValue( m_popup->GetSelectedItemsString() );
}


void DRC_RE_LAYER_SELECTION_COMBO::onPopupClose( wxCommandEvent& aEvent )
{
    SetValue( m_popup->GetSelectedItemsString() );
    aEvent.Skip();
}


void DRC_RE_LAYER_SELECTION_COMBO::onKeyDown( wxKeyEvent& aEvent )
{
    aEvent.Skip( false );
}


void DRC_RE_LAYER_SELECTION_COMBO::onMouseClick( wxMouseEvent& aEvent )
{
    if( !IsPopupShown() )
    {
        ShowPopup();
    }

    aEvent.Skip( false );
}