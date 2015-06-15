/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2014  CERN
 * Author: Maciej Suminski <maciej.suminski@cern.ch>
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
 * with this program.  If not, see <http://www.gnu.or/licenses/>.
 */

/**
 * Push and Shove router track width and via size dialog.
 */

#include "dialog_track_via_size.h"
#include <base_units.h>
#include <confirm.h>
#include <boost/optional.hpp>

#include "class_board_design_settings.h"

DIALOG_TRACK_VIA_SIZE::DIALOG_TRACK_VIA_SIZE( wxWindow* aParent, BOARD_DESIGN_SETTINGS& aSettings ) :
    DIALOG_TRACK_VIA_SIZE_BASE( aParent ),
    m_trackWidth( aParent, m_trackWidthText, m_trackWidthLabel ),
    m_viaDiameter( aParent, m_viaDiameterText, m_viaDiameterLabel ),
    m_viaDrill( aParent, m_viaDrillText, m_viaDrillLabel ),
    m_settings( aSettings )
{
    // Load router settings to dialog fields
    m_trackWidth.SetValue( m_settings.GetCustomTrackWidth() );
    m_viaDiameter.SetValue( m_settings.GetCustomViaSize() );
    m_viaDrill.SetValue( m_settings.GetCustomViaDrill() );

    m_trackWidthText->SetFocus();
    m_trackWidthText->SetSelection( -1, -1 );
    m_stdButtonsOK->SetDefault();

    GetSizer()->SetSizeHints( this );
    Centre();

    // Pressing ENTER when any of the text input fields is active applies changes
    #if wxCHECK_VERSION( 3, 0, 0 )
        Connect( wxEVT_TEXT_ENTER, wxCommandEventHandler( DIALOG_TRACK_VIA_SIZE::onOkClick ), NULL, this );
    #else
        Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_TRACK_VIA_SIZE::onOkClick ), NULL, this );
    #endif
}


bool DIALOG_TRACK_VIA_SIZE::check()
{
    if( m_trackWidth.GetValue() < 0 )
    {
        DisplayError( GetParent(), _( "Invalid track width" ) );
        m_trackWidthText->SetFocus();
        return false;
    }

    if( m_viaDiameter.GetValue() < 0 )
    {
        DisplayError( GetParent(), _( "Invalid via diameter" ) );
        m_viaDiameterText->SetFocus();
        return false;
    }

    if( m_viaDrill.GetValue() < 0 )
    {
        DisplayError( GetParent(), _( "Invalid via drill size" ) );
        m_viaDrillText->SetFocus();
        return false;
    }

    if( m_viaDrill.GetValue() >= m_viaDiameter.GetValue() )
    {
        DisplayError( GetParent(), _( "Via drill size has to be smaller than via diameter" ) );
        m_viaDrillText->SetFocus();
        return false;
    }

    return true;
}


void DIALOG_TRACK_VIA_SIZE::onClose( wxCloseEvent& aEvent )
{
    EndModal( 0 );
}


void DIALOG_TRACK_VIA_SIZE::onOkClick( wxCommandEvent& aEvent )
{
    if( check() )
    {
        // Store dialog values to the router settings
        m_settings.SetCustomTrackWidth( m_trackWidth.GetValue() );
        m_settings.SetCustomViaSize( m_viaDiameter.GetValue() );
        m_settings.SetCustomViaDrill( m_viaDrill.GetValue() );
        EndModal( 1 );
    }
}


void DIALOG_TRACK_VIA_SIZE::onCancelClick( wxCommandEvent& aEvent )
{
    EndModal( 0 );
}
