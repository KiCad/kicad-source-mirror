/**
 * @file kicad_netlist_reader.cpp
 */
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2011 Jean-Pierre Charras.
 * Copyright (C) 1992-2016 KiCad Developers, see change_log.txt for contributors.
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

#include <wx/wx.h>
#include <netlist_lexer.h>  // netlist_lexer is common to Eeschema and Pcbnew
#include <macros.h>

#include <pcb_netlist.h>
#include <netlist_reader.h>

using namespace NL_T;


void KICAD_NETLIST_READER::LoadNetlist()
{
    m_parser->Parse();

    if( m_footprintReader )
    {
        m_footprintReader->Load( m_netlist );

        // Sort the component pins so they are in the same order as the legacy format.  This
        // is useful for comparing legacy and s-expression netlist dumps.
        for( unsigned i = 0;  i < m_netlist->GetCount();  i++ )
            m_netlist->GetComponent( i )->SortPins();
    }
}


// KICAD_NETLIST_PARSER
KICAD_NETLIST_PARSER::KICAD_NETLIST_PARSER( LINE_READER* aReader, NETLIST* aNetlist ) :
    NETLIST_LEXER( aReader )
{
    m_lineReader = aReader;
    m_netlist    = aNetlist;
    token        = T_NONE;
}


void KICAD_NETLIST_PARSER::skipCurrent()
{
    int curr_level = 0;

    while( ( token = NextTok() ) != T_EOF )
    {
        if( token == T_LEFT )
            curr_level--;

        if( token == T_RIGHT )
        {
            curr_level++;

            if( curr_level > 0 )
                return;
        }
    }
}


void KICAD_NETLIST_PARSER::Parse()
{
    int plevel = 0;     // the count of ')' to read and end of file,
                        // after parsing all sections

    while( ( token = NextTok() ) != T_EOF )
    {
        if( token == T_LEFT )
            token = NextTok();

        switch( token )
        {
        case T_export:  // The netlist starts here.
            // nothing to do here,
            // just increment the count of ')' to read and end of file
            plevel++;
            break;

        case T_version:  // The netlist starts here.
            // version id not yet used: read it but does not use it
            NextTok();
            NeedRIGHT();
            break;

        case T_components:  // The section comp starts here.
            while( ( token = NextTok() ) != T_RIGHT )
            {
                if( token == T_LEFT )
                    token = NextTok();

                if( token == T_comp )   // A component section found. Read it
                    parseComponent();
            }

            break;

        case T_nets:    // The section nets starts here.
            while( ( token = NextTok() ) != T_RIGHT )
            {
                if( token == T_LEFT )
                    token = NextTok();

                if( token == T_net )
                {
                    // A net section if found. Read it
                    parseNet();
                }
            }

            break;

        case T_libparts:    // The section libparts starts here.
            while( ( token = NextTok() ) != T_RIGHT )
            {
                if( token == T_LEFT )
                    token = NextTok();

                if( token == T_libpart )
                {
                    // A libpart section if found. Read it
                    parseLibPartList();
                }
            }

            break;

        case T_libraries:    // The section libraries starts here.
            // List of libraries in use.
            // Not used here, just skip it
            skipCurrent();
            break;

        case T_design:    // The section design starts here.
            // Not used (mainly they are comments), just skip it
            skipCurrent();
            break;

        case T_RIGHT:    // The closing parenthesis of the file.
            // Not used (mainly they are comments), just skip it
            plevel--;
            break;

        default:
            skipCurrent();
            break;
        }
    }

    if( plevel != 0 )
    {
        wxLogDebug( wxT( "KICAD_NETLIST_PARSER::Parse(): bad parenthesis count (count = %d"),
                    plevel );
    }
}


void KICAD_NETLIST_PARSER::parseNet()
{
    /* Parses a section like
     * (net (code 20) (name /PC-A0)
     *  (node (ref BUS1) (pin 62))
     *  (node (ref U3) (pin 3))
     *  (node (ref U9) (pin M6)))
     */

    COMPONENT* component = NULL;
    wxString   code;
    wxString   name;
    wxString   reference;
    wxString   pin;
    int        nodecount = 0;

    // The token net was read, so the next data is (code <number>)
    while( (token = NextTok()) != T_RIGHT )
    {
        if( token == T_LEFT )
            token = NextTok();

        switch( token )
        {
        case T_code:
            NeedSYMBOLorNUMBER();
            code = FROM_UTF8( CurText() );
            NeedRIGHT();
            break;

        case T_name:
            NeedSYMBOLorNUMBER();
            name = FROM_UTF8( CurText() );
            NeedRIGHT();

            if( name.IsEmpty() )      // Give a dummy net name like N-000109
                name = wxT("N-00000") + code;

            break;

        case T_node:
            while( (token = NextTok()) != T_RIGHT )
            {
                if( token == T_LEFT )
                    token = NextTok();

                switch( token )
                {
                case T_ref:
                    NeedSYMBOLorNUMBER();
                    reference = FROM_UTF8( CurText() );
                    NeedRIGHT();
                    break;

                case T_pin:
                    NeedSYMBOLorNUMBER();
                    pin = FROM_UTF8( CurText() );
                    NeedRIGHT();
                    break;

                default:
                    skipCurrent();
                    break;
                }
            }


            component = m_netlist->GetComponentByReference( reference );

            // Cannot happen if the netlist is valid.
            if( component == NULL )
            {
                wxString msg;
                msg.Printf( _( "Cannot find component with reference \"%s\" in netlist." ),
                               GetChars( reference ) );
                THROW_PARSE_ERROR( msg, m_lineReader->GetSource(), m_lineReader->Line(),
                                   m_lineReader->LineNumber(), m_lineReader->Length() );
            }

            component->AddNet( pin, name );
            nodecount++;
            break;

        default:
            skipCurrent();
            break;
        }
    }
}


void KICAD_NETLIST_PARSER::parseComponent()
{
   /* Parses a section like
     * (comp (ref P1)
     * (value DB25FEMELLE)
     * (footprint DB25FC)
     * (libsource (lib conn) (part DB25))
     * (sheetpath (names /) (tstamps /))
     * (tstamp 3256759C))
     *
     * other fields (unused) are skipped
     * A component need a reference, value, footprint name and a full time stamp
     * The full time stamp is the sheetpath time stamp + the component time stamp
     */
    LIB_ID   fpid;
    wxString footprint;
    wxString ref;
    wxString value;
    wxString library;
    wxString name;
    wxString pathtimestamp, timestamp;

    // The token comp was read, so the next data is (ref P1)
    while( (token = NextTok()) != T_RIGHT )
    {
        if( token == T_LEFT )
            token = NextTok();

        switch( token )
        {
        case T_ref:
            NeedSYMBOLorNUMBER();
            ref = FROM_UTF8( CurText() );
            NeedRIGHT();
            break;

        case T_value:
            NeedSYMBOLorNUMBER();
            value = FROM_UTF8( CurText() );
            NeedRIGHT();
            break;

        case T_footprint:
            NeedSYMBOLorNUMBER();
            footprint = FromUTF8();
            NeedRIGHT();
            break;

        case T_libsource:
            // Read libsource
            while( (token = NextTok()) != T_RIGHT )
            {
                if( token == T_LEFT )
                    token = NextTok();

                if( token == T_lib )
                {
                    NeedSYMBOLorNUMBER();
                    library = FROM_UTF8( CurText() );
                    NeedRIGHT();
                }
                else if( token == T_part )
                {
                    NeedSYMBOLorNUMBER();
                    name = FROM_UTF8( CurText() );
                    NeedRIGHT();
                }
                else if( token == T_description )
                {
                    NeedSYMBOLorNUMBER();
                    NeedRIGHT();
                }
                else
                {
                    Expecting( "part, lib or description" );
                }
            }
            break;

        case T_sheetpath:
            while( ( token = NextTok() ) != T_tstamps );
            NeedSYMBOLorNUMBER();
            pathtimestamp = FROM_UTF8( CurText() );
            NeedRIGHT();
            NeedRIGHT();
            break;

        case T_tstamp:
            NeedSYMBOLorNUMBER();
            timestamp = FROM_UTF8( CurText() );
            NeedRIGHT();
            break;

        default:
            // Skip not used data (i.e all other tokens)
            skipCurrent();
            break;
        }
    }

    if( !footprint.IsEmpty() && fpid.Parse( footprint, LIB_ID::ID_PCB, true ) >= 0 )
    {
        wxString error;
        error.Printf( _( "Invalid footprint ID in\nfile: \"%s\"\nline: %d\noffset: %d" ),
                      CurSource(), CurLineNumber(), CurOffset() );

        THROW_IO_ERROR( error );
    }

    pathtimestamp += timestamp;
    COMPONENT* component = new COMPONENT( fpid, ref, value, pathtimestamp );
    component->SetName( name );
    component->SetLibrary( library );
    m_netlist->AddComponent( component );
}


void KICAD_NETLIST_PARSER::parseLibPartList()
{
   /* Parses a section like
    *   (libpart (lib device) (part C)
    *  (aliases
    *    (alias Cxx)
    *    (alias Cyy))
    *     (description "Condensateur non polarise")
    *     (footprints
    *       (fp SM*)
    *       (fp C?)
    *       (fp C1-1))
    *     (fields
    *       (field (name Reference) C)
    *       (field (name Value) C))
    *     (pins
    *       (pin (num 1) (name ~) (type passive))
    *       (pin (num 2) (name ~) (type passive))))
    *
    * Currently footprints section/fp are read and data stored
    * other fields (unused) are skipped
    */
    COMPONENT*        component = NULL;
    wxString          libName;
    wxString          libPartName;
    wxArrayString     footprintFilters;
    wxArrayString     aliases;
    int               pinCount = 0;

    // The last token read was libpart, so read the next token
    while( (token = NextTok()) != T_RIGHT )
    {
        if( token == T_LEFT )
            token = NextTok();

        switch( token )
        {
        case T_lib:
            NeedSYMBOLorNUMBER();
            libName = FROM_UTF8( CurText() );
            NeedRIGHT();
            break;

        case T_part:
            NeedSYMBOLorNUMBER();
            libPartName = FROM_UTF8( CurText() );
            NeedRIGHT();
            break;

        case T_footprints:
            // Read all fp elements (footprint filter item)
            while( (token = NextTok()) != T_RIGHT )
            {
                if( token == T_LEFT )
                    token = NextTok();

                if( token != T_fp )
                    Expecting( T_fp );

                NeedSYMBOLorNUMBER();
                footprintFilters.Add( FROM_UTF8( CurText() ) );
                NeedRIGHT();
            }
            break;

        case T_aliases:
            while( (token = NextTok()) != T_RIGHT )
            {
                if( token == T_LEFT )
                    token = NextTok();

                if( token != T_alias )
                    Expecting( T_alias );

                NeedSYMBOLorNUMBER();
                aliases.Add( FROM_UTF8( CurText() ) );
                NeedRIGHT();
            }
            break;

        case T_pins:
            while( (token = NextTok()) != T_RIGHT )
            {
                if( token == T_LEFT )
                    token = NextTok();

                if( token != T_pin )
                    Expecting( T_pin );

                pinCount++;

                skipCurrent();
            }
            break;

        default:
            // Skip not used data (i.e all other tokens)
            skipCurrent();
            break;
        }
    }

    // Find all of the components that reference this component library part definition.
    for( unsigned i = 0;  i < m_netlist->GetCount();  i++ )
    {
        component = m_netlist->GetComponent( i );

        if( component->IsLibSource( libName, libPartName ) )
        {
            component->SetFootprintFilters( footprintFilters );
            component->SetPinCount( pinCount );
        }

        for( unsigned jj = 0; jj < aliases.GetCount(); jj++ )
        {
            if( component->IsLibSource( libName, aliases[jj] ) )
            {
                component->SetFootprintFilters( footprintFilters );
                component->SetPinCount( pinCount );
            }
        }

    }
}
