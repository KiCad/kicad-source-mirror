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
 * @file cadstar_sch_archive_parser.cpp
 * @brief Reads in a CADSTAR Schematic Archive (*.csa) file
 */

#include <convert_to_biu.h> // SCH_IU_PER_MM
#include <macros.h>
#include <sch_plugins/cadstar/cadstar_sch_archive_parser.h>


void CADSTAR_SCH_ARCHIVE_PARSER::Parse()
{
    XNODE* fileRootNode = LoadArchiveFile( Filename, wxT( "CADSTARSCM" ) );

    XNODE* cNode = fileRootNode->GetChildren();

    if( !cNode )
        THROW_MISSING_NODE_IO_ERROR( wxT( "HEADER" ), wxT( "CADSTARSCM" ) );

    for( ; cNode; cNode = cNode->GetNext() )
    {
        if( cNode->GetName() == wxT( "HEADER" ) )
        {
            Header.Parse( cNode );

            switch( Header.Resolution )
            {
            case RESOLUTION::HUNDREDTH_MICRON:
                KiCadUnitMultiplier = SCH_IU_PER_MM / 1e5;
                break;

            default:
                wxASSERT_MSG( true, wxT( "Unknown File Resolution" ) );
                break;
            }
        }
        else if( cNode->GetName() == wxT( "ASSIGNMENTS" ) )
        {
            Assignments.Parse( cNode );
        }
        else if( cNode->GetName() == wxT( "LIBRARY" ) )
        {
            Library.Parse( cNode );
        }
        else if( cNode->GetName() == wxT( "DEFAULTS" ) )
        {
            // No design information here (no need to parse)
            // Only contains CADSTAR configuration data such as default shapes, text and units
            // In future some of this could be converted to KiCad but limited value
        }
        else if( cNode->GetName() == wxT( "PARTS" ) )
        {
            Parts.Parse( cNode );
        }
        else if( cNode->GetName() == wxT( "SHEETS" ) )
        {
            Sheets.Parse( cNode );
        }
        else if( cNode->GetName() == wxT( "SCHEMATIC" ) )
        {
            Schematic.Parse( cNode );
        }
        else if( cNode->GetName() == wxT( "DISPLAY" ) )
        {
            // No design information here (no need to parse)
            // Contains CADSTAR Display settings such as layer/element colours and visibility.
            // In the future these settings could be converted to KiCad
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNode->GetName(), wxT( "[root]" ) );
        }
    }

    delete fileRootNode;
}


CADSTAR_SCH_ARCHIVE_PARSER::TERMINAL_SHAPE_TYPE CADSTAR_SCH_ARCHIVE_PARSER::ParseTermShapeType(
        const wxString& aShapeStr )
{
    if( aShapeStr == wxT( "ANNULUS" ) )
        return TERMINAL_SHAPE_TYPE::ANNULUS;
    else if( aShapeStr == wxT( "BOX" ) )
        return TERMINAL_SHAPE_TYPE::BOX;
    else if( aShapeStr == wxT( "BULLET" ) )
        return TERMINAL_SHAPE_TYPE::BULLET;
    else if( aShapeStr == wxT( "ROUND" ) )
        return TERMINAL_SHAPE_TYPE::CIRCLE;
    else if( aShapeStr == wxT( "CROSS" ) )
        return TERMINAL_SHAPE_TYPE::CROSS;
    else if( aShapeStr == wxT( "DIAMOND" ) )
        return TERMINAL_SHAPE_TYPE::DIAMOND;
    else if( aShapeStr == wxT( "FINGER" ) )
        return TERMINAL_SHAPE_TYPE::FINGER;
    else if( aShapeStr == wxT( "OCTAGON" ) )
        return TERMINAL_SHAPE_TYPE::OCTAGON;
    else if( aShapeStr == wxT( "PLUS" ) )
        return TERMINAL_SHAPE_TYPE::PLUS;
    else if( aShapeStr == wxT( "POINTER" ) )
        return TERMINAL_SHAPE_TYPE::POINTER;
    else if( aShapeStr == wxT( "RECTANGLE" ) )
        return TERMINAL_SHAPE_TYPE::RECTANGLE;
    else if( aShapeStr == wxT( "ROUNDED" ) )
        return TERMINAL_SHAPE_TYPE::ROUNDED_RECT;
    else if( aShapeStr == wxT( "SQUARE" ) )
        return TERMINAL_SHAPE_TYPE::SQUARE;
    else if( aShapeStr == wxT( "STAR" ) )
        return TERMINAL_SHAPE_TYPE::STAR;
    else if( aShapeStr == wxT( "TRIANGLE" ) )
        return TERMINAL_SHAPE_TYPE::TRIANGLE;
    else
        return TERMINAL_SHAPE_TYPE::UNDEFINED;
}


bool CADSTAR_SCH_ARCHIVE_PARSER::TERMINAL_SHAPE::IsTermShape( XNODE* aNode )
{
    return ParseTermShapeType( aNode->GetName() ) != TERMINAL_SHAPE_TYPE::UNDEFINED;
}


void CADSTAR_SCH_ARCHIVE_PARSER::TERMINAL_SHAPE::Parse( XNODE* aNode )
{
    wxCHECK( IsTermShape( aNode ), );

    ShapeType = ParseTermShapeType( aNode->GetName() );
    Size      = GetXmlAttributeIDLong( aNode, 0 );

    switch( ShapeType )
    {
    case TERMINAL_SHAPE_TYPE::ANNULUS:
    case TERMINAL_SHAPE_TYPE::BOX:
    case TERMINAL_SHAPE_TYPE::CROSS:
    case TERMINAL_SHAPE_TYPE::PLUS:
    case TERMINAL_SHAPE_TYPE::STAR:
        InternalFeature = GetXmlAttributeIDLong( aNode, 1 );
        break;

    case TERMINAL_SHAPE_TYPE::ROUNDED_RECT:
        InternalFeature = GetXmlAttributeIDLong( aNode, 3 );
        KI_FALLTHROUGH;

    case TERMINAL_SHAPE_TYPE::BULLET:
    case TERMINAL_SHAPE_TYPE::FINGER:
    case TERMINAL_SHAPE_TYPE::POINTER:
    case TERMINAL_SHAPE_TYPE::RECTANGLE:
    case TERMINAL_SHAPE_TYPE::TRIANGLE:
        RightLength = GetXmlAttributeIDLong( aNode, 2 );
        LeftLength  = GetXmlAttributeIDLong( aNode, 1 );
        break;

    case TERMINAL_SHAPE_TYPE::CIRCLE:
    case TERMINAL_SHAPE_TYPE::DIAMOND:
    case TERMINAL_SHAPE_TYPE::OCTAGON:
    case TERMINAL_SHAPE_TYPE::SQUARE:
        //don't do anything
        break;

    case TERMINAL_SHAPE_TYPE::UNDEFINED:
        wxASSERT_MSG( false, "Unknown terminal shape type" );
        break;
    }

    if( aNode->GetChildren() )
    {
        if( aNode->GetChildren()->GetName() == wxT( "ORIENT" ) )
        {
            OrientAngle = GetXmlAttributeIDLong( aNode->GetChildren(), 0 );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( aNode->GetChildren()->GetName(), aNode->GetName() );
        }

        CheckNoNextNodes( aNode->GetChildren() );
    }
}


void CADSTAR_SCH_ARCHIVE_PARSER::TERMINALCODE::Parse( XNODE* aNode )
{
    wxCHECK( aNode->GetName() == wxT( "TERMINALCODE" ), );

    ID   = GetXmlAttributeIDString( aNode, 0 );
    Name = GetXmlAttributeIDString( aNode, 1 );

    XNODE*   cNode    = aNode->GetChildren();
    wxString location = wxString::Format( "TERMINALCODE -> %s", Name );

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( TERMINAL_SHAPE::IsTermShape( cNode ) )
            Shape.Parse( cNode );
        else if( cNodeName == wxT( "FILLED" ) )
            Filled = true;
        else
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, location );
    }
}


void CADSTAR_SCH_ARCHIVE_PARSER::CODEDEFS_SCM::Parse( XNODE* aNode )
{
    wxCHECK( aNode->GetName() == wxT( "CODEDEFS" ), );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString nodeName = cNode->GetName();

        if( ParseSubNode( cNode ) ) // in CADSTAR_ARCHIVE_PARSER::CODEDEFS
        {
            continue;
        }
        else if( nodeName == wxT( "TERMINALCODE" ) )
        {
            TERMINALCODE termcode;
            termcode.Parse( cNode );
            TerminalCodes.insert( std::make_pair( termcode.ID, termcode ) );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( nodeName, aNode->GetName() );
        }
    }
}


void CADSTAR_SCH_ARCHIVE_PARSER::ASSIGNMENTS_SCM::Parse( XNODE* aNode )
{
    wxCHECK( aNode->GetName() == wxT( "ASSIGNMENTS" ), );

    XNODE* cNode          = aNode->GetChildren();
    bool   settingsParsed = false;

    for( ; cNode; cNode = cNode->GetNext() )
    {
        if( cNode->GetName() == wxT( "CODEDEFS" ) )
        {
            Codedefs.Parse( cNode );
        }
        else if( cNode->GetName() == wxT( "SETTINGS" ) )
        {
            settingsParsed = true;
            Settings.Parse( cNode );
        }
        else if( cNode->GetName() == wxT( "GRIDS" ) )
        {
            Grids.Parse( cNode );
        }
        else if( cNode->GetName() == wxT( "NETCLASSEDITATTRIBSETTINGS" ) )
        {
            NetclassEditAttributeSettings = true;
        }
        else if( cNode->GetName() == wxT( "SPCCLASSEDITATTRIBSETTINGS" ) )
        {
            SpacingclassEditAttributeSettings = true;
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNode->GetName(), aNode->GetName() );
        }
    }

    if( !settingsParsed )
        THROW_MISSING_NODE_IO_ERROR( wxT( "SETTINGS" ), wxT( "ASSIGNMENTS" ) );
}


void CADSTAR_SCH_ARCHIVE_PARSER::TERMINAL::Parse( XNODE* aNode )
{
    wxCHECK( aNode->GetName() == wxT( "TERMINAL" ), );

    ID             = GetXmlAttributeIDLong( aNode, 0 );
    TerminalCodeID = GetXmlAttributeIDString( aNode, 1 );

    XNODE*   cNode    = aNode->GetChildren();
    wxString location = wxString::Format( "TERMINAL %ld", ID );

    if( !cNode )
        THROW_MISSING_NODE_IO_ERROR( wxT( "PT" ), location );

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( cNodeName == wxT( "ORIENT" ) )
            OrientAngle = GetXmlAttributeIDLong( cNode, 0 );
        else if( cNodeName == wxT( "PT" ) )
            Position.Parse( cNode );
        else
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, location );
    }
}


void CADSTAR_SCH_ARCHIVE_PARSER::PIN_NUM_LABEL_LOC::Parse( XNODE* aNode )
{
    wxCHECK( aNode->GetName() == wxT( "PINLABELLOC" )
                     || aNode->GetName() == wxT( "PINNUMNAMELOC" ), );

    TerminalID = GetXmlAttributeIDLong( aNode, 0 );
    TextCodeID = GetXmlAttributeIDString( aNode, 1 );

    //Parse child nodes
    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        if( ParseSubNode( cNode ) )
            continue;
        else
            THROW_UNKNOWN_NODE_IO_ERROR( cNode->GetName(), aNode->GetName() );
    }

    if( !Position.IsFullySpecified() )
        THROW_MISSING_NODE_IO_ERROR( wxT( "PT" ), aNode->GetName() );
}


void CADSTAR_SCH_ARCHIVE_PARSER::SYMDEF_SCM::Parse( XNODE* aNode )
{
    wxCHECK( aNode->GetName() == wxT( "SYMDEF" ), );

    ParseIdentifiers( aNode );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( ParseSubNode( cNode ) )
        {
            continue;
        }
        else if( cNodeName == wxT( "TERMINAL" ) )
        {
            TERMINAL term;
            term.Parse( cNode );
            Terminals.insert( std::make_pair( term.ID, term ) );
        }
        else if( cNodeName == wxT( "PINLABELLOC" ) )
        {
            PIN_NUM_LABEL_LOC loc;
            loc.Parse( cNode );
            PinLabelLocations.insert( std::make_pair( loc.TerminalID, loc ) );
        }
        else if( cNodeName == wxT( "PINNUMNAMELOC" ) )
        {
            PIN_NUM_LABEL_LOC loc;
            loc.Parse( cNode );
            PinNumberLocations.insert( std::make_pair( loc.TerminalID, loc ) );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, aNode->GetName() );
        }
    }

    if( !Stub && !Origin.IsFullySpecified() )
        THROW_MISSING_PARAMETER_IO_ERROR( wxT( "PT" ), aNode->GetName() );
}


void CADSTAR_SCH_ARCHIVE_PARSER::LIBRARY_SCM::Parse( XNODE* aNode )
{
    wxCHECK( aNode->GetName() == wxT( "LIBRARY" ), );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( cNodeName == wxT( "SYMDEF" ) )
        {
            SYMDEF_SCM symdef;
            symdef.Parse( cNode );
            SymbolDefinitions.insert( std::make_pair( symdef.ID, symdef ) );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, aNode->GetName() );
        }
    }
}


void CADSTAR_SCH_ARCHIVE_PARSER::SHEETS::Parse( XNODE* aNode )
{
    wxCHECK( aNode->GetName() == wxT( "SHEETS" ), );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        if( cNode->GetName() == wxT( "SHEET" ) )
        {
            LAYER_ID   id   = GetXmlAttributeIDString( cNode, 0 );
            SHEET_NAME name = GetXmlAttributeIDString( cNode, 1 );
            SheetNames.insert( std::make_pair( id, name ) );
            SheetOrder.push_back( id );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNode->GetName(), aNode->GetName() );
        }
    }
}


void CADSTAR_SCH_ARCHIVE_PARSER::COMP::Parse( XNODE* aNode )
{
    wxCHECK( aNode->GetName() == wxT( "COMP" ), );

    Designator = GetXmlAttributeIDString( aNode, 0 );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        if( cNode->GetName() == wxT( "READONLY" ) )
        {
            ReadOnly = true;
        }
        else if( cNode->GetName() == wxT( "ATTRLOC" ) )
        {
            AttrLoc.Parse( cNode );
            HasLocation = true;
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNode->GetName(), aNode->GetName() );
        }
    }
}


void CADSTAR_SCH_ARCHIVE_PARSER::PARTREF::Parse( XNODE* aNode )
{
    wxCHECK( aNode->GetName() == wxT( "PARTREF" ), );

    RefID = GetXmlAttributeIDString( aNode, 0 );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        if( cNode->GetName() == wxT( "READONLY" ) )
        {
            ReadOnly = true;
        }
        else if( cNode->GetName() == wxT( "ATTRLOC" ) )
        {
            AttrLoc.Parse( cNode );
            HasLocation = true;
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNode->GetName(), aNode->GetName() );
        }
    }
}


void CADSTAR_SCH_ARCHIVE_PARSER::TERMATTR::Parse( XNODE* aNode )
{
    wxCHECK( aNode->GetName() == wxT( "TERMATTR" ), /* void */ );

    TerminalID = GetXmlAttributeIDLong( aNode, 0 );

    XNODE* cNode = aNode->GetChildren();
    bool   attrParsed = false;

    for( ; cNode; cNode = cNode->GetNext() )
    {
        if( !attrParsed && cNode->GetName() == wxT( "ATTR" ) )
        {
            Value.Parse( cNode );
            attrParsed = true;
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNode->GetName(), aNode->GetName() );
        }
    }
}


void CADSTAR_SCH_ARCHIVE_PARSER::SYMPINNAME_LABEL::Parse( XNODE* aNode )
{
    wxCHECK( aNode->GetName() == wxT( "SYMPINNAME" ) || aNode->GetName() == wxT( "SYMPINLABEL" ), );

    TerminalID  = GetXmlAttributeIDLong( aNode, 0 );
    NameOrLabel = GetXmlAttributeIDString( aNode, 1 );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        if( cNode->GetName() == wxT( "ATTRLOC" ) )
        {
            AttrLoc.Parse( cNode );
            HasLocation = true;
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNode->GetName(), aNode->GetName() );
        }
    }
}


void CADSTAR_SCH_ARCHIVE_PARSER::SYMBOL::PIN_NUM::Parse( XNODE* aNode )
{
    wxCHECK( aNode->GetName() == wxT( "PINNUM" ), );

    TerminalID = GetXmlAttributeIDLong( aNode, 0 );
    PinNum     = GetXmlAttributeIDLong( aNode, 1 );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        if( cNode->GetName() == wxT( "ATTRLOC" ) )
        {
            AttrLoc.Parse( cNode );
            HasLocation = true;
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNode->GetName(), aNode->GetName() );
        }
    }
}


void CADSTAR_SCH_ARCHIVE_PARSER::SYMBOLVARIANT::Parse( XNODE* aNode )
{
    wxCHECK( aNode->GetName() == wxT( "SYMBOLVARIANT" ), );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( cNodeName == wxT( "SIGNALREF" ) )
        {
            Type = TYPE::SIGNALREF;
            CheckNoNextNodes( cNode );
        }
        else if( cNodeName == wxT( "GLOBALSIGNAL" ) )
        {
            Type      = TYPE::GLOBALSIGNAL;
            Reference = GetXmlAttributeIDString( cNode, 0 );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, aNode->GetName() );
        }
    }
}


void CADSTAR_SCH_ARCHIVE_PARSER::SIGNALREFERENCELINK::Parse( XNODE* aNode )
{
    wxCHECK( aNode->GetName() == wxT( "SIGNALREFERENCELINK" ), );

    TextCodeID = GetXmlAttributeIDString( aNode, 0 );
    LayerID    = GetXmlAttributeIDString( aNode, 2 );

    //Parse child nodes
    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        if( ParseSubNode( cNode ) )
            continue;
        else if( cNode->GetName() == wxT( "SIGREFTEXT" ) )
            Text = GetXmlAttributeIDString( aNode, 0 );
        else
            THROW_UNKNOWN_NODE_IO_ERROR( cNode->GetName(), aNode->GetName() );
    }

    if( !Position.IsFullySpecified() )
        THROW_MISSING_NODE_IO_ERROR( wxT( "PT" ), aNode->GetName() );
}


void CADSTAR_SCH_ARCHIVE_PARSER::SYMBOL::Parse( XNODE* aNode )
{
    wxCHECK( aNode->GetName() == wxT( "SYMBOL" ), );

    ID       = GetXmlAttributeIDString( aNode, 0 );
    SymdefID = GetXmlAttributeIDString( aNode, 1 );
    LayerID  = GetXmlAttributeIDString( aNode, 2 );

    XNODE*   cNode        = aNode->GetChildren();
    bool     originParsed = false;
    wxString location     = wxString::Format( "SYMBOL -> %s", ID );

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( !originParsed && cNodeName == wxT( "PT" ) )
        {
            Origin.Parse( cNode );
            originParsed = true;
        }
        else if( cNodeName == wxT( "COMP" ) )
        {
            ComponentRef.Parse( cNode );
            IsComponent = true;
        }
        else if( cNodeName == wxT( "PARTREF" ) )
        {
            PartRef.Parse( cNode );
            HasPartRef = true;
        }
        else if( cNodeName == wxT( "PARTNAMENOTVISIBLE" ) )
        {
            PartNameVisible = false;
        }
        else if( cNodeName == wxT( "VSYMMASTER" ) )
        {
            VariantParentSymbolID = GetXmlAttributeIDString( cNode, 0 );
            VariantID             = GetXmlAttributeIDString( cNode, 1 );
        }
        else if( cNodeName == wxT( "GROUPREF" ) )
        {
            GroupID = GetXmlAttributeIDString( cNode, 0 );
        }
        else if( cNodeName == wxT( "REUSEBLOCKREF" ) )
        {
            ReuseBlockRef.Parse( cNode );
        }
        else if( cNodeName == wxT( "SIGNALREFERENCELINK" ) )
        {
            SigRefLink.Parse( cNode );
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
        else if( cNodeName == wxT( "SCALE" ) )
        {
            ScaleRatioNumerator   = GetXmlAttributeIDLong( cNode, 0 );
            ScaleRatioDenominator = GetXmlAttributeIDLong( cNode, 1 );
        }
        else if( cNodeName == wxT( "READABILITY" ) )
        {
            Readability = ParseReadability( cNode );
        }
        else if( cNodeName == wxT( "GATE" ) )
        {
            GateID = GetXmlAttributeIDString( cNode, 0 );
        }
        else if( cNodeName == wxT( "SYMBOLVARIANT" ) )
        {
            IsSymbolVariant = true;
            SymbolVariant.Parse( cNode );
        }
        else if( cNodeName == wxT( "TERMATTR" ) )
        {
            TERMATTR termattr;
            termattr.Parse( cNode );
            TerminalAttributes.insert( std::make_pair( termattr.TerminalID, termattr ) );
        }
        else if( cNodeName == wxT( "SYMPINLABEL" ) )
        {
            SYMPINNAME_LABEL sympinname;
            sympinname.Parse( cNode );
            PinLabels.insert( std::make_pair( sympinname.TerminalID, sympinname ) );
        }
        else if( cNodeName == wxT( "SYMPINNAME" ) )
        {
            SYMPINNAME_LABEL sympinname;
            sympinname.Parse( cNode );
            PinNames.insert( std::make_pair( sympinname.TerminalID, sympinname ) );
        }
        else if( cNodeName == wxT( "PINNUM" ) )
        {
            PIN_NUM pinNum;
            pinNum.Parse( cNode );
            PinNumbers.insert( std::make_pair( pinNum.TerminalID, pinNum ) );
        }
        else if( cNodeName == wxT( "ATTR" ) )
        {
            ATTRIBUTE_VALUE attrVal;
            attrVal.Parse( cNode );
            AttributeValues.insert( std::make_pair( attrVal.AttributeID, attrVal ) );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, location );
        }
    }

    if( !originParsed )
        THROW_MISSING_PARAMETER_IO_ERROR( wxT( "PT" ), aNode->GetName() );
}


void CADSTAR_SCH_ARCHIVE_PARSER::SIGLOC::Parse( XNODE* aNode )
{
    wxCHECK( aNode->GetName() == wxT( "SIGLOC" ), );

    TextCodeID = GetXmlAttributeIDString( aNode, 0 );

    //Parse child nodes
    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        if( ParseSubNode( cNode ) )
            continue;
        else
            THROW_UNKNOWN_NODE_IO_ERROR( cNode->GetName(), aNode->GetName() );
    }

    if( !Position.IsFullySpecified() )
        THROW_MISSING_NODE_IO_ERROR( wxT( "PT" ), aNode->GetName() );
}


void CADSTAR_SCH_ARCHIVE_PARSER::BUS::Parse( XNODE* aNode )
{
    wxCHECK( aNode->GetName() == wxT( "BUS" ), );

    ID         = GetXmlAttributeIDString( aNode, 0 );
    LineCodeID = GetXmlAttributeIDString( aNode, 1 );
    LayerID    = GetXmlAttributeIDString( aNode, 2 );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( SHAPE::IsShape( cNode ) )
        {
            Shape.Parse( cNode );
        }
        else if( cNodeName == wxT( "BUSNAME" ) )
        {
            Name = GetXmlAttributeIDString( cNode, 0 );

            XNODE* subNode = cNode->GetChildren();

            if( subNode )
            {
                if( subNode->GetName() == wxT( "SIGLOC" ) )
                {
                    BusLabel.Parse( subNode );
                    HasBusLabel = true;
                }
                else
                {
                    THROW_UNKNOWN_NODE_IO_ERROR( subNode->GetName(), cNode->GetName() );
                }
            }
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, aNode->GetName() );
        }
    }
}


void CADSTAR_SCH_ARCHIVE_PARSER::BLOCK::Parse( XNODE* aNode )
{
    wxCHECK( aNode->GetName() == wxT( "BLOCK" ), );

    ID      = GetXmlAttributeIDString( aNode, 0 );
    LayerID = GetXmlAttributeIDString( aNode, 2 );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( cNodeName == wxT( "CLONE" ) )
        {
            Type = TYPE::CLONE;
        }
        else if( cNodeName == wxT( "PARENT" ) )
        {
            Type         = TYPE::PARENT;
            AssocLayerID = GetXmlAttributeIDString( cNode, 0 );
        }
        else if( cNodeName == wxT( "CHILD" ) )
        {
            Type         = TYPE::CHILD;
            AssocLayerID = GetXmlAttributeIDString( cNode, 0 );
        }
        else if( cNodeName == wxT( "BLOCKNAME" ) )
        {
            Name           = GetXmlAttributeIDString( cNode, 0 );
            XNODE* subNode = cNode->GetChildren();

            if( subNode )
            {
                if( subNode->GetName() == wxT( "ATTRLOC" ) )
                {
                    BlockLabel.Parse( subNode );
                    HasBlockLabel = true;
                }
                else
                {
                    THROW_UNKNOWN_NODE_IO_ERROR( subNode->GetName(), cNode->GetName() );
                }
            }
        }
        else if( cNodeName == wxT( "TERMINAL" ) )
        {
            TERMINAL term;
            term.Parse( cNode );
            Terminals.insert( std::make_pair( term.ID, term ) );
        }
        else if( cNodeName == wxT( "FIGURE" ) )
        {
            FIGURE figure;
            figure.Parse( cNode );
            Figures.insert( std::make_pair( figure.ID, figure ) );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, aNode->GetName() );
        }
    }
}


void CADSTAR_SCH_ARCHIVE_PARSER::NET_SCH::SYM_TERM::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "TERM" ) );

    ID         = GetXmlAttributeIDString( aNode, 0 );
    SymbolID   = GetXmlAttributeIDString( aNode, 1 );
    TerminalID = GetXmlAttributeIDLong( aNode, 2 );


    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( cNodeName == wxT( "SIGLOC" ) )
        {
            NetLabel.Parse( cNode );
            HasNetLabel = true;
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, aNode->GetName() );
        }
    }
}


void CADSTAR_SCH_ARCHIVE_PARSER::NET_SCH::BUS_TERM::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "BUSTERM" ) );

    ID    = GetXmlAttributeIDString( aNode, 0 );
    BusID = GetXmlAttributeIDString( aNode, 1 );


    XNODE* cNode             = aNode->GetChildren();
    bool   firstPointParsed  = false;
    bool   secondPointParsed = false;

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( cNodeName == wxT( "SIGLOC" ) )
        {
            NetLabel.Parse( cNode );
            HasNetLabel = true;
        }
        else if( cNodeName == wxT( "PT" ) )
        {
            if( !firstPointParsed )
            {
                FirstPoint.Parse( cNode );
                firstPointParsed = true;
            }
            else if( !secondPointParsed )
            {
                SecondPoint.Parse( cNode );
                secondPointParsed = true;
            }
            else
            {
                THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, aNode->GetName() );
            }
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, aNode->GetName() );
        }
    }

    if( !firstPointParsed || !secondPointParsed )
        THROW_MISSING_NODE_IO_ERROR( wxT( "PT" ), aNode->GetName() );
}


void CADSTAR_SCH_ARCHIVE_PARSER::NET_SCH::BLOCK_TERM::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "BLOCKTERM" ) );

    ID         = GetXmlAttributeIDString( aNode, 0 );
    BlockID    = GetXmlAttributeIDString( aNode, 1 );
    TerminalID = GetXmlAttributeIDLong( aNode, 2 );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( cNodeName == wxT( "SIGLOC" ) )
        {
            NetLabel.Parse( cNode );
            HasNetLabel = true;
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, aNode->GetName() );
        }
    }
}


void CADSTAR_SCH_ARCHIVE_PARSER::NET_SCH::CONNECTION_SCH::Parse( XNODE* aNode )
{
    ParseIdentifiers( aNode );
    LayerID = GetXmlAttributeIDString( aNode, 3 );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( ParseSubNode( cNode ) )
        {
            continue;
        }
        else if( cNodeName == wxT( "PATH" ) )
        {
            Path = ParseAllChildPoints( cNode, true );
        }
        else if( cNodeName == wxT( "GROUPREF" ) )
        {
            GroupID = GetXmlAttributeIDString( cNode, 0 );
        }
        else if( cNodeName == wxT( "REUSEBLOCKREF" ) )
        {
            ReuseBlockRef.Parse( cNode );
        }
        else if( cNodeName == wxT( "CONLINECODE" ) )
        {
            ConnectionLineCode = GetXmlAttributeIDString( cNode, 0 );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, wxT( "CONN" ) );
        }
    }
}


void CADSTAR_SCH_ARCHIVE_PARSER::NET_SCH::Parse( XNODE* aNode )
{
    ParseIdentifiers( aNode );

    //Parse child nodes
    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( cNodeName == wxT( "JPT" ) )
        {
            JUNCTION_SCH jpt;
            jpt.Parse( cNode );
            Junctions.insert( std::make_pair( jpt.ID, jpt ) );
        }
        else if( ParseSubNode( cNode ) )
        {
            continue;
        }
        else if( cNodeName == wxT( "TERM" ) )
        {
            SYM_TERM pin;
            pin.Parse( cNode );
            Terminals.insert( std::make_pair( pin.ID, pin ) );
        }
        else if( cNodeName == wxT( "BUSTERM" ) )
        {
            BUS_TERM bt;
            bt.Parse( cNode );
            BusTerminals.insert( std::make_pair( bt.ID, bt ) );
        }
        else if( cNodeName == wxT( "BLOCKTERM" ) )
        {
            BLOCK_TERM bt;
            bt.Parse( cNode );
            BlockTerminals.insert( std::make_pair( bt.ID, bt ) );
        }
        else if( cNodeName == wxT( "DANGLER" ) )
        {
            DANGLER dang;
            dang.Parse( cNode );
            Danglers.insert( std::make_pair( dang.ID, dang ) );
        }
        else if( cNodeName == wxT( "CONN" ) )
        {
            CONNECTION_SCH conn;
            conn.Parse( cNode );
            Connections.push_back( conn );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, wxT( "NET" ) );
        }
    }
}


void CADSTAR_SCH_ARCHIVE_PARSER::SCHEMATIC::Parse( XNODE* aNode )
{
    wxCHECK( aNode->GetName() == wxT( "SCHEMATIC" ), );

    XNODE* cNode = aNode->GetChildren();

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( cNodeName == wxT( "GROUP" ) )
        {
            GROUP group;
            group.Parse( cNode );
            Groups.insert( std::make_pair( group.ID, group ) );
        }
        else if( cNodeName == wxT( "REUSEBLOCK" ) )
        {
            REUSEBLOCK reuseblock;
            reuseblock.Parse( cNode );
            ReuseBlocks.insert( std::make_pair( reuseblock.ID, reuseblock ) );
        }
        else if( cNodeName == wxT( "FIGURE" ) )
        {
            FIGURE figure;
            figure.Parse( cNode );
            Figures.insert( std::make_pair( figure.ID, figure ) );
        }
        else if( cNodeName == wxT( "SYMBOL" ) )
        {
            SYMBOL sym;
            sym.Parse( cNode );
            Symbols.insert( std::make_pair( sym.ID, sym ) );
        }
        else if( cNodeName == wxT( "BUS" ) )
        {
            BUS bus;
            bus.Parse( cNode );
            Buses.insert( std::make_pair( bus.ID, bus ) );
        }
        else if( cNodeName == wxT( "BLOCK" ) )
        {
            BLOCK block;
            block.Parse( cNode );
            Blocks.insert( std::make_pair( block.ID, block ) );
        }
        else if( cNodeName == wxT( "NET" ) )
        {
            NET_SCH net;
            net.Parse( cNode );
            Nets.insert( std::make_pair( net.ID, net ) );
        }
        else if( cNodeName == wxT( "TEXT" ) )
        {
            TEXT txt;
            txt.Parse( cNode );
            Texts.insert( std::make_pair( txt.ID, txt ) );
        }
        else if( cNodeName == wxT( "DOCSYMBOL" ) )
        {
            DOCUMENTATION_SYMBOL docsym;
            docsym.Parse( cNode );
            DocumentationSymbols.insert( std::make_pair( docsym.ID, docsym ) );
        }
        else if( cNodeName == wxT( "VHIERARCHY" ) )
        {
            VariantHierarchy.Parse( cNode );
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, aNode->GetName() );
        }
    }
}


void CADSTAR_SCH_ARCHIVE_PARSER::NET_SCH::JUNCTION_SCH::Parse( XNODE* aNode )
{
    CADSTAR_ARCHIVE_PARSER::NET::JUNCTION::Parse( aNode );

    TerminalCodeID = GetXmlAttributeIDString( aNode, 1 );
    LayerID        = GetXmlAttributeIDString( aNode, 2 );
}


void CADSTAR_SCH_ARCHIVE_PARSER::NET_SCH::DANGLER::Parse( XNODE* aNode )
{
    wxASSERT( aNode->GetName() == wxT( "DANGLER" ) );

    ID             = GetXmlAttributeIDString( aNode, 0 );
    TerminalCodeID = GetXmlAttributeIDString( aNode, 1 );
    LayerID        = GetXmlAttributeIDString( aNode, 2 );

    XNODE* cNode          = aNode->GetChildren();
    bool   positionParsed = false;

    for( ; cNode; cNode = cNode->GetNext() )
    {
        wxString cNodeName = cNode->GetName();

        if( cNodeName == wxT( "SIGLOC" ) )
        {
            NetLabel.Parse( cNode );
            HasNetLabel = true;
        }
        else if( !positionParsed && cNodeName == wxT( "PT" ) )
        {
            Position.Parse( cNode );
            positionParsed = true;
        }
        else
        {
            THROW_UNKNOWN_NODE_IO_ERROR( cNodeName, aNode->GetName() );
        }
    }
}
