
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012-2015 KiCad Developers, see change_log.txt for contributors.

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


/*

Pcbnew PLUGIN for Eagle 6.x XML *.brd and footprint format.

XML parsing and converting:
Getting line numbers and byte offsets from the source XML file is not
possible using currently available XML libraries within KiCad project:
wxXmlDocument and boost::property_tree.

property_tree will give line numbers but no byte offsets, and only during
document loading. This means that if we have a problem after the document is
successfully loaded, there is no way to correlate back to line number and byte
offset of the problem. So a different approach is taken, one which relies on the
XML elements themselves using an XPATH type of reporting mechanism. The path to
the problem is reported in the error messages. This means keeping track of that
path as we traverse the XML document for the sole purpose of accurate error
reporting.

User can load the source XML file into firefox or other xml browser and follow
our error message.

Load() TODO's

*) verify zone fill clearances are correct

*/

#include <errno.h>

#include <wx/string.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include <eagle_plugin.h>

#include <common.h>
#include <macros.h>
#include <fctsys.h>
#include <trigo.h>
#include <macros.h>
#include <kicad_string.h>
#include <wx/filename.h>

#include <class_board.h>
#include <class_module.h>
#include <class_track.h>
#include <class_edge_mod.h>
#include <class_zone.h>
#include <class_pcb_text.h>
#include <class_dimension.h>

using namespace boost::property_tree;
using namespace std;

typedef EAGLE_PLUGIN::BIU                   BIU;
typedef PTREE::const_assoc_iterator         CA_ITER;
typedef PTREE::const_iterator               CITER;
typedef std::pair<CA_ITER, CA_ITER>         CA_ITER_RANGE;

typedef MODULE_MAP::iterator                MODULE_ITER;
typedef MODULE_MAP::const_iterator          MODULE_CITER;

typedef boost::optional<string>             opt_string;
typedef boost::optional<int>                opt_int;
typedef boost::optional<double>             opt_double;
typedef boost::optional<bool>               opt_bool;


/// segment (element) of our XPATH into the Eagle XML document tree in PTREE form.
struct TRIPLET
{
    const char* element;
    const char* attribute;
    const char* value;

    TRIPLET( const char* aElement, const char* aAttribute = "", const char* aValue = "" ) :
        element( aElement ),
        attribute( aAttribute ),
        value( aValue )
    {}
};


/**
 * Class XPATH
 * keeps track of what we are working on within a PTREE.
 * Then if an exception is thrown, the place within the tree that gave us
 * grief can be reported almost accurately.  To minimally impact
 * speed, merely assign const char* pointers during the tree walking
 * expedition.  The const char* pointers must be to C strings residing either in
 * the data or code segment (i.e. "compiled in") or within the XML document, but
 * not on the stack, since the stack is unwound during the throwing of the
 * exception.  The XML document will not immediately vanish since we capture
 * the xpath (using function Contents()) before the XML document tree (PTREE)
 * is destroyed.
 */
class XPATH
{
    std::vector<TRIPLET>    p;

public:
    void push( const char* aPathSegment, const char* aAttribute="" )
    {
        p.push_back( TRIPLET( aPathSegment, aAttribute ) );
    }

    void clear()    { p.clear(); }

    void pop()      { p.pop_back(); }

    /// modify the last path node's value
    void Value( const char* aValue )
    {
        p.back().value = aValue;
    }

    /// modify the last path node's attribute
    void Attribute( const char* aAttribute )
    {
        p.back().attribute = aAttribute;
    }

    /// return the contents of the XPATH as a single string
    string Contents()
    {
        typedef std::vector<TRIPLET>::const_iterator CITER;

        string ret;

        for( CITER it = p.begin();  it != p.end();  ++it )
        {
            if( it != p.begin() )
                ret += '.';

            ret += it->element;

            if( it->attribute[0] && it->value[0] )
            {
                ret += '[';
                ret += it->attribute;
                ret += '=';
                ret += it->value;
                ret += ']';
            }
        }

        return ret;
    }
};


/**
 * Function parseOptionalBool
 * returns an opt_bool and sets it true or false according to the presence
 * and value of an attribute within the CPTREE element.
 */
static opt_bool parseOptionalBool( CPTREE& attribs, const char* aName )
{
    opt_bool    ret;
    opt_string  stemp = attribs.get_optional<string>( aName );

    if( stemp )
        ret = !stemp->compare( "yes" );

    return ret;
}


// All of the 'E'STRUCTS below merely hold Eagle XML information verbatim, in binary.
// For maintenance and troubleshooting purposes, it was thought that we'd need to
// separate the conversion process into distinct steps. There is no intent to have KiCad
// forms of information in these 'E'STRUCTS.  They are only binary forms
// of the Eagle information in the corresponding Eagle XML nodes.


/// Eagle rotation
struct EROT
{
    bool    mirror;
    bool    spin;
    double  degrees;

    EROT() :
        mirror( false ),
        spin( false ),
        degrees( 0 )
    {}

    EROT( double aDegrees ) :
        mirror( false ),
        spin( false ),
        degrees( aDegrees )
    {}
};

typedef boost::optional<EROT>   opt_erot;

/// parse an Eagle XML "rot" field.  Unfortunately the DTD seems not to explain
/// this format very well.  [S][M]R<degrees>.   Examples: "R90", "MR180", "SR180"
static EROT erot( const string& aRot )
{
    EROT    rot;

    rot.spin    = aRot.find( 'S' ) != aRot.npos;
    rot.mirror  = aRot.find( 'M' ) != aRot.npos;
    rot.degrees = strtod( aRot.c_str()
                        + 1                     // skip leading 'R'
                        + int( rot.spin )       // skip optional leading 'S'
                        + int( rot.mirror ),    // skip optional leading 'M'
                        NULL );
    return rot;
}

/// Eagle "rot" fields are optional, handle that by returning opt_erot.
static opt_erot parseOptionalEROT( CPTREE& attribs )
{
    opt_erot    ret;
    opt_string  stemp = attribs.get_optional<string>( "rot" );
    if( stemp )
        ret = erot( *stemp );
    return ret;
}

/// Eagle wire
struct EWIRE
{
    double      x1;
    double      y1;
    double      x2;
    double      y2;
    double      width;
    LAYER_NUM   layer;

    // for style: (continuous | longdash | shortdash | dashdot)
    enum {
        CONTINUOUS,
        LONGDASH,
        SHORTDASH,
        DASHDOT,
    };
    opt_int     style;
    opt_double  curve;      ///< range is -359.9..359.9

    // for cap: (flat | round)
    enum {
        FLAT,
        ROUND,
    };
    opt_int     cap;

    EWIRE( CPTREE& aWire );
};

/**
 * Constructor EWIRE
 * converts a "wire"'s xml attributes ( &ltwire&gt )
 * to binary without additional conversion.
 * This result is an EWIRE with the &ltwire&gt textual data merely converted to binary.
 */
EWIRE::EWIRE( CPTREE& aWire )
{
    CPTREE& attribs = aWire.get_child( "<xmlattr>" );

    /*
    <!ELEMENT wire EMPTY>
    <!ATTLIST wire
          x1            %Coord;        #REQUIRED
          y1            %Coord;        #REQUIRED
          x2            %Coord;        #REQUIRED
          y2            %Coord;        #REQUIRED
          width         %Dimension;    #REQUIRED
          layer         %Layer;        #REQUIRED
          extent        %Extent;       #IMPLIED  -- only applicable for airwires --
          style         %WireStyle;    "continuous"
          curve         %WireCurve;    "0"
          cap           %WireCap;      "round"   -- only applicable if 'curve' is not zero --
          >
    */

    x1    = attribs.get<double>( "x1" );
    y1    = attribs.get<double>( "y1" );
    x2    = attribs.get<double>( "x2" );
    y2    = attribs.get<double>( "y2" );
    width = attribs.get<double>( "width" );
    layer = attribs.get<int>( "layer" );

    curve = attribs.get_optional<double>( "curve" );

    opt_string s = attribs.get_optional<string>( "style" );
    if( s )
    {
        if( !s->compare( "continuous" ) )
            style = EWIRE::CONTINUOUS;
        else if( !s->compare( "longdash" ) )
            style = EWIRE::LONGDASH;
        else if( !s->compare( "shortdash" ) )
            style = EWIRE::SHORTDASH;
        else if( !s->compare( "dashdot" ) )
            style = EWIRE::DASHDOT;
    }

    s = attribs.get_optional<string>( "cap" );
    if( s )
    {
        if( !s->compare( "round" ) )
            cap = EWIRE::ROUND;
        else if( !s->compare( "flat" ) )
            cap = EWIRE::FLAT;
    }
    // ignoring extent
}


/// Eagle via
struct EVIA
{
    double      x;
    double      y;
    int         layer_front_most;   /// < extent
    int         layer_back_most;    /// < inclusive
    double      drill;
    opt_double  diam;
    opt_string  shape;
    EVIA( CPTREE& aVia );
};

EVIA::EVIA( CPTREE& aVia )
{
    CPTREE& attribs = aVia.get_child( "<xmlattr>" );

    /*
    <!ELEMENT via EMPTY>
    <!ATTLIST via
          x             %Coord;        #REQUIRED
          y             %Coord;        #REQUIRED
          extent        %Extent;       #REQUIRED
          drill         %Dimension;    #REQUIRED
          diameter      %Dimension;    "0"
          shape         %ViaShape;     "round"
          alwaysstop    %Bool;         "no"
          >
    */

    x     = attribs.get<double>( "x" );
    y     = attribs.get<double>( "y" );

    string ext = attribs.get<string>( "extent" );

    sscanf( ext.c_str(), "%d-%d", &layer_front_most, &layer_back_most );

    drill = attribs.get<double>( "drill" );
    diam  = attribs.get_optional<double>( "diameter" );
    shape = attribs.get_optional<string>( "shape" );
}


/// Eagle circle
struct ECIRCLE
{
    double  x;
    double  y;
    double  radius;
    double  width;
    LAYER_NUM layer;

    ECIRCLE( CPTREE& aCircle );
};

ECIRCLE::ECIRCLE( CPTREE& aCircle )
{
    CPTREE& attribs = aCircle.get_child( "<xmlattr>" );

    /*
    <!ELEMENT circle EMPTY>
    <!ATTLIST circle
          x             %Coord;        #REQUIRED
          y             %Coord;        #REQUIRED
          radius        %Coord;        #REQUIRED
          width         %Dimension;    #REQUIRED
          layer         %Layer;        #REQUIRED
          >
    */

    x      = attribs.get<double>( "x" );
    y      = attribs.get<double>( "y" );
    radius = attribs.get<double>( "radius" );
    width  = attribs.get<double>( "width" );
    layer  = attribs.get<int>( "layer" );
}


/// Eagle XML rectangle in binary
struct ERECT
{
    double      x1;
    double      y1;
    double      x2;
    double      y2;
    int         layer;
    opt_erot    rot;

    ERECT( CPTREE& aRect );
};

ERECT::ERECT( CPTREE& aRect )
{
    CPTREE& attribs = aRect.get_child( "<xmlattr>" );

    /*
    <!ELEMENT rectangle EMPTY>
    <!ATTLIST rectangle
          x1            %Coord;        #REQUIRED
          y1            %Coord;        #REQUIRED
          x2            %Coord;        #REQUIRED
          y2            %Coord;        #REQUIRED
          layer         %Layer;        #REQUIRED
          rot           %Rotation;     "R0"
          >
    */

    x1    = attribs.get<double>( "x1" );
    y1    = attribs.get<double>( "y1" );
    x2    = attribs.get<double>( "x2" );
    y2    = attribs.get<double>( "y2" );
    layer = attribs.get<int>( "layer" );
    rot   = parseOptionalEROT( attribs );
}


/// Eagle "attribute" XML element, no foolin'.
struct EATTR
{
    string      name;
    opt_string  value;
    opt_double  x;
    opt_double  y;
    opt_double  size;
    opt_int     layer;
    opt_double  ratio;
    opt_erot    rot;

    enum {  // for 'display'
        Off,
        VALUE,
        NAME,
        BOTH,
    };
    opt_int     display;

    EATTR( CPTREE& aTree );
    EATTR() {}
};

/**
 * Constructor EATTR
 * parses an Eagle "attribute" XML element.  Note that an attribute element
 * is different than an XML element attribute.  The attribute element is a
 * full XML node in and of itself, and has attributes of its own.  Blame Eagle.
 */
EATTR::EATTR( CPTREE& aAttribute )
{
    CPTREE& attribs = aAttribute.get_child( "<xmlattr>" );

    /*
    <!ELEMENT attribute EMPTY>
    <!ATTLIST attribute
        name          %String;       #REQUIRED
        value         %String;       #IMPLIED
        x             %Coord;        #IMPLIED
        y             %Coord;        #IMPLIED
        size          %Dimension;    #IMPLIED
        layer         %Layer;        #IMPLIED
        font          %TextFont;     #IMPLIED
        ratio         %Int;          #IMPLIED
        rot           %Rotation;     "R0"
        display       %AttributeDisplay; "value" -- only in <element> or <instance> context --
        constant      %Bool;         "no"     -- only in <device> context --
        >
    */

    name    = attribs.get<string>( "name" );                    // #REQUIRED
    value   = attribs.get_optional<string>( "value" );

    x       = attribs.get_optional<double>( "x" );
    y       = attribs.get_optional<double>( "y" );
    size    = attribs.get_optional<double>( "size" );

    // KiCad cannot currently put a TEXTE_MODULE on a different layer than the MODULE
    // Eagle can it seems.
    layer   = attribs.get_optional<int>( "layer" );

    ratio   = attribs.get_optional<double>( "ratio" );
    rot     = parseOptionalEROT( attribs );

    opt_string stemp = attribs.get_optional<string>( "display" );
    if( stemp )
    {
        // (off | value | name | both)
        if( !stemp->compare( "off" ) )
            display = EATTR::Off;
        else if( !stemp->compare( "value" ) )
            display = EATTR::VALUE;
        else if( !stemp->compare( "name" ) )
            display = EATTR::NAME;
        else if( !stemp->compare( "both" ) )
            display = EATTR::BOTH;
    }
}

/// Eagle dimension element
struct EDIMENSION
{
    double      x1;
    double      y1;
    double      x2;
    double      y2;
    double      x3;
    double      y3;
    int         layer;

    opt_string dimensionType;

    EDIMENSION( CPTREE& aDimension );
};

EDIMENSION::EDIMENSION( CPTREE& aDimension )
{
    CPTREE& attribs = aDimension.get_child( "<xmlattr>" );

    /*
    <!ELEMENT dimension EMPTY>
    <!ATTLIST dimension
          x1            %Coord;        #REQUIRED
          y1            %Coord;        #REQUIRED
          x2            %Coord;        #REQUIRED
          y2            %Coord;        #REQUIRED
          x3            %Coord;        #REQUIRED
          y3            %Coord;        #REQUIRED
          layer         %Layer;        #REQUIRED
          dtype         %DimensionType; "parallel"
          >
    */

    x1      = attribs.get<double>( "x1" );
    y1      = attribs.get<double>( "y1" );
    x2      = attribs.get<double>( "x2" );
    y2      = attribs.get<double>( "y2" );
    x3      = attribs.get<double>( "x3" );
    y3      = attribs.get<double>( "y3" );
    layer   = attribs.get<int>( "layer" );

    opt_string dimensionType = attribs.get_optional<string>( "dtype" );
    if(!dimensionType)
    {
        // default type is parallel
    }
}

/// Eagle text element
struct ETEXT
{
    string      text;
    double      x;
    double      y;
    double      size;
    int         layer;
    opt_string  font;
    opt_double  ratio;
    opt_erot    rot;

    enum {          // for align
        CENTER,
        CENTER_LEFT,
        TOP_CENTER,
        TOP_LEFT,
        TOP_RIGHT,

        // opposites are -1 x above, used by code tricks in here
        CENTER_RIGHT  = -CENTER_LEFT,
        BOTTOM_CENTER = -TOP_CENTER,
        BOTTOM_LEFT   = -TOP_RIGHT,
        BOTTOM_RIGHT  = -TOP_LEFT,
    };

    opt_int     align;

    ETEXT( CPTREE& aText );
};

ETEXT::ETEXT( CPTREE& aText )
{
    CPTREE& attribs = aText.get_child( "<xmlattr>" );

    /*
    <!ELEMENT text (#PCDATA)>
    <!ATTLIST text
          x             %Coord;        #REQUIRED
          y             %Coord;        #REQUIRED
          size          %Dimension;    #REQUIRED
          layer         %Layer;        #REQUIRED
          font          %TextFont;     "proportional"
          ratio         %Int;          "8"
          rot           %Rotation;     "R0"
          align         %Align;        "bottom-left"
          >
    */

    text   = aText.data();
    x      = attribs.get<double>( "x" );
    y      = attribs.get<double>( "y" );
    size   = attribs.get<double>( "size" );
    layer  = attribs.get<int>( "layer" );

    font   = attribs.get_optional<string>( "font" );
    ratio  = attribs.get_optional<double>( "ratio" );
    rot    = parseOptionalEROT( attribs );

    opt_string stemp = attribs.get_optional<string>( "align" );
    if( stemp )
    {
        // (bottom-left | bottom-center | bottom-right | center-left |
        //   center | center-right | top-left | top-center | top-right)
        if( !stemp->compare( "center" ) )
            align = ETEXT::CENTER;
        else if( !stemp->compare( "center-right" ) )
            align = ETEXT::CENTER_RIGHT;
        else if( !stemp->compare( "top-left" ) )
            align = ETEXT::TOP_LEFT;
        else if( !stemp->compare( "top-center" ) )
            align = ETEXT::TOP_CENTER;
        else if( !stemp->compare( "top-right" ) )
            align = ETEXT::TOP_RIGHT;
        else if( !stemp->compare( "bottom-left" ) )
            align = ETEXT::BOTTOM_LEFT;
        else if( !stemp->compare( "bottom-center" ) )
            align = ETEXT::BOTTOM_CENTER;
        else if( !stemp->compare( "bottom-right" ) )
            align = ETEXT::BOTTOM_RIGHT;
        else if( !stemp->compare( "center-left" ) )
            align = ETEXT::CENTER_LEFT;
    }
}


/// Eagle thru hol pad
struct EPAD
{
    string      name;
    double      x;
    double      y;
    double      drill;
    opt_double  diameter;

    // for shape: (square | round | octagon | long | offset)
    enum {
        SQUARE,
        ROUND,
        OCTAGON,
        LONG,
        OFFSET,
    };
    opt_int     shape;
    opt_erot    rot;
    opt_bool    stop;
    opt_bool    thermals;
    opt_bool    first;

    EPAD( CPTREE& aPad );
};

EPAD::EPAD( CPTREE& aPad )
{
    CPTREE& attribs = aPad.get_child( "<xmlattr>" );

    /*
    <!ELEMENT pad EMPTY>
    <!ATTLIST pad
          name          %String;       #REQUIRED
          x             %Coord;        #REQUIRED
          y             %Coord;        #REQUIRED
          drill         %Dimension;    #REQUIRED
          diameter      %Dimension;    "0"
          shape         %PadShape;     "round"
          rot           %Rotation;     "R0"
          stop          %Bool;         "yes"
          thermals      %Bool;         "yes"
          first         %Bool;         "no"
          >
    */

    // #REQUIRED says DTD, throw exception if not found
    name  = attribs.get<string>( "name" );
    x     = attribs.get<double>( "x" );
    y     = attribs.get<double>( "y" );
    drill = attribs.get<double>( "drill" );

    diameter = attribs.get_optional<double>( "diameter" );

    opt_string s = attribs.get_optional<string>( "shape" );
    if( s )
    {
        // (square | round | octagon | long | offset)
        if( !s->compare( "square" ) )
            shape = EPAD::SQUARE;
        else if( !s->compare( "round" ) )
            shape = EPAD::ROUND;
        else if( !s->compare( "octagon" ) )
            shape = EPAD::OCTAGON;
        else if( !s->compare( "long" ) )
            shape = EPAD::LONG;
        else if( !s->compare( "offset" ) )
            shape = EPAD::OFFSET;
    }

    rot      = parseOptionalEROT( attribs );
    stop     = parseOptionalBool( attribs, "stop" );
    thermals = parseOptionalBool( attribs, "thermals" );
    first    = parseOptionalBool( attribs, "first" );
}


/// Eagle SMD pad
struct ESMD
{
    string      name;
    double      x;
    double      y;
    double      dx;
    double      dy;
    int         layer;
    opt_int     roundness;
    opt_erot    rot;
    opt_bool    stop;
    opt_bool    thermals;
    opt_bool    cream;

    ESMD( CPTREE& aSMD );
};

ESMD::ESMD( CPTREE& aSMD )
{
    CPTREE& attribs = aSMD.get_child( "<xmlattr>" );

    /*
    <!ATTLIST smd
          name          %String;       #REQUIRED
          x             %Coord;        #REQUIRED
          y             %Coord;        #REQUIRED
          dx            %Dimension;    #REQUIRED
          dy            %Dimension;    #REQUIRED
          layer         %Layer;        #REQUIRED
          roundness     %Int;          "0"
          rot           %Rotation;     "R0"
          stop          %Bool;         "yes"
          thermals      %Bool;         "yes"
          cream         %Bool;         "yes"
          >
    */

    // DTD #REQUIRED, throw exception if not found
    name  = attribs.get<string>( "name" );
    x     = attribs.get<double>( "x" );
    y     = attribs.get<double>( "y" );
    dx    = attribs.get<double>( "dx" );
    dy    = attribs.get<double>( "dy" );
    layer = attribs.get<int>( "layer" );
    rot   = parseOptionalEROT( attribs );

    roundness = attribs.get_optional<int>( "roundness" );
    thermals  = parseOptionalBool( attribs, "thermals" );
    stop      = parseOptionalBool( attribs, "stop" );
    thermals  = parseOptionalBool( attribs, "thermals" );
    cream     = parseOptionalBool( attribs, "cream" );
}


struct EVERTEX
{
    double      x;
    double      y;

    EVERTEX( CPTREE& aVertex );
};

EVERTEX::EVERTEX( CPTREE& aVertex )
{
    CPTREE& attribs = aVertex.get_child( "<xmlattr>" );

    /*
    <!ELEMENT vertex EMPTY>
    <!ATTLIST vertex
          x             %Coord;        #REQUIRED
          y             %Coord;        #REQUIRED
          curve         %WireCurve;    "0" -- the curvature from this vertex to the next one --
          >
    */

    x = attribs.get<double>( "x" );
    y = attribs.get<double>( "y" );
}


/// Eagle polygon, without vertices which are parsed as needed
struct EPOLYGON
{
    double      width;
    int         layer;
    opt_double  spacing;

    // KiCad priority is opposite of Eagle rank, that is:
    //  - Eagle Low rank drawn first
    //  - KiCad high priority drawn first
    // So since Eagle has an upper limit we define this, used for the cases
    // where no rank is specified.
    static const int    max_priority = 6;

    enum {      // for pour
        SOLID,
        HATCH,
        CUTOUT,
    };
    int         pour;
    opt_double  isolate;
    opt_bool    orphans;
    opt_bool    thermals;
    opt_int     rank;

    EPOLYGON( CPTREE& aPolygon );
};

EPOLYGON::EPOLYGON( CPTREE& aPolygon )
{
    CPTREE& attribs = aPolygon.get_child( "<xmlattr>" );

    /*
    <!ATTLIST polygon
          width         %Dimension;    #REQUIRED
          layer         %Layer;        #REQUIRED
          spacing       %Dimension;    #IMPLIED
          pour          %PolygonPour;  "solid"
          isolate       %Dimension;    #IMPLIED -- only in <signal> or <package> context --
          orphans       %Bool;         "no"  -- only in <signal> context --
          thermals      %Bool;         "yes" -- only in <signal> context --
          rank          %Int;          "0"   -- 1..6 in <signal> context, 0 or 7 in <package> context --
          >
    */

    width   = attribs.get<double>( "width" );
    layer   = attribs.get<int>( "layer" );
    spacing = attribs.get_optional<double>( "spacing" );
    isolate = attribs.get_optional<double>( "isolate" );
    // default pour to solid fill
    pour    = EPOLYGON::SOLID;
    opt_string s = attribs.get_optional<string>( "pour" );

    if( s )
    {
        // (solid | hatch | cutout)
        if( !s->compare( "hatch" ) )
            pour = EPOLYGON::HATCH;
        else if( !s->compare( "cutout" ) )
            pour = EPOLYGON::CUTOUT;
    }

    orphans  = parseOptionalBool( attribs, "orphans" );
    thermals = parseOptionalBool( attribs, "thermals" );
    rank     = attribs.get_optional<int>( "rank" );
}

/// Eagle hole element
struct EHOLE
{
    double      x;
    double      y;
    double      drill;

    EHOLE( CPTREE& aHole );
};

EHOLE::EHOLE( CPTREE& aHole )
{
    CPTREE& attribs = aHole.get_child( "<xmlattr>" );

    /*
    <!ELEMENT hole EMPTY>
    <!ATTLIST hole
          x             %Coord;        #REQUIRED
          y             %Coord;        #REQUIRED
          drill         %Dimension;    #REQUIRED
          >
    */

    // #REQUIRED:
    x     = attribs.get<double>( "x" );
    y     = attribs.get<double>( "y" );
    drill = attribs.get<double>( "drill" );
}


/// Eagle element element
struct EELEMENT
{
    string      name;
    string      library;
    string      package;
    string      value;
    double      x;
    double      y;
    opt_bool    locked;
    opt_bool    smashed;
    opt_erot    rot;

    EELEMENT( CPTREE& aElement );
};

EELEMENT::EELEMENT( CPTREE& aElement )
{
    CPTREE& attribs = aElement.get_child( "<xmlattr>" );

    /*
    <!ELEMENT element (attribute*, variant*)>
    <!ATTLIST element
          name          %String;       #REQUIRED
          library       %String;       #REQUIRED
          package       %String;       #REQUIRED
          value         %String;       #REQUIRED
          x             %Coord;        #REQUIRED
          y             %Coord;        #REQUIRED
          locked        %Bool;         "no"
          smashed       %Bool;         "no"
          rot           %Rotation;     "R0"
          >
    */

    // #REQUIRED
    name    = attribs.get<string>( "name" );
    library = attribs.get<string>( "library" );
    value   = attribs.get<string>( "value" );

    package = attribs.get<string>( "package" );
    ReplaceIllegalFileNameChars( &package );

    x = attribs.get<double>( "x" );
    y = attribs.get<double>( "y" );

    // optional
    locked  = parseOptionalBool( attribs, "locked" );
    smashed = parseOptionalBool( attribs, "smashed" );
    rot = parseOptionalEROT( attribs );
}


struct ELAYER
{
    int         number;
    string      name;
    int         color;
    int         fill;
    opt_bool    visible;
    opt_bool    active;

    ELAYER( CPTREE& aLayer );
};

ELAYER::ELAYER( CPTREE& aLayer )
{
    CPTREE& attribs = aLayer.get_child( "<xmlattr>" );

    /*
    <!ELEMENT layer EMPTY>
    <!ATTLIST layer
          number        %Layer;        #REQUIRED
          name          %String;       #REQUIRED
          color         %Int;          #REQUIRED
          fill          %Int;          #REQUIRED
          visible       %Bool;         "yes"
          active        %Bool;         "yes"
          >
    */

    number = attribs.get<int>( "number" );
    name   = attribs.get<string>( "name" );
    color  = attribs.get<int>( "color" );
    fill   = 1;    // Temporary value.
    visible = parseOptionalBool( attribs, "visible" );
    active  = parseOptionalBool( attribs, "active" );
}


/// Parse an eagle distance which is either mm, or mils if there is "mil" suffix.
/// Return is in BIU.
static double parseEagle( const string& aDistance )
{
    double ret = strtod( aDistance.c_str(), NULL );
    if( aDistance.npos != aDistance.find( "mil" ) )
        ret = IU_PER_MILS * ret;
    else
        ret = IU_PER_MM * ret;

    return ret;
}


/// subset of eagle.drawing.board.designrules in the XML document
struct ERULES
{
    int         psElongationLong;   ///< percent over 100%.  0-> not elongated, 100->twice as wide as is tall
                                    ///< Goes into making a scaling factor for "long" pads.

    int         psElongationOffset; ///< the offset of the hole within the "long" pad.

    double      rvPadTop;           ///< top pad size as percent of drill size
    // double   rvPadBottom;        ///< bottom pad size as percent of drill size

    double      rlMinPadTop;        ///< minimum copper annulus on through hole pads
    double      rlMaxPadTop;        ///< maximum copper annulus on through hole pads

    double      rvViaOuter;         ///< copper annulus is this percent of via hole
    double      rlMinViaOuter;      ///< minimum copper annulus on via
    double      rlMaxViaOuter;      ///< maximum copper annulus on via
    double      mdWireWire;         ///< wire to wire spacing I presume.


    ERULES() :
        psElongationLong    ( 100 ),
        psElongationOffset  ( 0 ),
        rvPadTop            ( 0.25 ),
        // rvPadBottom      ( 0.25 ),
        rlMinPadTop         ( Mils2iu( 10 ) ),
        rlMaxPadTop         ( Mils2iu( 20 ) ),

        rvViaOuter          ( 0.25 ),
        rlMinViaOuter       ( Mils2iu( 10 ) ),
        rlMaxViaOuter       ( Mils2iu( 20 ) ),
        mdWireWire          ( 0 )
    {}

    void parse( CPTREE& aRules );
};

void ERULES::parse( CPTREE& aRules )
{
    for( CITER it = aRules.begin();  it != aRules.end();  ++it )
    {
        if( it->first != "param" )
            continue;

        CPTREE& attribs = it->second.get_child( "<xmlattr>" );

        const string& name = attribs.get<string>( "name" );

        if( name == "psElongationLong" )
            psElongationLong = attribs.get<int>( "value" );
        else if( name == "psElongationOffset" )
            psElongationOffset = attribs.get<int>( "value" );
        else if( name == "rvPadTop" )
            rvPadTop = attribs.get<double>( "value" );
        else if( name == "rlMinPadTop" )
            rlMinPadTop = parseEagle( attribs.get<string>( "value" ) );
        else if( name == "rlMaxPadTop" )
            rlMaxPadTop = parseEagle( attribs.get<string>( "value" ) );
        else if( name == "rvViaOuter" )
            rvViaOuter = attribs.get<double>( "value" );
        else if( name == "rlMinViaOuter" )
            rlMinViaOuter = parseEagle( attribs.get<string>( "value" ) );
        else if( name == "rlMaxViaOuter" )
            rlMaxViaOuter = parseEagle( attribs.get<string>( "value" ) );
        else if( name == "mdWireWire" )
            mdWireWire = parseEagle( attribs.get<string>( "value" ) );
    }
}


/// Assemble a two part key as a simple concatonation of aFirst and aSecond parts,
/// using a separator.
static inline string makeKey( const string& aFirst, const string& aSecond )
{
    string key = aFirst + '\x02' +  aSecond;
    return key;
}


/// Make a unique time stamp
static inline unsigned long timeStamp( CPTREE& aTree )
{
    // in this case from a unique tree memory location
    return (unsigned long)(void*) &aTree;
}


/// Convert an Eagle curve end to a KiCad center for S_ARC
wxPoint kicad_arc_center( wxPoint start, wxPoint end, double angle )
{
    // Eagle give us start and end.
    // S_ARC wants start to give the center, and end to give the start.
    double dx = end.x - start.x, dy = end.y - start.y;
    wxPoint mid = (start + end) / 2;

    double dlen = sqrt( dx*dx + dy*dy );
    double dist = dlen / ( 2 * tan( DEG2RAD( angle ) / 2 ) );

    wxPoint center(
        mid.x + dist * ( dy / dlen ),
        mid.y - dist * ( dx / dlen )
    );

    return center;
}


EAGLE_PLUGIN::EAGLE_PLUGIN() :
    m_rules( new ERULES() ),
    m_xpath( new XPATH() ),
    m_mod_time( wxDateTime::Now() )
{
    init( NULL );

    clear_cu_map();
}


EAGLE_PLUGIN::~EAGLE_PLUGIN()
{
    delete m_rules;
    delete m_xpath;
}


const wxString EAGLE_PLUGIN::PluginName() const
{
    return wxT( "Eagle" );
}


const wxString EAGLE_PLUGIN::GetFileExtension() const
{
    return wxT( "brd" );
}


int inline EAGLE_PLUGIN::kicad( double d ) const
{
    return KiROUND( biu_per_mm * d );
}


wxSize inline EAGLE_PLUGIN::kicad_fontz( double d ) const
{
    // texts seem to better match eagle when scaled down by 0.95
    int kz = kicad( d ) * 95 / 100;
    return wxSize( kz, kz );
}


BOARD* EAGLE_PLUGIN::Load( const wxString& aFileName, BOARD* aAppendToMe,  const PROPERTIES* aProperties )
{
    LOCALE_IO   toggle;     // toggles on, then off, the C locale.
    PTREE       doc;

    init( aProperties );

    m_board = aAppendToMe ? aAppendToMe : new BOARD();

    // Give the filename to the board if it's new
    if( !aAppendToMe )
        m_board->SetFileName( aFileName );

    // delete on exception, iff I own m_board, according to aAppendToMe
    auto_ptr<BOARD> deleter( aAppendToMe ? NULL : m_board );

    try
    {
        // 8 bit "filename" should be encoded according to disk filename encoding,
        // (maybe this is current locale, maybe not, its a filesystem issue),
        // and is not necessarily utf8.
        string filename = (const char*) aFileName.char_str( wxConvFile );

        read_xml( filename, doc, xml_parser::no_comments );

        m_min_trace    = INT_MAX;
        m_min_via      = INT_MAX;
        m_min_via_hole = INT_MAX;

        loadAllSections( doc );

        BOARD_DESIGN_SETTINGS& designSettings = m_board->GetDesignSettings();

        if( m_min_trace < designSettings.m_TrackMinWidth )
            designSettings.m_TrackMinWidth = m_min_trace;

        if( m_min_via < designSettings.m_ViasMinSize )
            designSettings.m_ViasMinSize = m_min_via;

        if( m_min_via_hole < designSettings.m_ViasMinDrill )
            designSettings.m_ViasMinDrill = m_min_via_hole;

        if( m_rules->mdWireWire )
        {
            NETCLASSPTR defaultNetclass = designSettings.GetDefault();
            int         clearance = KiROUND( m_rules->mdWireWire );

            if( clearance < defaultNetclass->GetClearance() )
                defaultNetclass->SetClearance( clearance );
        }

        // should be empty, else missing m_xpath->pop()
        wxASSERT( m_xpath->Contents().size() == 0 );
    }

    catch( file_parser_error fpe )
    {
        // for xml_parser_error, what() has the line number in it,
        // but no byte offset.  That should be an adequate error message.
        THROW_IO_ERROR( fpe.what() );
    }

    // Class ptree_error is a base class for xml_parser_error & file_parser_error,
    // so one catch should be OK for all errors.
    catch( ptree_error pte )
    {
        string errmsg = pte.what();

        errmsg += " @\n";
        errmsg += m_xpath->Contents();

        THROW_IO_ERROR( errmsg );
    }

    // IO_ERROR exceptions are left uncaught, they pass upwards from here.

    // Ensure the copper layers count is a multiple of 2
    // Pcbnew does not like boards with odd layers count
    // (these boards cannot exist. they actually have a even layers count)
    int lyrcnt = m_board->GetCopperLayerCount();

    if( (lyrcnt % 2) != 0 )
    {
        lyrcnt++;
        m_board->SetCopperLayerCount( lyrcnt );
    }

    centerBoard();

    deleter.release();
    return m_board;
}


void EAGLE_PLUGIN::init( const PROPERTIES* aProperties )
{
    m_hole_count   = 0;
    m_min_trace    = 0;
    m_min_via      = 0;
    m_min_via_hole = 0;
    m_xpath->clear();
    m_pads_to_nets.clear();

    // m_templates.clear();     this is the FOOTPRINT cache too

    m_board = NULL;
    m_props = aProperties;

    mm_per_biu = 1/IU_PER_MM;
    biu_per_mm = IU_PER_MM;

    delete m_rules;
    m_rules = new ERULES();
}


void EAGLE_PLUGIN::clear_cu_map()
{
    // All cu layers are invalid until we see them in the <layers> section while
    // loading either a board or library.  See loadLayerDefs().
    for( unsigned i = 0;  i < DIM(m_cu_map);  ++i )
        m_cu_map[i] = -1;
}


void EAGLE_PLUGIN::loadAllSections( CPTREE& aDoc )
{
    CPTREE& drawing = aDoc.get_child( "eagle.drawing" );
    CPTREE& board   = drawing.get_child( "board" );

    m_xpath->push( "eagle.drawing" );

    {
        m_xpath->push( "board" );

        CPTREE& designrules = board.get_child( "designrules" );
        loadDesignRules( designrules );

        m_xpath->pop();
    }

    {
        m_xpath->push( "layers" );

        CPTREE& layers = drawing.get_child( "layers" );
        loadLayerDefs( layers );

        m_xpath->pop();
    }

    {
        m_xpath->push( "board" );

        CPTREE& plain = board.get_child( "plain" );
        loadPlain( plain );

        CPTREE&  signals = board.get_child( "signals" );
        loadSignals( signals );

        CPTREE&  libs = board.get_child( "libraries" );
        loadLibraries( libs );

        CPTREE& elems = board.get_child( "elements" );
        loadElements( elems );

        m_xpath->pop();     // "board"
    }

    m_xpath->pop();     // "eagle.drawing"
}


void EAGLE_PLUGIN::loadDesignRules( CPTREE& aDesignRules )
{
    m_xpath->push( "designrules" );
    m_rules->parse( aDesignRules );
    m_xpath->pop();     // "designrules"
}


void EAGLE_PLUGIN::loadLayerDefs( CPTREE& aLayers )
{
    typedef std::vector<ELAYER>     ELAYERS;
    typedef ELAYERS::const_iterator EITER;

    ELAYERS     cu;  // copper layers

    // find the subset of layers that are copper, and active
    for( CITER layer = aLayers.begin();  layer != aLayers.end();  ++layer )
    {
        ELAYER  elayer( layer->second );

        if( elayer.number >= 1 && elayer.number <= 16 && ( !elayer.active || *elayer.active ) )
        {
            cu.push_back( elayer );
        }
    }

    // establish cu layer map:
    int ki_layer_count = 0;

    for( EITER it = cu.begin();  it != cu.end();  ++it,  ++ki_layer_count )
    {
        if( ki_layer_count == 0 )
            m_cu_map[it->number] = F_Cu;
        else if( ki_layer_count == int( cu.size()-1 ) )
            m_cu_map[it->number] = B_Cu;
        else
        {
            // some eagle boards do not have contiguous layer number sequences.

#if 0   // pre LAYER_ID & LSET:
            m_cu_map[it->number] = cu.size() - 1 - ki_layer_count;
#else
            m_cu_map[it->number] = ki_layer_count;
#endif
        }
    }

#if 0 && defined(DEBUG)
    printf( "m_cu_map:\n" );
    for( unsigned i=0; i<DIM(m_cu_map);  ++i )
    {
        printf( "\t[%d]:%d\n", i, m_cu_map[i] );
    }
#endif

    // Set the layer names and cu count iff we're loading a board.
    if( m_board )
    {
        m_board->SetCopperLayerCount( cu.size() );

        for( EITER it = cu.begin();  it != cu.end();  ++it )
        {
            LAYER_ID layer =  kicad_layer( it->number );

            // these function provide their own protection against UNDEFINED_LAYER:
            m_board->SetLayerName( layer, FROM_UTF8( it->name.c_str() ) );
            m_board->SetLayerType( layer, LT_SIGNAL );

            // could map the colors here
        }
    }
}


void EAGLE_PLUGIN::loadPlain( CPTREE& aGraphics )
{
    m_xpath->push( "plain" );

    // (polygon | wire | text | circle | rectangle | frame | hole)*
    for( CITER gr = aGraphics.begin();  gr != aGraphics.end();  ++gr )
    {
        if( gr->first == "wire" )
        {
            m_xpath->push( "wire" );

            EWIRE       w( gr->second );
            LAYER_ID    layer = kicad_layer( w.layer );

            wxPoint start( kicad_x( w.x1 ), kicad_y( w.y1 ) );
            wxPoint end(   kicad_x( w.x2 ), kicad_y( w.y2 ) );

            if( layer != UNDEFINED_LAYER )
            {
                DRAWSEGMENT* dseg = new DRAWSEGMENT( m_board );
                m_board->Add( dseg, ADD_APPEND );

                if( !w.curve )
                {
                    dseg->SetStart( start );
                    dseg->SetEnd( end );
                }
                else
                {
                    wxPoint center = kicad_arc_center( start, end, *w.curve);

                    dseg->SetShape( S_ARC );
                    dseg->SetStart( center );
                    dseg->SetEnd( start );
                    dseg->SetAngle( *w.curve * -10.0 ); // KiCad rotates the other way
                }

                dseg->SetTimeStamp( timeStamp( gr->second ) );
                dseg->SetLayer( layer );
                dseg->SetWidth( Millimeter2iu( DEFAULT_PCB_EDGE_THICKNESS ) );
            }
            m_xpath->pop();
        }
        else if( gr->first == "text" )
        {
#if defined(DEBUG)
            if( gr->second.data() == "ATMEGA328" )
            {
                int breakhere = 1;
                (void) breakhere;
            }
#endif
            m_xpath->push( "text" );

            ETEXT       t( gr->second );
            LAYER_ID    layer = kicad_layer( t.layer );

            if( layer != UNDEFINED_LAYER )
            {
                TEXTE_PCB* pcbtxt = new TEXTE_PCB( m_board );
                m_board->Add( pcbtxt, ADD_APPEND );

                pcbtxt->SetLayer( layer );
                pcbtxt->SetTimeStamp( timeStamp( gr->second ) );
                pcbtxt->SetText( FROM_UTF8( t.text.c_str() ) );
                pcbtxt->SetTextPosition( wxPoint( kicad_x( t.x ), kicad_y( t.y ) ) );

                pcbtxt->SetSize( kicad_fontz( t.size ) );

                double  ratio = t.ratio ? *t.ratio : 8;     // DTD says 8 is default

                pcbtxt->SetThickness( kicad( t.size * ratio / 100 ) );

                int align = t.align ? *t.align : ETEXT::BOTTOM_LEFT;

                if( t.rot )
                {
                    int sign = t.rot->mirror ? -1 : 1;
                    pcbtxt->SetMirrored( t.rot->mirror );

                    double degrees = t.rot->degrees;

                    if( degrees == 90 || t.rot->spin )
                        pcbtxt->SetOrientation( sign * t.rot->degrees * 10 );
                    else if( degrees == 180 )
                        align = ETEXT::TOP_RIGHT;
                    else if( degrees == 270 )
                    {
                        pcbtxt->SetOrientation( sign * 90 * 10 );
                        align = ETEXT::TOP_RIGHT;
                    }
                }

                switch( align )
                {
                case ETEXT::CENTER:
                    // this was the default in pcbtxt's constructor
                    break;

                case ETEXT::CENTER_LEFT:
                    pcbtxt->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
                    break;

                case ETEXT::CENTER_RIGHT:
                    pcbtxt->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
                    break;

                case ETEXT::TOP_CENTER:
                    pcbtxt->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );
                    break;

                case ETEXT::TOP_LEFT:
                    pcbtxt->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
                    pcbtxt->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );
                    break;

                case ETEXT::TOP_RIGHT:
                    pcbtxt->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
                    pcbtxt->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );
                    break;

                case ETEXT::BOTTOM_CENTER:
                    pcbtxt->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
                    break;

                case ETEXT::BOTTOM_LEFT:
                    pcbtxt->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
                    pcbtxt->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
                    break;

                case ETEXT::BOTTOM_RIGHT:
                    pcbtxt->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
                    pcbtxt->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
                    break;
                }
            }
            m_xpath->pop();
        }
        else if( gr->first == "circle" )
        {
            m_xpath->push( "circle" );

            ECIRCLE     c( gr->second );
            LAYER_ID    layer = kicad_layer( c.layer );

            if( layer != UNDEFINED_LAYER )       // unsupported layer
            {
                DRAWSEGMENT* dseg = new DRAWSEGMENT( m_board );
                m_board->Add( dseg, ADD_APPEND );

                dseg->SetShape( S_CIRCLE );
                dseg->SetTimeStamp( timeStamp( gr->second ) );
                dseg->SetLayer( layer );
                dseg->SetStart( wxPoint( kicad_x( c.x ), kicad_y( c.y ) ) );
                dseg->SetEnd( wxPoint( kicad_x( c.x + c.radius ), kicad_y( c.y ) ) );
                dseg->SetWidth( kicad( c.width ) );
            }
            m_xpath->pop();
        }
        else if( gr->first == "rectangle" )
        {
            // This seems to be a simplified rectangular [copper] zone, cannot find any
            // net related info on it from the DTD.
            m_xpath->push( "rectangle" );

            ERECT       r( gr->second );
            LAYER_ID    layer = kicad_layer( r.layer );

            if( IsCopperLayer( layer ) )
            {
                // use a "netcode = 0" type ZONE:
                ZONE_CONTAINER* zone = new ZONE_CONTAINER( m_board );
                m_board->Add( zone, ADD_APPEND );

                zone->SetTimeStamp( timeStamp( gr->second ) );
                zone->SetLayer( layer );
                zone->SetNetCode( NETINFO_LIST::UNCONNECTED );

                CPolyLine::HATCH_STYLE outline_hatch = CPolyLine::DIAGONAL_EDGE;

                zone->Outline()->Start( layer, kicad_x( r.x1 ), kicad_y( r.y1 ), outline_hatch );
                zone->AppendCorner( wxPoint( kicad_x( r.x2 ), kicad_y( r.y1 ) ) );
                zone->AppendCorner( wxPoint( kicad_x( r.x2 ), kicad_y( r.y2 ) ) );
                zone->AppendCorner( wxPoint( kicad_x( r.x1 ), kicad_y( r.y2 ) ) );
                zone->Outline()->CloseLastContour();

                // this is not my fault:
                zone->Outline()->SetHatch(
                        outline_hatch, Mils2iu( zone->Outline()->GetDefaultHatchPitchMils() ), true );
            }

            m_xpath->pop();
        }
        else if( gr->first == "hole" )
        {
            m_xpath->push( "hole" );
            EHOLE   e( gr->second );

            // Fabricate a MODULE with a single PAD_ATTRIB_HOLE_NOT_PLATED pad.
            // Use m_hole_count to gen up a unique name.

            MODULE* module = new MODULE( m_board );
            m_board->Add( module, ADD_APPEND );

            char    temp[40];
            sprintf( temp, "@HOLE%d", m_hole_count++ );
            module->SetReference( FROM_UTF8( temp ) );
            module->Reference().SetVisible( false );

            wxPoint pos( kicad_x( e.x ), kicad_y( e.y ) );

            module->SetPosition( pos );

            // Add a PAD_ATTRIB_HOLE_NOT_PLATED pad to this module.
            D_PAD* pad = new D_PAD( module );
            module->Pads().PushBack( pad );

            pad->SetShape( PAD_SHAPE_CIRCLE );
            pad->SetAttribute( PAD_ATTRIB_HOLE_NOT_PLATED );

            /* pad's position is already centered on module at relative (0, 0)
            wxPoint padpos( kicad_x( e.x ), kicad_y( e.y ) );

            pad->SetPos0( padpos );
            pad->SetPosition( padpos + module->GetPosition() );
            */

            wxSize  sz( kicad( e.drill ), kicad( e.drill ) );

            pad->SetDrillSize( sz );
            pad->SetSize( sz );

            pad->SetLayerSet( LSET::AllCuMask() );
            m_xpath->pop();
        }
        else if( gr->first == "frame" )
        {
            // picture this
        }
        else if( gr->first == "polygon" )
        {
            // could be on a copper layer, could be on another layer.
            // copper layer would be done using netCode=0 type of ZONE_CONTAINER.
        }
        else if( gr->first == "dimension" )
        {
            EDIMENSION d( gr->second );

            DIMENSION* dimension = new DIMENSION( m_board );
            m_board->Add( dimension, ADD_APPEND );

            dimension->SetLayer( kicad_layer( d.layer ) );
            // The origin and end are assumed to always be in this order from eagle
            dimension->SetOrigin( wxPoint( kicad_x( d.x1 ), kicad_y( d.y1 ) ) );
            dimension->SetEnd( wxPoint( kicad_x( d.x2 ), kicad_y( d.y2 ) ) );
            dimension->Text().SetSize( m_board->GetDesignSettings().m_PcbTextSize );

            int width = m_board->GetDesignSettings().m_PcbTextWidth;
            int maxThickness = Clamp_Text_PenSize( width, dimension->Text().GetSize() );

            if( width > maxThickness )
                width = maxThickness;

            dimension->Text().SetThickness( width );
            dimension->SetWidth( width );

            // check which axis the dimension runs in
            // because the "height" of the dimension is perpendicular to that axis
            // Note the check is just if two axes are close enough to each other
            // Eagle appears to have some rounding errors
            if( fabs( d.x1 - d.x2 ) < 0.05 )
                dimension->SetHeight( kicad_x( d.x1 - d.x3 ) );
            else
                dimension->SetHeight( kicad_y( d.y3 - d.y1 ) );

            dimension->AdjustDimensionDetails();
         }
    }
    m_xpath->pop();
}


void EAGLE_PLUGIN::loadLibrary( CPTREE& aLib, const string* aLibName )
{
    m_xpath->push( "packages" );

    // library will have <xmlattr> node, skip that and get the single packages node
    CPTREE& packages = aLib.get_child( "packages" );

    // Create a MODULE for all the eagle packages, for use later via a copy constructor
    // to instantiate needed MODULES in our BOARD.  Save the MODULE templates in
    // a MODULE_MAP using a single lookup key consisting of libname+pkgname.

    for( CITER package = packages.begin();  package != packages.end();  ++package )
    {
        m_xpath->push( "package", "name" );

        const string& pack_ref = package->second.get<string>( "<xmlattr>.name" );

        string pack_name( pack_ref );

        ReplaceIllegalFileNameChars( &pack_name );

#if 0 && defined(DEBUG)
        if( pack_name == "TO220H" )
        {
            int breakhere = 1;
            (void) breakhere;
        }
#endif
        m_xpath->Value( pack_name.c_str() );

        string key = aLibName ? makeKey( *aLibName, pack_name ) : pack_name;

        MODULE* m = makeModule( package->second, pack_name );

        // add the templating MODULE to the MODULE template factory "m_templates"
        std::pair<MODULE_ITER, bool> r = m_templates.insert( key, m );

        if( !r.second
            // && !( m_props && m_props->Value( "ignore_duplicates" ) )
            )
        {
            wxString lib = aLibName ? FROM_UTF8( aLibName->c_str() ) : m_lib_path;
            wxString pkg = FROM_UTF8( pack_name.c_str() );

            wxString emsg = wxString::Format(
                _( "<package> name: '%s' duplicated in eagle <library>: '%s'" ),
                GetChars( pkg ),
                GetChars( lib )
                );
            THROW_IO_ERROR( emsg );
        }

        m_xpath->pop();
    }

    m_xpath->pop();     // "packages"
}


void EAGLE_PLUGIN::loadLibraries( CPTREE& aLibs )
{
    m_xpath->push( "libraries.library", "name" );

    for( CITER library = aLibs.begin();  library != aLibs.end();  ++library )
    {
        const string& lib_name = library->second.get<string>( "<xmlattr>.name" );

        m_xpath->Value( lib_name.c_str() );

        loadLibrary( library->second, &lib_name );
    }

    m_xpath->pop();
}


void EAGLE_PLUGIN::loadElements( CPTREE& aElements )
{
    m_xpath->push( "elements.element", "name" );

    EATTR   name;
    EATTR   value;
    bool refanceNamePresetInPackageLayout;
    bool valueNamePresetInPackageLayout;



    for( CITER it = aElements.begin();  it != aElements.end();  ++it )
    {
        if( it->first != "element" )
            continue;

        EELEMENT    e( it->second );

        // use "NULL-ness" as an indication of presence of the attribute:
        EATTR*      nameAttr  = 0;
        EATTR*      valueAttr = 0;

        m_xpath->Value( e.name.c_str() );

        string key = makeKey( e.library, e.package );

        MODULE_CITER mi = m_templates.find( key );

        if( mi == m_templates.end() )
        {
            wxString emsg = wxString::Format( _( "No '%s' package in library '%s'" ),
                                              GetChars( FROM_UTF8( e.package.c_str() ) ),
                                              GetChars( FROM_UTF8( e.library.c_str() ) ) );
            THROW_IO_ERROR( emsg );
        }

#if defined(DEBUG)
        if( e.name == "ARM_C8" )
        {
            int breakhere = 1;
            (void) breakhere;
        }
#endif
        // copy constructor to clone the template
        MODULE* m = new MODULE( *mi->second );
        m_board->Add( m, ADD_APPEND );

        // update the nets within the pads of the clone
        for( D_PAD* pad = m->Pads();  pad;  pad = pad->Next() )
        {
            string key  = makeKey( e.name, TO_UTF8( pad->GetPadName() ) );

            NET_MAP_CITER ni = m_pads_to_nets.find( key );
            if( ni != m_pads_to_nets.end() )
            {
                const ENET* enet = &ni->second;
                pad->SetNetCode( enet->netcode );
            }
        }

        refanceNamePresetInPackageLayout = true;
        valueNamePresetInPackageLayout = true;
        m->SetPosition( wxPoint( kicad_x( e.x ), kicad_y( e.y ) ) );
        // Is >NAME field set in package layout ?
        if( m->GetReference().size() == 0 )
        {
            m->Reference().SetVisible( false ); // No so no show
            refanceNamePresetInPackageLayout = false;
        }
        // Is >VALUE field set in package layout
        if( m->GetValue().size() == 0 )
        {
            m->Value().SetVisible( false );     // No so no show
            valueNamePresetInPackageLayout = false;
        }
        m->SetReference( FROM_UTF8( e.name.c_str() ) );
        m->SetValue( FROM_UTF8( e.value.c_str() ) );

        if( !e.smashed )
        { // Not smashed so show NAME & VALUE
            if( valueNamePresetInPackageLayout )
                m->Value().SetVisible( true );  // Only if place holder in package layout
            if( refanceNamePresetInPackageLayout )
                m->Reference().SetVisible( true );   // Only if place holder in package layout
        }
        else if( *e.smashed == true )
        { // Smasted so set default to no show for NAME and VALUE
            m->Value().SetVisible( false );
            m->Reference().SetVisible( false );

            // initalize these to default values incase the <attribute> elements are not present.
            m_xpath->push( "attribute", "name" );

            // VALUE and NAME can have something like our text "effects" overrides
            // in SWEET and new schematic.  Eagle calls these XML elements "attribute".
            // There can be one for NAME and/or VALUE both.  Features present in the
            // EATTR override the ones established in the package only if they are
            // present here (except for rot, which if not present means angle zero).
            // So the logic is a bit different than in packageText() and in plain text.
            for( CITER ait = it->second.begin();  ait != it->second.end();  ++ait )
            {

                if( ait->first != "attribute" )
                    continue;

                EATTR   a( ait->second );

                if( a.name == "NAME" )
                {
                    name = a;
                    nameAttr = &name;

                    // do we have a display attribute ?
                    if( a.display  )
                    {
                        // Yes!
                        switch( *a.display )
                        {
                        case EATTR::VALUE :
                            nameAttr->name = e.name;
                            m->SetReference( e.name );
                            if( refanceNamePresetInPackageLayout )
                                m->Reference().SetVisible( true );
                            break;

                        case EATTR::NAME :
                            if( refanceNamePresetInPackageLayout )
                            {
                                m->SetReference( "NAME" );
                                m->Reference().SetVisible( true );
                            }
                            break;

                        case EATTR::BOTH :
                            if( refanceNamePresetInPackageLayout )
                                m->Reference().SetVisible( true );
                            nameAttr->name =  nameAttr->name + " = " + e.name;
                            m->SetReference( "NAME = " + e.name );
                            break;

                        case EATTR::Off :
                            m->Reference().SetVisible( false );
                            break;

                        default:
                            nameAttr->name =  e.name;
                            if( refanceNamePresetInPackageLayout )
                                m->Reference().SetVisible( true );
                        }
                    }
                    else
                        // No display, so default is visable, and show value of NAME
                        m->Reference().SetVisible( true );
                }
                else if( a.name == "VALUE" )
                {
                    value = a;
                    valueAttr = &value;

                    if( a.display  )
                    {
                        // Yes!
                        switch( *a.display )
                        {
                        case EATTR::VALUE :
                            valueAttr->value = e.value;
                            m->SetValue( e.value );
                            if( valueNamePresetInPackageLayout )
                                m->Value().SetVisible( true );
                            break;

                        case EATTR::NAME :
                            if( valueNamePresetInPackageLayout )
                                m->Value().SetVisible( true );
                            m->SetValue( "VALUE" );
                            break;

                        case EATTR::BOTH :
                            if( valueNamePresetInPackageLayout )
                                m->Value().SetVisible( true );
                            valueAttr->value = "VALUE = " + e.value;
                            m->SetValue( "VALUE = " + e.value );
                            break;

                        case EATTR::Off :
                            m->Value().SetVisible( false );
                            break;

                        default:
                            valueAttr->value =  e.value;
                            if( valueNamePresetInPackageLayout )
                                m->Value().SetVisible( true );
                        }
                    }
                    else
                        // No display, so default is visible, and show value of NAME
                        m->Value().SetVisible( true );

                }
            }

            m_xpath->pop();     // "attribute"
        }

        orientModuleAndText( m, e, nameAttr, valueAttr );
    }

    m_xpath->pop();     // "elements.element"
}


void EAGLE_PLUGIN::orientModuleAndText( MODULE* m, const EELEMENT& e,
                    const EATTR* nameAttr, const EATTR* valueAttr )
{
    if( e.rot )
    {
        if( e.rot->mirror )
        {
            double orientation = e.rot->degrees + 180.0;
            m->SetOrientation( orientation * 10 );
            m->Flip( m->GetPosition() );
        }
        else
            m->SetOrientation( e.rot->degrees * 10 );
    }

    orientModuleText( m, e, &m->Reference(), nameAttr );
    orientModuleText( m, e, &m->Value(), valueAttr );
}


void EAGLE_PLUGIN::orientModuleText( MODULE* m, const EELEMENT& e,
                            TEXTE_MODULE* txt, const EATTR* aAttr )
{
    // Smashed part ?
    if( aAttr )
    { // Yes
        const EATTR& a = *aAttr;

        if( a.value )
        {
            txt->SetText( FROM_UTF8( a.value->c_str() ) );
        }

        if( a.x && a.y )    // boost::optional
        {
            wxPoint pos( kicad_x( *a.x ), kicad_y( *a.y ) );
            txt->SetTextPosition( pos );
        }

        // Even though size and ratio are both optional, I am not seeing
        // a case where ratio is present but size is not.
        double  ratio = 8;
        wxSize  fontz = txt->GetSize();

        if( a.size )
        {
            fontz = kicad_fontz( *a.size );
            txt->SetSize( fontz );

            if( a.ratio )
                ratio = *a.ratio;
        }

        int  lw = int( fontz.y * ratio / 100.0 );
        txt->SetThickness( lw );

        int align = ETEXT::BOTTOM_LEFT;     // bottom-left is eagle default

        // The "rot" in a EATTR seems to be assumed to be zero if it is not
        // present, and this zero rotation becomes an override to the
        // package's text field.  If they did not want zero, they specify
        // what they want explicitly.
        double  degrees  = a.rot ? a.rot->degrees : 0;
        double  orient;      // relative to parent

        int     sign = 1;
        bool    spin = false;

        if( a.rot )
        {
            spin = a.rot->spin;
            sign = a.rot->mirror ? -1 : 1;
            txt->SetMirrored( a.rot->mirror );
        }

        if( degrees == 90 || degrees == 0 || spin )
        {
            orient = degrees - m->GetOrientation() / 10;
            txt->SetOrientation( sign * orient * 10 );
        }
        else if( degrees == 180 )
        {
            orient = 0 - m->GetOrientation() / 10;
            txt->SetOrientation( sign * orient * 10 );
            align = ETEXT::TOP_RIGHT;
        }
        else if( degrees == 270 )
        {
            orient = 90 - m->GetOrientation() / 10;
            align = ETEXT::TOP_RIGHT;
            txt->SetOrientation( sign * orient * 10 );
        }
        else
        {
            orient = 90 - degrees - m->GetOrientation() / 10;
            txt->SetOrientation( sign * orient * 10 );
        }

        switch( align )
        {
        case ETEXT::TOP_RIGHT:
            txt->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
            txt->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );
            break;

        case ETEXT::BOTTOM_LEFT:
            txt->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
            txt->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
            break;

        default:
            ;
        }
    }
    else    // Part is not smash so use Lib default for NAME/VALUE // the text is per the original package, sans <attribute>
    {
        double degrees = ( txt->GetOrientation() + m->GetOrientation() ) / 10;

        // @todo there are a few more cases than these to contend with:
        if( (!txt->IsMirrored() && ( abs( degrees ) == 180 || abs( degrees ) == 270 ))
         || ( txt->IsMirrored() && ( degrees == 360 ) ) )
        {
            // ETEXT::TOP_RIGHT:
            txt->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
            txt->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );
        }
    }
}

MODULE* EAGLE_PLUGIN::makeModule( CPTREE& aPackage, const string& aPkgName ) const
{
    std::auto_ptr<MODULE>   m( new MODULE( m_board ) );

    m->SetFPID( FPID( aPkgName ) );

    opt_string description = aPackage.get_optional<string>( "description" );
    if( description )
        m->SetDescription( FROM_UTF8( description->c_str() ) );

    for( CITER it = aPackage.begin();  it != aPackage.end();  ++it )
    {
        CPTREE& t = it->second;

        if( it->first == "wire" )
            packageWire( m.get(), t );

        else if( it->first == "pad" )
            packagePad( m.get(), t );

        else if( it->first == "text" )
            packageText( m.get(), t );

        else if( it->first == "rectangle" )
            packageRectangle( m.get(), t );

        else if( it->first == "polygon" )
            packagePolygon( m.get(), t );

        else if( it->first == "circle" )
            packageCircle( m.get(), t );

        else if( it->first == "hole" )
            packageHole( m.get(), t );

        else if( it->first == "smd" )
            packageSMD( m.get(), t );
    }

    return m.release();
}


void EAGLE_PLUGIN::packageWire( MODULE* aModule, CPTREE& aTree ) const
{
    EWIRE       w( aTree );
    LAYER_ID    layer = kicad_layer( w.layer );

    if( IsNonCopperLayer( layer ) )     // only valid non-copper wires, skip copper package wires
    {
        wxPoint start( kicad_x( w.x1 ), kicad_y( w.y1 ) );
        wxPoint end(   kicad_x( w.x2 ), kicad_y( w.y2 ) );
        int     width = kicad( w.width );

        // FIXME: the cap attribute is ignored because kicad can't create lines
        //        with flat ends.
        EDGE_MODULE* dwg;
        if( !w.curve )
        {
            dwg = new EDGE_MODULE( aModule, S_SEGMENT );

            dwg->SetStart0( start );
            dwg->SetEnd0( end );
        }
        else
        {
            dwg = new EDGE_MODULE( aModule, S_ARC );
            wxPoint center = kicad_arc_center( start, end, *w.curve);

            dwg->SetStart0( center );
            dwg->SetEnd0( start );
            dwg->SetAngle( *w.curve * -10.0 ); // KiCad rotates the other way
        }

        dwg->SetLayer( layer );
        dwg->SetWidth( width );

        aModule->GraphicalItems().PushBack( dwg );
    }
}


void EAGLE_PLUGIN::packagePad( MODULE* aModule, CPTREE& aTree ) const
{
    // this is thru hole technology here, no SMDs
    EPAD e( aTree );

    D_PAD*  pad = new D_PAD( aModule );
    aModule->Pads().PushBack( pad );

    pad->SetPadName( FROM_UTF8( e.name.c_str() ) );

    // pad's "Position" is not relative to the module's,
    // whereas Pos0 is relative to the module's but is the unrotated coordinate.

    wxPoint padpos( kicad_x( e.x ), kicad_y( e.y ) );

    pad->SetPos0( padpos );

    RotatePoint( &padpos, aModule->GetOrientation() );

    pad->SetPosition( padpos + aModule->GetPosition() );

    pad->SetDrillSize( wxSize( kicad( e.drill ), kicad( e.drill ) ) );

    pad->SetLayerSet( LSET::AllCuMask().set( B_Mask ).set( F_Mask ) );

    if( e.shape )
    {
        switch( *e.shape )
        {
        case EPAD::ROUND:
            wxASSERT( pad->GetShape()==PAD_SHAPE_CIRCLE );    // verify set in D_PAD constructor
            break;

        case EPAD::OCTAGON:
            // no KiCad octagonal pad shape, use PAD_CIRCLE for now.
            // pad->SetShape( PAD_OCTAGON );
            wxASSERT( pad->GetShape()==PAD_SHAPE_CIRCLE );    // verify set in D_PAD constructor
            break;

        case EPAD::LONG:
            pad->SetShape( PAD_SHAPE_OVAL );
            break;

        case EPAD::SQUARE:
            pad->SetShape( PAD_SHAPE_RECT );
            break;

        case EPAD::OFFSET:
            ;   // don't know what to do here.
        }
    }
    else
    {
        // if shape is not present, our default is circle and that matches their default "round"
    }

    if( e.diameter )
    {
        int diameter = kicad( *e.diameter );
        pad->SetSize( wxSize( diameter, diameter ) );
    }
    else
    {
        double drillz  = pad->GetDrillSize().x;
        double annulus = drillz * m_rules->rvPadTop;   // copper annulus, eagle "restring"
        annulus = Clamp( m_rules->rlMinPadTop, annulus, m_rules->rlMaxPadTop );
        int diameter = KiROUND( drillz + 2 * annulus );
        pad->SetSize( wxSize( KiROUND( diameter ), KiROUND( diameter ) ) );
    }

    if( pad->GetShape() == PAD_SHAPE_OVAL )
    {
        // The Eagle "long" pad is wider than it is tall,
        // m_elongation is percent elongation
        wxSize sz = pad->GetSize();
        sz.x = ( sz.x * ( 100 + m_rules->psElongationLong ) ) / 100;
        pad->SetSize( sz );
    }

    if( e.rot )
    {
        pad->SetOrientation( e.rot->degrees * 10 );
    }

    // @todo: handle stop and thermal
}


void EAGLE_PLUGIN::packageText( MODULE* aModule, CPTREE& aTree ) const
{
    ETEXT       t( aTree );
    LAYER_ID    layer = kicad_layer( t.layer );

    if( layer == UNDEFINED_LAYER )
    {
        layer = Cmts_User;
    }

    TEXTE_MODULE* txt;

    if( t.text == ">NAME" || t.text == ">name" )
        txt = &aModule->Reference();
    else if( t.text == ">VALUE" || t.text == ">value" )
        txt = &aModule->Value();
    else
    {
        // FIXME: graphical text items are rotated for some reason.
        txt = new TEXTE_MODULE( aModule );
        aModule->GraphicalItems().PushBack( txt );
    }

    txt->SetTimeStamp( timeStamp( aTree ) );
    txt->SetText( FROM_UTF8( t.text.c_str() ) );

    wxPoint pos( kicad_x( t.x ), kicad_y( t.y ) );

    txt->SetTextPosition( pos );
    txt->SetPos0( pos - aModule->GetPosition() );

    txt->SetLayer( layer );

    txt->SetSize( kicad_fontz( t.size ) );

    double ratio = t.ratio ? *t.ratio : 8;  // DTD says 8 is default

    txt->SetThickness( kicad( t.size * ratio / 100 ) );

    int align = t.align ? *t.align : ETEXT::BOTTOM_LEFT;  // bottom-left is eagle default

    // An eagle package is never rotated, the DTD does not allow it.
    // angle -= aModule->GetOrienation();

    if( t.rot )
    {
        int sign = t.rot->mirror ? -1 : 1;
        txt->SetMirrored( t.rot->mirror );

        double degrees = t.rot->degrees;

        if( degrees == 90 || t.rot->spin )
            txt->SetOrientation( sign * degrees * 10 );
        else if( degrees == 180 )
            align = ETEXT::TOP_RIGHT;
        else if( degrees == 270 )
        {
            align = ETEXT::TOP_RIGHT;
            txt->SetOrientation( sign * 90 * 10 );
        }
    }

    switch( align )
    {
    case ETEXT::CENTER:
        // this was the default in pcbtxt's constructor
        break;

    case ETEXT::CENTER_LEFT:
        txt->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
        break;

    case ETEXT::CENTER_RIGHT:
        txt->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
        break;

    case ETEXT::TOP_CENTER:
        txt->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );
        break;

    case ETEXT::TOP_LEFT:
        txt->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
        txt->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );
        break;

    case ETEXT::TOP_RIGHT:
        txt->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
        txt->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );
        break;

    case ETEXT::BOTTOM_CENTER:
        txt->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
        break;

    case ETEXT::BOTTOM_LEFT:
        txt->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
        txt->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
        break;

    case ETEXT::BOTTOM_RIGHT:
        txt->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
        txt->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
        break;
    }
}


void EAGLE_PLUGIN::packageRectangle( MODULE* aModule, CPTREE& aTree ) const
{
    ERECT       r( aTree );
    LAYER_ID    layer = kicad_layer( r.layer );

    if( IsNonCopperLayer( layer ) )  // skip copper "package.rectangle"s
    {
        EDGE_MODULE* dwg = new EDGE_MODULE( aModule, S_POLYGON );
        aModule->GraphicalItems().PushBack( dwg );

        dwg->SetLayer( layer );
        dwg->SetWidth( 0 );

        dwg->SetTimeStamp( timeStamp( aTree ) );

        std::vector<wxPoint> pts;

        wxPoint start( wxPoint( kicad_x( r.x1 ), kicad_y( r.y1 ) ) );
        wxPoint end(   wxPoint( kicad_x( r.x1 ), kicad_y( r.y2 ) ) );

        pts.push_back( start );
        pts.push_back( wxPoint( kicad_x( r.x2 ), kicad_y( r.y1 ) ) );
        pts.push_back( wxPoint( kicad_x( r.x2 ), kicad_y( r.y2 ) ) );
        pts.push_back( end );

        dwg->SetPolyPoints( pts );

        dwg->SetStart0( start );
        dwg->SetEnd0( end );
    }
}


void EAGLE_PLUGIN::packagePolygon( MODULE* aModule, CPTREE& aTree ) const
{
    EPOLYGON    p( aTree );
    LAYER_ID    layer = kicad_layer( p.layer );

    if( IsNonCopperLayer( layer ) )  // skip copper "package.rectangle"s
    {
        EDGE_MODULE* dwg = new EDGE_MODULE( aModule, S_POLYGON );
        aModule->GraphicalItems().PushBack( dwg );

        dwg->SetWidth( 0 );     // it's filled, no need for boundary width

        /*
        switch( layer )
        {
        case Eco1_User:    layer = F_SilkS; break;
        case Eco2_User:    layer = B_SilkS;  break;

        // all MODULE templates (created from eagle packages) are on front layer
        // until cloned.
        case Cmts_User: layer = F_SilkS; break;
        }
        */

        dwg->SetLayer( layer );

        dwg->SetTimeStamp( timeStamp( aTree ) );

        std::vector<wxPoint> pts;
        pts.reserve( aTree.size() );

        for( CITER vi = aTree.begin();  vi != aTree.end();  ++vi )
        {
            if( vi->first != "vertex" )     // skip <xmlattr> node
                continue;

            EVERTEX v( vi->second );

            pts.push_back( wxPoint( kicad_x( v.x ), kicad_y( v.y ) ) );
        }

        dwg->SetPolyPoints( pts );

        dwg->SetStart0( *pts.begin() );
        dwg->SetEnd0( pts.back() );
    }
}


void EAGLE_PLUGIN::packageCircle( MODULE* aModule, CPTREE& aTree ) const
{
    ECIRCLE         e( aTree );
    LAYER_ID        layer = kicad_layer( e.layer );
    EDGE_MODULE*    gr = new EDGE_MODULE( aModule, S_CIRCLE );

    aModule->GraphicalItems().PushBack( gr );

    gr->SetWidth( kicad( e.width ) );

    switch( (int) layer )
    {
    case UNDEFINED_LAYER:   layer = Cmts_User;          break;
    /*
    case Eco1_User:            layer = F_SilkS; break;
    case Eco2_User:            layer = B_SilkS;  break;
    */
    default:
                            break;
    }

    gr->SetLayer( layer );
    gr->SetTimeStamp( timeStamp( aTree ) );

    gr->SetStart0( wxPoint( kicad_x( e.x ), kicad_y( e.y ) ) );
    gr->SetEnd0( wxPoint( kicad_x( e.x + e.radius ), kicad_y( e.y ) ) );
}


void EAGLE_PLUGIN::packageHole( MODULE* aModule, CPTREE& aTree ) const
{
    EHOLE   e( aTree );

    // we add a PAD_ATTRIB_HOLE_NOT_PLATED pad to this module.
    D_PAD* pad = new D_PAD( aModule );
    aModule->Pads().PushBack( pad );

    pad->SetShape( PAD_SHAPE_CIRCLE );
    pad->SetAttribute( PAD_ATTRIB_HOLE_NOT_PLATED );

    // Mechanical purpose only:
    // no offset, no net name, no pad name allowed
    // pad->SetOffset( wxPoint( 0, 0 ) );
    // pad->SetPadName( wxEmptyString );

    wxPoint padpos( kicad_x( e.x ), kicad_y( e.y ) );

    pad->SetPos0( padpos );
    pad->SetPosition( padpos + aModule->GetPosition() );

    wxSize  sz( kicad( e.drill ), kicad( e.drill ) );

    pad->SetDrillSize( sz );
    pad->SetSize( sz );

    pad->SetLayerSet( LSET::AllCuMask() /* | SOLDERMASK_LAYER_BACK | SOLDERMASK_LAYER_FRONT */ );
}


void EAGLE_PLUGIN::packageSMD( MODULE* aModule, CPTREE& aTree ) const
{
    ESMD        e( aTree );
    LAYER_ID    layer = kicad_layer( e.layer );

    if( !IsCopperLayer( layer ) )
    {
        return;
    }

    D_PAD*  pad = new D_PAD( aModule );
    aModule->Pads().PushBack( pad );

    pad->SetPadName( FROM_UTF8( e.name.c_str() ) );
    pad->SetShape( PAD_SHAPE_RECT );
    pad->SetAttribute( PAD_ATTRIB_SMD );

    // pad's "Position" is not relative to the module's,
    // whereas Pos0 is relative to the module's but is the unrotated coordinate.

    wxPoint padpos( kicad_x( e.x ), kicad_y( e.y ) );

    pad->SetPos0( padpos );

    RotatePoint( &padpos, aModule->GetOrientation() );

    pad->SetPosition( padpos + aModule->GetPosition() );

    pad->SetSize( wxSize( kicad( e.dx ), kicad( e.dy ) ) );

    pad->SetLayer( layer );

    static const LSET front( 3, F_Cu, F_Paste, F_Mask );
    static const LSET back(  3, B_Cu, B_Paste, B_Mask );

    if( layer == F_Cu )
        pad->SetLayerSet( front );
    else if( layer == B_Cu )
        pad->SetLayerSet( back );

    // Optional according to DTD
    if( e.roundness )    // set set shape to PAD_SHAPE_RECT above, in case roundness is not present
    {
        if( *e.roundness >= 75 )       // roundness goes from 0-100% as integer
        {
            if( e.dy == e.dx )
                pad->SetShape( PAD_SHAPE_CIRCLE );
            else
                pad->SetShape( PAD_SHAPE_OVAL );
        }
    }

    if( e.rot )
    {
        pad->SetOrientation( e.rot->degrees * 10 );
    }

    // don't know what stop, thermals, and cream should look like now.
}

/// non-owning container
typedef std::vector<ZONE_CONTAINER*>    ZONES;


void EAGLE_PLUGIN::loadSignals( CPTREE& aSignals )
{
    ZONES   zones;      // per net

    m_xpath->push( "signals.signal", "name" );

    int netCode = 1;

    for( CITER net = aSignals.begin();  net != aSignals.end();  ++net )
    {
        bool    sawPad = false;

        zones.clear();

        const string& nname = net->second.get<string>( "<xmlattr>.name" );
        wxString netName = FROM_UTF8( nname.c_str() );
        m_board->AppendNet( new NETINFO_ITEM( m_board, netName, netCode ) );

        m_xpath->Value( nname.c_str() );

#if defined(DEBUG)
        if( netName == wxT( "N$8" ) )
        {
            int breakhere = 1;
            (void) breakhere;
        }
#endif
        // (contactref | polygon | wire | via)*
        for( CITER it = net->second.begin();  it != net->second.end();  ++it )
        {
            if( it->first == "wire" )
            {
                m_xpath->push( "wire" );
                EWIRE   w( it->second );
                LAYER_ID  layer = kicad_layer( w.layer );

                if( IsCopperLayer( layer ) )
                {
                    TRACK*  t = new TRACK( m_board );

                    t->SetTimeStamp( timeStamp( it->second ) );

                    t->SetPosition( wxPoint( kicad_x( w.x1 ), kicad_y( w.y1 ) ) );
                    t->SetEnd( wxPoint( kicad_x( w.x2 ), kicad_y( w.y2 ) ) );

                    int width = kicad( w.width );
                    if( width < m_min_trace )
                        m_min_trace = width;

                    t->SetWidth( width );
                    t->SetLayer( layer );
                    t->SetNetCode( netCode );

                    m_board->m_Track.Insert( t, NULL );
                }
                else
                {
                    // put non copper wires where the sun don't shine.
                }

                m_xpath->pop();
            }

            else if( it->first == "via" )
            {
                m_xpath->push( "via" );
                EVIA    v( it->second );

                LAYER_ID  layer_front_most = kicad_layer( v.layer_front_most );
                LAYER_ID  layer_back_most  = kicad_layer( v.layer_back_most );

                if( IsCopperLayer( layer_front_most ) &&
                    IsCopperLayer( layer_back_most ) )
                {
                    int  kidiam;
                    int  drillz = kicad( v.drill );
                    VIA* via = new VIA( m_board );
                    m_board->m_Track.Insert( via, NULL );

                    via->SetLayerPair( layer_front_most, layer_back_most );

                    if( v.diam )
                    {
                        kidiam = kicad( *v.diam );
                        via->SetWidth( kidiam );
                    }
                    else
                    {
                        double annulus = drillz * m_rules->rvViaOuter;  // eagle "restring"
                        annulus = Clamp( m_rules->rlMinViaOuter, annulus, m_rules->rlMaxViaOuter );
                        kidiam = KiROUND( drillz + 2 * annulus );
                        via->SetWidth( kidiam );
                    }

                    via->SetDrill( drillz );

                    if( kidiam < m_min_via )
                        m_min_via = kidiam;

                    if( drillz < m_min_via_hole )
                        m_min_via_hole = drillz;

                    if( layer_front_most == F_Cu && layer_back_most == B_Cu )
                        via->SetViaType( VIA_THROUGH );
                    else if( layer_front_most == F_Cu || layer_back_most == B_Cu )
                        via->SetViaType( VIA_MICROVIA );
                    else
                        via->SetViaType( VIA_BLIND_BURIED );

                    via->SetTimeStamp( timeStamp( it->second ) );

                    wxPoint pos( kicad_x( v.x ), kicad_y( v.y ) );

                    via->SetPosition( pos  );
                    via->SetEnd( pos );

                    via->SetNetCode( netCode );
                }
                m_xpath->pop();
            }

            else if( it->first == "contactref" )
            {
                m_xpath->push( "contactref" );
                // <contactref element="RN1" pad="7"/>
                CPTREE& attribs = it->second.get_child( "<xmlattr>" );

                const string& reference = attribs.get<string>( "element" );
                const string& pad       = attribs.get<string>( "pad" );

                string key = makeKey( reference, pad ) ;

                // D(printf( "adding refname:'%s' pad:'%s' netcode:%d netname:'%s'\n", reference.c_str(), pad.c_str(), netCode, nname.c_str() );)

                m_pads_to_nets[ key ] = ENET( netCode, nname );

                m_xpath->pop();

                sawPad = true;
            }

            else if( it->first == "polygon" )
            {
                m_xpath->push( "polygon" );

                EPOLYGON    p( it->second );
                LAYER_ID    layer = kicad_layer( p.layer );

                if( IsCopperLayer( layer ) )
                {
                    // use a "netcode = 0" type ZONE:
                    ZONE_CONTAINER* zone = new ZONE_CONTAINER( m_board );
                    m_board->Add( zone, ADD_APPEND );
                    zones.push_back( zone );

                    zone->SetTimeStamp( timeStamp( it->second ) );
                    zone->SetLayer( layer );
                    zone->SetNetCode( netCode );

                    bool first = true;
                    for( CITER vi = it->second.begin();  vi != it->second.end();  ++vi )
                    {
                        if( vi->first != "vertex" )     // skip <xmlattr> node
                            continue;

                        EVERTEX v( vi->second );

                        // the ZONE_CONTAINER API needs work, as you can see:
                        if( first )
                        {
                            zone->Outline()->Start( layer,  kicad_x( v.x ), kicad_y( v.y ),
                                                    CPolyLine::NO_HATCH);
                            first = false;
                        }
                        else
                            zone->AppendCorner( wxPoint( kicad_x( v.x ), kicad_y( v.y ) ) );
                    }

                    zone->Outline()->CloseLastContour();

                    // If the pour is a cutout it needs to be set to a keepout
                    if( p.pour == EPOLYGON::CUTOUT )
                    {
                        zone->SetIsKeepout( true );
                        zone->SetDoNotAllowCopperPour( true );
                        zone->Outline()->SetHatchStyle( CPolyLine::NO_HATCH );
                    }

                    // if spacing is set the zone should be hatched
                    if( p.spacing )
                        zone->Outline()->SetHatch( CPolyLine::DIAGONAL_EDGE,
                                                   *p.spacing,
                                                   true );

                    // clearances, etc.
                    zone->SetArcSegmentCount( 32 );     // @todo: should be a constructor default?
                    zone->SetMinThickness( kicad( p.width ) );

                    // FIXME: KiCad zones have very rounded corners compared to eagle.
                    //        This means that isolation amounts that work well in eagle
                    //        tend to make copper intrude in soldermask free areas around pads.
                    if( p.isolate )
                    {
                        zone->SetZoneClearance( kicad( *p.isolate ) );
                    }

                    // missing == yes per DTD.
                    bool thermals = !p.thermals || *p.thermals;
                    zone->SetPadConnection( thermals ? PAD_ZONE_CONN_THERMAL : PAD_ZONE_CONN_FULL );
                    if( thermals )
                    {
                        // FIXME: eagle calculates dimensions for thermal spokes
                        //        based on what the zone is connecting to.
                        //        (i.e. width of spoke is half of the smaller side of an smd pad)
                        //        This is a basic workaround
                        zone->SetThermalReliefGap( kicad( p.width + 0.05 ) );
                        zone->SetThermalReliefCopperBridge( kicad( p.width + 0.05 ) );
                    }

                    int rank = p.rank ? *p.rank : p.max_priority;
                    zone->SetPriority( rank );
                }

                m_xpath->pop();     // "polygon"
            }
        }

        if( zones.size() && !sawPad )
        {
            // KiCad does not support an unconnected zone with its own non-zero netcode,
            // but only when assigned netcode = 0 w/o a name...
            for( ZONES::iterator it = zones.begin();  it != zones.end();  ++it )
                (*it)->SetNetCode( NETINFO_LIST::UNCONNECTED );

            // therefore omit this signal/net.
        }
        else
            netCode++;
    }

    m_xpath->pop();     // "signals.signal"
}


LAYER_ID EAGLE_PLUGIN::kicad_layer( int aEagleLayer ) const
{
    /* will assume this is a valid mapping for all eagle boards until I get paid more:

    <layers>
    <layer number="1" name="Top" color="4" fill="1" visible="yes" active="yes"/>
    <layer number="2" name="Route2" color="1" fill="3" visible="no" active="no"/>
    <layer number="3" name="Route3" color="4" fill="3" visible="no" active="no"/>
    <layer number="4" name="Route4" color="1" fill="4" visible="no" active="no"/>
    <layer number="5" name="Route5" color="4" fill="4" visible="no" active="no"/>
    <layer number="6" name="Route6" color="1" fill="8" visible="no" active="no"/>
    <layer number="7" name="Route7" color="4" fill="8" visible="no" active="no"/>
    <layer number="8" name="Route8" color="1" fill="2" visible="no" active="no"/>
    <layer number="9" name="Route9" color="4" fill="2" visible="no" active="no"/>
    <layer number="10" name="Route10" color="1" fill="7" visible="no" active="no"/>
    <layer number="11" name="Route11" color="4" fill="7" visible="no" active="no"/>
    <layer number="12" name="Route12" color="1" fill="5" visible="no" active="no"/>
    <layer number="13" name="Route13" color="4" fill="5" visible="no" active="no"/>
    <layer number="14" name="Route14" color="1" fill="6" visible="no" active="no"/>
    <layer number="15" name="Route15" color="4" fill="6" visible="no" active="no"/>
    <layer number="16" name="Bottom" color="1" fill="1" visible="yes" active="yes"/>
    <layer number="17" name="Pads" color="2" fill="1" visible="yes" active="yes"/>
    <layer number="18" name="Vias" color="14" fill="1" visible="yes" active="yes"/>
    <layer number="19" name="Unrouted" color="6" fill="1" visible="yes" active="yes"/>
    <layer number="20" name="Dimension" color="15" fill="1" visible="yes" active="yes"/>
    <layer number="21" name="tPlace" color="7" fill="1" visible="yes" active="yes"/>
    <layer number="22" name="bPlace" color="7" fill="1" visible="yes" active="yes"/>
    <layer number="23" name="tOrigins" color="15" fill="1" visible="yes" active="yes"/>
    <layer number="24" name="bOrigins" color="15" fill="1" visible="yes" active="yes"/>
    <layer number="25" name="tNames" color="7" fill="1" visible="yes" active="yes"/>
    <layer number="26" name="bNames" color="7" fill="1" visible="yes" active="yes"/>
    <layer number="27" name="tValues" color="7" fill="1" visible="no" active="yes"/>
    <layer number="28" name="bValues" color="7" fill="1" visible="no" active="yes"/>
    <layer number="29" name="tStop" color="2" fill="3" visible="no" active="yes"/>
    <layer number="30" name="bStop" color="5" fill="6" visible="no" active="yes"/>
    <layer number="31" name="tCream" color="7" fill="4" visible="no" active="yes"/>
    <layer number="32" name="bCream" color="7" fill="5" visible="no" active="yes"/>
    <layer number="33" name="tFinish" color="6" fill="3" visible="no" active="yes"/>
    <layer number="34" name="bFinish" color="6" fill="6" visible="no" active="yes"/>
    <layer number="35" name="tGlue" color="7" fill="4" visible="no" active="yes"/>
    <layer number="36" name="bGlue" color="7" fill="5" visible="no" active="yes"/>
    <layer number="37" name="tTest" color="7" fill="1" visible="no" active="yes"/>
    <layer number="38" name="bTest" color="7" fill="1" visible="no" active="yes"/>
    <layer number="39" name="tKeepout" color="4" fill="11" visible="no" active="yes"/>
    <layer number="40" name="bKeepout" color="1" fill="11" visible="no" active="yes"/>
    <layer number="41" name="tRestrict" color="4" fill="10" visible="no" active="yes"/>
    <layer number="42" name="bRestrict" color="1" fill="10" visible="no" active="yes"/>
    <layer number="43" name="vRestrict" color="2" fill="10" visible="no" active="yes"/>
    <layer number="44" name="Drills" color="7" fill="1" visible="no" active="yes"/>
    <layer number="45" name="Holes" color="7" fill="1" visible="no" active="yes"/>
    <layer number="46" name="Milling" color="3" fill="1" visible="no" active="yes"/>
    <layer number="47" name="Measures" color="7" fill="1" visible="no" active="yes"/>
    <layer number="48" name="Document" color="7" fill="1" visible="no" active="yes"/>
    <layer number="49" name="ReferenceLC" color="13" fill="1" visible="yes" active="yes"/>
    <layer number="50" name="ReferenceLS" color="12" fill="1" visible="yes" active="yes"/>
    <layer number="51" name="tDocu" color="7" fill="1" visible="yes" active="yes"/>
    <layer number="52" name="bDocu" color="7" fill="1" visible="yes" active="yes"/>

    * These layers are used only in eagle schematic.
    * They should not be found in board files.
    * They are listed for info only.
    <layer number="91" name="Nets" color="2" fill="1" visible="no" active="no"/>
    <layer number="92" name="Busses" color="1" fill="1" visible="no" active="no"/>
    <layer number="93" name="Pins" color="2" fill="1" visible="no" active="no"/>
    <layer number="94" name="Symbols" color="4" fill="1" visible="no" active="no"/>
    <layer number="95" name="Names" color="7" fill="1" visible="no" active="no"/>
    <layer number="96" name="Values" color="7" fill="1" visible="no" active="no"/>
    <layer number="97" name="Info" color="7" fill="1" visible="no" active="no"/>
    <layer number="98" name="Guide" color="6" fill="1" visible="no" active="no"/>

    * These layers are user layers
    <layer number="160" name="???" color="7" fill="1" visible="yes" active="yes"/>
    <layer number="161" name="???" color="7" fill="1" visible="yes" active="yes"/>

    </layers>

    */

    int kiLayer;

    // eagle copper layer:
    if( aEagleLayer >= 1 && aEagleLayer < int( DIM( m_cu_map ) ) )
    {
        kiLayer = m_cu_map[aEagleLayer];
    }

    else
    {
        // translate non-copper eagle layer to pcbnew layer
        switch( aEagleLayer )
        {
        case 20:    kiLayer = Edge_Cuts;    break;  // eagle says "Dimension" layer, but it's for board perimeter
        case 21:    kiLayer = F_SilkS;      break;
        case 22:    kiLayer = B_SilkS;      break;
        case 25:    kiLayer = F_SilkS;      break;
        case 26:    kiLayer = B_SilkS;      break;
        case 27:    kiLayer = F_SilkS;      break;
        case 28:    kiLayer = B_SilkS;      break;
        case 29:    kiLayer = F_Mask;       break;
        case 30:    kiLayer = B_Mask;       break;
        case 31:    kiLayer = F_Paste;      break;
        case 32:    kiLayer = B_Paste;      break;
        case 33:    kiLayer = F_Mask;       break;
        case 34:    kiLayer = B_Mask;       break;
        case 35:    kiLayer = F_Adhes;      break;
        case 36:    kiLayer = B_Adhes;      break;
        case 49:    kiLayer = Cmts_User;    break;
        case 50:    kiLayer = Cmts_User;    break;

        // Packages show the future chip pins on SMD parts using layer 51.
        // This is an area slightly smaller than the PAD/SMD copper area.
        // Carry those visual aids into the MODULE on the fabrication layer,
        // not silkscreen. This is perhaps not perfect, but there is not a lot
        // of other suitable paired layers
        case 51:    kiLayer = F_Fab;        break;
        case 52:    kiLayer = B_Fab;        break;

        // thes layers are defined as user layers. put them on ECO layers
        case 160:   kiLayer = Eco1_User;    break;
        case 161:   kiLayer = Eco2_User;    break;
        default:
            // some layers do not map to KiCad
            // DBG( printf( "unsupported eagle layer: %d\n", aEagleLayer );)
            kiLayer = UNDEFINED_LAYER;      break;
        }
    }

    return LAYER_ID( kiLayer );
}


void EAGLE_PLUGIN::centerBoard()
{
    if( m_props )
    {
        UTF8 page_width;
        UTF8 page_height;

        if( m_props->Value( "page_width",  &page_width ) &&
            m_props->Value( "page_height", &page_height ) )
        {
            EDA_RECT bbbox = m_board->ComputeBoundingBox( true );

            int w = atoi( page_width.c_str() );
            int h = atoi( page_height.c_str() );

            int desired_x = ( w - bbbox.GetWidth() )  / 2;
            int desired_y = ( h - bbbox.GetHeight() ) / 2;

            DBG(printf( "bbox.width:%d bbox.height:%d w:%d h:%d desired_x:%d desired_y:%d\n",
                bbbox.GetWidth(), bbbox.GetHeight(), w, h, desired_x, desired_y );)

            m_board->Move( wxPoint( desired_x - bbbox.GetX(), desired_y - bbbox.GetY() ) );
        }
    }
}


wxDateTime EAGLE_PLUGIN::getModificationTime( const wxString& aPath )
{
    wxFileName  fn( aPath );

    // Do not call wxFileName::GetModificationTime() on a non-existent file, because
    // if it fails, wx's implementation calls the crap wxLogSysError() which
    // eventually infects our UI with an unwanted popup window, so don't let it fail.
    if( !fn.IsFileReadable() )
    {
        wxString msg = wxString::Format(
            _( "File '%s' is not readable." ),
            GetChars( aPath ) );

        THROW_IO_ERROR( msg );
    }

    /*
    // update the writable flag while we have a wxFileName, in a network this
    // is possibly quite dynamic anyway.
    m_writable = fn.IsFileWritable();
    */

    wxDateTime modTime = fn.GetModificationTime();

    if( !modTime.IsValid() )
        modTime.Now();

    return modTime;
}


void EAGLE_PLUGIN::cacheLib( const wxString& aLibPath )
{
    try
    {
        wxDateTime  modtime = getModificationTime( aLibPath );

        // Fixes assertions in wxWidgets debug builds for the wxDateTime object.  Refresh the
        // cache if either of the wxDateTime objects are invalid or the last file modification
        // time differs from the current file modification time.
        bool load = !m_mod_time.IsValid() || !modtime.IsValid() ||
                    m_mod_time != modtime;

        if( aLibPath != m_lib_path || load )
        {
            PTREE       doc;
            LOCALE_IO   toggle;     // toggles on, then off, the C locale.

            m_templates.clear();

            // Set this before completion of loading, since we rely on it for
            // text of an exception.  Delay setting m_mod_time until after successful load
            // however.
            m_lib_path = aLibPath;

            // 8 bit "filename" should be encoded according to disk filename encoding,
            // (maybe this is current locale, maybe not, its a filesystem issue),
            // and is not necessarily utf8.
            string filename = (const char*) aLibPath.char_str( wxConvFile );

            read_xml( filename, doc, xml_parser::no_comments );

            // clear the cu map and then rebuild it.
            clear_cu_map();

            m_xpath->push( "eagle.drawing.layers" );
            CPTREE& layers  = doc.get_child( "eagle.drawing.layers" );
            loadLayerDefs( layers );
            m_xpath->pop();

            m_xpath->push( "eagle.drawing.library" );
            CPTREE& library = doc.get_child( "eagle.drawing.library" );
            loadLibrary( library, NULL );
            m_xpath->pop();

            m_mod_time = modtime;
        }
    }
    catch( file_parser_error fpe )
    {
        // for xml_parser_error, what() has the line number in it,
        // but no byte offset.  That should be an adequate error message.
        THROW_IO_ERROR( fpe.what() );
    }

    // Class ptree_error is a base class for xml_parser_error & file_parser_error,
    // so one catch should be OK for all errors.
    catch( ptree_error pte )
    {
        string errmsg = pte.what();

        errmsg += " @\n";
        errmsg += m_xpath->Contents();

        THROW_IO_ERROR( errmsg );
    }
}


wxArrayString EAGLE_PLUGIN::FootprintEnumerate( const wxString& aLibraryPath, const PROPERTIES* aProperties )
{
    init( aProperties );

    cacheLib( aLibraryPath );

    wxArrayString   ret;

    for( MODULE_CITER it = m_templates.begin();  it != m_templates.end();  ++it )
        ret.Add( FROM_UTF8( it->first.c_str() ) );

    return ret;
}


MODULE* EAGLE_PLUGIN::FootprintLoad( const wxString& aLibraryPath, const wxString& aFootprintName, const PROPERTIES* aProperties )
{
    init( aProperties );

    cacheLib( aLibraryPath );

    string key = TO_UTF8( aFootprintName );

    MODULE_CITER mi = m_templates.find( key );

    if( mi == m_templates.end() )
        return NULL;

    // copy constructor to clone the template
    MODULE* ret = new MODULE( *mi->second );

    return ret;
}


void EAGLE_PLUGIN::FootprintLibOptions( PROPERTIES* aListToAppendTo ) const
{
    PLUGIN::FootprintLibOptions( aListToAppendTo );

    /*
    (*aListToAppendTo)["ignore_duplicates"] = UTF8( _(
        "Ignore duplicately named footprints within the same Eagle library. "
        "Only the first similarly named footprint will be loaded."
        ));
    */
}


/*
void EAGLE_PLUGIN::Save( const wxString& aFileName, BOARD* aBoard, const PROPERTIES* aProperties )
{
    // Eagle lovers apply here.
}


void EAGLE_PLUGIN::FootprintSave( const wxString& aLibraryPath, const MODULE* aFootprint, const PROPERTIES* aProperties )
{
}


void EAGLE_PLUGIN::FootprintDelete( const wxString& aLibraryPath, const wxString& aFootprintName )
{
}


void EAGLE_PLUGIN::FootprintLibCreate( const wxString& aLibraryPath, const PROPERTIES* aProperties )
{
}


bool EAGLE_PLUGIN::FootprintLibDelete( const wxString& aLibraryPath, const PROPERTIES* aProperties )
{
}


bool EAGLE_PLUGIN::IsFootprintLibWritable( const wxString& aLibraryPath )
{
    return true;
}

*/
