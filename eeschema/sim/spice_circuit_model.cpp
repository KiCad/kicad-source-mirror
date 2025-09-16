/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016-2022 CERN
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#include "spice_circuit_model.h"

#include <wx/regex.h>
#include <wx/tokenzr.h>

#include <locale_io.h>
#include <richio.h>
#include <string_utils.h>

SIM_TRACE_TYPE SPICE_CIRCUIT_MODEL::VectorToSignal( const std::string& aVector,
                                                    wxString& aSignal ) const
{
    static wxString BRANCH( wxS( "#branch" ) );
    static wxString POWER( wxS( ":power" ) );

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
        else if( vector.EndsWith( POWER ) )
        {
            aSignal = wxT( "P(" ) + vector.Left( vector.Length() - POWER.Length() ) + wxT( ")" );
            return SPT_POWER;
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


wxString SPICE_CIRCUIT_MODEL::GetSchTextSimCommand()
{
    wxString      simCmd;

    ReadDirectives( 0 );

    for( const wxString& directive : GetDirectives() )
    {
        if( IsSimCommand( directive ) )
            simCmd += wxString::Format( wxT( "%s\r\n" ), directive );
    }

    return simCmd.Trim();
}


SIM_TYPE SPICE_CIRCUIT_MODEL::CommandToSimType( const wxString& aCmd )
{
    wxString cmd = aCmd.Lower().Trim();

    if( cmd == wxT( ".op" ) )                    return ST_OP;
    else if( cmd.StartsWith( wxT( ".ac" ) ) )    return ST_AC;
    else if( cmd.StartsWith( wxT( ".dc" ) ) )    return ST_DC;
    else if( cmd.StartsWith( wxT( ".tran" ) ) )  return ST_TRAN;
    else if( cmd.StartsWith( wxT( ".disto" ) ) ) return ST_DISTO;
    else if( cmd.StartsWith( wxT( ".noise" ) ) ) return ST_NOISE;
    else if( cmd.StartsWith( wxT( ".pz" ) ) )    return ST_PZ;
    else if( cmd.StartsWith( wxT( ".sens" ) ) )  return ST_SENS;
    else if( cmd.StartsWith( wxT( ".sp" ) ) )    return ST_SP;
    else if( cmd.StartsWith( wxT( ".tf" ) ) )    return ST_TF;

    else if( cmd.StartsWith( wxT( "fft" ) ) || cmd.Contains( wxT( "\nfft" ) ) )
        return ST_FFT;
    else
        return ST_UNKNOWN;
}


bool SPICE_CIRCUIT_MODEL::ParseDCCommand( const wxString& aCmd, SPICE_DC_PARAMS* aSource1,
                                          SPICE_DC_PARAMS* aSource2 )
{
    if( !aCmd.Lower().StartsWith( ".dc" ) )
        return false;

    wxString          cmd = aCmd.Mid( 3 );
    wxStringTokenizer tokens( cmd, " \t", wxTOKEN_STRTOK );

    aSource1->m_source = tokens.GetNextToken();
    aSource1->m_vstart = SPICE_VALUE( tokens.GetNextToken() );
    aSource1->m_vend = SPICE_VALUE( tokens.GetNextToken() );
    aSource1->m_vincrement = SPICE_VALUE( tokens.GetNextToken() );

    if( tokens.HasMoreTokens() )
    {
        aSource2->m_source = tokens.GetNextToken();
        aSource2->m_vstart = SPICE_VALUE( tokens.GetNextToken() );
        aSource2->m_vend = SPICE_VALUE( tokens.GetNextToken() );
        aSource2->m_vincrement = SPICE_VALUE( tokens.GetNextToken() );
    }

    return true;
}


bool SPICE_CIRCUIT_MODEL::ParsePZCommand( const wxString& aCmd, wxString* transferFunction,
                                          wxString* input, wxString* inputRef, wxString* output,
                                          wxString* outputRef, SPICE_PZ_ANALYSES* analyses )
{
    if( !aCmd.Lower().StartsWith( wxS( ".pz" ) ) )
        return false;

    *transferFunction = "vol";
    analyses->m_Poles = true;
    analyses->m_Zeros = true;

    wxStringTokenizer tokens( aCmd.Mid( 3 ), " \t", wxTOKEN_STRTOK );

    if( tokens.HasMoreTokens() )
        *input = tokens.GetNextToken();

    if( tokens.HasMoreTokens() )
        *inputRef = tokens.GetNextToken();

    if( tokens.HasMoreTokens() )
        *output = tokens.GetNextToken();

    if( tokens.HasMoreTokens() )
        *outputRef = tokens.GetNextToken();

    if( tokens.HasMoreTokens() )
        *transferFunction = tokens.GetNextToken();

    if( tokens.HasMoreTokens() )
    {
        wxString token = tokens.GetNextToken().Lower();

        if( token == wxS( "pol" ) )
            analyses->m_Zeros = false;
        else if( token == wxS( "zer" ) )
            analyses->m_Poles = false;
    }

    return true;
}

bool SPICE_CIRCUIT_MODEL::ParseNoiseCommand( const wxString& aCmd, wxString* aOutput,
                                             wxString* aRef, wxString* aSource, wxString* aScale,
                                             SPICE_VALUE* aPts, SPICE_VALUE* aFStart,
                                             SPICE_VALUE* aFStop, bool* aSaveAll )
{
    if( !aCmd.Lower().StartsWith( wxS( ".noise" ) ) )
        return false;

    wxString cmd = aCmd.Mid( 6 );

    cmd.Trim( false );

    if( !cmd.Lower().StartsWith( wxS( "v(" ) ) )
        return false;

    cmd = cmd.Mid( 2 );

    wxString function = cmd.Before( ')' );
    wxString params = cmd.After( ')' );

    wxStringTokenizer func_tokens( function, " ,\t", wxTOKEN_STRTOK );

    *aOutput = func_tokens.GetNextToken();
    *aRef = func_tokens.GetNextToken();

    wxStringTokenizer tokens( params, " \t", wxTOKEN_STRTOK );
    wxString          token = tokens.GetNextToken();

    if( !token.IsEmpty() )
    {
        *aSource = token;
        token = tokens.GetNextToken();
    }

    if( token.Lower() == "dec" || token.Lower() == "oct" || token.Lower() == "lin" )
    {
        *aScale = token;
        token = tokens.GetNextToken();
    }

    if( !token.IsEmpty() )
    {
        *aPts = token;
        token = tokens.GetNextToken();
    }

    if( !token.IsEmpty() )
    {
        *aFStart = SPICE_VALUE( token );
        token = tokens.GetNextToken();
    }

    if( !token.IsEmpty() )
    {
        *aFStop = SPICE_VALUE( token );
        token = tokens.GetNextToken();
    }

    if( !token.IsEmpty() )
        *aSaveAll = true;

    return true;
}


void SPICE_CIRCUIT_MODEL::WriteDirectives( const wxString& aSimCommand, unsigned aSimOptions,
                                           OUTPUTFORMATTER& aFormatter ) const
{
    if( aSimCommand.IsEmpty() )
        aSimOptions |= OPTION_SIM_COMMAND;

    NETLIST_EXPORTER_SPICE::WriteDirectives( aSimCommand, aSimOptions, aFormatter );

    if( !aSimCommand.IsEmpty() )
        aFormatter.Print( 0, "%s\n", TO_UTF8( aSimCommand ) );
}
