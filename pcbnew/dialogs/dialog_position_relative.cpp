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

#include <dialogs/dialog_position_relative.h>
#include <math/util.h>      // for KiROUND
#include <tools/pcb_actions.h>
#include <widgets/tab_traversal.h>
#include <pcb_edit_frame.h>
#include <board_design_settings.h>
#include <trigo.h>

// initialise statics
DIALOG_POSITION_RELATIVE::POSITION_RELATIVE_OPTIONS DIALOG_POSITION_RELATIVE::m_options;


DIALOG_POSITION_RELATIVE::DIALOG_POSITION_RELATIVE( PCB_BASE_FRAME* aParent, wxPoint& translation,
                                                    wxPoint& anchor ) :
    DIALOG_POSITION_RELATIVE_BASE( aParent ),
    m_toolMgr( aParent->GetToolManager() ),
    m_translation( translation ),
    m_anchor_position( anchor ),
    m_xOffset( aParent, m_xLabel, m_xEntry, m_xUnit ),
    m_yOffset( aParent, m_yLabel, m_yEntry, m_yUnit ),
    m_stateX( 0.0 ),
    m_stateY( 0.0 ),
    m_stateRadius( 0.0 ),
    m_stateTheta( 0.0 )
{
    // We can't set the tab order through wxWidgets due to shortcomings in their mnemonics
    // implementation on MSW
    m_tabOrder = {
        m_xEntry,
        m_yEntry,
        m_stdButtonsOK,
        m_stdButtonsCancel
    };

    // Configure display origin transforms
    m_xOffset.SetCoordType( ORIGIN_TRANSFORMS::REL_X_COORD );
    m_yOffset.SetCoordType( ORIGIN_TRANSFORMS::REL_Y_COORD );

    SetInitialFocus( m_xEntry );

    // and set up the entries according to the saved options
    m_polarCoords->SetValue( m_options.polarCoords );
    updateDialogControls( m_polarCoords->IsChecked() );

    m_xOffset.SetDoubleValue( m_options.entry1 );
    m_yOffset.SetDoubleValue( m_options.entry2 );

    m_stdButtonsOK->SetDefault();

    finishDialogSettings();
}


void DIALOG_POSITION_RELATIVE::ToPolarDeg( double x, double y, double& r, double& q )
{
    // convert to polar coordinates
    r = hypot( x, y );

    q = ( r != 0) ? RAD2DEG( atan2( y, x ) ) : 0;
}


bool DIALOG_POSITION_RELATIVE::GetTranslationInIU( wxRealPoint& val, bool polar )
{
    if( polar )
    {
        const double r = m_xOffset.GetDoubleValue();
        const double q = m_yOffset.GetDoubleValue();

        val.x = r * cos( DEG2RAD( q / 10.0 ) );
        val.y = r * sin( DEG2RAD( q / 10.0 ) );
    }
    else
    {
        // direct read
        val.x = m_xOffset.GetDoubleValue();
        val.y = m_yOffset.GetDoubleValue();
    }

    // no validation to do here, but in future, you could return false here
    return true;
}


void DIALOG_POSITION_RELATIVE::OnPolarChanged( wxCommandEvent& event )
{
    bool newPolar = m_polarCoords->IsChecked();
    double xOffset = m_xOffset.GetDoubleValue();
    double yOffset = m_yOffset.GetDoubleValue();
    updateDialogControls( newPolar );

    if( newPolar )
    {
        if( xOffset != m_stateX || yOffset != m_stateY )
        {
            m_stateX = xOffset;
            m_stateY = yOffset;
            ToPolarDeg( m_stateX, m_stateY, m_stateRadius, m_stateTheta );
            m_stateTheta *= 10.0;

            m_xOffset.SetDoubleValue( m_stateRadius );
            m_stateRadius = m_xOffset.GetDoubleValue();
            m_yOffset.SetDoubleValue( m_stateTheta );
            m_stateTheta = m_yOffset.GetDoubleValue();
        }
        else
        {
            m_xOffset.SetDoubleValue( m_stateRadius );
            m_yOffset.SetDoubleValue( m_stateTheta );
        }
    }
    else
    {
        if( xOffset != m_stateRadius || yOffset != m_stateTheta )
        {
            m_stateRadius = xOffset;
            m_stateTheta = yOffset;
            m_stateX = m_stateRadius * cos( DEG2RAD( m_stateTheta / 10.0 ) );
            m_stateY = m_stateRadius * sin( DEG2RAD( m_stateTheta / 10.0 ) );

            m_xOffset.SetDoubleValue( m_stateX );
            m_stateX = m_xOffset.GetDoubleValue();
            m_yOffset.SetDoubleValue( m_stateY );
            m_stateY = m_yOffset.GetDoubleValue();
        }
        else
        {
            m_xOffset.SetDoubleValue( m_stateX );
            m_yOffset.SetDoubleValue( m_stateY );
        }
    }
}


void DIALOG_POSITION_RELATIVE::updateDialogControls( bool aPolar )
{
    if( aPolar )
    {
        m_xOffset.SetLabel( _( "Distance:" ) );     // Polar radius
        m_yOffset.SetLabel( _( "Angle:" ) );        // Polar theta or angle
        m_yOffset.SetUnits( EDA_UNITS::DEGREES );
        m_clearX->SetToolTip( _( "Reset to the current distance from the reference position." ) );
        m_clearY->SetToolTip( _( "Reset to the current angle from the reference position." ) );
    }
    else
    {
        m_xOffset.SetLabel( _( "Offset X:" ) );
        m_yOffset.SetLabel( _( "Offset Y:" ) );
        m_yOffset.SetUnits( GetUserUnits() );
        m_clearX->SetToolTip( _( "Reset to the current X offset from the reference position." ) );
        m_clearY->SetToolTip( _( "Reset to the current Y offset from the reference position." ) );
    }
}


void DIALOG_POSITION_RELATIVE::OnClear( wxCommandEvent& event )
{
    wxObject* obj = event.GetEventObject();
    POSITION_RELATIVE_TOOL* posrelTool = m_toolMgr->GetTool<POSITION_RELATIVE_TOOL>();
    wxASSERT( posrelTool );

    wxPoint offset = posrelTool->GetSelectionAnchorPosition() - m_anchor_position;
    double  r, q;
    ToPolarDeg( offset.x, offset.y, r, q );


    if( obj == m_clearX )
    {
        m_stateX = offset.x;
        m_xOffset.SetDoubleValue( r );
        m_stateRadius = m_xOffset.GetDoubleValue();

        if( m_polarCoords->IsChecked() )
        {
            m_xOffset.SetDoubleValue( m_stateRadius );
        }
        else
        {
            m_xOffset.SetValue( m_stateX );
        }
    }
    else if( obj == m_clearY )
    {
        m_stateY = offset.y;
        m_yOffset.SetDoubleValue( q * 10 );
        m_stateTheta = m_yOffset.GetDoubleValue();

        if( m_polarCoords->IsChecked() )
        {
            m_yOffset.SetDoubleValue( m_stateTheta );
        }
        else
        {
            m_yOffset.SetValue( m_stateY );
        }
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

    m_anchor_position = board->GetDesignSettings().GetGridOrigin();
    m_referenceInfo->SetLabel( _( "Reference location: grid origin" ) );
}


void DIALOG_POSITION_RELATIVE::OnUseUserOriginClick( wxCommandEvent& event )
{
    PCB_BASE_FRAME* frame = (PCB_BASE_FRAME*) m_toolMgr->GetToolHolder();

    m_anchor_position = (wxPoint) frame->GetScreen()->m_LocalOrigin;
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
    wxRealPoint translation;
    bool ok = GetTranslationInIU( translation, m_polarCoords->IsChecked() );
    m_translation.x = KiROUND( translation.x );
    m_translation.y = KiROUND( translation.y );

    if( ok )
    {
        // save the settings
        m_options.polarCoords = m_polarCoords->GetValue();
        m_options.entry1      = m_xOffset.GetDoubleValue();
        m_options.entry2      = m_yOffset.GetDoubleValue();
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
