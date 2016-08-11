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

#include <fctsys.h>
#include <build_version.h>
#include <confirm.h>

#include <map>

#include <schframe.h>
#include <netlist.h>
#include <sch_reference_list.h>
#include <class_netlist_object.h>
#include <wx/tokenzr.h>
#include "netlist_exporter_pspice.h"

bool NETLIST_EXPORTER_PSPICE::WriteNetlist( const wxString& aOutFileName, unsigned aNetlistOptions )
{
    return false;
}


bool NETLIST_EXPORTER_PSPICE::Format( OUTPUTFORMATTER* formatter, int aCtl )
{
    bool aUsePrefix = aCtl & NET_USE_X_PREFIX;

    int                 ret = 0;
    int                 nbitems;
    wxString            text;
    wxArrayString       spiceCommandAtBeginFile;
    wxArrayString       spiceCommandAtEndFile;
    wxString            msg;
    wxString            netName;

    #define BUFYPOS_LEN 4
    wxChar              bufnum[BUFYPOS_LEN + 1];
    std::vector<int>    pinSequence;                    // numeric indices into m_SortedComponentPinList
    wxArrayString       stdPinNameArray;                // Array containing Standard Pin Names
    wxString            delimeters = wxT( "{:,; }" );
    wxString            disableStr = wxT( "N" );

    //std::map<wxString, int> netIndices;

    // Prepare list of nets generation (not used here, but...
    for( unsigned ii = 0; ii < m_masterList->size(); ii++ )
        m_masterList->GetItem( ii )->m_Flag = 0;

    // Create text list starting by [.-]pspice , or [.-]gnucap (simulator
    // commands) and create text list starting by [+]pspice , or [+]gnucap
    // (simulator commands)
    bufnum[BUFYPOS_LEN] = 0;
    SCH_SHEET_LIST sheetList( g_RootSheet );

    std::vector<wxString> directives;

    formatter->Print(0, "Kicad schematic\n");

    m_probes.clear();
    m_netMap.clear();
    m_netMap["GND"] = 0;

    for( unsigned i = 0; i < sheetList.size(); i++ )
    {
        for( EDA_ITEM* item = sheetList[i].LastDrawList(); item; item = item->Next() )
        {
            size_t l1, l2;
            wxChar ident;

            if( item->Type() != SCH_TEXT_T )
                continue;

            SCH_TEXT*   drawText = (SCH_TEXT*) item;

            text = drawText->GetText();

            if( text.IsEmpty() )
                continue;

            ident = text.GetChar( 0 );

            if( ident == '.' )
            {
                    printf("Directive found: '%s'\n", (const char *) text.c_str());
                    directives.push_back(text);
            }
        }
    }


    m_ReferencesAlreadyFound.Clear();


    int curNetIndex = 1;

    for( unsigned sheet_idx = 0; sheet_idx < sheetList.size(); sheet_idx++ )
    {
        //printf( "* Sheet Name: %s\n",
        //                TO_UTF8( sheetList[sheet_idx].PathHumanReadable() ) );

        for( EDA_ITEM* item = sheetList[sheet_idx].LastDrawList(); item; item = item->Next() )
        {
            SCH_COMPONENT* comp = findNextComponentAndCreatePinList( item, &sheetList[sheet_idx] );

            if( !comp )
                break;

            item = comp;

            // Reset NodeSeqIndex Count:
            pinSequence.clear();

            SCH_FIELD*   spicePrimitiveType = comp->FindField( wxT( "Spice_Primitive" ) );
            SCH_FIELD*   spiceModel = comp->FindField( wxT( "Spice_Model" ) );

            wxString RefName = comp->GetRef( &sheetList[sheet_idx] );
            wxString CompValue = comp->GetField( VALUE )->GetText();

            wxString model("");
            wxString primType ("X");

            if(spicePrimitiveType)
                primType = spicePrimitiveType->GetText();
            else {
                if (RefName.StartsWith(wxT("IC")) || RefName.StartsWith("U") )
                    primType = wxT("X"); // subckt
                else
                    primType = RefName.GetChar(0);
            }

            if(spiceModel)
            {
            //    printf("model specified\n");
                model = spiceModel->GetText();
            } else {
            //    printf("no model\n");
                model = CompValue;
            }

            // Check to see if component should be removed from Spice Netlist:
            SCH_FIELD*  netlistEnabledField = comp->FindField( wxT( "Spice_Netlist_Enabled" ) );

            if( netlistEnabledField )
            {
                wxString netlistEnabled = netlistEnabledField->GetText();

                if( netlistEnabled.CmpNoCase( disableStr ) == 0 )
                    continue;
            }

            // Check if Alternative Pin Sequence is Available:
            SCH_FIELD*  spiceSeqField = comp->FindField( wxT( "Spice_Node_Sequence" ) );

            if( spiceSeqField )
            {
                // Get String containing the Sequence of Nodes:
                wxString nodeSeqIndexLineStr = spiceSeqField->GetText();

                // Verify Field Exists and is not empty:
                if( !nodeSeqIndexLineStr.IsEmpty() )
                {

                    // Create an Array of Standard Pin Names from part definition:
                    stdPinNameArray.Clear();

                    for( unsigned ii = 0; ii < m_SortedComponentPinList.size(); ii++ )
                    {
                        NETLIST_OBJECT* pin = m_SortedComponentPinList[ii];

                        if( !pin )
                            continue;

                        stdPinNameArray.Add( pin->GetPinNumText() );
                    }

                    // Get Alt Pin Name Array From User:
                    wxStringTokenizer tkz( nodeSeqIndexLineStr, delimeters );

                    while( tkz.HasMoreTokens() )
                    {
                        wxString    pinIndex = tkz.GetNextToken();
                        int         seq;

                        // Find PinName In Standard List assign Standard List Index to Name:
                        seq = stdPinNameArray.Index(pinIndex);

                        if( seq != wxNOT_FOUND )
                        {
                            pinSequence.push_back( seq );
                        }
                    }

                }
            }


            if(CompValue == wxT("SPICE_PROBE"))
            {
                NETLIST_OBJECT* pin = m_SortedComponentPinList[0];

                //printf("Probe net: %s\n", (const char*) pin->GetNetName().c_str() );

                m_probes.push_back(pin->GetNetName());
                continue;
            }

            //Conditionally add Prefix only for devices that begin with U or IC:
            if( aUsePrefix )
            {
                //if( RefName.StartsWith( wxT( "U" ) ) || RefName.StartsWith( wxT( "IC" ) ) )
                //    RefName = wxT( "X" ) + RefName;
            }

            printf( "Ref %s primType %s model/value '%s'\n", TO_UTF8( RefName ), (const char*)primType.c_str(), (const char *)model.c_str() );

            int activePinIndex = 0;

            formatter->Print(0, "%s%s ", (const char *)primType.c_str(), (const char *)RefName.c_str());

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

                if (m_netMap.find(netName) == m_netMap.end())
                {
                    netIdx = curNetIndex++;
                    m_netMap[netName] = netIdx;
                } else {
                    netIdx = m_netMap[netName];
                }

                //printf("net %s index %d\n", (const char*)netName.c_str(), netIdx);
//                sprintPinNetName( netName , wxT( "N-%.6d" ), pin, aUseNetcodeAsNetName );

                //Replace parenthesis with underscore to prevent parse issues with Simulators:
//                netName.Replace( wxT( "(" ), wxT( "_" ) );
//                netName.Replace( wxT( ")" ), wxT( "_" ) );

//                if( netName.IsEmpty() )
//                    netName = wxT( "?" );

//                ret |= fprintf( f, " %s", TO_UTF8( netName ) );

                formatter->Print(0, "%d ", netIdx );
            }

            formatter->Print(0, "%s\n",(const char *) model.c_str());


        }


    }

    for( auto dir : directives )
    {
        formatter->Print(0, "%s\n", (const char *)dir.c_str());

    }

    formatter->Print(0, ".end\n");

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
