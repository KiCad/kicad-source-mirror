/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Roberto Fernandez Bautista <roberto.fer.bau@gmail.com>
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
 * @file cadstar_archive_parser.cpp
 * @brief Helper functions and common defines between schematic and PCB Archive files
 */

#include <plugins/cadstar/cadstar_archive_parser.h>


void CADSTAR_ARCHIVE_PARSER::FORMAT::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "FORMAT" ) );

    Type    = GetXmlAttributeIDString( aNode, 0 );
    SomeInt = GetXmlAttributeIDLong( aNode, 1 );
    Version = GetXmlAttributeIDLong( aNode, 2 );
}


void CADSTAR_ARCHIVE_PARSER::TIMESTAMP::Parse( XNODE* aNode )
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


void CADSTAR_ARCHIVE_PARSER::HEADER::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "HEADER" ) );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString nodeName = cNode->GetName();

        if( nodeName == wxT( "FORMAT" ) )
        {
            Format.Parse( cNode );
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
            Timestamp.Parse( cNode );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNode->GetName(), wxT( "HEADER" ) );
        }
    }
}


void CADSTAR_ARCHIVE_PARSER::VARIANT::Parse( XNODE* aNode )
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


void CADSTAR_ARCHIVE_PARSER::VARIANT_HIERARCHY::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "VHIERARCHY" ) );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        if( cNode->GetName() == wxT( "VMASTER" ) || cNode->GetName() == wxT( "VARIANT" ) )
        {
            VARIANT variant;
            variant.Parse( cNode );
            Variants.insert( std::make_pair( variant.ID, variant ) );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNode->GetName(), cNode->GetName() );
        }
    }
}


void CADSTAR_ARCHIVE_PARSER::LINECODE::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "LINECODE" ) );

    ID   = GetXmlAttributeIDString( aNode, 0 );
    Name = GetXmlAttributeIDString( aNode, 1 );

    if( !GetXmlAttributeIDString( aNode, 2 ).ToLong( &Width ) )
        THROW_PARSING_IO_ERROR( wxT( "Line Width" ), wxString::Format( "LINECODE -> %s", Name ) );

    XNODE* cNode = aNode->GetChildren();

    if( cNode->GetName() != wxT( "STYLE" ) )
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


void CADSTAR_ARCHIVE_PARSER::HATCH::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "HATCH" ) );

    Step      = GetXmlAttributeIDLong( aNode, 0 );
    LineWidth = GetXmlAttributeIDLong( aNode, 2 );

    XNODE* cNode = aNode->GetChildren();

    if( !cNode || cNode->GetName() != wxT( "ORIENT" ) )
        THROW_MISSING_NODE_IO_ERROR( wxT( "ORIENT" ), wxT( "HATCH" ) );

    OrientAngle = GetXmlAttributeIDLong( cNode, 0 );
}


void CADSTAR_ARCHIVE_PARSER::HATCHCODE::Parse( XNODE* aNode )
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
        hatch.Parse( cNode );
        Hatches.push_back( hatch );
    }
}


void CADSTAR_ARCHIVE_PARSER::FONT::Parse( XNODE* aNode )
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


void CADSTAR_ARCHIVE_PARSER::TEXTCODE::Parse( XNODE* aNode )
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
            Font.Parse( cNode );
        else
            THROW_UNKNOWN_NODE_IO_ERROR( cNode->GetName(), aNode->GetName() );
    }
}


void CADSTAR_ARCHIVE_PARSER::ROUTEREASSIGN::Parse( XNODE* aNode )
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


void CADSTAR_ARCHIVE_PARSER::ROUTECODE::Parse( XNODE* aNode )
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
            routereassign.Parse( cNode );
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


void CADSTAR_ARCHIVE_PARSER::EVALUE::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "E" ) );

    if( ( !GetXmlAttributeIDString( aNode, 0 ).ToLong( &Base ) )
            || ( !GetXmlAttributeIDString( aNode, 1 ).ToLong( &Exponent ) ) )
    {
        THROW_PARSING_IO_ERROR( wxT( "Base and Exponent" ),
                wxString::Format(
                        "%s->%s", aNode->GetParent()->GetName(), aNode->GetParent()->GetName() ) );
    }
}


void CADSTAR_ARCHIVE_PARSER::POINT::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "PT" ) );

    x = GetXmlAttributeIDLong( aNode, 0 );
    y = GetXmlAttributeIDLong( aNode, 1 );
}


void CADSTAR_ARCHIVE_PARSER::LONGPOINT::Parse( XNODE* aNode )
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


void CADSTAR_ARCHIVE_PARSER::VERTEX::Parse( XNODE* aNode )
{
    wxASSERT( IsVertex( aNode ) );

    wxString aNodeName = aNode->GetName();

    if( aNodeName == wxT( "PT" ) )
    {
        Type     = VERTEX_TYPE::POINT;
        Center.x = UNDEFINED_VALUE;
        Center.y = UNDEFINED_VALUE;
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

        Center.x = UNDEFINED_VALUE;
        Center.y = UNDEFINED_VALUE;

        std::vector<POINT> pts = ParseAllChildPoints( aNode, true, 1 );

        End = pts[0];
    }
    else
    {
        wxASSERT_MSG( true, wxT( "Unknown VERTEX type" ) );
    }
}


void CADSTAR_ARCHIVE_PARSER::CUTOUT::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "CUTOUT" ) );

    Vertices = ParseAllChildVertices( aNode, true );
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


void CADSTAR_ARCHIVE_PARSER::SHAPE::Parse( XNODE* aNode )
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
    {
        wxASSERT_MSG( true, wxT( "Unknown SHAPE type" ) );
    }
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


void CADSTAR_ARCHIVE_PARSER::GRID::Parse( XNODE* aNode )
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


void CADSTAR_ARCHIVE_PARSER::GRIDS::Parse( XNODE* aNode )
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
                WorkingGrid.Parse( workingGridNode );
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
                ScreenGrid.Parse( screenGridNode );
            }
        }
        else if( GRID::IsGrid( cNode ) )
        {
            GRID userGrid;
            userGrid.Parse( cNode );
            UserGrids.push_back( userGrid );
        }
    }
}


bool CADSTAR_ARCHIVE_PARSER::SETTINGS::ParseSubNode( XNODE* aChildNode )
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
        DesignOrigin.Parse( aChildNode->GetChildren() );
    }
    else if( cNodeName == wxT( "DESIGNAREA" ) )
    {
        std::vector<POINT> pts = ParseAllChildPoints( aChildNode, true, 2 );
        DesignArea             = std::make_pair( pts[0], pts[1] );
    }
    else if( cNodeName == wxT( "DESIGNREF" ) )
    {
        DesignOrigin.Parse( aChildNode->GetChildren() );
    }
    else if( cNodeName == wxT( "DESIGNLIMIT" ) )
    {
        DesignLimit.Parse( aChildNode->GetChildren() );
    }
    else
    {
        return false;
    }

    return true;
}


void CADSTAR_ARCHIVE_PARSER::SETTINGS::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "SETTINGS" ) );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( ParseSubNode( cNode ) )
            continue;
        else
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, wxT( "SETTINGS" ) );
    }
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


void CADSTAR_ARCHIVE_PARSER::ATTRIBUTE_LOCATION::ParseIdentifiers( XNODE* aNode )
{
    TextCodeID = GetXmlAttributeIDString( aNode, 0 );
    LayerID    = GetXmlAttributeIDString( aNode, 1 );
}


bool CADSTAR_ARCHIVE_PARSER::ATTRIBUTE_LOCATION::ParseSubNode( XNODE* aChildNode )
{
    wxString cNodeName = aChildNode->GetName();

    if( cNodeName == wxT( "PT" ) )
        Position.Parse( aChildNode );
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


void CADSTAR_ARCHIVE_PARSER::ATTRIBUTE_LOCATION::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "ATTRLOC" ) );

    ParseIdentifiers( aNode );

    //Parse child nodes
    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        if( ParseSubNode( cNode ) )
            continue;
        else
            THROW_UNKNOWN_NODE_IO_ERROR( cNode->GetName(), wxT( "ATTRLOC" ) );
    }

    if( !Position.IsFullySpecified() )
        THROW_MISSING_NODE_IO_ERROR( wxT( "PT" ), wxT( "ATTRLOC" ) );
}


void CADSTAR_ARCHIVE_PARSER::ATTRNAME::COLUMNORDER::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "COLUMNORDER" ) );

    ID    = GetXmlAttributeIDLong( aNode, 0 );
    Order = GetXmlAttributeIDLong( aNode, 1 );

    CheckNoChildNodes( aNode );
}


void CADSTAR_ARCHIVE_PARSER::ATTRNAME::COLUMNWIDTH::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "COLUMNWIDTH" ) );

    ID    = GetXmlAttributeIDLong( aNode, 0 );
    Width = GetXmlAttributeIDLong( aNode, 1 );

    CheckNoChildNodes( aNode );
}


void CADSTAR_ARCHIVE_PARSER::ATTRNAME::Parse( XNODE* aNode )
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
            cOrder.Parse( cNode );
            ColumnOrders.push_back( cOrder );
        }
        else if( cNodeName == wxT( "COLUMNWIDTH" ) )
        {
            COLUMNWIDTH cWidth;
            cWidth.Parse( cNode );
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


void CADSTAR_ARCHIVE_PARSER::ATTRIBUTE_VALUE::Parse( XNODE* aNode )
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
            AttributeLocation.Parse( cNode );
            HasLocation = true;
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNode->GetName(), wxT( "ATTR" ) );
        }
    }
}


void CADSTAR_ARCHIVE_PARSER::TEXT_LOCATION::Parse( XNODE* aNode )
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

        if( ParseSubNode( cNode ) )
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

    if( !Position.IsFullySpecified() )
        THROW_MISSING_NODE_IO_ERROR( wxT( "PT" ), wxT( "TEXTLOC" ) );
}


void CADSTAR_ARCHIVE_PARSER::NETCLASS::Parse( XNODE* aNode )
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
            attribute_val.Parse( cNode );
            Attributes.push_back( attribute_val );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, location );
        }
    }
}


void CADSTAR_ARCHIVE_PARSER::SPCCLASSNAME::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "SPCCLASSNAME" ) );

    ID   = GetXmlAttributeIDString( aNode, 0 );
    Name = GetXmlAttributeIDString( aNode, 1 );
}


bool CADSTAR_ARCHIVE_PARSER::CODEDEFS::ParseSubNode( XNODE* aChildNode )
{
    wxString nodeName = aChildNode->GetName();

    if( nodeName == wxT( "LINECODE" ) )
    {
        LINECODE linecode;
        linecode.Parse( aChildNode );
        LineCodes.insert( std::make_pair( linecode.ID, linecode ) );
    }
    else if( nodeName == wxT( "HATCHCODE" ) )
    {
        HATCHCODE hatchcode;
        hatchcode.Parse( aChildNode );
        HatchCodes.insert( std::make_pair( hatchcode.ID, hatchcode ) );
    }
    else if( nodeName == wxT( "TEXTCODE" ) )
    {
        TEXTCODE textcode;
        textcode.Parse( aChildNode );
        TextCodes.insert( std::make_pair( textcode.ID, textcode ) );
    }
    else if( nodeName == wxT( "ROUTECODE" ) )
    {
        ROUTECODE routecode;
        routecode.Parse( aChildNode );
        RouteCodes.insert( std::make_pair( routecode.ID, routecode ) );
    }
    else if( nodeName == wxT( "ATTRNAME" ) )
    {
        ATTRNAME attrname;
        attrname.Parse( aChildNode );
        AttributeNames.insert( std::make_pair( attrname.ID, attrname ) );
    }
    else if( nodeName == wxT( "NETCLASS" ) )
    {
        NETCLASS netclass;
        netclass.Parse( aChildNode );
        NetClasses.insert( std::make_pair( netclass.ID, netclass ) );
    }
    else if( nodeName == wxT( "SPCCLASSNAME" ) )
    {
        SPCCLASSNAME spcclassname;
        spcclassname.Parse( aChildNode );
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


void CADSTAR_ARCHIVE_PARSER::REUSEBLOCK::Parse( XNODE* aNode )
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


void CADSTAR_ARCHIVE_PARSER::REUSEBLOCKREF::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "REUSEBLOCKREF" ) );

    ReuseBlockID  = GetXmlAttributeIDString( aNode, 0 );
    ItemReference = GetXmlAttributeIDString( aNode, 1 );

    CheckNoChildNodes( aNode );
}


void CADSTAR_ARCHIVE_PARSER::GROUP::Parse( XNODE* aNode )
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
            ReuseBlockRef.Parse( cNode );
        else
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, wxT( "GROUP" ) );
    }
}


void CADSTAR_ARCHIVE_PARSER::FIGURE::Parse( XNODE* aNode )
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
            Shape.Parse( cNode );
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
            ReuseBlockRef.Parse( cNode );
        }
        else if( cNodeName == wxT( "ATTR" ) )
        {
            ATTRIBUTE_VALUE attr;
            attr.Parse( cNode );
            AttributeValues.insert( std::make_pair( attr.AttributeID, attr ) );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, location );
        }
    }
}


void CADSTAR_ARCHIVE_PARSER::TEXT::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "TEXT" ) );

    ID = GetXmlAttributeIDString( aNode, 0 );
    //TODO: Need to lex/parse "Text" to identify design fields (e.g "<@DESIGN_TITLE@>") and
    // hyperlinks (e.g. "<@HYPERLINK\"[link]\"[link text]@>")
    Text       = GetXmlAttributeIDString( aNode, 1 );
    TextCodeID = GetXmlAttributeIDString( aNode, 2 );
    LayerID    = GetXmlAttributeIDString( aNode, 3 );

    XNODE* cNode = aNode->GetChildren();

    if( !cNode )
        THROW_MISSING_NODE_IO_ERROR( wxT( "PT" ), wxT( "TEXT" ) );

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( cNodeName == wxT( "PT" ) )
            Position.Parse( cNode );
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
            ReuseBlockRef.Parse( cNode );
        else
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, wxT( "TEXT" ) );
    }
}


void CADSTAR_ARCHIVE_PARSER::SYMDEF::ParseIdentifiers( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "SYMDEF" ) );

    ID            = GetXmlAttributeIDString( aNode, 0 );
    ReferenceName = GetXmlAttributeIDString( aNode, 1 );
    Alternate     = GetXmlAttributeIDString( aNode, 2 );
}


bool CADSTAR_ARCHIVE_PARSER::SYMDEF::ParseSubNode( XNODE* aChildNode )
{
    wxString cNodeName = aChildNode->GetName();

    if( cNodeName == wxT( "PT" ) )
    {
        Origin.Parse( aChildNode );
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
        figure.Parse( aChildNode );
        Figures.insert( std::make_pair( figure.ID, figure ) );
    }
    else if( cNodeName == wxT( "TEXT" ) )
    {
        TEXT txt;
        txt.Parse( aChildNode );
        Texts.insert( std::make_pair( txt.ID, txt ) );
    }
    else if( cNodeName == wxT( "TEXTLOC" ) )
    {
        TEXT_LOCATION textloc;
        textloc.Parse( aChildNode );
        TextLocations.insert( std::make_pair( textloc.AttributeID, textloc ) );
    }
    else if( cNodeName == wxT( "ATTR" ) )
    {
        ATTRIBUTE_VALUE attrVal;
        attrVal.Parse( aChildNode );
        AttributeValues.insert( std::make_pair( attrVal.AttributeID, attrVal ) );
    }
    else
    {
        return false;
    }

    return true;
}


void CADSTAR_ARCHIVE_PARSER::PART::DEFINITION::GATE::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "GATEDEFINITION" ) );

    ID        = GetXmlAttributeIDString( aNode, 0 );
    Name      = GetXmlAttributeIDString( aNode, 1 );
    Alternate = GetXmlAttributeIDString( aNode, 2 );
    PinCount  = GetXmlAttributeIDLong( aNode, 3 );

    CheckNoChildNodes( aNode );
}


CADSTAR_ARCHIVE_PARSER::PART::PIN_TYPE CADSTAR_ARCHIVE_PARSER::PART::GetPinType( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "PINTYPE" ) );

    wxString pinTypeStr = GetXmlAttributeIDString( aNode, 0 );

    std::map<wxString, PIN_TYPE> pinTypeMap = { { wxT( "INPUT" ), PIN_TYPE::INPUT },
        { wxT( "OUTPUT_OR" ), PIN_TYPE::OUTPUT_OR },
        { wxT( "OUTPUT_NOT_OR" ), PIN_TYPE::OUTPUT_NOT_OR },
        { wxT( "OUTPUT_NOT_NORM_OR" ), PIN_TYPE::OUTPUT_NOT_NORM_OR },
        { wxT( "POWER" ), PIN_TYPE::POWER }, { wxT( "GROUND" ), PIN_TYPE::GROUND },
        { wxT( "TRISTATE_BIDIR" ), PIN_TYPE::TRISTATE_BIDIR },
        { wxT( "TRISTATE_INPUT" ), PIN_TYPE::TRISTATE_INPUT },
        { wxT( "TRISTATE_DRIVER" ), PIN_TYPE::TRISTATE_DRIVER } };

    if( pinTypeMap.find( pinTypeStr ) == pinTypeMap.end() )
        THROW_UNKNOWN_PARAMETER_IO_ERROR( pinTypeStr, aNode->GetName() );

    return pinTypeMap[pinTypeStr];
}


void CADSTAR_ARCHIVE_PARSER::PART::DEFINITION::PIN::Parse( XNODE* aNode )
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
            Position = (POSITION) GetXmlAttributeIDLong( cNode, 0 );
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


void CADSTAR_ARCHIVE_PARSER::PART::PART_PIN::Parse( XNODE* aNode )
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


void CADSTAR_ARCHIVE_PARSER::PART::DEFINITION::PIN_EQUIVALENCE::Parse( XNODE* aNode )
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


void CADSTAR_ARCHIVE_PARSER::PART::DEFINITION::SWAP_GATE::Parse( XNODE* aNode )
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


void CADSTAR_ARCHIVE_PARSER::PART::DEFINITION::SWAP_GROUP::Parse( XNODE* aNode )
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
            swapGate.Parse( cNode );
            SwapGates.push_back( swapGate );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, aNode->GetName() );
        }
    }
}


void CADSTAR_ARCHIVE_PARSER::PART::DEFINITION::Parse( XNODE* aNode )
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
            gate.Parse( cNode );
            GateSymbols.insert( std::make_pair( gate.ID, gate ) );
        }
        else if( cNodeName == wxT( "PARTDEFINITIONPIN" ) )
        {
            PIN pin;
            pin.Parse( cNode );
            Pins.insert( std::make_pair( pin.ID, pin ) );
        }
        else if( cNodeName == wxT( "ATTR" ) )
        {
            ATTRIBUTE_VALUE attr;
            attr.Parse( cNode );
            AttributeValues.insert( std::make_pair( attr.AttributeID, attr ) );
        }
        else if( cNodeName == wxT( "PINEQUIVALENCE" ) )
        {
            PIN_EQUIVALENCE pinEq;
            pinEq.Parse( cNode );
            PinEquivalences.push_back( pinEq );
        }
        else if( cNodeName == wxT( "SWAPGROUP" ) )
        {
            SWAP_GROUP swapGroup;
            swapGroup.Parse( cNode );
            SwapGroups.push_back( swapGroup );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, aNode->GetName() );
        }
    }
}


void CADSTAR_ARCHIVE_PARSER::PART::Parse( XNODE* aNode )
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
            Definition.Parse( cNode );
        }
        else if( cNodeName == wxT( "PARTPIN" ) )
        {
            PART_PIN pin;
            pin.Parse( cNode );
            PartPins.insert( std::make_pair( pin.ID, pin ) );
        }
        else if( cNodeName == wxT( "ATTR" ) )
        {
            ATTRIBUTE_VALUE attr;
            attr.Parse( cNode );
            AttributeValues.insert( std::make_pair( attr.AttributeID, attr ) );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, aNode->GetName() );
        }
    }
}


void CADSTAR_ARCHIVE_PARSER::PARTS::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "PARTS" ) );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( cNodeName == wxT( "PART" ) )
        {
            PART part;
            part.Parse( cNode );
            PartDefinitions.insert( std::make_pair( part.ID, part ) );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, aNode->GetName() );
        }
    }
}


void CADSTAR_ARCHIVE_PARSER::NET::JUNCTION::ParseIdentifiers( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "JPT" ) );

    ID      = GetXmlAttributeIDString( aNode, 0 );
    LayerID = GetXmlAttributeIDString( aNode, 1 );
}


bool CADSTAR_ARCHIVE_PARSER::NET::JUNCTION::ParseSubNode( XNODE* aChildNode )
{
    wxString cNodeName = aChildNode->GetName();

    if( cNodeName == wxT( "PT" ) )
        Location.Parse( aChildNode );
    else if( cNodeName == wxT( "FIX" ) )
        Fixed = true;
    else if( cNodeName == wxT( "GROUPREF" ) )
        GroupID = GetXmlAttributeIDString( aChildNode, 0 );
    else if( cNodeName == wxT( "REUSEBLOCKREF" ) )
        ReuseBlockRef.Parse( aChildNode );
    else
        return false;

    return true;
}


void CADSTAR_ARCHIVE_PARSER::NET::JUNCTION::Parse( XNODE* aNode )
{
    ParseIdentifiers( aNode );
    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        if( ParseSubNode( cNode ) )
            continue;
        else
            THROW_UNKNOWN_NODE_IO_ERROR( cNode->GetName(), aNode->GetName() );
    }
}


void CADSTAR_ARCHIVE_PARSER::NET::CONNECTION::ParseIdentifiers( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "CONN" ) );

    StartNode   = GetXmlAttributeIDString( aNode, 0 );
    EndNode     = GetXmlAttributeIDString( aNode, 1 );
    RouteCodeID = GetXmlAttributeIDString( aNode, 2 );
}


bool CADSTAR_ARCHIVE_PARSER::NET::CONNECTION::ParseSubNode( XNODE* aChildNode )
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
        ReuseBlockRef.Parse( aChildNode );
    }
    else if( cNodeName == wxT( "ATTR" ) )
    {
        ATTRIBUTE_VALUE attrVal;
        attrVal.Parse( aChildNode );
        AttributeValues.insert( std::make_pair( attrVal.AttributeID, attrVal ) );
    }
    else
    {
        return false;
    }

    return true;
}


void CADSTAR_ARCHIVE_PARSER::NET::ParseIdentifiers( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "NET" ) );

    ID = GetXmlAttributeIDString( aNode, 0 );
}


bool CADSTAR_ARCHIVE_PARSER::NET::ParseSubNode( XNODE* aChildNode )
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
        jpt.Parse( aChildNode );
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
        attrVal.Parse( aChildNode );
        AttributeValues.insert( std::make_pair( attrVal.AttributeID, attrVal ) );
    }
    else
    {
        return false;
    }

    return true;
}


void CADSTAR_ARCHIVE_PARSER::DOCUMENTATION_SYMBOL::Parse( XNODE* aNode )
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
            Origin.Parse( cNode );
            originParsed = true;
        }
        else if( cNodeName == wxT( "GROUPREF" ) )
        {
            GroupID = GetXmlAttributeIDString( cNode, 0 );
        }
        else if( cNodeName == wxT( "REUSEBLOCKREF" ) )
        {
            ReuseBlockRef.Parse( cNode );
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
            attr.Parse( cNode );
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


void CADSTAR_ARCHIVE_PARSER::InsertAttributeAtEnd( XNODE* aNode, wxString aValue )
{
    wxString result;
    int      numAttributes = 0;

    if( aNode->GetAttribute( wxT( "numAttributes" ), &result ) )
    {
        numAttributes = wxAtoi( result );
        aNode->DeleteAttribute( wxT( "numAttributes" ) );
        ++numAttributes;
    }

    aNode->AddAttribute( wxT( "numAttributes" ), wxString::Format( wxT( "%i" ), numAttributes ) );

    wxString paramName = wxT( "attr" );
    paramName << numAttributes;

    aNode->AddAttribute( paramName, aValue );
}


XNODE* CADSTAR_ARCHIVE_PARSER::LoadArchiveFile(
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
                    THROW_IO_ERROR( _( "The selected file is not valid or might be corrupt!" ) );

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

    // Not enough closing brackets
    if( iNode != NULL )
        THROW_IO_ERROR( _( "The selected file is not valid or might be corrupt!" ) );

    // Throw if no data was parsed
    if( cNode )
        return cNode;
    else
        THROW_IO_ERROR( _( "The selected file is not valid or might be corrupt!" ) );

    return NULL;
}


bool CADSTAR_ARCHIVE_PARSER::IsValidAttribute( wxXmlAttribute* aAttribute )
{
    return aAttribute->GetName() != wxT( "numAttributes" );
}


wxString CADSTAR_ARCHIVE_PARSER::GetXmlAttributeIDString(
        XNODE* aNode, unsigned int aID, bool aIsRequired )
{
    wxString attrName, retVal;
    attrName = "attr";
    attrName << aID;

    if( !aNode->GetAttribute( attrName, &retVal ) )
    {
        if( aIsRequired )
            THROW_MISSING_PARAMETER_IO_ERROR( std::to_string( aID ), aNode->GetName() );
        else
            return wxEmptyString;
    }

    return retVal;
}


long CADSTAR_ARCHIVE_PARSER::GetXmlAttributeIDLong(
        XNODE* aNode, unsigned int aID, bool aIsRequired )
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


void CADSTAR_ARCHIVE_PARSER::ParseChildEValue( XNODE* aNode, EVALUE& aValueToParse )
{
    if( aNode->GetChildren()->GetName() == wxT( "E" ) )
        aValueToParse.Parse( aNode->GetChildren() );
    else
        THROW_UNKNOWN_NODE_IO_ERROR( aNode->GetChildren()->GetName(), aNode->GetName() );
}


std::vector<CADSTAR_ARCHIVE_PARSER::POINT> CADSTAR_ARCHIVE_PARSER::ParseAllChildPoints(
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
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNode->GetName(), aNode->GetName() );
        }
    }

    return retVal;
}


std::vector<CADSTAR_ARCHIVE_PARSER::CUTOUT> CADSTAR_ARCHIVE_PARSER::ParseAllChildCutouts(
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
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNode->GetName(), aNode->GetName() );
        }
    }

    return retVal;
}
