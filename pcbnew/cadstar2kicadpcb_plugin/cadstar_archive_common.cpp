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

#include <cadstar_archive_common.h>


void CADSTAR_ARCHIVE_COMMON::EVALUE::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "E" ) );

    if( ( !GetXmlAttributeIDString( aNode, 0 ).ToLong( &Base ) )
            || ( !GetXmlAttributeIDString( aNode, 1 ).ToLong( &Exponent ) ) )
        THROW_PARSING_IO_ERROR( wxT( "Base and Exponent" ),
                wxString::Format(
                        "%s->%s", aNode->GetParent()->GetName(), aNode->GetParent()->GetName() ) );
}


void CADSTAR_ARCHIVE_COMMON::POINT::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "PT" ) );

    X = GetXmlAttributeIDLong( aNode, 0 );
    Y = GetXmlAttributeIDLong( aNode, 1 );
}


bool CADSTAR_ARCHIVE_COMMON::VERTEX::IsVertex( XNODE* aNode )
{
    wxString aNodeName = aNode->GetName();

    if( aNodeName == wxT( "PT" ) || aNodeName == wxT( "ACWARC" ) || aNodeName == wxT( "CWARC" )
            || aNodeName == wxT( "CWSEMI" ) || aNodeName == wxT( "ACWSEMI" ) )
        return true;
    else
        return false;
}


void CADSTAR_ARCHIVE_COMMON::VERTEX::Parse( XNODE* aNode )
{
    wxASSERT( IsVertex( aNode ) );

    wxString aNodeName = aNode->GetName();

    if( aNodeName == wxT( "PT" ) )
    {
        Type     = VERTEX_TYPE::POINT;
        Center.X = UNDEFINED_VALUE;
        Center.Y = UNDEFINED_VALUE;
        End.Parse( aNode );
    }
    else if( aNodeName == wxT( "ACWARC" ) || aNodeName == wxT( "CWARC" ) )
    {
        if( aNodeName == wxT( "ACWARC" ) )
            Type = VERTEX_TYPE::ANTICLOCKWISE_ARC;
        else
            Type = VERTEX_TYPE::CLOCKWISE_ARC;

        std::vector<POINT> pts = ParseAllChildPoints( aNode, true, 2 );

        Center = pts[0];
        End    = pts[1];
    }
    else if( aNodeName == wxT( "ACWSEMI" ) || aNodeName == wxT( "CWSEMI" ) )
    {
        if( aNodeName == wxT( "ACWSEMI" ) )
            Type = VERTEX_TYPE::ANTICLOCKWISE_SEMICIRCLE;
        else
            Type = VERTEX_TYPE::CLOCKWISE_SEMICIRCLE;

        Center.X = UNDEFINED_VALUE;
        Center.Y = UNDEFINED_VALUE;

        std::vector<POINT> pts = ParseAllChildPoints( aNode, true, 1 );

        End = pts[0];
    }
    else
    {
        wxASSERT_MSG( true, wxT( "Unknown VERTEX type" ) );
    }
}


double CADSTAR_ARCHIVE_COMMON::EVALUE::GetDouble()
{
    return Base * std::pow( 10.0, Exponent );
}


void CADSTAR_ARCHIVE_COMMON::InsertAttributeAtEnd( XNODE* aNode, wxString aValue )
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


XNODE* CADSTAR_ARCHIVE_COMMON::LoadArchiveFile(
        const wxString& aFileName, const wxString& aFileTypeIdentifier )
{
    KEYWORD   emptyKeywords[1] = {};
    XNODE *   iNode = NULL, *cNode = NULL;
    int       tok;
    bool      cadstarFileCheckDone = false;
    wxString  str;
    wxCSConv  win1252( wxT( "windows-1252" ) );
    wxMBConv* conv = &win1252; // Initial testing suggests file encoding to be Windows-1252
                               // More samples required.
    FILE* fp = wxFopen( aFileName, wxT( "rt" ) );

    if( !fp )
        THROW_IO_ERROR( wxString::Format( _( "Cannot open file '%s'" ), aFileName ) );

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

                if( cNode->GetName() != aFileTypeIdentifier )
                {
                    THROW_IO_ERROR( _( "The selected file is not valid or might be corrupt!" ) );
                }
                cadstarFileCheckDone = true;
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

wxString CADSTAR_ARCHIVE_COMMON::GetXmlAttributeIDString( XNODE* aNode, unsigned int aID )
{
    wxString attrName, retVal;
    attrName = "attr";
    attrName << aID;

    if( !aNode->GetAttribute( attrName, &retVal ) )
        THROW_MISSING_PARAMETER_IO_ERROR( std::to_string( aID ), aNode->GetName() );

    return retVal;
}


long CADSTAR_ARCHIVE_COMMON::GetXmlAttributeIDLong( XNODE* aNode, unsigned int aID )
{
    long retVal;

    if( !GetXmlAttributeIDString( aNode, aID ).ToLong( &retVal ) )
        THROW_PARSING_IO_ERROR( std::to_string( aID ), aNode->GetName() );

    return retVal;
}


void CADSTAR_ARCHIVE_COMMON::CheckNoChildNodes( XNODE* aNode )
{
    if( aNode->GetChildren() )
    {
        THROW_UNKNOWN_NODE_IO_ERROR( aNode->GetChildren()->GetName(), aNode->GetName() );
    }
}


void CADSTAR_ARCHIVE_COMMON::CheckNoNextNodes( XNODE* aNode )
{
    if( aNode->GetNext() )
    {
        THROW_UNKNOWN_NODE_IO_ERROR( aNode->GetNext()->GetName(), aNode->GetParent()->GetName() );
    }
}


void CADSTAR_ARCHIVE_COMMON::ParseChildEValue( XNODE* aNode, EVALUE& aValueToParse )
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

std::vector<CADSTAR_ARCHIVE_COMMON::POINT> CADSTAR_ARCHIVE_COMMON::ParseAllChildPoints(
        XNODE* aNode, bool aTestAllChildNodes, int aExpectedNumPoints )
{
    std::vector<POINT> retVal;

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

    if( aExpectedNumPoints != UNDEFINED_VALUE && retVal.size() != aExpectedNumPoints )
        THROW_IO_ERROR( wxString::Format(
                _( "Unexpected number of points in '%s'. Found %d but expected %d." ),
                aNode->GetName(), retVal.size(), aExpectedNumPoints ) );

    return retVal;
}


std::vector<CADSTAR_ARCHIVE_COMMON::VERTEX> CADSTAR_ARCHIVE_COMMON::ParseAllChildVertices(
        XNODE* aNode, bool aTestAllChildNodes )
{
    std::vector<VERTEX> retVal;

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        if( VERTEX::IsVertex( cNode ) )
        {
            VERTEX vertex;
            //TODO try.. catch + throw again with more detailed error information
            vertex.Parse( cNode );
            retVal.push_back( vertex );
        }
        else if( aTestAllChildNodes )
            THROW_UNKNOWN_NODE_IO_ERROR( cNode->GetName(), aNode->GetName() );
    }

    return retVal;
}


std::vector<CADSTAR_ARCHIVE_COMMON::CUTOUT> CADSTAR_ARCHIVE_COMMON::ParseAllChildCutouts(
        XNODE* aNode, bool aTestAllChildNodes )
{
    std::vector<CUTOUT> retVal;

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        if( cNode->GetName() == wxT( "CUTOUT" ) )
        {
            CUTOUT cutout;
            //TODO try.. catch + throw again with more detailed error information
            cutout.Parse( cNode );
            retVal.push_back( cutout );
        }
        else if( aTestAllChildNodes )
            THROW_UNKNOWN_NODE_IO_ERROR( cNode->GetName(), aNode->GetName() );
    }

    return retVal;
}

void CADSTAR_ARCHIVE_COMMON::CUTOUT::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "CUTOUT" ) );

    Vertices = ParseAllChildVertices( aNode, true );
}


bool CADSTAR_ARCHIVE_COMMON::SHAPE::IsShape( XNODE* aNode )
{
    wxString aNodeName = aNode->GetName();

    if( aNodeName == wxT( "OPENSHAPE" ) || aNodeName == wxT( "OUTLINE" )
            || aNodeName == wxT( "SOLID" ) || aNodeName == wxT( "HATCHED" ) )
        return true;
    else
        return false;
}

void CADSTAR_ARCHIVE_COMMON::SHAPE::Parse( XNODE* aNode )
{
    wxASSERT( IsShape( aNode ) );

    wxString aNodeName = aNode->GetName();

    if( aNodeName == wxT( "OPENSHAPE" ) )
    {
        Type     = SHAPE_TYPE::OPENSHAPE;
        Vertices = ParseAllChildVertices( aNode, true );
        Cutouts.clear();
        HatchCodeID = wxEmptyString;
    }
    else if( aNodeName == wxT( "OUTLINE" ) )
    {
        Type        = SHAPE_TYPE::OUTLINE;
        Vertices    = ParseAllChildVertices( aNode, false );
        Cutouts     = ParseAllChildCutouts( aNode, false );
        HatchCodeID = wxEmptyString;
    }
    else if( aNodeName == wxT( "SOLID" ) )
    {
        Type        = SHAPE_TYPE::SOLID;
        Vertices    = ParseAllChildVertices( aNode, false );
        Cutouts     = ParseAllChildCutouts( aNode, false );
        HatchCodeID = wxEmptyString;
    }
    else if( aNodeName == wxT( "HATCHED" ) )
    {
        Type        = SHAPE_TYPE::HATCHED;
        Vertices    = ParseAllChildVertices( aNode, false );
        Cutouts     = ParseAllChildCutouts( aNode, false );
        HatchCodeID = GetXmlAttributeIDString( aNode, 0 );
    }
    else
        wxASSERT_MSG( true, wxT( "Unknown SHAPE type" ) );
}
