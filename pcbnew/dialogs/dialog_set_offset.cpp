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

#include "dialog_set_offset.h"


struct SET_OFFSET_OPTIONS
{
    bool   polarCoords = false;
    double entry1 = 0.0;
    double entry2 = 0.0;
};

static SET_OFFSET_OPTIONS s_savedOptions;


DIALOG_SET_OFFSET::DIALOG_SET_OFFSET( PCB_BASE_FRAME& aParent, VECTOR2I& aOffset,
                                      bool aClearToZero ) :
        DIALOG_SET_OFFSET_BASE( &aParent ), m_clearToZero( aClearToZero ),
        m_originalOffset( aOffset ), m_updatedOffset( aOffset ),
        m_xOffset( &aParent, m_xLabel, m_xEntry, m_xUnit ),
        m_yOffset( &aParent, m_yLabel, m_yEntry, m_yUnit )
{
    m_xOffset.SetCoordType( ORIGIN_TRANSFORMS::REL_X_COORD );
    m_yOffset.SetCoordType( ORIGIN_TRANSFORMS::REL_Y_COORD );

    SetInitialFocus( m_xEntry );

    // and set up the entries according to the saved options
    m_polarCoords->SetValue( s_savedOptions.polarCoords );
    updateDialogControls( m_polarCoords->IsChecked() );

    m_xOffset.SetDoubleValue( s_savedOptions.entry1 );
    m_yOffset.SetDoubleValue( s_savedOptions.entry2 );

    if( m_clearToZero )
    {
        wxString text = _( "Clear" );
        m_clearX->SetLabel( text );
        m_clearY->SetLabel( text );

        text = _( "Reset this value to zero." );
        m_clearX->SetToolTip( text );
        m_clearY->SetToolTip( text );
    }
    else
    {
        wxString text = _( "Reset" );
        m_clearX->SetLabel( text );
        m_clearY->SetLabel( text );

        text = _( "Reset this value to the original value." );
        m_clearX->SetToolTip( text );
        m_clearY->SetToolTip( text );
    }

    SetupStandardButtons();

    finishDialogSettings();
}


void DIALOG_SET_OFFSET::OnTextFocusLost( wxFocusEvent& event )
{
    wxTextCtrl* obj = static_cast<wxTextCtrl*>( event.GetEventObject() );

    if( obj->GetValue().IsEmpty() )
        obj->SetValue( "0" );

    event.Skip();
}


static void ToPolarDeg( double x, double y, double& r, EDA_ANGLE& q )
{
    // convert to polar coordinates
    r = hypot( x, y );

    q = ( r != 0 ) ? EDA_ANGLE( VECTOR2D( x, y ) ) : ANGLE_0;
}


void DIALOG_SET_OFFSET::OnClear( wxCommandEvent& event )
{
    if( m_clearToZero )
    {
        m_xOffset.SetDoubleValue( 0.0 );
        m_yOffset.SetDoubleValue( 0.0 );

        m_stateX = 0.0;
        m_stateY = 0.0;
        m_stateRadius = 0.0;
        m_stateTheta = ANGLE_0;
        return;
    }

    const wxObject* const obj = event.GetEventObject();
    VECTOR2I              offset = m_originalOffset;
    double                r;
    EDA_ANGLE             q;
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

void DIALOG_SET_OFFSET::OnPolarChanged( wxCommandEvent& event )
{
    bool   newPolar = m_polarCoords->IsChecked();
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

void DIALOG_SET_OFFSET::updateDialogControls( bool aPolar )
{
    if( aPolar )
    {
        m_xOffset.SetLabel( _( "Distance:" ) ); // Polar radius
        m_yOffset.SetLabel( _( "Angle:" ) );    // Polar theta or angle
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

bool DIALOG_SET_OFFSET::TransferDataToWindow()
{
    m_xOffset.SetValue( m_originalOffset.x );
    m_yOffset.SetValue( m_originalOffset.y );

    return true;
}

bool DIALOG_SET_OFFSET::TransferDataFromWindow()
{
    m_updatedOffset.x = m_xOffset.GetValue();
    m_updatedOffset.y = m_yOffset.GetValue();

    return true;
}
