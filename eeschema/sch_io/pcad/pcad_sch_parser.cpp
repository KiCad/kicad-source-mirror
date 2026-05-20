/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include <dsnlexer.h>
#include <xnode.h>
#include <macros.h>

#include <wx/xml/xml.h>
#include <wx/string.h>
#include <wx/wxcrt.h>

#include <stdexcept>
#include <cstring>


namespace PCAD_SCH
{

static const char ACCEL_ASCII_KEYWORD[] = "ACCEL_ASCII";


// ---------------------------------------------------------------------------
// File loading: P-Cad ASCII → wxXmlDocument using DSNLEXER (same as PCB side)
// ---------------------------------------------------------------------------

static void loadXml( const wxString& aFilename, wxXmlDocument& aDoc )
{
    char line[sizeof( ACCEL_ASCII_KEYWORD )];

    FILE* fp = wxFopen( aFilename, wxT( "rt" ) );

    if( !fp )
        THROW_IO_ERROR( wxString::Format( _( "Cannot open file '%s'" ), aFilename ) );

    if( !fgets( line, sizeof( line ), fp )
        || memcmp( line, ACCEL_ASCII_KEYWORD, sizeof( ACCEL_ASCII_KEYWORD ) - 1 ) != 0 )
    {
        fclose( fp );
        THROW_IO_ERROR( wxString::Format( _( "'%s' is not a P-CAD ASCII file" ), aFilename ) );
    }

    fseek( fp, 0, SEEK_SET );

    static KEYWORD emptyKeywords[1] = {};
    // DSNLEXER takes ownership of fp and closes it
    DSNLEXER lexer( emptyKeywords, 0, nullptr, fp, aFilename );
    wxCSConv  conv( wxT( "windows-1251" ) );

    XNODE* root = new XNODE( wxXML_ELEMENT_NODE, wxT( "pcad" ) );
    XNODE* cur  = root;

    int tok;

    while( ( tok = lexer.NextTok() ) != DSN_EOF )
    {
        if( tok == DSN_RIGHT )
        {
            cur = cur->GetParent();

            if( !cur )
                THROW_IO_ERROR( _( "Unexpected ')' in P-CAD file" ) );
        }
        else if( tok == DSN_LEFT )
        {
            tok = lexer.NextTok();
            XNODE* child = new XNODE( wxXML_ELEMENT_NODE,
                                      wxString( lexer.CurText(), conv ) );
            cur->AddChild( child );
            cur = child;
        }
        else if( cur != root )
        {
            wxString val( lexer.CurText(), conv );

            if( tok == DSN_STRING )
            {
                wxString existing;

                if( cur->GetAttribute( wxT( "Name" ), &existing ) )
                {
                    cur->DeleteAttribute( wxT( "Name" ) );
                    cur->AddAttribute( wxT( "Name" ), existing + wxT( ' ' ) + val );
                }
                else
                {
                    cur->AddAttribute( wxT( "Name" ), val );
                }
            }
            else if( !val.IsEmpty() )
            {
                wxString content = cur->GetNodeContent() + wxT( ' ' ) + val;
                wxXmlNode* textNode = cur->GetChildren();

                // Find or create text child
                while( textNode && textNode->GetType() != wxXML_TEXT_NODE )
                    textNode = textNode->GetNext();

                if( textNode )
                    textNode->SetContent( content );
                else
                    cur->AddChild( new wxXmlNode( wxXML_TEXT_NODE, wxEmptyString, content ) );
            }
        }
    }

    aDoc.SetRoot( root );
}


// ---------------------------------------------------------------------------
// XML tree helpers
// ---------------------------------------------------------------------------

XNODE* PCAD_SCH_PARSER::findChild( XNODE* aNode, const wxString& aTag )
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


double PCAD_SCH_PARSER::parseDouble( const wxString& aStr )
{
    double val = 0.0;
    aStr.ToCDouble( &val );
    return val;
}


double PCAD_SCH_PARSER::nodeChildDouble( XNODE* aNode, const wxString& aTag, double aDefault )
{
    XNODE* child = findChild( aNode, aTag );

    if( !child )
        return aDefault;

    wxString content = child->GetNodeContent().Trim( true ).Trim( false );

    if( content.IsEmpty() )
    {
        wxString name = child->GetAttribute( wxT( "Name" ) );
        content = name.BeforeFirst( ' ' );
    }

    return parseDouble( content );
}


wxString PCAD_SCH_PARSER::nodeChildStr( XNODE* aNode, const wxString& aTag,
                                        const wxString& aDefault )
{
    XNODE* child = findChild( aNode, aTag );

    if( !child )
        return aDefault;

    wxString name = child->GetAttribute( wxT( "Name" ) );

    if( !name.IsEmpty() )
        return name;

    return child->GetNodeContent().Trim( true ).Trim( false );
}


// ---------------------------------------------------------------------------
// Parse a (pt X Y) child node and return (x, y) in mils
// ---------------------------------------------------------------------------

static bool parsePt( XNODE* aNode, double& aX, double& aY )
{
    XNODE* ptNode = PCAD_SCH_PARSER::findChild( aNode, wxT( "pt" ) );

    if( !ptNode )
        return false;

    wxString content = ptNode->GetNodeContent().Trim( true ).Trim( false );

    // content looks like " 400.0 200.0"
    wxString xs = content.BeforeFirst( ' ' ).Trim( true ).Trim( false );
    wxString ys = content.AfterFirst( ' ' ).Trim( true ).Trim( false );

    if( xs.IsEmpty() )
    {
        // may be in Name attribute: "400.0 200.0"
        wxString name = ptNode->GetAttribute( wxT( "Name" ) );
        xs = name.BeforeFirst( ' ' ).Trim( true ).Trim( false );
        ys = name.AfterFirst( ' ' ).Trim( true ).Trim( false );
    }

    aX = PCAD_SCH_PARSER::parseDouble( xs );
    aY = PCAD_SCH_PARSER::parseDouble( ys );
    return true;
}



// ---------------------------------------------------------------------------
// Top-level parse
// ---------------------------------------------------------------------------

void PCAD_SCH_PARSER::LoadFromFile( const wxString& aFilename, SCHEMATIC& aSchematic )
{
    wxXmlDocument doc;
    loadXml( aFilename, doc );

    XNODE* root = static_cast<XNODE*>( doc.GetRoot() );

    if( !root )
        THROW_IO_ERROR( _( "Empty P-CAD document" ) );

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

    // Build fast-lookup maps
    for( const SYMBOL_DEF& sd : aSchematic.symbolDefs )
        aSchematic.symbolDefsByName[sd.name] = &sd;

    for( const COMP_INST& ci : aSchematic.compInsts )
        aSchematic.compInstsByRef[ci.refDes] = &ci;
}


// ---------------------------------------------------------------------------
// (library "name" ...)
// ---------------------------------------------------------------------------

void PCAD_SCH_PARSER::parseLibrary( XNODE* aNode, SCHEMATIC& aSchematic )
{
    for( XNODE* child = aNode->GetChildren(); child; child = child->GetNext() )
    {
        if( child->GetName() == wxT( "symbolDef" ) )
        {
            SYMBOL_DEF sd;
            parseSymbolDef( child, sd );
            aSchematic.symbolDefs.push_back( std::move( sd ) );
        }
    }
}


// ---------------------------------------------------------------------------
// (symbolDef "name" ...)
// ---------------------------------------------------------------------------

void PCAD_SCH_PARSER::parseSymbolDef( XNODE* aNode, SYMBOL_DEF& aSymDef )
{
    aSymDef.name = aNode->GetAttribute( wxT( "Name" ) );
    aSymDef.originalName = nodeChildStr( aNode, wxT( "originalName" ) );

    for( XNODE* child = aNode->GetChildren(); child; child = child->GetNext() )
    {
        const wxString& tag = child->GetName();

        if( tag == wxT( "pin" ) )
            aSymDef.pins.push_back( parsePin( child ) );
        else if( tag == wxT( "line" ) )
            aSymDef.lines.push_back( parseLine( child ) );
        else if( tag == wxT( "arc" ) )
            aSymDef.arcs.push_back( parseArc( child ) );
        else if( tag == wxT( "attr" ) )
            aSymDef.attrs.push_back( parseAttr( child ) );
    }
}


// ---------------------------------------------------------------------------
// (pin (pinNum N) (pt X Y) (rotation R) [(isFlipped True)] [(pinLength L)] ...)
// ---------------------------------------------------------------------------

PIN PCAD_SCH_PARSER::parsePin( XNODE* aNode )
{
    PIN pin;

    // pinNum → number string
    XNODE* pinNumNode = findChild( aNode, wxT( "pinNum" ) );

    if( pinNumNode )
    {
        wxString content = pinNumNode->GetNodeContent().Trim( true ).Trim( false );

        if( content.IsEmpty() )
            content = pinNumNode->GetAttribute( wxT( "Name" ) );

        pin.number = content;
    }

    // pt → connection endpoint
    parsePt( aNode, pin.x, pin.y );

    // rotation
    XNODE* rotNode = findChild( aNode, wxT( "rotation" ) );

    if( rotNode )
    {
        wxString content = rotNode->GetNodeContent().Trim( true ).Trim( false );

        if( content.IsEmpty() )
            content = rotNode->GetAttribute( wxT( "Name" ) );

        pin.rotation = parseDouble( content );
    }

    // isFlipped
    XNODE* flippedNode = findChild( aNode, wxT( "isFlipped" ) );

    if( flippedNode )
    {
        wxString val = flippedNode->GetNodeContent().Trim( true ).Trim( false );

        if( val.IsEmpty() )
            val = flippedNode->GetAttribute( wxT( "Name" ) );

        pin.isFlipped = ( val.CmpNoCase( wxT( "True" ) ) == 0 );
    }

    // pinLength
    XNODE* lenNode = findChild( aNode, wxT( "pinLength" ) );

    if( lenNode )
    {
        wxString content = lenNode->GetNodeContent().Trim( true ).Trim( false );

        if( content.IsEmpty() )
            content = lenNode->GetAttribute( wxT( "Name" ) );

        if( !content.IsEmpty() )
            pin.pinLength = parseDouble( content );
    }

    // pinDes → pin number display text (fall back if pinNum not set)
    if( pin.number.IsEmpty() )
    {
        XNODE* pinDesNode = findChild( aNode, wxT( "pinDes" ) );

        if( pinDesNode )
        {
            XNODE* textNode = findChild( pinDesNode, wxT( "text" ) );

            if( textNode )
                pin.number = textNode->GetAttribute( wxT( "Name" ) );
        }
    }

    // pinName → pin name
    XNODE* pinNameNode = findChild( aNode, wxT( "pinName" ) );

    if( pinNameNode )
    {
        XNODE* textNode = findChild( pinNameNode, wxT( "text" ) );

        if( textNode )
        {
            wxString nameAttr = textNode->GetAttribute( wxT( "Name" ) );

            if( !nameAttr.IsEmpty() )
                pin.name = nameAttr;
        }
    }

    return pin;
}


// ---------------------------------------------------------------------------
// (line (pt X1 Y1) (pt X2 Y2) ...)
// The format can have inline pts or nested pt children.
// In symbolDef: (line (pt X1 Y1) (pt X2 Y2))
// ---------------------------------------------------------------------------

LINE PCAD_SCH_PARSER::parseLine( XNODE* aNode )
{
    LINE line{};

    // Collect all (pt ...) children
    std::vector<std::pair<double, double>> pts;

    for( XNODE* child = aNode->GetChildren(); child; child = child->GetNext() )
    {
        if( child->GetName() == wxT( "pt" ) )
        {
            wxString content = child->GetNodeContent().Trim( true ).Trim( false );

            if( content.IsEmpty() )
                content = child->GetAttribute( wxT( "Name" ) );

            double x = 0, y = 0;
            wxString xs = content.BeforeFirst( ' ' );
            wxString ys = content.AfterFirst( ' ' ).Trim( true ).Trim( false );
            x = parseDouble( xs );
            y = parseDouble( ys );
            pts.push_back( { x, y } );
        }
    }

    if( pts.size() >= 2 )
    {
        line.x1 = pts[0].first;
        line.y1 = pts[0].second;
        line.x2 = pts[1].first;
        line.y2 = pts[1].second;
    }

    return line;
}


// ---------------------------------------------------------------------------
// (arc ...) — simplified; P-Cad arcs use center/radius/angles
// ---------------------------------------------------------------------------

ARC PCAD_SCH_PARSER::parseArc( XNODE* aNode )
{
    ARC arc{};

    parsePt( aNode, arc.x, arc.y );
    arc.radius = nodeChildDouble( aNode, wxT( "radius" ) );
    arc.startAngle = nodeChildDouble( aNode, wxT( "startAngle" ) );
    arc.sweepAngle = nodeChildDouble( aNode, wxT( "sweepAngle" ) );

    return arc;
}


// ---------------------------------------------------------------------------
// (attr "Name" "Value" (pt X Y) (isVisible True/False) ...)
// ---------------------------------------------------------------------------

ATTR PCAD_SCH_PARSER::parseAttr( XNODE* aNode )
{
    ATTR attr;

    wxString nameAttr = aNode->GetAttribute( wxT( "Name" ) );

    // Name attribute may be "AttrName AttrValue" or just "AttrName"
    attr.name  = nameAttr.BeforeFirst( ' ' );
    attr.value = nameAttr.AfterFirst( ' ' ).Trim( true ).Trim( false );

    parsePt( aNode, attr.x, attr.y );

    XNODE* visNode = findChild( aNode, wxT( "isVisible" ) );

    if( visNode )
    {
        wxString val = visNode->GetNodeContent().Trim( true ).Trim( false );

        if( val.IsEmpty() )
            val = visNode->GetAttribute( wxT( "Name" ) );

        attr.isVisible = ( val.CmpNoCase( wxT( "True" ) ) == 0 );
    }

    XNODE* justNode = findChild( aNode, wxT( "justify" ) );

    if( justNode )
    {
        wxString val = justNode->GetNodeContent().Trim( true ).Trim( false );

        if( val.IsEmpty() )
            val = justNode->GetAttribute( wxT( "Name" ) );

        if(      val == wxT("LowerLeft")   ) attr.justify = JUSTIFY::LOWER_LEFT;
        else if( val == wxT("LowerCenter") ) attr.justify = JUSTIFY::LOWER_CENTER;
        else if( val == wxT("LowerRight")  ) attr.justify = JUSTIFY::LOWER_RIGHT;
        else if( val == wxT("UpperLeft")   ) attr.justify = JUSTIFY::UPPER_LEFT;
        else if( val == wxT("UpperCenter") ) attr.justify = JUSTIFY::UPPER_CENTER;
        else if( val == wxT("UpperRight")  ) attr.justify = JUSTIFY::UPPER_RIGHT;
        else if( val == wxT("Center")      ) attr.justify = JUSTIFY::CENTER;
        else if( val == wxT("Right")       ) attr.justify = JUSTIFY::RIGHT;
        else                                 attr.justify = JUSTIFY::LEFT;
    }

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
        ci.refDes      = child->GetAttribute( wxT( "Name" ) );
        ci.compRef     = nodeChildStr( child, wxT( "compRef" ) );
        ci.originalName = nodeChildStr( child, wxT( "originalName" ) );
        ci.value       = nodeChildStr( child, wxT( "compValue" ) );

        aSchematic.compInsts.push_back( std::move( ci ) );
    }
}


// ---------------------------------------------------------------------------
// (schematicDesign "name" (schDesignHeader ...) (sheet ...) ...)
// ---------------------------------------------------------------------------

void PCAD_SCH_PARSER::parseSchematicDesign( XNODE* aNode, SCHEMATIC& aSchematic )
{
    // Extract workspace size from schDesignHeader
    XNODE* header = findChild( aNode, wxT( "schDesignHeader" ) );

    if( header )
    {
        XNODE* wsNode = findChild( header, wxT( "workspaceSize" ) );

        if( wsNode )
        {
            wxString content = wsNode->GetNodeContent().Trim( true ).Trim( false );

            if( content.IsEmpty() )
                content = wsNode->GetAttribute( wxT( "Name" ) );

            wxString ws = content.BeforeFirst( ' ' );
            wxString hs = content.AfterFirst( ' ' ).Trim( true ).Trim( false );
            double w = parseDouble( ws );
            double h = parseDouble( hs );

            if( w > 0 )
                aSchematic.workspaceWidth = w;

            if( h > 0 )
                aSchematic.workspaceHeight = h;
        }
    }

    for( XNODE* child = aNode->GetChildren(); child; child = child->GetNext() )
    {
        if( child->GetName() == wxT( "sheet" ) )
        {
            SHEET sheet;
            sheet.name = child->GetAttribute( wxT( "Name" ) );

            XNODE* numNode = findChild( child, wxT( "sheetNum" ) );

            if( numNode )
            {
                wxString val = numNode->GetNodeContent().Trim( true ).Trim( false );

                if( val.IsEmpty() )
                    val = numNode->GetAttribute( wxT( "Name" ) );

                long num = 1;
                val.ToLong( &num );
                sheet.sheetNum = static_cast<int>( num );
            }

            parseSheet( child, sheet );
            aSchematic.sheets.push_back( std::move( sheet ) );
        }
    }
}


// ---------------------------------------------------------------------------
// (sheet "name" (sheetNum N) (junction ...) (wire ...) (symbol ...) ...)
// ---------------------------------------------------------------------------

void PCAD_SCH_PARSER::parseSheet( XNODE* aNode, SHEET& aSheet )
{
    for( XNODE* child = aNode->GetChildren(); child; child = child->GetNext() )
    {
        const wxString& tag = child->GetName();

        if( tag == wxT( "wire" ) )
        {
            // (wire (line (pt X1 Y1) (pt X2 Y2) (width W) (netNameRef "name") ...) ...)
            XNODE* lineNode = findChild( child, wxT( "line" ) );

            if( lineNode )
            {
                std::vector<std::pair<double, double>> pts;

                for( XNODE* lc = lineNode->GetChildren(); lc; lc = lc->GetNext() )
                {
                    if( lc->GetName() == wxT( "pt" ) )
                    {
                        wxString content = lc->GetNodeContent().Trim( true ).Trim( false );

                        if( content.IsEmpty() )
                            content = lc->GetAttribute( wxT( "Name" ) );

                        double x = parseDouble( content.BeforeFirst( ' ' ) );
                        double y = parseDouble( content.AfterFirst( ' ' ).Trim( true ).Trim( false ) );
                        pts.push_back( { x, y } );
                    }
                }

                if( pts.size() >= 2 )
                {
                    WIRE wire;
                    wire.x1 = pts[0].first;
                    wire.y1 = pts[0].second;
                    wire.x2 = pts[1].first;
                    wire.y2 = pts[1].second;

                    XNODE* netRef = findChild( lineNode, wxT( "netNameRef" ) );

                    if( netRef )
                    {
                        wire.netName = netRef->GetAttribute( wxT( "Name" ) );

                        if( wire.netName.IsEmpty() )
                            wire.netName = netRef->GetNodeContent().Trim( true ).Trim( false );
                    }

                    aSheet.wires.push_back( std::move( wire ) );
                }
            }
        }
        else if( tag == wxT( "junction" ) )
        {
            JUNCTION junc;
            parsePt( child, junc.x, junc.y );

            XNODE* netRef = findChild( child, wxT( "netNameRef" ) );

            if( netRef )
            {
                junc.netName = netRef->GetAttribute( wxT( "Name" ) );

                if( junc.netName.IsEmpty() )
                    junc.netName = netRef->GetNodeContent().Trim( true ).Trim( false );
            }

            aSheet.junctions.push_back( std::move( junc ) );
        }
        else if( tag == wxT( "symbol" ) )
        {
            // (symbol (symbolRef "RES_1") (refDesRef "R17") (partNum 1) (pt X Y) (rotation R) ...)
            SYMBOL_INST inst;
            inst.symbolRef = nodeChildStr( child, wxT( "symbolRef" ) );
            inst.refDesRef = nodeChildStr( child, wxT( "refDesRef" ) );

            XNODE* partNode = findChild( child, wxT( "partNum" ) );

            if( partNode )
            {
                wxString val = partNode->GetNodeContent().Trim( true ).Trim( false );

                if( val.IsEmpty() )
                    val = partNode->GetAttribute( wxT( "Name" ) );

                long pn = 1;
                val.ToLong( &pn );
                inst.partNum = static_cast<int>( pn );
            }

            parsePt( child, inst.x, inst.y );

            XNODE* rotNode = findChild( child, wxT( "rotation" ) );

            if( rotNode )
            {
                wxString val = rotNode->GetNodeContent().Trim( true ).Trim( false );

                if( val.IsEmpty() )
                    val = rotNode->GetAttribute( wxT( "Name" ) );

                inst.rotation = parseDouble( val );
            }

            XNODE* flipNode = findChild( child, wxT( "isFlipped" ) );

            if( flipNode )
            {
                wxString val = flipNode->GetNodeContent().Trim( true ).Trim( false );

                if( val.IsEmpty() )
                    val = flipNode->GetAttribute( wxT( "Name" ) );

                inst.isFlipped = ( val.CmpNoCase( wxT( "True" ) ) == 0 );
            }

            aSheet.symbols.push_back( std::move( inst ) );
        }
    }
}

} // namespace PCAD_SCH
