/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2013 jp.charras at wanadoo.fr
 * Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.TXT for contributors.
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

#include "netlist_exporter_pspice.h"
#include <fctsys.h>
#include <build_version.h>
#include <confirm.h>

#include <map>
#include <search_stack.h>

#include <schframe.h>
#include <netlist.h>
#include <sch_reference_list.h>
#include <class_netlist_object.h>

#include <wx/tokenzr.h>
#include <wx/regex.h>

bool NETLIST_EXPORTER_PSPICE::WriteNetlist( const wxString& aOutFileName, unsigned aNetlistOptions )
{
    FILE_OUTPUTFORMATTER outputFile( aOutFileName, wxT( "wt" ), '\'' );

    return Format( &outputFile, aNetlistOptions );
}


bool NETLIST_EXPORTER_PSPICE::Format( OUTPUTFORMATTER* aFormatter, unsigned aCtl )
{
    // Netlist options
    const bool useNetcodeAsNetName = aCtl & NET_USE_NETCODES_AS_NETNAMES;

    ProcessNetlist( aCtl );

    aFormatter->Print( 0, ".title KiCad schematic\n" );

    for( const auto& item : m_spiceItems )
    {
        aFormatter->Print( 0, "%c%s ", item.m_primitive, (const char*) item.m_refName.c_str() );

        // Pins to node mapping
        int activePinIndex = 0;

        for( unsigned ii = 0; ii < item.m_pins.size(); ii++ )
        {
            // Case of Alt Sequence definition with Unused/Invalid Node index:
            // Valid used Node Indexes are in the set
            // {0,1,2,...m_item.m_pin.size()-1}
            if( !item.m_pinSequence.empty() )
            {
                // All Vector values must be less <= max package size
                // And Total Vector size should be <= package size
                if( ( (unsigned) item.m_pinSequence[ii] < item.m_pins.size() )
                    && ( ii < item.m_pinSequence.size() ) )
                {
                    // Case of Alt Pin Sequence in control good Index:
                    activePinIndex = item.m_pinSequence[ii];
                }
                else
                {
                    // Case of Alt Pin Sequence in control Bad Index or not using all
                    // pins for simulation:
                    wxASSERT_MSG( false, "Used an invalid pin number in node sequence" );
                    continue;
                }
            }
            // Case of Standard Pin Sequence in control:
            else
            {
                activePinIndex = ii;
            }

            NETLIST_OBJECT* pin = item.m_pins[activePinIndex];
            assert( pin );
            wxString netName = pin->GetNetName();

            if( useNetcodeAsNetName )
            {
                assert( m_netMap.count( netName ) );
                aFormatter->Print( 0, "%d ", m_netMap[netName] );
            }
            else
            {
                sprintPinNetName( netName , wxT( "N-%.6d" ), pin, useNetcodeAsNetName );

                //Replace parenthesis with underscore to prevent parse issues with simulators
                netName.Replace( wxT( "(" ), wxT( "_" ) );
                netName.Replace( wxT( ")" ), wxT( "_" ) );

                if( netName.IsEmpty() )
                    netName = wxT( "?" );

                aFormatter->Print( 0, "%s ", TO_UTF8( netName ) );
            }
        }

        aFormatter->Print( 0, "%s\n", (const char*) item.m_model.c_str() );
    }

    // Print out all directives found in the text fields on the schematics
    writeDirectives( aFormatter, aCtl );

    aFormatter->Print( -1, ".end\n" );

    return true;
}


wxString NETLIST_EXPORTER_PSPICE::GetSpiceFieldDefVal( const wxString& aField,
        SCH_COMPONENT* aComponent, unsigned aCtl )
{
    if( aField == "Spice_Primitive" )
    {
        const wxString& refName = aComponent->GetField( REFERENCE )->GetText();

        // Convert ICs to subcircuits
        if( aCtl & NET_USE_X_PREFIX && ( refName.StartsWith( "IC" ) || refName.StartsWith( "U" ) ) )
            return wxString( "X" );
        else
            return refName.GetChar( 0 );
    }

    if( aField == "Spice_Model" )
    {
        wxChar prim = aComponent->GetField( REFERENCE )->GetText().GetChar( 0 );
        wxString value = aComponent->GetField( VALUE )->GetText();

        // Is it a passive component?
        if( aCtl & NET_ADJUST_PASSIVE_VALS && ( prim == 'C' || prim == 'L' || prim == 'R' ) )
        {
            // Regular expression to match common formats used for passive parts description
            // (e.g. 100k, 2k3, 1 uF)
            wxRegEx passiveVal( "^([0-9\\. ]+)([fFpPnNuUmMkKgGtT]|M(e|E)(g|G))?([fFhH]|ohm)?([-1-9 ]*)$" );

            if( passiveVal.Matches( value ) )
            {
                wxString prefix( passiveVal.GetMatch( value, 1 ) );
                wxString unit( passiveVal.GetMatch( value, 2 ) );
                wxString suffix( passiveVal.GetMatch( value, 6 ) );

                prefix.Trim(); prefix.Trim( false );
                unit.Trim(); unit.Trim( false );
                suffix.Trim(); suffix.Trim( false );

                // Make 'mega' units comply with the Spice expectations
                if( unit == "M" )
                    unit = "Meg";

                value = prefix + unit + suffix;
            }
        }

        return value;
    }

    if( aField == "Spice_Netlist_Enabled" )
    {
        return wxString( "Y" );
    }

    if( aField == "Spice_Node_Sequence" )
    {
        wxString nodeSeq;
        std::vector<LIB_PIN*> pins;

        aComponent->GetPins( pins );

        for( auto pin : pins )
            nodeSeq += pin->GetNumberString() + " ";

        nodeSeq.Trim();

        return nodeSeq;
    }

    wxASSERT_MSG( "Missing default value definition for a Spice field: %s" , aField );

    return wxString( "<unknown>" );
}


void NETLIST_EXPORTER_PSPICE::ProcessNetlist( unsigned aCtl )
{
    const wxString      delimiters( "{:,; }" );
    SCH_SHEET_LIST      sheetList( g_RootSheet );

    // Prepare list of nets generation (not used here, but...
    for( unsigned ii = 0; ii < m_masterList->size(); ii++ )
        m_masterList->GetItem( ii )->m_Flag = 0;

    UpdateDirectives( aCtl );

    m_netMap.clear();

    // 0 is reserved for "GND"
    m_netMap["GND"] = 0;
    int netIdx = 1;

    m_ReferencesAlreadyFound.Clear();

    for( unsigned sheet_idx = 0; sheet_idx < sheetList.size(); sheet_idx++ )
    {
        // Process component attributes to find Spice directives
        for( EDA_ITEM* item = sheetList[sheet_idx].LastDrawList(); item; item = item->Next() )
        {
            SCH_COMPONENT* comp = findNextComponentAndCreatePinList( item, &sheetList[sheet_idx] );

            if( !comp )
                break;

            item = comp;

            SPICE_ITEM spiceItem;
            spiceItem.m_parent = comp;

            // Obtain Spice fields
            SCH_FIELD* fieldPrim = comp->FindField( wxT( "Spice_Primitive" ) );
            SCH_FIELD* fieldModel = comp->FindField( wxT( "Spice_Model" ) );
            SCH_FIELD* fieldEnabled = comp->FindField( wxT( "Spice_Netlist_Enabled" ) );
            SCH_FIELD* fieldSeq = comp->FindField( wxT( "Spice_Node_Sequence" ) );

            spiceItem.m_primitive = fieldPrim ? fieldPrim->GetText()[0]
                : GetSpiceFieldDefVal( "Spice_Primitive", comp, aCtl )[0];

            spiceItem.m_model = fieldModel ? fieldModel->GetText()
                : GetSpiceFieldDefVal( "Spice_Model", comp, aCtl );

            spiceItem.m_refName = comp->GetRef( &sheetList[sheet_idx] );

            // Check to see if component should be removed from Spice netlist
                spiceItem.m_enabled = fieldEnabled ? StringToBool( fieldEnabled->GetText() ) : true;

            wxArrayString pinNames;

            // Store pin information
            for( unsigned ii = 0; ii < m_SortedComponentPinList.size(); ii++ )
            {
                NETLIST_OBJECT* pin = m_SortedComponentPinList[ii];
                assert( pin );
                spiceItem.m_pins.push_back( pin );
                pinNames.Add( pin->GetPinNumText() );

                // Create net mapping
                const wxString& netName = pin->GetNetName();
                if( m_netMap.count( netName ) == 0 )
                    m_netMap[netName] = netIdx++;
            }

            // Check if an alternative pin sequence is available:
            if( fieldSeq )
            {
                // Get the string containing the sequence of nodes:
                wxString nodeSeqIndexLineStr = fieldSeq->GetText();

                // Verify field exists and is not empty:
                if( !nodeSeqIndexLineStr.IsEmpty() )
                {
                    // Get Alt Pin Name Array From User:
                    wxStringTokenizer tkz( nodeSeqIndexLineStr, delimiters );

                    while( tkz.HasMoreTokens() )
                    {
                        wxString    pinIndex = tkz.GetNextToken();
                        int         seq;

                        // Find PinName In Standard List assign Standard List Index to Name:
                        seq = pinNames.Index( pinIndex );

                        if( seq != wxNOT_FOUND )
                            spiceItem.m_pinSequence.push_back( seq );
                    }
                }
            }

            m_spiceItems.push_back( spiceItem );
        }
    }
}


void NETLIST_EXPORTER_PSPICE::UpdateDirectives( unsigned aCtl )
{
    const SCH_SHEET_LIST& sheetList = g_RootSheet;

    m_directives.clear();

    for( unsigned i = 0; i < sheetList.size(); i++ )
    {
        for( EDA_ITEM* item = sheetList[i].LastDrawList(); item; item = item->Next() )
        {
            if( item->Type() != SCH_TEXT_T )
                continue;

            wxString text = static_cast<SCH_TEXT*>( item )->GetText();

            if( text.IsEmpty() )
                continue;

            if( text.GetChar( 0 ) == '.' )
            {
                wxStringTokenizer tokenizer( text, "\r\n" );

                while( tokenizer.HasMoreTokens() )
                {
                    wxString directive( tokenizer.GetNextToken() );

                    // Fix paths for .include directives
                    if( aCtl & NET_ADJUST_INCLUDE_PATHS && m_paths && directive.StartsWith( ".inc" ) )
                    {
                        wxString file( directive.AfterFirst( ' ' ) );
                        wxString path( m_paths->FindValidPath( file ) );
                        m_directives.push_back( wxString( ".include " ) + path );
                    }
                    else
                    {
                        m_directives.push_back( directive );
                    }
                }
            }
        }
    }
}


void NETLIST_EXPORTER_PSPICE::writeDirectives( OUTPUTFORMATTER* aFormatter, unsigned aCtl ) const
{
    for( auto& dir : m_directives )
    {
        aFormatter->Print( 0, "%s\n", (const char*) dir.c_str() );
    }
}


const std::vector<wxString> NETLIST_EXPORTER_PSPICE::m_spiceFields = {
    "Spice_Primitive",
    "Spice_Model",
    "Spice_Netlist_Enabled",
    "Spice_Node_Sequence"
};
