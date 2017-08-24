/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012-2016 KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2017 CERN.
 * @author Alejandro García Montoro <alejandro.garciamontoro@gmail.com>
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

#include <eagle_parser.h>

#include <functional>
#include <sstream>
#include <iomanip>

// Template specializations below parse wxString to the used types:
//      - string
//      - double
//      - int
//      - bool
//      - EROT

constexpr auto DEFAULT_ALIGNMENT = ETEXT::BOTTOM_LEFT;

template<>
string Convert<string>( wxString aValue )
{
    return aValue.ToStdString();
}


template <>
double Convert<double>( wxString aValue )
{
    double value;

    if( aValue.ToDouble( &value ) )
        return value;
    else
        throw XML_PARSER_ERROR( "Conversion to double failed. Original value: '" +
                                aValue.ToStdString() + "'." );
}


template <>
int Convert<int>( wxString aValue )
{
    if( aValue.IsEmpty() )
        throw XML_PARSER_ERROR( "Conversion to int failed. Original value is empty." );

    return wxAtoi( aValue );
}


template <>
bool Convert<bool>( wxString aValue )
{
    if( aValue != "yes" && aValue != "no" )
        throw XML_PARSER_ERROR( "Conversion to bool failed. Original value, '" +
                                aValue.ToStdString() +
                                "', is neither 'yes' nor 'no'." );

    return aValue == "yes";
}


/// parse an Eagle XML "rot" field.  Unfortunately the DTD seems not to explain
/// this format very well.  [S][M]R<degrees>.   Examples: "R90", "MR180", "SR180"
template<>
EROT Convert<EROT>( wxString aRot )
{
    EROT value;

    value.spin    = aRot.find( 'S' ) != aRot.npos;
    value.mirror  = aRot.find( 'M' ) != aRot.npos;
    value.degrees = strtod( aRot.c_str()
                            + 1                        // skip leading 'R'
                            + int( value.spin )       // skip optional leading 'S'
                            + int( value.mirror ),    // skip optional leading 'M'
                            NULL );

    return value;
}

/**
 * Function parseRequiredAttribute
 * parsese the aAttribute of the XML node aNode.
 * @param  aNode      is the node whose attribute will be parsed.
 * @param  aAttribute is the attribute that will be parsed.
 * @throw  XML_PARSER_ERROR - exception thrown if the required attribute is missing
 * @return T - the attributed parsed as the specified type.
 */
template<typename T>
T parseRequiredAttribute( wxXmlNode* aNode, string aAttribute )
{
    wxString value;

    if( aNode->GetAttribute( aAttribute, &value ) )
        return Convert<T>( value );
    else
        throw XML_PARSER_ERROR( "The required attribute " + aAttribute + " is missing." );
}

/**
 * Function parseOptionalAttribute
 * parses the aAttribute of the XML node aNode.
 * @param  aNode      is the node whose attribute will be parsed.
 * @param  aAttribute is the attribute that will be parsed.
 * @return OPTIONAL_XML_ATTRIBUTE<T> - an optional XML attribute, parsed as the specified type if
 *                                   found.
 */
template<typename T>
OPTIONAL_XML_ATTRIBUTE<T> parseOptionalAttribute( wxXmlNode* aNode, string aAttribute )
{
    return OPTIONAL_XML_ATTRIBUTE<T>( aNode->GetAttribute( aAttribute ) );
}


NODE_MAP MapChildren( wxXmlNode* currentNode )
{
    // Map node_name -> node_pointer
    NODE_MAP nodesMap;

    // Loop through all children mapping them in nodesMap
    if( currentNode )
        currentNode = currentNode->GetChildren();

    while( currentNode )
    {
        // Create a new pair in the map
        //      key: current node name
        //      value: current node pointer
        nodesMap[currentNode->GetName().ToStdString()] = currentNode;

        // Get next child
        currentNode = currentNode->GetNext();
    }

    return nodesMap;
}


string makeKey( const string& aFirst, const string& aSecond )
{
    string key = aFirst + '\x02' +  aSecond;
    return key;
}


unsigned long timeStamp( wxXmlNode* aTree )
{
    // in this case from a unique tree memory location
    return (unsigned long)(void*) aTree;
}


time_t moduleTstamp( const string& aName, const string& aValue, int aUnit )
{
    std::size_t h1 = std::hash<string>{}( aName );
    std::size_t h2 = std::hash<string>{}( aValue );
    std::size_t h3 = std::hash<int>{}( aUnit );

    return h1 ^ (h2 << 1) ^ (h3 << 2);
}


string modulePath( const string& aName, const string& aValue )
{
    // TODO handle subsheet
    std::ostringstream s;

    s << '/' << std::setfill( '0' ) << std::uppercase << std::hex << std::setw( 8 )
      << moduleTstamp( aName, aValue, 0 );

    return s.str();
}


wxPoint kicad_arc_center( const wxPoint& aStart, const wxPoint& aEnd, double aAngle )
{
    // Eagle give us start and end.
    // S_ARC wants start to give the center, and end to give the start.
    double dx = aEnd.x - aStart.x, dy = aEnd.y - aStart.y;
    wxPoint mid = ( aStart + aEnd ) / 2;

    double dlen = sqrt( dx*dx + dy*dy );
    double dist = dlen / ( 2 * tan( DEG2RAD( aAngle ) / 2 ) );

    wxPoint center(
        mid.x + dist * ( dy / dlen ),
        mid.y - dist * ( dx / dlen )
    );

    return center;
}

int parseAlignment(wxString alignment)
{
    // (bottom-left | bottom-center | bottom-right | center-left |
    //   center | center-right | top-left | top-center | top-right)
    if( alignment == "center" )
        return ETEXT::CENTER;
    else if( alignment == "center-right" )
        return  ETEXT::CENTER_RIGHT;
    else if( alignment == "top-left" )
        return  ETEXT::TOP_LEFT;
    else if( alignment == "top-center" )
        return  ETEXT::TOP_CENTER;
    else if( alignment == "top-right" )
        return  ETEXT::TOP_RIGHT;
    else if( alignment == "bottom-left" )
        return  ETEXT::BOTTOM_LEFT;
    else if( alignment == "bottom-center" )
        return  ETEXT::BOTTOM_CENTER;
    else if( alignment == "bottom-right" )
        return  ETEXT::BOTTOM_RIGHT;
    else if( alignment == "center-left" )
        return  ETEXT::CENTER_LEFT;

    return DEFAULT_ALIGNMENT;
}

// convert textsize method.
wxSize convertTextSize(ETEXT& etext ) {

    wxSize textsize;
    if(etext.font){
        wxString font = etext.font.Get();
        if(font == "vector")
        {
            textsize = wxSize( etext.size * EUNIT_TO_MIL, etext.size * EUNIT_TO_MIL );
        }
        else if ( font == "fixed")
        {
            textsize = wxSize( etext.size * EUNIT_TO_MIL, etext.size * EUNIT_TO_MIL*0.80 );
        }

    }
    else
    {
        textsize =  wxSize( etext.size * EUNIT_TO_MIL*0.85, etext.size * EUNIT_TO_MIL );
    }
    return textsize;

}


EWIRE::EWIRE( wxXmlNode* aWire )
{
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

    x1    = parseRequiredAttribute<double>( aWire, "x1" );
    y1    = parseRequiredAttribute<double>( aWire, "y1" );
    x2    = parseRequiredAttribute<double>( aWire, "x2" );
    y2    = parseRequiredAttribute<double>( aWire, "y2" );
    width = parseRequiredAttribute<double>( aWire, "width" );
    layer = parseRequiredAttribute<int>( aWire, "layer" );
    curve = parseOptionalAttribute<double>( aWire, "curve" );

    opt_string s = parseOptionalAttribute<string>( aWire, "style" );

    if( s == "continuous" )
        style = EWIRE::CONTINUOUS;
    else if( s == "longdash" )
        style = EWIRE::LONGDASH;
    else if( s == "shortdash" )
        style = EWIRE::SHORTDASH;
    else if( s == "dashdot" )
        style = EWIRE::DASHDOT;

    s = parseOptionalAttribute<string>( aWire, "cap" );

    if( s == "round" )
        cap = EWIRE::ROUND;
    else if( s == "flat" )
        cap = EWIRE::FLAT;
}


EJUNCTION::EJUNCTION( wxXmlNode* aJunction )
{
    /*
    <!ELEMENT junction EMPTY>
    <!ATTLIST junction
          x             %Coord;        #REQUIRED
          y             %Coord;        #REQUIRED
          >
    */

    x    = parseRequiredAttribute<double>( aJunction, "x" );
    y    = parseRequiredAttribute<double>( aJunction, "y" );
}


ELABEL::ELABEL( wxXmlNode* aLabel, const wxString& aNetName )
{
    /*
    <!ELEMENT label EMPTY>
    <!ATTLIST label
          x             %Coord;        #REQUIRED
          y             %Coord;        #REQUIRED
          size          %Dimension;    #REQUIRED
          layer         %Layer;        #REQUIRED
          font          %TextFont;     "proportional"
          ratio         %Int;          "8"
          rot           %Rotation;     "R0"
          xref          %Bool;         "no"
          >
    */

    x    = parseRequiredAttribute<double>( aLabel, "x" );
    y    = parseRequiredAttribute<double>( aLabel, "y" );
    size = parseRequiredAttribute<double>( aLabel, "size" );
    layer = parseRequiredAttribute<int>( aLabel, "layer" );
    rot   = parseOptionalAttribute<EROT>( aLabel, "rot" );
    xref  = parseOptionalAttribute<string>( aLabel, "xref" );
    netname = aNetName;
}


EVIA::EVIA( wxXmlNode* aVia )
{
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

    x = parseRequiredAttribute<double>( aVia, "x" );
    y = parseRequiredAttribute<double>( aVia, "y" );

    string ext = parseRequiredAttribute<string>( aVia, "extent" );

    sscanf( ext.c_str(), "%d-%d", &layer_front_most, &layer_back_most );

    drill = parseRequiredAttribute<double>( aVia, "drill" );
    diam  = parseOptionalAttribute<double>( aVia, "diameter" );
    shape = parseOptionalAttribute<string>( aVia, "shape" );
}


ECIRCLE::ECIRCLE( wxXmlNode* aCircle )
{
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

    x      = parseRequiredAttribute<double>( aCircle, "x" );
    y      = parseRequiredAttribute<double>( aCircle, "y" );
    radius = parseRequiredAttribute<double>( aCircle, "radius" );
    width  = parseRequiredAttribute<double>( aCircle, "width" );
    layer  = parseRequiredAttribute<int>( aCircle, "layer" );
}


ERECT::ERECT( wxXmlNode* aRect )
{
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

    x1    = parseRequiredAttribute<double>( aRect, "x1" );
    y1    = parseRequiredAttribute<double>( aRect, "y1" );
    x2    = parseRequiredAttribute<double>( aRect, "x2" );
    y2    = parseRequiredAttribute<double>( aRect, "y2" );
    layer = parseRequiredAttribute<int>( aRect, "layer" );
    rot   = parseOptionalAttribute<EROT>( aRect, "rot" );
}


EATTR::EATTR( wxXmlNode* aTree )
{
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

    name  = parseRequiredAttribute<string>( aTree, "name" );
    value = parseOptionalAttribute<string>( aTree, "value" );

    x     = parseOptionalAttribute<double>( aTree, "x" );
    y     = parseOptionalAttribute<double>( aTree, "y" );
    size  = parseOptionalAttribute<double>( aTree, "size" );

    // KiCad cannot currently put a TEXTE_MODULE on a different layer than the MODULE
    // Eagle can it seems.
    layer = parseOptionalAttribute<int>( aTree, "layer" );
    ratio = parseOptionalAttribute<double>( aTree, "ratio" );
    rot   = parseOptionalAttribute<EROT>( aTree, "rot" );

    opt_string stemp = parseOptionalAttribute<string>( aTree, "display" );

    // (off | value | name | both)
    if( stemp == "off" )
        display = EATTR::Off;
    else if( stemp == "value" )
        display = EATTR::VALUE;
    else if( stemp == "name" )
        display = EATTR::NAME;
    else if( stemp == "both" )
        display = EATTR::BOTH;

    stemp = parseOptionalAttribute<string>( aTree, "align" );

    align = stemp ? parseAlignment( *stemp ) : DEFAULT_ALIGNMENT;
}


EDIMENSION::EDIMENSION( wxXmlNode* aDimension )
{
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

    x1    = parseRequiredAttribute<double>( aDimension, "x1" );
    y1    = parseRequiredAttribute<double>( aDimension, "y1" );
    x2    = parseRequiredAttribute<double>( aDimension, "x2" );
    y2    = parseRequiredAttribute<double>( aDimension, "y2" );
    x3    = parseRequiredAttribute<double>( aDimension, "x3" );
    y3    = parseRequiredAttribute<double>( aDimension, "y3" );
    layer = parseRequiredAttribute<int>( aDimension, "layer" );

    opt_string dimType = parseOptionalAttribute<string>( aDimension, "dtype" );

    if( !dimType )
    {
        // default type is parallel
    }
}


ETEXT::ETEXT( wxXmlNode* aText )
{
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

    text  = aText->GetNodeContent();
    x     = parseRequiredAttribute<double>( aText, "x" );
    y     = parseRequiredAttribute<double>( aText, "y" );
    size  = parseRequiredAttribute<double>( aText, "size" );
    layer = parseRequiredAttribute<int>( aText, "layer" );

    font  = parseOptionalAttribute<string>( aText, "font" );
    ratio = parseOptionalAttribute<double>( aText, "ratio" );
    rot   = parseOptionalAttribute<EROT>( aText, "rot" );

    opt_string stemp = parseOptionalAttribute<string>( aText, "align" );

    align = stemp ? parseAlignment( *stemp ) : DEFAULT_ALIGNMENT;
}


EPAD::EPAD( wxXmlNode* aPad )
{
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
    name         = parseRequiredAttribute<string>( aPad, "name" );
    x            = parseRequiredAttribute<double>( aPad, "x" );
    y            = parseRequiredAttribute<double>( aPad, "y" );
    drill        = parseRequiredAttribute<double>( aPad, "drill" );

    // Optional attributes
    diameter     = parseOptionalAttribute<double>( aPad, "diameter" );

    opt_string s = parseOptionalAttribute<string>( aPad, "shape" );

    // (square | round | octagon | long | offset)
    if( s == "square" )
        shape = EPAD::SQUARE;
    else if( s == "round" )
        shape = EPAD::ROUND;
    else if( s == "octagon" )
        shape = EPAD::OCTAGON;
    else if( s == "long" )
        shape = EPAD::LONG;
    else if( s == "offset" )
        shape = EPAD::OFFSET;

    rot      = parseOptionalAttribute<EROT>( aPad, "rot" );
    stop     = parseOptionalAttribute<bool>( aPad, "stop" );
    thermals = parseOptionalAttribute<bool>( aPad, "thermals" );
    first    = parseOptionalAttribute<bool>( aPad, "first" );
}


ESMD::ESMD( wxXmlNode* aSMD )
{
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
    name      = parseRequiredAttribute<string>( aSMD, "name" );
    x         = parseRequiredAttribute<double>( aSMD, "x" );
    y         = parseRequiredAttribute<double>( aSMD, "y" );
    dx        = parseRequiredAttribute<double>( aSMD, "dx" );
    dy        = parseRequiredAttribute<double>( aSMD, "dy" );
    layer     = parseRequiredAttribute<int>( aSMD, "layer" );

    roundness = parseOptionalAttribute<int>( aSMD, "roundness" );
    rot       = parseOptionalAttribute<EROT>( aSMD, "rot" );
    thermals  = parseOptionalAttribute<bool>( aSMD, "thermals" );
    stop      = parseOptionalAttribute<bool>( aSMD, "stop" );
    thermals  = parseOptionalAttribute<bool>( aSMD, "thermals" );
    cream     = parseOptionalAttribute<bool>( aSMD, "cream" );
}


EPIN::EPIN( wxXmlNode* aPin )
{
    /*
    <!ELEMENT pin EMPTY>
    <!ATTLIST pin
              name          %String;       #REQUIRED
              x             %Coord;        #REQUIRED
              y             %Coord;        #REQUIRED
              visible       %PinVisible;   "both"
              length        %PinLength;    "long"
              direction     %PinDirection; "io"
              function      %PinFunction;  "none"
              swaplevel     %Int;          "0"
              rot           %Rotation;     "R0"
              >
    */

    // DTD #REQUIRED, throw exception if not found
    name      = parseRequiredAttribute<string>( aPin, "name" );
    x         = parseRequiredAttribute<double>( aPin, "x" );
    y         = parseRequiredAttribute<double>( aPin, "y" );

    visible   = parseOptionalAttribute<string>( aPin, "visible" );
    length    = parseOptionalAttribute<string>( aPin, "length" );
    direction = parseOptionalAttribute<string>( aPin, "direction" );
    function  = parseOptionalAttribute<string>( aPin, "function" );
    swaplevel = parseOptionalAttribute<int>( aPin, "swaplevel" );
    rot       = parseOptionalAttribute<EROT>( aPin, "rot" );
}


EVERTEX::EVERTEX( wxXmlNode* aVertex )
{
    /*
    <!ELEMENT vertex EMPTY>
    <!ATTLIST vertex
          x             %Coord;        #REQUIRED
          y             %Coord;        #REQUIRED
          curve         %WireCurve;    "0" -- the curvature from this vertex to the next one --
          >
    */

    x = parseRequiredAttribute<double>( aVertex, "x" );
    y = parseRequiredAttribute<double>( aVertex, "y" );
}


EPOLYGON::EPOLYGON( wxXmlNode* aPolygon )
{
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

    width        = parseRequiredAttribute<double>( aPolygon, "width" );
    layer        = parseRequiredAttribute<int>( aPolygon, "layer" );

    spacing      = parseOptionalAttribute<double>( aPolygon, "spacing" );
    isolate      = parseOptionalAttribute<double>( aPolygon, "isolate" );
    opt_string s = parseOptionalAttribute<string>( aPolygon, "pour" );

    // default pour to solid fill
    pour = EPOLYGON::SOLID;

    // (solid | hatch | cutout)
    if( s == "hatch" )
        pour = EPOLYGON::HATCH;
    else if( s == "cutout" )
        pour = EPOLYGON::CUTOUT;

    orphans  = parseOptionalAttribute<bool>( aPolygon, "orphans" );
    thermals = parseOptionalAttribute<bool>( aPolygon, "thermals" );
    rank     = parseOptionalAttribute<int>( aPolygon, "rank" );
}


EHOLE::EHOLE( wxXmlNode* aHole )
{
    /*
    <!ELEMENT hole EMPTY>
    <!ATTLIST hole
          x             %Coord;        #REQUIRED
          y             %Coord;        #REQUIRED
          drill         %Dimension;    #REQUIRED
          >
    */

    // #REQUIRED:
    x     = parseRequiredAttribute<double>( aHole, "x" );
    y     = parseRequiredAttribute<double>( aHole, "y" );
    drill = parseRequiredAttribute<double>( aHole, "drill" );
}


EELEMENT::EELEMENT( wxXmlNode* aElement )
{
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
    name    = parseRequiredAttribute<string>( aElement, "name" );
    library = parseRequiredAttribute<string>( aElement, "library" );
    value   = parseRequiredAttribute<string>( aElement, "value" );
    package = parseRequiredAttribute<string>( aElement, "package" );
    ReplaceIllegalFileNameChars( &package );

    x       = parseRequiredAttribute<double>( aElement, "x" );
    y       = parseRequiredAttribute<double>( aElement, "y" );

    // optional
    locked  = parseOptionalAttribute<bool>( aElement, "locked" );
    smashed = parseOptionalAttribute<bool>( aElement, "smashed" );
    rot     = parseOptionalAttribute<EROT>( aElement, "rot" );
}


ELAYER::ELAYER( wxXmlNode* aLayer )
{
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

    number  = parseRequiredAttribute<int>( aLayer, "number" );
    name    = parseRequiredAttribute<string>( aLayer, "name" );
    color   = parseRequiredAttribute<int>( aLayer, "color" );
    fill    = 1;    // Temporary value.
    visible = parseOptionalAttribute<bool>( aLayer, "visible" );
    active  = parseOptionalAttribute<bool>( aLayer, "active" );
}


EPART::EPART( wxXmlNode* aPart )
{
    /*
     *  <!ELEMENT part (attribute*, variant*)>
     *  <!ATTLIST part
     *  name          %String;       #REQUIRED
     *  library       %String;       #REQUIRED
     *  deviceset     %String;       #REQUIRED
     *  device        %String;       #REQUIRED
     *  technology    %String;       ""
     *  value         %String;       #IMPLIED
     *  >
     */
    // #REQUIRED
    name = parseRequiredAttribute<string>( aPart, "name" );
    library = parseRequiredAttribute<string>( aPart, "library" );
    deviceset = parseRequiredAttribute<string>( aPart, "deviceset" );
    device = parseRequiredAttribute<string>( aPart, "device" );
    technology = parseOptionalAttribute<string>( aPart, "technology" );
    value = parseOptionalAttribute<string>( aPart, "value" );

}


EINSTANCE::EINSTANCE( wxXmlNode* aInstance )
{
    /*
     *  <!ELEMENT instance (attribute)*>
     *  <!ATTLIST instance
     *     part          %String;       #REQUIRED
     *     gate          %String;       #REQUIRED
     *     x             %Coord;        #REQUIRED
     *     y             %Coord;        #REQUIRED
     *     smashed       %Bool;         "no"
     *     rot           %Rotation;     "R0"
     *     >
     */
    part    = parseRequiredAttribute<string>( aInstance, "part" );
    gate    = parseRequiredAttribute<string>( aInstance, "gate" );

    x   = parseRequiredAttribute<double>( aInstance, "x" );
    y   = parseRequiredAttribute<double>( aInstance, "y" );

    // optional
    smashed = parseOptionalAttribute<bool>( aInstance, "smashed" );
    rot = parseOptionalAttribute<EROT>( aInstance, "rot" );
}


EGATE::EGATE( wxXmlNode* aGate )
{
    /*
     *   <!ELEMENT gate EMPTY>
     *   <!ATTLIST gate
     *   name          %String;       #REQUIRED
     *   symbol        %String;       #REQUIRED
     *   x             %Coord;        #REQUIRED
     *   y             %Coord;        #REQUIRED
     *   addlevel      %GateAddLevel; "next"
     *   swaplevel     %Int;          "0"
     *   >
     */

    name = parseRequiredAttribute<string>( aGate, "name" );
    symbol = parseRequiredAttribute<string>( aGate, "symbol" );

    x   = parseRequiredAttribute<double>( aGate, "x" );
    y   = parseRequiredAttribute<double>( aGate, "y" );

    opt_string stemp = parseOptionalAttribute<string>( aGate, "addlevel" );

    // (off | value | name | both)
    if( stemp == "must" )
        addlevel = EGATE::MUST;
    else if( stemp == "can" )
        addlevel = EGATE::CAN;
    else if( stemp == "next" )
        addlevel = EGATE::NEXT;
    else if( stemp == "request" )
        addlevel = EGATE::REQUEST;
    else if( stemp == "always" )
        addlevel = EGATE::ALWAYS;
    else
        addlevel = EGATE::NEXT;
}


ECONNECT::ECONNECT( wxXmlNode* aConnect )
{
    /*
     *  <!ELEMENT connect EMPTY>
     *  <!ATTLIST connect
     *         gate          %String;       #REQUIRED
     *         pin           %String;       #REQUIRED
     *         pad           %String;       #REQUIRED
     *         route         %ContactRoute; "all"
     *         >
     */
    gate =  parseRequiredAttribute<string>( aConnect, "gate" );
    pin =  parseRequiredAttribute<string>( aConnect, "pin" );
    pad = parseRequiredAttribute<string>( aConnect, "pad" );

    //TODO:
    //int contactroute;

};


EDEVICE::EDEVICE( wxXmlNode* aDevice )
{
    /*
    <!ELEMENT device (connects?, technologies?)>
    <!ATTLIST device
              name          %String;       ""
              package       %String;       #IMPLIED
              >
*/
    name = parseRequiredAttribute<string>( aDevice, "name" );
    package = parseOptionalAttribute<string>( aDevice, "package" );

    NODE_MAP aDeviceChildren = MapChildren(aDevice);
    wxXmlNode* connectNode = getChildrenNodes(aDeviceChildren, "connects");

    while(connectNode){
        connects.push_back(ECONNECT(connectNode));
        connectNode = connectNode->GetNext();
    }

};


EDEVICESET::EDEVICESET( wxXmlNode* aDeviceSet )
{
    /*
    <!ELEMENT deviceset (description?, gates, devices)>
    <!ATTLIST deviceset
              name          %String;       #REQUIRED
              prefix        %String;       ""
              uservalue     %Bool;         "no"
              >
    */

    name = parseRequiredAttribute<string>(aDeviceSet, "name");
    prefix = parseOptionalAttribute<string>( aDeviceSet, "prefix" );
    uservalue  =  parseOptionalAttribute<bool>( aDeviceSet, "uservalue" );

    /* Russell: Parsing of devices and gates moved to sch_eagle_plugin.cpp
    *
    //TODO: description

    NODE_MAP aDeviceSetChildren = MapChildren(aDeviceSet);
    wxXmlNode* deviceNode = getChildrenNodes(aDeviceSetChildren, "device");

    while(deviceNode){
        devices.push_back(EDEVICE(deviceNode));
        deviceNode->GetNext();
    }

    wxXmlNode* gateNode = getChildrenNodes(aDeviceSetChildren, "gate");

    while(gateNode){
        gates.push_back(EGATE(gateNode));
        gateNode->GetNext();
    }
    */

}
