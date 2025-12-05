/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <io/eagle/eagle_parser.h>

#include <core/profile.h>
#include <io/io_base.h>

#include <string_utils.h>
#include <richio.h>
#include <trace_helpers.h>
#include <wx/log.h>
#include <wx/regex.h>
#include <wx/tokenzr.h>

#include <functional>
#include <cstdio>
#include <array>

constexpr auto DEFAULT_ALIGNMENT = ETEXT::BOTTOM_LEFT;


wxString escapeName( const wxString& aNetName )
{
    wxString ret( aNetName );

    ret.Replace( "!", "~" );

    return ConvertToNewOverbarNotation( ret );
}


wxString interpretText( const wxString& aText )
{
    wxString token = aText;

    if( substituteVariable( &token ) )
        return token;

    wxString text;
    bool sectionOpen = false;

    for( wxString::size_type i = 0; i < aText.size(); i++ )
    {
        // Interpret escaped characters
        if( aText[ i ] == '\\' )
        {
            if( i + 1 != aText.size() )
                text.Append( aText[ i + 1 ] );

            i++;
            continue;
        }

        // Escape ~ for KiCAD
        if( aText[i] == '~' )
        {
            text.Append( '~' );
            text.Append( '~' );
            continue;
        }

        if( aText[ i ] == '!' )
        {
            if( sectionOpen )
            {
                text.Append( '~' );
                sectionOpen = false;
                continue;
            }

            static wxString escapeChars( wxT( " )]}'\"" ) );

            if( i + 1 != aText.size() && escapeChars.Find( aText[i + 1] ) == wxNOT_FOUND )
            {
                sectionOpen = true;
                text.Append( '~' );
            }
            else
            {
                text.Append( aText[ i ] );
            }

            continue;
        }

        if( aText[i] == ',' && sectionOpen )
        {
            text.Append( '~' );
            sectionOpen = false;
        }

        text.Append( aText[ i ] );
    }

    return text;
}


bool substituteVariable( wxString* aText )
{
    if( aText->StartsWith( '>' ) && aText->AfterFirst( ' ' ).IsEmpty() )
    {
        wxString token = aText->Upper();

        if     ( token == wxT( ">NAME" ) )             *aText = wxT( "${REFERENCE}" );
        else if( token == wxT( ">VALUE" ) )            *aText = wxT( "${VALUE}" );
        else if( token == wxT( ">PART" ) )             *aText = wxT( "${REFERENCE}" );
        else if( token == wxT( ">GATE" ) )             *aText = wxT( "${UNIT}" );
        else if( token == wxT( ">MODULE" ) )           *aText = wxT( "${FOOTPRINT_NAME}" );
        else if( token == wxT( ">SHEETNR" ) )          *aText = wxT( "${#}" );
        else if( token == wxT( ">SHEETS" ) )           *aText = wxT( "${##}" );
        else if( token == wxT( ">SHEET" ) )            *aText = wxT( "${#}/${##}" );
        else if( token == wxT( ">SHEETNR_TOTAL" ) )    *aText = wxT( "${#}" );
        else if( token == wxT( ">SHEETS_TOTAL" ) )     *aText = wxT( "${##}" );
        else if( token == wxT( ">SHEET_TOTAL" ) )      *aText = wxT( "${#}/${##}" );
        else if( token == wxT( ">SHEET_HEADLINE" ) )   *aText = wxT( "${SHEETNAME}" );
        else if( token == wxT( ">ASSEMBLY_VARIANT" ) ) *aText = wxT( "${ASSEMBLY_VARIANT}" );
        else if( token == wxT( ">DRAWING_NAME" ) )     *aText = wxT( "${PROJECTNAME}" );
        else if( token == wxT( ">LAST_DATE_TIME" ) )   *aText = wxT( "${CURRENT_DATE}" );
        else if( token == wxT( ">PLOT_DATE_TIME" ) )   *aText = wxT( "${CURRENT_DATE}" );
        else *aText = wxString::Format( wxS( "${%s}" ), aText->Mid( 1 ).Trim() );

        return true;
    }

    return false;
}


wxString convertDescription( wxString aDescr )
{
    aDescr.Replace( wxS( "\n" ), wxS( " " ) );
    aDescr.Replace( wxS( "\r" ), wxEmptyString );

    wxRegEx( wxS( "<a\\s+(?:[^>]*?\\s+)?href=\"([^\"]*)\"[^>]*>" ) )
            .ReplaceAll( &aDescr, wxS( "\\1 " ) );

    aDescr.Replace( wxS( "<p>" ), wxS( "\n\n" ) );
    aDescr.Replace( wxS( "</p>" ), wxS( "\n\n" ) );

    aDescr.Replace( wxS( "<br>" ), wxS( "\n" ) );
    aDescr.Replace( wxS( "<ul>" ), wxS( "\n" ) );
    aDescr.Replace( wxS( "</ul>" ), wxS( "\n\n" ) );
    aDescr.Replace( wxS( "<li></li>" ), wxS( "\n" ) );
    aDescr.Replace( wxS( "<li>" ), wxS( "\n \u2022 " ) ); // Bullet point

    aDescr = RemoveHTMLTags( aDescr );

    wxRegEx( wxS( "\n +" ) ).ReplaceAll( &aDescr, wxS( "\n" ) );
    wxRegEx( wxS( " +\n" ) ).ReplaceAll( &aDescr, wxS( "\n" ) );

    wxRegEx( wxS( "\n{3,}" ) ).ReplaceAll( &aDescr, wxS( "\n\n" ) );
    wxRegEx( wxS( "^\n+" ) ).ReplaceAll( &aDescr, wxEmptyString );
    wxRegEx( wxS( "\n+$" ) ).ReplaceAll( &aDescr, wxEmptyString );

    return aDescr;
}


size_t GetNodeCount( const wxXmlNode* aNode )
{
    size_t cnt = 0;

    PROF_TIMER timer;

    std::function<size_t( const wxXmlNode* )> countNodes =
            [&]( const wxXmlNode* node )
            {
                size_t count = 0;

                while( node )
                {
                    if( const wxXmlNode* child =  node->GetChildren() )
                        count += countNodes( child );
                    else
                        count++;

                    node = node->GetNext();
                }

                return count;
            };

    cnt = countNodes( aNode );

    timer.Stop();

    wxLogTrace( traceEagleIo, wxS( "XML node '%s' count = %zu took %0.4f ms." ),
                aNode->GetName(), cnt, timer.msecs() );

    return cnt;
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
    static std::array<int, 9> DIVIDERS = { 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000 };

    int integer, pre_fraction, post_fraction;
    long long unsigned fraction;

    // The following check is needed to handle correctly negative fractions where the integer
    // part == 0.
    bool negative = ( aValue[0] == '-' );

    // %n is used to find out how many digits contains the fraction part, e.g. 0.001 contains 3
    // digits.
    int ret = sscanf( aValue.c_str(), "%d.%n%llu%n", &integer, &pre_fraction, &fraction,
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
        if( digits >= static_cast<int>( DIVIDERS.size() ) )
        {
            long long unsigned denom = pow( 10, digits - DIVIDERS.size() + 1 );
            digits = DIVIDERS.size() - 1;
            fraction /= denom;
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


EURN::EURN( const wxString& aUrn )
{
    Parse( aUrn );
}


void EURN::Parse( const wxString& aUrn )
{
    wxStringTokenizer tokens( aUrn, ":" );

    host = tokens.GetNextToken();
    path = tokens.GetNextToken();
    assetType = tokens.GetNextToken();

    // Split off the version if there is one.
    wxString tmp = tokens.GetNextToken();

    assetId = tmp.BeforeFirst( '/' );
    assetVersion = tmp.AfterLast( '/' );
}


bool EURN::IsValid() const
{
    if( host != "urn" )
        return false;

    if( path.IsEmpty() )
        return false;

    static std::set<wxString> validAssetTypes =
    {
        "component",
        "footprint",
        "library",
        "package",
        "symbol",
        "fs.file"
    };

    if( validAssetTypes.count( assetType ) == 0 )
        return false;

    if( assetId.IsEmpty() )
        return false;

    return true;
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
    if( aValue != "yes" && aValue != "no" )
        throw XML_PARSER_ERROR( "Conversion to bool failed. Original value, '" +
                                aValue.ToStdString() +
                                "', is neither 'yes' nor 'no'." );

    return aValue == "yes";
}


/// parse an Eagle XML "rot" field.  Unfortunately the DTD seems not to explain
/// this format very well.  [S][M]R<degrees>.   Examples: "R90", "MR180", "SR180"
template<>
EROT Convert<EROT>( const wxString& aRot )
{
    EROT value;

    value.spin    = aRot.find( 'S' ) != aRot.npos;
    value.mirror  = aRot.find( 'M' ) != aRot.npos;

    size_t rPos = aRot.find( 'R' );

    if( rPos == wxString::npos )
    {
        value.degrees = 0.0;
        return value;
    }

    // Calculate the offset after 'R', 'S', and 'M'
    size_t offset;

    for( offset = 0; offset < aRot.size(); offset++ )
    {
        if( wxIsdigit( aRot[offset] ) )
            break;
    }

    wxString degreesStr = aRot.Mid( offset );

    // Use locale-independent conversion
    if( !degreesStr.ToCDouble( &value.degrees ) )
        value.degrees = 0.0;

    return value;
}


template<>
ECOORD Convert<ECOORD>( const wxString& aCoord )
{
    // Eagle uses millimeters as the default unit
    return ECOORD( aCoord, ECOORD::EAGLE_UNIT::EU_MM );
}


template<>
EURN Convert<EURN>( const wxString& aUrn )
{
    return EURN( aUrn );
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
        throw XML_PARSER_ERROR( "The required attribute " + aAttribute + " is missing at "
                                "line " + wxString::Format( "%d", aNode->GetLineNumber() ) +
                                "." );
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


VECTOR2I ConvertArcCenter( const VECTOR2I& aStart, const VECTOR2I& aEnd, double aAngle )
{
    // Eagle give us start and end.
    // S_ARC wants start to give the center, and end to give the start.
    double dx = aEnd.x - aStart.x, dy = aEnd.y - aStart.y;
    VECTOR2I mid = ( aStart + aEnd ) / 2;

    double dlen = sqrt( dx*dx + dy*dy );

    if( !std::isnormal( dlen ) || !std::isnormal( aAngle ) )
    {
        // Note that we allow the floating point output here because this message is displayed to the user and should
        // be in their locale.
        THROW_IO_ERROR( wxString::Format( _( "Invalid Arc with radius %0.2f and angle %0.2f" ), //format:allow
                                          dlen,
                                          aAngle ) );
    }

    double dist = dlen / ( 2 * tan( DEG2RAD( aAngle ) / 2 ) );

    VECTOR2I center(
        mid.x + dist * ( dy / dlen ),
        mid.y - dist * ( dx / dlen )
    );

    return center;
}


static int parseAlignment( const wxString& aAlignment )
{
    // (bottom-left | bottom-center | bottom-right | center-left |
    // center | center-right | top-left | top-center | top-right)
    if( aAlignment == "center" )
        return ETEXT::CENTER;
    else if( aAlignment == "center-right" )
        return ETEXT::CENTER_RIGHT;
    else if( aAlignment == "top-left" )
        return ETEXT::TOP_LEFT;
    else if( aAlignment == "top-center" )
        return ETEXT::TOP_CENTER;
    else if( aAlignment == "top-right" )
        return ETEXT::TOP_RIGHT;
    else if( aAlignment == "bottom-left" )
        return ETEXT::BOTTOM_LEFT;
    else if( aAlignment == "bottom-center" )
        return ETEXT::BOTTOM_CENTER;
    else if( aAlignment == "bottom-right" )
        return ETEXT::BOTTOM_RIGHT;
    else if( aAlignment == "center-left" )
        return ETEXT::CENTER_LEFT;

    return DEFAULT_ALIGNMENT;
}


void EAGLE_BASE::Report( const wxString& aMsg, SEVERITY aSeverity )
{
    if( !io )
        return;

    io->Report( aMsg, aSeverity );
}


void EAGLE_BASE::AdvanceProgressPhase()
{
    if( !io )
        return;

    io->AdvanceProgressPhase();
}


EWIRE::EWIRE( wxXmlNode* aWire, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
{
    /*
     * <!ELEMENT wire EMPTY>
     * <!ATTLIST wire
     *           x1            %Coord;        #REQUIRED
     *           y1            %Coord;        #REQUIRED
     *           x2            %Coord;        #REQUIRED
     *           y2            %Coord;        #REQUIRED
     *           width         %Dimension;    #REQUIRED
     *           layer         %Layer;        #REQUIRED
     *           extent        %Extent;       #IMPLIED  -- only applicable for airwires --
     *           style         %WireStyle;    "continuous"
     *           curve         %WireCurve;    "0"
     *           cap           %WireCap;      "round"   -- only applicable if 'curve' is not zero --
     *           >
     */

    x1    = parseRequiredAttribute<ECOORD>( aWire, "x1" );
    y1    = parseRequiredAttribute<ECOORD>( aWire, "y1" );
    x2    = parseRequiredAttribute<ECOORD>( aWire, "x2" );
    y2    = parseRequiredAttribute<ECOORD>( aWire, "y2" );
    width = parseRequiredAttribute<ECOORD>( aWire, "width" );
    layer = parseRequiredAttribute<int>( aWire, "layer" );
    curve = parseOptionalAttribute<double>( aWire, "curve" );

    opt_wxString s = parseOptionalAttribute<wxString>( aWire, "style" );

    if( s == "continuous" )
        style = EWIRE::CONTINUOUS;
    else if( s == "longdash" )
        style = EWIRE::LONGDASH;
    else if( s == "shortdash" )
        style = EWIRE::SHORTDASH;
    else if( s == "dashdot" )
        style = EWIRE::DASHDOT;

    s = parseOptionalAttribute<wxString>( aWire, "cap" );

    if( s == "round" )
        cap = EWIRE::ROUND;
    else if( s == "flat" )
        cap = EWIRE::FLAT;

    AdvanceProgressPhase();
}


ESEGMENT::ESEGMENT( wxXmlNode* aSegment, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
{
    /*
     * <!ELEMENT segment (pinref | portref | wire | junction | label | probe)*>
     *           <!-- 'pinref' and 'junction' are only valid in a <net> context -->
     */
    for( wxXmlNode* child = aSegment->GetChildren(); child; child = child->GetNext() )
    {
        if( child->GetName() == "pinref" )
            pinRefs.emplace_back( std::make_unique<EPINREF>( child, aIo ) );
        else if( child->GetName() == "portref" )
            portRefs.emplace_back( std::make_unique<EPORTREF>( child, aIo ) );
        else if( child->GetName() == "wire" )
            wires.emplace_back( std::make_unique<EWIRE>( child, aIo ) );
        else if( child->GetName() == "junction" )
            junctions.emplace_back( std::make_unique<EJUNCTION>( child, aIo ) );
        else if( child->GetName() == "label" )
            labels.emplace_back( std::make_unique<ELABEL>( child, aIo ) );
        else if( child->GetName() == "probe" )
            probes.emplace_back( std::make_unique<EPROBE>( child, aIo ) );
    }

    AdvanceProgressPhase();
}


EBUS::EBUS( wxXmlNode* aBus, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
{
    /*
     * <!ELEMENT bus (segment)*>
     * <!ATTLIST bus
     *           name          %String;       #REQUIRED
     *           >
     */
    name = parseRequiredAttribute<wxString>( aBus, "name" );

    for( wxXmlNode* child = aBus->GetChildren(); child; child = child->GetNext() )
    {
        if( child->GetName() == "segment" )
            segments.emplace_back( std::make_unique<ESEGMENT>( child, aIo ) );
    }

    AdvanceProgressPhase();
}


EJUNCTION::EJUNCTION( wxXmlNode* aJunction, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
{
    /*
     *    <!ELEMENT junction EMPTY>
     *    <!ATTLIST junction
     *              x             %Coord;        #REQUIRED
     *              y             %Coord;        #REQUIRED
     *              >
     */

    x    = parseRequiredAttribute<ECOORD>( aJunction, "x" );
    y    = parseRequiredAttribute<ECOORD>( aJunction, "y" );

    AdvanceProgressPhase();
}


ELABEL::ELABEL( wxXmlNode* aLabel, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
{
    /*
     * <!ELEMENT label EMPTY>
     * <!ATTLIST label
     *           x             %Coord;        #REQUIRED
     *           y             %Coord;        #REQUIRED
     *           size          %Dimension;    #REQUIRED
     *           layer         %Layer;        #REQUIRED
     *           font          %TextFont;     "proportional"
     *           ratio         %Int;          "8"
     *           rot           %Rotation;     "R0"
     *           xref          %Bool;         "no"
     *           align         %Align;        "bottom-left"
     *           grouprefs     IDREFS         #IMPLIED
     *           >
     *           <!-- rot:  Only 0, 90, 180 or 270 -->
     *           <!-- xref: Only in <net> context -->
     */
    x    = parseRequiredAttribute<ECOORD>( aLabel, "x" );
    y    = parseRequiredAttribute<ECOORD>( aLabel, "y" );
    size = parseRequiredAttribute<ECOORD>( aLabel, "size" );
    layer = parseRequiredAttribute<int>( aLabel, "layer" );
    font  = parseOptionalAttribute<wxString>( aLabel, "font" );
    ratio = parseOptionalAttribute<int>( aLabel, "ratio" );
    rot   = parseOptionalAttribute<EROT>( aLabel, "rot" );
    xref  = parseOptionalAttribute<wxString>( aLabel, "xref" );
    align  = parseOptionalAttribute<wxString>( aLabel, "align" );

    AdvanceProgressPhase();
}


ENET::ENET( wxXmlNode* aNet, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
{
    /*
     * <!ELEMENT net (segment)*>
     * <!ATTLIST net
     *           name          %String;       #REQUIRED
     *           class         %Class;        "0"
     *           >
     */
    netname = parseRequiredAttribute<wxString>( aNet, "name" );
    netcode = parseRequiredAttribute<int>( aNet, "class" );

    for( wxXmlNode* segment = aNet->GetChildren(); segment; segment = segment->GetNext() )
        segments.emplace_back( std::make_unique<ESEGMENT>( segment ) );

    AdvanceProgressPhase();
}


EVIA::EVIA( wxXmlNode* aVia, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
{
    /*
     * <!ELEMENT via EMPTY>
     * <!ATTLIST via
     *           x             %Coord;        #REQUIRED
     *           y             %Coord;        #REQUIRED
     *           extent        %Extent;       #REQUIRED
     *           drill         %Dimension;    #REQUIRED
     *           diameter      %Dimension;    "0"
     *           shape         %ViaShape;     "round"
     *           alwaysstop    %Bool;         "no"
     *           >
     */

    x = parseRequiredAttribute<ECOORD>( aVia, "x" );
    y = parseRequiredAttribute<ECOORD>( aVia, "y" );

    wxString ext = parseRequiredAttribute<wxString>( aVia, "extent" );
    sscanf( ext.c_str(), "%d-%d", &layer_front_most, &layer_back_most );

    drill = parseRequiredAttribute<ECOORD>( aVia, "drill" );
    diam  = parseOptionalAttribute<ECOORD>( aVia, "diameter" );
    shape = parseOptionalAttribute<wxString>( aVia, "shape" );

    AdvanceProgressPhase();
}


ECIRCLE::ECIRCLE( wxXmlNode* aCircle, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
{
    /*
     * <!ELEMENT circle EMPTY>
     * <!ATTLIST circle
     *           x             %Coord;        #REQUIRED
     *           y             %Coord;        #REQUIRED
     *           radius        %Coord;        #REQUIRED
     *           width         %Dimension;    #REQUIRED
     *           layer         %Layer;        #REQUIRED
     *           >
     */

    x      = parseRequiredAttribute<ECOORD>( aCircle, "x" );
    y      = parseRequiredAttribute<ECOORD>( aCircle, "y" );
    radius = parseRequiredAttribute<ECOORD>( aCircle, "radius" );
    width  = parseRequiredAttribute<ECOORD>( aCircle, "width" );
    layer  = parseRequiredAttribute<int>( aCircle, "layer" );

    AdvanceProgressPhase();
}


ERECT::ERECT( wxXmlNode* aRect, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
{
    /*
     * <!ELEMENT rectangle EMPTY>
     * <!ATTLIST rectangle
     *           x1            %Coord;        #REQUIRED
     *           y1            %Coord;        #REQUIRED
     *           x2            %Coord;        #REQUIRED
     *           y2            %Coord;        #REQUIRED
     *           layer         %Layer;        #REQUIRED
     *           rot           %Rotation;     "R0"
     *           >
     */

    x1    = parseRequiredAttribute<ECOORD>( aRect, "x1" );
    y1    = parseRequiredAttribute<ECOORD>( aRect, "y1" );
    x2    = parseRequiredAttribute<ECOORD>( aRect, "x2" );
    y2    = parseRequiredAttribute<ECOORD>( aRect, "y2" );
    layer = parseRequiredAttribute<int>( aRect, "layer" );
    rot   = parseOptionalAttribute<EROT>( aRect, "rot" );

    AdvanceProgressPhase();
}


EDESCRIPTION::EDESCRIPTION( wxXmlNode* aDescription, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
{
    /*
     * <!ELEMENT description (#PCDATA)>
     * <!ATTLIST description
     *           language      %String;       "en"
     *           >
     */

    text = aDescription->GetNodeContent();
    language = parseOptionalAttribute<wxString>( aDescription, "language" );

    AdvanceProgressPhase();
}


EATTR::EATTR( wxXmlNode* aTree, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
{
    /*
     * <!ELEMENT attribute EMPTY>
     * <!ATTLIST attribute
     *           name      %String;       #REQUIRED
     *           value     %String;       #IMPLIED
     *           x         %Coord;        #IMPLIED
     *           y         %Coord;        #IMPLIED
     *           size      %Dimension;    #IMPLIED
     *           layer     %Layer;        #IMPLIED
     *           font      %TextFont;     #IMPLIED
     *           ratio     %Int;          #IMPLIED
     *           rot       %Rotation;     "R0"
     *           display   %AttributeDisplay; "value" -- only in <element> or <instance> context --
     *           constant  %Bool;         "no"     -- only in <device> context --
     *           >
     */

    name  = parseRequiredAttribute<wxString>( aTree, "name" );
    value = parseOptionalAttribute<wxString>( aTree, "value" );

    x     = parseOptionalAttribute<ECOORD>( aTree, "x" );
    y     = parseOptionalAttribute<ECOORD>( aTree, "y" );
    size  = parseOptionalAttribute<ECOORD>( aTree, "size" );

    layer = parseOptionalAttribute<int>( aTree, "layer" );
    ratio = parseOptionalAttribute<double>( aTree, "ratio" );
    rot   = parseOptionalAttribute<EROT>( aTree, "rot" );

    opt_wxString stemp = parseOptionalAttribute<wxString>( aTree, "display" );

    // (off | value | name | both)
    if( stemp == "off" )
        display = EATTR::Off;
    else if( stemp == "name" )
        display = EATTR::NAME;
    else if( stemp == "both" )
        display = EATTR::BOTH;
    else // "value" is the default
        display = EATTR::VALUE;

    stemp = parseOptionalAttribute<wxString>( aTree, "align" );

    align = stemp ? parseAlignment( *stemp ) : DEFAULT_ALIGNMENT;

    AdvanceProgressPhase();
}


EPINREF::EPINREF( wxXmlNode* aPinRef, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
{
    /*
     * <!ELEMENT pinref EMPTY>
     * <!ATTLIST pinref
     *           part          %String;       #REQUIRED
     *           gate          %String;       #REQUIRED
     *           pin           %String;       #REQUIRED
     *           >
     */
    part = parseRequiredAttribute<wxString>( aPinRef, "part" );
    gate = parseRequiredAttribute<wxString>( aPinRef, "gate" );
    pin = parseRequiredAttribute<wxString>( aPinRef, "pin" );

    AdvanceProgressPhase();
}


EPORTREF::EPORTREF( wxXmlNode* aPortRef, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
{
    /*
     * <!ELEMENT portref EMPTY>
     * <!ATTLIST portref
     *           moduleinst    %String;       #REQUIRED
     *           port          %String;       #REQUIRED
     *           >
     */
    moduleinst = parseRequiredAttribute<wxString>( aPortRef, "moduleinst" );
    port = parseRequiredAttribute<wxString>( aPortRef, "port" );

    AdvanceProgressPhase();
}


EPROBE::EPROBE( wxXmlNode* aProbe, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
{
    /*
     * <!ELEMENT probe EMPTY>
     * <!ATTLIST probe
     *         x             %Coord;        #REQUIRED
     *         y             %Coord;        #REQUIRED
     *         size          %Dimension;    #REQUIRED
     *         layer         %Layer;        #REQUIRED
     *         font          %TextFont;     "proportional"
     *         ratio         %Int;          "8"
     *         rot           %Rotation;     "R0"
     *         xref          %Bool;         "no"
     *         grouprefs     IDREFS         #IMPLIED
     *         >
     *         <!-- rot:  Only 0, 90, 180 or 270 -->
     *         <!-- xref: Only in <net> context -->
     */
    x = parseRequiredAttribute<ECOORD>( aProbe, "x" );
    y = parseRequiredAttribute<ECOORD>( aProbe, "y" );
    size = parseRequiredAttribute<double>( aProbe, "size" );
    layer = parseRequiredAttribute<int>( aProbe, "layer" );
    font = parseOptionalAttribute<wxString>( aProbe, "font" );
    ratio = parseOptionalAttribute<int>( aProbe, "ratio" );
    rot = parseOptionalAttribute<EROT>( aProbe, "rot" );
    xref = parseOptionalAttribute<bool>( aProbe, "xref" );

    AdvanceProgressPhase();
}


EDIMENSION::EDIMENSION( wxXmlNode* aDimension, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
{
    /*
     * <!ELEMENT dimension EMPTY>
     *     <!ATTLIST dimension
     *           x1            %Coord;        #REQUIRED
     *           y1            %Coord;        #REQUIRED
     *           x2            %Coord;        #REQUIRED
     *           y2            %Coord;        #REQUIRED
     *           x3            %Coord;        #REQUIRED
     *           y3            %Coord;        #REQUIRED
     *           textsize      %Coord;
     *           layer         %Layer;        #REQUIRED
     *           dtype         %DimensionType; "parallel"
     *           >
     */

    x1       = parseRequiredAttribute<ECOORD>( aDimension, wxT( "x1" ) );
    y1       = parseRequiredAttribute<ECOORD>( aDimension, wxT( "y1" ) );
    x2       = parseRequiredAttribute<ECOORD>( aDimension, wxT( "x2" ) );
    y2       = parseRequiredAttribute<ECOORD>( aDimension, wxT( "y2" ) );
    x3       = parseRequiredAttribute<ECOORD>( aDimension, wxT( "x3" ) );
    y3       = parseRequiredAttribute<ECOORD>( aDimension, wxT( "y3" ) );
    textsize = parseOptionalAttribute<ECOORD>( aDimension, wxT( "textsize" ) );
    layer    = parseRequiredAttribute<int>( aDimension, wxT( "layer" ) );
    dimensionType = parseOptionalAttribute<wxString>( aDimension, wxT( "dtype" ) );

    AdvanceProgressPhase();
}


ETEXT::ETEXT( wxXmlNode* aText, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
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
    x     = parseRequiredAttribute<ECOORD>( aText, "x" );
    y     = parseRequiredAttribute<ECOORD>( aText, "y" );
    size  = parseRequiredAttribute<ECOORD>( aText, "size" );
    layer = parseRequiredAttribute<int>( aText, "layer" );

    font  = parseOptionalAttribute<wxString>( aText, "font" );
    ratio = parseOptionalAttribute<double>( aText, "ratio" );
    rot   = parseOptionalAttribute<EROT>( aText, "rot" );

    opt_wxString stemp = parseOptionalAttribute<wxString>( aText, "align" );

    align = stemp ? parseAlignment( *stemp ) : DEFAULT_ALIGNMENT;

    AdvanceProgressPhase();
}


VECTOR2I ETEXT::ConvertSize() const
{
    VECTOR2I textsize;

    if( font )
    {
        const wxString& fontName = font.CGet();

        if( fontName == "vector" )
        {
            textsize = VECTOR2I( size.ToSchUnits(), size.ToSchUnits() );
        }
        else if( fontName == "fixed" )
        {
            textsize = VECTOR2I( size.ToSchUnits(), size.ToSchUnits() * 0.80 );
        }
        else
        {
            textsize = VECTOR2I( size.ToSchUnits(), size.ToSchUnits() );
        }
    }
    else
    {
        textsize = VECTOR2I( size.ToSchUnits() * 0.85, size.ToSchUnits() );
    }

    return textsize;
}


EFRAME::EFRAME( wxXmlNode* aFrameNode, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
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

    x1 = parseRequiredAttribute<ECOORD>( aFrameNode, "x1" );
    y1 = parseRequiredAttribute<ECOORD>( aFrameNode, "y1" );
    x2 = parseRequiredAttribute<ECOORD>( aFrameNode, "x2" );
    y2 = parseRequiredAttribute<ECOORD>( aFrameNode, "y2" );
    columns = parseRequiredAttribute<int>( aFrameNode, "columns" );
    rows = parseRequiredAttribute<int>( aFrameNode, "rows" );
    layer = parseRequiredAttribute<int>( aFrameNode, "layer" );
    border_left = parseOptionalAttribute<bool>( aFrameNode, "border-left" );
    border_top = parseOptionalAttribute<bool>( aFrameNode, "border-top" );
    border_right = parseOptionalAttribute<bool>( aFrameNode, "border-right" );
    border_bottom = parseOptionalAttribute<bool>( aFrameNode, "border-bottom" );

    AdvanceProgressPhase();
}


EPAD_COMMON::EPAD_COMMON( wxXmlNode* aPad, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
{
    // #REQUIRED says DTD, throw exception if not found
    name      = parseRequiredAttribute<wxString>( aPad, "name" );
    x         = parseRequiredAttribute<ECOORD>( aPad, "x" );
    y         = parseRequiredAttribute<ECOORD>( aPad, "y" );
    rot      = parseOptionalAttribute<EROT>( aPad, "rot" );
    stop     = parseOptionalAttribute<bool>( aPad, "stop" );
    thermals = parseOptionalAttribute<bool>( aPad, "thermals" );
}


EPAD::EPAD( wxXmlNode* aPad, IO_BASE* aIo ) :
    EPAD_COMMON( aPad, aIo )
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

    // #REQUIRED says DTD, but DipTrace doesn't write it sometimes
    drill = parseOptionalAttribute<ECOORD>( aPad, "drill" );

    // Optional attributes
    diameter = parseOptionalAttribute<ECOORD>( aPad, "diameter" );

    opt_wxString s = parseOptionalAttribute<wxString>( aPad, "shape" );

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

    first    = parseOptionalAttribute<bool>( aPad, "first" );

    AdvanceProgressPhase();
}


ESMD::ESMD( wxXmlNode* aSMD, IO_BASE* aIo ) :
    EPAD_COMMON( aSMD, aIo )
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
    dx        = parseRequiredAttribute<ECOORD>( aSMD, "dx" );
    dy        = parseRequiredAttribute<ECOORD>( aSMD, "dy" );
    layer     = parseRequiredAttribute<int>( aSMD, "layer" );

    roundness = parseOptionalAttribute<int>( aSMD, "roundness" );
    cream     = parseOptionalAttribute<bool>( aSMD, "cream" );

    AdvanceProgressPhase();
}


EPIN::EPIN( wxXmlNode* aPin, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
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
    name      = parseRequiredAttribute<wxString>( aPin, "name" );
    x         = parseRequiredAttribute<ECOORD>( aPin, "x" );
    y         = parseRequiredAttribute<ECOORD>( aPin, "y" );

    visible   = parseOptionalAttribute<wxString>( aPin, "visible" );
    length    = parseOptionalAttribute<wxString>( aPin, "length" );
    direction = parseOptionalAttribute<wxString>( aPin, "direction" );
    function  = parseOptionalAttribute<wxString>( aPin, "function" );
    swaplevel = parseOptionalAttribute<int>( aPin, "swaplevel" );
    rot       = parseOptionalAttribute<EROT>( aPin, "rot" );

    AdvanceProgressPhase();
}


EVERTEX::EVERTEX( wxXmlNode* aVertex, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
{
    /*
     * <!ELEMENT vertex EMPTY>
     * <!ATTLIST vertex
     *           x           %Coord;       #REQUIRED
     *           y           %Coord;       #REQUIRED
     *           curve       %WireCurve;   "0" -- the curvature from this vertex to the next one --
     *           >
     */

    x = parseRequiredAttribute<ECOORD>( aVertex, "x" );
    y = parseRequiredAttribute<ECOORD>( aVertex, "y" );
    curve = parseOptionalAttribute<double>( aVertex, "curve" );

    AdvanceProgressPhase();
}


EPOLYGON::EPOLYGON( wxXmlNode* aPolygon, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
{
    /*
     * <!ELEMENT polygon (vertex)*>
     *           <!-- the vertices must define a valid polygon; if the last vertex is the same
     *                as the first one, it is ignored -->
     * <!ATTLIST polygon
     *           width         %Dimension;    #REQUIRED
     *           layer         %Layer;        #REQUIRED
     *           spacing       %Dimension;    #IMPLIED
     *           pour          %PolygonPour;  "solid"
     *           isolate       %Dimension;    #IMPLIED
     *           orphans       %Bool;         "no"
     *           thermals      %Bool;         "yes"
     *           rank          %Int;          "0"
     *           grouprefs     IDREFS         #IMPLIED
     *           >
     *           <!-- isolate: Only in <signal> or <package> context -->
     *           <!-- orphans: Only in <signal> context -->
     *           <!-- thermals:Only in <signal> context -->
     *           <!-- rank:    1..6 in <signal> context, 0 or 7 in <package> context -->
     */

    width        = parseRequiredAttribute<ECOORD>( aPolygon, "width" );
    layer        = parseRequiredAttribute<int>( aPolygon, "layer" );

    spacing      = parseOptionalAttribute<ECOORD>( aPolygon, "spacing" );
    isolate      = parseOptionalAttribute<ECOORD>( aPolygon, "isolate" );
    opt_wxString s = parseOptionalAttribute<wxString>( aPolygon, "pour" );

    // default pour to solid fill
    pour = EPOLYGON::ESOLID;

    // (solid | hatch | cutout)
    if( s == "hatch" )
        pour = EPOLYGON::EHATCH;
    else if( s == "cutout" )
        pour = EPOLYGON::ECUTOUT;

    orphans  = parseOptionalAttribute<bool>( aPolygon, "orphans" );
    thermals = parseOptionalAttribute<bool>( aPolygon, "thermals" );
    rank     = parseOptionalAttribute<int>( aPolygon, "rank" );

    for( wxXmlNode* child = aPolygon->GetChildren(); child; child = child->GetNext() )
    {
        if( child->GetName() == "vertex" )
            vertices.emplace_back( std::make_unique<EVERTEX>( child, aIo ) );
    }

    AdvanceProgressPhase();
}


ESPLINE::ESPLINE( wxXmlNode* aSpline, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
{
    /*
     * <!ELEMENT spline (vertex)*>
     * <!-- Four simple (non-curve) vertices define the control points of a degree-3 spline
     *      curve -->
     * <!ATTLIST spline
     *           width          %Dimension;    #REQUIRED
     *           >
     */
    width = parseRequiredAttribute<double>( aSpline, "width" );

    for( wxXmlNode* child = aSpline->GetChildren(); child; child = child->GetNext() )
    {
        if( child->GetName() == "vertex" )
            vertices.emplace_back( std::make_unique<EVERTEX>( child, aIo ) );
    }

    AdvanceProgressPhase();
}


EVARIANT::EVARIANT( wxXmlNode* aVariant, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
{
    /*
     * <!ELEMENT variant EMPTY>
     * <!ATTLIST variant
     *           name          %String;       #REQUIRED
     *           populate      %Bool;         "yes"
     *           value         %String;       #IMPLIED
     *           technology    %String;       #IMPLIED
     *           >
     *           <!-- technology: Only in part context -->
     */
    name = parseRequiredAttribute<wxString>( aVariant, "name" );
    populate = parseOptionalAttribute<bool>( aVariant, "populate" );
    value = parseOptionalAttribute<wxString>( aVariant, "value" );
    technology = parseOptionalAttribute<wxString>( aVariant, "technology" );

    AdvanceProgressPhase();
}


EMODEL::EMODEL( wxXmlNode* aModel, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
{
    /*
     * <!ELEMENT model (#PCDATA)>
     * <!ATTLIST model
     *           name          %String;       #REQUIRED
     *           >
     */
    name = parseRequiredAttribute<wxString>( aModel, "name" );
    model = aModel->GetNodeContent();

    AdvanceProgressPhase();
}


EPINMAP::EPINMAP( wxXmlNode* aPinMap, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
{
    /*
     * <!ELEMENT pinmap EMPTY>
     * <!ATTLIST pinmap
     *           gate          %String;       #REQUIRED
     *           pin           %String;       #REQUIRED
     *           pinorder      %String;       #REQUIRED
     *           >
     */
    gate = parseRequiredAttribute<wxString>( aPinMap, "gate" );
    pin = parseRequiredAttribute<wxString>( aPinMap, "pin" );
    pinorder = parseRequiredAttribute<wxString>( aPinMap, "pinorder" );

    AdvanceProgressPhase();
}


EPINMAPPING::EPINMAPPING( wxXmlNode* aPinMapping, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
{
    /*
     * <!ELEMENT pinmapping (pinmap+)>
     * <!ATTLIST pinmapping
     *           isusermap     %Bool;         "no"
     *           iddevicewide  %Bool;         "yes"
     *           spiceprefix   %String;       ""
     *           >
     */
    isusermap = parseOptionalAttribute<bool>( aPinMapping, "isusermap" );
    iddevicewide = parseOptionalAttribute<bool>( aPinMapping, "iddevicewide" );
    spiceprefix = parseOptionalAttribute<wxString>( aPinMapping, "spiceprefix" );

    for( wxXmlNode* child = aPinMapping->GetChildren(); child; child = child->GetNext() )
    {
        if( child->GetName() == "pinmap" )
            pinmaps.emplace_back( std::make_unique<EPINMAP>( child, aIo ) );
    }

    AdvanceProgressPhase();
}


ESPICE::ESPICE( wxXmlNode* aSpice, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
{
    /*
     * <!ELEMENT spice (pinmapping, model)>
     */
    pinmapping = std::make_unique<EPINMAPPING>( aSpice );

    if( aSpice->GetName() == "model" )
        model = std::make_unique<EMODEL>( aSpice );

    AdvanceProgressPhase();
}


EHOLE::EHOLE( wxXmlNode* aHole, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
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
    x     = parseRequiredAttribute<ECOORD>( aHole, "x" );
    y     = parseRequiredAttribute<ECOORD>( aHole, "y" );
    drill = parseRequiredAttribute<ECOORD>( aHole, "drill" );

    AdvanceProgressPhase();
}


EELEMENT::EELEMENT( wxXmlNode* aElement, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
{
    /*
    <!ELEMENT element (attribute*, variant*)>
    <!ATTLIST element
          name          %String;       #REQUIRED
          library       %String;       #REQUIRED
          library_urn   %Urn;          ""
          package       %String;       #REQUIRED
          package3d_urn %Urn;          ""
          override_package3d_urn %Urn; ""
          override_package_urn %Urn;    ""
          override_locally_modified %Bool; "no"
          value         %String;       #REQUIRED
          x             %Coord;        #REQUIRED
          y             %Coord;        #REQUIRED
          locked        %Bool;         "no"
          populate      %Bool;         "yes"
          smashed       %Bool;         "no"
          rot           %Rotation;     "R0"
          grouprefs     IDREFS         #IMPLIED
          >
    */

    // #REQUIRED
    name    = parseRequiredAttribute<wxString>( aElement, "name" );
    library = parseRequiredAttribute<wxString>( aElement, "library" );
    value   = parseRequiredAttribute<wxString>( aElement, "value" );
    std::string p = parseRequiredAttribute<std::string>( aElement, "package" );
    ReplaceIllegalFileNameChars( p, '_' );
    package = wxString::FromUTF8( p.c_str() );

    x       = parseRequiredAttribute<ECOORD>( aElement, "x" );
    y       = parseRequiredAttribute<ECOORD>( aElement, "y" );

    // optional
    library_urn = parseOptionalAttribute<EURN>( aElement, "library_urn" );
    locked  = parseOptionalAttribute<bool>( aElement, "locked" );
    smashed = parseOptionalAttribute<bool>( aElement, "smashed" );
    rot     = parseOptionalAttribute<EROT>( aElement, "rot" );

    for( wxXmlNode* child = aElement->GetChildren(); child; child = child->GetNext() )
    {
        if( child->GetName() == "attribute" )
        {
            std::unique_ptr<EATTR> attr = std::make_unique<EATTR>( child, aIo );
            attributes[ attr->name ] = std::move( attr );
        }
        else if( child->GetName() == "variant" )
        {
            std::unique_ptr<EVARIANT> variant = std::make_unique<EVARIANT>( child, aIo );
            variants[ variant->name ] = std::move( variant );
        }
    }

    AdvanceProgressPhase();
}


ELAYER::ELAYER( wxXmlNode* aLayer, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
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
    name    = parseRequiredAttribute<wxString>( aLayer, "name" );
    color   = parseRequiredAttribute<int>( aLayer, "color" );
    fill    = parseRequiredAttribute<int>( aLayer, "fill" );
    visible = parseOptionalAttribute<bool>( aLayer, "visible" );
    active  = parseOptionalAttribute<bool>( aLayer, "active" );

    AdvanceProgressPhase();
}


EPART::EPART( wxXmlNode* aPart, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
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
    name = parseRequiredAttribute<wxString>( aPart, "name" );
    library = parseRequiredAttribute<wxString>( aPart, "library" );
    libraryUrn = parseOptionalAttribute<EURN>( aPart, "library_urn" );
    deviceset = parseRequiredAttribute<wxString>( aPart, "deviceset" );
    device = parseRequiredAttribute<wxString>( aPart, "device" );
    package3d_urn = parseOptionalAttribute<wxString>( aPart, "package3d_urn" );
    override_package3d_urn = parseOptionalAttribute<wxString>( aPart, "override_package3d_urn" );
    override_package_urn = parseOptionalAttribute<wxString>( aPart, "override_package_urn" );
    override_locally_modified = parseOptionalAttribute<bool>( aPart, "override_locally_modified" );
    technology = parseOptionalAttribute<wxString>( aPart, "technology" );
    value = parseOptionalAttribute<wxString>( aPart, "value" );

    for( wxXmlNode* child = aPart->GetChildren(); child; child = child->GetNext() )
    {
        if( child->GetName() == "attribute" )
        {
            std::unique_ptr<EATTR> attr = std::make_unique<EATTR>( child, aIo );
            attributes[ attr->name ] = std::move( attr );
        }
        else if( child->GetName() == "variant" )
        {
            std::unique_ptr<EVARIANT> variant = std::make_unique<EVARIANT>( child, aIo );
            variants[ variant->name ] = std::move( variant );
        }
        else if( child->GetName() == "spice" )
        {
            spice = std::make_unique<ESPICE>( child, aIo );
        }
    }

    AdvanceProgressPhase();
}


EINSTANCE::EINSTANCE( wxXmlNode* aInstance, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
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
    part    = parseRequiredAttribute<wxString>( aInstance, "part" );
    gate    = parseRequiredAttribute<wxString>( aInstance, "gate" );

    x   = parseRequiredAttribute<ECOORD>( aInstance, "x" );
    y   = parseRequiredAttribute<ECOORD>( aInstance, "y" );

    // optional
    smashed = parseOptionalAttribute<bool>( aInstance, "smashed" );
    rot = parseOptionalAttribute<EROT>( aInstance, "rot" );

    for( wxXmlNode* child = aInstance->GetChildren(); child; child = child->GetNext() )
    {
        if( child->GetName() == "attribute" )
        {
            std::unique_ptr<EATTR> attr = std::make_unique<EATTR>( child, aIo );
            attributes[ attr->name ] = std::move( attr );
        }
    }

    AdvanceProgressPhase();
}


EGATE::EGATE( wxXmlNode* aGate, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
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

    name = parseRequiredAttribute<wxString>( aGate, "name" );
    symbol = parseRequiredAttribute<wxString>( aGate, "symbol" );

    x   = parseRequiredAttribute<ECOORD>( aGate, "x" );
    y   = parseRequiredAttribute<ECOORD>( aGate, "y" );

    opt_wxString stemp = parseOptionalAttribute<wxString>( aGate, "addlevel" );

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

    swaplevel = parseOptionalAttribute<int>( aGate, "swaplevel" );

    AdvanceProgressPhase();
}


ECONNECT::ECONNECT( wxXmlNode* aConnect, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
{
    /*
     * <!ELEMENT connect EMPTY>
     * <!ATTLIST connect
     *         gate          %String;       #REQUIRED
     *         pin           %String;       #REQUIRED
     *         pad           %String;       #REQUIRED
     *         route         %ContactRoute; "all"
     *         >
     */
    gate = parseRequiredAttribute<wxString>( aConnect, "gate" );
    pin = parseRequiredAttribute<wxString>( aConnect, "pin" );
    pad = parseRequiredAttribute<wxString>( aConnect, "pad" );
    contactroute = parseOptionalAttribute<wxString>( aConnect, "contactroute" );

    AdvanceProgressPhase();
}


ETECHNOLOGY::ETECHNOLOGY( wxXmlNode* aTechnology, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
{
    /*
     * <!ELEMENT technology (attribute)*>
     * <!ATTLIST technology
     *           name          %String;       #REQUIRED
     *           >
     */
    name = parseRequiredAttribute<wxString>( aTechnology, "name" );

    for( wxXmlNode* child = aTechnology->GetChildren(); child; child = child->GetNext() )
    {
        if( child->GetName() == "attribute" )
            attributes.emplace_back( std::make_unique<EATTR>( child, aIo ) );
    }

    AdvanceProgressPhase();
}


EPACKAGE3DINST::EPACKAGE3DINST( wxXmlNode* aPackage3dInst, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
{
    /*
     * <!ELEMENT package3dinstance EMPTY>
     * <!ATTLIST package3dinstance
     *           package3d_urn %Urn;          #REQUIRED
     *           >
     */
    package3d_urn = parseRequiredAttribute<wxString>( aPackage3dInst, "package3d_urn" );

    AdvanceProgressPhase();
}


EDEVICE::EDEVICE( wxXmlNode* aDevice, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
{
    /*
     * <!ELEMENT device (connects?, package3dinstances?, technologies?)>
     * <!ATTLIST device
     *           name          %String;       ""
     *           package       %String;       #IMPLIED
     */
    name = parseRequiredAttribute<wxString>( aDevice, "name" );
    opt_wxString pack = parseOptionalAttribute<wxString>( aDevice, "package" );

    if( pack )
    {
        std::string p( pack->c_str() );
        ReplaceIllegalFileNameChars( p, '_' );
        package.Set( wxString::FromUTF8( p.c_str() ) );
    }

    for( wxXmlNode* child = aDevice->GetChildren(); child; child = child->GetNext() )
    {
        if( child->GetName() == "connects" )
        {
            for( wxXmlNode* connect = child->GetChildren(); connect; connect = connect->GetNext() )
            {
                if( connect->GetName() == "connect" )
                    connects.emplace_back( std::make_unique<ECONNECT>( connect, aIo ) );
            }

            AdvanceProgressPhase();
        }
        else if( child->GetName() == "packages3dinstances" )
        {
            for( wxXmlNode* package3dinst = child->GetChildren(); package3dinst;
                 package3dinst = package3dinst->GetNext() )
            {
                if( package3dinst->GetName() == "package3dinstance" )
                    package3dinstances.emplace_back( std::make_unique<EPACKAGE3DINST>( package3dinst, aIo ) );
            }

            AdvanceProgressPhase();
        }
        else if( child->GetName() == "technologies" )
        {
            for( wxXmlNode* technology = child->GetChildren(); technology;
                 technology = technology->GetNext() )
            {
                if( technology->GetName() == "technology" )
                    technologies.emplace_back( std::make_unique<ETECHNOLOGY>( technology, aIo ) );
            }

            AdvanceProgressPhase();
        }
    }

    AdvanceProgressPhase();
}


EDEVICE_SET::EDEVICE_SET( wxXmlNode* aDeviceSet, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
{
    /*
     * <!ELEMENT deviceset (description?, gates, devices, spice?)>
     * <!ATTLIST deviceset
     *           name          %String;       #REQUIRED
     *           urn              %Urn;       ""
     *           locally_modified %Bool;      "no"
     *           prefix        %String;       ""
     *           uservalue     %Bool;         "no"
     *           library_version  %Int;       ""
     *           library_locally_modified %Bool; "no"
     *           >
     *           <!-- library_version and library_locally_modified: Only in managed libraries
     *                inside boards or schematics -->
     */
    name = parseRequiredAttribute<wxString>( aDeviceSet, "name" );
    urn = parseOptionalAttribute<EURN>( aDeviceSet, "urn" );
    locally_modified = parseOptionalAttribute<bool>( aDeviceSet, "locally_modified" );
    prefix = parseOptionalAttribute<wxString>( aDeviceSet, "prefix" );
    uservalue = parseOptionalAttribute<bool>( aDeviceSet, "uservalue" );
    library_version = parseOptionalAttribute<int>( aDeviceSet, "library_version" );
    library_locally_modified =
        parseOptionalAttribute<bool>( aDeviceSet, "library_locally_modified" );

    for( wxXmlNode* child = aDeviceSet->GetChildren(); child; child = child->GetNext() )
    {
        if( child->GetName() == "description" )
        {
            description = std::make_optional<EDESCRIPTION>( child, aIo );
        }
        else if( child->GetName() == "gates" )
        {
            for( wxXmlNode* gate = child->GetChildren(); gate; gate = gate->GetNext() )
            {
                std::unique_ptr<EGATE> tmp = std::make_unique<EGATE>( gate, aIo );
                gates[tmp->name] = std::move( tmp );
            }

            AdvanceProgressPhase();
        }
        else if( child->GetName() == "devices" )
        {
            for( wxXmlNode* device = child->GetChildren(); device; device = device->GetNext() )
                devices.emplace_back( std::make_unique<EDEVICE>( device, aIo ) );

            AdvanceProgressPhase();
        }
        else if( child->GetName() == "spice" )
        {
            spice = std::make_optional<ESPICE>( child, aIo );
        }
    }

    AdvanceProgressPhase();
}


ECLASS::ECLASS( wxXmlNode* aClass, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
{
    /*
     * <!ELEMENT class (clearance)*>
     * <!ATTLIST class
     *           number        %Class;        #REQUIRED
     *           name          %String;       #REQUIRED
     *           width         %Dimension;    "0"
     *           drill         %Dimension;    "0"
     *           >
     */
    number = parseRequiredAttribute<wxString>( aClass, "number" );
    name   = parseRequiredAttribute<wxString>( aClass, "name" );
    width  = parseOptionalAttribute<ECOORD>( aClass, "width" );
    drill  = parseOptionalAttribute<ECOORD>( aClass, "drill" );

    for( wxXmlNode* child = aClass->GetChildren(); child; child = child->GetNext() )
    {
        if( child->GetName() == "clearance" )
        {
            wxString to = parseRequiredAttribute<wxString>( child, "class" );
            ECOORD value = parseRequiredAttribute<ECOORD>( child, "value" );

            clearanceMap[to] = value;

            AdvanceProgressPhase();
        }
    }

    AdvanceProgressPhase();
}


EPLAIN::EPLAIN( wxXmlNode* aPlain, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
{
    /*
     * <!ELEMENT plain (polygon | wire | text | dimension | circle | spline | rectangle |
     *                  frame | hole)*>
     */
    for( wxXmlNode* child = aPlain->GetChildren(); child; child = child->GetNext() )
    {
        if( child->GetName() == "polygon" )
            polygons.emplace_back( std::make_unique<EPOLYGON>( child, aIo ) );
        else if( child->GetName() == "wire" )
            wires.emplace_back( std::make_unique<EWIRE>( child, aIo ) );
        else if( child->GetName() == "text" )
            texts.emplace_back( std::make_unique<ETEXT>( child, aIo ) );
        else if( child->GetName() == "dimension" )
            dimensions.emplace_back( std::make_unique<EDIMENSION>( child, aIo ) );
        else if( child->GetName() == "circle" )
            circles.emplace_back( std::make_unique<ECIRCLE>( child, aIo ) );
        else if( child->GetName() == "spline" )
            splines.emplace_back( std::make_unique<ESPLINE>( child, aIo ) );
        else if( child->GetName() == "rectangle" )
            rectangles.emplace_back( std::make_unique<ERECT>( child, aIo ) );
        else if( child->GetName() == "frame" )
            frames.emplace_back( std::make_unique<EFRAME>( child, aIo ) );
        else if( child->GetName() == "hole" )
            holes.emplace_back( std::make_unique<EHOLE>( child, aIo ) );
    }

    AdvanceProgressPhase();
}


EMODULEINST::EMODULEINST( wxXmlNode* aModuleInst, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
{
    /*
     * <!ELEMENT moduleinst (attribute)*>
     * <!ATTLIST moduleinst
     *           name          %String;       #REQUIRED
     *           module        %String;       #REQUIRED
     *           modulevariant %String;       ""
     *           x             %Coord;        #REQUIRED
     *           y             %Coord;        #REQUIRED
     *           offset        %Int;          "0"
     *           smashed       %Bool;         "no"
     *           rot           %Rotation;     "R0"
     *           >
     *           <!-- rot: Only 0, 90, 180 or 270 -->
     */
    name = parseRequiredAttribute<wxString>( aModuleInst, "name" );
    moduleinst = parseRequiredAttribute<wxString>( aModuleInst, "module" );
    moduleVariant = parseOptionalAttribute<wxString>( aModuleInst, "modulevariant" );
    x = parseRequiredAttribute<ECOORD>( aModuleInst, "x" );
    y = parseRequiredAttribute<ECOORD>( aModuleInst, "y" );
    offset = parseOptionalAttribute<int>( aModuleInst, "offset" );
    smashed = parseOptionalAttribute<bool>( aModuleInst, "smashed" );
    rotation = parseOptionalAttribute<EROT>( aModuleInst, "rot" );

    AdvanceProgressPhase();
}


ESHEET::ESHEET( wxXmlNode* aSheet, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
{
    /*
     * <!ELEMENT sheet (description?, plain?, moduleinsts?, instances?, busses?, nets?)>
     */
    for( wxXmlNode* child = aSheet->GetChildren(); child; child = child->GetNext() )
    {
        if( child->GetName() == "description" )
        {
            description = std::make_optional<EDESCRIPTION>( child, aIo );
        }
        else if( child->GetName() == "plain" )
        {
            plain = std::make_unique<EPLAIN>( child, aIo );
        }
        else if( child->GetName() == "moduleinsts" )
        {
            for( wxXmlNode* moduleinst = child->GetChildren(); moduleinst;
                 moduleinst = moduleinst->GetNext() )
            {
                if( moduleinst->GetName() == "moduleinst" )
                {
                    std::unique_ptr<EMODULEINST> inst = std::make_unique<EMODULEINST>( moduleinst, aIo );
                    moduleinsts[ inst->name ] = std::move( inst );
                }
            }

            AdvanceProgressPhase();
        }
        else if( child->GetName() == "instances" )
        {
            for( wxXmlNode* instance = child->GetChildren(); instance; instance = instance->GetNext() )
            {
                if( instance->GetName() == "instance" )
                    instances.emplace_back( std::make_unique<EINSTANCE>( instance, aIo ) );
            }

            AdvanceProgressPhase();
        }
        else if( child->GetName() == "busses" )
        {
            for( wxXmlNode* bus = child->GetChildren(); bus; bus = bus->GetNext() )
            {
                if( bus->GetName() == "bus" )
                    busses.emplace_back( std::make_unique<EBUS>( bus, aIo ) );
            }

            AdvanceProgressPhase();
        }
        else if( child->GetName() == "nets" )
        {
            for( wxXmlNode* net = child->GetChildren(); net; net = net->GetNext() )
            {
                if( net->GetName() == "net" )
                    nets.emplace_back( std::make_unique<ENET>( net, aIo ) );
            }

            AdvanceProgressPhase();
        }
    }

    AdvanceProgressPhase();
}


ESCHEMATIC_GROUP::ESCHEMATIC_GROUP( wxXmlNode* aSchematicGroup, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
{
    /*
     * <!ELEMENT schematic_group (attribute*, description?)>
     * <!ATTLIST schematic_group
     *           name              ID             #REQUIRED
     *           selectable        %Bool;         #IMPLIED
     *           width             %Dimension;    #IMPLIED
     *           titleSize         %Dimension;    #IMPLIED
     *           titleFont         %TextFont;     #IMPLIED
     *           style             %WireStyle;    #IMPLIED
     *           showAnnotations   %Bool;         #IMPLIED
     *           layer             %Layer;        #IMPLIED
     *           grouprefs         IDREFS         #IMPLIED
     *           >
     */
    name = parseRequiredAttribute<wxString>( aSchematicGroup, "name" );
    selectable = parseOptionalAttribute<bool>( aSchematicGroup, "selectable" );
    width = parseOptionalAttribute<ECOORD>( aSchematicGroup, "width" );
    titleSize = parseOptionalAttribute<ECOORD>( aSchematicGroup, "titleSize" );
    titleFont = parseOptionalAttribute<wxString>( aSchematicGroup, "font" );
    wireStyle = parseOptionalAttribute<wxString>( aSchematicGroup, "style" );
    showAnnotations = parseOptionalAttribute<bool>( aSchematicGroup, "showAnnotations" );
    layer = parseOptionalAttribute<int>( aSchematicGroup, "layer" );
    grouprefs = parseOptionalAttribute<wxString>( aSchematicGroup, "grouprefs" );

    for( wxXmlNode* child = aSchematicGroup->GetChildren(); child; child = child->GetNext() )
    {
        if( child->GetName() == "description" )
        {
            description = std::make_optional<EDESCRIPTION>( child, aIo );
        }
        else if( child->GetName() == "attribute" )
        {
            attributes.emplace_back( std::make_unique<EATTR>( child, aIo ) );
        }
    }

    AdvanceProgressPhase();
}


EMODULE::EMODULE( wxXmlNode* aModule, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
{
    /*
     * <!ELEMENT module (description?, ports?, variantdefs?, groups?, parts?, sheets?)>
     * <!ATTLIST module
     *           name          %String;       #REQUIRED
     *           prefix        %String;       ""
     *           dx            %Coord;        #REQUIRED
     *           dy            %Coord;        #REQUIRED
     *           >
     */
    name = parseRequiredAttribute<wxString>( aModule, "name" );
    prefix = parseOptionalAttribute<wxString>( aModule, "prefix" );
    dx = parseRequiredAttribute<ECOORD>( aModule, "dx" );
    dy = parseRequiredAttribute<ECOORD>( aModule, "dy" );

    for( wxXmlNode* child = aModule->GetChildren(); child; child = child->GetNext() )
    {
        if( child->GetName() == "description" )
        {
            description = std::make_optional<EDESCRIPTION>( child, aIo );
        }
        else if( child->GetName() == "ports" )
        {
            for( wxXmlNode* port = child->GetChildren(); port; port = port->GetNext() )
            {
                if( port->GetName() == "port"  )
                {
                    std::unique_ptr<EPORT> tmp = std::make_unique<EPORT>( port, aIo );
                    ports[ tmp->name ] = std::move( tmp );
                }
            }

            AdvanceProgressPhase();
        }
        else if( child->GetName() == "variantdefs" )
        {
            for( wxXmlNode* variantdef = child->GetChildren(); variantdef;
                 variantdef = variantdef->GetNext() )
            {
                if( variantdef->GetName() == "variantdef"  )
                {
                    std::unique_ptr<EVARIANTDEF> tmp = std::make_unique<EVARIANTDEF>( variantdef,
                                                                                      aIo );
                    variantdefs[ tmp->name ] = std::move( tmp );
                }
            }

            AdvanceProgressPhase();
        }
        else if( child->GetName() == "groups" )
        {
            for( wxXmlNode* group = child->GetChildren(); group; group = group->GetNext() )
            {
                if( group->GetName() == "schematic_group"  )
                {
                    std::unique_ptr<ESCHEMATIC_GROUP> tmp =
                            std::make_unique<ESCHEMATIC_GROUP>( group, aIo );
                    groups[ tmp->name ] = std::move( tmp );
                }
            }

            AdvanceProgressPhase();
        }
        else if( child->GetName() == "parts" )
        {
            for( wxXmlNode* part = child->GetChildren(); part; part = part->GetNext() )
            {
                if( part->GetName() == "part"  )
                {
                    std::unique_ptr<EPART> tmp = std::make_unique<EPART>( part, aIo );
                    parts[ tmp->name ] = std::move( tmp );
                }
            }

            AdvanceProgressPhase();
        }
        else if( child->GetName() == "sheets" )
        {
            for( wxXmlNode* sheet = child->GetChildren(); sheet; sheet = sheet->GetNext() )
            {
                if( sheet->GetName() == "sheet"  )
                    sheets.emplace_back( std::make_unique<ESHEET>( sheet, aIo ) );
            }

            AdvanceProgressPhase();
        }
    }

    AdvanceProgressPhase();
}


EPORT::EPORT( wxXmlNode* aPort, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
{
    /*
     * <!ELEMENT port EMPTY>
     * <!ATTLIST port
     *           name          %String;       #REQUIRED
     *           side          %Int;          #REQUIRED
     *           coord         %Coord;        #REQUIRED
     *           direction     %PortDirection; "io"
     *           >
     */
    name = parseRequiredAttribute<wxString>( aPort, "name" );
    side = parseRequiredAttribute<wxString>( aPort, "side" );
    coord = parseRequiredAttribute<ECOORD>( aPort, "coord" );
    direction = parseOptionalAttribute<wxString>( aPort, "direction" );

    AdvanceProgressPhase();
}


EVARIANTDEF::EVARIANTDEF( wxXmlNode* aVariantDef, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
{
    /*
     * <!ELEMENT variantdef EMPTY>
     * <!ATTLIST variantdef
     *           name          %String;       #REQUIRED
     *           current       %Bool;         "no"
     *           >
     */
    name = parseRequiredAttribute<wxString>( aVariantDef, "name" );
    current = parseOptionalAttribute<bool>( aVariantDef, "current" );

    AdvanceProgressPhase();
}


ENOTE::ENOTE( wxXmlNode* aNote, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
{
    /*
     * <!ELEMENT note (#PCDATA)>
     * <!ATTLIST note
     *           version       %Real;         #REQUIRED
     *           severity      %Severity;     #REQUIRED
     *           >
     *           <!-- version: The EAGLE program version that introduced this compatibility note -->
     */
    version = parseRequiredAttribute<double>( aNote, "version" );
    severity = parseRequiredAttribute<wxString>( aNote, "severity" );

    note = aNote->GetNodeContent();

    AdvanceProgressPhase();
}


ECOMPATIBILITY::ECOMPATIBILITY( wxXmlNode* aCompatibility, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
{
    /*
     * <!ELEMENT compatibility (note)*>
     */
    for( wxXmlNode* child = aCompatibility->GetNext(); child; child = child->GetNext() )
    {
        if( child->GetName() == "note" )
            notes.emplace_back( std::make_unique<ENOTE>( child ) );
    }

    AdvanceProgressPhase();
}


ESETTING::ESETTING( wxXmlNode* aSetting, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
{
    /*
     * <!ELEMENT setting EMPTY>
     * <!ATTLIST setting
     *           alwaysvectorfont  %Bool;         #IMPLIED
     *           verticaltext      %VerticalText; "up"
     *           keepoldvectorfont %Bool;         "no"
     *           >
     */
    alwaysvectorfont = parseOptionalAttribute<bool>( aSetting, "alwaysvectorfont" );
    verticaltext = parseOptionalAttribute<wxString>( aSetting, "verticaltext" );
    keepoldvectorfont = parseOptionalAttribute<bool>( aSetting, "keepoldvectorfont" );

    AdvanceProgressPhase();
}


EGRID::EGRID( wxXmlNode* aGrid, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
{
    /*
     * <!ELEMENT grid EMPTY>
     * <!ATTLIST grid
     *           distance      %Real;         #IMPLIED
     *           unitdist      %GridUnit;     #IMPLIED
     *           unit          %GridUnit;     #IMPLIED
     *           style         %GridStyle;    "lines"
     *           multiple      %Int;          "1"
     *           display       %Bool;         "no"
     *           altdistance   %Real;         #IMPLIED
     *           altunitdist   %GridUnit;     #IMPLIED
     *           altunit       %GridUnit;     #IMPLIED
     *           >
     */
    distance = parseOptionalAttribute<double>( aGrid, "distance" );
    unitdist = parseOptionalAttribute<wxString>( aGrid, "unitdist" );
    unit = parseOptionalAttribute<wxString>( aGrid, "unit" );
    style = parseOptionalAttribute<wxString>( aGrid, "style" );
    multiple = parseOptionalAttribute<int>( aGrid, "multiple" );
    display = parseOptionalAttribute<bool>( aGrid, "display" );
    altdistance = parseOptionalAttribute<double>( aGrid, "altdistance" );
    altunitdist = parseOptionalAttribute<wxString>( aGrid, "altunitdist" );
    altunit = parseOptionalAttribute<wxString>( aGrid, "altunit" );

    AdvanceProgressPhase();
}


EFILTER::EFILTER( wxXmlNode* aFilter, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
{
    /*
     * <!ELEMENT filter EMPTY>
     * <!ATTLIST filter
     *           name          %String;       #REQUIRED
     *           expression    %String;       #REQUIRED
     *           >
     */
    name = parseRequiredAttribute<wxString>( aFilter, "name" );
    expression = parseRequiredAttribute<wxString>( aFilter, "expression" );

    AdvanceProgressPhase();
};


EPACKAGE::EPACKAGE( wxXmlNode* aPackage, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
{
    /*
     * <!ELEMENT package (description?, (polygon | wire | text | dimension | circle |
     *           rectangle | frame | hole | pad | smd)*)>
     * <!ATTLIST package
     *           name          %String;       #REQUIRED
     *           urn              %Urn;       ""
     *           locally_modified %Bool;      "no"
     *           library_version  %Int;       ""
     *           library_locally_modified %Bool; "no"
     *           >
     *           <!-- library_version and library_locally_modified: Only in managed libraries
     *                inside boards or schematics -->
     */
    name = parseRequiredAttribute<wxString>( aPackage, "name" );
    urn = parseOptionalAttribute<EURN>( aPackage, "urn" );
    locally_modified = parseOptionalAttribute<bool>( aPackage, "locally_modified" );
    library_version = parseOptionalAttribute<int>( aPackage, "library_version" );
    library_locally_modified = parseOptionalAttribute<bool>( aPackage, "library_locally_modified" );

    for( wxXmlNode* child = aPackage->GetChildren(); child; child = child->GetNext() )
    {
        if( child->GetName() == "description" )
        {
            description = std::make_optional<EDESCRIPTION>( child, aIo );
        }
        else if( child->GetName() == "polygon" )
        {
            polygons.emplace_back( std::make_unique<EPOLYGON>( child, aIo ) );
        }
        else if( child->GetName() == "wire" )
        {
            wires.emplace_back( std::make_unique<EWIRE>( child, aIo ) );
        }
        else if( child->GetName() == "text" )
        {
            texts.emplace_back( std::make_unique<ETEXT>( child, aIo ) );
        }
        else if( child->GetName() == "dimension" )
        {
            dimensions.emplace_back( std::make_unique<EDIMENSION>( child, aIo ) );
        }
        else if( child->GetName() == "circle" )
        {
            circles.emplace_back( std::make_unique<ECIRCLE>( child, aIo ) );
        }
        else if( child->GetName() == "rectangle" )
        {
            rectangles.emplace_back( std::make_unique<ERECT>( child, aIo ) );
        }
        else if( child->GetName() == "frame" )
        {
            frames.emplace_back( std::make_unique<EFRAME>( child, aIo ) );
        }
        else if( child->GetName() == "hole" )
        {
            holes.emplace_back( std::make_unique<EHOLE>( child, aIo ) );
        }
        else if( child->GetName() == "pad" )
        {
            thtpads.emplace_back( std::make_unique<EPAD>( child, aIo ) );
        }
        else if( child->GetName() == "smd" )
        {
            smdpads.emplace_back( std::make_unique<ESMD>( child, aIo ) );
        }
    }

    AdvanceProgressPhase();
}


EPACKAGEINSTANCE::EPACKAGEINSTANCE( wxXmlNode* aPackageInstance, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
{
    /*
     * <!ELEMENT packageinstance EMPTY>
     * <!ATTLIST packageinstance
     *           name          %String;       #REQUIRED
     *           >
     */
    name = parseRequiredAttribute<wxString>( aPackageInstance, "name" );

    AdvanceProgressPhase();
}


EPACKAGE3D::EPACKAGE3D( wxXmlNode* aPackage3d, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
{
    /*
     * <!ELEMENT package3d (description?, packageinstances?)>
     * <!ATTLIST package3d
     *           name          %String;       ""
     *           urn              %Urn;       #REQUIRED
     *           type   %Package3dType;       #REQUIRED
     *           library_version  %Int;       ""
     *           library_locally_modified %Bool; "no"
     *           >
     *           <!-- library_version and library_locally_modified: Only in managed libraries
     *                inside boards or schematics -->
     */
    name = parseRequiredAttribute<wxString>( aPackage3d, "name" );
    urn = parseRequiredAttribute<wxString>( aPackage3d, "urn" );
    type = parseRequiredAttribute<wxString>( aPackage3d, "type" );
    library_version = parseOptionalAttribute<int>( aPackage3d, "library_version" );
    library_locally_modified = parseOptionalAttribute<bool>( aPackage3d,
                                                             "library_locally_modified" );

    for( wxXmlNode* child = aPackage3d->GetChildren(); child; child = child->GetNext() )
    {
        if( child->GetName() == "description" )
        {
            description = std::make_optional<EDESCRIPTION>( child, aIo );
        }
        else if( child->GetName() == "packageinstances" )
        {
            for( wxXmlNode* instance = child->GetChildren(); instance;
                 instance = instance->GetNext() )
                packageinstances.emplace_back( std::make_unique<EPACKAGEINSTANCE>( instance,
                                                                                   aIo ) );

            AdvanceProgressPhase();
        }
    }

    AdvanceProgressPhase();
}


ESYMBOL::ESYMBOL( wxXmlNode* aSymbol, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
{
    /*
     * <!ELEMENT symbol (description?, (polygon | wire | text | dimension | pin | circle |
     *                   rectangle | frame)*)>
     * <!ATTLIST symbol
     *           name          %String;       #REQUIRED
     *           urn              %Urn;       ""
     *           locally_modified %Bool;      "no"
     *           library_version  %Int;       ""
     *           library_locally_modified %Bool; "no"
     *           >
     *           <!-- library_version and library_locally_modified: Only in managed libraries
     *                inside boards or schematics -->
     */

    name = parseRequiredAttribute<wxString>( aSymbol, "name" );
    urn = parseOptionalAttribute<EURN>( aSymbol, "urn" );
    locally_modified = parseOptionalAttribute<bool>( aSymbol, "locally_modified" );
    library_version = parseOptionalAttribute<int>( aSymbol, "library_version" );
    library_locally_modified = parseOptionalAttribute<bool>( aSymbol, "library_locally_modified" );

    for( wxXmlNode* child = aSymbol->GetChildren(); child; child = child->GetNext() )
    {
        if( child->GetName() == "description" )
        {
            description = std::make_optional<EDESCRIPTION>( child, aIo );
        }
        else if( child->GetName() == "polygon" )
        {
            polygons.emplace_back( std::make_unique<EPOLYGON>( child, aIo ) );
        }
        else if( child->GetName() == "wire" )
        {
            wires.emplace_back( std::make_unique<EWIRE>( child, aIo ) );
        }
        else if( child->GetName() == "text" )
        {
            texts.emplace_back( std::make_unique<ETEXT>( child, aIo ) );
        }
        else if( child->GetName() == "dimension" )
        {
            dimensions.emplace_back( std::make_unique<EDIMENSION>( child, aIo ) );
        }
        else if( child->GetName() == "pin" )
        {
            pins.emplace_back( std::make_unique<EPIN>( child, aIo ) );
        }
        else if( child->GetName() == "circle" )
        {
            circles.emplace_back( std::make_unique<ECIRCLE>( child, aIo ) );
        }
        else if( child->GetName() == "rectangle" )
        {
            rectangles.emplace_back( std::make_unique<ERECT>( child, aIo ) );
        }
        else if( child->GetName() == "frame" )
        {
            frames.emplace_back( std::make_unique<EFRAME>( child, aIo ) );
        }
    }

    AdvanceProgressPhase();
}


ELIBRARY::ELIBRARY( wxXmlNode* aLibrary, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
{
    /*
     * <!ELEMENT library (description?, packages?, packages3d?, symbols?, devicesets?)>
     * <!ATTLIST library
     *           name          %String;       #REQUIRED
     *           urn           %Urn;          ""
     *           >
     *           <!-- name: Only in libraries used inside boards or schematics -->
     *           <!-- urn: Only in online libraries used inside boards or schematics -->
     */

    // The name and urn attributes are only valid in schematic and board files.
    wxString parentNodeName;

    if( aLibrary->GetParent() )
        parentNodeName = aLibrary->GetParent()->GetName();

    if( parentNodeName == "libraries" )
    {
        name = parseRequiredAttribute<wxString>( aLibrary, "name" );
        urn = parseOptionalAttribute<EURN>( aLibrary, "urn" );
    }

    for( wxXmlNode* child = aLibrary->GetChildren(); child; child = child->GetNext() )
    {
        if( child->GetName() == "description" )
        {
            description = std::make_optional<EDESCRIPTION>( child, aIo );
        }
        else if( child->GetName() == "packages" )
        {
            for( wxXmlNode* package = child->GetChildren(); package; package = package->GetNext() )
            {
                if( package->GetName() == "package"  )
                {
                    std::unique_ptr<EPACKAGE> tmp = std::make_unique<EPACKAGE>( package, aIo );
                    packages[ tmp->name ] = std::move( tmp );
                }
            }

            AdvanceProgressPhase();
        }
        else if( child->GetName() == "packages3d" )
        {
            for( wxXmlNode* package3d = child->GetChildren(); package3d;
                 package3d = package3d->GetNext() )
            {
                if( package3d->GetName() == "package3d"  )
                {
                    std::unique_ptr<EPACKAGE3D> tmp = std::make_unique<EPACKAGE3D>( package3d,
                                                                                    aIo );
                    packages3d[ tmp->name ] = std::move( tmp );
                }
            }

            AdvanceProgressPhase();
        }
        else if( child->GetName() == "symbols" )
        {
            for( wxXmlNode* symbol = child->GetChildren(); symbol; symbol = symbol->GetNext() )
            {
                if( symbol->GetName() == "symbol"  )
                {
                    std::unique_ptr<ESYMBOL> tmp = std::make_unique<ESYMBOL>( symbol, aIo );
                    symbols[ tmp->name ] = std::move( tmp );
                }
            }

            AdvanceProgressPhase();
        }
        else if( child->GetName() == "devicesets" )
        {
            for( wxXmlNode* deviceset = child->GetChildren(); deviceset;
                 deviceset = deviceset->GetNext() )
            {
                if( deviceset->GetName() == "deviceset"  )
                {
                    std::unique_ptr<EDEVICE_SET> tmp = std::make_unique<EDEVICE_SET>( deviceset,
                                                                                      aIo );
                    devicesets[ tmp->name ] = std::move( tmp );
                }
            }

            AdvanceProgressPhase();
        }
    }

    AdvanceProgressPhase();
}


wxString ELIBRARY::GetName() const
{
    wxString libName = name;

    // Use the name when no library urn exists.
    if( !urn )
        return libName;

    // Suffix the library name with the urn library identifier.  Eagle schematics can have
    // mulitple libraries with the same name.  The urn library identifier is used to prevent
    // library name clashes.
    if( urn->IsValid() )
        libName += wxS( "_" ) + urn->assetId;

    return libName;
}


EAPPROVED::EAPPROVED( wxXmlNode* aApproved, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
{
    /*
     * <!ELEMENT approved EMPTY>
     * <!ATTLIST approved
     *           hash          %String;       #REQUIRED
     *           >
     */
    hash = parseRequiredAttribute<wxString>( aApproved, "hash" );

    AdvanceProgressPhase();
}


ESCHEMATIC::ESCHEMATIC( wxXmlNode* aSchematic, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
{
    /*
     * <!ELEMENT schematic (description?, libraries?, attributes?, variantdefs?, classes?,
     *           modules?, groups?, parts?, sheets?, errors?)>
     * <!ATTLIST schematic
     *           xreflabel     %String;       #IMPLIED
     *           xrefpart      %String;       #IMPLIED
     *           >
     */
    xreflabel = parseOptionalAttribute<wxString>( aSchematic, "xreflabel" );
    xrefpart = parseOptionalAttribute<wxString>( aSchematic, "xrefpart" );

    for( wxXmlNode* child = aSchematic->GetChildren(); child; child = child->GetNext() )
    {
        if( child->GetName() == "description" )
        {
            description = std::make_optional<EDESCRIPTION>( child, aIo );
        }
        else if( child->GetName() == "libraries" )
        {
            for( wxXmlNode* library = child->GetChildren(); library; library = library->GetNext() )
            {
                if( library->GetName() == "library"  )
                {
                    std::unique_ptr<ELIBRARY> tmp = std::make_unique<ELIBRARY>( library, aIo );

                    wxString libName = tmp->GetName();

                    // Prevent duplicate library names.  This should only happen if the Eagle
                    // file has an invalid format.
                    if( libraries.find( libName ) != libraries.end() )
                    {
                        wxString uniqueName;
                        std::set<wxString> usedNames;

                        for( const auto& [setName, setLibrary] : libraries )
                            usedNames.emplace( setName );

                        if( usedNames.find( libName ) != usedNames.end() )
                        {
                            int i = 1;

                            do
                            {
                                uniqueName.Format( wxS( "%s_%d" ), libName, i );
                                i += 1;
                            } while( usedNames.find( uniqueName ) != usedNames.end() );
                        }

                        libName = uniqueName;
                    }

                    libraries[ libName ] = std::move( tmp );
                }
            }

            AdvanceProgressPhase();
        }
        else if( child->GetName() == "attributes" )
        {
            for( wxXmlNode* attribute = child->GetChildren(); attribute;
                 attribute = attribute->GetNext() )
            {
                if( attribute->GetName() == "attribute"  )
                {
                    std::unique_ptr<EATTR> tmp = std::make_unique<EATTR>( attribute, aIo );
                    attributes[ tmp->name ] = std::move( tmp );
                }
            }

            AdvanceProgressPhase();
        }
        else if( child->GetName() == "variantdefs" )
        {
            for( wxXmlNode* variantdef = child->GetChildren(); variantdef;
                 variantdef = variantdef->GetNext() )
            {
                if( variantdef->GetName() == "variantdef"  )
                {
                    std::unique_ptr<EVARIANTDEF> tmp = std::make_unique<EVARIANTDEF>( variantdef,
                                                                                      aIo );
                    variantdefs[ tmp->name ] = std::move( tmp );
                }
            }

            AdvanceProgressPhase();
        }
        else if( child->GetName() == "classes" )
        {
            for( wxXmlNode* eclass = child->GetChildren(); eclass; eclass = eclass->GetNext() )
            {
                if( eclass->GetName() == "class"  )
                {
                    std::unique_ptr<ECLASS> tmp = std::make_unique<ECLASS>( eclass, aIo );
                    classes[ tmp->number ] = std::move( tmp );
                }
            }

            AdvanceProgressPhase();
        }
        else if( child->GetName() == "modules" )
        {
            for( wxXmlNode* mod = child->GetChildren(); mod; mod = mod->GetNext() )
            {
                if( mod->GetName() == "module"  )
                {
                    std::unique_ptr<EMODULE> tmp = std::make_unique<EMODULE>( mod, aIo );
                    modules[ tmp->name ] = std::move( tmp );
                }
            }

            AdvanceProgressPhase();
        }
        else if( child->GetName() == "groups" )
        {
            for( wxXmlNode* group = child->GetChildren(); group; group = group->GetNext() )
            {
                if( group->GetName() == "schematic_group"  )
                {
                    std::unique_ptr<ESCHEMATIC_GROUP> tmp =
                            std::make_unique<ESCHEMATIC_GROUP>( group, aIo );
                    groups[ tmp->name ] = std::move( tmp );
                }
            }

            AdvanceProgressPhase();
        }
        else if( child->GetName() == "parts" )
        {
            for( wxXmlNode* part = child->GetChildren(); part; part = part->GetNext() )
            {
                if( part->GetName() == "part"  )
                {
                    std::unique_ptr<EPART> tmp = std::make_unique<EPART>( part, aIo );
                    parts[ tmp->name ] = std::move( tmp );
                }
            }

            AdvanceProgressPhase();
        }
        else if( child->GetName() == "sheets" )
        {
            for( wxXmlNode* sheet = child->GetChildren(); sheet; sheet = sheet->GetNext() )
            {
                if( sheet->GetName() == "sheet"  )
                    sheets.emplace_back( std::make_unique<ESHEET>( sheet, aIo ) );
            }

            AdvanceProgressPhase();
        }
        else if( child->GetName() == "errors" )
        {
            for( wxXmlNode* error = child->GetChildren(); error; error = error->GetNext() )
            {
                if( error->GetName() == "approved"  )
                    errors.emplace_back( std::make_unique<EAPPROVED>( error, aIo ) );
            }

            AdvanceProgressPhase();
        }
    }

    AdvanceProgressPhase();
}


EDRAWING::EDRAWING( wxXmlNode* aDrawing, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
{
    /*
     * <!ELEMENT drawing (settings?, grid?, filters?, layers, (library | schematic | board))>
     */
    for( wxXmlNode* child = aDrawing->GetChildren(); child; child = child->GetNext() )
    {
        if( child->GetName() == "settings" )
        {
            for( wxXmlNode* setting = child->GetChildren(); setting; setting = setting->GetNext() )
                settings.emplace_back( std::make_unique<ESETTING>( setting, aIo ) );

            AdvanceProgressPhase();
        }
        else if( child->GetName() == "grid" )
        {
            grid = std::make_optional<EGRID>( child, aIo );
        }
        else if( child->GetName() == "filters" )
        {
            for( wxXmlNode* filter = child->GetChildren(); filter; filter = filter->GetNext() )
            {
                if( filter->GetName() == "filter" )
                    filters.emplace_back( std::make_unique<EFILTER>( filter, aIo ) );
            }

            AdvanceProgressPhase();
        }
        else if( child->GetName() == "layers" )
        {
            for( wxXmlNode* layer = child->GetChildren(); layer; layer = layer->GetNext() )
            {
                if( layer->GetName() == "layer" )
                    layers.emplace_back( std::make_unique<ELAYER>( layer, aIo ) );
            }

            AdvanceProgressPhase();
        }
        else if( child->GetName() == "schematic" )
        {
            schematic = std::make_optional<ESCHEMATIC>( child, aIo );
        }
        else if( child->GetName() == "library" )
        {
            library = std::make_optional<ELIBRARY>( child, aIo );
        }
    }

    // std::optional<std::unique_ptr<EBOARD>> board;

    AdvanceProgressPhase();
}


EAGLE_DOC::EAGLE_DOC( wxXmlNode* aEagleDoc, IO_BASE* aIo ) :
    EAGLE_BASE( aIo )
{
    /*
     * <!ELEMENT eagle (compatibility?, drawing, compatibility?)>
     * <!ATTLIST eagle
     *           version       %Real;         #REQUIRED
     *           >
     *           <!-- version: The EAGLE program version that generated this file, in the
     *                form V.RR -->
     */

    version = parseRequiredAttribute<wxString>( aEagleDoc, "version" );

    for( wxXmlNode* child = aEagleDoc->GetChildren(); child; child = child->GetNext() )
    {
        if( child->GetName() == "compitibility" )
            compatibility = std::make_optional<ECOMPATIBILITY>( child, aIo );
        else if( child->GetName() == "drawing" )
            drawing = std::make_unique<EDRAWING>( child, aIo );
    }

    AdvanceProgressPhase();
}
