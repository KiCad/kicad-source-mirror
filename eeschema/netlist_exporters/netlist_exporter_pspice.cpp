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
    return false;
}


bool NETLIST_EXPORTER_PSPICE::Format( OUTPUTFORMATTER* formatter, int aCtl )
{
    int                 ret = 0;
    std::vector<int>    pinSequence;                    // numeric indices into m_SortedComponentPinList
    wxArrayString       stdPinNameArray;                // Array containing Standard Pin Names
    const wxString      delimiters( "{:,; }" );

    // Prepare list of nets generation (not used here, but...
    for( unsigned ii = 0; ii < m_masterList->size(); ii++ )
        m_masterList->GetItem( ii )->m_Flag = 0;

    SCH_SHEET_LIST sheetList( g_RootSheet );
    std::vector<wxString> directives;

    formatter->Print( 0, ".title KiCad schematic\n" );

    m_netMap.clear();

    // Ground net has to be always assigned to node 0
    m_netMap["GND"] = 0;

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
                    if( m_paths && directive.StartsWith( ".inc" ) )
                    {
                        wxString file( directive.AfterFirst( ' ' ) );
                        wxString path( m_paths->FindValidPath( file ) );
                        directives.push_back( wxString( ".include " ) + path );
                    }
                    else
                    {
                        directives.push_back( directive );
                    }
                }
            }
        }
    }

    m_ReferencesAlreadyFound.Clear();

    int curNetIndex = 1;

    for( unsigned sheet_idx = 0; sheet_idx < sheetList.size(); sheet_idx++ )
    {
        // Process component attributes to find Spice directives
        for( EDA_ITEM* item = sheetList[sheet_idx].LastDrawList(); item; item = item->Next() )
        {
            SCH_COMPONENT* comp = findNextComponentAndCreatePinList( item, &sheetList[sheet_idx] );

            if( !comp )
                break;

            item = comp;

            // Reset NodeSeqIndex Count:
            pinSequence.clear();

            // Obtain Spice fields
            SCH_FIELD* spicePrimitiveType = comp->FindField( wxT( "Spice_Primitive" ) );
            SCH_FIELD* spiceModel = comp->FindField( wxT( "Spice_Model" ) );
            SCH_FIELD* netlistEnabledField = comp->FindField( wxT( "Spice_Netlist_Enabled" ) );
            SCH_FIELD* spiceSeqField = comp->FindField( wxT( "Spice_Node_Sequence" ) );

            wxString model = spiceModel ? spiceModel->GetText()
                : GetSpiceFieldDefVal( "Spice_Model", comp );

            wxString primType = spicePrimitiveType ? spicePrimitiveType->GetText()
                : GetSpiceFieldDefVal( "Spice_Primitive", comp );

            const wxString& RefName = comp->GetRef( &sheetList[sheet_idx] );

            // Check to see if component should be removed from Spice Netlist:
            if( netlistEnabledField )
            {
                wxString netlistEnabled = netlistEnabledField->GetText();

                if( netlistEnabled.CmpNoCase( "N" ) == 0
                        || netlistEnabled.CmpNoCase( "F" ) == 0
                        || netlistEnabled == "0" )
                    continue;
            }

            // Check if an alternative pin sequence is available:
            if( spiceSeqField )
            {
                // Get the string containing the sequence of nodes:
                wxString nodeSeqIndexLineStr = spiceSeqField->GetText();

                // Verify field exists and is not empty:
                if( !nodeSeqIndexLineStr.IsEmpty() )
                {
                    // Create an array of standard pin names from part definition:
                    stdPinNameArray.Clear();

                    for( unsigned ii = 0; ii < m_SortedComponentPinList.size(); ii++ )
                    {
                        NETLIST_OBJECT* pin = m_SortedComponentPinList[ii];

                        if( !pin )
                            continue;

                        stdPinNameArray.Add( pin->GetPinNumText() );
                    }

                    // Get Alt Pin Name Array From User:
                    wxStringTokenizer tkz( nodeSeqIndexLineStr, delimiters );

                    while( tkz.HasMoreTokens() )
                    {
                        wxString    pinIndex = tkz.GetNextToken();
                        int         seq;

                        // Find PinName In Standard List assign Standard List Index to Name:
                        seq = stdPinNameArray.Index( pinIndex );

                        if( seq != wxNOT_FOUND )
                            pinSequence.push_back( seq );
                    }
                }
            }

            int activePinIndex = 0;

            formatter->Print( 0, "%s%s ", (const char*) primType.c_str(), (const char*) RefName.c_str() );

            for( unsigned ii = 0; ii < m_SortedComponentPinList.size(); ii++ )
            {
                // Case of Alt Sequence definition with Unused/Invalid Node index:
                // Valid used Node Indexes are in the set
                // {0,1,2,...m_SortedComponentPinList.size()-1}
                if( pinSequence.size() )
                {
                    // All Vector values must be less <= max package size
                    // And Total Vector size should be <= package size
                    if( ( (unsigned) pinSequence[ii] < m_SortedComponentPinList.size() )
                      && ( ii < pinSequence.size() ) )
                    {
                        // Case of Alt Pin Sequence in control good Index:
                        activePinIndex = pinSequence[ii];
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

                NETLIST_OBJECT* pin = m_SortedComponentPinList[activePinIndex];

                if( !pin )
                    continue;

                wxString netName = pin->GetNetName();
                int netIdx;

                // Assign a node number (associated with net)
                if( m_netMap.find( netName ) == m_netMap.end() )
                {
                    netIdx = curNetIndex++;
                    m_netMap[netName] = netIdx;
                } else {
                    netIdx = m_netMap[netName];
                }

// TODO remove?
                //printf("net %s index %d\n", (const char*)netName.c_str(), netIdx);
//                sprintPinNetName( netName , wxT( "N-%.6d" ), pin, aUseNetcodeAsNetName );

                //Replace parenthesis with underscore to prevent parse issues with Simulators:
//                netName.Replace( wxT( "(" ), wxT( "_" ) );
//                netName.Replace( wxT( ")" ), wxT( "_" ) );

//                if( netName.IsEmpty() )
//                    netName = wxT( "?" );

//                ret |= fprintf( f, " %s", TO_UTF8( netName ) );

                formatter->Print( 0, "%d ", netIdx );
            }

            formatter->Print( 0, "%s\n", (const char*) model.c_str() );
        }
    }

    // Print out all directives found in the text fields on the schematics
    for( auto& dir : directives )
    {
        formatter->Print( 0, "%s\n", (const char*) dir.c_str() );
    }

    formatter->Print( -1, ".end\n" );

// TODO remove?
#if 0
    m_SortedComponentPinList.clear();

    // Print texts starting with [+]pspice or [+]gnucap
    nbitems = spiceCommandAtEndFile.GetCount();

    if( nbitems )
    {
        ret |= fprintf( f, "\n" );
        spiceCommandAtEndFile.Sort();

        for( int ii = 0; ii < nbitems; ii++ )
        {
            spiceCommandAtEndFile[ii].Remove( 0, +BUFYPOS_LEN );
            spiceCommandAtEndFile[ii].Trim( true );
            spiceCommandAtEndFile[ii].Trim( false );
            ret |= fprintf( f, "%s\n", TO_UTF8( spiceCommandAtEndFile[ii] ) );
        }
    }

    ret |= fprintf( f, "\n.end\n" );
    fclose( f );
#endif

    return ret >= 0;
}


wxString NETLIST_EXPORTER_PSPICE::GetSpiceFieldDefVal( const wxString& aField,
        SCH_COMPONENT* aComponent )
{
    if( aField == "Spice_Primitive" )
    {
        const wxString& refName = aComponent->GetField( REFERENCE )->GetText();

        // Convert ICs to subcircuits
        if( refName.StartsWith( "IC" ) || refName.StartsWith( "U" ) )
            return wxString( "X" );
        else
            return refName.GetChar( 0 );
    }

    if( aField == "Spice_Model" )
    {
        wxChar prim = aComponent->GetField( REFERENCE )->GetText().GetChar( 0 );
        wxString value = aComponent->GetField( VALUE )->GetText();

        // Is it a passive component?
        if( prim == 'C' || prim == 'L' || prim == 'R' )
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

                wxLogDebug( "Changed passive value: %s..", value );
                value = prefix + unit + suffix;
                wxLogDebug( "..to: %s", value );
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

const std::vector<wxString> NETLIST_EXPORTER_PSPICE::m_spiceFields = {
    "Spice_Primitive",
    "Spice_Model",
    "Spice_Netlist_Enabled",
    "Spice_Node_Sequence"
};
