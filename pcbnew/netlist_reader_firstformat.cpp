/**
 * @file pcbnew/netlist_reader_firstformat.cpp
 */
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2011 Jean-Pierre Charras.
 * Copyright (C) 1992-2011 KiCad Developers, see change_log.txt for contributors.
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


/*
 *  Netlist reader using the first format of pcbnew netlist.
 * This netlist reader build the list of modules found in netlist
 * (list in m_modulesInNetlist)
 * and update pads netnames
 */

#include <fctsys.h>
#include <kicad_string.h>
#include <wxPcbStruct.h>
#include <richio.h>

#include <class_board.h>
#include <class_module.h>
#include <pcbnew.h>

#include <netlist_reader.h>

// constants used by ReadOldFmtNetlistModuleDescr():
#define BUILDLIST   true
#define READMODULE false


/*
 * Function ReadOldFmtdNetList
 * Update footprints (load missing footprints and delete on request extra
 * footprints)
 * Update References, values, "TIME STAMP" and connectivity data
 * return true if Ok
 *
 *  the format of the netlist is something like:
 * # EESchema Netlist Version 1.0 generee le  18/5/2005-12:30:22
 *  (
 *  ( 40C08647 $noname R20 4,7K {Lib=R}
 *  (    1 VCC )
 *  (    2 MODB_1 )
 *  )
 *  ( 40C0863F $noname R18 4,7_k {Lib=R}
 *  (    1 VCC )
 *  (    2 MODA_1 )
 *  )
 *  }
 * #End
 */
bool NETLIST_READER::ReadOldFmtdNetList( FILE* aFile )
{
    int state   = 0;
    bool is_comment = false;

    /* First, read the netlist: Build the list of footprints found in netlist
     */
    // netlineReader dtor will close aFile
    FILE_LINE_READER netlineReader( aFile, m_netlistFullName );

    while( netlineReader.ReadLine() )
    {
        char* line = StrPurge( netlineReader.Line() );

        if( is_comment ) // Comments in progress
        {
            // Test for end of the current comment
            if( ( line = strchr( line, '}' ) ) == NULL )
                continue;

            is_comment = false;
        }
        if( *line == '{' ) // Start Comment
        {
            is_comment = true;

            if( ( line = strchr( line, '}' ) ) == NULL )
                continue;
        }

        if( *line == '(' )
            state++;

        if( *line == ')' )
            state--;

        if( state == 2 )
        {
            ReadOldFmtNetlistModuleDescr( line, BUILDLIST );
            continue;
        }

        if( state >= 3 ) // First pass: pad descriptions are not read here.
        {
            state--;
        }
    }

    if( BuildModuleListOnly() )
        return true; // at this point, the module list is read and built.

    // Load new footprints
    bool success = InitializeModules();

    if( ! success )
        wxMessageBox( _("Some footprints are not found in libraries") );

    TestFootprintsMatchingAndExchange();

    /* Second read , All footprints are on board.
     * Update the schematic info (pad netnames)
     */
    netlineReader.Rewind();
    m_currModule = NULL;
    state = 0;
    is_comment = false;

    while( netlineReader.ReadLine() )
    {
        char* line = StrPurge( netlineReader.Line() );

        if( is_comment )   // we are reading a comment
        {
            // Test for end of the current comment
            if( ( line = strchr( line, '}' ) ) == NULL )
                continue;
            is_comment = false;
        }

        if( *line == '{' ) // this is the beginning of a comment
        {
            is_comment = true;

            if( ( line = strchr( line, '}' ) ) == NULL )
                continue;
        }

        if( *line == '(' )
            state++;

        if( *line == ')' )
            state--;

        if( state == 2 )
        {
            m_currModule = ReadOldFmtNetlistModuleDescr( line, READMODULE );
            continue;
        }

        if( state >= 3 )
        {
            if( m_currModule )
                SetPadNetName( line );
            state--;
        }
    }

    return true;
}


/* Function ReadOldFmtNetlistModuleDescr
 * Read the beginning of a footprint  description, from the netlist
 * and add a module info to m_modulesInNetlist
 * Analyze lines like:
 * ( /40C08647 $noname R20 4.7K {Lib=R}
 * (1 VCC)
 * (2 MODB_1)
 * )
 */
MODULE* NETLIST_READER::ReadOldFmtNetlistModuleDescr( char* aText, bool aBuildList )
{
    char*    text;
    wxString timeStampPath;         // the full time stamp read from netlist
    wxString footprintName;         // the footprint name read from netlist
    wxString cmpValue;             // the component value read from netlist
    wxString cmpReference;      // the component schematic reference read from netlist
    bool     error = false;
    char     line[1024];

    strcpy( line, aText );

    cmpValue = wxT( "~" );

    // Read descr line like  /40C08647 $noname R20 4.7K {Lib=R}

    // Read time stamp (first word)
    if( ( text = strtok( line, " ()\t\n" ) ) == NULL )
        error = true;
    else
        timeStampPath = FROM_UTF8( text );

    // Read footprint name (second word)
    if( ( text = strtok( NULL, " ()\t\n" ) ) == NULL )
        error = true;
    else
        footprintName = FROM_UTF8( text );

    // Read schematic reference (third word)
    if( ( text = strtok( NULL, " ()\t\n" ) ) == NULL )
        error = true;
    else
        cmpReference = FROM_UTF8( text );

    // Read schematic value (forth word)
    if( ( text = strtok( NULL, " ()\t\n" ) ) == NULL )
        error = true;
    else
        cmpValue = FROM_UTF8( text );

    if( error )
        return NULL;

    if( aBuildList )
    {
        MODULE_INFO* mod_info = new MODULE_INFO( footprintName, cmpReference,
                                                cmpValue, timeStampPath );
        AddModuleInfo( mod_info );
        return NULL;
    }

    // search the module loaded on board
    // reference and time stamps are already updated so we can used search by reference
    MODULE* module = m_pcbframe->GetBoard()->FindModuleByReference( cmpReference );
    if( module == NULL )
    {
        if( m_messageWindow )
        {
            wxString msg;
            msg.Printf( _( "Component [%s] not found" ), GetChars( cmpReference ) );
            m_messageWindow->AppendText( msg + wxT( "\n" ) );
        }
    }

    return module;
}


/*
 * Function SetPadNetName
 *  Update a pad netname using the current footprint
 *  Line format: ( <pad number> = <net name> )
 *  Param aText = current line read from netlist
 */
bool NETLIST_READER::SetPadNetName( char* aText )
{
    char*       p;
    char        line[256];

    if( m_currModule == NULL )
        return false;

    strncpy( line, aText, sizeof(line) );

    if( ( p = strtok( line, " ()\t\n" ) ) == NULL )
        return false;

    wxString pinName = FROM_UTF8( p );

    if( ( p = strtok( NULL, " ()\t\n" ) ) == NULL )
        return false;

    wxString netName = FROM_UTF8( p );

    bool found = false;
    for( D_PAD* pad = m_currModule->m_Pads;  pad;  pad = pad->Next() )
    {
        wxString padName = pad->GetPadName();

        if( padName == pinName )
        {
            found = true;
            if( (char) netName[0] != '?' )
                pad->SetNetname( netName );
            else
                pad->SetNetname( wxEmptyString );
        }
    }

    if( !found )
    {
        if( m_messageWindow )
        {
            wxString msg;
            msg.Printf( _( "Module [%s]: Pad [%s] not found" ),
                        GetChars( m_currModule->m_Reference->m_Text ),
                        GetChars( pinName ) );
            m_messageWindow->AppendText( msg + wxT( "\n" ) );
        }
    }

    return found;
}
