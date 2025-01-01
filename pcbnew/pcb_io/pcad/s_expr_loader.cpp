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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <pcad/s_expr_loader.h>

#include <dsnlexer.h>
#include <macros.h>
#include <xnode.h>

#include <wx/string.h>
#include <wx/xml/xml.h>
#include <wx/wxcrt.h>

namespace PCAD2KICAD {

static KEYWORD empty_keywords[1] = {};
static const char ACCEL_ASCII_KEYWORD[] = "ACCEL_ASCII";


void LoadInputFile( const wxString& aFileName, wxXmlDocument* aXmlDoc )
{
    char      line[sizeof( ACCEL_ASCII_KEYWORD )];
    int       tok;
    XNODE*    iNode = nullptr, *cNode = nullptr;
    wxString  str, propValue, content;
    wxCSConv  conv( wxT( "windows-1251" ) );

    FILE* fp = wxFopen( aFileName, wxT( "rt" ) );

    if( !fp )
        THROW_IO_ERROR( wxT( "Unable to open file: " ) + aFileName );

    // check file format
    if( !fgets( line, sizeof( line ), fp )
        // first line starts with "ACCEL_ASCII" with optional stuff on same line after that.
        || memcmp( line, ACCEL_ASCII_KEYWORD, sizeof(ACCEL_ASCII_KEYWORD)-1 ) )
        THROW_IO_ERROR( wxT( "Unknown file type" ) );

    // rewind the file
    fseek( fp, 0, SEEK_SET );

    // lexer now owns fp, will close on exception or return
    DSNLEXER lexer( empty_keywords, 0, nullptr, fp,  aFileName );

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
    {
        aXmlDoc->SetRoot( iNode );
        //aXmlDoc->Save( wxT( "test.xml" ) );
    }
}

} // namespace PCAD2KICAD
