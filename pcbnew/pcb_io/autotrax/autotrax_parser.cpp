/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Format interpretation derived from pcb-rnd src_plugins/io_autotrax:
 *   Copyright (C) 2016, 2017, 2018, 2020 Tibor 'Igor2' Palinkas
 *   Copyright (C) 2016, 2017 Erich S. Heinzle
 * Used under GPL v2-or-later.
 *
 * Copyright (C) 2026 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "autotrax_parser.h"

#include <reporter.h>
#include <wx/tokenzr.h>
#include <wx/translation.h>

using namespace AUTOTRAX;


/// Parse a token as a double using the C locale, returning 0 on failure so a
/// malformed field degrades to zero rather than corrupting on comma-decimal
/// locales the way bare strtod() would.
static double toDouble( const wxString& aToken )
{
    double value = 0.0;
    aToken.ToCDouble( &value );
    return value;
}


/// Parse a token as an integer using the C locale, returning 0 on failure.
static int toInt( const wxString& aToken )
{
    long value = 0;
    aToken.ToCLong( &value );
    return static_cast<int>( value );
}


bool AUTOTRAX_PARSER::Sniff( const wxString& aContents )
{
    // Empty lines are retained so a blank or comment preamble does not hide the
    // magic header on the first line of data.
    for( wxString line : wxStringTokenize( aContents, wxT( "\n" ), wxTOKEN_RET_EMPTY ) )
    {
        line.Trim( true ).Trim( false );

        if( line.IsEmpty() || line[0] == '#' )
            continue;

        return line.StartsWith( wxT( "PCB FILE 4" ) ) || line.StartsWith( wxT( "PCB FILE 5" ) );
    }

    return false;
}


void AUTOTRAX_PARSER::warn( const wxString& aMsg ) const
{
    if( m_reporter )
        m_reporter->Report( wxString::Format( _( "Autotrax import, line %d: %s" ), m_lineNo, aMsg ),
                            RPT_SEVERITY_WARNING );
}


bool AUTOTRAX_PARSER::nextLine( wxString& aLine )
{
    if( m_index >= m_lines.GetCount() )
        return false;

    // Every keyword, label and free-text field is surrounded by optional
    // whitespace (including the trailing CR of DOS line endings), so trimming
    // here lets the rest of the parser compare and tokenize clean lines.
    aLine = m_lines[m_index++];
    aLine.Trim( true ).Trim( false );
    m_lineNo = static_cast<int>( m_index );
    return true;
}


wxArrayString AUTOTRAX_PARSER::tokenize( const wxString& aLine )
{
    return wxStringTokenize( aLine, wxT( " \t\r\n" ), wxTOKEN_STRTOK );
}


bool AUTOTRAX_PARSER::parseTrack( TRACK& aOut )
{
    wxString line;

    if( !nextLine( line ) )
        return false;

    wxArrayString tok = tokenize( line );

    if( tok.GetCount() < 6 )
    {
        warn( _( "insufficient track fields" ) );
        return false;
    }

    aOut.x1 = toDouble( tok[0] );
    aOut.y1 = toDouble( tok[1] );
    aOut.x2 = toDouble( tok[2] );
    aOut.y2 = toDouble( tok[3] );
    aOut.width = toDouble( tok[4] );
    aOut.layer = toInt( tok[5] );
    return true;
}


bool AUTOTRAX_PARSER::parseArc( ARC& aOut )
{
    wxString line;

    if( !nextLine( line ) )
        return false;

    wxArrayString tok = tokenize( line );

    if( tok.GetCount() < 6 )
    {
        warn( _( "insufficient arc fields" ) );
        return false;
    }

    aOut.centerX = toDouble( tok[0] );
    aOut.centerY = toDouble( tok[1] );
    aOut.radius = toDouble( tok[2] );
    aOut.segments = toInt( tok[3] );
    aOut.width = toDouble( tok[4] );
    aOut.layer = toInt( tok[5] );
    return true;
}


bool AUTOTRAX_PARSER::parseVia( VIA& aOut )
{
    wxString line;

    if( !nextLine( line ) )
        return false;

    wxArrayString tok = tokenize( line );

    if( tok.GetCount() < 4 )
    {
        warn( _( "insufficient via fields" ) );
        return false;
    }

    aOut.x = toDouble( tok[0] );
    aOut.y = toDouble( tok[1] );
    aOut.diameter = toDouble( tok[2] );
    aOut.drill = toDouble( tok[3] );
    return true;
}


bool AUTOTRAX_PARSER::parsePad( PAD& aOut )
{
    wxString line;

    if( !nextLine( line ) )
        return false;

    wxArrayString tok = tokenize( line );

    // The attribute line carries eight fields; the pad name is on the next line
    // and is always consumed so the record is left fully read on error too.
    if( tok.GetCount() < 8 )
    {
        warn( _( "insufficient pad fields" ) );
        nextLine( line );
        return false;
    }

    aOut.x = toDouble( tok[0] );
    aOut.y = toDouble( tok[1] );
    aOut.xSize = toDouble( tok[2] );
    aOut.ySize = toDouble( tok[3] );
    aOut.shape = toInt( tok[4] );
    aOut.drill = toDouble( tok[5] );
    aOut.planeFlags = toInt( tok[6] );
    aOut.layer = toInt( tok[7] );

    if( nextLine( line ) )
        aOut.name = line;

    return true;
}


bool AUTOTRAX_PARSER::parseFill( FILL& aOut )
{
    wxString line;

    if( !nextLine( line ) )
        return false;

    wxArrayString tok = tokenize( line );

    if( tok.GetCount() < 5 )
    {
        warn( _( "insufficient fill fields" ) );
        return false;
    }

    aOut.x1 = toDouble( tok[0] );
    aOut.y1 = toDouble( tok[1] );
    aOut.x2 = toDouble( tok[2] );
    aOut.y2 = toDouble( tok[3] );
    aOut.layer = toInt( tok[4] );
    return true;
}


bool AUTOTRAX_PARSER::parseText( TEXT& aOut )
{
    wxString line;

    if( !nextLine( line ) )
        return false;

    wxArrayString tok = tokenize( line );

    if( tok.GetCount() < 6 )
    {
        warn( _( "insufficient text fields" ) );
        return false;
    }

    aOut.x = toDouble( tok[0] );
    aOut.y = toDouble( tok[1] );
    aOut.height = toDouble( tok[2] );
    aOut.direction = toInt( tok[3] ) % 4; // mirroring bit ignored
    aOut.width = toDouble( tok[4] );
    aOut.layer = toInt( tok[5] );

    // The text string is on the following line; an empty line yields empty text.
    if( nextLine( line ) )
        aOut.text = line;

    return true;
}


void AUTOTRAX_PARSER::parseComponent( COMPONENT& aOut )
{
    wxString line;

    // Three fixed string lines: refdes, footprint name, value.
    if( nextLine( line ) )
        aOut.refdes = line;

    if( nextLine( line ) )
        aOut.name = line;

    if( nextLine( line ) )
        aOut.value = line;

    // Two text-placement lines (refdes/value label positions) are ignored.
    nextLine( line );
    nextLine( line );

    // Component origin.
    if( nextLine( line ) )
    {
        wxArrayString tok = tokenize( line );

        if( tok.GetCount() >= 2 )
        {
            aOut.x = toDouble( tok[0] );
            aOut.y = toDouble( tok[1] );
        }
        else
        {
            warn( _( "insufficient component fields" ) );
        }
    }

    while( nextLine( line ) )
    {
        wxString kw = line;

        if( kw.StartsWith( wxT( "ENDCOMP" ) ) )
            break;

        if( kw == wxT( "CT" ) )
        {
            TRACK t;

            if( parseTrack( t ) )
                aOut.tracks.push_back( t );
        }
        else if( kw == wxT( "CA" ) )
        {
            ARC a;

            if( parseArc( a ) )
                aOut.arcs.push_back( a );
        }
        else if( kw == wxT( "CV" ) )
        {
            VIA v;

            if( parseVia( v ) )
                aOut.vias.push_back( v );
        }
        else if( kw == wxT( "CF" ) )
        {
            FILL f;

            if( parseFill( f ) )
                aOut.fills.push_back( f );
        }
        else if( kw == wxT( "CP" ) )
        {
            PAD p;

            if( parsePad( p ) )
                aOut.pads.push_back( p );
        }
        else if( kw == wxT( "CS" ) )
        {
            TEXT s;

            if( parseText( s ) )
                aOut.texts.push_back( s );
        }
    }
}


void AUTOTRAX_PARSER::parseNetDef()
{
    // A NETDEF section names a net on the line following the NETDEF keyword, then
    // describes it with optional component blocks ([..]) and a node list ((..)).
    // The importer collects the "refdes-pad" node tokens inside (..) and the node
    // table ({..}) is skipped. Component metadata is already carried by COMP.
    wxString line;

    if( !nextLine( line ) )
        return;

    wxString netName = line;

    bool inNet = false;
    bool inNodeTable = false;

    while( nextLine( line ) )
    {
        wxString s = line;

        if( s.StartsWith( wxT( "ENDPCB" ) ) )
        {
            m_index--; // let the top-level loop see ENDPCB
            return;
        }

        if( s.IsEmpty() )
            continue;

        if( s[0] == '[' )
        {
            // Skip the component block up to its closing ']'.
            while( nextLine( line ) )
            {
                if( line == wxT( "]" ) )
                    break;
            }
        }
        else if( s[0] == '(' )
        {
            inNet = true;
        }
        else if( s[0] == ')' )
        {
            return; // a NETDEF describes a single net; done at its closing ')'
        }
        else if( s[0] == '{' )
        {
            inNodeTable = true;
        }
        else if( s[0] == '}' )
        {
            inNodeTable = false;
        }
        else if( inNet && !inNodeTable && !netName.IsEmpty() )
        {
            m_board->netNodes.push_back( { netName, s } );
        }
    }
}


bool AUTOTRAX_PARSER::Parse( const wxString& aContents, BOARD_DATA& aBoard )
{
    m_board = &aBoard;

    // Empty lines are retained (wxTOKEN_RET_EMPTY) because COMP header parsing
    // counts the refdes/name/value/placement lines positionally.
    m_lines = wxStringTokenize( aContents, wxT( "\n" ), wxTOKEN_RET_EMPTY );
    m_index = 0;
    m_lineNo = 0;

    bool     haveHeader = false;
    wxString line;

    while( nextLine( line ) )
    {
        wxString kw = line;

        if( kw.IsEmpty() )
            continue;

        if( kw.StartsWith( wxT( "PCB FILE 4" ) ) )
        {
            aBoard.version = 4;
            haveHeader = true;
        }
        else if( kw.StartsWith( wxT( "PCB FILE 5" ) ) )
        {
            aBoard.version = 5;
            haveHeader = true;
        }
        else if( kw.StartsWith( wxT( "ENDPCB" ) ) )
        {
            break;
        }
        else if( kw.StartsWith( wxT( "NETDEF" ) ) )
        {
            parseNetDef();
        }
        else if( kw.StartsWith( wxT( "COMP" ) ) )
        {
            COMPONENT comp;
            parseComponent( comp );
            aBoard.components.push_back( std::move( comp ) );
        }
        else if( kw == wxT( "FT" ) )
        {
            TRACK t;

            if( parseTrack( t ) )
                aBoard.tracks.push_back( t );
        }
        else if( kw == wxT( "FA" ) )
        {
            ARC a;

            if( parseArc( a ) )
                aBoard.arcs.push_back( a );
        }
        else if( kw == wxT( "FV" ) )
        {
            VIA v;

            if( parseVia( v ) )
                aBoard.vias.push_back( v );
        }
        else if( kw == wxT( "FF" ) )
        {
            FILL f;

            if( parseFill( f ) )
                aBoard.fills.push_back( f );
        }
        else if( kw == wxT( "FP" ) )
        {
            PAD p;

            if( parsePad( p ) )
                aBoard.pads.push_back( p );
        }
        else if( kw == wxT( "FS" ) )
        {
            TEXT s;

            if( parseText( s ) )
                aBoard.texts.push_back( s );
        }
    }

    return haveHeader;
}
