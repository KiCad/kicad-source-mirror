/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 John Beard, john.j.beard@gmail.com
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

#include <wxPcbStruct.h>
#include <base_units.h>
#include <macros.h>

#include <module_editor_frame.h>

#include "dialog_move_exact.h"

// initialise statics
DIALOG_MOVE_EXACT::MOVE_EXACT_OPTIONS DIALOG_MOVE_EXACT::m_options;


DIALOG_MOVE_EXACT::DIALOG_MOVE_EXACT( PCB_BASE_FRAME* aParent,
                                      wxPoint& translation, double& rotation ):
    DIALOG_MOVE_EXACT_BASE( aParent ),
    m_translation( translation ),
    m_rotation( rotation )
{
    // set the unit labels
    m_xUnit->SetLabelText( GetAbbreviatedUnitsLabel( g_UserUnit ) );
    m_yUnit->SetLabelText( GetAbbreviatedUnitsLabel( g_UserUnit ) );

    // tabbing goes through the entries in sequence
    m_yEntry->MoveAfterInTabOrder( m_xEntry );
    m_rotEntry->MoveAfterInTabOrder( m_yEntry );

    // and set up the entries according to the saved options
    m_polarCoords->SetValue( m_options.polarCoords );
    m_xEntry->SetValue( wxString::FromDouble( m_options.entry1 ) );
    m_yEntry->SetValue( wxString::FromDouble( m_options.entry2 ) );
    m_rotEntry->SetValue( wxString::FromDouble( m_options.entryRotation ) );
    updateDlgTexts( m_polarCoords->IsChecked() );

    m_stdButtonsOK->SetDefault();

    GetSizer()->SetSizeHints( this );
}


DIALOG_MOVE_EXACT::~DIALOG_MOVE_EXACT()
{
}


void DIALOG_MOVE_EXACT::ToPolarDeg( double x, double y, double& r, double& q )
{
    // convert to polar coordinates
    r = hypot ( x, y );

    q = ( r != 0) ? RAD2DEG( atan2( y, x ) ) : 0;
}


bool DIALOG_MOVE_EXACT::GetTranslationInIU ( wxPoint& val, bool polar )
{
    if( polar )
    {
        const int r = ValueFromTextCtrl( *m_xEntry );
        const double q = DoubleValueFromString( DEGREES, m_yEntry->GetValue() );

        val.x = r * cos( DEG2RAD( q / 10.0 ) );
        val.y = r * sin( DEG2RAD( q / 10.0 ) );
    }
    else
    {
        // direct read
        val.x = ValueFromTextCtrl( *m_xEntry );
        val.y = ValueFromTextCtrl( *m_yEntry );
    }

    // no validation to do here, but in future, you could return false here
    return true;
}


void DIALOG_MOVE_EXACT::OnPolarChanged( wxCommandEvent& event )
{
    bool newPolar = m_polarCoords->IsChecked();
    updateDlgTexts( newPolar );
    wxPoint val;

    // get the value as previously stored
    GetTranslationInIU( val, !newPolar );

    if( newPolar )
    {
        // convert to polar coordinates
        double r, q;
        ToPolarDeg( val.x, val.y, r, q );

        PutValueInLocalUnits( *m_xEntry, KiROUND( r / 10.0) * 10 );
        m_yEntry->SetValue( wxString::FromDouble( q ) );
    }
    else
    {
        // vector is already in Cartesian, so just render out
        // note - round off the last decimal place (10nm) to prevent
        // (some) rounding causing errors when round-tripping
        // you can never eliminate entirely, however
        PutValueInLocalUnits( *m_xEntry, KiROUND( val.x / 10.0 ) * 10 );
        PutValueInLocalUnits( *m_yEntry, KiROUND( val.y / 10.0 ) * 10 );
    }
    Layout();
}


void DIALOG_MOVE_EXACT::updateDlgTexts( bool aPolar )
{
    if( aPolar )
    {
        m_xLabel->SetLabelText( _( "Distance:" ) );     // Polar radius
        m_yLabel->SetLabelText( _( "Angle:" ) );        // Polar theta or angle

        m_yUnit->SetLabelText( GetAbbreviatedUnitsLabel( DEGREES ) );
    }
    else
    {
        m_xLabel->SetLabelText( _( "Move vector X:" ) );
        m_yLabel->SetLabelText( _( "Move vector Y:" ) );

        m_yUnit->SetLabelText( GetAbbreviatedUnitsLabel( g_UserUnit ) );
    }
}


void DIALOG_MOVE_EXACT::OnClear( wxCommandEvent& event )
{
    wxObject* obj = event.GetEventObject();
    wxTextCtrl* entry = NULL;

    if( obj == m_clearX )
    {
        entry = m_xEntry;
    }
    else if( obj == m_clearY )
    {
        entry = m_yEntry;
    }
    else if( obj == m_clearRot )
    {
        entry = m_rotEntry;
    }

    if( entry )
        entry->SetValue( "0" );
}


void DIALOG_MOVE_EXACT::OnOkClick( wxCommandEvent& event )
{
    m_rotation = DoubleValueFromString( DEGREES, m_rotEntry->GetValue() );

    // for the output, we only deliver a Cartesian vector
    bool ok = GetTranslationInIU( m_translation, m_polarCoords->IsChecked() );

    if( ok )
    {
        // save the settings
        m_options.polarCoords = m_polarCoords->GetValue();
        m_xEntry->GetValue().ToDouble( &m_options.entry1 );
        m_yEntry->GetValue().ToDouble( &m_options.entry2 );
        m_rotEntry->GetValue().ToDouble( &m_options.entryRotation );

        event.Skip();
    }
}


void DIALOG_MOVE_EXACT::OnTextFocusLost( wxFocusEvent& event )
{
    wxTextCtrl* obj = static_cast<wxTextCtrl*>( event.GetEventObject() );

    if( obj->GetValue().IsEmpty() )
        obj->SetValue( "0" );

    event.Skip();
}
