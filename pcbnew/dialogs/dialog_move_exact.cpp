/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 John Beard, john.j.beard@gmail.com
 * Copyright (C) 1992-2014 KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2017 KiCad Developers, see CHANGELOG.TXT for contributors.
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


DIALOG_MOVE_EXACT::DIALOG_MOVE_EXACT(PCB_BASE_FRAME *aParent, MOVE_PARAMETERS &aParams ) :
    DIALOG_MOVE_EXACT_BASE( aParent ),
    m_translation( aParams.translation ),
    m_rotation( aParams.rotation ),
    m_origin( aParams.origin ),
    m_anchor( aParams.anchor ),
    m_allowOverride( aParams.allowOverride ),
    m_editingFootprint( aParams.editingFootprint )
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
    m_originChooser->SetSelection( m_options.origin );

    if( m_allowOverride )
    {
        m_cbOverride->SetValue( m_options.overrideAnchor );
        m_anchorChoice->Enable( m_options.overrideAnchor );

        // ME_ANCHOR_FROM_LIBRARY is not in the wxChoice options so show the first choice instead
        if( m_options.anchor == ANCHOR_FROM_LIBRARY )
        {
            m_anchorChoice->SetSelection( ANCHOR_TOP_LEFT_PAD );
        }
        else
        {
            m_anchorChoice->SetSelection( m_options.anchor );
        }

        if( m_options.origin == RELATIVE_TO_CURRENT_POSITION )
        {
            // no footprint override necessary in this mode
            m_cbOverride->Disable();
            m_anchorChoice->Disable();
        }

        if( m_editingFootprint )
        {
            // there is no point in showing the center footprint option when editing footprints
            m_anchorChoice->Delete( ANCHOR_CENTER_FOOTPRINT );
        }
    }
    else
    {
        // hide the checkbox and choice control if overides are not allowed
        bMainSizer->Hide( bAnchorSizer, true );
    }

    if( wxPoint( 0, 0 ) == aParent->GetScreen()->m_O_Curseur )
    {
        // disble the user origin option when the user oigin is not set
        m_originChooser->Enable( RELATIVE_TO_USER_ORIGIN, false );
        m_originChooser->SetItemToolTip( RELATIVE_TO_USER_ORIGIN,
                                         wxString( "The user origin is currently not set\n"
                                                   "Set it by using the <space> hotkey" ) );
    }

    if( wxPoint( 0, 0 ) == aParent->GetGridOrigin() )
    {
        // disble the grid origin option when the user oigin is not set
        m_originChooser->Enable( RELATIVE_TO_GRID_ORIGIN, false );
        m_originChooser->SetItemToolTip( RELATIVE_TO_GRID_ORIGIN,
                                         wxString( "The grid origin is currently not set\n"
                                                   "Set it by using the tool in the <place> menu" ) );
    }

    if( wxPoint( 0, 0 ) == aParent->GetAuxOrigin() )
    {
        // disble the grid origin option when the drill/place oigin is not set
        m_originChooser->Enable( RELATIVE_TO_DRILL_PLACE_ORIGIN, false );
        m_originChooser->SetItemToolTip( RELATIVE_TO_DRILL_PLACE_ORIGIN,
                                         wxString( "The drill/place origin is currently not set\n"
                                                   "Set it by using the tool in the <place> menu" ) );
    }

    updateDlgTexts( m_polarCoords->IsChecked() );

    m_stdButtonsOK->SetDefault();

    GetSizer()->SetSizeHints( this );
    Layout();
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


void DIALOG_MOVE_EXACT::OnOriginChanged( wxCommandEvent& event )
{
    if( m_originChooser->GetSelection() == RELATIVE_TO_CURRENT_POSITION )
    {
        //no need to override the achor in this mode since the reference in the current position
        m_cbOverride->Disable();
        m_anchorChoice->Disable();
    }
    else if( m_allowOverride )
    {
        m_cbOverride->Enable();

        if( m_cbOverride->IsChecked() )
            m_anchorChoice->Enable();
    }
}


void DIALOG_MOVE_EXACT::OnOverrideChanged( wxCommandEvent& event )
{
    if( m_cbOverride->IsChecked() )
    {
        m_anchorChoice->Enable();
    }
    else
    {
        m_anchorChoice->Disable();
    }
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
    m_origin = static_cast<MOVE_EXACT_ORIGIN>( m_originChooser->GetSelection() );

    if( m_cbOverride->IsChecked() && m_allowOverride )
    {
        m_anchor = static_cast<MOVE_EXACT_ANCHOR>( m_anchorChoice->GetSelection() );
    }
    else
    {
        m_anchor = ANCHOR_FROM_LIBRARY;
    }

    // for the output, we only deliver a Cartesian vector
    bool ok = GetTranslationInIU( m_translation, m_polarCoords->IsChecked() );

    if( ok )
    {
        // save the settings
        m_options.polarCoords = m_polarCoords->GetValue();
        m_options.entry1 = DoubleValueFromString( UNSCALED_UNITS, m_xEntry->GetValue() );
        m_options.entry2 = DoubleValueFromString( UNSCALED_UNITS, m_yEntry->GetValue() );
        m_options.entryRotation = DoubleValueFromString( UNSCALED_UNITS, m_rotEntry->GetValue() );
        m_options.origin = m_origin;
        m_options.anchor = static_cast<MOVE_EXACT_ANCHOR>( m_anchorChoice->GetSelection() );
        m_options.overrideAnchor = m_cbOverride->IsChecked();
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
