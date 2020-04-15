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
#include <wx/regex.h>
#include <wx/tokenzr.h>

wxString NETLIST_EXPORTER_PSPICE_SIM::ComponentToVector(
        const wxString& aName, SIM_PLOT_TYPE aType, const wxString& aParam ) const
{
    wxString res;

    // Some of the flags should exclude mutually
    assert( ( ( aType & SPT_VOLTAGE ) == 0 ) != ( ( aType & SPT_CURRENT ) == 0 ) );
    assert( ( ( aType & SPT_AC_PHASE ) == 0 ) || ( ( aType & SPT_AC_MAG ) == 0 ) );

    if( aType & SPT_VOLTAGE )
    {
        // netnames are escaped (can contain "{slash}" for '/') Unscape them:
        wxString spicenet = UnescapeString( aName );

        // Spice netlist netnames does not accept some chars, which are replaced
        // by eeschema netlist generator.
        // Replace these forbidden chars to find the actual spice net name
        NETLIST_EXPORTER_PSPICE::ReplaceForbiddenChars( spicenet );

        return wxString::Format( "V(%s)", spicenet );
    }

    else if( aType & SPT_CURRENT )
    {
        wxString device = GetSpiceDevice( aName ).Lower();
        wxString param = aParam.Lower();

        if( device[0] == 'x' )
        {
            return "current probe of .subckt not yet implemented";
        }
        else
        {
            return wxString::Format( "@%s[%s]",
                                     device,
                                     param.IsEmpty() ? "i" : param );
        }
    }

    return res;
}


SIM_PLOT_TYPE NETLIST_EXPORTER_PSPICE_SIM::VectorToSignal(
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


const std::vector<wxString>& NETLIST_EXPORTER_PSPICE_SIM::GetCurrents( SPICE_PRIMITIVE aPrimitive )
{
    static const std::vector<wxString> passive = { "I" };
    static const std::vector<wxString> diode = { "Id" };
    static const std::vector<wxString> bjt = { "Ib", "Ic", "Ie" };
    static const std::vector<wxString> mos = { "Ig", "Id", "Is" };
    static const std::vector<wxString> empty;

    switch( aPrimitive )
    {
        case SP_RESISTOR:
        case SP_CAPACITOR:
        case SP_INDUCTOR:
        case SP_VSOURCE:
            return passive;

        case SP_DIODE:
            return diode;

        case SP_BJT:
            return bjt;

        case SP_MOSFET:
            return mos;

        default:
            return empty;
    }
}


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


wxString NETLIST_EXPORTER_PSPICE_SIM::GetUsedSimCommand()
{
    return m_simCommand.IsEmpty() ? GetSheetSimCommand() : m_simCommand;
}


SIM_TYPE NETLIST_EXPORTER_PSPICE_SIM::GetSimType()
{
    return CommandToSimType( GetUsedSimCommand() );
}


SIM_TYPE NETLIST_EXPORTER_PSPICE_SIM::CommandToSimType( const wxString& aCmd )
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


bool NETLIST_EXPORTER_PSPICE_SIM::ParseDCCommand( const wxString& aCmd, SPICE_DC_PARAMS* aSource1,
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


void NETLIST_EXPORTER_PSPICE_SIM::writeDirectives( OUTPUTFORMATTER* aFormatter, unsigned aCtl ) const
{
    // Add a directive to obtain currents
    //aFormatter->Print( 0, ".options savecurrents\n" );        // does not work :(

    for( const auto& item : GetSpiceItems() )
    {
        for( const auto& current :
                NETLIST_EXPORTER_PSPICE_SIM::GetCurrents( (SPICE_PRIMITIVE) item.m_primitive ) )
        {
            if( !item.m_enabled )
                continue;

            /// @todo is it required to switch to lowercase
            aFormatter->Print( 0, ".save %s\n",
                    (const char*) ComponentToVector( item.m_refName, SPT_CURRENT, current )
                            .c_str() );
        }
    }

    // If we print out .save directives for currents, then it needs to be done for voltages as well
    for( const auto& netMap : GetNetIndexMap() )
    {
        // the "0" and the "GND" nets are automaticallly saved internally by ngspice.
        // Skip them
        wxString netname = ComponentToVector( netMap.first, SPT_VOLTAGE );

        if( netname == "V(0)" || netname == "V(GND)" )
            continue;

        aFormatter->Print( 0, ".save %s\n", (const char*) netname.c_str() );
    }

    if( m_simCommand.IsEmpty() )
    {
        // Fallback to the default behavior and just write all directives
        NETLIST_EXPORTER_PSPICE::writeDirectives( aFormatter, aCtl );
    }
    else
    {
        // Dump all directives but simulation commands
        for( const auto& dir : GetDirectives() )
        {
            if( !IsSimCommand( dir ) )
                aFormatter->Print( 0, "%s\n", (const char*) dir.c_str() );
        }

        // Finish with our custom simulation command
        aFormatter->Print( 0, "%s\n", (const char*) m_simCommand.c_str() );
    }
}
