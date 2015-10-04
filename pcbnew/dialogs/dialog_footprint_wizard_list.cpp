/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012-2014 Miguel Angel Ajo <miguelangel@nbee.es>
 * Copyright (C) 1992-2014 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file dialog_footprint_wizard_list.cpp
 */

#include <wx/grid.h>

#include <fctsys.h>
#include <pcbnew.h>
#include <kiface_i.h>
#include <dialog_footprint_wizard_list.h>
#include <class_footprint_wizard.h>

#define ROW_NAME 0
#define ROW_DESCR 1
#define FPWIZARTDLIST_HEIGHT_KEY wxT( "FpWizardListHeight" )
#define FPWIZARTDLIST_WIDTH_KEY  wxT( "FpWizardListWidth" )

DIALOG_FOOTPRINT_WIZARD_LIST::DIALOG_FOOTPRINT_WIZARD_LIST( wxWindow* aParent )
    : DIALOG_FOOTPRINT_WIZARD_LIST_BASE( aParent )
{
    int n_wizards = FOOTPRINT_WIZARDS::GetWizardsCount();
    m_config = Kiface().KifaceSettings();

    // Current wizard selection, empty or first
    m_footprintWizard = NULL;

    if( n_wizards )
        m_footprintWizard = FOOTPRINT_WIZARDS::GetWizard( 0 );

    // Choose selection mode and insert the needed rows

    m_footprintWizardsGrid->SetSelectionMode( wxGrid::wxGridSelectRows );
    m_footprintWizardsGrid->InsertRows( 0, n_wizards, true );

    // Put all wizards in the list
    for( int i=0; i<n_wizards; i++ )
    {
        FOOTPRINT_WIZARD *wizard = FOOTPRINT_WIZARDS::GetWizard( i );
        wxString name = wizard->GetName();
        wxString description = wizard->GetDescription();
        wxString image = wizard->GetImage();

        m_footprintWizardsGrid->SetCellValue( i, ROW_NAME, name );
        m_footprintWizardsGrid->SetCellValue( i, ROW_DESCR, description );

    }

    // Select the first row
    m_footprintWizardsGrid->ClearSelection();
    m_footprintWizardsGrid->SelectRow( 0, false );

    if( m_config )
    {
        wxSize size;
        m_config->Read( FPWIZARTDLIST_WIDTH_KEY, &size.x, -1 );
        m_config->Read( FPWIZARTDLIST_HEIGHT_KEY, &size.y, -1 );
        SetSize( size );
    }

    Center();
}


DIALOG_FOOTPRINT_WIZARD_LIST::~DIALOG_FOOTPRINT_WIZARD_LIST()
{
    if( m_config && !IsIconized() )
    {
        m_config->Write( FPWIZARTDLIST_WIDTH_KEY, GetSize().x );
        m_config->Write( FPWIZARTDLIST_HEIGHT_KEY, GetSize().y );
    }
}



void DIALOG_FOOTPRINT_WIZARD_LIST::OnCellWizardClick( wxGridEvent& event )
{
    int click_row = event.GetRow();
    m_footprintWizard = FOOTPRINT_WIZARDS::GetWizard( click_row );
    m_footprintWizardsGrid->SelectRow( event.GetRow(), false );
}


FOOTPRINT_WIZARD* DIALOG_FOOTPRINT_WIZARD_LIST::GetWizard()
{
    return m_footprintWizard;
}
