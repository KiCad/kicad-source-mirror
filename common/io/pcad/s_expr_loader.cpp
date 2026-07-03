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
#include <macros.h>
#include <xnode.h>

#include <wx/ffile.h>
#include <wx/string.h>
#include <wx/xml/xml.h>

namespace PCAD2KICAD {

static KEYWORD empty_keywords[1] = {};
static const char ACCEL_ASCII_KEYWORD[] = "ACCEL_ASCII";


// P-CAD text values may span lines inside one quoted string, but DSNLEXER
// scans strings within a single line.  Escape raw line breaks inside quoted
// strings so the lexer's escape handling reconstructs them.  The scan mirrors
// the lexer's rules: a quote opens a string only at a token boundary (some
// writers emit unescaped quotes inside bare tokens), and inside a string a
// backslash consumes the following character, so an escaped quote does not
// terminate it.
static void escapeStringLineBreaks( std::string& aContent )
{
    std::string result;
    result.reserve( aContent.size() );

    bool inString = false;
    char prev = ' ';

    for( size_t i = 0; i < aContent.size(); i++ )
    {
        char c = aContent[i];

        if( inString )
        {
            if( c == '\\' && i + 1 < aContent.size() )
            {
                result += c;
                result += aContent[++i];
                prev = aContent[i];
                continue;
            }

            if( c == '"' )
            {
                inString = false;
            }
            else if( c == '\n' )
            {
                result += "\\n";
                prev = c;
                continue;
            }
            else if( c == '\r' )
            {
                result += "\\r";
                prev = c;
                continue;
            }
        }
        else if( c == '"' && ( prev == ' ' || prev == '\t' || prev == '\n' || prev == '\r'
                               || prev == '(' || prev == ')' ) )
        {
            inString = true;
        }

        result += c;
        prev = c;
    }

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

    // Stream the rest of the file looking for the section token, keeping an
    // overlap so a match split across reads is still found.
    std::string window;
    char        buf[64 * 1024];

    for( ;; )
    {
        size_t got = file.Read( buf, sizeof( buf ) );

        if( got == 0 || got == static_cast<size_t>( wxInvalidOffset ) )
            return false;

        window.append( buf, got );

        if( window.find( aSection ) != std::string::npos )
            return true;

        if( window.size() > aSection.size() )
            window.erase( 0, window.size() - aSection.size() );
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
        THROW_IO_ERROR( wxT( "Unable to open file: " ) + aFileName );

    std::string fileContent;

    {
        wxFileOffset length = file.Length();

        if( length <= 0 )
            THROW_IO_ERROR( wxT( "Unable to read file: " ) + aFileName );

        fileContent.resize( static_cast<size_t>( length ) );

        if( file.Read( fileContent.data(), fileContent.size() ) != fileContent.size() )
            THROW_IO_ERROR( wxT( "Unable to read file: " ) + aFileName );
    }

    // check file format; the first line starts with "ACCEL_ASCII" with optional
    // stuff on the same line after that.
    if( fileContent.compare( 0, sizeof( ACCEL_ASCII_KEYWORD ) - 1, ACCEL_ASCII_KEYWORD ) != 0 )
        THROW_IO_ERROR( wxT( "Unknown file type" ) );

    escapeStringLineBreaks( fileContent );

    DSNLEXER lexer( empty_keywords, 0, nullptr, fileContent, aFileName );

    iNode = new XNODE( wxXML_ELEMENT_NODE, wxT( "www.lura.sk" ) );

    while( ( tok = lexer.NextTok() ) != DSN_EOF )
    {
        if( tok == DSN_RIGHT )
        {
            iNode = iNode->GetParent();

            // this will happen if there are more right parens than left
            if( !iNode )
                THROW_IO_ERROR( wxT( "Unexpected right paren" ) );
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

    if( iNode )
        aXmlDoc->SetRoot( iNode );
}

} // namespace PCAD2KICAD
