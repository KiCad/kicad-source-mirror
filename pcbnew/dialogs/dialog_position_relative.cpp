/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017-2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <pcb_edit_frame.h>
#include "tools/pcb_actions.h"

#include "dialog_position_relative.h"

// initialise statics
DIALOG_POSITION_RELATIVE::POSITION_RELATIVE_OPTIONS DIALOG_POSITION_RELATIVE::m_options;


DIALOG_POSITION_RELATIVE::DIALOG_POSITION_RELATIVE( PCB_BASE_FRAME* aParent, wxPoint& translation,
                                                    wxPoint& anchor ) :
    DIALOG_POSITION_RELATIVE_BASE( aParent ),
    m_toolMgr( aParent->GetToolManager() ),
    m_translation( translation ),
    m_anchor_position( anchor ),
    m_xOffset( aParent, m_xLabel, m_xEntry, m_xUnit ),
    m_yOffset( aParent, m_yLabel, m_yEntry, m_yUnit )
{
    // tabbing goes through the entries in sequence
    m_yEntry->MoveAfterInTabOrder( m_xEntry );

    // and set up the entries according to the saved options
    m_polarCoords->SetValue( m_options.polarCoords );
    updateDialogControls( m_polarCoords->IsChecked() );

    m_xOffset.SetValue( m_options.entry1 );
    m_yOffset.SetValue( m_options.entry2 );

    m_stdButtonsOK->SetDefault();

    FinishDialogSettings();
}


void DIALOG_POSITION_RELATIVE::ToPolarDeg( double x, double y, double& r, double& q )
{
    // convert to polar coordinates
    r = hypot( x, y );

    q = ( r != 0) ? RAD2DEG( atan2( y, x ) ) : 0;
}


bool DIALOG_POSITION_RELATIVE::GetTranslationInIU ( wxPoint& val, bool polar )
{
    if( polar )
    {
        const int r = m_xOffset.GetValue();
        const double q = m_yOffset.GetValue();

        val.x = r * cos( DEG2RAD( q / 10.0 ) );
        val.y = r * sin( DEG2RAD( q / 10.0 ) );
    }
    else
    {
        // direct read
        val.x = m_xOffset.GetValue();
        val.y = m_yOffset.GetValue();
    }

    // no validation to do here, but in future, you could return false here
    return true;
}


void DIALOG_POSITION_RELATIVE::OnPolarChanged( wxCommandEvent& event )
{
    bool newPolar = m_polarCoords->IsChecked();
    wxPoint val;

    // get the value as previously stored
    GetTranslationInIU( val, !newPolar );

    // now switch the controls to the new representations
    updateDialogControls( newPolar );

    if( newPolar )
    {
        // convert to polar coordinates
        double r, q;
        ToPolarDeg( val.x, val.y, r, q );

        m_xOffset.SetValue( KiROUND( r / 10.0) * 10 );
        m_yOffset.SetValue( q * 10 );
    }
    else
    {
        // vector is already in Cartesian, so just render out
        // note - round off the last decimal place (10nm) to prevent
        // (some) rounding causing errors when round-tripping
        // you can never eliminate entirely, however
        m_xOffset.SetValue( KiROUND( val.x / 10.0 ) * 10 );
        m_yOffset.SetValue( KiROUND( val.y / 10.0 ) * 10 );
    }
}


void DIALOG_POSITION_RELATIVE::updateDialogControls( bool aPolar )
{
    if( aPolar )
    {
        m_xOffset.SetLabel( _( "Distance:" ) );     // Polar radius
        m_yOffset.SetLabel( _( "Angle:" ) );        // Polar theta or angle
        m_yOffset.SetUnits( DEGREES );
    }
    else
    {
        m_xOffset.SetLabel( _( "Offset X:" ) );
        m_yOffset.SetLabel( _( "Offset Y:" ) );
        m_yOffset.SetUnits( GetUserUnits() );
    }
}


void DIALOG_POSITION_RELATIVE::OnClear( wxCommandEvent& event )
{
    wxObject* obj = event.GetEventObject();

    if( obj == m_clearX )
    {
        m_xOffset.SetValue( 0 );
    }
    else if( obj == m_clearY )
    {
        m_yOffset.SetValue( 0 );
    }
}


void DIALOG_POSITION_RELATIVE::OnSelectItemClick( wxCommandEvent& event )
{
    event.Skip();

    POSITION_RELATIVE_TOOL* posrelTool = m_toolMgr->GetTool<POSITION_RELATIVE_TOOL>();
    wxASSERT( posrelTool );
    m_toolMgr->RunAction( PCB_ACTIONS::selectpositionRelativeItem, true );

    Hide();
}


void DIALOG_POSITION_RELATIVE::OnUseGridOriginClick( wxCommandEvent& event )
{
    BOARD* board = (BOARD*) m_toolMgr->GetModel();

    m_anchor_position = board->GetGridOrigin();
    m_referenceInfo->SetLabel( _( "Reference location: grid origin" ) );
}


void DIALOG_POSITION_RELATIVE::OnUseUserOriginClick( wxCommandEvent& event )
{
    PCB_BASE_FRAME* frame = (PCB_BASE_FRAME*) m_toolMgr->GetEditFrame();

    m_anchor_position = frame->GetScreen()->m_O_Curseur;
    m_referenceInfo->SetLabel( _( "Reference location: local coordinates origin" ) );
}


void DIALOG_POSITION_RELATIVE::UpdateAnchor( EDA_ITEM* aItem )
{
    wxString reference = _( "<none selected>" );
    BOARD_ITEM* item = dynamic_cast<BOARD_ITEM*>( aItem );

    if( item )
    {
        m_anchor_position = item->GetPosition();
        reference = item->GetSelectMenuText( GetUserUnits() );
    }

    m_referenceInfo->SetLabel( wxString::Format( "Reference item: %s", reference ) );

    Show( true );
}


void DIALOG_POSITION_RELATIVE::OnOkClick( wxCommandEvent& event )
{
    // for the output, we only deliver a Cartesian vector
    bool ok = GetTranslationInIU( m_translation, m_polarCoords->IsChecked() );

    if( ok )
    {
        // save the settings
        m_options.polarCoords = m_polarCoords->GetValue();
        m_options.entry1      = m_xOffset.GetValue();
        m_options.entry2      = m_yOffset.GetValue();
        POSITION_RELATIVE_TOOL* posrelTool = m_toolMgr->GetTool<POSITION_RELATIVE_TOOL>();
        wxASSERT( posrelTool );

        posrelTool->RelativeItemSelectionMove( m_anchor_position, m_translation );

        event.Skip();
    }
}


void DIALOG_POSITION_RELATIVE::OnTextFocusLost( wxFocusEvent& event )
{
    wxTextCtrl* obj = static_cast<wxTextCtrl*>( event.GetEventObject() );

    if( obj->GetValue().IsEmpty() )
        obj->SetValue( "0" );

    event.Skip();
}
