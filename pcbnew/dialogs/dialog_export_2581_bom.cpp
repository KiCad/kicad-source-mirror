/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include "dialog_export_2581_bom.h"

#include <set>

#include <board.h>
#include <footprint.h>
#include <pcb_field.h>


DIALOG_EXPORT_2581_BOM::DIALOG_EXPORT_2581_BOM( wxWindow* aParent, BOARD* aBoard,
                                                const IPC2581_BOM_FIELDS& aFields ) :
        DIALOG_EXPORT_2581_BOM_BASE( aParent ),
        m_fields( aFields )
{
    m_textDistributor->SetSize( m_choiceDistPN->GetSize() );

    std::set<wxString> options;

    for( FOOTPRINT* fp : aBoard->Footprints() )
    {
        for( PCB_FIELD* field : fp->GetFields() )
        {
            wxCHECK2( field, continue );

            options.insert( field->GetName() );
        }
    }

    std::vector<wxString> items( options.begin(), options.end() );
    m_oemRef->Append( items );
    m_choiceMPN->Append( items );
    m_choiceMfg->Append( items );
    m_choiceDistPN->Append( items );

    SetupStandardButtons();
    finishDialogSettings();
}


wxString DIALOG_EXPORT_2581_BOM::choiceValue( const wxChoice* aChoice ) const
{
    if( !aChoice->IsEnabled() || aChoice->GetSelection() <= 0 )
        return wxEmptyString;

    return aChoice->GetStringSelection();
}


bool DIALOG_EXPORT_2581_BOM::TransferDataToWindow()
{
    m_textBomRev->SetValue( m_fields.m_revision );

    if( !m_oemRef->SetStringSelection( m_fields.m_internalId ) )
        m_oemRef->SetSelection( 0 );

    if( m_choiceMPN->SetStringSelection( m_fields.m_mfgPn ) )
    {
        m_choiceMfg->Enable( true );

        if( !m_choiceMfg->SetStringSelection( m_fields.m_mfg ) )
            m_choiceMfg->SetSelection( 0 );
    }
    else
    {
        m_choiceMPN->SetSelection( 0 );
        m_choiceMfg->SetSelection( 0 );
        m_choiceMfg->Enable( false );
    }

    if( m_choiceDistPN->SetStringSelection( m_fields.m_distPn ) )
    {
        m_textDistributor->Enable( true );
        m_textDistributor->SetValue( m_fields.m_dist );
    }
    else
    {
        m_choiceDistPN->SetSelection( 0 );
        m_textDistributor->SetValue( _( "N/A" ) );
        m_textDistributor->Enable( false );
    }

    return true;
}


bool DIALOG_EXPORT_2581_BOM::TransferDataFromWindow()
{
    m_fields.m_revision = m_textBomRev->GetValue();
    m_fields.m_internalId = choiceValue( m_oemRef );
    m_fields.m_mfgPn = choiceValue( m_choiceMPN );
    m_fields.m_mfg = choiceValue( m_choiceMfg );
    m_fields.m_distPn = choiceValue( m_choiceDistPN );

    if( !m_textDistributor->IsEnabled() || m_textDistributor->GetValue() == _( "N/A" ) )
        m_fields.m_dist = wxEmptyString;
    else
        m_fields.m_dist = m_textDistributor->GetValue();

    return true;
}


void DIALOG_EXPORT_2581_BOM::onMfgPNChange( wxCommandEvent& event )
{
    if( event.GetSelection() == 0 )
    {
        m_choiceMfg->Enable( false );
    }
    else
    {
        m_choiceMfg->Enable( true );

        // Do not guess the manufacturer if the user selected one
        if( m_choiceMfg->GetSelection() > 0 )
            return;

        int it = 0;

        if( it = m_choiceMfg->FindString( wxT( "manufacturer" ) ); it != wxNOT_FOUND )
            m_choiceMfg->Select( it );
        else if( it = m_choiceMfg->FindString( _( "manufacturer" ) ); it != wxNOT_FOUND )
            m_choiceMfg->Select( it );
        else if( it = m_choiceMfg->FindString( wxT( "mfg" ) ); it != wxNOT_FOUND )
            m_choiceMfg->Select( it );
        else if( it = m_choiceMfg->FindString( _( "mfg" ) ); it != wxNOT_FOUND )
            m_choiceMfg->Select( it );
    }
}


void DIALOG_EXPORT_2581_BOM::onDistPNChange( wxCommandEvent& event )
{
    if( event.GetSelection() == 0 )
    {
        m_textDistributor->Enable( false );
        m_textDistributor->SetValue( _( "N/A" ) );
    }
    else
    {
        m_textDistributor->Enable( true );

        // Do not guess the distributor if the user selected one
        if( m_textDistributor->GetValue() != _( "N/A" ) )
            return;

        wxString dist = m_choiceDistPN->GetStringSelection();
        dist.MakeUpper();

        // Guess the distributor from the part number column

        if( dist.Contains( wxT( "DIGIKEY" ) ) )
        {
            m_textDistributor->SetValue( wxT( "Digi-Key" ) );
        }
        else if( dist.Contains( wxT( "DIGI-KEY" ) ) )
        {
            m_textDistributor->SetValue( wxT( "Digi-Key" ) );
        }
        else if( dist.Contains( wxT( "MOUSER" ) ) )
        {
            m_textDistributor->SetValue( wxT( "Mouser" ) );
        }
        else if( dist.Contains( wxT( "NEWARK" ) ) )
        {
            m_textDistributor->SetValue( wxT( "Newark" ) );
        }
        else if( dist.Contains( wxT( "RS COMPONENTS" ) ) )
        {
            m_textDistributor->SetValue( wxT( "RS Components" ) );
        }
        else if( dist.Contains( wxT( "FARNELL" ) ) )
        {
            m_textDistributor->SetValue( wxT( "Farnell" ) );
        }
        else if( dist.Contains( wxT( "ARROW" ) ) )
        {
            m_textDistributor->SetValue( wxT( "Arrow" ) );
        }
        else if( dist.Contains( wxT( "AVNET" ) ) )
        {
            m_textDistributor->SetValue( wxT( "Avnet" ) );
        }
        else if( dist.Contains( wxT( "TME" ) ) )
        {
            m_textDistributor->SetValue( wxT( "TME" ) );
        }
        else if( dist.Contains( wxT( "LCSC" ) ) )
        {
            m_textDistributor->SetValue( wxT( "LCSC" ) );
        }
    }
}
