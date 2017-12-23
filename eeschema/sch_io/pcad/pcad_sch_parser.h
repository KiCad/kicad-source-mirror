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

#ifndef PCAD_SCH_PARSER_H
#define PCAD_SCH_PARSER_H

#include <wx/string.h>
#include <map>
#include <vector>

class XNODE;
class wxXmlDocument;

namespace PCAD_SCH
{

enum class JUSTIFY
{
    LOWER_LEFT, LOWER_CENTER, LOWER_RIGHT,
    UPPER_LEFT, UPPER_CENTER, UPPER_RIGHT,
    LEFT, CENTER, RIGHT
};

enum class LINE_KIND
{
    SOLID,
    DASHED,
    DOTTED
};

/// One font description inside a (textStyleDef ...).  A style carries up to two
/// fonts (Stroke and TrueType); textStyleDisplayTType selects the effective one.
struct FONT
{
    bool   isTrueType = false;
    double height = 100.0;        // mils
    double strokeWidth = 10.0;    // mils
    bool   isBold = false;
    bool   isItalic = false;
};

struct TEXT_STYLE
{
    wxString name;
    FONT     strokeFont;
    FONT     ttfFont;
    bool     hasTtfFont = false;
    bool     displayTType = false;

    const FONT& EffectiveFont() const
    {
        return ( displayTType && hasTtfFont ) ? ttfFont : strokeFont;
    }
};

/// A positioned text: free sheet text, wire/bus display names, pin name/designator
/// texts and attribute values all share this shape.
struct TEXT_ITEM
{
    wxString text;
    double   x = 0, y = 0;        // mils
    double   rotation = 0;        // degrees CCW (P-CAD Y-up)
    bool     isFlipped = false;
    bool     isVisible = true;
    JUSTIFY  justify = JUSTIFY::LOWER_LEFT;
    wxString styleRef;            // references a TEXT_STYLE by name
};

struct PIN
{
    wxString  pinNum;             // symbol pin ordinal inside the symbolDef
    wxString  defaultPinDes;      // default pad designator when no compDef mapping exists
    double    x = 0, y = 0;       // body attachment point, mils
    double    rotation = 0;       // 0/90/180/270 CCW
    bool      isFlipped = false;
    double    pinLength = 300.0;  // mils - P-CAD default
    wxString  outsideEdgeStyle;   // Dot, Clock, ...
    wxString  insideEdgeStyle;
    bool      showPinDes = true;  // (pinDisplay (dispPinDes ...)) - designator text
    bool      showPinName = false;// (pinDisplay (dispPinName ...))
    TEXT_ITEM pinDesText;
    TEXT_ITEM pinNameText;
};

struct LINE
{
    std::vector<std::pair<double, double>> pts;   // mils
    double    width = 10.0;
    LINE_KIND style = LINE_KIND::SOLID;
};

/// Center/radius/angles form; triplePointArc is converted at parse time.
struct ARC
{
    double x = 0, y = 0, radius = 0;              // mils
    double startAngle = 0, sweepAngle = 0;        // degrees
    double width = 10.0;
};

struct POLY
{
    std::vector<std::pair<double, double>> pts;   // mils
};

enum class IEEE_KIND
{
    NONE,
    ADDER,
    AMPLIFIER,
    ASTABLE,
    COMPLEX,
    GENERATOR,
    HYSTERESIS,
    MULTIPLIER
};

struct IEEE_SYMBOL
{
    IEEE_KIND kind = IEEE_KIND::NONE;
    double    x = 0, y = 0;       // mils
    double    height = 0;         // mils
    double    rotation = 0;       // degrees
    bool      isFlipped = false;
};

struct ATTR
{
    wxString  name;
    wxString  value;
    TEXT_ITEM placement;
};

struct SYMBOL_DEF
{
    wxString                 name;
    wxString                 originalName;
    std::vector<PIN>         pins;
    std::vector<LINE>        lines;
    std::vector<ARC>         arcs;
    std::vector<POLY>        polys;
    std::vector<TEXT_ITEM>   texts;
    std::vector<IEEE_SYMBOL> ieeeSymbols;
    std::vector<ATTR>        attrs;
};

/// (compPin "padDes" ...) inside a compDef: maps a symbol pin ordinal to the
/// physical pad designator and carries the electrical type.
struct COMP_PIN
{
    wxString padDes;              // node Name attribute - the pad designator
    wxString pinName;
    int      partNum = 1;
    wxString symPinNum;
    wxString pinType;             // Passive, Input, Output, Power, Bidirectional, ...
};

struct COMP_DEF
{
    wxString              name;
    wxString              originalName;
    wxString              refDesPrefix;
    int                   numParts = 1;
    bool                  isPower = false;      // (compType Power)
    wxString              description;
    wxString              attachedPattern;      // footprint name
    std::vector<COMP_PIN> compPins;
    std::vector<wxString> attachedSymbols;      // indexed by partNum (1-based; [0] unused)
};

struct COMP_INST
{
    wxString refDes;
    wxString compRef;             // references a COMP_DEF name
    wxString originalName;
    wxString value;               // compValue
};

struct WIRE
{
    std::vector<std::pair<double, double>> pts;   // mils; a wire line may be a polyline
    double    width = 10.0;
    wxString  netName;
    bool      dispName = false;
    TEXT_ITEM label;              // net-name label placement when dispName is set
};

struct BUS
{
    wxString  name;
    std::vector<std::pair<double, double>> pts;
    bool      dispName = false;
    TEXT_ITEM label;
};

struct BUS_ENTRY
{
    wxString busNameRef;
    double   x = 0, y = 0;        // mils - wire-side end of the entry
    wxString orient;              // Left, Right, Up, Down - direction toward the bus
};

struct PORT
{
    double   x = 0, y = 0;        // mils
    wxString netNameRef;
    wxString portType;            // e.g. NoOutline_Sgl_Vert, BothAngle_Dbl_Horz
    double   rotation = 0;
    bool     isFlipped = false;
};

struct JUNCTION
{
    double   x = 0, y = 0;        // mils
    wxString netName;
};

struct SYMBOL_INST
{
    wxString          symbolRef;  // references a SYMBOL_DEF name
    wxString          refDesRef;
    int               partNum = 1;
    double            x = 0, y = 0;   // mils
    double            rotation = 0;
    bool              isFlipped = false;
    std::vector<ATTR> attrs;      // per-instance RefDes/Value/... placements
};

struct SHEET
{
    wxString                 name;
    int                      sheetNum = 1;
    std::vector<WIRE>        wires;
    std::vector<BUS>         buses;
    std::vector<BUS_ENTRY>   busEntries;
    std::vector<PORT>        ports;
    std::vector<JUNCTION>    junctions;
    std::vector<SYMBOL_INST> symbols;
    std::vector<TEXT_ITEM>   texts;
    std::vector<LINE>        lines;
    std::vector<ARC>         arcs;
    std::vector<POLY>        polys;
    std::vector<IEEE_SYMBOL> ieeeSymbols;
};

struct TITLE_SHEET
{
    std::map<wxString, wxString> fields;   // fieldDef name -> value
};

struct SCHEMATIC
{
    double workspaceWidth = 17000.0;      // mils
    double workspaceHeight = 11000.0;     // mils
    bool   isMetric = false;              // (fileUnits ...) - default unit for bare numbers

    std::vector<TEXT_STYLE>                textStyles;
    std::map<wxString, const TEXT_STYLE*>  textStylesByName;

    std::vector<SYMBOL_DEF>                symbolDefs;
    std::map<wxString, const SYMBOL_DEF*>  symbolDefsByName;

    std::vector<COMP_DEF>                  compDefs;
    std::map<wxString, const COMP_DEF*>    compDefsByName;

    /// compAlias name -> compDef name
    std::map<wxString, wxString>           compAliases;

    std::vector<COMP_INST>                 compInsts;
    std::map<wxString, const COMP_INST*>   compInstsByRef;

    std::vector<SHEET>                     sheets;
    TITLE_SHEET                            titleSheet;

    const TEXT_STYLE* FindTextStyle( const wxString& aName ) const
    {
        auto it = textStylesByName.find( aName );
        return it == textStylesByName.end() ? nullptr : it->second;
    }
};


class PCAD_SCH_PARSER
{
public:
    PCAD_SCH_PARSER() = default;

    void LoadFromFile( const wxString& aFilename, SCHEMATIC& aSchematic );

    static XNODE*   FindChild( XNODE* aNode, const wxString& aTag );
    static wxString NodeText( XNODE* aNode );

private:
    void parseHeader( XNODE* aNode, SCHEMATIC& aSchematic );
    void parseLibrary( XNODE* aNode, SCHEMATIC& aSchematic );
    void parseTextStyleDef( XNODE* aNode, SCHEMATIC& aSchematic );
    void parseSymbolDef( XNODE* aNode, SYMBOL_DEF& aSymDef );
    void parseCompDef( XNODE* aNode, COMP_DEF& aCompDef );

    PIN         parsePin( XNODE* aNode );
    LINE        parseLine( XNODE* aNode );
    ARC         parseArc( XNODE* aNode );
    ARC         parseTriplePointArc( XNODE* aNode );
    POLY        parsePoly( XNODE* aNode );
    ATTR        parseAttr( XNODE* aNode );
    TEXT_ITEM   parseText( XNODE* aNode );
    IEEE_SYMBOL parseIeeeSymbol( XNODE* aNode );

    void parseNetlist( XNODE* aNode, SCHEMATIC& aSchematic );
    void parseSchematicDesign( XNODE* aNode, SCHEMATIC& aSchematic );
    void parseTitleSheet( XNODE* aNode, SCHEMATIC& aSchematic );
    void parseSheet( XNODE* aNode, SHEET& aSheet );
    WIRE parseWire( XNODE* aNode );
    BUS  parseBus( XNODE* aNode );

    // Measurement-aware value parsing.  A bare number uses the file default
    // (fileUnits); an explicit suffix ("1.5mm", "0.198 mm") always wins.
    double      toMils( const wxString& aValue ) const;
    bool        parsePt( XNODE* aNode, double& aX, double& aY ) const;
    bool        parsePtNode( XNODE* aPtNode, double& aX, double& aY ) const;
    double      childDouble( XNODE* aNode, const wxString& aTag, double aDefault = 0.0 ) const;
    static wxString childStr( XNODE* aNode, const wxString& aTag,
                              const wxString& aDefault = wxEmptyString );
    static bool childFlag( XNODE* aNode, const wxString& aTag );
    static JUSTIFY parseJustify( const wxString& aValue );

    bool m_isMetric = false;
};

} // namespace PCAD_SCH

#endif // PCAD_SCH_PARSER_H
