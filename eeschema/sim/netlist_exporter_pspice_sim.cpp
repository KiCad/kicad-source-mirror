/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#include "netlist_exporter_pspice_sim.h"
#include "sim_types.h"

wxString NETLIST_EXPORTER_PSPICE_SIM::GetSheetSimCommand()
{
    wxString simCmd;

    UpdateDirectives( NET_ALL_FLAGS );

    for( const auto& dir : GetDirectives() )
    {
        if( IsSimCommand( dir ) )
            simCmd += wxString::Format( "%s\r\n", dir );
    }

    return simCmd;
}


SIM_TYPE NETLIST_EXPORTER_PSPICE_SIM::GetSimType()
{
    return CommandToSimType( m_simCommand.IsEmpty() ? GetSheetSimCommand() : m_simCommand );
}


SIM_TYPE NETLIST_EXPORTER_PSPICE_SIM::CommandToSimType( const wxString& aCmd )
{
    const std::map<wxString, SIM_TYPE> simCmds = {
        { ".ac", ST_AC }, { ".dc", ST_DC }, { ".disto", ST_DISTORTION }, { ".noise", ST_NOISE },
        { ".op", ST_OP }, { ".pz", ST_POLE_ZERO }, { ".sens", ST_SENSITIVITY }, { ".tf", ST_TRANS_FUNC },
        { ".tran", ST_TRANSIENT }
    };
    wxString lcaseCmd = aCmd.Lower();

    for( const auto& c : simCmds )
    {
        if( lcaseCmd.StartsWith( c.first ) )
            return c.second;
    }

    return ST_UNKNOWN;
}


void NETLIST_EXPORTER_PSPICE_SIM::writeDirectives( OUTPUTFORMATTER* aFormatter, unsigned aCtl ) const
{
    if( m_simCommand.IsEmpty() )
    {
        // Fallback to the default behavior and just write all directives
        NETLIST_EXPORTER_PSPICE::writeDirectives( aFormatter, aCtl );
    }

    // Dump all directives, but simulation commands
    for( const auto& dir : GetDirectives() )
    {
        if( !IsSimCommand( dir ) )
            aFormatter->Print( 0, "%s\n", (const char*) dir.c_str() );
    }

    // Finish with our custom simulation command
    aFormatter->Print( 0, "%s\n", (const char*) m_simCommand.c_str() );
}
