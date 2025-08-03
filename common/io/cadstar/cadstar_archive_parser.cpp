/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020-2021 Roberto Fernandez Bautista <roberto.fer.bau@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file cadstar_archive_parser.cpp
 * @brief Helper functions and common defines between schematic and PCB Archive files
 */
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/xml/xml.h>

#include <dsnlexer.h>
#include <geometry/shape_poly_set.h>
#include <io/cadstar/cadstar_archive_parser.h>
#include <eda_item.h>
#include <eda_text.h>
#include <macros.h>
#include <progress_reporter.h>
#include <string_utils.h>
#include <trigo.h>

// Ratio derived from CADSTAR default font. See doxygen comment in cadstar_archive_parser.h
const double CADSTAR_ARCHIVE_PARSER::TXT_HEIGHT_RATIO = ( 24.0 - 5.0 ) / 24.0;

// Cadstar fields and their KiCad equivalent
const std::map<CADSTAR_ARCHIVE_PARSER::TEXT_FIELD_NAME, wxString>
        CADSTAR_ARCHIVE_PARSER::CADSTAR_TO_KICAD_FIELDS =
                {   { TEXT_FIELD_NAME::DESIGN_TITLE, wxT( "DESIGN_TITLE" ) },
                    { TEXT_FIELD_NAME::SHORT_JOBNAME, wxT( "SHORT_JOBNAME" ) },
                    { TEXT_FIELD_NAME::LONG_JOBNAME, wxT( "LONG_JOBNAME" ) },
                    { TEXT_FIELD_NAME::NUM_OF_SHEETS, wxT( "##" ) },
                    { TEXT_FIELD_NAME::SHEET_NUMBER, wxT( "#" ) },
                    { TEXT_FIELD_NAME::SHEET_NAME, wxT( "SHEETNAME" ) },
                    { TEXT_FIELD_NAME::VARIANT_NAME, wxT( "VARIANT_NAME" ) },
                    { TEXT_FIELD_NAME::VARIANT_DESCRIPTION, wxT( "VARIANT_DESCRIPTION" ) },
                    { TEXT_FIELD_NAME::REG_USER, wxT( "REG_USER" ) },
                    { TEXT_FIELD_NAME::COMPANY_NAME, wxT( "COMPANY_NAME" ) },
                    { TEXT_FIELD_NAME::CURRENT_USER, wxT( "CURRENT_USER" ) },
                    { TEXT_FIELD_NAME::DATE, wxT( "DATE" ) },
                    { TEXT_FIELD_NAME::TIME, wxT( "TIME" ) },
                    { TEXT_FIELD_NAME::MACHINE_NAME, wxT( "MACHINE_NAME" ) } };


void CADSTAR_ARCHIVE_PARSER::FORMAT::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "FORMAT" ) );

    Type    = GetXmlAttributeIDString( aNode, 0 );
    SomeInt = GetXmlAttributeIDLong( aNode, 1 );
    Version = GetXmlAttributeIDLong( aNode, 2 );
}


void CADSTAR_ARCHIVE_PARSER::TIMESTAMP::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "TIMESTAMP" ) );

    if( !GetXmlAttributeIDString( aNode, 0 ).ToLong( &Year )
            || !GetXmlAttributeIDString( aNode, 1 ).ToLong( &Month )
            || !GetXmlAttributeIDString( aNode, 2 ).ToLong( &Day )
            || !GetXmlAttributeIDString( aNode, 3 ).ToLong( &Hour )
            || !GetXmlAttributeIDString( aNode, 4 ).ToLong( &Minute )
            || !GetXmlAttributeIDString( aNode, 5 ).ToLong( &Second ) )
        THROW_PARSING_IO_ERROR( wxT( "TIMESTAMP" ), wxString::Format( "HEADER" ) );
}


void CADSTAR_ARCHIVE_PARSER::HEADER::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "HEADER" ) );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString nodeName = cNode->GetName();

        if( nodeName == wxT( "FORMAT" ) )
        {
            Format.Parse( cNode, aContext );
        }
        else if( nodeName == wxT( "JOBFILE" ) )
        {
            JobFile = GetXmlAttributeIDString( cNode, 0 );
        }
        else if( nodeName == wxT( "JOBTITLE" ) )
        {
            JobTitle = GetXmlAttributeIDString( cNode, 0 );
        }
        else if( nodeName == wxT( "GENERATOR" ) )
        {
            Generator = GetXmlAttributeIDString( cNode, 0 );
        }
        else if( nodeName == wxT( "RESOLUTION" ) )
        {
            XNODE* subNode = cNode->GetChildren();

            if( ( subNode->GetName() == wxT( "METRIC" ) )
                    && ( GetXmlAttributeIDString( subNode, 0 ) == wxT( "HUNDREDTH" ) )
                    && ( GetXmlAttributeIDString( subNode, 1 ) == wxT( "MICRON" ) ) )
            {
                Resolution = RESOLUTION::HUNDREDTH_MICRON;
            }
            else
            {
                // TODO Need to find out if there are other possible resolutions. Logically
                // there must be other base units that could be used, such as "IMPERIAL INCH"
                // or "METRIC MM" but so far none of settings in CADSTAR generated a different
                // output resolution to "HUNDREDTH MICRON"
                THROW_UNKNOWN_NODE_IO_ERROR( subNode->GetName(), wxT( "HEADER->RESOLUTION" ) );
            }
        }
        else if( nodeName == wxT( "TIMESTAMP" ) )
        {
            Timestamp.Parse( cNode, aContext );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNode->GetName(), wxT( "HEADER" ) );
        }
    }
}


void CADSTAR_ARCHIVE_PARSER::VARIANT::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "VMASTER" ) || aNode->GetName() == wxT( "VARIANT" ) );

    ID = GetXmlAttributeIDString( aNode, 0 );

    if( aNode->GetName() == wxT( "VMASTER" ) )
    {
        Name        = GetXmlAttributeIDString( aNode, 1 );
        Description = GetXmlAttributeIDString( aNode, 2 );
    }
    else
    {
        ParentID    = GetXmlAttributeIDString( aNode, 1 );
        Name        = GetXmlAttributeIDString( aNode, 2 );
        Description = GetXmlAttributeIDString( aNode, 3 );
    }
}


void CADSTAR_ARCHIVE_PARSER::VARIANT_HIERARCHY::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "VHIERARCHY" ) );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        if( cNode->GetName() == wxT( "VMASTER" ) || cNode->GetName() == wxT( "VARIANT" ) )
        {
            VARIANT variant;
            variant.Parse( cNode, aContext );
            Variants.insert( std::make_pair( variant.ID, variant ) );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNode->GetName(), cNode->GetName() );
        }
    }
}


void CADSTAR_ARCHIVE_PARSER::LINECODE::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "LINECODE" ) );

    ID   = GetXmlAttributeIDString( aNode, 0 );
    Name = GetXmlAttributeIDString( aNode, 1 );

    if( !GetXmlAttributeIDString( aNode, 2 ).ToLong( &Width ) )
        THROW_PARSING_IO_ERROR( wxT( "Line Width" ), wxString::Format( "LINECODE -> %s", Name ) );

    XNODE* cNode = aNode->GetChildren();

    if( !cNode || cNode->GetName() != wxT( "STYLE" ) )
        THROW_UNKNOWN_NODE_IO_ERROR( cNode->GetName(), wxString::Format( "LINECODE -> %s", Name ) );

    wxString styleStr = GetXmlAttributeIDString( cNode, 0 );

    if( styleStr == wxT( "SOLID" ) )
    {
        Style = LINESTYLE::SOLID;
    }
    else if( styleStr == wxT( "DASH" ) )
    {
        Style = LINESTYLE::DASH;
    }
    else if( styleStr == wxT( "DASHDOT" ) )
    {
        Style = LINESTYLE::DASHDOT;
    }
    else if( styleStr == wxT( "DASHDOTDOT" ) )
    {
        Style = LINESTYLE::DASHDOTDOT;
    }
    else if( styleStr == wxT( "DOT" ) )
    {
        Style = LINESTYLE::DOT;
    }
    else
    {
        THROW_UNKNOWN_PARAMETER_IO_ERROR( wxString::Format( "STYLE %s", styleStr ),
                                          wxString::Format( "LINECODE -> %s", Name ) );
    }
}


void CADSTAR_ARCHIVE_PARSER::HATCH::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "HATCH" ) );

    Step      = GetXmlAttributeIDLong( aNode, 0 );
    LineWidth = GetXmlAttributeIDLong( aNode, 2 );

    XNODE* cNode = aNode->GetChildren();

    if( !cNode || cNode->GetName() != wxT( "ORIENT" ) )
        THROW_MISSING_NODE_IO_ERROR( wxT( "ORIENT" ), wxT( "HATCH" ) );

    OrientAngle = GetXmlAttributeIDLong( cNode, 0 );
}


void CADSTAR_ARCHIVE_PARSER::HATCHCODE::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "HATCHCODE" ) );

    ID   = GetXmlAttributeIDString( aNode, 0 );
    Name = GetXmlAttributeIDString( aNode, 1 );

    XNODE*   cNode    = aNode->GetChildren();
    wxString location = wxString::Format( "HATCHCODE -> %s", Name );

    for( ; cNode; cNode = cNode->GetNext() )
    {
        if( cNode->GetName() != wxT( "HATCH" ) )
            THROW_UNKNOWN_NODE_IO_ERROR( cNode->GetName(), location );

        HATCH hatch;
        hatch.Parse( cNode, aContext );
        Hatches.push_back( hatch );
    }
}


void CADSTAR_ARCHIVE_PARSER::FONT::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "FONT" ) );

    Name      = GetXmlAttributeIDString( aNode, 0 );
    Modifier1 = GetXmlAttributeIDLong( aNode, 1 );
    Modifier2 = GetXmlAttributeIDLong( aNode, 2 );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( cNodeName == wxT( "ITALIC" ) )
            Italic = true;
        else if( cNodeName == wxT( "KERNING" ) )
            KerningPairs = true;
        else
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, aNode->GetName() );
    }
}


void CADSTAR_ARCHIVE_PARSER::TEXTCODE::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "TEXTCODE" ) );

    ID   = GetXmlAttributeIDString( aNode, 0 );
    Name = GetXmlAttributeIDString( aNode, 1 );

    LineWidth = GetXmlAttributeIDLong( aNode, 2 );
    Height    = GetXmlAttributeIDLong( aNode, 3 );
    Width     = GetXmlAttributeIDLong( aNode, 4 );

    XNODE* cNode = aNode->GetChildren();

    if( cNode )
    {
        if( cNode->GetName() == wxT( "FONT" ) )
            Font.Parse( cNode, aContext );
        else
            THROW_UNKNOWN_NODE_IO_ERROR( cNode->GetName(), aNode->GetName() );
    }
}


void CADSTAR_ARCHIVE_PARSER::ROUTEREASSIGN::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "ROUTEREASSIGN" ) );

    LayerID      = GetXmlAttributeIDString( aNode, 0 );
    OptimalWidth = GetXmlAttributeIDLong( aNode, 1, false );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( cNodeName == wxT( "NECKWIDTH" ) )
            NeckedWidth = GetXmlAttributeIDLong( cNode, 0 );
        else if( cNodeName == wxT( "SROUTEWIDTH" ) )
            OptimalWidth = GetXmlAttributeIDLong( cNode, 0 );
        else if( cNodeName == wxT( "MINWIDTH" ) )
            MinWidth = GetXmlAttributeIDLong( cNode, 0 );
        else if( cNodeName == wxT( "MAXWIDTH" ) )
            MaxWidth = GetXmlAttributeIDLong( cNode, 0 );
        else
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, aNode->GetName() );
    }
}


void CADSTAR_ARCHIVE_PARSER::ROUTECODE::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "ROUTECODE" ) );

    ID           = GetXmlAttributeIDString( aNode, 0 );
    Name         = GetXmlAttributeIDString( aNode, 1 );
    OptimalWidth = GetXmlAttributeIDLong( aNode, 2, false );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( cNodeName == wxT( "NECKWIDTH" ) )
        {
            NeckedWidth = GetXmlAttributeIDLong( cNode, 0 );
        }
        else if( cNodeName == wxT( "SROUTEWIDTH" ) )
        {
            OptimalWidth = GetXmlAttributeIDLong( cNode, 0 );
        }
        else if( cNodeName == wxT( "MINWIDTH" ) )
        {
            MinWidth = GetXmlAttributeIDLong( cNode, 0 );
        }
        else if( cNodeName == wxT( "MAXWIDTH" ) )
        {
            MaxWidth = GetXmlAttributeIDLong( cNode, 0 );
        }
        else if( cNodeName == wxT( "ROUTEREASSIGN" ) )
        {
            ROUTEREASSIGN routereassign;
            routereassign.Parse( cNode, aContext );
            RouteReassigns.push_back( routereassign );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, aNode->GetName() );
        }
    }
}


double CADSTAR_ARCHIVE_PARSER::EVALUE::GetDouble()
{
    return Base * std::pow( 10.0, Exponent );
}


void CADSTAR_ARCHIVE_PARSER::EVALUE::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "E" ) );

    if( ( !GetXmlAttributeIDString( aNode, 0 ).ToLong( &Base ) )
            || ( !GetXmlAttributeIDString( aNode, 1 ).ToLong( &Exponent ) ) )
    {
        THROW_PARSING_IO_ERROR( wxT( "Base and Exponent" ),
                                wxString::Format( "%s->%s", aNode->GetParent()->GetName(),
                                                  aNode->GetParent()->GetName() ) );
    }
}


void CADSTAR_ARCHIVE_PARSER::POINT::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "PT" ) );

    x = GetXmlAttributeIDLong( aNode, 0 );
    y = GetXmlAttributeIDLong( aNode, 1 );
}


void CADSTAR_ARCHIVE_PARSER::LONGPOINT::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "PT" ) );

    x = GetXmlAttributeIDLong( aNode, 0 );
    y = GetXmlAttributeIDLong( aNode, 1 );
}


bool CADSTAR_ARCHIVE_PARSER::VERTEX::IsVertex( XNODE* aNode )
{
    wxString aNodeName = aNode->GetName();

    if( aNodeName == wxT( "PT" ) || aNodeName == wxT( "ACWARC" ) || aNodeName == wxT( "CWARC" )
            || aNodeName == wxT( "CWSEMI" ) || aNodeName == wxT( "ACWSEMI" ) )
    {
        return true;
    }
    else
    {
        return false;
    }
}


void CADSTAR_ARCHIVE_PARSER::VERTEX::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( IsVertex( aNode ) );

    wxString aNodeName = aNode->GetName();

    if( aNodeName == wxT( "PT" ) )
    {
        Type     = VERTEX_TYPE::VT_POINT;
        Center.x = UNDEFINED_VALUE;
        Center.y = UNDEFINED_VALUE;
        End.Parse( aNode, aContext );
    }
    else if( aNodeName == wxT( "ACWARC" ) || aNodeName == wxT( "CWARC" ) )
    {
        if( aNodeName == wxT( "ACWARC" ) )
            Type = VERTEX_TYPE::ANTICLOCKWISE_ARC;
        else
            Type = VERTEX_TYPE::CLOCKWISE_ARC;

        std::vector<POINT> pts = ParseAllChildPoints( aNode, aContext, true, 2 );

        Center = pts[0];
        End    = pts[1];
    }
    else if( aNodeName == wxT( "ACWSEMI" ) || aNodeName == wxT( "CWSEMI" ) )
    {
        if( aNodeName == wxT( "ACWSEMI" ) )
            Type = VERTEX_TYPE::ANTICLOCKWISE_SEMICIRCLE;
        else
            Type = VERTEX_TYPE::CLOCKWISE_SEMICIRCLE;

        Center.x = UNDEFINED_VALUE;
        Center.y = UNDEFINED_VALUE;

        std::vector<POINT> pts = ParseAllChildPoints( aNode, aContext, true, 1 );

        End = pts[0];
    }
    else
    {
        wxASSERT_MSG( true, wxT( "Unknown VERTEX type" ) );
    }
}


void CADSTAR_ARCHIVE_PARSER::VERTEX::AppendToChain( SHAPE_LINE_CHAIN* aChainToAppendTo,
        const std::function<VECTOR2I( const VECTOR2I& )> aCadstarToKicadPointCallback,
        int aAccuracy ) const
{
    if( Type == VERTEX_TYPE::VT_POINT )
    {
        aChainToAppendTo->Append( aCadstarToKicadPointCallback( End ) );
        return;
    }

    wxCHECK_MSG( aChainToAppendTo->PointCount() > 0, /*void*/,
                 "Can't append an arc to vertex to an empty chain" );

    aChainToAppendTo->Append( BuildArc( aChainToAppendTo->GetPoint( -1 ),
                                        aCadstarToKicadPointCallback ),
                              aAccuracy );
}


SHAPE_ARC CADSTAR_ARCHIVE_PARSER::VERTEX::BuildArc( const VECTOR2I& aPrevPoint,
        const std::function<VECTOR2I( const VECTOR2I& )> aCadstarToKicadPointCallback ) const
{
    wxCHECK_MSG( Type != VERTEX_TYPE::VT_POINT, SHAPE_ARC(),
                 "Can't build an arc for a straight segment!" );

    VECTOR2I startPoint = aPrevPoint;
    VECTOR2I endPoint = aCadstarToKicadPointCallback( End );
    VECTOR2I centerPoint;

    if( Type == VERTEX_TYPE::ANTICLOCKWISE_SEMICIRCLE || Type == VERTEX_TYPE::CLOCKWISE_SEMICIRCLE )
        centerPoint = ( startPoint / 2 ) + ( endPoint / 2 );
    else
        centerPoint = aCadstarToKicadPointCallback( Center );

    bool clockwise = Type == VERTEX_TYPE::CLOCKWISE_ARC
                  || Type == VERTEX_TYPE::CLOCKWISE_SEMICIRCLE;

    // A bit of a hack to figure out if we need to invert clockwise due to the transform
    VECTOR2I transform = aCadstarToKicadPointCallback( { 500, 500 } )
                       - aCadstarToKicadPointCallback( { 0, 0 } );

    if( ( transform.x > 0 && transform.y < 0 ) || ( transform.x < 0 && transform.y > 0 ) )
        clockwise = !clockwise;

    SHAPE_ARC arc;

    return arc.ConstructFromStartEndCenter( startPoint, endPoint, centerPoint, clockwise );
}


void CADSTAR_ARCHIVE_PARSER::CUTOUT::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "CUTOUT" ) );

    Vertices = ParseAllChildVertices( aNode, aContext, true );
}


bool CADSTAR_ARCHIVE_PARSER::SHAPE::IsShape( XNODE* aNode )
{
    wxString aNodeName = aNode->GetName();

    if( aNodeName == wxT( "OPENSHAPE" ) || aNodeName == wxT( "OUTLINE" )
            || aNodeName == wxT( "SOLID" ) || aNodeName == wxT( "HATCHED" ) )
    {
        return true;
    }
    else
    {
        return false;
    }
}


void CADSTAR_ARCHIVE_PARSER::SHAPE::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( IsShape( aNode ) );

    wxString aNodeName = aNode->GetName();

    if( aNodeName == wxT( "OPENSHAPE" ) )
    {
        Type     = SHAPE_TYPE::OPENSHAPE;
        Vertices = ParseAllChildVertices( aNode, aContext, true );
        Cutouts.clear();
        HatchCodeID = wxEmptyString;
    }
    else if( aNodeName == wxT( "OUTLINE" ) )
    {
        Type        = SHAPE_TYPE::OUTLINE;
        Vertices    = ParseAllChildVertices( aNode, aContext, false );
        Cutouts     = ParseAllChildCutouts( aNode, aContext, false );
        HatchCodeID = wxEmptyString;
    }
    else if( aNodeName == wxT( "SOLID" ) )
    {
        Type        = SHAPE_TYPE::SOLID;
        Vertices    = ParseAllChildVertices( aNode, aContext, false );
        Cutouts     = ParseAllChildCutouts( aNode, aContext, false );
        HatchCodeID = wxEmptyString;
    }
    else if( aNodeName == wxT( "HATCHED" ) )
    {
        Type        = SHAPE_TYPE::HATCHED;
        Vertices    = ParseAllChildVertices( aNode, aContext, false );
        Cutouts     = ParseAllChildCutouts( aNode, aContext, false );
        HatchCodeID = GetXmlAttributeIDString( aNode, 0 );
    }
    else
    {
        wxASSERT_MSG( true, wxT( "Unknown SHAPE type" ) );
    }
}

SHAPE_LINE_CHAIN CADSTAR_ARCHIVE_PARSER::SHAPE::OutlineAsChain(
        const std::function<VECTOR2I( const VECTOR2I& )> aCadstarToKicadPointCallback,
        int aMaxError  ) const
{
    SHAPE_LINE_CHAIN outline;

    if( Vertices.size() == 0 )
        return outline;

    for( const auto& vertex : Vertices )
        vertex.AppendToChain( &outline, aCadstarToKicadPointCallback, aMaxError );

    if( Type != SHAPE_TYPE::OPENSHAPE )
    {
        outline.SetClosed( true );

        // Append after closing, to ensre first and last point remain the same
        outline.Append( outline.CPoint( 0 ), true );
    }

    return outline;
}


SHAPE_POLY_SET CADSTAR_ARCHIVE_PARSER::SHAPE::ConvertToPolySet(
        const std::function<VECTOR2I( const VECTOR2I& )> aCadstarToKicadPointCallback,
        int aMaxError ) const
{
    SHAPE_POLY_SET polyset;

    // We shouldn't convert openshapes to polyset!
    wxCHECK( Type != SHAPE_TYPE::OPENSHAPE, polyset );

    polyset.AddOutline( OutlineAsChain( aCadstarToKicadPointCallback, aMaxError ) );

    for( const auto& cutout : Cutouts )
    {
        SHAPE_LINE_CHAIN hole;

        if( cutout.Vertices.size() == 0 )
            continue;

        for( const auto& cutoutVertex : cutout.Vertices )
            cutoutVertex.AppendToChain( &hole, aCadstarToKicadPointCallback, aMaxError );

        hole.SetClosed( true );

        // Append after closing, to ensre first and last point remain the same
        cutout.Vertices.at( 0 ).AppendToChain( &hole, aCadstarToKicadPointCallback, aMaxError );

        polyset.AddHole( hole );
    }

    return polyset;
}


CADSTAR_ARCHIVE_PARSER::UNITS CADSTAR_ARCHIVE_PARSER::ParseUnits( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "UNITS" ) );

    wxString unit = GetXmlAttributeIDString( aNode, 0 );

    if( unit == wxT( "CENTIMETER" ) )
        return UNITS::CENTIMETER;
    else if( unit == wxT( "INCH" ) )
        return UNITS::INCH;
    else if( unit == wxT( "METER" ) )
        return UNITS::METER;
    else if( unit == wxT( "MICROMETRE" ) )
        return UNITS::MICROMETRE;
    else if( unit == wxT( "MM" ) )
        return UNITS::MM;
    else if( unit == wxT( "THOU" ) )
        return UNITS::THOU;
    else if( unit == wxT( "DESIGN" ) )
        return UNITS::DESIGN;
    else
        THROW_UNKNOWN_PARAMETER_IO_ERROR( unit, wxT( "UNITS" ) );

    return UNITS();
}


CADSTAR_ARCHIVE_PARSER::ANGUNITS CADSTAR_ARCHIVE_PARSER::ParseAngunits( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "ANGUNITS" ) );

    wxString angUnitStr = GetXmlAttributeIDString( aNode, 0 );

    if( angUnitStr == wxT( "DEGREES" ) )
        return ANGUNITS::DEGREES;
    else if( angUnitStr == wxT( "RADIANS" ) )
        return ANGUNITS::RADIANS;
    else
        THROW_UNKNOWN_PARAMETER_IO_ERROR( angUnitStr, aNode->GetName() );

    return ANGUNITS();
}


bool CADSTAR_ARCHIVE_PARSER::GRID::IsGrid( XNODE* aNode )
{
    wxString aNodeName = aNode->GetName();

    if( aNodeName == wxT( "FRACTIONALGRID" ) || aNodeName == wxT( "STEPGRID" ) )
        return true;
    else
        return false;
}


void CADSTAR_ARCHIVE_PARSER::GRID::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( IsGrid( aNode ) );

    wxString aNodeName = aNode->GetName();

    if( aNodeName == wxT( "FRACTIONALGRID" ) )
        Type = GRID_TYPE::FRACTIONALGRID;
    else if( aNodeName == wxT( "STEPGRID" ) )
        Type = GRID_TYPE::STEPGRID;
    else
        wxASSERT_MSG( true, wxT( "Unknown Grid Type" ) );

    Name   = GetXmlAttributeIDString( aNode, 0 );
    Param1 = GetXmlAttributeIDLong( aNode, 1 );
    Param2 = GetXmlAttributeIDLong( aNode, 2 );
}


void CADSTAR_ARCHIVE_PARSER::GRIDS::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "GRIDS" ) );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( cNodeName == wxT( "WORKINGGRID" ) )
        {
            XNODE* workingGridNode = cNode->GetChildren();

            if( !GRID::IsGrid( workingGridNode ) )
            {
                THROW_UNKNOWN_NODE_IO_ERROR(
                        workingGridNode->GetName(), wxT( "GRIDS -> WORKINGGRID" ) );
            }
            else
            {
                WorkingGrid.Parse( workingGridNode, aContext );
            }
        }
        else if( cNodeName == wxT( "SCREENGRID" ) )
        {
            XNODE* screenGridNode = cNode->GetChildren();

            if( !GRID::IsGrid( screenGridNode ) )
            {
                THROW_UNKNOWN_NODE_IO_ERROR(
                        screenGridNode->GetName(), wxT( "GRIDS -> SCREENGRID" ) );
            }
            else
            {
                ScreenGrid.Parse( screenGridNode, aContext );
            }
        }
        else if( GRID::IsGrid( cNode ) )
        {
            GRID userGrid;
            userGrid.Parse( cNode, aContext );
            UserGrids.push_back( userGrid );
        }
    }
}


bool CADSTAR_ARCHIVE_PARSER::SETTINGS::ParseSubNode( XNODE* aChildNode, PARSER_CONTEXT* aContext )
{
    wxString cNodeName = aChildNode->GetName();

    if( cNodeName == wxT( "UNITS" ) )
    {
        Units = ParseUnits( aChildNode );
    }
    else if( cNodeName == wxT( "UNITSPRECISION" ) )
    {
        UnitDisplPrecision = GetXmlAttributeIDLong( aChildNode, 0 );
    }
    else if( cNodeName == wxT( "INTERLINEGAP" ) )
    {
        InterlineGap = GetXmlAttributeIDLong( aChildNode, 0 );
    }
    else if( cNodeName == wxT( "BARLINEGAP" ) )
    {
        BarlineGap = GetXmlAttributeIDLong( aChildNode, 0 );
    }
    else if( cNodeName == wxT( "ALLOWBARTEXT" ) )
    {
        AllowBarredText = true;
    }
    else if( cNodeName == wxT( "ANGULARPRECISION" ) )
    {
        AngularPrecision = GetXmlAttributeIDLong( aChildNode, 0 );
    }
    else if( cNodeName == wxT( "DESIGNORIGIN" ) )
    {
        DesignOrigin.Parse( aChildNode->GetChildren(), aContext );
    }
    else if( cNodeName == wxT( "DESIGNAREA" ) )
    {
        std::vector<POINT> pts = ParseAllChildPoints( aChildNode, aContext, true, 2 );
        DesignArea             = std::make_pair( pts[0], pts[1] );
    }
    else if( cNodeName == wxT( "DESIGNREF" ) )
    {
        DesignOrigin.Parse( aChildNode->GetChildren(), aContext );
    }
    else if( cNodeName == wxT( "DESIGNLIMIT" ) )
    {
        DesignLimit.Parse( aChildNode->GetChildren(), aContext );
    }
    else if( cNodeName == wxT( "PINNOOFFSET" ) )
    {
        PinNoOffset = GetXmlAttributeIDLong( aChildNode, 0 );
    }
    else if( cNodeName == wxT( "PINNOANGLE" ) )
    {
        PinNoAngle = GetXmlAttributeIDLong( aChildNode, 0 );
    }
    else
    {
        return false;
    }

    return true;
}


void CADSTAR_ARCHIVE_PARSER::SETTINGS::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "SETTINGS" ) );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( ParseSubNode( cNode, aContext ) )
            continue;
        else
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, wxT( "SETTINGS" ) );
    }
}


wxString CADSTAR_ARCHIVE_PARSER::ParseTextFields( const wxString& aTextString,
                                                  PARSER_CONTEXT* aContext )
{
    static const std::map<TEXT_FIELD_NAME, wxString> txtTokens =
    {
        { TEXT_FIELD_NAME::DESIGN_TITLE,        wxT( "DESIGN TITLE" ) },
        { TEXT_FIELD_NAME::SHORT_JOBNAME,       wxT( "SHORT_JOBNAME" ) },
        { TEXT_FIELD_NAME::LONG_JOBNAME,        wxT( "LONG_JOBNAME" ) },
        { TEXT_FIELD_NAME::NUM_OF_SHEETS,       wxT( "NUM_OF_SHEETS" ) },
        { TEXT_FIELD_NAME::SHEET_NUMBER,        wxT( "SHEET_NUMBER" ) },
        { TEXT_FIELD_NAME::SHEET_NAME,          wxT( "SHEET_NAME" ) },
        { TEXT_FIELD_NAME::VARIANT_NAME,        wxT( "VARIANT_NAME" ) },
        { TEXT_FIELD_NAME::VARIANT_DESCRIPTION, wxT( "VARIANT_DESCRIPTION" ) },
        { TEXT_FIELD_NAME::REG_USER,            wxT( "REG_USER" ) },
        { TEXT_FIELD_NAME::COMPANY_NAME,        wxT( "COMPANY_NAME" ) },
        { TEXT_FIELD_NAME::CURRENT_USER,        wxT( "CURRENT_USER" ) },
        { TEXT_FIELD_NAME::DATE,                wxT( "DATE" ) },
        { TEXT_FIELD_NAME::TIME,                wxT( "TIME" ) },
        { TEXT_FIELD_NAME::MACHINE_NAME,        wxT( "MACHINE_NAME" ) },
        { TEXT_FIELD_NAME::FROM_FILE,           wxT( "FROM_FILE" ) },
        { TEXT_FIELD_NAME::DISTANCE,            wxT( "DISTANCE" ) },
        { TEXT_FIELD_NAME::UNITS_SHORT,         wxT( "UNITS SHORT" ) },
        { TEXT_FIELD_NAME::UNITS_ABBREV,        wxT( "UNITS ABBREV" ) },
        { TEXT_FIELD_NAME::UNITS_FULL,          wxT( "UNITS FULL" ) },
        { TEXT_FIELD_NAME::HYPERLINK,           wxT( "HYPERLINK" ) }
    };

    wxString remainingStr = aTextString;
    wxString returnStr;

    while( remainingStr.size() > 0 )
    {
        // Find the start token
        size_t startpos = remainingStr.Find( wxT( "<@" ) );

        if( static_cast<int>( startpos ) == wxNOT_FOUND )
        {
            // No more fields to parse, add to return string
            returnStr += remainingStr;
            break;
        }

        if( startpos > 0 )
            returnStr += remainingStr.SubString( 0, startpos - 1 );

        if( ( startpos + 2 ) >= remainingStr.size() )
            break;

        remainingStr = remainingStr.Mid( startpos + 2 );

        //Find the expected token for the field
        TEXT_FIELD_NAME foundField = TEXT_FIELD_NAME::NONE;

        for( std::pair<TEXT_FIELD_NAME, wxString> txtF : txtTokens )
        {
            if( remainingStr.StartsWith( txtF.second ) )
            {
                foundField = txtF.first;
                break;
            }
        }

        if( foundField == TEXT_FIELD_NAME::NONE )
        {
            // Not a valid field, lets keep looking
            returnStr += wxT( "<@" );
            continue;
        }

        //Now lets find the end token
        size_t endpos = remainingStr.Find( wxT( "@>" ) );

        if( static_cast<int>( endpos ) == wxNOT_FOUND )
        {
            // The field we found isn't valid as it doesn't have a termination
            // Lets append the whole thing as plain text
            returnStr += wxT( "<@" ) + remainingStr;
            break;
        }

        size_t   valueStart = txtTokens.at( foundField ).size();
        wxString fieldValue = remainingStr.SubString( valueStart, endpos - 1 );
        wxString address;

        if( foundField == TEXT_FIELD_NAME::FROM_FILE || foundField == TEXT_FIELD_NAME::HYPERLINK )
        {
            // The first character should always be a double quotation mark
            wxASSERT_MSG( fieldValue.at( 0 ) == '"', "Expected '\"' as the first character" );

            size_t splitPos = fieldValue.find_first_of( '"', 1 );
            address         = fieldValue.SubString( 1, splitPos - 1 );

            if( foundField == TEXT_FIELD_NAME::HYPERLINK )
            {
                // Assume the last two characters are "@>"
                wxASSERT_MSG( remainingStr.EndsWith( wxT( "@>" ) ),
                              "Expected '@>' at the end of a hyperlink" );

                fieldValue = remainingStr.SubString( valueStart + splitPos + 1,
                                                     remainingStr.Length() - 3 );

                remainingStr = wxEmptyString;
            }
            else
            {
                fieldValue = fieldValue.Mid( splitPos + 1 );
            }
        }

        switch( foundField )
        {
        case TEXT_FIELD_NAME::DESIGN_TITLE:
        case TEXT_FIELD_NAME::SHORT_JOBNAME:
        case TEXT_FIELD_NAME::LONG_JOBNAME:
        case TEXT_FIELD_NAME::VARIANT_NAME:
        case TEXT_FIELD_NAME::VARIANT_DESCRIPTION:
        case TEXT_FIELD_NAME::REG_USER:
        case TEXT_FIELD_NAME::COMPANY_NAME:
        case TEXT_FIELD_NAME::CURRENT_USER:
        case TEXT_FIELD_NAME::DATE:
        case TEXT_FIELD_NAME::TIME:
        case TEXT_FIELD_NAME::MACHINE_NAME:

            if( aContext->TextFieldToValuesMap.find( foundField )
                    != aContext->TextFieldToValuesMap.end() )
            {
                aContext->InconsistentTextFields.insert( foundField );
            }
            else
            {
                aContext->TextFieldToValuesMap.insert( { foundField, fieldValue } );
            }

            KI_FALLTHROUGH;

        case TEXT_FIELD_NAME::NUM_OF_SHEETS:
        case TEXT_FIELD_NAME::SHEET_NUMBER:
        case TEXT_FIELD_NAME::SHEET_NAME:
            returnStr += wxT( "${" ) + CADSTAR_TO_KICAD_FIELDS.at( foundField ) + wxT( "}" );
            break;

        case TEXT_FIELD_NAME::DISTANCE:
        case TEXT_FIELD_NAME::UNITS_SHORT:
        case TEXT_FIELD_NAME::UNITS_ABBREV:
        case TEXT_FIELD_NAME::UNITS_FULL:
            // Just flatten the text for distances
            returnStr += fieldValue;
            break;

        case TEXT_FIELD_NAME::FROM_FILE:
        {
            wxFileName fn( address );
            wxString   fieldFmt = wxT( "FROM_FILE_%s_%s" );
            wxString   fieldName = wxString::Format( fieldFmt, fn.GetName(), fn.GetExt() );

            int version = 1;

            while(
                aContext->FilenamesToTextMap.find( fieldName )
                            != aContext->FilenamesToTextMap.end()
                    && aContext->FilenamesToTextMap.at( fieldName ) != fieldValue )
            {
                fieldName = wxString::Format( fieldFmt, fn.GetName(), fn.GetExt() )
                            + wxString::Format( wxT( "_%d" ), version++ );
            }

            aContext->FilenamesToTextMap[fieldName] = fieldValue;
            returnStr += wxT( "${" ) + fieldName + wxT( "}" );
        }
        break;

        case TEXT_FIELD_NAME::HYPERLINK:
        {
            aContext->TextToHyperlinksMap[fieldValue] = address;
            returnStr += ParseTextFields( fieldValue, aContext );
        }
        break;

        case TEXT_FIELD_NAME::NONE:
            wxFAIL_MSG( "We should have already covered this scenario above" );
            break;
        }


        if( ( endpos + 2 ) >= remainingStr.size() )
            break;

        remainingStr = remainingStr.Mid( endpos + 2 );
    }

    return returnStr;
}


CADSTAR_ARCHIVE_PARSER::ALIGNMENT CADSTAR_ARCHIVE_PARSER::ParseAlignment( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "ALIGN" ) );

    wxString alignmentStr = GetXmlAttributeIDString( aNode, 0 );

    if( alignmentStr == wxT( "BOTTOMCENTER" ) )
        return ALIGNMENT::BOTTOMCENTER;
    else if( alignmentStr == wxT( "BOTTOMLEFT" ) )
        return ALIGNMENT::BOTTOMLEFT;
    else if( alignmentStr == wxT( "BOTTOMRIGHT" ) )
        return ALIGNMENT::BOTTOMRIGHT;
    else if( alignmentStr == wxT( "CENTERCENTER" ) )
        return ALIGNMENT::CENTERCENTER;
    else if( alignmentStr == wxT( "CENTERLEFT" ) )
        return ALIGNMENT::CENTERLEFT;
    else if( alignmentStr == wxT( "CENTERRIGHT" ) )
        return ALIGNMENT::CENTERRIGHT;
    else if( alignmentStr == wxT( "TOPCENTER" ) )
        return ALIGNMENT::TOPCENTER;
    else if( alignmentStr == wxT( "TOPLEFT" ) )
        return ALIGNMENT::TOPLEFT;
    else if( alignmentStr == wxT( "TOPRIGHT" ) )
        return ALIGNMENT::TOPRIGHT;
    else
        THROW_UNKNOWN_PARAMETER_IO_ERROR( alignmentStr, wxT( "ALIGN" ) );

    //shouldn't be here but avoids compiler warning
    return ALIGNMENT::NO_ALIGNMENT;
}


CADSTAR_ARCHIVE_PARSER::JUSTIFICATION CADSTAR_ARCHIVE_PARSER::ParseJustification( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "JUSTIFICATION" ) );

    wxString justificationStr = GetXmlAttributeIDString( aNode, 0 );

    if( justificationStr == wxT( "LEFT" ) )
        return JUSTIFICATION::LEFT;
    else if( justificationStr == wxT( "RIGHT" ) )
        return JUSTIFICATION::RIGHT;
    else if( justificationStr == wxT( "CENTER" ) )
        return JUSTIFICATION::CENTER;
    else
        THROW_UNKNOWN_PARAMETER_IO_ERROR( justificationStr, wxT( "JUSTIFICATION" ) );

    return JUSTIFICATION::LEFT;
}


CADSTAR_ARCHIVE_PARSER::READABILITY CADSTAR_ARCHIVE_PARSER::ParseReadability( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "READABILITY" ) );

    wxString readabilityStr = GetXmlAttributeIDString( aNode, 0 );

    if( readabilityStr == wxT( "BOTTOM_TO_TOP" ) )
        return READABILITY::BOTTOM_TO_TOP;
    else if( readabilityStr == wxT( "TOP_TO_BOTTOM" ) )
        return READABILITY::TOP_TO_BOTTOM;
    else
        THROW_UNKNOWN_PARAMETER_IO_ERROR( readabilityStr, wxT( "READABILITY" ) );

    return READABILITY::BOTTOM_TO_TOP;
}


void CADSTAR_ARCHIVE_PARSER::ATTRIBUTE_LOCATION::ParseIdentifiers( XNODE* aNode,
                                                                   PARSER_CONTEXT* aContext )
{
    TextCodeID = GetXmlAttributeIDString( aNode, 0 );
    LayerID    = GetXmlAttributeIDString( aNode, 1 );
}


bool CADSTAR_ARCHIVE_PARSER::ATTRIBUTE_LOCATION::ParseSubNode( XNODE* aChildNode,
                                                               PARSER_CONTEXT* aContext )
{
    wxString cNodeName = aChildNode->GetName();

    if( cNodeName == wxT( "PT" ) )
        Position.Parse( aChildNode, aContext );
    else if( cNodeName == wxT( "ORIENT" ) )
        OrientAngle = GetXmlAttributeIDLong( aChildNode, 0 );
    else if( cNodeName == wxT( "MIRROR" ) )
        Mirror = true;
    else if( cNodeName == wxT( "FIX" ) )
        Fixed = true;
    else if( cNodeName == wxT( "ALIGN" ) )
        Alignment = ParseAlignment( aChildNode );
    else if( cNodeName == wxT( "JUSTIFICATION" ) )
        Justification = ParseJustification( aChildNode );
    else
        return false;

    return true;
}


void CADSTAR_ARCHIVE_PARSER::ATTRIBUTE_LOCATION::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "ATTRLOC" ) );

    ParseIdentifiers( aNode, aContext );

    //Parse child nodes
    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        if( ParseSubNode( cNode, aContext ) )
            continue;
        else
            THROW_UNKNOWN_NODE_IO_ERROR( cNode->GetName(), wxT( "ATTRLOC" ) );
    }

    if( Position.x == UNDEFINED_VALUE || Position.y == UNDEFINED_VALUE )
        THROW_MISSING_NODE_IO_ERROR( wxT( "PT" ), wxT( "ATTRLOC" ) );
}


void CADSTAR_ARCHIVE_PARSER::ATTRNAME::COLUMNORDER::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "COLUMNORDER" ) );

    ID    = GetXmlAttributeIDLong( aNode, 0 );
    Order = GetXmlAttributeIDLong( aNode, 1 );

    CheckNoChildNodes( aNode );
}


void CADSTAR_ARCHIVE_PARSER::ATTRNAME::COLUMNWIDTH::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "COLUMNWIDTH" ) );

    ID    = GetXmlAttributeIDLong( aNode, 0 );
    Width = GetXmlAttributeIDLong( aNode, 1 );

    CheckNoChildNodes( aNode );
}


void CADSTAR_ARCHIVE_PARSER::ATTRNAME::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "ATTRNAME" ) );

    ID   = GetXmlAttributeIDString( aNode, 0 );
    Name = GetXmlAttributeIDString( aNode, 1 );

    XNODE*   cNode    = aNode->GetChildren();
    wxString location = wxString::Format( "ATTRNAME -> %s", Name );

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( cNodeName == wxT( "ATTROWNER" ) )
        {
            wxString attOwnerVal = GetXmlAttributeIDString( cNode, 0 );

            if( attOwnerVal == wxT( "ALL_ITEMS" ) )
                AttributeOwner = ATTROWNER::ALL_ITEMS;
            else if( attOwnerVal == wxT( "AREA" ) )
                AttributeOwner = ATTROWNER::AREA;
            else if( attOwnerVal == wxT( "BOARD" ) )
                AttributeOwner = ATTROWNER::BOARD;
            else if( attOwnerVal == wxT( "COMPONENT" ) )
                AttributeOwner = ATTROWNER::COMPONENT;
            else if( attOwnerVal == wxT( "CONNECTION" ) )
                AttributeOwner = ATTROWNER::CONNECTION;
            else if( attOwnerVal == wxT( "COPPER" ) )
                AttributeOwner = ATTROWNER::COPPER;
            else if( attOwnerVal == wxT( "DOCSYMBOL" ) )
                AttributeOwner = ATTROWNER::DOCSYMBOL;
            else if( attOwnerVal == wxT( "FIGURE" ) )
                AttributeOwner = ATTROWNER::FIGURE;
            else if( attOwnerVal == wxT( "NET" ) )
                AttributeOwner = ATTROWNER::NET;
            else if( attOwnerVal == wxT( "NETCLASS" ) )
                AttributeOwner = ATTROWNER::NETCLASS;
            else if( attOwnerVal == wxT( "PART" ) )
                AttributeOwner = ATTROWNER::PART;
            else if( attOwnerVal == wxT( "PART_DEFINITION" ) )
                AttributeOwner = ATTROWNER::PART_DEFINITION;
            else if( attOwnerVal == wxT( "PIN" ) )
                AttributeOwner = ATTROWNER::PIN;
            else if( attOwnerVal == wxT( "SIGNALREF" ) )
                AttributeOwner = ATTROWNER::SIGNALREF;
            else if( attOwnerVal == wxT( "SYMBOL" ) )
                AttributeOwner = ATTROWNER::SYMBOL;
            else if( attOwnerVal == wxT( "SYMDEF" ) )
                AttributeOwner = ATTROWNER::SYMDEF;
            else if( attOwnerVal == wxT( "TEMPLATE" ) )
                AttributeOwner = ATTROWNER::TEMPLATE;
            else if( attOwnerVal == wxT( "TESTPOINT" ) )
                AttributeOwner = ATTROWNER::TESTPOINT;
            else
                THROW_UNKNOWN_PARAMETER_IO_ERROR( attOwnerVal, location );
        }
        else if( cNodeName == wxT( "ATTRUSAGE" ) )
        {
            wxString attUsageVal = GetXmlAttributeIDString( cNode, 0 );

            if( attUsageVal == wxT( "BOTH" ) )
                AttributeUsage = ATTRUSAGE::BOTH;
            else if( attUsageVal == wxT( "COMPONENT" ) )
                AttributeUsage = ATTRUSAGE::COMPONENT;
            else if( attUsageVal == wxT( "PART_DEFINITION" ) )
                AttributeUsage = ATTRUSAGE::PART_DEFINITION;
            else if( attUsageVal == wxT( "PART_LIBRARY" ) )
                AttributeUsage = ATTRUSAGE::PART_LIBRARY;
            else if( attUsageVal == wxT( "SYMBOL" ) )
                AttributeUsage = ATTRUSAGE::SYMBOL;
            else
                THROW_UNKNOWN_PARAMETER_IO_ERROR( attUsageVal, location );
        }
        else if( cNodeName == wxT( "NOTRANSFER" ) )
        {
            NoTransfer = true;
        }
        else if( cNodeName == wxT( "COLUMNORDER" ) )
        {
            COLUMNORDER cOrder;
            cOrder.Parse( cNode, aContext );
            ColumnOrders.push_back( cOrder );
        }
        else if( cNodeName == wxT( "COLUMNWIDTH" ) )
        {
            COLUMNWIDTH cWidth;
            cWidth.Parse( cNode, aContext );
            ColumnWidths.push_back( cWidth );
        }
        else if( cNodeName == wxT( "COLUMNINVISIBLE" ) )
        {
            ColumnInvisible = true;
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, location );
        }
    }
}


void CADSTAR_ARCHIVE_PARSER::ATTRIBUTE_VALUE::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "ATTR" ) );

    AttributeID = GetXmlAttributeIDString( aNode, 0 );
    Value       = GetXmlAttributeIDString( aNode, 1 );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        if( cNode->GetName() == wxT( "READONLY" ) )
        {
            ReadOnly = true;
        }
        else if( cNode->GetName() == wxT( "ATTRLOC" ) )
        {
            AttributeLocation.Parse( cNode, aContext );
            HasLocation = true;
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNode->GetName(), wxT( "ATTR" ) );
        }
    }
}


void CADSTAR_ARCHIVE_PARSER::TEXT_LOCATION::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "TEXTLOC" ) );

    wxString attributeStr     = GetXmlAttributeIDString( aNode, 0 );
    bool     attributeIDisSet = false;

    if( attributeStr == wxT( "PART_NAME" ) )
    {
        AttributeID      = PART_NAME_ATTRID;
        attributeIDisSet = true;
    }
    else if( attributeStr == wxT( "COMP_NAME" ) )
    {
        AttributeID      = COMPONENT_NAME_ATTRID;
        attributeIDisSet = true;
    }
    else if( attributeStr == wxT( "COMP_NAME2" ) )
    {
        AttributeID      = COMPONENT_NAME_2_ATTRID;
        attributeIDisSet = true;
    }
    else if( attributeStr == wxT( "SYMBOL_NAME" ) )
    {
        AttributeID      = SYMBOL_NAME_ATTRID;
        attributeIDisSet = true;
    }
    else if( attributeStr == wxT( "LINK_ORIGIN" ) )
    {
        AttributeID      = LINK_ORIGIN_ATTRID;
        attributeIDisSet = true;
    }
    else if( attributeStr == wxT( "SIGNALNAME_ORIGIN" ) )
    {
        AttributeID      = SIGNALNAME_ORIGIN_ATTRID;
        attributeIDisSet = true;
    }
    else if( attributeStr == wxT( "ATTRREF" ) )
    {
        //We will initialise when we parse all child nodes
        attributeIDisSet = false;
    }
    else
    {
        THROW_UNKNOWN_PARAMETER_IO_ERROR( attributeStr, wxT( "TEXTLOC" ) );
    }

    TextCodeID = GetXmlAttributeIDString( aNode, 1 );
    LayerID    = GetXmlAttributeIDString( aNode, 2, false );

    //Parse child nodes
    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( ParseSubNode( cNode, aContext ) )
        {
            continue;
        }
        else if( !attributeIDisSet && cNodeName == wxT( "ATTRREF" ) )
        {
            AttributeID      = GetXmlAttributeIDString( cNode, 0 );
            attributeIDisSet = true;
        }
        else if( cNodeName == wxT( "ORIENT" ) )
        {
            OrientAngle = GetXmlAttributeIDLong( cNode, 0 );
        }
        else if( cNodeName == wxT( "MIRROR" ) )
        {
            Mirror = true;
        }
        else if( cNodeName == wxT( "FIX" ) )
        {
            Fixed = true;
        }
        else if( cNodeName == wxT( "ALIGN" ) )
        {
            Alignment = ParseAlignment( cNode );
        }
        else if( cNodeName == wxT( "JUSTIFICATION" ) )
        {
            Justification = ParseJustification( cNode );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, wxT( "TEXTLOC" ) );
        }
    }

    if( Position.x == UNDEFINED_VALUE || Position.y == UNDEFINED_VALUE )
        THROW_MISSING_NODE_IO_ERROR( wxT( "PT" ), wxT( "TEXTLOC" ) );
}


void CADSTAR_ARCHIVE_PARSER::CADSTAR_NETCLASS::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "NETCLASS" ) );

    ID   = GetXmlAttributeIDString( aNode, 0 );
    Name = GetXmlAttributeIDString( aNode, 1 );

    XNODE*   cNode    = aNode->GetChildren();
    wxString location = wxString::Format( "NETCLASS -> %s", Name );

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( cNodeName == wxT( "ATTR" ) )
        {
            ATTRIBUTE_VALUE attribute_val;
            attribute_val.Parse( cNode, aContext );
            Attributes.push_back( attribute_val );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, location );
        }
    }
}


void CADSTAR_ARCHIVE_PARSER::SPCCLASSNAME::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "SPCCLASSNAME" ) );

    ID   = GetXmlAttributeIDString( aNode, 0 );
    Name = GetXmlAttributeIDString( aNode, 1 );
}


bool CADSTAR_ARCHIVE_PARSER::CODEDEFS::ParseSubNode( XNODE* aChildNode, PARSER_CONTEXT* aContext )
{
    wxString nodeName = aChildNode->GetName();

    if( nodeName == wxT( "LINECODE" ) )
    {
        LINECODE linecode;
        linecode.Parse( aChildNode, aContext );
        LineCodes.insert( std::make_pair( linecode.ID, linecode ) );
    }
    else if( nodeName == wxT( "HATCHCODE" ) )
    {
        HATCHCODE hatchcode;
        hatchcode.Parse( aChildNode, aContext );
        HatchCodes.insert( std::make_pair( hatchcode.ID, hatchcode ) );
    }
    else if( nodeName == wxT( "TEXTCODE" ) )
    {
        TEXTCODE textcode;
        textcode.Parse( aChildNode, aContext );
        TextCodes.insert( std::make_pair( textcode.ID, textcode ) );
    }
    else if( nodeName == wxT( "ROUTECODE" ) )
    {
        ROUTECODE routecode;
        routecode.Parse( aChildNode, aContext );
        RouteCodes.insert( std::make_pair( routecode.ID, routecode ) );
    }
    else if( nodeName == wxT( "ATTRNAME" ) )
    {
        ATTRNAME attrname;
        attrname.Parse( aChildNode, aContext );
        AttributeNames.insert( std::make_pair( attrname.ID, attrname ) );
    }
    else if( nodeName == wxT( "NETCLASS" ) )
    {
        CADSTAR_NETCLASS netclass;
        netclass.Parse( aChildNode, aContext );
        NetClasses.insert( std::make_pair( netclass.ID, netclass ) );
    }
    else if( nodeName == wxT( "SPCCLASSNAME" ) )
    {
        SPCCLASSNAME spcclassname;
        spcclassname.Parse( aChildNode, aContext );
        SpacingClassNames.insert( std::make_pair( spcclassname.ID, spcclassname ) );
    }
    else
    {
        return false;
    }

    return true;
}


CADSTAR_ARCHIVE_PARSER::SWAP_RULE CADSTAR_ARCHIVE_PARSER::ParseSwapRule( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "SWAPRULE" ) );

    SWAP_RULE retval;
    wxString  swapRuleStr = GetXmlAttributeIDString( aNode, 0 );

    if( swapRuleStr == wxT( "NO_SWAP" ) )
        retval = SWAP_RULE::NO_SWAP;
    else if( swapRuleStr == wxT( "USE_SWAP_LAYER" ) )
        retval = SWAP_RULE::USE_SWAP_LAYER;
    else
        THROW_UNKNOWN_PARAMETER_IO_ERROR( swapRuleStr, wxT( "SWAPRULE" ) );

    return retval;
}


void CADSTAR_ARCHIVE_PARSER::REUSEBLOCK::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "REUSEBLOCK" ) );

    ID       = GetXmlAttributeIDString( aNode, 0 );
    Name     = GetXmlAttributeIDString( aNode, 1 );
    FileName = GetXmlAttributeIDString( aNode, 2 );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( cNodeName == wxT( "MIRROR" ) )
            Mirror = true;
        else if( cNodeName == wxT( "ORIENT" ) )
            OrientAngle = GetXmlAttributeIDLong( cNode, 0 );
        else
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, wxT( "REUSEBLOCK" ) );
    }
}


bool CADSTAR_ARCHIVE_PARSER::REUSEBLOCKREF::IsEmpty()
{
    return ReuseBlockID == wxEmptyString && ItemReference == wxEmptyString;
}


void CADSTAR_ARCHIVE_PARSER::REUSEBLOCKREF::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "REUSEBLOCKREF" ) );

    ReuseBlockID  = GetXmlAttributeIDString( aNode, 0 );
    ItemReference = GetXmlAttributeIDString( aNode, 1 );

    CheckNoChildNodes( aNode );
}


void CADSTAR_ARCHIVE_PARSER::GROUP::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "GROUP" ) );

    ID   = GetXmlAttributeIDString( aNode, 0 );
    Name = GetXmlAttributeIDString( aNode, 1 );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( cNodeName == wxT( "FIX" ) )
            Fixed = true;
        else if( cNodeName == wxT( "TRANSFER" ) )
            Transfer = true;
        else if( cNodeName == wxT( "GROUPREF" ) )
            GroupID = GetXmlAttributeIDString( cNode, 0 );
        else if( cNodeName == wxT( "REUSEBLOCKREF" ) )
            ReuseBlockRef.Parse( cNode, aContext );
        else
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, wxT( "GROUP" ) );
    }
}


void CADSTAR_ARCHIVE_PARSER::FIGURE::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "FIGURE" ) );

    ID         = GetXmlAttributeIDString( aNode, 0 );
    LineCodeID = GetXmlAttributeIDString( aNode, 1 );
    LayerID    = GetXmlAttributeIDString( aNode, 2 );

    XNODE*   cNode              = aNode->GetChildren();
    bool     shapeIsInitialised = false; // Stop more than one Shape Object
    wxString location           = wxString::Format( "Figure %s", ID );

    if( !cNode )
        THROW_MISSING_NODE_IO_ERROR( wxT( "Shape" ), location );

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( !shapeIsInitialised && Shape.IsShape( cNode ) )
        {
            Shape.Parse( cNode, aContext );
            shapeIsInitialised = true;
        }
        else if( cNodeName == wxT( "SWAPRULE" ) )
        {
            SwapRule = ParseSwapRule( cNode );
        }
        else if( cNodeName == wxT( "FIX" ) )
        {
            Fixed = true;
        }
        else if( cNodeName == wxT( "GROUPREF" ) )
        {

            GroupID = GetXmlAttributeIDString( cNode, 0 );
        }
        else if( cNodeName == wxT( "REUSEBLOCKREF" ) )
        {
            ReuseBlockRef.Parse( cNode, aContext );
        }
        else if( cNodeName == wxT( "ATTR" ) )
        {
            ATTRIBUTE_VALUE attr;
            attr.Parse( cNode, aContext );
            AttributeValues.insert( std::make_pair( attr.AttributeID, attr ) );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, location );
        }
    }
}


void CADSTAR_ARCHIVE_PARSER::TEXT::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    Parse( aNode, aContext, true );
}


void CADSTAR_ARCHIVE_PARSER::TEXT::Parse( XNODE* aNode, PARSER_CONTEXT* aContext,
                                          bool aParseFields )
{
    wxASSERT( aNode->GetName() == wxT( "TEXT" ) );

    ID = GetXmlAttributeIDString( aNode, 0 );
    Text = GetXmlAttributeIDString( aNode, 1 );

    if( aParseFields )
        Text = ParseTextFields( Text, aContext );

    TextCodeID = GetXmlAttributeIDString( aNode, 2 );
    LayerID    = GetXmlAttributeIDString( aNode, 3 );

    XNODE* cNode = aNode->GetChildren();

    if( !cNode )
        THROW_MISSING_NODE_IO_ERROR( wxT( "PT" ), wxT( "TEXT" ) );

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( cNodeName == wxT( "PT" ) )
            Position.Parse( cNode, aContext );
        else if( cNodeName == wxT( "ORIENT" ) )
            OrientAngle = GetXmlAttributeIDLong( cNode, 0 );
        else if( cNodeName == wxT( "MIRROR" ) )
            Mirror = true;
        else if( cNodeName == wxT( "FIX" ) )
            Fixed = true;
        else if( cNodeName == wxT( "SWAPRULE" ) )
            SwapRule = ParseSwapRule( cNode );
        else if( cNodeName == wxT( "ALIGN" ) )
            Alignment = ParseAlignment( cNode );
        else if( cNodeName == wxT( "JUSTIFICATION" ) )
            Justification = ParseJustification( cNode );
        else if( cNodeName == wxT( "GROUPREF" ) )
            GroupID = GetXmlAttributeIDString( cNode, 0 );
        else if( cNodeName == wxT( "REUSEBLOCKREF" ) )
            ReuseBlockRef.Parse( cNode, aContext );
        else
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, wxT( "TEXT" ) );
    }
}


wxString CADSTAR_ARCHIVE_PARSER::SYMDEF::BuildLibName() const
{
    return generateLibName( ReferenceName, Alternate );
}


void CADSTAR_ARCHIVE_PARSER::SYMDEF::ParseIdentifiers( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "SYMDEF" ) );

    ID            = GetXmlAttributeIDString( aNode, 0 );
    ReferenceName = GetXmlAttributeIDString( aNode, 1 );
    Alternate     = GetXmlAttributeIDString( aNode, 2 );
}


bool CADSTAR_ARCHIVE_PARSER::SYMDEF::ParseSubNode( XNODE* aChildNode, PARSER_CONTEXT* aContext )
{
    wxString cNodeName = aChildNode->GetName();

    if( cNodeName == wxT( "PT" ) )
    {
        Origin.Parse( aChildNode, aContext );
    }
    else if( cNodeName == wxT( "STUB" ) )
    {
        Stub = true;
    }
    else if( cNodeName == wxT( "VERSION" ) )
    {
        Version = GetXmlAttributeIDLong( aChildNode, 0 );
    }
    else if( cNodeName == wxT( "FIGURE" ) )
    {
        FIGURE figure;
        figure.Parse( aChildNode, aContext );
        Figures.insert( std::make_pair( figure.ID, figure ) );
    }
    else if( cNodeName == wxT( "TEXT" ) )
    {
        TEXT txt;
        txt.Parse( aChildNode, aContext );
        Texts.insert( std::make_pair( txt.ID, txt ) );
    }
    else if( cNodeName == wxT( "TEXTLOC" ) )
    {
        TEXT_LOCATION textloc;
        textloc.Parse( aChildNode, aContext );
        TextLocations.insert( std::make_pair( textloc.AttributeID, textloc ) );
    }
    else if( cNodeName == wxT( "ATTR" ) )
    {
        ATTRIBUTE_VALUE attrVal;
        attrVal.Parse( aChildNode, aContext );
        AttributeValues.insert( std::make_pair( attrVal.AttributeID, attrVal ) );
    }
    else
    {
        return false;
    }

    return true;
}


void CADSTAR_ARCHIVE_PARSER::PART::DEFINITION::GATE::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "GATEDEFINITION" ) );

    ID        = GetXmlAttributeIDString( aNode, 0 );
    Name      = GetXmlAttributeIDString( aNode, 1 );
    Alternate = GetXmlAttributeIDString( aNode, 2 );
    PinCount  = GetXmlAttributeIDLong( aNode, 3 );

    CheckNoChildNodes( aNode );
}


CADSTAR_PIN_TYPE CADSTAR_ARCHIVE_PARSER::PART::GetPinType( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "PINTYPE" ) );

    wxString pinTypeStr = GetXmlAttributeIDString( aNode, 0 );

    std::map<wxString, CADSTAR_PIN_TYPE> pinTypeMap = {
        { wxT( "INPUT" ),               CADSTAR_PIN_TYPE::PIN_INPUT },
        { wxT( "OUTPUT_OR" ),           CADSTAR_PIN_TYPE::OUTPUT_OR },
        { wxT( "OUTPUT_NOT_OR" ),       CADSTAR_PIN_TYPE::OUTPUT_NOT_OR },
        { wxT( "OUTPUT_NOT_NORM_OR" ),  CADSTAR_PIN_TYPE::OUTPUT_NOT_NORM_OR },
        { wxT( "POWER" ),               CADSTAR_PIN_TYPE::POWER },
        { wxT( "GROUND" ),              CADSTAR_PIN_TYPE::GROUND },
        { wxT( "TRISTATE_BIDIR" ),      CADSTAR_PIN_TYPE::TRISTATE_BIDIR },
        { wxT( "TRISTATE_INPUT" ),      CADSTAR_PIN_TYPE::TRISTATE_INPUT },
        { wxT( "TRISTATE_DRIVER" ),     CADSTAR_PIN_TYPE::TRISTATE_DRIVER } };

    if( pinTypeMap.find( pinTypeStr ) == pinTypeMap.end() )
        THROW_UNKNOWN_PARAMETER_IO_ERROR( pinTypeStr, aNode->GetName() );

    return pinTypeMap[pinTypeStr];
}


void CADSTAR_ARCHIVE_PARSER::PART::DEFINITION::PIN::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "PARTDEFINITIONPIN" ) );

    ID = GetXmlAttributeIDLong( aNode, 0 );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( cNodeName == wxT( "PINNAME" ) )
        {
            Name = GetXmlAttributeIDString( cNode, 0 );
        }
        else if( cNodeName == wxT( "PINLABEL" ) )
        {
            Label = GetXmlAttributeIDString( cNode, 0 );
        }
        else if( cNodeName == wxT( "PINSIGNAL" ) )
        {
            Signal = GetXmlAttributeIDString( cNode, 0 );
        }
        else if( cNodeName == wxT( "PINTERM" ) )
        {
            TerminalGate = GetXmlAttributeIDString( cNode, 0 );
            TerminalPin  = GetXmlAttributeIDLong( cNode, 1 );
        }
        else if( cNodeName == wxT( "PINTYPE" ) )
        {
            Type = GetPinType( cNode );
        }
        else if( cNodeName == wxT( "PINLOAD" ) )
        {
            Load = GetXmlAttributeIDLong( cNode, 0 );
        }
        else if( cNodeName == wxT( "PINPOSITION" ) )
        {
            Position = CADSTAR_PIN_POSITION( GetXmlAttributeIDLong( cNode, 0 ) );
        }
        else if( cNodeName == wxT( "PINIDENTIFIER" ) )
        {
            Identifier = GetXmlAttributeIDString( cNode, 0 );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, aNode->GetName() );
        }
    }
}


void CADSTAR_ARCHIVE_PARSER::PART::PART_PIN::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "PARTPIN" ) );

    ID = GetXmlAttributeIDLong( aNode, 0 );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( cNodeName == wxT( "PINNAME" ) )
            Name = GetXmlAttributeIDString( cNode, 0 );
        else if( cNodeName == wxT( "PINTYPE" ) )
            Type = GetPinType( cNode );
        else if( cNodeName == wxT( "PINIDENTIFIER" ) )
            Identifier = GetXmlAttributeIDString( cNode, 0 );
        else
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, aNode->GetName() );
    }
}


void CADSTAR_ARCHIVE_PARSER::PART::DEFINITION::PIN_EQUIVALENCE::Parse( XNODE* aNode,
                                                                       PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "PINEQUIVALENCE" ) );

    wxXmlAttribute* xmlAttribute = aNode->GetAttributes();

    for( ; xmlAttribute; xmlAttribute = xmlAttribute->GetNext() )
    {
        if( !IsValidAttribute( xmlAttribute ) )
            continue;

        long pinId;

        if( !xmlAttribute->GetValue().ToLong( &pinId ) )
            THROW_UNKNOWN_PARAMETER_IO_ERROR( xmlAttribute->GetValue(), aNode->GetName() );

        PinIDs.push_back( (PART_DEFINITION_PIN_ID) pinId );
    }

    CheckNoChildNodes( aNode );
}


void CADSTAR_ARCHIVE_PARSER::PART::DEFINITION::SWAP_GATE::Parse( XNODE* aNode,
                                                                 PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "SWAPGATE" ) );

    wxXmlAttribute* xmlAttribute = aNode->GetAttributes();

    for( ; xmlAttribute; xmlAttribute = xmlAttribute->GetNext() )
    {
        if( !IsValidAttribute( xmlAttribute ) )
            continue;

        long pinId;

        if( !xmlAttribute->GetValue().ToLong( &pinId ) )
            THROW_UNKNOWN_PARAMETER_IO_ERROR( xmlAttribute->GetValue(), aNode->GetName() );

        PinIDs.push_back( (PART_DEFINITION_PIN_ID) pinId );
    }

    CheckNoChildNodes( aNode );
}


void CADSTAR_ARCHIVE_PARSER::PART::DEFINITION::SWAP_GROUP::Parse( XNODE* aNode,
                                                                  PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "SWAPGROUP" ) );

    GateName = GetXmlAttributeIDString( aNode, 0 );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( cNodeName == wxT( "EXTERNAL" ) )
        {
            External = true;
        }
        else if( cNodeName == wxT( "SWAPGATE" ) )
        {
            SWAP_GATE swapGate;
            swapGate.Parse( cNode, aContext );
            SwapGates.push_back( swapGate );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, aNode->GetName() );
        }
    }
}


void CADSTAR_ARCHIVE_PARSER::PART::DEFINITION::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "PARTDEFINITION" ) );

    Name = GetXmlAttributeIDString( aNode, 0 );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( cNodeName == wxT( "HIDEPINNAMES" ) )
        {
            HidePinNames = true;
        }
        else if( cNodeName == wxT( "MAXPIN" ) )
        {
            MaxPinCount = GetXmlAttributeIDLong( cNode, 0 );
        }
        else if( cNodeName == wxT( "GATEDEFINITION" ) )
        {
            GATE gate;
            gate.Parse( cNode, aContext );
            GateSymbols.insert( std::make_pair( gate.ID, gate ) );
        }
        else if( cNodeName == wxT( "PARTDEFINITIONPIN" ) )
        {
            PIN pin;
            pin.Parse( cNode, aContext );
            Pins.insert( std::make_pair( pin.ID, pin ) );
        }
        else if( cNodeName == wxT( "ATTR" ) )
        {
            ATTRIBUTE_VALUE attr;
            attr.Parse( cNode, aContext );
            AttributeValues.insert( std::make_pair( attr.AttributeID, attr ) );
        }
        else if( cNodeName == wxT( "PINEQUIVALENCE" ) )
        {
            PIN_EQUIVALENCE pinEq;
            pinEq.Parse( cNode, aContext );
            PinEquivalences.push_back( pinEq );
        }
        else if( cNodeName == wxT( "SWAPGROUP" ) )
        {
            SWAP_GROUP swapGroup;
            swapGroup.Parse( cNode, aContext );
            SwapGroups.push_back( swapGroup );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, aNode->GetName() );
        }
    }
}


void CADSTAR_ARCHIVE_PARSER::PART::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "PART" ) );

    ID   = GetXmlAttributeIDString( aNode, 0 );
    Name = GetXmlAttributeIDString( aNode, 1 );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( cNodeName == wxT( "VERSION" ) )
        {
            Version = GetXmlAttributeIDLong( cNode, 0 );
        }
        else if( cNodeName == wxT( "HIDEPINNAMES" ) )
        {
            HidePinNames = true;
        }
        else if( cNodeName == wxT( "PARTDEFINITION" ) )
        {
            Definition.Parse( cNode, aContext );
        }
        else if( cNodeName == wxT( "PARTPIN" ) )
        {
            PART_PIN pin;
            pin.Parse( cNode, aContext );
            PartPins.insert( std::make_pair( pin.ID, pin ) );
        }
        else if( cNodeName == wxT( "ATTR" ) )
        {
            ATTRIBUTE_VALUE attr;
            attr.Parse( cNode, aContext );
            AttributeValues.insert( std::make_pair( attr.AttributeID, attr ) );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, aNode->GetName() );
        }
    }
}


void CADSTAR_ARCHIVE_PARSER::PARTS::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "PARTS" ) );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( cNodeName == wxT( "PART" ) )
        {
            PART part;
            part.Parse( cNode, aContext );
            PartDefinitions.insert( std::make_pair( part.ID, part ) );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, aNode->GetName() );
        }

        aContext->CheckPointCallback();
    }
}


void CADSTAR_ARCHIVE_PARSER::NET::JUNCTION::ParseIdentifiers( XNODE* aNode,
                                                              PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "JPT" ) );

    ID      = GetXmlAttributeIDString( aNode, 0 );
    LayerID = GetXmlAttributeIDString( aNode, 1 );
}


bool CADSTAR_ARCHIVE_PARSER::NET::JUNCTION::ParseSubNode( XNODE* aChildNode,
                                                          PARSER_CONTEXT* aContext )
{
    wxString cNodeName = aChildNode->GetName();

    if( cNodeName == wxT( "PT" ) )
        Location.Parse( aChildNode, aContext );
    else if( cNodeName == wxT( "FIX" ) )
        Fixed = true;
    else if( cNodeName == wxT( "GROUPREF" ) )
        GroupID = GetXmlAttributeIDString( aChildNode, 0 );
    else if( cNodeName == wxT( "REUSEBLOCKREF" ) )
        ReuseBlockRef.Parse( aChildNode, aContext );
    else
        return false;

    return true;
}


void CADSTAR_ARCHIVE_PARSER::NET::JUNCTION::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    ParseIdentifiers( aNode, aContext );
    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        if( ParseSubNode( cNode, aContext ) )
            continue;
        else
            THROW_UNKNOWN_NODE_IO_ERROR( cNode->GetName(), aNode->GetName() );
    }
}


void CADSTAR_ARCHIVE_PARSER::NET::CONNECTION::ParseIdentifiers( XNODE* aNode,
                                                                PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "CONN" ) );

    StartNode   = GetXmlAttributeIDString( aNode, 0 );
    EndNode     = GetXmlAttributeIDString( aNode, 1 );
    RouteCodeID = GetXmlAttributeIDString( aNode, 2 );
}


bool CADSTAR_ARCHIVE_PARSER::NET::CONNECTION::ParseSubNode( XNODE* aChildNode,
                                                            PARSER_CONTEXT* aContext )
{
    wxString cNodeName = aChildNode->GetName();

    if( cNodeName == wxT( "FIX" ) )
    {
        Fixed = true;
    }
    else if( cNodeName == wxT( "HIDDEN" ) )
    {
        Hidden = true;
    }
    else if( cNodeName == wxT( "GROUPREF" ) )
    {
        GroupID = GetXmlAttributeIDString( aChildNode, 0 );
    }
    else if( cNodeName == wxT( "REUSEBLOCKREF" ) )
    {
        ReuseBlockRef.Parse( aChildNode, aContext );
    }
    else if( cNodeName == wxT( "ATTR" ) )
    {
        ATTRIBUTE_VALUE attrVal;
        attrVal.Parse( aChildNode, aContext );
        AttributeValues.insert( std::make_pair( attrVal.AttributeID, attrVal ) );
    }
    else
    {
        return false;
    }

    return true;
}


void CADSTAR_ARCHIVE_PARSER::NET::ParseIdentifiers( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "NET" ) );

    ID = GetXmlAttributeIDString( aNode, 0 );
}


bool CADSTAR_ARCHIVE_PARSER::NET::ParseSubNode( XNODE* aChildNode, PARSER_CONTEXT* aContext )
{
    wxString cNodeName = aChildNode->GetName();

    if( cNodeName == wxT( "NETCODE" ) )
    {
        RouteCodeID = GetXmlAttributeIDString( aChildNode, 0 );
    }
    else if( cNodeName == wxT( "SIGNAME" ) )
    {
        Name = GetXmlAttributeIDString( aChildNode, 0 );
    }
    else if( cNodeName == wxT( "SIGNUM" ) )
    {
        SignalNum = GetXmlAttributeIDLong( aChildNode, 0 );
    }
    else if( cNodeName == wxT( "HIGHLIT" ) )
    {
        Highlight = true;
    }
    else if( cNodeName == wxT( "JPT" ) )
    {
        JUNCTION jpt;
        jpt.Parse( aChildNode, aContext );
        Junctions.insert( std::make_pair( jpt.ID, jpt ) );
    }
    else if( cNodeName == wxT( "NETCLASSREF" ) )
    {
        NetClassID = GetXmlAttributeIDString( aChildNode, 0 );
    }
    else if( cNodeName == wxT( "SPACINGCLASS" ) )
    {
        SpacingClassID = GetXmlAttributeIDString( aChildNode, 0 );
    }
    else if( cNodeName == wxT( "ATTR" ) )
    {
        ATTRIBUTE_VALUE attrVal;
        attrVal.Parse( aChildNode, aContext );
        AttributeValues.insert( std::make_pair( attrVal.AttributeID, attrVal ) );
    }
    else
    {
        return false;
    }

    return true;
}


void CADSTAR_ARCHIVE_PARSER::DOCUMENTATION_SYMBOL::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "DOCSYMBOL" ) );

    ID       = GetXmlAttributeIDString( aNode, 0 );
    SymdefID = GetXmlAttributeIDString( aNode, 1 );
    LayerID  = GetXmlAttributeIDString( aNode, 2 );

    XNODE* cNode        = aNode->GetChildren();
    bool   originParsed = false;

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( !originParsed && cNodeName == wxT( "PT" ) )
        {
            Origin.Parse( cNode, aContext );
            originParsed = true;
        }
        else if( cNodeName == wxT( "GROUPREF" ) )
        {
            GroupID = GetXmlAttributeIDString( cNode, 0 );
        }
        else if( cNodeName == wxT( "REUSEBLOCKREF" ) )
        {
            ReuseBlockRef.Parse( cNode, aContext );
        }
        else if( cNodeName == wxT( "FIX" ) )
        {
            Fixed = true;
        }
        else if( cNodeName == wxT( "MIRROR" ) )
        {
            Mirror = true;
        }
        else if( cNodeName == wxT( "READABILITY" ) )
        {
            Readability = ParseReadability( cNode );
        }
        else if( cNodeName == wxT( "ORIENT" ) )
        {
            OrientAngle = GetXmlAttributeIDLong( cNode, 0 );
        }
        else if( cNodeName == wxT( "ATTR" ) )
        {
            ATTRIBUTE_VALUE attr;
            attr.Parse( cNode, aContext );
            AttributeValues.insert( std::make_pair( attr.AttributeID, attr ) );
        }
        else if( cNodeName == wxT( "SCALE" ) )
        {
            ScaleRatioNumerator   = GetXmlAttributeIDLong( cNode, 0 );
            ScaleRatioDenominator = GetXmlAttributeIDLong( cNode, 1 );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, aNode->GetName() );
        }
    }

    if( !originParsed )
        THROW_MISSING_PARAMETER_IO_ERROR( wxT( "PT" ), aNode->GetName() );
}


void CADSTAR_ARCHIVE_PARSER::DFLTSETTINGS::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "DFLTSETTINGS" ) );

    Color = GetXmlAttributeIDString( aNode, 0 );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( cNodeName == wxT( "INVISIBLE" ) )
        {
            IsVisible = false;
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, aNode->GetName() );
        }
    }
}


void CADSTAR_ARCHIVE_PARSER::ATTRCOL::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "ATTRCOL" ) );

    AttributeID = GetXmlAttributeIDString( aNode, 0 );
    Color = GetXmlAttributeIDString( aNode, 1 );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( cNodeName == wxT( "INVISIBLE" ) )
        {
            IsVisible = false;
        }
        else if( cNodeName == wxT( "NOTPICKABLE" ) )
        {
            IsPickable = false;
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, aNode->GetName() );
        }
    }
}


void CADSTAR_ARCHIVE_PARSER::ATTRCOLORS::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "ATTRCOLORS" ) );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( cNodeName == wxT( "DFLTSETTINGS" ) )
        {
            DefaultSettings.Parse( cNode, aContext );
        }
        else if( cNodeName == wxT( "ATTRCOL" ) )
        {
            ATTRCOL attrcol;
            attrcol.Parse( cNode, aContext );
            AttributeColors.insert( { attrcol.AttributeID, attrcol } );
        }
        else if( cNodeName == wxT( "INVISIBLE" ) )
        {
            IsVisible = false;
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, aNode->GetName() );
        }
    }
}


void CADSTAR_ARCHIVE_PARSER::PARTNAMECOL::Parse( XNODE* aNode, PARSER_CONTEXT* aContext )
{
    wxASSERT( aNode->GetName() == wxT( "PARTNAMECOL" ) );

    Color = GetXmlAttributeIDString( aNode, 0 );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( cNodeName == wxT( "INVISIBLE" ) )
        {
            IsVisible = false;
        }
        else if( cNodeName == wxT( "NOTPICKABLE" ) )
        {
            IsPickable = false;
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, aNode->GetName() );
        }
    }
}


void CADSTAR_ARCHIVE_PARSER::InsertAttributeAtEnd( XNODE* aNode, wxString aValue )
{
    static const wxString c_numAttributes = wxT( "numAttributes" );

    wxString result;
    long     numAttributes = 0;

    if( aNode->GetAttribute( c_numAttributes, &result ) )
    {
        numAttributes = wxAtol( result );
        aNode->DeleteAttribute( c_numAttributes );
        ++numAttributes;
    }

#if wxUSE_UNICODE_WCHAR
    std::wstring numAttrStr = std::to_wstring( numAttributes );
#else
    std::string numAttrStr = std::to_string( numAttributes );
#endif

    aNode->AddAttribute( c_numAttributes, numAttrStr );

    wxString paramName = wxT( "attr" );
    paramName << numAttrStr;

    aNode->AddAttribute( paramName, aValue );
}


XNODE* CADSTAR_ARCHIVE_PARSER::LoadArchiveFile( const wxString& aFileName,
                                                const wxString& aFileTypeIdentifier,
                                                PROGRESS_REPORTER* aProgressReporter )
{
    KEYWORD   emptyKeywords[1] = {};
    XNODE*    rootNode = nullptr;
    XNODE*    cNode = nullptr;
    XNODE*    iNode = nullptr;
    int       tok;
    bool      cadstarFileCheckDone = false;
    wxString  str;
    wxCSConv  win1252( wxT( "windows-1252" ) );
    wxMBConv* conv = &win1252; // Initial testing suggests file encoding to be Windows-1252
                               // More samples required.

    // Open the file and get the file size
    FILE* fp = wxFopen( aFileName, wxT( "rt" ) );

    if( !fp )
        THROW_IO_ERROR( wxString::Format( _( "Cannot open file '%s'" ), aFileName ) );

    fseek( fp, 0L, SEEK_END );
    long fileSize = ftell( fp );
    rewind( fp );

    DSNLEXER lexer( emptyKeywords, 0, nullptr,  fp, aFileName );

    auto currentProgress =  [&]() -> double
                            {
                                return static_cast<double>( ftell( fp ) ) / fileSize;
                            };

    double previousReportedProgress = -1.0;

    while( ( tok = lexer.NextTok() ) != DSN_EOF )
    {
        if( aProgressReporter && ( currentProgress() - previousReportedProgress ) > 0.01 )
        {
            if( !aProgressReporter->KeepRefreshing() )
            {
                delete rootNode;

                THROW_IO_ERROR( _( "File import canceled by user." ) );
            }

            aProgressReporter->SetCurrentProgress( currentProgress() );
            previousReportedProgress = currentProgress();
        }

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
                delete rootNode;
                THROW_IO_ERROR( _( "The selected file is not valid or might be corrupt!" ) );
            }
        }
        else if( tok == DSN_LEFT )
        {
            tok   = lexer.NextTok();
            str   = wxString( lexer.CurText(), *conv );
            cNode = new XNODE( wxXML_ELEMENT_NODE, str );

            if( !rootNode )
                rootNode = cNode;

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
                    delete rootNode;
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
            delete rootNode;
            THROW_IO_ERROR( _( "The selected file is not valid or might be corrupt!" ) );
        }
    }

    // Not enough closing brackets
    if( iNode != nullptr )
    {
        delete rootNode;
        THROW_IO_ERROR( _( "The selected file is not valid or might be corrupt!" ) );
    }

    // Throw if no data was parsed
    if( rootNode )
    {
        return rootNode;
    }
    else
    {
        delete rootNode;
        THROW_IO_ERROR( _( "The selected file is not valid or might be corrupt!" ) );
    }

    return nullptr;
}


bool CADSTAR_ARCHIVE_PARSER::IsValidAttribute( wxXmlAttribute* aAttribute )
{
    return aAttribute->GetName() != wxT( "numAttributes" );
}


wxString CADSTAR_ARCHIVE_PARSER::GetXmlAttributeIDString( XNODE* aNode, unsigned int aID,
                                                          bool aIsRequired )
{
#if wxUSE_UNICODE_WCHAR
    std::wstring idStr = std::to_wstring( aID );
#else
    std::string idStr = std::to_string( aID );
#endif

    wxString attrName = wxS( "attr" );
    attrName << idStr;

    wxString retVal;

    if( !aNode->GetAttribute( attrName, &retVal ) )
    {
        if( aIsRequired )
            THROW_MISSING_PARAMETER_IO_ERROR( std::to_string( aID ), aNode->GetName() );
        else
            return wxEmptyString;
    }

    return retVal;
}


long CADSTAR_ARCHIVE_PARSER::GetXmlAttributeIDLong( XNODE* aNode, unsigned int aID,
                                                    bool aIsRequired )
{
    long retVal;
    bool success = GetXmlAttributeIDString( aNode, aID, aIsRequired ).ToLong( &retVal );

    if( !success )
    {
        if( aIsRequired )
            THROW_PARSING_IO_ERROR( std::to_string( aID ), aNode->GetName() );
        else
            return UNDEFINED_VALUE;
    }

    return retVal;
}


void CADSTAR_ARCHIVE_PARSER::CheckNoChildNodes( XNODE* aNode )
{
    if( aNode && aNode->GetChildren() )
        THROW_UNKNOWN_NODE_IO_ERROR( aNode->GetChildren()->GetName(), aNode->GetName() );
}


void CADSTAR_ARCHIVE_PARSER::CheckNoNextNodes( XNODE* aNode )
{
    if( aNode && aNode->GetNext() )
        THROW_UNKNOWN_NODE_IO_ERROR( aNode->GetNext()->GetName(), aNode->GetParent()->GetName() );
}


void CADSTAR_ARCHIVE_PARSER::ParseChildEValue( XNODE* aNode, PARSER_CONTEXT* aContext,
                                               EVALUE& aValueToParse )
{
    if( aNode->GetChildren()->GetName() == wxT( "E" ) )
        aValueToParse.Parse( aNode->GetChildren(), aContext );
    else
        THROW_UNKNOWN_NODE_IO_ERROR( aNode->GetChildren()->GetName(), aNode->GetName() );
}


std::vector<CADSTAR_ARCHIVE_PARSER::POINT> CADSTAR_ARCHIVE_PARSER::ParseAllChildPoints(
        XNODE* aNode, PARSER_CONTEXT* aContext, bool aTestAllChildNodes, int aExpectedNumPoints )
{
    std::vector<POINT> retVal;

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        if( cNode->GetName() == wxT( "PT" ) )
        {
            POINT pt;
            //TODO try.. catch + throw again with more detailed error information
            pt.Parse( cNode, aContext );
            retVal.push_back( pt );
        }
        else if( aTestAllChildNodes )
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNode->GetName(), aNode->GetName() );
        }
    }

    if( aExpectedNumPoints != UNDEFINED_VALUE
            && retVal.size() != static_cast<size_t>( aExpectedNumPoints ) )
    {
        THROW_IO_ERROR( wxString::Format(
                _( "Unexpected number of points in '%s'. Found %d but expected %d." ),
                aNode->GetName(), retVal.size(), aExpectedNumPoints ) );
    }

    return retVal;
}


std::vector<CADSTAR_ARCHIVE_PARSER::VERTEX> CADSTAR_ARCHIVE_PARSER::ParseAllChildVertices(
        XNODE* aNode, PARSER_CONTEXT* aContext, bool aTestAllChildNodes )
{
    std::vector<VERTEX> retVal;

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        if( VERTEX::IsVertex( cNode ) )
        {
            VERTEX vertex;
            //TODO try.. catch + throw again with more detailed error information
            vertex.Parse( cNode, aContext );
            retVal.push_back( vertex );
        }
        else if( aTestAllChildNodes )
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNode->GetName(), aNode->GetName() );
        }
    }

    return retVal;
}


std::vector<CADSTAR_ARCHIVE_PARSER::CUTOUT> CADSTAR_ARCHIVE_PARSER::ParseAllChildCutouts(
        XNODE* aNode, PARSER_CONTEXT* aContext, bool aTestAllChildNodes )
{
    std::vector<CUTOUT> retVal;

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        if( cNode->GetName() == wxT( "CUTOUT" ) )
        {
            CUTOUT cutout;
            //TODO try.. catch + throw again with more detailed error information
            cutout.Parse( cNode, aContext );
            retVal.push_back( cutout );
        }
        else if( aTestAllChildNodes )
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNode->GetName(), aNode->GetName() );
        }
    }

    return retVal;
}


long CADSTAR_ARCHIVE_PARSER::GetNumberOfChildNodes( XNODE* aNode )
{
    XNODE* childNodes = aNode->GetChildren();
    long   retval = 0;

    for( ; childNodes; childNodes = childNodes->GetNext() )
        retval++;

    return retval;
}


long CADSTAR_ARCHIVE_PARSER::GetNumberOfStepsForReporting(
        XNODE* aRootNode, std::vector<wxString> aSubNodeChildrenToCount )
{
    XNODE* level1Node = aRootNode->GetChildren();
    long   retval = 0;

    for( ; level1Node; level1Node = level1Node->GetNext() )
    {
        for( wxString childNodeName : aSubNodeChildrenToCount )
        {
            if( level1Node->GetName() == childNodeName )
                retval += GetNumberOfChildNodes( level1Node );
        }

        retval++;
    }

    return retval;
}


wxString CADSTAR_ARCHIVE_PARSER::HandleTextOverbar( wxString aCadstarString )
{
    wxString escapedText = aCadstarString;

    escapedText.Replace( wxT( "'" ), wxT( "~" ) );

    return ConvertToNewOverbarNotation( escapedText );
}


void CADSTAR_ARCHIVE_PARSER::FixTextPositionNoAlignment( EDA_TEXT* aKiCadTextItem )
{
    if( !aKiCadTextItem->GetText().IsEmpty() )
    {
        VECTOR2I positionOffset( 0, aKiCadTextItem->GetInterline( nullptr ) );
        RotatePoint( positionOffset, aKiCadTextItem->GetTextAngle() );

        //Count num of additional lines
        wxString text = aKiCadTextItem->GetText();
        int      numExtraLines = text.Replace( "\n", "\n" );
        numExtraLines -= text.at( text.size() - 1 ) == '\n'; // Ignore new line character at end
        positionOffset.x *= numExtraLines;
        positionOffset.y *= numExtraLines;

        aKiCadTextItem->Offset( positionOffset );
    }
}


wxString CADSTAR_ARCHIVE_PARSER::generateLibName( const wxString& aRefName,
                                                  const wxString& aAlternateName )
{
    if( aAlternateName.IsEmpty() )
        return EscapeString( aRefName, CTX_LIBID );
    else
        return EscapeString( aRefName + wxT( " (" ) + aAlternateName + wxT( ")" ), CTX_LIBID );
}


void CADSTAR_ARCHIVE_PARSER::checkPoint()
{
    if( m_progressReporter )
    {
        m_progressReporter->AdvanceProgress();

        if( !m_progressReporter->KeepRefreshing() )
            THROW_IO_ERROR( _( "File import canceled by user." ) );
    }
}

