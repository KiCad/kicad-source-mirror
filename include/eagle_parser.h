/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012-2018 KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2017 CERN
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


#ifndef _EAGLE_PARSER_H_
#define _EAGLE_PARSER_H_

#include <cerrno>
#include <unordered_map>

#include <wx/xml/xml.h>
#include <wx/string.h>
#include <wx/filename.h>

#include <layers_id_colors_and_visibility.h>
#include <convert_to_biu.h>
#include <macros.h>
#include <trigo.h>
#include <kicad_string.h>
#include <common.h>

class MODULE;
struct EINSTANCE;
struct EPART;
struct ETEXT;

typedef std::unordered_map<wxString, wxXmlNode*> NODE_MAP;
typedef std::map<wxString, MODULE*> MODULE_MAP;
typedef std::map<wxString, EINSTANCE*> EINSTANCE_MAP;
typedef std::map<wxString, std::unique_ptr<EPART>> EPART_MAP;

///> Translates Eagle special characters to their counterparts in KiCad.
wxString escapeName( const wxString& aNetName );

static inline wxXmlNode* getChildrenNodes( NODE_MAP& aMap, const wxString& aName )
{
    auto it = aMap.find( aName );
    return it == aMap.end() ? nullptr : it->second->GetChildren();
}


/**
 * Class XML_PARSER_ERROR
 * implements a simple wrapper around runtime_error to isolate the errors thrown by the
 * Eagle XML parser.
 */
struct XML_PARSER_ERROR : std::runtime_error
{
    /**
     * Constructor XML_PARSER_ERROR
     * build an XML error by just calling its parent class constructor, std::runtime_error, with
     * the passed message.
     * @param aMessage is an explanatory error message.
     */
    XML_PARSER_ERROR( const wxString& aMessage ) noexcept :
        std::runtime_error( "XML parser failed - " + aMessage.ToStdString() )
    {}
};


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
    std::vector<TRIPLET> p;

public:
    void push( const char* aPathSegment, const char* aAttribute="" )
    {
        p.emplace_back( aPathSegment, aAttribute );
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
    wxString Contents()
    {
        typedef std::vector<TRIPLET>::const_iterator CITER_TRIPLET;

        wxString ret;

        for( CITER_TRIPLET it = p.begin();  it != p.end();  ++it )
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
 * Function Convert
 * converts a wxString to a generic type T.
 * @param  aValue is a wxString containing the value that will be converted to type T.
 * @throw XML_PARSER_ERROR - an exception is thrown if the parsing fails or if the conversion to
 *        type T is unknown.
 */
template<typename T>
T Convert( const wxString& aValue )
{
    throw XML_PARSER_ERROR( "Conversion failed. Unknown type." );
}

template <>
wxString Convert<wxString>( const wxString& aValue );

/**
 * Class OPTIONAL_XML_ATTRIBUTE
 * models an optional XML attribute.
 * This was implemented as an alternative to OPT. This class should be replaced with a
 * simple typedef per type using std::optional when C++17 is published.
 */
template <typename T>
class OPTIONAL_XML_ATTRIBUTE
{
private:
    /// A boolean indicating if the data is present or not.
    bool m_isAvailable;

    /// The actual data if m_isAvailable is true; otherwise, garbage.
    T m_data;

public:
    /**
     * Constructor OPTIONAL_XML_ATTRIBUTE
     * construct a default OPTIONAL_XML_ATTRIBUTE, whose data is not available.
     */
    OPTIONAL_XML_ATTRIBUTE() :
        m_isAvailable( false ),
        m_data( T() )
    {}

    /**
     * Constructor OPTIONAL_XML_ATTRIBUTE
     * @param  aData is a wxString containing the value that should be converted to type T. If
     *               aData is empty, the attribute is understood as unavailable; otherwise, the
     *               conversion to T is tried.
     */
    OPTIONAL_XML_ATTRIBUTE( const wxString& aData )
    {
        m_data = T();
        m_isAvailable = !aData.IsEmpty();

        if( m_isAvailable )
            Set( aData );
    }

    /**
     * Constructor OPTIONAL_XML_ATTRIBUTE
     * @param  aData is the value of the XML attribute. If this constructor is called, the
     *               attribute is available.
     */
    template<typename V = T>
    OPTIONAL_XML_ATTRIBUTE( T aData ) :
        m_isAvailable( true ),
        m_data( aData )
    {}

    /**
     * Operator bool
     * @return bool - the availability of the attribute.
     */
    operator bool() const
    {
        return m_isAvailable;
    }

    /**
     * Assignment operator
     * to a string (optionally) containing the data.
     * @param aData is a wxString that should be converted to T. If the string is empty, the
     *              attribute is set to unavailable.
     */
    OPTIONAL_XML_ATTRIBUTE<T>& operator =( const wxString& aData )
    {
        m_isAvailable = !aData.IsEmpty();

        if( m_isAvailable )
            Set( aData );

        return *this;
    }

    /**
     * Assignment operator
     * to an object of the base type containing the data.
     * @param aData is the actual value of the attribute. Calling this assignment, the attribute
     *              is automatically made available.
     */
    OPTIONAL_XML_ATTRIBUTE<T>& operator =( T aData )
    {
        m_data = aData;
        m_isAvailable = true;

        return *this;
    }

    /**
     * Equal operator
     * to an object of the base type.
     * @param aOther is the object of the base type that should be compared with this one.
     */
    bool operator ==( const T& aOther ) const
    {
        return m_isAvailable && ( aOther == m_data );
    }

    /**
     * Function Set
     * tries to convert a string to the base type.
     * @param aString is the string that will be converted to the base type.
     */
    void Set( const wxString& aString )
    {
        m_data = Convert<T>( aString );
        m_isAvailable = !aString.IsEmpty();
    }

    /**
     * Function Get
     * returns a reference to the value of the attribute assuming it is available.
     * @return T& - the value of the attribute.
     */
    T& Get()
    {
        assert( m_isAvailable );
        return m_data;
    }

    /**
     * Function CGet
     * returns a constant reference to the value of the attribute assuming it is available.
     * @return const T& - the value of the attribute.
     */
    const T& CGet() const
    {
        assert( m_isAvailable );
        return m_data;
    }

    /**
     * Operator *
     * returns a reference to the value of the attribute assuming it is available.
     * @return T& - the value of the attribute.
     */
    T& operator*()
    {
        return Get();
    }

    /**
     * Operator *
     * returns a constant reference to the value of the attribute assuming it is available.
     * @return const T& - the value of the attribute.
     */
    const T& operator*() const
    {
        return CGet();
    }

    /**
     * Operator ->
     * returns a pointer to the value of the attribute assuming it is available.
     * @return T* - the value of the attribute.
     */
    T* operator->()
    {
        return &Get();
    }

    /**
     * Operator ->
     * returns a constant pointer to the value of the attribute assuming it is available.
     * @return const T* - the value of the attribute.
     */
    const T* operator->() const
    {
        return &CGet();
    }
};


/**
 * Function MapChildren
 * provides an easy access to the children of an XML node via their names.
 * @param  currentNode is a pointer to a wxXmlNode, whose children will be mapped.
 * @return NODE_MAP - a map linking the name of each children to the children itself (via a
 *                  wxXmlNode*)
 */
NODE_MAP MapChildren( wxXmlNode* aCurrentNode );

///> Make a unique time stamp
timestamp_t EagleTimeStamp( wxXmlNode* aTree );

///> Computes module timestamp basing on its name, value and unit
timestamp_t EagleModuleTstamp( const wxString& aName, const wxString& aValue, int aUnit );

///> Convert an Eagle curve end to a KiCad center for S_ARC
wxPoint ConvertArcCenter( const wxPoint& aStart, const wxPoint& aEnd, double aAngle );

// Pre-declare for typedefs
struct EROT;
struct ECOORD;
typedef OPTIONAL_XML_ATTRIBUTE<wxString> opt_wxString;
typedef OPTIONAL_XML_ATTRIBUTE<int>     opt_int;
typedef OPTIONAL_XML_ATTRIBUTE<double>  opt_double;
typedef OPTIONAL_XML_ATTRIBUTE<bool>    opt_bool;
typedef OPTIONAL_XML_ATTRIBUTE<EROT>    opt_erot;
typedef OPTIONAL_XML_ATTRIBUTE<ECOORD>  opt_ecoord;


// All of the 'E'STRUCTS below merely hold Eagle XML information verbatim, in binary.
// For maintenance and troubleshooting purposes, it was thought that we'd need to
// separate the conversion process into distinct steps. There is no intent to have KiCad
// forms of information in these 'E'STRUCTS.  They are only binary forms
// of the Eagle information in the corresponding Eagle XML nodes.

// Eagle coordinates
struct ECOORD
{
    enum EAGLE_UNIT
    {
        EU_NM,     ///< nanometers
        EU_MM,     ///< millimeters
        EU_INCH,   ///< inches
        EU_MIL,    ///< mils/thous
    };

    ///> Value expressed in nanometers
    long long int value;

    ///> Unit used for the value field
    static constexpr EAGLE_UNIT ECOORD_UNIT = EU_NM;

    ECOORD()
        : value( 0 )
    {
    }

    ECOORD( int aValue, enum EAGLE_UNIT aUnit )
        : value( ConvertToNm( aValue, aUnit ) )
    {
    }

    ECOORD( const wxString& aValue, enum EAGLE_UNIT aUnit );

    int ToMils() const
    {
        return value / 25400;
    }

    int ToNanoMeters() const
    {
        return value;
    }

    float ToMm() const
    {
        return value / 1000000.0;
    }

    int ToSchUnits() const { return ToMils(); }
    int ToPcbUnits() const { return ToNanoMeters(); }

    ECOORD operator+( const ECOORD& aOther ) const
    {
        return ECOORD( value + aOther.value, ECOORD_UNIT );
    }

    ECOORD operator-( const ECOORD& aOther ) const
    {
        return ECOORD( value - aOther.value, ECOORD_UNIT );
    }

    bool operator==( const ECOORD& aOther ) const
    {
        return value == aOther.value;
    }

    ///> Converts a size expressed in a certain unit to nanometers.
    static long long int ConvertToNm( int aValue, enum EAGLE_UNIT aUnit );
};


/// Eagle net
struct ENET
{
    int     netcode;
    wxString netname;

    ENET( int aNetCode, const wxString& aNetName ) :
        netcode( aNetCode ),
        netname( aNetName )
    {}

    ENET() :
        netcode( 0 )
    {}
};


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


/// Eagle wire
struct EWIRE
{
    ECOORD     x1;
    ECOORD     y1;
    ECOORD     x2;
    ECOORD     y2;
    ECOORD     width;
    LAYER_NUM  layer;

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

    EWIRE( wxXmlNode* aWire );
};


/// Eagle Junction
struct EJUNCTION
{
    ECOORD     x;
    ECOORD     y;

    EJUNCTION( wxXmlNode* aJunction);
};


/// Eagle label
struct ELABEL
{
    ECOORD     x;
    ECOORD     y;
    ECOORD     size;
    LAYER_NUM  layer;
    opt_erot rot;
    opt_wxString xref;
    wxString netname;

    ELABEL( wxXmlNode* aLabel, const wxString& aNetName );
};


/// Eagle via
struct EVIA
{
    ECOORD     x;
    ECOORD     y;
    int        layer_front_most;   /// < extent
    int        layer_back_most;    /// < inclusive
    ECOORD     drill;
    opt_ecoord diam;
    opt_wxString shape;

    EVIA( wxXmlNode* aVia );
};


/// Eagle circle
struct ECIRCLE
{
    ECOORD    x;
    ECOORD    y;
    ECOORD    radius;
    ECOORD    width;
    LAYER_NUM layer;

    ECIRCLE( wxXmlNode* aCircle );
};


/// Eagle XML rectangle in binary
struct ERECT
{
    ECOORD   x1;
    ECOORD   y1;
    ECOORD   x2;
    ECOORD   y2;
    int      layer;
    opt_erot rot;

    ERECT( wxXmlNode* aRect );
};


/**
 * Class EATTR
 * parses an Eagle "attribute" XML element.  Note that an attribute element
 * is different than an XML element attribute.  The attribute element is a
 * full XML node in and of itself, and has attributes of its own.  Blame Eagle.
 */
struct EATTR
{
    wxString   name;
    opt_wxString value;
    opt_ecoord x;
    opt_ecoord y;
    opt_ecoord size;
    opt_int    layer;
    opt_double ratio;
    opt_erot   rot;

    enum {  // for 'display'
        Off,
        VALUE,
        NAME,
        BOTH,
    };
    opt_int     display;
    opt_int     align;

    EATTR( wxXmlNode* aTree );
    EATTR() {}
};


/// Eagle dimension element
struct EDIMENSION
{
    ECOORD x1;
    ECOORD y1;
    ECOORD x2;
    ECOORD y2;
    ECOORD x3;
    ECOORD y3;
    int    layer;

    opt_wxString dimensionType;

    EDIMENSION( wxXmlNode* aDimension );
};


/// Eagle text element
struct ETEXT
{
    wxString   text;
    ECOORD     x;
    ECOORD     y;
    ECOORD     size;
    int        layer;
    opt_wxString font;
    opt_double ratio;
    opt_erot   rot;

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

    opt_int align;

    ETEXT( wxXmlNode* aText );

    /// Calculate text size based on font type and size
    wxSize ConvertSize() const;
};


/// Structure holding common properties for through-hole and SMD pads
struct EPAD_COMMON
{
    wxString   name;
    ECOORD     x, y;
    opt_erot   rot;
    opt_bool   stop;
    opt_bool   thermals;

    EPAD_COMMON( wxXmlNode* aPad );
};


/// Eagle thru hole pad
struct EPAD : public EPAD_COMMON
{
    ECOORD     drill;
    opt_ecoord diameter;

    // for shape: (square | round | octagon | long | offset)
    enum {
        UNDEF = -1,
        SQUARE,
        ROUND,
        OCTAGON,
        LONG,
        OFFSET,
    };
    opt_int  shape;
    opt_bool first;

    EPAD( wxXmlNode* aPad );
};


/// Eagle SMD pad
struct ESMD : public EPAD_COMMON
{
    ECOORD   dx;
    ECOORD   dy;
    int      layer;
    opt_int  roundness;
    opt_bool cream;

    ESMD( wxXmlNode* aSMD );
};


/// Eagle pin element
struct EPIN
{
    wxString name;
    ECOORD   x;
    ECOORD   y;

    opt_wxString visible;
    opt_wxString length;
    opt_wxString direction;
    opt_wxString function;
    opt_int swaplevel;
    opt_erot rot;

    EPIN( wxXmlNode* aPin );
};


/// Eagle vertex
struct EVERTEX
{
    ECOORD      x;
    ECOORD      y;
    opt_double  curve;      ///< range is -359.9..359.9

    EVERTEX( wxXmlNode* aVertex );
};


/// Eagle polygon, without vertices which are parsed as needed
struct EPOLYGON
{
    ECOORD     width;
    int        layer;
    opt_ecoord spacing;

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
    int        pour;
    opt_ecoord isolate;
    opt_bool   orphans;
    opt_bool   thermals;
    opt_int    rank;

    EPOLYGON( wxXmlNode* aPolygon );
};


/// Eagle hole element
struct EHOLE
{
    ECOORD x;
    ECOORD y;
    ECOORD drill;

    EHOLE( wxXmlNode* aHole );
};


/// Eagle element element
struct EELEMENT
{
    wxString name;
    wxString library;
    wxString package;
    wxString value;
    ECOORD   x;
    ECOORD   y;
    opt_bool locked;
    opt_bool smashed;
    opt_erot rot;

    EELEMENT( wxXmlNode* aElement );
};


struct ELAYER
{
    int      number;
    wxString name;
    int      color;
    int      fill;
    opt_bool visible;
    opt_bool active;

    ELAYER( wxXmlNode* aLayer );
};


struct EAGLE_LAYER
{
    enum
    {
        TOP         = 1,
        ROUTE2      = 2,
        ROUTE3      = 3,
        ROUTE4      = 4,
        ROUTE5      = 5,
        ROUTE6      = 6,
        ROUTE7      = 7,
        ROUTE8      = 8,
        ROUTE9      = 9,
        ROUTE10     = 10,
        ROUTE11     = 11,
        ROUTE12     = 12,
        ROUTE13     = 13,
        ROUTE14     = 14,
        ROUTE15     = 15,
        BOTTOM      = 16,
        PADS        = 17,
        VIAS        = 18,
        UNROUTED    = 19,
        DIMENSION   = 20,
        TPLACE      = 21,
        BPLACE      = 22,
        TORIGINS    = 23,
        BORIGINS    = 24,
        TNAMES      = 25,
        BNAMES      = 26,
        TVALUES     = 27,
        BVALUES     = 28,
        TSTOP       = 29,
        BSTOP       = 30,
        TCREAM      = 31,
        BCREAM      = 32,
        TFINISH     = 33,
        BFINISH     = 34,
        TGLUE       = 35,
        BGLUE       = 36,
        TTEST       = 37,
        BTEST       = 38,
        TKEEPOUT    = 39,
        BKEEPOUT    = 40,
        TRESTRICT   = 41,
        BRESTRICT   = 42,
        VRESTRICT   = 43,
        DRILLS      = 44,
        HOLES       = 45,
        MILLING     = 46,
        MEASURES    = 47,
        DOCUMENT    = 48,
        REFERENCELC = 49,
        REFERENCELS = 50,
        TDOCU       = 51,
        BDOCU       = 52,
        NETS        = 91,
        BUSSES      = 92,
        PINS        = 93,
        SYMBOLS     = 94,
        NAMES       = 95,
        VALUES      = 96,
        INFO        = 97,
        GUIDE       = 98,
        USERLAYER1  = 160,
        USERLAYER2  = 161
    };
};


struct EPART
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

    wxString name;
    wxString library;
    wxString deviceset;
    wxString device;
    opt_wxString technology;
    opt_wxString value;
    std::map<std::string,std::string> attribute;
    std::map<std::string,std::string> variant;

    EPART( wxXmlNode* aPart );
};


struct EINSTANCE
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

    wxString part;
    wxString gate;
    ECOORD  x;
    ECOORD  y;
    opt_bool    smashed;
    opt_erot    rot;

    EINSTANCE( wxXmlNode* aInstance );
};


struct EGATE
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

    wxString name;
    wxString symbol;

    ECOORD  x;
    ECOORD  y;

    opt_int addlevel;
    opt_int swaplevel;

    enum
    {
        MUST,
        CAN,
        NEXT,
        REQUEST,
        ALWAYS
    };

    EGATE( wxXmlNode* aGate );
};


struct ECONNECT
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
    wxString gate;
    wxString pin;
    wxString pad;
    //int contactroute; // TODO

    ECONNECT( wxXmlNode* aConnect );
};


struct EDEVICE
{
    /*
    <!ELEMENT device (connects?, technologies?)>
    <!ATTLIST device
              name          %String;       ""
              package       %String;       #IMPLIED
              >
*/
    wxString    name;
    opt_wxString package;

    std::vector<ECONNECT> connects;

    EDEVICE( wxXmlNode* aDevice );
};


struct EDEVICE_SET
{
    /*
    <!ELEMENT deviceset (description?, gates, devices)>
    <!ATTLIST deviceset
              name          %String;       #REQUIRED
              prefix        %String;       ""
              uservalue     %Bool;         "no"
              >
    */

    wxString name;
    opt_wxString prefix;
    opt_bool uservalue;
    //std::vector<EDEVICE> devices;
    //std::vector<EGATE> gates;


    EDEVICE_SET( wxXmlNode* aDeviceSet );
};


#endif // _EAGLE_PARSER_H_
