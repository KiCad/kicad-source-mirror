/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012-2013 Alexander Lunev <al.lunev@yahoo.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <io/pcad/s_expr_loader.h>

#include <dsnlexer.h>
#include <memory>
#include <macros.h>
#include <xnode.h>

#include <algorithm>

#include <wx/ffile.h>
#include <wx/string.h>
#include <wx/translation.h>
#include <wx/xml/xml.h>

namespace PCAD2KICAD {

static KEYWORD empty_keywords[1] = {};
static const char ACCEL_ASCII_KEYWORD[] = "ACCEL_ASCII";


// P-CAD text values may span lines inside one quoted string, but DSNLEXER
// scans strings within a single line.  Escape raw line breaks inside quoted
// strings so the lexer's escape handling reconstructs them; CR is dropped so
// CRLF files yield the same text the old text-mode reader produced.  The scan
// mirrors the lexer's rules.  A quote opens a string only at a token boundary
// (some writers emit unescaped quotes inside bare tokens), and inside a
// string a backslash consumes the following character, so an escaped quote
// does not terminate it.
static void escapeStringLineBreaks( std::string& aContent )
{
    std::string result;
    bool        modified = false;
    bool        inString = false;
    bool        atBoundary = true;

    for( size_t i = 0; i < aContent.size(); i++ )
    {
        char c = aContent[i];

        if( !modified && ( c == '\n' || c == '\r' ) && inString )
        {
            // first in-string line break; switch to rewriting from here on
            result.assign( aContent, 0, i );
            modified = true;
        }

        if( inString )
        {
            if( c == '\\' && i + 1 < aContent.size() )
            {
                if( modified )
                {
                    result += c;
                    result += aContent[i + 1];
                }

                ++i;
                continue;
            }

            if( c == '"' )
            {
                // a closing quote also ends the token, so a quote directly
                // after it starts a new string
                inString = false;
                atBoundary = true;
            }
            else if( c == '\n' )
            {
                result += "\\n";
                continue;
            }
            else if( c == '\r' )
            {
                continue;
            }
        }
        else if( c == '"' && atBoundary )
        {
            inString = true;
        }
        else
        {
            // NUL counts as whitespace to DSNLEXER
            atBoundary = ( c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '('
                           || c == ')' || c == '\0' );
        }

        if( modified )
            result += c;
    }

    if( modified )
        aContent.swap( result );
}


bool FileMatchesFormat( const wxString& aFileName, const std::string& aSection )
{
    wxFFile file( aFileName, wxT( "rb" ) );

    if( !file.IsOpened() )
        return false;

    char header[sizeof( ACCEL_ASCII_KEYWORD )];

    if( file.Read( header, sizeof( header ) - 1 )
        != static_cast<size_t>( sizeof( header ) - 1 ) )
    {
        return false;
    }

    if( memcmp( header, ACCEL_ASCII_KEYWORD, sizeof( ACCEL_ASCII_KEYWORD ) - 1 ) != 0 )
        return false;

    if( aSection.empty() )
        return true;

    // Writers emit the design sections at the start of a line; anchoring the
    // search there keeps a section name quoted inside a text value from
    // matching.  A file holds exactly one design section, so seeing the
    // sibling section rejects the file without reading to the end.
    const std::string wanted = "\n(" + aSection;
    const std::string sibling = ( aSection == "pcbDesign" ) ? "\n(schematicDesign"
                                                            : "\n(pcbDesign";

    const size_t overlap = std::max( wanted.size(), sibling.size() );

    // seed with a line break so a section directly after the header matches
    std::string window = "\n";
    char        buf[64 * 1024];

    for( ;; )
    {
        size_t got = file.Read( buf, sizeof( buf ) );

        if( got == 0 || got == static_cast<size_t>( wxInvalidOffset ) )
            return false;

        // normalize CRLF so the line anchor matches either ending
        window.append( buf, got );
        window.erase( std::remove( window.begin(), window.end(), '\r' ), window.end() );

        if( window.find( wanted ) != std::string::npos )
            return true;

        if( window.find( sibling ) != std::string::npos )
            return false;

        if( window.size() > overlap )
            window.erase( 0, window.size() - overlap );
    }
}


void LoadInputFile( const wxString& aFileName, wxXmlDocument* aXmlDoc )
{
    int       tok = 0;
    XNODE*    iNode = nullptr, *cNode = nullptr;
    wxString  str, propValue, content;
    wxCSConv  conv( wxT( "windows-1251" ) );

    wxFFile file( aFileName, wxT( "rb" ) );

    if( !file.IsOpened() )
        THROW_IO_ERROR( wxString::Format( _( "Cannot open file '%s'." ), aFileName ) );

    std::string fileContent;

    {
        wxFileOffset length = file.Length();

        if( length <= 0 )
            THROW_IO_ERROR( wxString::Format( _( "Cannot read file '%s'." ), aFileName ) );

        fileContent.resize( static_cast<size_t>( length ) );

        if( file.Read( fileContent.data(), fileContent.size() ) != fileContent.size() )
            THROW_IO_ERROR( wxString::Format( _( "Cannot read file '%s'." ), aFileName ) );
    }

    // check file format; the first line starts with "ACCEL_ASCII" with optional
    // stuff on the same line after that.
    if( fileContent.compare( 0, sizeof( ACCEL_ASCII_KEYWORD ) - 1, ACCEL_ASCII_KEYWORD ) != 0 )
        THROW_IO_ERROR( wxString::Format( _( "'%s' is not a P-CAD ASCII file." ), aFileName ) );

    escapeStringLineBreaks( fileContent );

    DSNLEXER lexer( empty_keywords, 0, nullptr, fileContent, aFileName );

    // owns the tree on every throw path until it is handed to the document
    std::unique_ptr<XNODE> root( new XNODE( wxXML_ELEMENT_NODE, wxT( "www.lura.sk" ) ) );

    iNode = root.get();

    while( ( tok = lexer.NextTok() ) != DSN_EOF )
    {
        if( tok == DSN_RIGHT )
        {
            iNode = iNode->GetParent();

            // this will happen if there are more right parens than left
            if( !iNode )
                THROW_IO_ERROR( wxString::Format( _( "Unbalanced parentheses in '%s'." ),
                                                  aFileName ) );
        }
        else if( tok == DSN_LEFT )
        {
            tok = lexer.NextTok();
            str = wxEmptyString;
            cNode = new XNODE( wxXML_ELEMENT_NODE, wxString( lexer.CurText(), conv ) );
            iNode->AddChild( cNode );
            iNode = cNode;
        }
        else if( cNode )
        {
            str = wxString( lexer.CurText(), conv );

            if( tok == DSN_STRING )
            {
                // update attribute
                if( iNode->GetAttribute( wxT( "Name" ), &propValue ) )
                {
                    iNode->DeleteAttribute( wxT( "Name" ) );
                    iNode->AddAttribute( wxT( "Name" ), propValue + wxT( ' ' ) + str );
                }
                else
                {
                    iNode->AddAttribute( wxT( "Name" ), str );
                }
            }
            else if( str != wxEmptyString )
            {
                // update node content
                content = cNode->GetNodeContent() + wxT( ' ' ) + str;

                if( cNode->GetChildren() )
                    cNode->GetChildren()->SetContent( content );
                else
                    cNode->AddChild( new wxXmlNode( wxXML_TEXT_NODE, wxEmptyString, content ) );
            }
        }
    }

    // more left parens than right means the file was truncated mid-save
    if( iNode != root.get() )
        THROW_IO_ERROR( wxString::Format( _( "Unbalanced parentheses in '%s'." ), aFileName ) );

    aXmlDoc->SetRoot( root.release() );
}

} // namespace PCAD2KICAD
