/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016-2022 CERN
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.TXT for contributors.
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

#include "ngspice_circuit_model.h"
#include <macros.h>     // for TO_UTF8 def
#include <wx/regex.h>
#include <wx/tokenzr.h>
#include <locale_io.h>


SIM_TRACE_TYPE NGSPICE_CIRCUIT_MODEL::VectorToSignal( const std::string& aVector,
                                                      wxString& aSignal ) const
{
    static wxString BRANCH( wxS( "#branch" ) );
    // See ngspice manual chapt. 31.1 "Accessing internal device parameters"
    static wxRegEx internalDevParameter( wxS( "^@(\\w*[\\.\\w+]*)\\[(\\w*)\\]$" ), wxRE_ADVANCED );

    wxString vector( aVector );

    if( !internalDevParameter.Matches( vector ) )
    {
        if( vector.EndsWith( BRANCH ) )
        {
            aSignal = wxT( "I(" ) + vector.Left( vector.Length() - BRANCH.Length() ) + wxT( ")" );
            return SPT_CURRENT;
        }
        else
        {
            aSignal = wxT( "V(" ) + vector + wxT( ")" );
            return SPT_VOLTAGE;
        }
    }
    else
    {
        wxString paramType = internalDevParameter.GetMatch( vector, 2 );

        if( paramType.Lower()[0] == 'i' )
        {
            // this is a branch current
            paramType[0] = 'I';
            aSignal      = paramType + wxT( "(" );
            aSignal += internalDevParameter.GetMatch( vector, 1 ).Upper() + wxT( ")" );
            return SPT_CURRENT;
        }
        else
        {
            return SPT_UNKNOWN;
        }
    }
}


wxString NGSPICE_CIRCUIT_MODEL::GetSchTextSimCommand()
{
    wxString      simCmd;

    ReadDirectives( 0 );

    for( const wxString& directive : GetDirectives() )
    {
        if( IsSimCommand( directive ) )
            simCmd += wxString::Format( wxT( "%s\r\n" ), directive );
    }

    return simCmd;
}


SIM_TYPE NGSPICE_CIRCUIT_MODEL::GetSimType()
{
    return CommandToSimType( GetSimCommand() );
}


SIM_TYPE NGSPICE_CIRCUIT_MODEL::CommandToSimType( const wxString& aCmd )
{
    wxString cmd = aCmd.Lower();

    if( cmd.StartsWith( wxT( ".ac" ) ) )
        return ST_AC;
    else if( cmd.StartsWith( wxT( ".dc" ) ) )
        return ST_DC;
    else if( cmd.StartsWith( wxT( ".tran" ) ) )
        return ST_TRANSIENT;
    else if( cmd == wxT( ".op" ) )
        return ST_OP;
    else if( cmd.StartsWith( wxT( ".disto" ) ) )
        return ST_DISTORTION;
    else if( cmd.StartsWith( wxT( ".noise" ) ) )
        return ST_NOISE;
    else if( cmd.StartsWith( wxT( ".pz" ) ) )
        return ST_POLE_ZERO;
    else if( cmd.StartsWith( wxT( ".sens" ) ) )
        return ST_SENSITIVITY;
    else if( cmd.StartsWith( wxT( ".tf" ) ) )
        return ST_TRANS_FUNC;
    else
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
    if( GetSimCommandOverride().IsEmpty() )
        aNetlistOptions |= OPTION_SIM_COMMAND;

    NETLIST_EXPORTER_SPICE::WriteDirectives( aFormatter, aNetlistOptions );

    if( !GetSimCommandOverride().IsEmpty() )
        aFormatter.Print( 0, "%s\n", TO_UTF8( GetSimCommandOverride() ) );
}
