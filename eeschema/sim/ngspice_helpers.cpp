/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.TXT for contributors.
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

#include "ngspice_helpers.h"
#include <string_utils.h>
#include <macros.h>     // for TO_UTF8 def
#include <wx/regex.h>
#include <wx/tokenzr.h>
#include <locale_io.h>


SIM_PLOT_TYPE NGSPICE_CIRCUIT_MODEL::VectorToSignal(
        const std::string& aVector, wxString& aSignal ) const
{
    using namespace std;

    // See ngspice manual chapt. 31.1 "Accessing internal device parameters"
    wxRegEx  internalDevParameter( "^@(\\w*[\\.\\w+]*)\\[(\\w*)\\]$", wxRE_ADVANCED );
    wxString vector( aVector );

    if( !internalDevParameter.Matches( vector ) )
    {
        // any text is a node name, which returns voltage
        aSignal = "V(" + aVector + ")";
        return SPT_VOLTAGE;
    }
    else
    {
        wxString paramType = internalDevParameter.GetMatch( vector, 2 );

        if( paramType.Lower()[0] == 'i' )
        {
            // this is a branch current
            paramType[0] = 'I';
            aSignal      = paramType + "(";
            aSignal += internalDevParameter.GetMatch( vector, 1 ).Upper() + ")";
            return SPT_CURRENT;
        }
        else
        {
            return SPT_UNKNOWN;
        }
    }
}


wxString NGSPICE_CIRCUIT_MODEL::GetSheetSimCommand()
{
    wxString simCmd;

    ReadDirectives();

    for( const auto& directive : GetDirectives() )
    {
        if( IsSimCommand( directive ) )
            simCmd += wxString::Format( "%s\r\n", directive );
    }

    return simCmd;
}


wxString NGSPICE_CIRCUIT_MODEL::GetUsedSimCommand()
{
    return m_simCommand.IsEmpty() ? GetSheetSimCommand() : m_simCommand;
}


SIM_TYPE NGSPICE_CIRCUIT_MODEL::GetSimType()
{
    return CommandToSimType( GetUsedSimCommand() );
}


SIM_TYPE NGSPICE_CIRCUIT_MODEL::CommandToSimType( const wxString& aCmd )
{
    const std::vector<std::pair<wxString, SIM_TYPE>> simCmds = {
        { "^.ac\\M.*", ST_AC },
        { "^.dc\\M.*", ST_DC },
        { "^.tran\\M.*", ST_TRANSIENT },
        { "^.op\\M.*", ST_OP },
        { "^.disto\\M.*", ST_DISTORTION },
        { "^.noise\\M.*", ST_NOISE },
        { "^.pz\\M.*", ST_POLE_ZERO },
        { "^.sens\\M.*", ST_SENSITIVITY },
        { "^.tf\\M.*", ST_TRANS_FUNC } };
    wxRegEx simCmd;

    for( const auto& c : simCmds )
    {
        simCmd.Compile( c.first, wxRE_ADVANCED | wxRE_NOSUB | wxRE_ICASE );

        if( simCmd.Matches( aCmd ) )
            return c.second;
    }

    return ST_UNKNOWN;
}


bool NGSPICE_CIRCUIT_MODEL::ParseDCCommand( const wxString& aCmd, SPICE_DC_PARAMS* aSource1,
                                                  SPICE_DC_PARAMS* aSource2 )
{
    if( !aCmd.Lower().StartsWith( ".dc" ) )
        return false;

    wxString cmd = aCmd.Mid( 3 ).Trim().Trim( false );

    wxStringTokenizer tokens( cmd );

    size_t num = tokens.CountTokens();

    if( num != 4 && num != 8 )
        return false;

    aSource1->m_source = tokens.GetNextToken();
    aSource1->m_vstart = SPICE_VALUE( tokens.GetNextToken() );
    aSource1->m_vend = SPICE_VALUE( tokens.GetNextToken() );
    aSource1->m_vincrement = SPICE_VALUE( tokens.GetNextToken() );

    if( num == 8 )
    {
        aSource2->m_source = tokens.GetNextToken();
        aSource2->m_vstart = SPICE_VALUE( tokens.GetNextToken() );
        aSource2->m_vend = SPICE_VALUE( tokens.GetNextToken() );
        aSource2->m_vincrement = SPICE_VALUE( tokens.GetNextToken() );
    }

    return true;
}


void NGSPICE_CIRCUIT_MODEL::WriteDirectives( OUTPUTFORMATTER& aFormatter,
                                             unsigned         aNetlistOptions ) const
{
    NETLIST_EXPORTER_SPICE::WriteDirectives( aFormatter, aNetlistOptions );
    aFormatter.Print( 0, "%s\n", TO_UTF8( GetSimCommand() ) );
}
