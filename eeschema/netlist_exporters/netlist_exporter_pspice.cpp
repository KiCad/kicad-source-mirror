/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2013 jp.charras at wanadoo.fr
 * Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <schframe.h>
#include <netlist.h>
#include <sch_reference_list.h>
#include <class_netlist_object.h>
#include <wx/tokenzr.h>
#include "netlist_exporter_pspice.h"

bool NETLIST_EXPORTER_PSPICE::WriteNetlist( const wxString& aOutFileName, unsigned aNetlistOptions )
{
    FILE* f = NULL;
    bool aUsePrefix = aNetlistOptions & NET_USE_X_PREFIX;
    bool aUseNetcodeAsNetName = aNetlistOptions & NET_USE_NETCODES_AS_NETNAMES;

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

    if( ( f = wxFopen( aOutFileName, wxT( "wt" ) ) ) == NULL )
    {
        wxString msg;
        msg.Printf( _( "Failed to create file '%s'" ),
                    GetChars( aOutFileName ) );
        DisplayError( NULL, msg );
        return false;
    }

    ret |= fprintf( f, "* %s\n\n", TO_UTF8( aOutFileName ) );
    ret |= fprintf( f, "* %s (Spice format) creation date: %s\n\n",
                    NETLIST_HEAD_STRING, TO_UTF8( DateAndTime() ) );

    // Prepare list of nets generation (not used here, but...
    for( unsigned ii = 0; ii < m_masterList->size(); ii++ )
        m_masterList->GetItem( ii )->m_Flag = 0;

    ret |= fprintf( f, "* To exclude a component from the Spice Netlist add [Spice_Netlist_Enabled] user FIELD set to: N\n" );
    ret |= fprintf( f, "* To reorder the component spice node sequence add [Spice_Node_Sequence] user FIELD and define sequence: 2,1,0\n" );

    // Create text list starting by [.-]pspice , or [.-]gnucap (simulator
    // commands) and create text list starting by [+]pspice , or [+]gnucap
    // (simulator commands)
    bufnum[BUFYPOS_LEN] = 0;
    SCH_SHEET_LIST sheetList;

    for( SCH_SHEET_PATH* sheet = sheetList.GetFirst(); sheet; sheet = sheetList.GetNext() )
    {
        for( EDA_ITEM* item = sheet->LastDrawList(); item; item = item->Next() )
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

            if( ident != '.' && ident != '-' && ident != '+' )
                continue;

            text.Remove( 0, 1 );    // Remove the first char.
            text.Remove( 6 );       // text contains 6 char.
            text.MakeLower();

            if( text != wxT( "pspice" ) && text != wxT( "gnucap" ) )
                continue;

            text = drawText->GetText().Mid( 7 );
            l1 = text.Length();
            text.Trim( false );
            l2 = text.Length();

            if( l1 == l2 )
                continue;           // no whitespace after ident text

            {
                // Put the Y position as an ascii string, for sort by vertical
                // position, using usual sort string by alphabetic value
                int ypos = drawText->GetPosition().y;

                for( int ii = 0; ii < BUFYPOS_LEN; ii++ )
                {
                    bufnum[BUFYPOS_LEN - 1 - ii] = (ypos & 63) + ' ';
                    ypos >>= 6;
                }

                // First BUFYPOS_LEN char are the Y position.
                msg.Printf( wxT( "%s %s" ), bufnum, text.GetData() );

                if( ident == '+' )
                    spiceCommandAtEndFile.Add( msg );
                else
                    spiceCommandAtBeginFile.Add( msg );
            }
        }
    }

    // Print texts starting by [.-]pspice , ou [.-]gnucap (of course, without
    // the Y position string)
    nbitems = spiceCommandAtBeginFile.GetCount();

    if( nbitems )
    {
        spiceCommandAtBeginFile.Sort();

        for( int ii = 0; ii < nbitems; ii++ )
        {
            spiceCommandAtBeginFile[ii].Remove( 0, BUFYPOS_LEN );
            spiceCommandAtBeginFile[ii].Trim( true );
            spiceCommandAtBeginFile[ii].Trim( false );
            ret |= fprintf( f, "%s\n", TO_UTF8( spiceCommandAtBeginFile[ii] ) );
        }
    }
    ret |= fprintf( f, "\n" );

    //  Create component list

    m_ReferencesAlreadyFound.Clear();

    for( SCH_SHEET_PATH* sheet = sheetList.GetFirst();  sheet;  sheet = sheetList.GetNext() )
    {
        ret |= fprintf( f, "* Sheet Name: %s\n", TO_UTF8( sheet->PathHumanReadable() ) );

        for( EDA_ITEM* item = sheet->LastDrawList();  item;  item = item->Next() )
        {
            SCH_COMPONENT* comp = findNextComponentAndCreatePinList( item, sheet );

            if( !comp )
                break;

            item = comp;

            // Reset NodeSeqIndex Count:
            pinSequence.clear();

            // Check to see if component should be removed from Spice Netlist:
            SCH_FIELD*  netlistEnabledField = comp->FindField( wxT( "Spice_Netlist_Enabled" ) );

            if( netlistEnabledField )
            {
                wxString netlistEnabled = netlistEnabledField->GetText();

                if( netlistEnabled.IsEmpty() )
                    break;

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
                if( nodeSeqIndexLineStr.IsEmpty() )
                    break;

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

            //Get Standard Reference Designator:
            wxString RefName = comp->GetRef( sheet );

            //Conditionally add Prefix only for devices that begin with U or IC:
            if( aUsePrefix )
            {
                if( RefName.StartsWith( wxT( "U" ) ) || RefName.StartsWith( wxT( "IC" ) ) )
                    RefName = wxT( "X" ) + RefName;
            }

            ret |= fprintf( f, "%s ", TO_UTF8( RefName ) );

            // Write pin list:
            int activePinIndex = 0;

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

                sprintPinNetName( netName , wxT( "N-%.6d" ), pin, aUseNetcodeAsNetName );

                //Replace parenthesis with underscore to prevent parse issues with Simulators:
                netName.Replace( wxT( "(" ), wxT( "_" ) );
                netName.Replace( wxT( ")" ), wxT( "_" ) );

                if( netName.IsEmpty() )
                    netName = wxT( "?" );

                ret |= fprintf( f, " %s", TO_UTF8( netName ) );
            }

            // Get Component Value Name:
            wxString CompValue = comp->GetField( VALUE )->GetText();

            // Check if Override Model Name is Provided:
            SCH_FIELD* spiceModelField = comp->FindField( wxT( "spice_model" ) );

            if( spiceModelField )
            {
                // Get Model Name String:
                wxString ModelNameStr = spiceModelField->GetText();

                // Verify Field Exists and is not empty:
                if( !ModelNameStr.IsEmpty() )
                    CompValue = ModelNameStr;
            }

            // Print Component Value:
            ret |= fprintf( f, " %s\t\t",TO_UTF8( CompValue ) );

            // Show Seq Spec on same line as component using line-comment ";":
            for( unsigned i = 0;  i < pinSequence.size();  ++i )
            {
                if( i==0 )
                    ret |= fprintf( f, ";Node Sequence Spec.<" );

                ret |= fprintf( f, "%s", TO_UTF8( stdPinNameArray.Item( pinSequence[i] ) ) );

                if( i < pinSequence.size()-1 )
                    ret |= fprintf( f, "," );
                else
                    ret |= fprintf( f, ">" );
            }

            // Next Netlist line record:
            ret |= fprintf( f, "\n" );
        }
    }

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

    return ret >= 0;
}
