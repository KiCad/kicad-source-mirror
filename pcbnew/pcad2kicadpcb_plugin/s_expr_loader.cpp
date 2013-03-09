/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012-2013 Alexander Lunev <al.lunev@yahoo.com>
 * Copyright (C) 2012 KiCad Developers, see CHANGELOG.TXT for contributors.
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

/**
 * @file s_expr_loader.cpp
 */

#include <dsnlexer.h>
#include <macros.h>
#include <wx/xml/xml.h>
#include <xnode.h>

namespace PCAD2KICAD {

static KEYWORD empty_keywords[1] = {};

void LoadInputFile( wxString aFileName, wxXmlDocument* aXmlDoc )
{
    int       tok;
    XNODE*    iNode = NULL, *cNode = NULL;
    wxString  str;
    bool      growing = false;
    bool      attr = false;
    wxCSConv  conv( wxT( "windows-1251" ) );

    FILE* fp = wxFopen( aFileName, wxT( "rt" ) );

    if( !fp )
        THROW_IO_ERROR( wxT( "Unable to open file: " ) + aFileName );

    // lexer now owns fp, will close on exception or return
    DSNLEXER lexer( empty_keywords, 0, fp,  aFileName );

    iNode = new XNODE( wxXML_ELEMENT_NODE, wxT( "www.lura.sk" ) );

    while( ( tok = lexer.NextTok() ) != DSN_EOF )
    {
        if( growing && ( tok == DSN_LEFT || tok == DSN_RIGHT ) )
        {
            if( attr )
            {
                cNode->AddAttribute( wxT( "Name" ), str.Trim( false ) );
            }
            else if( str != wxEmptyString )
            {
                cNode->AddChild( new XNODE( wxXML_TEXT_NODE, wxEmptyString, str ) );
            }

            growing = false;
            attr = false;
        }

        if( tok == DSN_RIGHT )
        {
            iNode = iNode->GetParent();
        }
        else if( tok == DSN_LEFT )
        {
            tok = lexer.NextTok();
            str = wxEmptyString;
            cNode = new XNODE( wxXML_ELEMENT_NODE, wxString( lexer.CurText(), conv ) );
            iNode->AddChild( cNode );
            iNode = cNode;
            growing = true;
        }
        else
        {
            str += wxT( ' ' );
            str += wxString( lexer.CurText(), conv );
            if( tok == DSN_STRING )
                attr = true;
        }
    }

    if( iNode )
    {
        aXmlDoc->SetRoot( iNode );
        //aXmlDoc->Save( wxT( "test.xml" ) );
    }
}

} // namespace PCAD2KICAD
