/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2011 Jean-Pierre Charras.
 * Copyright (C) 1992-2021 KiCad Developers, see change_log.txt for contributors.
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

#include <netlist_lexer.h>  // netlist_lexer is common to Eeschema and Pcbnew
#include <string_utils.h>
#include <nlohmann/json.hpp>

#include "pcb_netlist.h"
#include "netlist_reader.h"

using namespace NL_T;


void KICAD_NETLIST_READER::LoadNetlist()
{
    KICAD_NETLIST_PARSER parser( m_lineReader, m_netlist );

    parser.Parse();

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
    int plevel = 0;     // the count of ')' to read at end of file after parsing all sections

    while( ( token = NextTok() ) != T_EOF )
    {
        if( token == T_LEFT )
            token = NextTok();

        switch( token )
        {
        case T_export:  // The netlist starts here.
            // nothing to do here, just increment the count of ')' to read at end of file
            plevel++;
            break;

        case T_version:  // The netlist starts here.
            // version id not yet used: read it but does not use it
            NextTok();
            NeedRIGHT();
            break;

        case T_components:  // The section comp starts here.
            while( ( token = NextTok() ) != T_EOF )
            {
                if( token == T_RIGHT )
                    break;
                else if( token == T_LEFT )
                    token = NextTok();

                if( token == T_comp )       // A component section found. Read it
                    parseComponent();
            }

            break;

        case T_nets:    // The section nets starts here.
            while( ( token = NextTok() ) != T_EOF )
            {
                if( token == T_RIGHT )
                    break;
                else if( token == T_LEFT )
                    token = NextTok();

                if( token == T_net )        // A net section if found. Read it
                    parseNet();
            }

            break;

        case T_libparts:    // The section libparts starts here.
            while( ( token = NextTok() ) != T_EOF )
            {
                if( token == T_RIGHT )
                    break;
                else if( token == T_LEFT )
                    token = NextTok();

                if( token == T_libpart )    // A libpart section if found. Read it
                    parseLibPartList();
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
            plevel--;
            break;

        default:
            skipCurrent();
            break;
        }
    }

    if( plevel != 0 )
    {
        wxFAIL_MSG( wxString::Format( wxT( "KICAD_NETLIST_PARSER::Parse(): bad parenthesis "
                                           "count (count = %d" ),
                                      plevel ) );
    }
}


void KICAD_NETLIST_PARSER::parseNet()
{
    /* Parses a section like
     * (net (code 20) (name /PC-A0)
     *  (node (ref "BUS1") (pin "62)")
     *  (node (ref "U3") ("pin 3") (pin_function "clock"))
     *  (node (ref "U9") (pin "M6") (pin_function "reset")))
     */

    wxString   code;
    wxString   name;
    wxString   reference;
    wxString   pin_number;
    wxString   pin_function;
    wxString   pin_type;

    // The token net was read, so the next data is (code <number>)
    while( (token = NextTok() ) != T_EOF )
    {
        if( token == T_RIGHT )
            break;
        else if( token == T_LEFT )
            token = NextTok();

        switch( token )
        {
        case T_code:
            NeedSYMBOLorNUMBER();
            code = From_UTF8( CurText() );
            NeedRIGHT();
            break;

        case T_name:
            NeedSYMBOLorNUMBER();
            name = From_UTF8( CurText() );
            NeedRIGHT();
            break;

        case T_node:
            // By default: no pin function or type.
            pin_function.Clear();
            pin_type.Clear();

            while( (token = NextTok() ) != T_EOF )
            {
                if( token == T_RIGHT )
                    break;
                else if( token == T_LEFT )
                    token = NextTok();

                switch( token )
                {
                case T_ref:
                    NeedSYMBOLorNUMBER();
                    reference = From_UTF8( CurText() );
                    NeedRIGHT();
                    break;

                case T_pin:
                    NeedSYMBOLorNUMBER();
                    pin_number = From_UTF8( CurText() );
                    NeedRIGHT();
                    break;

                case T_pinfunction:
                    NeedSYMBOLorNUMBER();
                    pin_function = From_UTF8( CurText() );
                    NeedRIGHT();
                    break;

                case T_pintype:
                    NeedSYMBOLorNUMBER();
                    pin_type = From_UTF8( CurText() );
                    NeedRIGHT();
                    break;

                default:
                    skipCurrent();
                    break;
                }
            }

            // Don't assume component will be found; it might be "DNP" or "Exclude from board".
            if( COMPONENT* component = m_netlist->GetComponentByReference( reference ) )
            {
                if( strtol( code.c_str(), nullptr, 10 ) >= 1 )
                {
                    if( name.IsEmpty() )      // Give a dummy net name like N-000009
                        name = wxT("N-00000") + code;

                    component->AddNet( pin_number, name, pin_function, pin_type );
                }
            }

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
     *   (value DB25FEMALE)
     *   (footprint DB25FC)
     *   (libsource (lib conn) (part DB25))
     *   (property (name PINCOUNT) (value 25))
     *   (sheetpath (names /) (tstamps /))
     *   (tstamp 68183921-93a5-49ac-91b0-49d05a0e1647))
     *
     * other fields (unused) are skipped
     * A component need a reference, value, footprint name and a full time stamp
     * The full time stamp is the sheetpath time stamp + the component time stamp
     */
    LIB_ID      fpid;
    wxString    footprint;
    wxString    ref;
    wxString    value;
    wxString    library;
    wxString    name;
    wxString    humanSheetPath;
    KIID_PATH   path;

    std::vector<KIID>            uuids;
    std::map<wxString, wxString> properties;
    nlohmann::ordered_map<wxString, wxString> fields;

    // The token comp was read, so the next data is (ref P1)
    while( (token = NextTok() ) != T_RIGHT )
    {
        if( token == T_LEFT )
            token = NextTok();

        switch( token )
        {
        case T_ref:
            NeedSYMBOLorNUMBER();
            ref = From_UTF8( CurText() );
            NeedRIGHT();
            break;

        case T_value:
            NeedSYMBOLorNUMBER();
            value = From_UTF8( CurText() );
            NeedRIGHT();
            break;

        case T_footprint:
            NeedSYMBOLorNUMBER();
            footprint = FromUTF8();
            NeedRIGHT();
            break;

        case T_libsource:
            // Read libsource
            while( ( token = NextTok() ) != T_RIGHT )
            {
                if( token == T_LEFT )
                    token = NextTok();

                if( token == T_lib )
                {
                    NeedSYMBOLorNUMBER();
                    library = From_UTF8( CurText() );
                    NeedRIGHT();
                }
                else if( token == T_part )
                {
                    NeedSYMBOLorNUMBER();
                    name = From_UTF8( CurText() );
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

        case T_property:
        {
            wxString propName;
            wxString propValue;

            while( (token = NextTok() ) != T_RIGHT )
            {
                if( token == T_LEFT )
                    token = NextTok();

                if( token == T_name )
                {
                    NeedSYMBOLorNUMBER();
                    propName = From_UTF8( CurText() );
                    NeedRIGHT();
                }
                else if( token == T_value )
                {
                    NeedSYMBOLorNUMBER();
                    propValue = From_UTF8( CurText() );
                    NeedRIGHT();
                }
                else
                {
                    Expecting( "name or value" );
                }
            }

            if( !propName.IsEmpty() )
                properties[ propName ] = propValue;
        }
            break;

        case T_fields:

            // Read fields
            while( ( token = NextTok() ) != T_RIGHT )
            {
                if( token == T_LEFT )
                    token = NextTok();

                if( token == T_field )
                {
                    wxString fieldName;
                    wxString fieldValue;

                    while( ( token = NextTok() ) != T_RIGHT )
                    {
                        if( token == T_LEFT )
                            token = NextTok();

                        if( token == T_name )
                        {
                            NeedSYMBOLorNUMBER();
                            fieldName = From_UTF8( CurText() );
                            NeedRIGHT();
                        }
                        else if( token == T_STRING )
                        {
                            fieldValue = From_UTF8( CurText() );
                        }
                    }

                    if( !fieldName.IsEmpty() )
                        fields[fieldName] = fieldValue;
                }
                else
                {
                    Expecting( "field" );
                }
            }
            break;

        case T_sheetpath:
            while( ( token = NextTok() ) != T_EOF )
            {
                if( token == T_names )
                {
                    NeedSYMBOLorNUMBER();
                    humanSheetPath = From_UTF8( CurText() );
                    printf("SPath '%s'\n", humanSheetPath.c_str().AsChar() );
                    NeedRIGHT();
                }
                if( token == T_tstamps )
                {
                    NeedSYMBOLorNUMBER();
                    path = KIID_PATH( From_UTF8( CurText() ) );
                    NeedRIGHT();
                    break;
                }
            }

            NeedRIGHT();

            break;

        case T_tstamps:
            while( ( token = NextTok() ) != T_EOF )
            {
                if( token == T_RIGHT )
                    break;

                uuids.emplace_back( From_UTF8( CurText() ) );
            }

            break;

        default:
            // Skip not used data (i.e all other tokens)
            skipCurrent();
            break;
        }
    }

    if( !footprint.IsEmpty() && fpid.Parse( footprint, true ) >= 0 )
    {
        wxString error;
        error.Printf( _( "Invalid footprint ID in\nfile: '%s'\nline: %d\nofff: %d" ),
                      CurSource(), CurLineNumber(), CurOffset() );

        THROW_IO_ERROR( error );
    }

    COMPONENT* component = new COMPONENT( fpid, ref, value, path, uuids );
    component->SetName( name );
    component->SetLibrary( library );
    component->SetProperties( properties );
    component->SetFields( fields );
    component->SetHumanReadablePath( humanSheetPath );
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
    while( (token = NextTok() ) != T_RIGHT )
    {
        if( token == T_LEFT )
            token = NextTok();

        switch( token )
        {
        case T_lib:
            NeedSYMBOLorNUMBER();
            libName = From_UTF8( CurText() );
            NeedRIGHT();
            break;

        case T_part:
            NeedSYMBOLorNUMBER();
            libPartName = From_UTF8( CurText() );
            NeedRIGHT();
            break;

        case T_footprints:
            // Read all fp elements (footprint filter item)
            while( (token = NextTok() ) != T_RIGHT )
            {
                if( token == T_LEFT )
                    token = NextTok();

                if( token != T_fp )
                    Expecting( T_fp );

                NeedSYMBOLorNUMBER();
                footprintFilters.Add( From_UTF8( CurText() ) );
                NeedRIGHT();
            }
            break;

        case T_aliases:
            while( (token = NextTok() ) != T_RIGHT )
            {
                if( token == T_LEFT )
                    token = NextTok();

                if( token != T_alias )
                    Expecting( T_alias );

                NeedSYMBOLorNUMBER();
                aliases.Add( From_UTF8( CurText() ) );
                NeedRIGHT();
            }
            break;

        case T_pins:
            while( (token = NextTok() ) != T_RIGHT )
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
