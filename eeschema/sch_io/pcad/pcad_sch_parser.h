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

#ifndef PCAD_SCH_PARSER_H
#define PCAD_SCH_PARSER_H

#include <wx/string.h>
#include <vector>
#include <map>

class XNODE;
class wxXmlDocument;

namespace PCAD_SCH
{

struct PIN
{
    wxString number;
    wxString name;
    double   x = 0, y = 0;       // connection endpoint in mils
    double   rotation = 0;        // 0/90/180/270 CCW
    bool     isFlipped = false;
    double   pinLength = 300.0;   // mils — P-Cad default; pt is body attachment
};

struct LINE
{
    double x1, y1, x2, y2;       // mils
};

struct ARC
{
    double x, y, radius;          // mils
    double startAngle, sweepAngle; // degrees
};

enum class JUSTIFY
{
    LOWER_LEFT, LOWER_CENTER, LOWER_RIGHT,
    UPPER_LEFT, UPPER_CENTER, UPPER_RIGHT,
    LEFT, CENTER, RIGHT
};

struct ATTR
{
    wxString name;
    wxString value;
    double   x = 0, y = 0;
    bool     isVisible = true;
    JUSTIFY  justify = JUSTIFY::LEFT;
};

struct SYMBOL_DEF
{
    wxString          name;
    wxString          originalName;
    std::vector<PIN>  pins;
    std::vector<LINE> lines;
    std::vector<ARC>  arcs;
    std::vector<ATTR> attrs;
};

struct COMP_INST
{
    wxString refDes;
    wxString compRef;        // symbolDef name
    wxString originalName;
    wxString value;          // compValue
};

struct WIRE
{
    double   x1, y1, x2, y2; // mils
    wxString netName;
};

struct JUNCTION
{
    double   x, y;           // mils
    wxString netName;
};

struct SYMBOL_INST
{
    wxString symbolRef;      // references a SYMBOL_DEF name
    wxString refDesRef;
    int      partNum = 1;
    double   x = 0, y = 0;  // mils
    double   rotation = 0;
    bool     isFlipped = false;
};

struct SHEET
{
    wxString                   name;
    int                        sheetNum = 1;
    std::vector<WIRE>          wires;
    std::vector<JUNCTION>      junctions;
    std::vector<SYMBOL_INST>   symbols;
};

struct SCHEMATIC
{
    double workspaceWidth = 17000.0;   // mils
    double workspaceHeight = 11000.0;  // mils

    std::vector<SYMBOL_DEF>              symbolDefs;
    std::map<wxString, const SYMBOL_DEF*> symbolDefsByName;

    std::vector<COMP_INST>               compInsts;
    std::map<wxString, const COMP_INST*> compInstsByRef;

    std::vector<SHEET>                   sheets;
};


class PCAD_SCH_PARSER
{
public:
    PCAD_SCH_PARSER() = default;

    void        LoadFromFile( const wxString& aFilename, SCHEMATIC& aSchematic );

    // Public so free helper functions in the .cpp can use them.
    static XNODE*   findChild( XNODE* aNode, const wxString& aTag );
    static double   parseDouble( const wxString& aStr );

private:
    void        parseLibrary( XNODE* aNode, SCHEMATIC& aSchematic );
    void        parseSymbolDef( XNODE* aNode, SYMBOL_DEF& aSymDef );
    PIN         parsePin( XNODE* aNode );
    LINE        parseLine( XNODE* aNode );
    ARC         parseArc( XNODE* aNode );
    ATTR        parseAttr( XNODE* aNode );

    void        parseNetlist( XNODE* aNode, SCHEMATIC& aSchematic );
    void        parseSchematicDesign( XNODE* aNode, SCHEMATIC& aSchematic );
    void        parseSheet( XNODE* aNode, SHEET& aSheet );

    static double   nodeChildDouble( XNODE* aNode, const wxString& aTag, double aDefault = 0.0 );
    static wxString nodeChildStr( XNODE* aNode, const wxString& aTag,
                                  const wxString& aDefault = wxEmptyString );
};

} // namespace PCAD_SCH

#endif // PCAD_SCH_PARSER_H
