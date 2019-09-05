/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 John Beard, john.j.beard@gmail.com
 * Copyright (C) 2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <dialogs/dialog_move_exact.h>
#include <widgets/tab_traversal.h>
#include <pcb_edit_frame.h>

// initialise statics
DIALOG_MOVE_EXACT::MOVE_EXACT_OPTIONS DIALOG_MOVE_EXACT::m_options;


DIALOG_MOVE_EXACT::DIALOG_MOVE_EXACT( PCB_BASE_FRAME *aParent, wxPoint& aTranslate,
                                      double& aRotate, ROTATION_ANCHOR& aAnchor,
                                      const EDA_RECT& aBbox ) :
    DIALOG_MOVE_EXACT_BASE( aParent ),
    m_translation( aTranslate ),
    m_rotation( aRotate ),
    m_rotationAnchor( aAnchor ),
    m_bbox( aBbox ),
    m_moveX( aParent, m_xLabel, m_xEntry, m_xUnit ),
    m_moveY( aParent, m_yLabel, m_yEntry, m_yUnit ),
    m_rotate( aParent, m_rotLabel, m_rotEntry, m_rotUnit )
{
    // We can't set the tab order through wxWidgets due to shortcomings in their mnemonics
    // implementation on MSW
    m_tabOrder = {
        m_xEntry,
        m_yEntry,
        m_rotEntry,
        m_anchorOptions,
        m_stdButtonsOK,
        m_stdButtonsCancel
    };

    updateDialogControls( m_options.polarCoords );

    m_menuIDs.push_back( aAnchor );
    m_menuIDs.push_back( ROTATE_AROUND_USER_ORIGIN );

    if( aParent->IsType( FRAME_PCB_EDITOR ) )
        m_menuIDs.push_back( ROTATE_AROUND_AUX_ORIGIN );

    buildRotationAnchorMenu();

    // and set up the entries according to the saved options
    m_polarCoords->SetValue( m_options.polarCoords );
    m_moveX.SetValue( m_options.entry1 );
    m_moveY.SetValue( m_options.entry2 );

    m_rotate.SetUnits( DEGREES );
    m_rotate.SetValue( m_options.entryRotation );
    m_anchorOptions->SetSelection( std::min( m_options.entryAnchorSelection, m_menuIDs.size() ) );

    m_stdButtonsOK->SetDefault();

    FinishDialogSettings();
}


void DIALOG_MOVE_EXACT::buildRotationAnchorMenu()
{
    wxArrayString menuItems;

    for( auto anchorID : m_menuIDs )
    {
        switch( anchorID )
        {
        case ROTATE_AROUND_ITEM_ANCHOR:
            menuItems.push_back( _( "Rotate around item anchor" ) );
            break;
        case ROTATE_AROUND_SEL_CENTER:
            menuItems.push_back( _( "Rotate around selection center" ) );
            break;
        case ROTATE_AROUND_USER_ORIGIN:
            menuItems.push_back( _( "Rotate around local coordinates origin" ) );
            break;
        case ROTATE_AROUND_AUX_ORIGIN:
            menuItems.push_back( _( "Rotate around drill/place origin" ) );
            break;
        }
    }

    m_anchorOptions->Set( menuItems );
}


void DIALOG_MOVE_EXACT::ToPolarDeg( double x, double y, double& r, double& q )
{
    // convert to polar coordinates
    r = hypot( x, y );

    q = ( r != 0) ? RAD2DEG( atan2( y, x ) ) : 0;
}


bool DIALOG_MOVE_EXACT::GetTranslationInIU ( wxPoint& val, bool polar )
{
    if( polar )
    {
        const int r = m_moveX.GetValue();
        const double q = m_moveY.GetValue();

        val.x = r * cos( DEG2RAD( q / 10.0 ) );
        val.y = r * sin( DEG2RAD( q / 10.0 ) );
    }
    else
    {
        // direct read
        val.x = m_moveX.GetValue();
        val.y = m_moveY.GetValue();
    }

    // no validation to do here, but in future, you could return false here
    return true;
}


void DIALOG_MOVE_EXACT::OnPolarChanged( wxCommandEvent& event )
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

        m_moveX.SetValue( KiROUND( r / 10.0) * 10 );
        m_moveY.SetValue( q * 10 );
    }
    else
    {
        // vector is already in Cartesian, so just render out
        // note - round off the last decimal place (10nm) to prevent
        // (some) rounding causing errors when round-tripping
        // you can never eliminate entirely, however
        m_moveX.SetValue( KiROUND( val.x / 10.0 ) * 10 );
        m_moveY.SetValue( KiROUND( val.y / 10.0 ) * 10 );
    }

}


void DIALOG_MOVE_EXACT::updateDialogControls( bool aPolar )
{
    if( aPolar )
    {
        m_moveX.SetLabel( _( "Distance:" ) );     // Polar radius
        m_moveY.SetLabel( _( "Angle:" ) );        // Polar theta or angle
        m_moveY.SetUnits( DEGREES );
    }
    else
    {
        m_moveX.SetLabel( _( "Move X:" ) );
        m_moveY.SetLabel( _( "Move Y:" ) );
        m_moveY.SetUnits( GetUserUnits() );
    }

    Layout();
}


void DIALOG_MOVE_EXACT::OnClear( wxCommandEvent& event )
{
    wxObject* obj = event.GetEventObject();

    if( obj == m_clearX )
    {
        m_moveX.SetValue( 0 );
    }
    else if( obj == m_clearY )
    {
        m_moveY.SetValue( 0 );
    }
    else if( obj == m_clearRot )
    {
        m_rotate.SetValue( 0 );
    }

    // Keep m_stdButtonsOK focused to allow enter key actiavte the OK button
    m_stdButtonsOK->SetFocus();
}


bool DIALOG_MOVE_EXACT::TransferDataFromWindow()
{
    // for the output, we only deliver a Cartesian vector
    bool ok = GetTranslationInIU( m_translation, m_polarCoords->IsChecked() );
    m_rotation = m_rotate.GetValue();
    m_rotationAnchor = m_menuIDs[ m_anchorOptions->GetSelection() ];

    if( ok )
    {
        // save the settings
        m_options.polarCoords = m_polarCoords->GetValue();
        m_options.entry1 = m_moveX.GetValue();
        m_options.entry2 = m_moveY.GetValue();
        m_options.entryRotation = m_rotate.GetValue();
        m_options.entryAnchorSelection = (size_t) std::max( m_anchorOptions->GetSelection(), 0 );
        return true;
    }

    return false;
}


void DIALOG_MOVE_EXACT::OnTextFocusLost( wxFocusEvent& event )
{
    wxTextCtrl* obj = static_cast<wxTextCtrl*>( event.GetEventObject() );

    if( obj->GetValue().IsEmpty() )
        obj->SetValue( "0" );

    event.Skip();
}


void DIALOG_MOVE_EXACT::OnTextChanged( wxCommandEvent& event )
{

    double delta_x = m_moveX.GetValue();
    double delta_y = m_moveY.GetValue();
    double max_border = std::numeric_limits<int>::max() * 0.7071;

    if( m_bbox.GetLeft() + delta_x < -max_border ||
            m_bbox.GetRight() + delta_x > max_border ||
            m_bbox.GetTop() + delta_y < -max_border ||
            m_bbox.GetBottom() + delta_y > max_border )
    {
        const wxString invalid_length = _( "Invalid movement values.  Movement would place selection "
                                           "outside of the maximum board area." );

        m_xEntry->SetToolTip( invalid_length );
        m_xEntry->SetForegroundColour( *wxRED );
        m_yEntry->SetToolTip( invalid_length );
        m_yEntry->SetForegroundColour( *wxRED );
        m_stdButtons->GetAffirmativeButton()->Disable();
    }
    else
    {
        m_xEntry->SetToolTip( "" );
        m_xEntry->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWTEXT ) );
        m_yEntry->SetToolTip( "" );
        m_yEntry->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWTEXT ) );
        m_stdButtons->GetAffirmativeButton()->Enable();
        event.Skip();
    }

}
