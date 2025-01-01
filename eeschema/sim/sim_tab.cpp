/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016-2023 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Sylwester Kocjan <s.kocjan@o2.pl>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "confirm.h"
#include "sim_tab.h"

#include "simulator_frame.h"
#include "spice_circuit_model.h"


SIM_TAB::SIM_TAB() :
        m_simCommand( wxEmptyString ),
        m_simOptions( NETLIST_EXPORTER_SPICE::OPTION_DEFAULT_FLAGS )
{
}


SIM_TAB::SIM_TAB( const wxString& aSimCommand, wxWindow* parent ) :
        wxWindow( parent, wxID_ANY ),
        m_simCommand( aSimCommand ),
        m_simOptions( NETLIST_EXPORTER_SPICE::OPTION_DEFAULT_FLAGS )
{
}


SIM_TAB::~SIM_TAB()
{
}


bool SIM_TAB::IsPlottable( SIM_TYPE aSimType )
{
    switch( aSimType )
    {
    case ST_AC:
    case ST_DC:
    case ST_SP:
    case ST_TRAN:
    case ST_NOISE:
    case ST_FFT:
        return true;

    default:
        return false;
    }
}

void SIM_TAB::ApplyPreferences( const SIM_PREFERENCES& /*aPrefs*/ )
{
}


SIM_TYPE SIM_TAB::GetSimType() const
{
    return SPICE_CIRCUIT_MODEL::CommandToSimType( m_simCommand );
}


SIM_NOPLOT_TAB::SIM_NOPLOT_TAB( const wxString& aSimCommand, wxWindow* parent ) :
        SIM_TAB( aSimCommand, parent )
{
    m_sizer = new wxBoxSizer( wxVERTICAL );
    m_sizer->Add( 0, 1, 1, wxEXPAND, 5 );

    m_textInfo = new wxStaticText( dynamic_cast<wxWindow*>( this ), wxID_ANY, "", wxDefaultPosition,
                                   wxDefaultSize, wxALL | wxEXPAND | wxALIGN_CENTER_HORIZONTAL );
    m_textInfo->SetFont( KIUI::GetControlFont( this ).Bold() );
    m_textInfo->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT ) );
    m_textInfo->SetLabel( _( "Simulation provided no plots. Please refer to console window "
                             "for results." ) );

    m_sizer->Add( m_textInfo, 1, wxALL | wxEXPAND, 5 );
    m_sizer->Add( 0, 1, 1, wxEXPAND, 5 );

    dynamic_cast<wxWindow*>( this )->SetSizer( m_sizer );
}


SIM_NOPLOT_TAB::~SIM_NOPLOT_TAB()
{
}


void SIM_NOPLOT_TAB::OnLanguageChanged()
{
    m_textInfo->SetLabel( _( "Simulation provided no plots. Please refer to console window "
                             "for results." ) );
}


