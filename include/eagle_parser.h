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


#ifndef _EAGLE_PARSER_H_
#define _EAGLE_PARSER_H_

#include <errno.h>
#include <unordered_map>

#include <wx/xml/xml.h>
#include <wx/string.h>
#include <wx/filename.h>

#include <layers_id_colors_and_visibility.h>
#include <convert_to_biu.h>
#include <macros.h>
#include <trigo.h>
#include <kicad_string.h>

using std::string;

class MODULE;

typedef std::unordered_map< string,  wxXmlNode* > NODE_MAP;
typedef std::map< string, MODULE* > MODULE_MAP;


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
    XML_PARSER_ERROR( const string& aMessage ) noexcept :
        std::runtime_error( "XML parser failed - " + aMessage )
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
        typedef std::vector<TRIPLET>::const_iterator CITER_TRIPLET;

        string ret;

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


/**
 * Class OPTIONAL_XML_ATTRIBUTE
 * models an optional XML attribute.
 * This was implemented as an alternative to boost::optional. This class should be replaced with a
 * simple typedef per type using std::optional when C++17 is published.
 */
template <class T>
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
	   m_isAvailable( false )
    {}

    /**
     * Constructor OPTIONAL_XML_ATTRIBUTE
     * @param  aData is a wxString containing the value that should be converted to type T. If
     *               aData is empty, the attribute is understood as unavailable; otherwise, the
     *               conversion to T is tried.
     */
    OPTIONAL_XML_ATTRIBUTE( const wxString& aData )
    {
        m_isAvailable = !aData.IsEmpty();

        if( m_isAvailable )
            Set( aData );
    }

    /**
     * Constructor OPTIONAL_XML_ATTRIBUTE
     * @param  aData is the value of the XML attribute. If this constructor is called, the
     *               attribute is available.
     */
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

// Pre-declare for typedefs
struct EROT;

typedef OPTIONAL_XML_ATTRIBUTE<string>  opt_string;
typedef OPTIONAL_XML_ATTRIBUTE<int>     opt_int;
typedef OPTIONAL_XML_ATTRIBUTE<double>  opt_double;
typedef OPTIONAL_XML_ATTRIBUTE<bool>    opt_bool;
typedef OPTIONAL_XML_ATTRIBUTE<EROT>    opt_erot;


// All of the 'E'STRUCTS below merely hold Eagle XML information verbatim, in binary.
// For maintenance and troubleshooting purposes, it was thought that we'd need to
// separate the conversion process into distinct steps. There is no intent to have KiCad
// forms of information in these 'E'STRUCTS.  They are only binary forms
// of the Eagle information in the corresponding Eagle XML nodes.


/// Eagle net
struct ENET
{
    int         netcode;
    std::string netname;

    ENET( int aNetCode, const std::string& aNetName ) :
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
    double     x1;
    double     y1;
    double     x2;
    double     y2;
    double     width;
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


/// Eagle via
struct EVIA
{
    double     x;
    double     y;
    int        layer_front_most;   /// < extent
    int        layer_back_most;    /// < inclusive
    double     drill;
    opt_double diam;
    opt_string shape;

    EVIA( wxXmlNode* aVia );
};


/// Eagle circle
struct ECIRCLE
{
    double    x;
    double    y;
    double    radius;
    double    width;
    LAYER_NUM layer;

    ECIRCLE( wxXmlNode* aCircle );
};


/// Eagle XML rectangle in binary
struct ERECT
{
    double   x1;
    double   y1;
    double   x2;
    double   y2;
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
    string     name;
    opt_string value;
    opt_double x;
    opt_double y;
    opt_double size;
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

    EATTR( wxXmlNode* aTree );
    EATTR() {}
};


/// Eagle dimension element
struct EDIMENSION
{
    double x1;
    double y1;
    double x2;
    double y2;
    double x3;
    double y3;
    int    layer;

    opt_string dimensionType;

    EDIMENSION( wxXmlNode* aDimension );
};


/// Eagle text element
struct ETEXT
{
    string     text;
    double     x;
    double     y;
    double     size;
    int        layer;
    opt_string font;
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

    opt_int     align;

    ETEXT( wxXmlNode* aText );
};


/// Eagle thru hol pad
struct EPAD
{
    string     name;
    double     x;
    double     y;
    double     drill;
    opt_double diameter;

    // for shape: (square | round | octagon | long | offset)
    enum {
        SQUARE,
        ROUND,
        OCTAGON,
        LONG,
        OFFSET,
    };
    opt_int  shape;
    opt_erot rot;
    opt_bool stop;
    opt_bool thermals;
    opt_bool first;

    EPAD( wxXmlNode* aPad );
};


/// Eagle SMD pad
struct ESMD
{
    string   name;
    double   x;
    double   y;
    double   dx;
    double   dy;
    int      layer;
    opt_int  roundness;
    opt_erot rot;
    opt_bool stop;
    opt_bool thermals;
    opt_bool cream;

    ESMD( wxXmlNode* aSMD );
};


/// Eagle vertex
struct EVERTEX
{
    double      x;
    double      y;

    EVERTEX( wxXmlNode* aVertex );
};


/// Eagle polygon, without vertices which are parsed as needed
struct EPOLYGON
{
    double     width;
    int        layer;
    opt_double spacing;

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
    opt_double isolate;
    opt_bool   orphans;
    opt_bool   thermals;
    opt_int    rank;

    EPOLYGON( wxXmlNode* aPolygon );
};


/// Eagle hole element
struct EHOLE
{
    double x;
    double y;
    double drill;

    EHOLE( wxXmlNode* aHole );
};


/// Eagle element element
struct EELEMENT
{
    string   name;
    string   library;
    string   package;
    string   value;
    double   x;
    double   y;
    opt_bool locked;
    opt_bool smashed;
    opt_erot rot;

    EELEMENT( wxXmlNode* aElement );
};


struct ELAYER
{
    int      number;
    string   name;
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

/**
 * Function MapChildren
 * provides an easy access to the children of an XML node via their names.
 * @param  currentNode is a pointer to a wxXmlNode, whose children will be mapped.
 * @return NODE_MAP - a map linking the name of each children to the children itself (via a
 *                  wxXmlNode*)
 */
NODE_MAP MapChildren( wxXmlNode* currentNode );

/// Assemble a two part key as a simple concatenation of aFirst and aSecond parts,
/// using a separator.
string makeKey( const string& aFirst, const string& aSecond );

/// Make a unique time stamp
unsigned long timeStamp( wxXmlNode* aTree );

/// Convert an Eagle curve end to a KiCad center for S_ARC
wxPoint kicad_arc_center( const wxPoint& aStart, const wxPoint& aEnd, double aAngle );

#endif // _EAGLE_PARSER_H_
