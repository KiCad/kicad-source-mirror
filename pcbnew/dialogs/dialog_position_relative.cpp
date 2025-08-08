/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include "dialogs/dialog_position_relative.h"

#include <math/util.h>      // for KiROUND
#include <tools/pcb_actions.h>
#include <widgets/tab_traversal.h>
#include <pcb_edit_frame.h>
#include <board_design_settings.h>
#include <tools/pcb_picker_tool.h>
#include <tools/position_relative_tool.h>
#include <trigo.h>

// initialise statics
DIALOG_POSITION_RELATIVE::ANCHOR_TYPE DIALOG_POSITION_RELATIVE::s_anchorType = DIALOG_POSITION_RELATIVE::ANCHOR_ITEM;


DIALOG_POSITION_RELATIVE::DIALOG_POSITION_RELATIVE( PCB_BASE_FRAME* aParent ) :
    DIALOG_POSITION_RELATIVE_BASE( aParent ),
    m_toolMgr( aParent->GetToolManager() ),
    m_xOffset( aParent, m_xLabel, m_xEntry, m_xUnit ),
    m_yOffset( aParent, m_yLabel, m_yEntry, m_yUnit ),
    m_stateX( 0.0 ),
    m_stateY( 0.0 ),
    m_stateRadius( 0.0 )
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

    updateDialogControls( m_polarCoords->IsChecked() );

    SetupStandardButtons();

    finishDialogSettings();
}


void DIALOG_POSITION_RELATIVE::ToPolarDeg( double x, double y, double& r, EDA_ANGLE& q )
{
    // convert to polar coordinates
    r = hypot( x, y );

    q = ( r != 0) ? EDA_ANGLE( VECTOR2D( x, y ) ) : ANGLE_0;
}


bool DIALOG_POSITION_RELATIVE::getTranslationInIU( VECTOR2I& val, bool polar )
{
    if( polar )
    {
        const double    r = m_xOffset.GetDoubleValue();
        const EDA_ANGLE q = m_yOffset.GetAngleValue();

        val.x = KiROUND( r * q.Cos() );
        val.y = KiROUND( r * q.Sin() );
    }
    else
    {
        // direct read
        val.x = KiROUND( m_xOffset.GetDoubleValue() );
        val.y = KiROUND( m_yOffset.GetDoubleValue() );
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

            m_xOffset.SetDoubleValue( m_stateRadius );
            m_stateRadius = m_xOffset.GetDoubleValue();
            m_yOffset.SetAngleValue( m_stateTheta );
            m_stateTheta = m_yOffset.GetAngleValue();
        }
        else
        {
            m_xOffset.SetDoubleValue( m_stateRadius );
            m_yOffset.SetAngleValue( m_stateTheta );
        }
    }
    else
    {
        if( xOffset != m_stateRadius || yOffset != m_stateTheta.AsDegrees() )
        {
            m_stateRadius = xOffset;
            m_stateTheta = EDA_ANGLE( yOffset, DEGREES_T );
            m_stateX = m_stateRadius * m_stateTheta.Cos();
            m_stateY = m_stateRadius * m_stateTheta.Sin();

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

    VECTOR2I  offset = posrelTool->GetSelectionAnchorPosition() - getAnchorPos();
    double    r;
    EDA_ANGLE q;
    ToPolarDeg( offset.x, offset.y, r, q );

    if( obj == m_clearX )
    {
        m_stateX = offset.x;
        m_xOffset.SetDoubleValue( r );
        m_stateRadius = m_xOffset.GetDoubleValue();

        if( m_polarCoords->IsChecked() )
            m_xOffset.SetDoubleValue( m_stateRadius );
        else
            m_xOffset.SetValue( m_stateX );
    }
    else if( obj == m_clearY )
    {
        m_stateY = offset.y;
        m_yOffset.SetAngleValue( q );
        m_stateTheta = m_yOffset.GetAngleValue();

        if( m_polarCoords->IsChecked() )
            m_yOffset.SetAngleValue( m_stateTheta );
        else
            m_yOffset.SetValue( m_stateY );
    }
}


void DIALOG_POSITION_RELATIVE::OnSelectItemClick( wxCommandEvent& event )
{
    event.Skip();

    PCB_PICKER_TOOL* pickerTool = m_toolMgr->GetTool<PCB_PICKER_TOOL>();
    wxCHECK( pickerTool, /* void */ );

    m_toolMgr->RunAction( PCB_ACTIONS::selectItemInteractively,
                          PCB_PICKER_TOOL::INTERACTIVE_PARAMS{ this, _( "Select reference item..." ) } );

    Hide();
}


void DIALOG_POSITION_RELATIVE::OnSelectPointClick( wxCommandEvent& event )
{
    event.Skip();

    PCB_PICKER_TOOL* pickerTool = m_toolMgr->GetTool<PCB_PICKER_TOOL>();
    wxCHECK( pickerTool, /* void */ );

    // Hide, but do not close, the dialog
    Hide();

    m_toolMgr->RunAction( PCB_ACTIONS::selectPointInteractively,
                          PCB_PICKER_TOOL::INTERACTIVE_PARAMS { this, _( "Select reference point..." ) } );
}


void DIALOG_POSITION_RELATIVE::updateAnchorInfo( const BOARD_ITEM* aItem )
{
    switch( s_anchorType )
    {
    case ANCHOR_GRID_ORIGIN:
        m_referenceInfo->SetLabel( _( "Reference location: grid origin" ) );
        break;

    case ANCHOR_USER_ORIGIN:
        m_referenceInfo->SetLabel( _( "Reference location: local coordinates origin" ) );
        break;

    case ANCHOR_ITEM:
    {
        UNITS_PROVIDER unitsProvider( pcbIUScale, GetUserUnits() );
        wxString       msg = _( "<none selected>" );

        if( aItem )
            msg = aItem->GetItemDescription( &unitsProvider, true );

        m_referenceInfo->SetLabel( wxString::Format( _( "Reference item: %s" ), msg ) );
        break;
    }

    case ANCHOR_POINT:
        m_referenceInfo->SetLabel( wxString::Format( _( "Reference location: selected point (%s, %s)" ),
                                                     m_parentFrame->MessageTextFromValue( m_anchorItemPosition.x ),
                                                     m_parentFrame->MessageTextFromValue( m_anchorItemPosition.y ) ) );
        break;
    }
}


VECTOR2I DIALOG_POSITION_RELATIVE::getAnchorPos()
{
    switch( s_anchorType )
    {
    case ANCHOR_GRID_ORIGIN:
        return static_cast<BOARD*>( m_toolMgr->GetModel() )->GetDesignSettings().GetGridOrigin();

    case ANCHOR_USER_ORIGIN:
        return static_cast<PCB_BASE_FRAME*>( m_toolMgr->GetToolHolder() )->GetScreen()->m_LocalOrigin;

    case ANCHOR_ITEM:
    case ANCHOR_POINT:
        return m_anchorItemPosition;
    }

    // Needed by some compilers to avoid a fatal compil error (no return value).
    return m_anchorItemPosition;
}


void DIALOG_POSITION_RELATIVE::OnUseGridOriginClick( wxCommandEvent& event )
{
    s_anchorType = ANCHOR_GRID_ORIGIN;
    updateAnchorInfo( nullptr );
}


void DIALOG_POSITION_RELATIVE::OnUseUserOriginClick( wxCommandEvent& event )
{
    s_anchorType = ANCHOR_USER_ORIGIN;
    updateAnchorInfo( nullptr );
}


void DIALOG_POSITION_RELATIVE::UpdatePickedItem( const EDA_ITEM* aItem )
{
    const BOARD_ITEM* item = nullptr;

    if( aItem && aItem->IsBOARD_ITEM() )
        item = static_cast<const BOARD_ITEM*>( aItem );

    s_anchorType = ANCHOR_ITEM;
    updateAnchorInfo( item );

    if( item )
        m_anchorItemPosition = item->GetPosition();

    Show( true );
}


void DIALOG_POSITION_RELATIVE::UpdatePickedPoint( const std::optional<VECTOR2I>& aPoint )
{
    s_anchorType = ANCHOR_POINT;

    if( aPoint )
        m_anchorItemPosition = *aPoint;

    updateAnchorInfo( nullptr );

    Show( true );
}


void DIALOG_POSITION_RELATIVE::OnOkClick( wxCommandEvent& event )
{
    // for the output, we only deliver a Cartesian vector
    VECTOR2I translation;

    if( getTranslationInIU( translation, m_polarCoords->IsChecked() ) )
    {
        POSITION_RELATIVE_TOOL* posrelTool = m_toolMgr->GetTool<POSITION_RELATIVE_TOOL>();

        posrelTool->RelativeItemSelectionMove( getAnchorPos(), translation );

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
