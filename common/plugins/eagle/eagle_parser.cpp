/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012-2021 KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2017 CERN.
 *
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

#include <plugins/eagle/eagle_parser.h>

#include <string_utils.h>
#include <richio.h>
#include <wx/log.h>

#include <functional>
#include <cstdio>

constexpr auto DEFAULT_ALIGNMENT = ETEXT::BOTTOM_LEFT;


wxString escapeName( const wxString& aNetName )
{
    wxString ret( aNetName );

    ret.Replace( wxT( "!" ), wxT( "~" ) );

    return ConvertToNewOverbarNotation( ret );
}


template<> template<>
OPTIONAL_XML_ATTRIBUTE<wxString>::OPTIONAL_XML_ATTRIBUTE( wxString aData )
{
    m_isAvailable = !aData.IsEmpty();

    if( m_isAvailable )
        Set( aData );
}


ECOORD::ECOORD( const wxString& aValue, enum ECOORD::EAGLE_UNIT aUnit )
{
    // This array is used to adjust the fraction part value basing on the number of digits
    // in the fraction.
    constexpr int DIVIDERS[] = { 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000 };
    constexpr unsigned int DIVIDERS_MAX_IDX = sizeof( DIVIDERS ) / sizeof( DIVIDERS[0] ) - 1;

    int integer, fraction, pre_fraction, post_fraction;

    // The following check is needed to handle correctly negative fractions where the integer
    // part == 0.
    bool negative = ( aValue[0] == '-' );

    // %n is used to find out how many digits contains the fraction part, e.g. 0.001 contains 3
    // digits.
    int ret = sscanf( aValue.c_str(), "%d.%n%d%n", &integer, &pre_fraction, &fraction,
                      &post_fraction );

    if( ret == 0 )
        throw XML_PARSER_ERROR( "Invalid coordinate" );

    // process the integer part
    value = ConvertToNm( integer, aUnit );

    // process the fraction part
    if( ret == 2 )
    {
        int digits = post_fraction - pre_fraction;

        // adjust the number of digits if necessary as we cannot handle anything smaller than
        // nanometers (rounding).
        if( (unsigned) digits > DIVIDERS_MAX_IDX )
        {
            int diff = digits - DIVIDERS_MAX_IDX;
            digits = DIVIDERS_MAX_IDX;
            fraction /= DIVIDERS[diff];
        }

        int frac_value = ConvertToNm( fraction, aUnit ) / DIVIDERS[digits];

        // keep the sign in mind
        value = negative ? value - frac_value : value + frac_value;
    }
}


long long int ECOORD::ConvertToNm( int aValue, enum EAGLE_UNIT aUnit )
{
    long long int ret;

    switch( aUnit )
    {
        default:
        case EU_NM:    ret = aValue; break;
        case EU_MM:    ret = (long long) aValue * 1000000; break;
        case EU_INCH:  ret = (long long) aValue * 25400000; break;
        case EU_MIL:   ret = (long long) aValue * 25400; break;
    }

    if( ( ret > 0 ) != ( aValue > 0 ) )
        wxLogError( _( "Invalid size %lld: too large" ), aValue );

    return ret;
}


// Template specializations below parse wxString to the used types:
//      - wxString (preferred)
//      - string
//      - double
//      - int
//      - bool
//      - EROT
//      - ECOORD

template <>
wxString Convert<wxString>( const wxString& aValue )
{
    return aValue;
}


template <>
std::string Convert<std::string>( const wxString& aValue )
{
    return std::string( aValue.ToUTF8() );
}


template <>
double Convert<double>( const wxString& aValue )
{
    double value;

    if( aValue.ToCDouble( &value ) )
        return value;
    else
        throw XML_PARSER_ERROR( "Conversion to double failed. Original value: '" +
                                aValue.ToStdString() + "'." );
}


template <>
int Convert<int>( const wxString& aValue )
{
    if( aValue.IsEmpty() )
        throw XML_PARSER_ERROR( "Conversion to int failed. Original value is empty." );

    return wxAtoi( aValue );
}


template <>
bool Convert<bool>( const wxString& aValue )
{
    if( aValue != wxT( "yes" ) && aValue != wxT( "no" ) )
        throw XML_PARSER_ERROR( "Conversion to bool failed. Original value, '" +
                                aValue.ToStdString() +
                                "', is neither 'yes' nor 'no'." );

    return aValue == wxT( "yes" );
}


/// parse an Eagle XML "rot" field.  Unfortunately the DTD seems not to explain
/// this format very well.  [S][M]R<degrees>.   Examples: "R90", "MR180", "SR180"
template<>
EROT Convert<EROT>( const wxString& aRot )
{
    EROT value;

    value.spin    = aRot.find( 'S' ) != aRot.npos;
    value.mirror  = aRot.find( 'M' ) != aRot.npos;
    value.degrees = strtod( aRot.c_str()
                            + 1                        // skip leading 'R'
                            + int( value.spin )       // skip optional leading 'S'
                            + int( value.mirror ),    // skip optional leading 'M'
                            nullptr );

    return value;
}


template<>
ECOORD Convert<ECOORD>( const wxString& aCoord )
{
    // Eagle uses millimeters as the default unit
    return ECOORD( aCoord, ECOORD::EAGLE_UNIT::EU_MM );
}


/**
 * Parse \a aAttribute of the XML node \a aNode.
 *
 * @param  aNode      is the node whose attribute will be parsed.
 * @param  aAttribute is the attribute that will be parsed.
 * @throw  XML_PARSER_ERROR - exception thrown if the required attribute is missing
 * @return T - the attributed parsed as the specified type.
 */
template<typename T>
T parseRequiredAttribute( wxXmlNode* aNode, const wxString& aAttribute )
{
    wxString value;

    if( aNode->GetAttribute( aAttribute, &value ) )
        return Convert<T>( value );
    else
        throw XML_PARSER_ERROR( "The required attribute " + aAttribute + " is missing." );
}


/**
 * Parse option \a aAttribute of the XML node \a aNode.
 *
 * @param  aNode      is the node whose attribute will be parsed.
 * @param  aAttribute is the attribute that will be parsed.
 * @return OPTIONAL_XML_ATTRIBUTE<T> - an optional XML attribute, parsed as the specified type if
 *                                     found.
 */
template<typename T>
OPTIONAL_XML_ATTRIBUTE<T> parseOptionalAttribute( wxXmlNode* aNode, const wxString& aAttribute )
{
    return OPTIONAL_XML_ATTRIBUTE<T>( aNode->GetAttribute( aAttribute ) );
}


NODE_MAP MapChildren( wxXmlNode* aCurrentNode )
{
    // Map node_name -> node_pointer
    NODE_MAP nodesMap;

    // Loop through all children mapping them in nodesMap
    if( aCurrentNode )
        aCurrentNode = aCurrentNode->GetChildren();

    while( aCurrentNode )
    {
        // Create a new pair in the map
        //      key: current node name
        //      value: current node pointer
        nodesMap[aCurrentNode->GetName()] = aCurrentNode;

        // Get next child
        aCurrentNode = aCurrentNode->GetNext();
    }

    return nodesMap;
}


wxPoint ConvertArcCenter( const wxPoint& aStart, const wxPoint& aEnd, double aAngle )
{
    // Eagle give us start and end.
    // S_ARC wants start to give the center, and end to give the start.
    double dx = aEnd.x - aStart.x, dy = aEnd.y - aStart.y;
    wxPoint mid = ( aStart + aEnd ) / 2;

    double dlen = sqrt( dx*dx + dy*dy );

    if( !std::isnormal( dlen ) || !std::isnormal( aAngle ) )
    {
        THROW_IO_ERROR( wxString::Format( _( "Invalid Arc with radius %f and angle %f" ),
                                          dlen,
                                          aAngle ) );
    }

    double dist = dlen / ( 2 * tan( DEG2RAD( aAngle ) / 2 ) );

    wxPoint center(
        mid.x + dist * ( dy / dlen ),
        mid.y - dist * ( dx / dlen )
    );

    return center;
}


static int parseAlignment( const wxString& aAlignment )
{
    // (bottom-left | bottom-center | bottom-right | center-left |
    // center | center-right | top-left | top-center | top-right)
    if( aAlignment == wxT( "center" ) )
        return ETEXT::CENTER;
    else if( aAlignment == wxT( "center-right" ) )
        return ETEXT::CENTER_RIGHT;
    else if( aAlignment == wxT( "top-left" ) )
        return ETEXT::TOP_LEFT;
    else if( aAlignment == wxT( "top-center" ) )
        return ETEXT::TOP_CENTER;
    else if( aAlignment == wxT( "top-right" ) )
        return ETEXT::TOP_RIGHT;
    else if( aAlignment == wxT( "bottom-left" ) )
        return ETEXT::BOTTOM_LEFT;
    else if( aAlignment == wxT( "bottom-center" ) )
        return ETEXT::BOTTOM_CENTER;
    else if( aAlignment == wxT( "bottom-right" ) )
        return ETEXT::BOTTOM_RIGHT;
    else if( aAlignment == wxT( "center-left" ) )
        return ETEXT::CENTER_LEFT;

    return DEFAULT_ALIGNMENT;
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

    x1    = parseRequiredAttribute<ECOORD>( aWire, wxT( "x1" ) );
    y1    = parseRequiredAttribute<ECOORD>( aWire, wxT( "y1" ) );
    x2    = parseRequiredAttribute<ECOORD>( aWire, wxT( "x2" ) );
    y2    = parseRequiredAttribute<ECOORD>( aWire, wxT( "y2" ) );
    width = parseRequiredAttribute<ECOORD>( aWire, wxT( "width" ) );
    layer = parseRequiredAttribute<int>( aWire, wxT( "layer" ) );
    curve = parseOptionalAttribute<double>( aWire, wxT( "curve" ) );

    opt_wxString s = parseOptionalAttribute<wxString>( aWire, wxT( "style" ) );

    if( s == wxT( "continuous" ) )
        style = EWIRE::CONTINUOUS;
    else if( s == wxT( "longdash" ) )
        style = EWIRE::LONGDASH;
    else if( s == wxT( "shortdash" ) )
        style = EWIRE::SHORTDASH;
    else if( s == wxT( "dashdot" ) )
        style = EWIRE::DASHDOT;

    s = parseOptionalAttribute<wxString>( aWire, wxT( "cap" ) );

    if( s == wxT( "round" ) )
        cap = EWIRE::ROUND;
    else if( s == wxT( "flat" ) )
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

    x    = parseRequiredAttribute<ECOORD>( aJunction, wxT( "x" ) );
    y    = parseRequiredAttribute<ECOORD>( aJunction, wxT( "y" ) );
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

    x    = parseRequiredAttribute<ECOORD>( aLabel, wxT( "x" ) );
    y    = parseRequiredAttribute<ECOORD>( aLabel, wxT( "y" ) );
    size = parseRequiredAttribute<ECOORD>( aLabel, wxT( "size" ) );
    layer = parseRequiredAttribute<int>( aLabel, wxT( "layer" ) );
    rot   = parseOptionalAttribute<EROT>( aLabel, wxT( "rot" ) );
    xref  = parseOptionalAttribute<wxString>( aLabel, wxT( "xref" ) );
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

    x = parseRequiredAttribute<ECOORD>( aVia, wxT( "x" ) );
    y = parseRequiredAttribute<ECOORD>( aVia, wxT( "y" ) );

    wxString ext = parseRequiredAttribute<wxString>( aVia, wxT( "extent" ) );
    sscanf( ext.c_str(), "%d-%d", &layer_front_most, &layer_back_most );

    drill = parseRequiredAttribute<ECOORD>( aVia, wxT( "drill" ) );
    diam  = parseOptionalAttribute<ECOORD>( aVia, wxT( "diameter" ) );
    shape = parseOptionalAttribute<wxString>( aVia, wxT( "shape" ) );
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

    x      = parseRequiredAttribute<ECOORD>( aCircle, wxT( "x" ) );
    y      = parseRequiredAttribute<ECOORD>( aCircle, wxT( "y" ) );
    radius = parseRequiredAttribute<ECOORD>( aCircle, wxT( "radius" ) );
    width  = parseRequiredAttribute<ECOORD>( aCircle, wxT( "width" ) );
    layer  = parseRequiredAttribute<int>( aCircle, wxT( "layer" ) );
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

    x1    = parseRequiredAttribute<ECOORD>( aRect, wxT( "x1" ) );
    y1    = parseRequiredAttribute<ECOORD>( aRect, wxT( "y1" ) );
    x2    = parseRequiredAttribute<ECOORD>( aRect, wxT( "x2" ) );
    y2    = parseRequiredAttribute<ECOORD>( aRect, wxT( "y2" ) );
    layer = parseRequiredAttribute<int>( aRect, wxT( "layer" ) );
    rot   = parseOptionalAttribute<EROT>( aRect, wxT( "rot" ) );
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

    name  = parseRequiredAttribute<wxString>( aTree, wxT( "name" ) );
    value = parseOptionalAttribute<wxString>( aTree, wxT( "value" ) );

    x     = parseOptionalAttribute<ECOORD>( aTree, wxT( "x" ) );
    y     = parseOptionalAttribute<ECOORD>( aTree, wxT( "y" ) );
    size  = parseOptionalAttribute<ECOORD>( aTree, wxT( "size" ) );

    layer = parseOptionalAttribute<int>( aTree, wxT( "layer" ) );
    ratio = parseOptionalAttribute<double>( aTree, wxT( "ratio" ) );
    rot   = parseOptionalAttribute<EROT>( aTree, wxT( "rot" ) );

    opt_wxString stemp = parseOptionalAttribute<wxString>( aTree, wxT( "display" ) );

    // (off | value | name | both)
    if( stemp == wxT( "off" ) )
        display = EATTR::Off;
    else if( stemp == wxT( "name" ) )
        display = EATTR::NAME;
    else if( stemp == wxT( "both" ) )
        display = EATTR::BOTH;
    else // "value" is the default
        display = EATTR::VALUE;

    stemp = parseOptionalAttribute<wxString>( aTree, wxT( "align" ) );

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

    x1    = parseRequiredAttribute<ECOORD>( aDimension, wxT( "x1" ) );
    y1    = parseRequiredAttribute<ECOORD>( aDimension, wxT( "y1" ) );
    x2    = parseRequiredAttribute<ECOORD>( aDimension, wxT( "x2" ) );
    y2    = parseRequiredAttribute<ECOORD>( aDimension, wxT( "y2" ) );
    x3    = parseRequiredAttribute<ECOORD>( aDimension, wxT( "x3" ) );
    y3    = parseRequiredAttribute<ECOORD>( aDimension, wxT( "y3" ) );
    layer = parseRequiredAttribute<int>( aDimension, wxT( "layer" ) );
    dimensionType = parseOptionalAttribute<wxString>( aDimension, wxT( "dtype" ) );
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
    x     = parseRequiredAttribute<ECOORD>( aText, wxT( "x" ) );
    y     = parseRequiredAttribute<ECOORD>( aText, wxT( "y" ) );
    size  = parseRequiredAttribute<ECOORD>( aText, wxT( "size" ) );
    layer = parseRequiredAttribute<int>( aText, wxT( "layer" ) );

    font  = parseOptionalAttribute<wxString>( aText, wxT( "font" ) );
    ratio = parseOptionalAttribute<double>( aText, wxT( "ratio" ) );
    rot   = parseOptionalAttribute<EROT>( aText, wxT( "rot" ) );

    opt_wxString stemp = parseOptionalAttribute<wxString>( aText, wxT( "align" ) );

    align = stemp ? parseAlignment( *stemp ) : DEFAULT_ALIGNMENT;
}


wxSize ETEXT::ConvertSize() const
{
    wxSize textsize;

    if( font )
    {
        const wxString& fontName = font.CGet();

        if( fontName == wxT( "vector" ) )
        {
            textsize = wxSize( size.ToSchUnits(), size.ToSchUnits() );
        }
        else if( fontName == wxT( "fixed" ) )
        {
            textsize = wxSize( size.ToSchUnits(), size.ToSchUnits() * 0.80 );
        }
        else
        {
            textsize = wxSize( size.ToSchUnits(), size.ToSchUnits() );
        }
    }
    else
    {
        textsize = wxSize( size.ToSchUnits() * 0.85, size.ToSchUnits() );
    }

    return textsize;
}


EFRAME::EFRAME( wxXmlNode* aFrameNode )
{
    /*
     * <!ELEMENT frame EMPTY>
     * <!ATTLIST frame
     *          x1            %Coord;       #REQUIRED
     *          y1            %Coord;       #REQUIRED
     *          x2            %Coord;       #REQUIRED
     *          y2            %Coord;       #REQUIRED
     *          columns       %Int;         #REQUIRED
     *          rows          %Int;         #REQUIRED
     *          layer         %Layer;       #REQUIRED
     *          border-left   %Bool;        "yes"
     *          border-top    %Bool;        "yes"
     *          border-right  %Bool;        "yes"
     *          border-bottom %Bool;        "yes"
     *          >
     */
    border_left = true;
    border_top = true;
    border_right = true;
    border_bottom = true;

    x1 = parseRequiredAttribute<ECOORD>( aFrameNode, wxT( "x1" ) );
    y1 = parseRequiredAttribute<ECOORD>( aFrameNode, wxT( "y1" ) );
    x2 = parseRequiredAttribute<ECOORD>( aFrameNode, wxT( "x2" ) );
    y2 = parseRequiredAttribute<ECOORD>( aFrameNode, wxT( "y2" ) );
    columns = parseRequiredAttribute<int>( aFrameNode, wxT( "columns" ) );
    rows = parseRequiredAttribute<int>( aFrameNode, wxT( "rows" ) );
    layer = parseRequiredAttribute<int>( aFrameNode, wxT( "layer" ) );
    border_left = parseOptionalAttribute<bool>( aFrameNode, wxT( "border-left" ) );
    border_top = parseOptionalAttribute<bool>( aFrameNode, wxT( "border-top" ) );
    border_right = parseOptionalAttribute<bool>( aFrameNode, wxT( "border-right" ) );
    border_bottom = parseOptionalAttribute<bool>( aFrameNode, wxT( "border-bottom" ) );
}


EPAD_COMMON::EPAD_COMMON( wxXmlNode* aPad )
{
    // #REQUIRED says DTD, throw exception if not found
    name      = parseRequiredAttribute<wxString>( aPad, wxT( "name" ) );
    x         = parseRequiredAttribute<ECOORD>( aPad, wxT( "x" ) );
    y         = parseRequiredAttribute<ECOORD>( aPad, wxT( "y" ) );
    rot      = parseOptionalAttribute<EROT>( aPad, wxT( "rot" ) );
    stop     = parseOptionalAttribute<bool>( aPad, wxT( "stop" ) );
    thermals = parseOptionalAttribute<bool>( aPad, wxT( "thermals" ) );
}


EPAD::EPAD( wxXmlNode* aPad )
    : EPAD_COMMON( aPad )
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
    drill        = parseRequiredAttribute<ECOORD>( aPad, wxT( "drill" ) );

    // Optional attributes
    diameter     = parseOptionalAttribute<ECOORD>( aPad, wxT( "diameter" ) );

    opt_wxString s = parseOptionalAttribute<wxString>( aPad, wxT( "shape" ) );

    // (square | round | octagon | long | offset)
    if( s == wxT( "square" ) )
        shape = EPAD::SQUARE;
    else if( s == wxT( "round" ) )
        shape = EPAD::ROUND;
    else if( s == wxT( "octagon" ) )
        shape = EPAD::OCTAGON;
    else if( s == wxT( "long" ) )
        shape = EPAD::LONG;
    else if( s == wxT( "offset" ) )
        shape = EPAD::OFFSET;

    first    = parseOptionalAttribute<bool>( aPad, wxT( "first" ) );
}


ESMD::ESMD( wxXmlNode* aSMD )
    : EPAD_COMMON( aSMD )
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
    dx        = parseRequiredAttribute<ECOORD>( aSMD, wxT( "dx" ) );
    dy        = parseRequiredAttribute<ECOORD>( aSMD, wxT( "dy" ) );
    layer     = parseRequiredAttribute<int>( aSMD, wxT( "layer" ) );

    roundness = parseOptionalAttribute<int>( aSMD, wxT( "roundness" ) );
    cream     = parseOptionalAttribute<bool>( aSMD, wxT( "cream" ) );
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
    name      = parseRequiredAttribute<wxString>( aPin, wxT( "name" ) );
    x         = parseRequiredAttribute<ECOORD>( aPin, wxT( "x" ) );
    y         = parseRequiredAttribute<ECOORD>( aPin, wxT( "y" ) );

    visible   = parseOptionalAttribute<wxString>( aPin, wxT( "visible" ) );
    length    = parseOptionalAttribute<wxString>( aPin, wxT( "length" ) );
    direction = parseOptionalAttribute<wxString>( aPin, wxT( "direction" ) );
    function  = parseOptionalAttribute<wxString>( aPin, wxT( "function" ) );
    swaplevel = parseOptionalAttribute<int>( aPin, wxT( "swaplevel" ) );
    rot       = parseOptionalAttribute<EROT>( aPin, wxT( "rot" ) );
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

    x = parseRequiredAttribute<ECOORD>( aVertex, wxT( "x" ) );
    y = parseRequiredAttribute<ECOORD>( aVertex, wxT( "y" ) );
    curve = parseOptionalAttribute<double>( aVertex, wxT( "curve" ) );
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
          rank          %Int;          "0"   -- 1..6 in <signal> context, 0 or 7 in
                                                <package> context --
          >
    */

    width        = parseRequiredAttribute<ECOORD>( aPolygon, wxT( "width" ) );
    layer        = parseRequiredAttribute<int>( aPolygon, wxT( "layer" ) );

    spacing      = parseOptionalAttribute<ECOORD>( aPolygon, wxT( "spacing" ) );
    isolate      = parseOptionalAttribute<ECOORD>( aPolygon, wxT( "isolate" ) );
    opt_wxString s = parseOptionalAttribute<wxString>( aPolygon, wxT( "pour" ) );

    // default pour to solid fill
    pour = EPOLYGON::SOLID;

    // (solid | hatch | cutout)
    if( s == wxT( "hatch" ) )
        pour = EPOLYGON::HATCH;
    else if( s == wxT( "cutout" ) )
        pour = EPOLYGON::CUTOUT;

    orphans  = parseOptionalAttribute<bool>( aPolygon, wxT( "orphans" ) );
    thermals = parseOptionalAttribute<bool>( aPolygon, wxT( "thermals" ) );
    rank     = parseOptionalAttribute<int>( aPolygon, wxT( "rank" ) );
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
    x     = parseRequiredAttribute<ECOORD>( aHole, wxT( "x" ) );
    y     = parseRequiredAttribute<ECOORD>( aHole, wxT( "y" ) );
    drill = parseRequiredAttribute<ECOORD>( aHole, wxT( "drill" ) );
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
    name    = parseRequiredAttribute<wxString>( aElement, wxT( "name" ) );
    library = parseRequiredAttribute<wxString>( aElement, wxT( "library" ) );
    value   = parseRequiredAttribute<wxString>( aElement, wxT( "value" ) );
    std::string p = parseRequiredAttribute<std::string>( aElement, wxT( "package" ) );
    ReplaceIllegalFileNameChars( &p, '_' );
    package = wxString::FromUTF8( p.c_str() );

    x       = parseRequiredAttribute<ECOORD>( aElement, wxT( "x" ) );
    y       = parseRequiredAttribute<ECOORD>( aElement, wxT( "y" ) );

    // optional
    locked  = parseOptionalAttribute<bool>( aElement, wxT( "locked" ) );
    smashed = parseOptionalAttribute<bool>( aElement, wxT( "smashed" ) );
    rot     = parseOptionalAttribute<EROT>( aElement, wxT( "rot" ) );
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

    number  = parseRequiredAttribute<int>( aLayer, wxT( "number" ) );
    name    = parseRequiredAttribute<wxString>( aLayer, wxT( "name" ) );
    color   = parseRequiredAttribute<int>( aLayer, wxT( "color" ) );
    fill    = 1;    // Temporary value.
    visible = parseOptionalAttribute<bool>( aLayer, wxT( "visible" ) );
    active  = parseOptionalAttribute<bool>( aLayer, wxT( "active" ) );
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
    name = parseRequiredAttribute<wxString>( aPart, wxT( "name" ) );
    library = parseRequiredAttribute<wxString>( aPart, wxT( "library" ) );
    deviceset = parseRequiredAttribute<wxString>( aPart, wxT( "deviceset" ) );
    device = parseRequiredAttribute<wxString>( aPart, wxT( "device" ) );
    technology = parseOptionalAttribute<wxString>( aPart, wxT( "technology" ) );
    value = parseOptionalAttribute<wxString>( aPart, wxT( "value" ) );

    for( wxXmlNode* child = aPart->GetChildren(); child; child = child->GetNext() )
    {
        if( child->GetName() == wxT( "attribute" ) )
        {
            std::string aname, avalue;

            for( wxXmlAttribute* x = child->GetAttributes(); x; x = x->GetNext() )
            {
                if( x->GetName() == wxT( "name" ) )
                    aname = x->GetValue();
                else if( x->GetName() == wxT( "value" ) )
                    avalue = x->GetValue();
            }

            if( aname.size() && avalue.size() )
                attribute[aname] = avalue;
        }
        else if( child->GetName() == wxT( "variant" ) )
        {
            std::string aname, avalue;

            for( wxXmlAttribute* x = child->GetAttributes(); x; x = x->GetNext() )
            {
                if( x->GetName() == wxT( "name" ) )
                    aname = x->GetValue();
                else if( x->GetName() == wxT( "value" ) )
                    avalue = x->GetValue();
            }

            if( aname.size() && avalue.size() )
                variant[aname] = avalue;
        }
    }
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
    part    = parseRequiredAttribute<wxString>( aInstance, wxT( "part" ) );
    gate    = parseRequiredAttribute<wxString>( aInstance, wxT( "gate" ) );

    x   = parseRequiredAttribute<ECOORD>( aInstance, wxT( "x" ) );
    y   = parseRequiredAttribute<ECOORD>( aInstance, wxT( "y" ) );

    // optional
    smashed = parseOptionalAttribute<bool>( aInstance, wxT( "smashed" ) );
    rot = parseOptionalAttribute<EROT>( aInstance, wxT( "rot" ) );
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

    name = parseRequiredAttribute<wxString>( aGate, wxT( "name" ) );
    symbol = parseRequiredAttribute<wxString>( aGate, wxT( "symbol" ) );

    x   = parseRequiredAttribute<ECOORD>( aGate, wxT( "x" ) );
    y   = parseRequiredAttribute<ECOORD>( aGate, wxT( "y" ) );

    opt_wxString stemp = parseOptionalAttribute<wxString>( aGate, wxT( "addlevel" ) );

    // (off | value | name | both)
    if( stemp == wxT( "must" ) )
        addlevel = EGATE::MUST;
    else if( stemp == wxT( "can" ) )
        addlevel = EGATE::CAN;
    else if( stemp == wxT( "next" ) )
        addlevel = EGATE::NEXT;
    else if( stemp == wxT( "request" ) )
        addlevel = EGATE::REQUEST;
    else if( stemp == wxT( "always" ) )
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
    gate =  parseRequiredAttribute<wxString>( aConnect, wxT( "gate" ) );
    pin =  parseRequiredAttribute<wxString>( aConnect, wxT( "pin" ) );
    pad = parseRequiredAttribute<wxString>( aConnect, wxT( "pad" ) );
}


EDEVICE::EDEVICE( wxXmlNode* aDevice )
{
    /*
        <!ELEMENT device (connects?, technologies?)>
        <!ATTLIST device
                name          %String;       ""
                package       %String;       #IMPLIED
                >
     */
    name = parseRequiredAttribute<wxString>( aDevice, wxT( "name" ) );
    opt_wxString pack = parseOptionalAttribute<wxString>( aDevice, wxT( "package" ) );

    if( pack )
    {
        std::string p( pack->c_str() );
        ReplaceIllegalFileNameChars( &p, '_' );
        package.Set( wxString::FromUTF8( p.c_str() ) );
    }

    NODE_MAP   aDeviceChildren = MapChildren( aDevice );
    wxXmlNode* connectNode = getChildrenNodes( aDeviceChildren, wxT( "connects" ) );

    while( connectNode )
    {
        connects.emplace_back( connectNode );
        connectNode = connectNode->GetNext();
    }
}


EDEVICE_SET::EDEVICE_SET( wxXmlNode* aDeviceSet )
{
    /*
      <!ELEMENT deviceset (description?, gates, devices)>
      <!ATTLIST deviceset
              name          %String;       #REQUIRED
              prefix        %String;       ""
              uservalue     %Bool;         "no"
              >
     */

    name = parseRequiredAttribute<wxString>( aDeviceSet, wxT( "name" ) );
    prefix = parseOptionalAttribute<wxString>( aDeviceSet, wxT( "prefix" ) );
    uservalue = parseOptionalAttribute<bool>( aDeviceSet, wxT( "uservalue" ) );

    /* Russell: Parsing of devices and gates moved to sch_eagle_plugin.cpp
    *
    //TODO: description

    NODE_MAP aDeviceSetChildren = MapChildren(aDeviceSet);
    wxXmlNode* deviceNode = getChildrenNodes(aDeviceSetChildren, wxT( "device" ));

    while(deviceNode){
        devices.push_back(EDEVICE(deviceNode));
        deviceNode->GetNext();
    }

    wxXmlNode* gateNode = getChildrenNodes(aDeviceSetChildren, wxT( "gate" ));

    while(gateNode){
        gates.push_back(EGATE(gateNode));
        gateNode->GetNext();
    }
    */

}


ECLASS::ECLASS( wxXmlNode* aClass )
{
    number = parseRequiredAttribute<wxString>( aClass, wxT( "number" ) );
    name   = parseRequiredAttribute<wxString>( aClass, wxT( "name" ) );

    for( wxXmlNode* child = aClass->GetChildren(); child; child = child->GetNext() )
    {
        if( child->GetName() == wxT( "clearance" ) )
        {
            wxString to = parseRequiredAttribute<wxString>( child, wxT( "class" ) );
            ECOORD value = parseRequiredAttribute<ECOORD>( child, wxT( "value" ) );

            clearanceMap[to] = value;
        }
    }
}
