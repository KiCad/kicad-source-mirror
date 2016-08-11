/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#include "netlist_exporter_pspice_sim.h"

wxString NETLIST_EXPORTER_PSPICE_SIM::GetSheetSimCommand()
{
    wxString simCmd;

    UpdateDirectives( NET_ALL_FLAGS );

    for( const auto& dir : GetDirectives() )
    {
        if( isSimCommand( dir ) )
            simCmd += wxString::Format( "%s\r\n", dir );
    }

    return simCmd;
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
        if( !isSimCommand( dir ) )
            aFormatter->Print( 0, "%s\n", (const char*) dir.c_str() );
    }

    // Finish with our custom simulation command
    aFormatter->Print( 0, "%s\n", (const char*) m_simCommand.c_str() );
}


bool NETLIST_EXPORTER_PSPICE_SIM::isSimCommand( const wxString& aCmd )
{
    const std::vector<wxString> simCmds = {
        ".ac", ".dc", ".disto", ".noise", ".op", ".pz", ".sens", ".tf", ".tran", ".pss"
    };

    wxString lcaseCmd = aCmd.Lower();

    for( const auto& c : simCmds )
    {
        if( lcaseCmd.StartsWith( c ) )
            return true;
    }

    return false;
}

