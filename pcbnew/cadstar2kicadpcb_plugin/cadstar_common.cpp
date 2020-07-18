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


void CADSTAR_COMMON::EVALUE::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "E" ) );

    if( ( !CADSTAR_COMMON::GetAttributeID( aNode, 0 ).ToLong( &Base ) )
            || ( !CADSTAR_COMMON::GetAttributeID( aNode, 1 ).ToLong( &Exponent ) ) )
        THROW_PARSING_IO_ERROR( wxT( "Base and Exponent" ),
                wxString::Format(
                        "%s->%s", aNode->GetParent()->GetName(), aNode->GetParent()->GetName() ) );
}


void CADSTAR_COMMON::POINT::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "PT" ) );

    X = CADSTAR_COMMON::GetAttributeIDLong( aNode, 0 );
    Y = CADSTAR_COMMON::GetAttributeIDLong( aNode, 1 );
}


double CADSTAR_COMMON::EVALUE::GetDouble()
{
    return Base * std::pow( 10.0, Exponent );
}


void CADSTAR_COMMON::InsertAttributeAtEnd( XNODE* aNode, wxString aValue )
{
    wxString result, paramName = "attr0";
    int      i = 0;

    while( aNode->GetAttribute( paramName, &result ) )
    {
        paramName = wxT( "attr" );
        paramName << i++;
    }

    aNode->AddAttribute( paramName, aValue );
}


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
                //we will add it as attribute as well as child node
                InsertAttributeAtEnd( iNode, str );
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
            //Insert even if string is empty
            InsertAttributeAtEnd( iNode, str );
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
        //no data?
        THROW_IO_ERROR( _( "The selected file is not valid or might be corrupt!" ) );
    }

    return NULL;
}

wxString CADSTAR_COMMON::GetAttributeID( XNODE* aNode, unsigned int aID )
{
    wxString attrName, retVal;
    attrName = "attr";
    attrName << aID;

    if( !aNode->GetAttribute( attrName, &retVal ) )
        THROW_MISSING_PARAMETER_IO_ERROR( std::to_string( aID ), aNode->GetName() );

    return retVal;
}


long CADSTAR_COMMON::GetAttributeIDLong( XNODE* aNode, unsigned int aID )
{
    long retVal;

    if( !CADSTAR_COMMON::GetAttributeID( aNode, aID ).ToLong( &retVal ) )
        THROW_PARSING_IO_ERROR( std::to_string( aID ), aNode->GetName() );

    return retVal;
}


void CADSTAR_COMMON::CheckNoChildNodes( XNODE* aNode )
{
    if( aNode->GetChildren() )
    {
        THROW_UNKNOWN_NODE_IO_ERROR( aNode->GetChildren()->GetName(), aNode->GetName() );
    }
}


void CADSTAR_COMMON::CheckNoNextNodes( XNODE* aNode )
{
    if( aNode->GetNext() )
    {
        THROW_UNKNOWN_NODE_IO_ERROR( aNode->GetNext()->GetName(), aNode->GetParent()->GetName() );
    }
}


void CADSTAR_COMMON::ParseChildEValue( XNODE* aNode, CADSTAR_COMMON::EVALUE& aValueToParse )
{
    if( aNode->GetChildren()->GetName() == wxT( "E" ) )
    {
        aValueToParse.Parse( aNode->GetChildren() );
    }
    else
    {
        THROW_UNKNOWN_NODE_IO_ERROR( aNode->GetChildren()->GetName(), aNode->GetName() );
    }
}

std::vector<CADSTAR_COMMON::POINT> CADSTAR_COMMON::ParseAllChildPoints(
        XNODE* aNode, bool aTestAllChildNodes, int aExpectedNumPoints )
{
    std::vector<CADSTAR_COMMON::POINT> retVal;

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        if( cNode->GetName() == wxT( "PT" ) )
        {
            POINT pt;
            //TODO try.. catch + throw again with more detailed error information
            pt.Parse( cNode );
            retVal.push_back( pt );
        }
        else if( aTestAllChildNodes )
            THROW_UNKNOWN_NODE_IO_ERROR( cNode->GetName(), aNode->GetName() );
    }

    if( aExpectedNumPoints >= 0 && retVal.size() != aExpectedNumPoints )
        THROW_IO_ERROR( wxString::Format(
                _( "Unexpected number of points in '%s'. Found %d but expected %d." ),
                aNode->GetName(), retVal.size(), aExpectedNumPoints ) );

    return retVal;
}
