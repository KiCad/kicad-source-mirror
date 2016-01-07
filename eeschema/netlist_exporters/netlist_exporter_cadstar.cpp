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
#include <sch_reference_list.h>
#include <class_netlist_object.h>

#include "netlist_exporter_cadstar.h"

/* Generate CADSTAR net list. */
static wxString StartLine( wxT( "." ) );

bool NETLIST_EXPORTER_CADSTAR::WriteNetlist( const wxString& aOutFileName, unsigned aNetlistOptions )
{
    (void)aNetlistOptions;      //unused
    int ret = 0;
    FILE* f = NULL;

    if( ( f = wxFopen( aOutFileName, wxT( "wt" ) ) ) == NULL )
    {
        wxString msg;
        msg.Printf( _( "Failed to create file '%s'" ),
                    GetChars( aOutFileName ) );
        DisplayError( NULL, msg );
        return false;
    }

    wxString StartCmpDesc = StartLine + wxT( "ADD_COM" );
    wxString msg;
    SCH_SHEET_PATH* sheet;
    EDA_ITEM* DrawList;
    SCH_COMPONENT* component;
    wxString title = wxT( "Eeschema " ) + GetBuildVersion();

    ret |= fprintf( f, "%sHEA\n", TO_UTF8( StartLine ) );
    ret |= fprintf( f, "%sTIM %s\n", TO_UTF8( StartLine ), TO_UTF8( DateAndTime() ) );
    ret |= fprintf( f, "%sAPP ", TO_UTF8( StartLine ) );
    ret |= fprintf( f, "\"%s\"\n", TO_UTF8( title ) );
    ret |= fprintf( f, "\n" );

    // Prepare list of nets generation
    for( unsigned ii = 0; ii < m_masterList->size(); ii++ )
        m_masterList->GetItem( ii )->m_Flag = 0;

    // Create netlist module section
    m_ReferencesAlreadyFound.Clear();

    SCH_SHEET_LIST SheetList;

    for( sheet = SheetList.GetFirst(); sheet != NULL; sheet = SheetList.GetNext() )
    {
        for( DrawList = sheet->LastDrawList(); DrawList != NULL; DrawList = DrawList->Next() )
        {
            DrawList = component = findNextComponentAndCreatePinList( DrawList, sheet );

            if( component == NULL )
                break;

            /*
            doing nothing with footprint
            if( !component->GetField( FOOTPRINT )->IsVoid() )
            {
                footprint = component->GetField( FOOTPRINT )->m_Text;
                footprint.Replace( wxT( " " ), wxT( "_" ) );
            }
            else
                footprint = wxT( "$noname" );
            */

            msg = component->GetRef( sheet->Last() );
            ret |= fprintf( f, "%s     ", TO_UTF8( StartCmpDesc ) );
            ret |= fprintf( f, "%s", TO_UTF8( msg ) );

            msg = component->GetField( VALUE )->GetText();
            msg.Replace( wxT( " " ), wxT( "_" ) );
            ret |= fprintf( f, "     \"%s\"", TO_UTF8( msg ) );
            ret |= fprintf( f, "\n" );
        }
    }

    ret |= fprintf( f, "\n" );

    m_SortedComponentPinList.clear();

    if( ! writeListOfNets( f ) )
        ret = -1;   // set error

    ret |= fprintf( f, "\n%sEND\n", TO_UTF8( StartLine ) );

    fclose( f );

    return ret >= 0;
}


bool NETLIST_EXPORTER_CADSTAR::writeListOfNets( FILE* f )
{
    int ret = 0;
    wxString InitNetDesc  = StartLine + wxT( "ADD_TER" );
    wxString StartNetDesc = StartLine + wxT( "TER" );
    wxString netcodeName, InitNetDescLine;
    unsigned ii;
    int print_ter = 0;
    int NetCode, lastNetCode = -1;
    SCH_COMPONENT* Cmp;
    wxString netName;

    for( ii = 0; ii < m_masterList->size(); ii++ )
    {
        NETLIST_OBJECT* nitem = m_masterList->GetItem( ii );

        // Get the NetName of the current net :
        if( ( NetCode = nitem->GetNet() ) != lastNetCode )
        {
            netName = nitem->GetNetName();
            netcodeName = wxT( "\"" );

            if( !netName.IsEmpty() )
                netcodeName << netName;
            else  // this net has no name: create a default name $<net number>
                netcodeName << wxT( "$" ) << NetCode;

            netcodeName += wxT( "\"" );
            lastNetCode  = NetCode;
            print_ter    = 0;
        }


        if( nitem->m_Type != NET_PIN )
            continue;

        if( nitem->m_Flag != 0 )
            continue;

        Cmp = nitem->GetComponentParent();
        wxString refstr = Cmp->GetRef( nitem->m_SheetPath.Last() );

        if( refstr[0] == '#' )
            continue;  // Power supply symbols.

        switch( print_ter )
        {
        case 0:
            {
                char buf[5];
                wxString str_pinnum;
                strncpy( buf, (char*) &nitem->m_PinNum, 4 );
                buf[4]     = 0;
                str_pinnum = FROM_UTF8( buf );
                InitNetDescLine.Printf( wxT( "\n%s   %s   %.4s     %s" ),
                                       GetChars( InitNetDesc ),
                                       GetChars( refstr ),
                                       GetChars( str_pinnum ),
                                       GetChars( netcodeName ) );
            }
            print_ter++;
            break;

        case 1:
            ret |= fprintf( f, "%s\n", TO_UTF8( InitNetDescLine ) );
            ret |= fprintf( f, "%s       %s   %.4s\n",
                            TO_UTF8( StartNetDesc ),
                            TO_UTF8( refstr ),
                            (char*) &nitem->m_PinNum );
            print_ter++;
            break;

        default:
            ret |= fprintf( f, "            %s   %.4s\n",
                            TO_UTF8( refstr ),
                            (char*) &nitem->m_PinNum );
            break;
        }

        nitem->m_Flag = 1;
    }

    return ret >= 0;
}
