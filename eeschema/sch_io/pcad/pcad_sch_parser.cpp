/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007, 2008 Lubo Racko <developer@lura.sk>
 * Copyright (C) 2012-2013 Alexander Lunev <al.lunev@yahoo.com>
 * Copyright (C) 2017 Eldar Khayrullin <eldar.khayrullin@mail.ru>
 * Copyright (C) 2025 KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <sch_io/pcad/pcad_sch_parser.h>

#include <io/pcad/s_expr_loader.h>
#include <ki_exception.h>
#include <xnode.h>

#include <wx/xml/xml.h>
#include <wx/string.h>
#include <wx/tokenzr.h>


namespace PCAD_SCH
{

XNODE* PCAD_SCH_PARSER::FindChild( XNODE* aNode, const wxString& aTag )
{
    if( !aNode )
        return nullptr;

    for( XNODE* child = aNode->GetChildren(); child; child = child->GetNext() )
    {
        if( child->GetName() == aTag )
            return child;
    }

    return nullptr;
}


// A node's value can land in two places depending on how the writer quoted it:
// quoted strings are concatenated into the "Name" attribute, unquoted tokens
// into the node text content.
wxString PCAD_SCH_PARSER::NodeText( XNODE* aNode )
{
    if( !aNode )
        return wxEmptyString;

    wxString name = aNode->GetAttribute( wxT( "Name" ) );

    if( !name.IsEmpty() )
        return name;

    return aNode->GetNodeContent().Trim( true ).Trim( false );
}


wxString PCAD_SCH_PARSER::childStr( XNODE* aNode, const wxString& aTag,
                                    const wxString& aDefault )
{
    XNODE* child = FindChild( aNode, aTag );

    if( !child )
        return aDefault;

    return NodeText( child );
}


bool PCAD_SCH_PARSER::childFlag( XNODE* aNode, const wxString& aTag )
{
    return childStr( aNode, aTag ).CmpNoCase( wxT( "True" ) ) == 0;
}


// ---------------------------------------------------------------------------
// Measurement handling.  Values are stored as mils.  A value token may carry
// its unit attached ("31.115mm") or as the following token ("0.19843 mm");
// bare numbers use the file default from (fileUnits ...).
// ---------------------------------------------------------------------------

double PCAD_SCH_PARSER::toMils( const wxString& aValue ) const
{
    wxString str = aValue;
    str.Trim( true ).Trim( false );

    if( str.IsEmpty() )
        return 0.0;

    bool isMm = m_isMetric;

    if( str.EndsWith( wxT( "mm" ) ) )
    {
        isMm = true;
        str.RemoveLast( 2 );
    }
    else if( str.EndsWith( wxT( "mil" ) ) || str.EndsWith( wxT( "Mil" ) ) )
    {
        isMm = false;
        str.RemoveLast( 3 );
    }

    str.Trim( true );

    double val = 0.0;
    str.ToCDouble( &val );

    if( isMm )
        val /= 0.0254;

    return val;
}


// Split a node value into measurement tokens, re-attaching separated unit
// suffixes ("0.19843 mm" is one value).
static std::vector<wxString> splitMeasureTokens( const wxString& aContent )
{
    std::vector<wxString>  result;
    wxStringTokenizer      tokenizer( aContent, wxT( " \t\r\n" ), wxTOKEN_STRTOK );

    while( tokenizer.HasMoreTokens() )
    {
        wxString tok = tokenizer.GetNextToken();

        if( !result.empty() && ( tok == wxT( "mm" ) || tok.CmpNoCase( wxT( "mil" ) ) == 0 ) )
            result.back() += tok;
        else
            result.push_back( tok );
    }

    return result;
}


bool PCAD_SCH_PARSER::parsePtNode( XNODE* aPtNode, double& aX, double& aY ) const
{
    if( !aPtNode )
        return false;

    std::vector<wxString> tokens = splitMeasureTokens( NodeText( aPtNode ) );

    if( tokens.size() < 2 )
        return false;

    aX = toMils( tokens[0] );
    aY = toMils( tokens[1] );
    return true;
}


bool PCAD_SCH_PARSER::parsePt( XNODE* aNode, double& aX, double& aY ) const
{
    return parsePtNode( FindChild( aNode, wxT( "pt" ) ), aX, aY );
}


double PCAD_SCH_PARSER::childDouble( XNODE* aNode, const wxString& aTag, double aDefault ) const
{
    XNODE* child = FindChild( aNode, aTag );

    if( !child )
        return aDefault;

    std::vector<wxString> tokens = splitMeasureTokens( NodeText( child ) );

    if( tokens.empty() )
        return aDefault;

    return toMils( tokens[0] );
}


// Rotations and angles are plain decimal degrees, never measurements.
static double childAngle( XNODE* aNode, const wxString& aTag, double aDefault = 0.0 )
{
    XNODE* child = PCAD_SCH_PARSER::FindChild( aNode, aTag );

    if( !child )
        return aDefault;

    double val = aDefault;
    PCAD_SCH_PARSER::NodeText( child ).ToCDouble( &val );
    return val;
}


JUSTIFY PCAD_SCH_PARSER::parseJustify( const wxString& aValue )
{
    if( aValue == wxT( "LowerLeft" ) )   return JUSTIFY::LOWER_LEFT;
    if( aValue == wxT( "LowerCenter" ) ) return JUSTIFY::LOWER_CENTER;
    if( aValue == wxT( "LowerRight" ) )  return JUSTIFY::LOWER_RIGHT;
    if( aValue == wxT( "UpperLeft" ) )   return JUSTIFY::UPPER_LEFT;
    if( aValue == wxT( "UpperCenter" ) ) return JUSTIFY::UPPER_CENTER;
    if( aValue == wxT( "UpperRight" ) )  return JUSTIFY::UPPER_RIGHT;
    if( aValue == wxT( "Center" ) )      return JUSTIFY::CENTER;
    if( aValue == wxT( "Right" ) )       return JUSTIFY::RIGHT;
    if( aValue == wxT( "Left" ) )        return JUSTIFY::LEFT;

    return JUSTIFY::LOWER_LEFT;
}


// ---------------------------------------------------------------------------
// Top level
// ---------------------------------------------------------------------------

void PCAD_SCH_PARSER::LoadFromFile( const wxString& aFilename, SCHEMATIC& aSchematic )
{
    wxXmlDocument doc;
    PCAD2KICAD::LoadInputFile( aFilename, &doc );

    XNODE* root = static_cast<XNODE*>( doc.GetRoot() );

    if( !root )
        THROW_IO_ERROR( _( "Empty P-CAD document" ) );

    // fileUnits must be known before any measurement is parsed.
    if( XNODE* header = FindChild( root, wxT( "asciiHeader" ) ) )
        parseHeader( header, aSchematic );

    m_isMetric = aSchematic.isMetric;

    for( XNODE* node = root->GetChildren(); node; node = node->GetNext() )
    {
        const wxString& tag = node->GetName();

        if( tag == wxT( "library" ) )
            parseLibrary( node, aSchematic );
        else if( tag == wxT( "netlist" ) )
            parseNetlist( node, aSchematic );
        else if( tag == wxT( "schematicDesign" ) )
            parseSchematicDesign( node, aSchematic );
    }

    for( const TEXT_STYLE& ts : aSchematic.textStyles )
        aSchematic.textStylesByName[ts.name] = &ts;

    for( const SYMBOL_DEF& sd : aSchematic.symbolDefs )
        aSchematic.symbolDefsByName[sd.name] = &sd;

    for( const COMP_DEF& cd : aSchematic.compDefs )
        aSchematic.compDefsByName[cd.name] = &cd;

    for( const COMP_INST& ci : aSchematic.compInsts )
        aSchematic.compInstsByRef[ci.refDes] = &ci;
}


void PCAD_SCH_PARSER::parseHeader( XNODE* aNode, SCHEMATIC& aSchematic )
{
    wxString units = childStr( aNode, wxT( "fileUnits" ) );

    aSchematic.isMetric = ( units.CmpNoCase( wxT( "mm" ) ) == 0 );
}


// ---------------------------------------------------------------------------
// (library "name" ...)
// ---------------------------------------------------------------------------

void PCAD_SCH_PARSER::parseLibrary( XNODE* aNode, SCHEMATIC& aSchematic )
{
    for( XNODE* child = aNode->GetChildren(); child; child = child->GetNext() )
    {
        const wxString& tag = child->GetName();

        if( tag == wxT( "textStyleDef" ) )
        {
            parseTextStyleDef( child, aSchematic );
        }
        else if( tag == wxT( "symbolDef" ) )
        {
            SYMBOL_DEF sd;
            parseSymbolDef( child, sd );
            aSchematic.symbolDefs.push_back( std::move( sd ) );
        }
        else if( tag == wxT( "compDef" ) )
        {
            COMP_DEF cd;
            parseCompDef( child, cd );
            aSchematic.compDefs.push_back( std::move( cd ) );
        }
        else if( tag == wxT( "compAlias" ) )
        {
            // (compAlias "ALIAS ORIGINAL") - first word aliases the rest
            wxString both = child->GetAttribute( wxT( "Name" ) );
            wxString alias = both.BeforeFirst( ' ' );
            wxString original = both.AfterFirst( ' ' );
            original.Trim( true ).Trim( false );

            if( !alias.IsEmpty() && !original.IsEmpty() )
                aSchematic.compAliases[alias] = original;
        }
    }
}


void PCAD_SCH_PARSER::parseTextStyleDef( XNODE* aNode, SCHEMATIC& aSchematic )
{
    TEXT_STYLE style;
    style.name = aNode->GetAttribute( wxT( "Name" ) );
    style.displayTType = childFlag( aNode, wxT( "textStyleDisplayTType" ) );

    for( XNODE* child = aNode->GetChildren(); child; child = child->GetNext() )
    {
        if( child->GetName() != wxT( "font" ) )
            continue;

        FONT font;
        font.isTrueType = ( childStr( child, wxT( "fontType" ) ) == wxT( "TrueType" ) );
        font.height = childDouble( child, wxT( "fontHeight" ), 100.0 );
        font.strokeWidth = childDouble( child, wxT( "strokeWidth" ), 10.0 );
        font.isItalic = childFlag( child, wxT( "fontItalic" ) );

        wxString weight = childStr( child, wxT( "fontWeight" ) );
        long     weightVal = 0;

        if( weight.ToLong( &weightVal ) )
            font.isBold = ( weightVal >= 700 );

        if( font.isTrueType )
        {
            style.ttfFont = font;
            style.hasTtfFont = true;
        }
        else
        {
            style.strokeFont = font;
        }
    }

    aSchematic.textStyles.push_back( std::move( style ) );
}


// ---------------------------------------------------------------------------
// (symbolDef "name" ...)
// ---------------------------------------------------------------------------

void PCAD_SCH_PARSER::parseSymbolDef( XNODE* aNode, SYMBOL_DEF& aSymDef )
{
    aSymDef.name = aNode->GetAttribute( wxT( "Name" ) );
    aSymDef.originalName = childStr( aNode, wxT( "originalName" ) );

    for( XNODE* child = aNode->GetChildren(); child; child = child->GetNext() )
    {
        const wxString& tag = child->GetName();

        if( tag == wxT( "pin" ) )
            aSymDef.pins.push_back( parsePin( child ) );
        else if( tag == wxT( "line" ) )
            aSymDef.lines.push_back( parseLine( child ) );
        else if( tag == wxT( "arc" ) )
            aSymDef.arcs.push_back( parseArc( child ) );
        else if( tag == wxT( "triplePointArc" ) )
            aSymDef.arcs.push_back( parseTriplePointArc( child ) );
        else if( tag == wxT( "poly" ) )
            aSymDef.polys.push_back( parsePoly( child ) );
        else if( tag == wxT( "text" ) )
            aSymDef.texts.push_back( parseText( child ) );
        else if( tag == wxT( "ieeeSymbol" ) )
            aSymDef.ieeeSymbols.push_back( parseIeeeSymbol( child ) );
        else if( tag == wxT( "attr" ) )
            aSymDef.attrs.push_back( parseAttr( child ) );
    }
}


// ---------------------------------------------------------------------------
// (compDef "name" (compHeader ...) (compPin ...)... (attachedSymbol ...)...)
// ---------------------------------------------------------------------------

void PCAD_SCH_PARSER::parseCompDef( XNODE* aNode, COMP_DEF& aCompDef )
{
    aCompDef.name = aNode->GetAttribute( wxT( "Name" ) );
    aCompDef.originalName = childStr( aNode, wxT( "originalName" ) );

    if( XNODE* header = FindChild( aNode, wxT( "compHeader" ) ) )
    {
        aCompDef.refDesPrefix = childStr( header, wxT( "refDesPrefix" ) );

        long num = 1;

        if( childStr( header, wxT( "numParts" ) ).ToLong( &num ) && num > 0 )
            aCompDef.numParts = static_cast<int>( num );

        aCompDef.isPower = ( childStr( header, wxT( "compType" ) ) == wxT( "Power" ) );
    }

    aCompDef.attachedSymbols.resize( aCompDef.numParts + 1 );

    for( XNODE* child = aNode->GetChildren(); child; child = child->GetNext() )
    {
        const wxString& tag = child->GetName();

        if( tag == wxT( "compPin" ) )
        {
            COMP_PIN pin;
            pin.padDes = child->GetAttribute( wxT( "Name" ) );
            pin.pinName = childStr( child, wxT( "pinName" ) );
            pin.symPinNum = childStr( child, wxT( "symPinNum" ) );
            pin.pinType = childStr( child, wxT( "pinType" ) );

            long part = 1;

            if( childStr( child, wxT( "partNum" ) ).ToLong( &part ) && part > 0 )
                pin.partNum = static_cast<int>( part );

            aCompDef.compPins.push_back( std::move( pin ) );
        }
        else if( tag == wxT( "attachedSymbol" ) )
        {
            // Only the Normal alternate maps to the base KiCad body style.
            if( childStr( child, wxT( "altType" ) ) != wxT( "Normal" ) )
                continue;

            long part = 1;
            childStr( child, wxT( "partNum" ) ).ToLong( &part );

            wxString symName = childStr( child, wxT( "symbolName" ) );

            if( part >= 1 && !symName.IsEmpty() )
            {
                if( part >= static_cast<long>( aCompDef.attachedSymbols.size() ) )
                    aCompDef.attachedSymbols.resize( part + 1 );

                aCompDef.attachedSymbols[part] = symName;
            }
        }
        else if( tag == wxT( "attachedPattern" ) )
        {
            aCompDef.attachedPattern = childStr( child, wxT( "patternName" ) );
        }
        else if( tag == wxT( "attr" ) )
        {
            // (attr "Description <text>")
            wxString name = child->GetAttribute( wxT( "Name" ) );

            if( name.StartsWith( wxT( "Description " ) ) )
                aCompDef.description = name.AfterFirst( ' ' ).Trim( true ).Trim( false );
        }
    }
}


// ---------------------------------------------------------------------------
// (pin (pinNum N) (pt X Y) (rotation R) [(isFlipped True)] [(pinLength L)]
//      [(outsideEdgeStyle Dot)] [(pinDisplay ...)] (pinDes (text ...))
//      (pinName (text ...)) [(defaultPinDes "D")])
// ---------------------------------------------------------------------------

PIN PCAD_SCH_PARSER::parsePin( XNODE* aNode )
{
    PIN pin;

    pin.pinNum = childStr( aNode, wxT( "pinNum" ) );
    pin.defaultPinDes = childStr( aNode, wxT( "defaultPinDes" ) );
    parsePt( aNode, pin.x, pin.y );
    pin.rotation = childAngle( aNode, wxT( "rotation" ) );
    pin.isFlipped = childFlag( aNode, wxT( "isFlipped" ) );
    pin.pinLength = childDouble( aNode, wxT( "pinLength" ), 300.0 );
    pin.outsideEdgeStyle = childStr( aNode, wxT( "outsideEdgeStyle" ) );
    pin.insideEdgeStyle = childStr( aNode, wxT( "insideEdgeStyle" ) );

    if( XNODE* disp = FindChild( aNode, wxT( "pinDisplay" ) ) )
    {
        wxString des = childStr( disp, wxT( "dispPinDes" ) );

        if( !des.IsEmpty() )
            pin.showPinDes = ( des.CmpNoCase( wxT( "True" ) ) == 0 );

        wxString name = childStr( disp, wxT( "dispPinName" ) );

        if( !name.IsEmpty() )
            pin.showPinName = ( name.CmpNoCase( wxT( "True" ) ) == 0 );
    }

    if( XNODE* pinDes = FindChild( aNode, wxT( "pinDes" ) ) )
    {
        if( XNODE* textNode = FindChild( pinDes, wxT( "text" ) ) )
            pin.pinDesText = parseText( textNode );
    }

    if( XNODE* pinName = FindChild( aNode, wxT( "pinName" ) ) )
    {
        if( XNODE* textNode = FindChild( pinName, wxT( "text" ) ) )
            pin.pinNameText = parseText( textNode );
    }

    return pin;
}


// ---------------------------------------------------------------------------
// (line (pt X1 Y1) (pt X2 Y2)... [(width W)] [(style DashedLine)])
// ---------------------------------------------------------------------------

LINE PCAD_SCH_PARSER::parseLine( XNODE* aNode )
{
    LINE line;

    for( XNODE* child = aNode->GetChildren(); child; child = child->GetNext() )
    {
        if( child->GetName() == wxT( "pt" ) )
        {
            double x = 0, y = 0;

            if( parsePtNode( child, x, y ) )
                line.pts.emplace_back( x, y );
        }
    }

    line.width = childDouble( aNode, wxT( "width" ), 10.0 );

    wxString style = childStr( aNode, wxT( "style" ) );

    if( style == wxT( "DashedLine" ) )
        line.style = LINE_KIND::DASHED;
    else if( style == wxT( "DottedLine" ) )
        line.style = LINE_KIND::DOTTED;

    return line;
}


// ---------------------------------------------------------------------------
// (arc (pt CX CY) (radius R) (startAngle A) (sweepAngle S) [(width W)])
// ---------------------------------------------------------------------------

ARC PCAD_SCH_PARSER::parseArc( XNODE* aNode )
{
    ARC arc;

    parsePt( aNode, arc.x, arc.y );
    arc.radius = childDouble( aNode, wxT( "radius" ) );
    arc.startAngle = childAngle( aNode, wxT( "startAngle" ) );
    arc.sweepAngle = childAngle( aNode, wxT( "sweepAngle" ) );
    arc.width = childDouble( aNode, wxT( "width" ), 10.0 );

    return arc;
}


// ---------------------------------------------------------------------------
// (triplePointArc (pt CX CY) (pt X1 Y1) (pt X2 Y2) [(width W)])
// Three points: center, start and end.  Equal start/end means a full circle.
// ---------------------------------------------------------------------------

ARC PCAD_SCH_PARSER::parseTriplePointArc( XNODE* aNode )
{
    ARC arc;

    std::vector<std::pair<double, double>> pts;

    for( XNODE* child = aNode->GetChildren(); child; child = child->GetNext() )
    {
        if( child->GetName() == wxT( "pt" ) )
        {
            double x = 0, y = 0;

            if( parsePtNode( child, x, y ) )
                pts.emplace_back( x, y );
        }
    }

    arc.width = childDouble( aNode, wxT( "width" ), 10.0 );

    if( pts.size() < 3 )
        return arc;

    arc.x = pts[0].first;
    arc.y = pts[0].second;

    double dx1 = pts[1].first - arc.x;
    double dy1 = pts[1].second - arc.y;
    double dx2 = pts[2].first - arc.x;
    double dy2 = pts[2].second - arc.y;

    arc.radius = std::sqrt( dx1 * dx1 + dy1 * dy1 );
    arc.startAngle = atan2( dy1, dx1 ) * 180.0 / M_PI;

    if( pts[1] == pts[2] )
    {
        arc.sweepAngle = 360.0;
    }
    else
    {
        double endAngle = atan2( dy2, dx2 ) * 180.0 / M_PI;
        arc.sweepAngle = endAngle - arc.startAngle;

        // P-CAD arcs sweep counterclockwise from start to end
        if( arc.sweepAngle <= 0 )
            arc.sweepAngle += 360.0;
    }

    return arc;
}


POLY PCAD_SCH_PARSER::parsePoly( XNODE* aNode )
{
    POLY poly;

    for( XNODE* child = aNode->GetChildren(); child; child = child->GetNext() )
    {
        if( child->GetName() == wxT( "pt" ) )
        {
            double x = 0, y = 0;

            if( parsePtNode( child, x, y ) )
                poly.pts.emplace_back( x, y );
        }
    }

    return poly;
}


// ---------------------------------------------------------------------------
// (text (pt X Y) "string" (textStyleRef "style") [(rotation R)]
//       [(isFlipped True)] [(justify J)] [(isVisible ...)])
// ---------------------------------------------------------------------------

TEXT_ITEM PCAD_SCH_PARSER::parseText( XNODE* aNode )
{
    TEXT_ITEM item;

    item.text = aNode->GetAttribute( wxT( "Name" ) );
    parsePt( aNode, item.x, item.y );
    item.rotation = childAngle( aNode, wxT( "rotation" ) );
    item.isFlipped = childFlag( aNode, wxT( "isFlipped" ) );
    item.justify = parseJustify( childStr( aNode, wxT( "justify" ) ) );
    item.styleRef = childStr( aNode, wxT( "textStyleRef" ) );

    wxString visible = childStr( aNode, wxT( "isVisible" ) );

    if( !visible.IsEmpty() )
        item.isVisible = ( visible.CmpNoCase( wxT( "True" ) ) == 0 );

    return item;
}


IEEE_SYMBOL PCAD_SCH_PARSER::parseIeeeSymbol( XNODE* aNode )
{
    IEEE_SYMBOL sym;

    wxString kind = aNode->GetNodeContent().Trim( true ).Trim( false );

    if( kind.IsEmpty() )
        kind = aNode->GetAttribute( wxT( "Name" ) );

    if( kind == wxT( "Adder" ) )           sym.kind = IEEE_KIND::ADDER;
    else if( kind == wxT( "Amplifier" ) )  sym.kind = IEEE_KIND::AMPLIFIER;
    else if( kind == wxT( "Astable" ) )    sym.kind = IEEE_KIND::ASTABLE;
    else if( kind == wxT( "Complex" ) )    sym.kind = IEEE_KIND::COMPLEX;
    else if( kind == wxT( "Generator" ) )  sym.kind = IEEE_KIND::GENERATOR;
    else if( kind == wxT( "Hysteresis" ) ) sym.kind = IEEE_KIND::HYSTERESIS;
    else if( kind == wxT( "Multiplier" ) ) sym.kind = IEEE_KIND::MULTIPLIER;

    parsePt( aNode, sym.x, sym.y );
    sym.height = childDouble( aNode, wxT( "height" ) );
    sym.rotation = childAngle( aNode, wxT( "rotation" ) );
    sym.isFlipped = childFlag( aNode, wxT( "isFlipped" ) );

    return sym;
}


// ---------------------------------------------------------------------------
// (attr "Name Value" (pt X Y) (isVisible ...) (justify ...) (rotation ...)
//       (textStyleRef "..."))
// ---------------------------------------------------------------------------

ATTR PCAD_SCH_PARSER::parseAttr( XNODE* aNode )
{
    ATTR attr;

    wxString nameAttr = aNode->GetAttribute( wxT( "Name" ) );

    // The quoted attribute name and quoted value are concatenated by the
    // loader; the name is the first word, the value everything after it.
    attr.name = nameAttr.BeforeFirst( ' ' );
    attr.value = nameAttr.AfterFirst( ' ' );
    attr.value.Trim( true ).Trim( false );

    attr.placement = parseText( aNode );
    attr.placement.text = attr.value;

    return attr;
}


// ---------------------------------------------------------------------------
// (netlist "name" (compInst "refDes" ...) ...)
// ---------------------------------------------------------------------------

void PCAD_SCH_PARSER::parseNetlist( XNODE* aNode, SCHEMATIC& aSchematic )
{
    for( XNODE* child = aNode->GetChildren(); child; child = child->GetNext() )
    {
        if( child->GetName() != wxT( "compInst" ) )
            continue;

        COMP_INST ci;
        ci.refDes = child->GetAttribute( wxT( "Name" ) );
        ci.compRef = childStr( child, wxT( "compRef" ) );
        ci.originalName = childStr( child, wxT( "originalName" ) );
        ci.value = childStr( child, wxT( "compValue" ) );

        aSchematic.compInsts.push_back( std::move( ci ) );
    }
}


// ---------------------------------------------------------------------------
// (schematicDesign "name" (schDesignHeader ...) (titleSheet ...) (sheet ...)...)
// ---------------------------------------------------------------------------

void PCAD_SCH_PARSER::parseSchematicDesign( XNODE* aNode, SCHEMATIC& aSchematic )
{
    if( XNODE* header = FindChild( aNode, wxT( "schDesignHeader" ) ) )
    {
        if( XNODE* wsNode = FindChild( header, wxT( "workspaceSize" ) ) )
        {
            double w = 0, h = 0;

            if( parsePtNode( wsNode, w, h ) )
            {
                if( w > 0 )
                    aSchematic.workspaceWidth = w;

                if( h > 0 )
                    aSchematic.workspaceHeight = h;
            }
        }

        if( XNODE* titleSheet = FindChild( header, wxT( "titleSheet" ) ) )
            parseTitleSheet( titleSheet, aSchematic );
    }

    for( XNODE* child = aNode->GetChildren(); child; child = child->GetNext() )
    {
        if( child->GetName() == wxT( "sheet" ) )
        {
            SHEET sheet;
            sheet.name = child->GetAttribute( wxT( "Name" ) );

            long num = static_cast<long>( aSchematic.sheets.size() ) + 1;
            childStr( child, wxT( "sheetNum" ) ).ToLong( &num );
            sheet.sheetNum = static_cast<int>( num );

            parseSheet( child, sheet );
            aSchematic.sheets.push_back( std::move( sheet ) );
        }
    }
}


// ---------------------------------------------------------------------------
// (titleSheet "name" scale ... (fieldSetRef ...))  Field values come from the
// design-level (fieldSet (fieldDef "Name" "Value") ...) definitions.
// ---------------------------------------------------------------------------

void PCAD_SCH_PARSER::parseTitleSheet( XNODE* aNode, SCHEMATIC& aSchematic )
{
    // fieldDef name/value pairs live under the titleSheet's design node siblings
    // (fieldSet ...); walk up to the design header parent and scan.
    XNODE* design = aNode->GetParent();

    while( design && design->GetName() != wxT( "schematicDesign" ) )
        design = design->GetParent();

    if( !design )
        return;

    for( XNODE* child = design->GetChildren(); child; child = child->GetNext() )
    {
        if( child->GetName() != wxT( "fieldSet" ) )
            continue;

        for( XNODE* field = child->GetChildren(); field; field = field->GetNext() )
        {
            if( field->GetName() != wxT( "fieldDef" ) )
                continue;

            // (fieldDef "Name" "Value") - loader concatenates to "Name Value"
            wxString both = field->GetAttribute( wxT( "Name" ) );
            wxString name = both.BeforeFirst( ' ' );
            wxString value = both.AfterFirst( ' ' );
            value.Trim( true ).Trim( false );

            if( !name.IsEmpty() && !value.IsEmpty() )
                aSchematic.titleSheet.fields[name] = value;
        }
    }
}


// ---------------------------------------------------------------------------
// (wire (line (pt ...) (pt ...) [(endStyle ...)] (width W) (netNameRef "N"))
//       [(dispName True)] [(text ...)])
// ---------------------------------------------------------------------------

WIRE PCAD_SCH_PARSER::parseWire( XNODE* aNode )
{
    WIRE wire;

    if( XNODE* lineNode = FindChild( aNode, wxT( "line" ) ) )
    {
        for( XNODE* lc = lineNode->GetChildren(); lc; lc = lc->GetNext() )
        {
            if( lc->GetName() == wxT( "pt" ) )
            {
                double x = 0, y = 0;

                if( parsePtNode( lc, x, y ) )
                    wire.pts.emplace_back( x, y );
            }
        }

        wire.width = childDouble( lineNode, wxT( "width" ), 10.0 );

        if( XNODE* netRef = FindChild( lineNode, wxT( "netNameRef" ) ) )
            wire.netName = NodeText( netRef );
    }

    wire.dispName = childFlag( aNode, wxT( "dispName" ) );

    if( XNODE* textNode = FindChild( aNode, wxT( "text" ) ) )
    {
        wire.label = parseText( textNode );

        if( wire.label.text.IsEmpty() )
            wire.label.text = wire.netName;
    }

    return wire;
}


// ---------------------------------------------------------------------------
// (bus "name" (pt ...) (pt ...) [(dispName True)] [(text ...)])
// ---------------------------------------------------------------------------

BUS PCAD_SCH_PARSER::parseBus( XNODE* aNode )
{
    BUS bus;

    bus.name = aNode->GetAttribute( wxT( "Name" ) );

    for( XNODE* child = aNode->GetChildren(); child; child = child->GetNext() )
    {
        if( child->GetName() == wxT( "pt" ) )
        {
            double x = 0, y = 0;

            if( parsePtNode( child, x, y ) )
                bus.pts.emplace_back( x, y );
        }
    }

    bus.dispName = childFlag( aNode, wxT( "dispName" ) );

    if( XNODE* textNode = FindChild( aNode, wxT( "text" ) ) )
    {
        bus.label = parseText( textNode );

        if( bus.label.text.IsEmpty() )
            bus.label.text = bus.name;
    }

    return bus;
}


// ---------------------------------------------------------------------------
// (sheet "name" (sheetNum N) ...)
// ---------------------------------------------------------------------------

void PCAD_SCH_PARSER::parseSheet( XNODE* aNode, SHEET& aSheet )
{
    for( XNODE* child = aNode->GetChildren(); child; child = child->GetNext() )
    {
        const wxString& tag = child->GetName();

        if( tag == wxT( "wire" ) )
        {
            WIRE wire = parseWire( child );

            if( wire.pts.size() >= 2 )
                aSheet.wires.push_back( std::move( wire ) );
        }
        else if( tag == wxT( "bus" ) )
        {
            BUS bus = parseBus( child );

            if( bus.pts.size() >= 2 )
                aSheet.buses.push_back( std::move( bus ) );
        }
        else if( tag == wxT( "busEntry" ) )
        {
            BUS_ENTRY entry;
            entry.busNameRef = childStr( child, wxT( "busNameRef" ) );
            parsePt( child, entry.x, entry.y );
            entry.orient = childStr( child, wxT( "orient" ) );

            aSheet.busEntries.push_back( std::move( entry ) );
        }
        else if( tag == wxT( "port" ) )
        {
            PORT port;
            parsePt( child, port.x, port.y );

            if( XNODE* netRef = FindChild( child, wxT( "netNameRef" ) ) )
                port.netNameRef = NodeText( netRef );

            port.portType = childStr( child, wxT( "portType" ) );
            port.rotation = childAngle( child, wxT( "rotation" ) );
            port.isFlipped = childFlag( child, wxT( "isFlipped" ) );

            aSheet.ports.push_back( std::move( port ) );
        }
        else if( tag == wxT( "junction" ) )
        {
            JUNCTION junc;
            parsePt( child, junc.x, junc.y );

            if( XNODE* netRef = FindChild( child, wxT( "netNameRef" ) ) )
                junc.netName = NodeText( netRef );

            aSheet.junctions.push_back( std::move( junc ) );
        }
        else if( tag == wxT( "symbol" ) )
        {
            SYMBOL_INST inst;
            inst.symbolRef = childStr( child, wxT( "symbolRef" ) );
            inst.refDesRef = childStr( child, wxT( "refDesRef" ) );

            long part = 1;

            if( childStr( child, wxT( "partNum" ) ).ToLong( &part ) && part > 0 )
                inst.partNum = static_cast<int>( part );

            parsePt( child, inst.x, inst.y );
            inst.rotation = childAngle( child, wxT( "rotation" ) );
            inst.isFlipped = childFlag( child, wxT( "isFlipped" ) );

            for( XNODE* sub = child->GetChildren(); sub; sub = sub->GetNext() )
            {
                if( sub->GetName() == wxT( "attr" ) )
                    inst.attrs.push_back( parseAttr( sub ) );
            }

            aSheet.symbols.push_back( std::move( inst ) );
        }
        else if( tag == wxT( "text" ) )
        {
            aSheet.texts.push_back( parseText( child ) );
        }
        else if( tag == wxT( "line" ) )
        {
            LINE line = parseLine( child );

            if( line.pts.size() >= 2 )
                aSheet.lines.push_back( std::move( line ) );
        }
        else if( tag == wxT( "arc" ) )
        {
            aSheet.arcs.push_back( parseArc( child ) );
        }
        else if( tag == wxT( "triplePointArc" ) )
        {
            aSheet.arcs.push_back( parseTriplePointArc( child ) );
        }
        else if( tag == wxT( "poly" ) )
        {
            POLY poly = parsePoly( child );

            if( poly.pts.size() >= 3 )
                aSheet.polys.push_back( std::move( poly ) );
        }
        else if( tag == wxT( "ieeeSymbol" ) )
        {
            aSheet.ieeeSymbols.push_back( parseIeeeSymbol( child ) );
        }
    }
}

} // namespace PCAD_SCH
