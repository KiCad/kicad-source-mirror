/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Thomas Pointhuber <thomas.pointhuber@gmx.at>
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef ALTIUM_PARSER_SCH_H
#define ALTIUM_PARSER_SCH_H

#include <cstdint>
#include <cstring>
#include <map>
#include <vector>

#include <wx/gdicmn.h>
#include <wx/string.h>

// this constant specifies a item which is not inside an component
const int ALTIUM_COMPONENT_NONE = -1;

class ALTIUM_PARSER;

struct ASCH_STORAGE_FILE
{
    wxString          filename;
    std::vector<char> data;

    explicit ASCH_STORAGE_FILE( ALTIUM_PARSER& aReader );
};


enum class ALTIUM_SCH_RECORD
{
    HEADER              = 0,
    COMPONENT           = 1,
    PIN                 = 2,
    IEEE_SYMBOL         = 3,
    LABEL               = 4,
    BEZIER              = 5,
    POLYLINE            = 6,
    POLYGON             = 7,
    ELLIPSE             = 8,
    PIECHART            = 9,
    ROUND_RECTANGLE     = 10,
    ELLIPTICAL_ARC      = 11,
    ARC                 = 12,
    LINE                = 13,
    RECTANGLE           = 14,
    SHEET_SYMBOL        = 15,
    SHEET_ENTRY         = 16,
    POWER_PORT          = 17,
    PORT                = 18,
    NO_ERC              = 22,
    NET_LABEL           = 25,
    BUS                 = 26,
    WIRE                = 27,
    TEXT_FRAME          = 28,
    JUNCTION            = 29,
    IMAGE               = 30,
    SHEET               = 31,
    SHEET_NAME          = 32,
    FILE_NAME           = 33,
    DESIGNATOR          = 34,
    BUS_ENTRY           = 37,
    TEMPLATE            = 39,
    PARAMETER           = 41,
    WARNING_SIGN        = 43,
    IMPLEMENTATION_LIST = 44,
    IMPLEMENTATION      = 45,
    RECORD_46           = 46,
    RECORD_47           = 47,
    RECORD_48           = 48,
    NOTE                = 209,
    RECORD_215          = 215,
    RECORD_216          = 216,
    RECORD_217          = 217,
    RECORD_218          = 218,
    RECORD_226          = 226,
};


enum class ASCH_RECORD_ORIENTATION
{
    RIGHTWARDS = 0,
    UPWARDS    = 1,
    LEFTWARDS  = 2,
    DOWNWARDS  = 3
};


struct ASCH_SYMBOL
{
    int      currentpartid;
    wxString libreference;
    wxString sourcelibraryname;
    wxString componentdescription;

    int     orientation;
    bool    isMirrored;
    wxPoint location;

    int partcount;
    int displaymodecount;
    int displaymode;

    explicit ASCH_SYMBOL( const std::map<wxString, wxString>& aProperties );
};


enum class ASCH_PIN_SYMBOL_OUTER
{
    UNKNOWN                = -1,
    NO_SYMBOL              = 0,
    RIGHT_LEFT_SIGNAL_FLOW = 2,
    ANALOG_SIGNAL_IN       = 5,
    NOT_LOGIC_CONNECTION   = 6,
    DIGITAL_SIGNAL_IN      = 25,
    LEFT_RIGHT_SIGNAL_FLOW = 33,
    BIDI_SIGNAL_FLOW       = 34
};


enum class ASCH_PIN_SYMBOL_INNER
{
    UNKNOWN                = -1,
    NO_SYMBOL              = 0,
    POSPONED_OUTPUT        = 8,
    OPEN_COLLECTOR         = 9,
    HIZ                    = 10,
    HIGH_CURRENT           = 11,
    PULSE                  = 12,
    SCHMITT                = 13,
    OPEN_COLLECTOR_PULL_UP = 22,
    OPEN_EMITTER           = 23,
    OPEN_EMITTER_PULL_UP   = 24,
    SHIFT_LEFT             = 30,
    OPEN_OUTPUT            = 32
};


enum class ASCH_PIN_SYMBOL_OUTEREDGE
{
    UNKNOWN    = -1,
    NO_SYMBOL  = 0,
    NEGATED    = 1,
    LOW_INPUT  = 4,
    LOW_OUTPUT = 17
};


enum class ASCH_PIN_SYMBOL_INNEREDGE
{
    UNKNOWN   = -1,
    NO_SYMBOL = 0,
    CLOCK     = 3,
};


enum class ASCH_PIN_ELECTRICAL
{
    UNKNOWN = -1,

    INPUT          = 0,
    BIDI           = 1,
    OUTPUT         = 2,
    OPEN_COLLECTOR = 3,
    PASSIVE        = 4,
    TRISTATE       = 5,
    OPEN_EMITTER   = 6,
    POWER          = 7
};


struct ASCH_PIN
{
    int ownerindex;
    int ownerpartid;
    int ownerpartdisplaymode;

    wxString name;
    wxString text;
    wxString designator;

    ASCH_PIN_SYMBOL_OUTER symbolOuter;
    ASCH_PIN_SYMBOL_INNER symbolInner;

    ASCH_PIN_SYMBOL_OUTEREDGE symbolOuterEdge;
    ASCH_PIN_SYMBOL_INNEREDGE symbolInnerEdge;

    ASCH_PIN_ELECTRICAL     electrical;
    ASCH_RECORD_ORIENTATION orientation;

    wxPoint location;
    int     pinlength;

    wxPoint kicadLocation; // location of pin in KiCad without rounding error

    bool showPinName;
    bool showDesignator;

    explicit ASCH_PIN( const std::map<wxString, wxString>& aProperties );
};


enum class ASCH_LABEL_JUSTIFICATION
{
    UNKNOWN = -1,

    BOTTOM_LEFT   = 0,
    BOTTOM_CENTER = 1,
    BOTTOM_RIGHT  = 2,
    CENTER_LEFT   = 3,
    CENTER_CENTER = 4,
    CENTER_RIGHT  = 5,
    TOP_LEFT      = 6,
    TOP_CENTER    = 7,
    TOP_RIGHT     = 8
};


enum class ASCH_TEXT_FRAME_ALIGNMENT
{
    LEFT    = 1,
    CENTER  = 2,
    RIGHT   = 3
};


struct ASCH_LABEL
{
    int ownerindex;
    int ownerpartid;

    wxPoint location;

    wxString text;

    int  fontId;
    bool isMirrored;

    ASCH_LABEL_JUSTIFICATION justification;

    explicit ASCH_LABEL( const std::map<wxString, wxString>& aProperties );
};


struct ASCH_TEXT_FRAME
{
    wxPoint location;
    wxSize  size;

    wxString text;

    int  fontId;
    bool isWordWrapped;
    bool border;
    int  textMargin;
    int  areaColor;

    ASCH_TEXT_FRAME_ALIGNMENT alignment;

    explicit ASCH_TEXT_FRAME( const std::map<wxString, wxString>& aProperties );
};


struct ASCH_NOTE : ASCH_TEXT_FRAME
{
    wxString author;

    explicit ASCH_NOTE( const std::map<wxString, wxString>& aProperties );
};


struct ASCH_BEZIER
{
    int ownerindex;
    int ownerpartid;
    int ownerpartdisplaymode;

    std::vector<wxPoint> points;

    int lineWidth;

    explicit ASCH_BEZIER( const std::map<wxString, wxString>& aProperties );
};


enum class ASCH_POLYLINE_LINESTYLE
{
    SOLID       = 0,
    DASHED      = 1,
    DOTTED      = 2,
    DASH_DOTTED = 3
};


struct ASCH_POLYLINE
{
    int ownerindex;
    int ownerpartid;
    int ownerpartdisplaymode;

    std::vector<wxPoint> points;

    int lineWidth;

    ASCH_POLYLINE_LINESTYLE linestyle;

    explicit ASCH_POLYLINE( const std::map<wxString, wxString>& aProperties );
};


struct ASCH_POLYGON
{
    int ownerindex;
    int ownerpartid;
    int ownerpartdisplaymode;

    std::vector<wxPoint> points;

    int  lineWidth;
    bool isSolid;

    int color;
    int areacolor;

    explicit ASCH_POLYGON( const std::map<wxString, wxString>& aProperties );
};


struct ASCH_ROUND_RECTANGLE
{
    int ownerindex;
    int ownerpartid;
    int ownerpartdisplaymode;

    wxPoint bottomLeft;
    wxPoint topRight;

    wxSize cornerradius;

    int  lineWidth;
    bool isSolid;
    bool isTransparent;

    int color;
    int areacolor;

    explicit ASCH_ROUND_RECTANGLE( const std::map<wxString, wxString>& aProperties );
};


struct ASCH_ARC
{
    int ownerindex;
    int ownerpartid;
    int ownerpartdisplaymode;

    wxPoint center;
    int     radius;
    double  startAngle;
    double  endAngle;

    int lineWidth;

    explicit ASCH_ARC( const std::map<wxString, wxString>& aProperties );
};


struct ASCH_LINE
{
    int ownerindex;
    int ownerpartid;
    int ownerpartdisplaymode;

    wxPoint point1;
    wxPoint point2;

    int lineWidth;

    explicit ASCH_LINE( const std::map<wxString, wxString>& aProperties );
};


struct ASCH_RECTANGLE
{
    int ownerindex;
    int ownerpartid;
    int ownerpartdisplaymode;

    wxPoint bottomLeft;
    wxPoint topRight;

    int  lineWidth;
    bool isSolid;
    bool isTransparent;

    int color;
    int areacolor;

    explicit ASCH_RECTANGLE( const std::map<wxString, wxString>& aProperties );
};


struct ASCH_SHEET_SYMBOL
{
    wxPoint location;
    wxSize  size;

    bool isSolid;

    int color;
    int areacolor;

    explicit ASCH_SHEET_SYMBOL( const std::map<wxString, wxString>& aProperties );
};


enum class ASCH_SHEET_ENTRY_SIDE
{
    LEFT   = 0,
    RIGHT  = 1,
    TOP    = 2,
    BOTTOM = 3
};


enum class ASCH_PORT_IOTYPE
{
    UNSPECIFIED = 0,
    OUTPUT      = 1,
    INPUT       = 2,
    BIDI        = 3,
};


enum class ASCH_PORT_STYLE
{
    NONE_HORIZONTAL = 0,
    LEFT            = 1,
    RIGHT           = 2,
    LEFT_RIGHT      = 3,
    NONE_VERTICAL   = 4,
    TOP             = 5,
    BOTTOM          = 6,
    TOP_BOTTOM      = 7
};


struct ASCH_SHEET_ENTRY
{
    int ownerindex;
    int ownerpartid;

    int distanceFromTop;

    ASCH_SHEET_ENTRY_SIDE side;
    ASCH_PORT_IOTYPE      iotype;
    ASCH_PORT_STYLE       style;

    wxString name;

    explicit ASCH_SHEET_ENTRY( const std::map<wxString, wxString>& aProperties );
};


enum class ASCH_POWER_PORT_STYLE
{
    UNKNOWN = -1,

    CIRCLE            = 0,
    ARROW             = 1,
    BAR               = 2,
    WAVE              = 3,
    POWER_GROUND      = 4,
    SIGNAL_GROUND     = 5,
    EARTH             = 6,
    GOST_ARROW        = 7,
    GOST_POWER_GROUND = 8,
    GOST_EARTH        = 9,
    GOST_BAR          = 10
};


struct ASCH_POWER_PORT
{
    int ownerpartid;

    wxString text;
    bool     showNetName;

    wxPoint                 location;
    ASCH_RECORD_ORIENTATION orientation;
    ASCH_POWER_PORT_STYLE   style;

    explicit ASCH_POWER_PORT( const std::map<wxString, wxString>& aProperties );
};


struct ASCH_PORT
{
    int ownerpartid;

    wxString name;
    wxString harnessType;

    wxPoint location;
    int     width;
    int     height;

    ASCH_PORT_IOTYPE iotype;
    ASCH_PORT_STYLE  style;

    explicit ASCH_PORT( const std::map<wxString, wxString>& aProperties );
};


struct ASCH_NO_ERC
{
    wxPoint location;

    bool isActive;
    bool supressAll;

    explicit ASCH_NO_ERC( const std::map<wxString, wxString>& aProperties );
};


struct ASCH_NET_LABEL
{
    wxString text;

    wxPoint location;

    ASCH_RECORD_ORIENTATION orientation;

    explicit ASCH_NET_LABEL( const std::map<wxString, wxString>& aProperties );
};


struct ASCH_BUS
{
    int indexinsheet;
    int lineWidth;

    std::vector<wxPoint> points;

    explicit ASCH_BUS( const std::map<wxString, wxString>& aProperties );
};


struct ASCH_WIRE
{
    int indexinsheet;
    int lineWidth;

    std::vector<wxPoint> points;

    explicit ASCH_WIRE( const std::map<wxString, wxString>& aProperties );
};


struct ASCH_JUNCTION
{
    int ownerpartid;

    wxPoint location;

    explicit ASCH_JUNCTION( const std::map<wxString, wxString>& aProperties );
};


struct ASCH_IMAGE
{
    int indexinsheet;
    int ownerpartid;

    wxString filename;
    wxPoint  location;
    wxPoint  corner;

    bool embedimage;
    bool keepaspect;

    explicit ASCH_IMAGE( const std::map<wxString, wxString>& aProperties );
};


struct ASCH_SHEET_FONT
{
    wxString fontname;

    int size;
    int rotation;

    bool italic;
    bool bold;
    bool underline;

    explicit ASCH_SHEET_FONT( const std::map<wxString, wxString>& aProperties, int aId );
};


enum class ASCH_SHEET_SIZE
{
    UNKNOWN = -1, // use A4

    A4      = 0,  // 1150 × 760
    A3      = 1,  // 1550 × 1110
    A2      = 2,  // 2230 × 1570
    A1      = 3,  // 3150 × 2230
    A0      = 4,  // 4460 × 3150
    A       = 5,  // 950 × 750
    B       = 6,  // 1500 × 950
    C       = 7,  // 2000 × 1500
    D       = 8,  // 3200 × 2000
    E       = 9,  // 4200 × 3200
    LETTER  = 10, // 1100 × 850
    LEGAL   = 11, // 1400 × 850
    TABLOID = 12, // 1700 × 1100
    ORCAD_A = 13, // 990 × 790
    ORCAD_B = 14, // 1540 × 990
    ORCAD_C = 15, // 2060 × 1560
    ORCAD_D = 16, // 3260 × 2060
    ORCAD_E = 17  // 4280 × 3280
};

wxPoint ASchSheetGetSize( ASCH_SHEET_SIZE aSheetSize );


enum class ASCH_SHEET_WORKSPACEORIENTATION
{
    LANDSCAPE = 0,
    PORTRAIT  = 1
};


struct ASCH_SHEET
{
    std::vector<ASCH_SHEET_FONT> fonts;

    ASCH_SHEET_SIZE                 sheetSize;
    ASCH_SHEET_WORKSPACEORIENTATION sheetOrientation;

    explicit ASCH_SHEET( const std::map<wxString, wxString>& aProperties );
};


struct ASCH_SHEET_NAME
{
    int ownerindex;
    int ownerpartid;

    wxString text;

    ASCH_RECORD_ORIENTATION orientation;
    wxPoint                 location;

    bool isHidden;

    explicit ASCH_SHEET_NAME( const std::map<wxString, wxString>& aProperties );
};


struct ASCH_FILE_NAME
{
    int ownerindex;
    int ownerpartid;

    wxString text;

    ASCH_RECORD_ORIENTATION orientation;
    wxPoint                 location;

    bool isHidden;

    explicit ASCH_FILE_NAME( const std::map<wxString, wxString>& aProperties );
};


struct ASCH_DESIGNATOR
{
    int ownerindex;
    int ownerpartid;

    wxString name;
    wxString text;

    ASCH_RECORD_ORIENTATION orientation;
    wxPoint location;

    explicit ASCH_DESIGNATOR( const std::map<wxString, wxString>& aProperties );
};


struct ASCH_IMPLEMENTATION
{
    int ownerindex;

    wxString name;
    wxString type;
    wxString libname;

    bool isCurrent;

    explicit ASCH_IMPLEMENTATION( const std::map<wxString, wxString>& aProperties );
};


struct ASCH_IMPLEMENTATION_LIST
{
    int ownerindex;
    explicit ASCH_IMPLEMENTATION_LIST( const std::map<wxString, wxString>& aProperties );
};


struct ASCH_BUS_ENTRY
{
    wxPoint location;
    wxPoint corner;

    explicit ASCH_BUS_ENTRY( const std::map<wxString, wxString>& aProperties );
};


struct ASCH_PARAMETER
{
    int ownerindex;
    int ownerpartid;

    wxPoint                 location;
    ASCH_RECORD_ORIENTATION orientation;

    wxString name;
    wxString text;

    bool isHidden;
    bool isMirrored;
    bool isShowName;

    explicit ASCH_PARAMETER( const std::map<wxString, wxString>& aProperties );
};

#endif //ALTIUM_PARSER_SCH_H
