/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Wayne Stambaugh <stambaughw@gmail.com>
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <sch_junction.h>
#include <dialog_junction_props.h>
#include <settings/settings_manager.h>
#include <sch_edit_frame.h>
#include <widgets/color_swatch.h>
#include <sch_commit.h>


DIALOG_JUNCTION_PROPS::DIALOG_JUNCTION_PROPS( SCH_EDIT_FRAME* aParent,
                                              std::deque<SCH_JUNCTION*>& aJunctions ) :
          DIALOG_JUNCTION_PROPS_BASE( aParent ),
          m_frame( aParent ),
          m_junctions( aJunctions ),
          m_diameter( aParent, m_staticTextDiameter, m_textCtrlDiameter,
                      m_staticTextDiameterUnits, true )
{
    m_colorSwatch->SetDefaultColor( COLOR4D::UNSPECIFIED );

    KIGFX::COLOR4D canvas = m_frame->GetColorSettings()->GetColor( LAYER_SCHEMATIC_BACKGROUND );
    m_colorSwatch->SetSwatchBackground( canvas.ToColour() );

    m_helpLabel1->SetFont( KIUI::GetInfoFont( this ).Italic() );
    m_helpLabel2->SetFont( KIUI::GetInfoFont( this ).Italic() );

    SetInitialFocus( m_textCtrlDiameter );

    SetupStandardButtons( { { wxID_APPLY, _( "Default" ) } } );

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


bool DIALOG_JUNCTION_PROPS::TransferDataToWindow()
{
    auto firstJunction = m_junctions.front();

    if( std::all_of( m_junctions.begin() + 1, m_junctions.end(),
            [&]( const SCH_JUNCTION* r )
            {
                return r->GetDiameter() == firstJunction->GetDiameter();
            } ) )
    {
        m_diameter.SetValue( firstJunction->GetDiameter() );
    }
    else
    {
        m_diameter.SetValue( INDETERMINATE_ACTION );
    }

    if( std::all_of( m_junctions.begin() + 1, m_junctions.end(),
            [&]( const SCH_JUNCTION* r )
            {
                return r->GetColor() == firstJunction->GetColor();
            } ) )
    {
        m_colorSwatch->SetSwatchColor( firstJunction->GetColor(), false );
    }
    else
    {
        m_colorSwatch->SetSwatchColor( COLOR4D::UNSPECIFIED, false );
    }

    return true;
}


void DIALOG_JUNCTION_PROPS::resetDefaults( wxCommandEvent& event )
{
    m_diameter.SetValue( 0 );
    m_colorSwatch->SetSwatchColor( COLOR4D::UNSPECIFIED, false );

    Refresh();
}


bool DIALOG_JUNCTION_PROPS::TransferDataFromWindow()
{
    SCH_COMMIT commit( m_frame );

    for( SCH_JUNCTION* junction : m_junctions )
    {
        commit.Modify( junction, m_frame->GetScreen() );

        if( !m_diameter.IsIndeterminate() )
            junction->SetDiameter( m_diameter.GetValue() );

        junction->SetColor( m_colorSwatch->GetSwatchColor() );

        m_frame->GetCanvas()->GetView()->Update( junction );
    }

    commit.Push( m_junctions.size() == 1 ? _( "Edit Junction" ) : _( "Edit Junctions" ) );

    return true;
}
