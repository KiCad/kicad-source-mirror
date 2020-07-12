/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Roberto Fernandez Bautista <@Qbort>
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file cadstar_common.cpp
 * @brief Helper functions and common defines
 */

#include <cadstar_common.h>

XNODE* CADSTAR_COMMON::LoadArchiveFile( const wxString& aFileName, FILE_TYPE aType )
{
    KEYWORD   emptyKeywords[1] = {};
    XNODE *   iNode = NULL, *cNode = NULL;
    int       tok;
    bool      cadstarFileCheckDone = false;
    wxString  str, fileIdentifier;
    wxCSConv  win1252( wxT( "windows-1252" ) );
    wxMBConv* conv = &win1252; // Initial testing suggests file encoding to be Windows-1252
                               // More samples required.
    FILE* fp = wxFopen( aFileName, wxT( "rt" ) );

    if( !fp )
        THROW_IO_ERROR( wxString::Format( _( "Cannot open file '%s'" ), aFileName ) );


    switch( aType )
    {
    case FILE_TYPE::PCB_ARCHIVE:
        fileIdentifier = wxT( "CADSTARPCB" );
        break;

        //add others here
        // SCHEMATIC_ARCHIVE
        // ...

    default:
        wxASSERT_MSG( true, "Unknown CADSTAR filetype specified" );
    }

    DSNLEXER lexer( emptyKeywords, 0, fp, aFileName );

    while( ( tok = lexer.NextTok() ) != DSN_EOF )
    {
        if( tok == DSN_RIGHT )
        {
            cNode = iNode;
            if( cNode )
            {
                iNode = cNode->GetParent();
            }
            else
            {
                //too many closing brackets
                THROW_IO_ERROR( _( "The selected file is not valid or might be corrupt!" ) );
            }
        }
        else if( tok == DSN_LEFT )
        {
            tok   = lexer.NextTok();
            str   = wxString( lexer.CurText(), *conv );
            cNode = new XNODE( wxXML_ELEMENT_NODE, str );

            if( iNode )
            {
                iNode->AddChild( cNode );
            }
            else if( !cadstarFileCheckDone )
            {

                if( cNode->GetName() != fileIdentifier )
                {
                    THROW_IO_ERROR( _( "The selected file is not valid or might be corrupt!" ) );
                }
            }

            iNode = cNode;
        }
        else if( iNode )
        {
            str = wxString( lexer.CurText(), *conv );

            if( !str.IsEmpty() )
            {
                wxString result, paramName = "attr0";
                int      i = 0;

                while( iNode->GetAttribute( paramName, &result ) )
                {
                    paramName = wxT( "attr" );
                    paramName << i++;
                }

                iNode->AddAttribute( paramName, str );
            }
        }
        else
        {
            //not enough closing brackets
            THROW_IO_ERROR( _( "The selected file is not valid or might be corrupt!" ) );
        }
    }

    if( iNode != NULL )
    {
        //not enough closing brackets
        THROW_IO_ERROR( _( "The selected file is not valid or might be corrupt!" ) );
    }

    if( cNode )
    {
        return cNode;
    }
    else
    {
        THROW_IO_ERROR( _( "The selected file is not valid or might be corrupt!" ) );
    }

    return NULL;
}

wxString CADSTAR_COMMON::GetAttributeID( XNODE* aNode, unsigned int aID )
{
    wxString attrName = "attr";
    attrName << aID;
    return aNode->GetAttribute( attrName, wxEmptyString );
}

void CADSTAR_COMMON::CheckNoChildNodes( XNODE* aNode, wxString aLocation )
{
    if( aNode->GetChildren() )
    {
        THROW_UNKNOWN_NODE_IO_ERROR( aNode->GetChildren()->GetName(), aLocation );
    }
}
