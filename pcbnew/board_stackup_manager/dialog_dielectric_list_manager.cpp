/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "dialog_dielectric_list_manager.h"
#include <wx/msgdlg.h>

DIALOG_DIELECTRIC_MATERIAL::DIALOG_DIELECTRIC_MATERIAL( wxWindow* aParent,
                                                        DIELECTRIC_SUBSTRATE_LIST& aMaterialList ) :
        DIALOG_DIELECTRIC_MATERIAL_BASE( aParent ),
        m_materialList( aMaterialList )
{
    initMaterialList();
    SetupStandardButtons();
}

DIALOG_DIELECTRIC_MATERIAL::~DIALOG_DIELECTRIC_MATERIAL()
{
}


bool DIALOG_DIELECTRIC_MATERIAL::TransferDataFromWindow()
{
    // Validate double values from wxTextCtrl
    double dummy;

    if( !m_tcEpsilonR->GetValue().ToDouble( &dummy ) || dummy < 0.0 )
    {
        wxMessageBox( _( "Incorrect value for Epsilon R" ) );
        return false;
    }

    if( !m_tcLossTg->GetValue().ToDouble( &dummy ) || dummy < 0.0 )
    {
        wxMessageBox( _( "Incorrect value for Loss Tangent" ) );
        return false;
    }

    return true;
}


bool DIALOG_DIELECTRIC_MATERIAL::TransferDataToWindow()
{
    // Init m_tcEpsilonR and m_tcLossTg to a dummy (default) value
    DIELECTRIC_SUBSTRATE dummy;
    dummy.m_EpsilonR = 1.0;
    dummy.m_LossTangent = 0.0;

    m_tcEpsilonR->SetValue( dummy.FormatEpsilonR() );
    m_tcLossTg->SetValue( dummy.FormatLossTangent() );

    return true;
}


DIELECTRIC_SUBSTRATE DIALOG_DIELECTRIC_MATERIAL::GetSelectedSubstrate()
{
    DIELECTRIC_SUBSTRATE substrate;
    double               tmp;

    // return the selected/created substrate. A empty substrate can be returned
    substrate.m_Name = m_tcMaterial->GetValue();

    m_tcEpsilonR->GetValue().ToDouble( &tmp );
    substrate.m_EpsilonR = tmp;

    m_tcLossTg->GetValue().ToDouble( &tmp );
    substrate.m_LossTangent = tmp;

    return substrate;
}


void DIALOG_DIELECTRIC_MATERIAL::initMaterialList()
{
    m_lcMaterials->AppendColumn( _( "Material" ) );
    m_lcMaterials->AppendColumn( _( "Epsilon R" ) );
    m_lcMaterials->AppendColumn( _( "Loss Tan" ) );

    m_lcMaterials->SetColumnWidth( 0, m_lcMaterials->GetColumnWidth( 1 ) * 3 / 2 );

    // Fills m_lcMaterials with available materials
    // The first item is expected a not specified material
    // Other names are proper nouns, and are not translated
    for( int idx = 0; idx < m_materialList.GetCount(); ++idx )
    {
        DIELECTRIC_SUBSTRATE* item = m_materialList.GetSubstrate( idx );

        long tmp = m_lcMaterials->InsertItem( idx, idx == 0 ? wxGetTranslation( item->m_Name )
                                                            : item->m_Name );

        m_lcMaterials->SetItemData( tmp, idx );
        m_lcMaterials->SetItem( tmp, 1, item->FormatEpsilonR() );
        m_lcMaterials->SetItem( tmp, 2, item->FormatLossTangent() );
    }
}


void DIALOG_DIELECTRIC_MATERIAL::onListItemSelected( wxListEvent& event )
{
    int idx = event.GetIndex();

    if( idx < 0 )
        return;

    if( idx == 0 )
        m_tcMaterial->SetValue( wxGetTranslation( m_materialList.GetSubstrate( 0 )->m_Name ) );
    else
        m_tcMaterial->SetValue( m_materialList.GetSubstrate( idx )->m_Name );

    m_tcEpsilonR->SetValue( m_materialList.GetSubstrate( idx )->FormatEpsilonR() );
    m_tcLossTg->SetValue( m_materialList.GetSubstrate( idx )->FormatLossTangent() );
}


void DIALOG_DIELECTRIC_MATERIAL::onListKeyDown( wxListEvent& event )
{
    int idx = event.GetIndex();

    switch( event.GetKeyCode() )
    {
    case WXK_DELETE:
        if( idx >= 0 )
        {
            m_lcMaterials->DeleteItem( idx );
            m_materialList.DeleteSubstrate( idx );

            // Get the new material information for the next item in the list
            // (or last if the deleted item was the last item)
            int next = ( idx < m_materialList.GetCount() ) ? idx : idx - 1;

            m_lcMaterials->SetItemState( next, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );
            m_lcMaterials->SetItemState( next, wxLIST_STATE_FOCUSED, wxLIST_STATE_FOCUSED );
            m_lcMaterials->EnsureVisible( next );
        }

        break;

    default:
        event.Skip();
    }
}
