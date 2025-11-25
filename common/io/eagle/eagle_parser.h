/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2017 CERN
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


#ifndef _EAGLE_PARSER_H_
#define _EAGLE_PARSER_H_

#include <map>
#include <memory>
#include <optional>
#include <unordered_map>

#include <wx/xml/xml.h>
#include <wx/string.h>
#include <wx/filename.h>

#include <layer_ids.h>
#include <trigo.h>
#include <core/wx_stl_compat.h>
#include <widgets/report_severity.h>

class FOOTPRINT;
class IO_BASE;

struct EINSTANCE;
struct EPART;
struct ETEXT;
struct ESEGMENT;

typedef std::unordered_map<wxString, wxXmlNode*> NODE_MAP;
typedef std::map<wxString, EINSTANCE*> EINSTANCE_MAP;
typedef std::map<wxString, std::unique_ptr<EPART>> EPART_MAP;

/// Translates Eagle special characters to their counterparts in KiCad.
wxString escapeName( const wxString& aNetName );

/// Interprets special characters in Eagle text and converts them to KiCAD notation.
wxString interpretText( const wxString& aText );

/// Translates Eagle special text reference to a KiCad variable reference.
bool substituteVariable( wxString* aText );

/// Converts Eagle's HTML description into KiCad description format.
wxString convertDescription( wxString aDescr );

static inline wxXmlNode* getChildrenNodes( NODE_MAP& aMap, const wxString& aName )
{
    auto it = aMap.find( aName );
    return it == aMap.end() ? nullptr : it->second->GetChildren();
}


/**
 * Implement a simple wrapper around runtime_error to isolate the errors thrown by the
 * Eagle XML parser.
 */
struct XML_PARSER_ERROR : std::runtime_error
{
    /**
     * Build an XML error by just calling its parent class constructor, std::runtime_error, with
     * the passed message.
     *
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
 * Keep track of what we are working on within a PTREE.
 *
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

    /// Modify the last path node's attribute.
    void Attribute( const char* aAttribute )
    {
        p.back().attribute = aAttribute;
    }

    /// Return the contents of the XPATH as a single string.
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
 * Convert a wxString to a generic type T.
 *
 * @param aValue is a wxString containing the value that will be converted to type T.
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
 * Model an optional XML attribute.
 *
 * This was implemented as an alternative to std::optional. This class should be replaced with a
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
     * Construct a default OPTIONAL_XML_ATTRIBUTE, whose data is not available.
     */
    OPTIONAL_XML_ATTRIBUTE() :
        m_isAvailable( false ),
        m_data( T() )
    {}

    /**
     * @param aData is a wxString containing the value that should be converted to type T. If
     *              aData is empty, the attribute is understood as unavailable; otherwise, the
     *              conversion to T is tried.
     */
    OPTIONAL_XML_ATTRIBUTE( const wxString& aData )
    {
        m_data = T();
        m_isAvailable = !aData.IsEmpty();

        if( m_isAvailable )
            Set( aData );
    }

    /**
     * @param aData is the value of the XML attribute. If this constructor is called, the
     *              attribute is available.
     */
    template<typename V = T>
    OPTIONAL_XML_ATTRIBUTE( T aData ) :
        m_isAvailable( true ),
        m_data( aData )
    {}

    /**
     * @return bool the availability of the attribute.
     */
    operator bool() const
    {
        return m_isAvailable;
    }

    /**
     * Assign to a string (optionally) containing the data.
     *
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
     * Assign to an object of the base type containing the data.
     *
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
     * @param aOther is the object of the base type that should be compared with this one.
     */
    bool operator ==( const T& aOther ) const
    {
        return m_isAvailable && ( aOther == m_data );
    }

    /**
     * Attempt to convert a string to the base type.
     *
     * @param aString is the string that will be converted to the base type.
     */
    void Set( const wxString& aString )
    {
        m_data = Convert<T>( aString );
        m_isAvailable = !aString.IsEmpty();
    }

    /**
     * Return a reference to the value of the attribute assuming it is available.
     *
     * @return T& - the value of the attribute.
     */
    T& Get()
    {
        assert( m_isAvailable );
        return m_data;
    }

    /**
     * Return a constant reference to the value of the attribute assuming it is available.
     *
     * @return const T& - the value of the attribute.
     */
    const T& CGet() const
    {
        assert( m_isAvailable );
        return m_data;
    }

    /**
     * Return a reference to the value of the attribute assuming it is available.
     *
     * @return T& - the value of the attribute.
     */
    T& operator*()
    {
        return Get();
    }

    /**
     * Return a constant reference to the value of the attribute assuming it is available.
     *
     * @return const T& - the value of the attribute.
     */
    const T& operator*() const
    {
        return CGet();
    }

    /**
     * Return a pointer to the value of the attribute assuming it is available.
     *
     * @return T* - the value of the attribute.
     */
    T* operator->()
    {
        return &Get();
    }

    /**
     * Return a constant pointer to the value of the attribute assuming it is available.
     *
     * @return const T* - the value of the attribute.
     */
    const T* operator->() const
    {
        return &CGet();
    }
};


/**
 * Fetch the number of XML nodes within \a aNode.
 *
 * @param aNode is the parent node of the children to count.
 *
 * @return the count of all child XML nodes below \a aNode.
 */
size_t GetNodeCount( const wxXmlNode* aNode );


/**
 * Provide an easy access to the children of an XML node via their names.
 *
 * @param  currentNode is a pointer to a wxXmlNode, whose children will be mapped.
 * @return NODE_MAP is a map linking the name of each children to the children itself (via a
 *                  wxXmlNode*)
 */
NODE_MAP MapChildren( wxXmlNode* aCurrentNode );

/// Convert an Eagle curve end to a KiCad center for S_ARC.
VECTOR2I ConvertArcCenter( const VECTOR2I& aStart, const VECTOR2I& aEnd, double aAngle );

// Pre-declare for typedefs
struct EROT;
struct ECOORD;
struct EURN;

typedef OPTIONAL_XML_ATTRIBUTE<wxString> opt_wxString;
typedef OPTIONAL_XML_ATTRIBUTE<int>      opt_int;
typedef OPTIONAL_XML_ATTRIBUTE<double>   opt_double;
typedef OPTIONAL_XML_ATTRIBUTE<bool>     opt_bool;
typedef OPTIONAL_XML_ATTRIBUTE<EROT>     opt_erot;
typedef OPTIONAL_XML_ATTRIBUTE<ECOORD>   opt_ecoord;
typedef OPTIONAL_XML_ATTRIBUTE<EURN>     opt_eurn;


struct EAGLE_BASE
{
    EAGLE_BASE( IO_BASE* aIo = nullptr ) :
        io( aIo ) {}

    IO_BASE* io;

    /**
     * Send a message to the #IO_BASE #REPORTER object if one exists.
     *
     * @param aMsg is the message to send to the #REPORTER object.
     */
    void Report( const wxString& aMsg, SEVERITY aSeverity = RPT_SEVERITY_UNDEFINED );

    void AdvanceProgressPhase();
};


/**
 * Container that parses Eagle library file "urn" definitions.
 *
 * According to the eagle.dtd, the "urn" definition is as follows:
 *
 * <!ENTITY % Urn "%String;"> <!-- of the form "urn:adsk.eagle:<ASSET_TYPE>:<ASSET_ID>/<VERSION>"
 *   - <ASSET_TYPE> is "symbol", "footprint", "package", "component", or "library"
 *   - <ASSET_ID> is an integer
 *   - <VERSION> is an integer
 *
 *   The "/<VERSION>" is omitted when referencing the asset without specifying a particular version.
 *   For example, "urn:adsk.eagle:component:60986/2" references version 2 of component 60986 and
 *   "urn:adsk.eagle:library:60968" references library 60986 without specifying a version.
 * 
 * Autodesk Fusion can use a different format instead, e.g.:
 * "urn:adsk.wipprod:fs.file:vf.RSKzNVZVQPeDNm7Bnfr3WQ"
 */
struct EURN : public EAGLE_BASE
{
    EURN() {}

    /// Parse an Eagle "urn" string.
    EURN( const wxString& aUrn );

    void Parse( const wxString& aUrn );

    /**
     * Check if the string passed to the ctor was a valid Eagle urn.
     *
     * @retval true if the urn string is valid.
     * @retval false if the urn string is not valid.
     */
    bool IsValid() const;

    wxString host;         ///< Should always be "urn".
    wxString path;         ///< Path to the asset type below.
    wxString assetType;    ///< Must be "symbol", "footprint", "package", "component", or "library".
    wxString assetId;      ///< The unique asset identifier for the asset type.
    wxString assetVersion; ///< May be empty depending on the asset type.
};


// All of the 'E'STRUCTS below merely hold Eagle XML information verbatim, in binary.
// For maintenance and troubleshooting purposes, it was thought that we'd need to
// separate the conversion process into distinct steps. There is no intent to have KiCad
// forms of information in these 'E'STRUCTS.  They are only binary forms
// of the Eagle information in the corresponding Eagle XML nodes.


struct EDESCRIPTION : public EAGLE_BASE
{
    /*
     * <!ELEMENT description (#PCDATA)>
     * <!ATTLIST description
     *           language      %String;       "en"
     *           >
     */

    wxString text;
    opt_wxString language;

    EDESCRIPTION( wxXmlNode* aDescription, IO_BASE* aIo = nullptr );
};


// Eagle coordinates
struct ECOORD : public EAGLE_BASE
{
    enum EAGLE_UNIT
    {
        EU_NM,     ///< nanometers
        EU_MM,     ///< millimeters
        EU_INCH,   ///< inches
        EU_MIL,    ///< mils/thous
    };

    /// Value expressed in nanometers.
    long long int value;

    /// Unit used for the value field.
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

    int To100NanoMeters() const
    {
        return value / 100;
    }

    int ToNanoMeters() const
    {
        return value;
    }

    float ToMm() const
    {
        return value / 1000000.0;
    }

    int ToSchUnits() const { return To100NanoMeters(); }
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

    /// Converts a size expressed in a certain unit to nanometers.
    static long long int ConvertToNm( int aValue, enum EAGLE_UNIT aUnit );
};


/// Eagle net
struct ENET : public EAGLE_BASE
{
    /*
     * <!ELEMENT net (segment)*>
     * <!ATTLIST net
     *           name          %String;       #REQUIRED
     *           class         %Class;        "0"
     *           >
     */
    wxString netname;
    int      netcode;

    std::vector<std::unique_ptr<ESEGMENT>> segments;

    ENET( int aNetCode, const wxString& aNetName ) :
        netname( aNetName ),
        netcode( aNetCode )
    {}

    ENET() :
        netcode( 0 )
    {}

    ENET( wxXmlNode* aNet, IO_BASE* aIo = nullptr );
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


/// Eagle vertex
struct EVERTEX : public EAGLE_BASE
{
    /*
     * <!ELEMENT vertex EMPTY>
     * <!ATTLIST vertex
     *           x             %Coord;        #REQUIRED
     *           y             %Coord;        #REQUIRED
     *           curve         %WireCurve;    "0"
     *           >
     *           <!-- curve: The curvature from this vertex to the next one -->
     */
    ECOORD      x;
    ECOORD      y;
    opt_double  curve;      ///< range is -359.9..359.9

    EVERTEX( wxXmlNode* aVertex, IO_BASE* aIo = nullptr );
};


/// Eagle wire
struct EWIRE : public EAGLE_BASE
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
     *           extent        %Extent;       #IMPLIED
     *           style         %WireStyle;    "continuous"
     *           curve         %WireCurve;    "0"
     *           cap           %WireCap;      "round"
     *           grouprefs     IDREFS         #IMPLIED
     *           >
     *           <!-- extent: Only applicable for airwires -->
     *           <!-- cap   : Only applicable if 'curve' is not zero -->
     */
    ECOORD x1;
    ECOORD y1;
    ECOORD x2;
    ECOORD y2;
    ECOORD width;
    int    layer;

    // for style: (continuous | longdash | shortdash | dashdot)
    enum { CONTINUOUS,
           LONGDASH,
           SHORTDASH,
           DASHDOT };

    opt_wxString extent;
    opt_int      style;
    opt_double   curve; ///< range is -359.9..359.9

    // for cap: (flat | round)
    enum { FLAT,
           ROUND };

    opt_int     cap;

    // TODO add grouprefs

    EWIRE( wxXmlNode* aWire, IO_BASE* aIo = nullptr );
};


/// Eagle Junction
struct EJUNCTION : public EAGLE_BASE
{
    /*
     * <!ELEMENT junction EMPTY>
     * <!ATTLIST junction
     *           x             %Coord;        #REQUIRED
     *           y             %Coord;        #REQUIRED
     *           grouprefs     IDREFS         #IMPLIED
     *           >
     */

    ECOORD     x;
    ECOORD     y;

    EJUNCTION( wxXmlNode* aJunction, IO_BASE* aIo = nullptr );
};


/// Eagle label
struct ELABEL : public EAGLE_BASE
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

    ECOORD       x;
    ECOORD       y;
    ECOORD       size;
    int          layer;
    opt_wxString font;
    opt_int      ratio;
    opt_erot     rot;
    opt_bool     xref;
    opt_wxString align;

    // TODO Add grouprefs

    ELABEL( wxXmlNode* aLabel, IO_BASE* aIo = nullptr );
};


/// Eagle via
struct EVIA : public EAGLE_BASE
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
     *           grouprefs     IDREFS         #IMPLIED
     *           >
     */
    ECOORD       x;
    ECOORD       y;
    int          layer_front_most;   /// < extent
    int          layer_back_most;    /// < inclusive
    ECOORD       drill;
    opt_ecoord   diam;
    opt_wxString shape;
    opt_bool     alwaysStop;

    // TODO add grouprefs

    EVIA( wxXmlNode* aVia, IO_BASE* aIo = nullptr );
};


/// Eagle circle
struct ECIRCLE : public EAGLE_BASE
{
    /*
     * <!ELEMENT circle EMPTY>
     * <!ATTLIST circle
     *           x             %Coord;        #REQUIRED
     *           y             %Coord;        #REQUIRED
     *           radius        %Coord;        #REQUIRED
     *           width         %Dimension;    #REQUIRED
     *           layer         %Layer;        #REQUIRED
     *           grouprefs     IDREFS         #IMPLIED
     *           >
     */
    ECOORD x;
    ECOORD y;
    ECOORD radius;
    ECOORD width;
    int    layer;

    // TODO add grouprefs

    ECIRCLE( wxXmlNode* aCircle, IO_BASE* aIo = nullptr );
};


/// Eagle XML rectangle in binary
struct ERECT : public EAGLE_BASE
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
     *           grouprefs     IDREFS         #IMPLIED
     *           >
     */
    ECOORD   x1;
    ECOORD   y1;
    ECOORD   x2;
    ECOORD   y2;
    int      layer;
    opt_erot rot;

    ERECT( wxXmlNode* aRect, IO_BASE* aIo = nullptr );
};


struct ESPLINE : public EAGLE_BASE
{
    /*
     * <!ELEMENT spline (vertex)*>
     * <!-- Four simple (non-curve) vertices define the control points of a degree-3
     *      spline curve -->
     * <!ATTLIST spline
     *           width          %Dimension;    #REQUIRED
     *           >
     */
    std::vector<std::unique_ptr<EVERTEX>> vertices;
    double                                width;

    ESPLINE( wxXmlNode* aSpline, IO_BASE* aIo = nullptr );
};


/**
 * Parse an Eagle "attribute" XML element.
 *
 * @note An attribute element is different than an XML element attribute.  The attribute element
 *       is a full XML node in and of itself, and has attributes of its own.  Blame Eagle.
 */
struct EATTR : public EAGLE_BASE
{
    /*
     * <!ELEMENT attribute EMPTY>
     * <!ATTLIST attribute
     *           name          %String;       #REQUIRED
     *           value         %String;       #IMPLIED
     *           x             %Coord;        #IMPLIED
     *           y             %Coord;        #IMPLIED
     *           size          %Dimension;    #IMPLIED
     *           layer         %Layer;        #IMPLIED
     *           font          %TextFont;     #IMPLIED
     *           ratio         %Int;          #IMPLIED
     *           rot           %Rotation;     "R0"
     *           display       %AttributeDisplay; "value"
     *           constant      %Bool;         "no"
     *           align         %Align;        "bottom-left"
     *           grouprefs     IDREFS         #IMPLIED
     *           >
     *           <!-- display: Only in <element> or <instance> context -->
     *           <!-- constant:Only in <device> context -->
     */
    wxString     name;
    opt_wxString value;
    opt_ecoord   x;
    opt_ecoord   y;
    opt_ecoord   size;
    opt_int      layer;
    opt_wxString font;
    opt_double   ratio;
    opt_erot     rot;

    enum {  // for 'display'
        Off,
        VALUE,
        NAME,
        BOTH,
    };

    opt_bool     constant;
    opt_int      display;
    opt_int      align;

    // TODO add groupdefs

    EATTR( wxXmlNode* aTree, IO_BASE* aIo = nullptr );
    EATTR() {}
};


/// Eagle dimension element
struct EDIMENSION : public EAGLE_BASE
{
    /*
     * <!ELEMENT dimension EMPTY>
     * <!ATTLIST dimension
     *           x1            %Coord;        #REQUIRED
     *           y1            %Coord;        #REQUIRED
     *           x2            %Coord;        #REQUIRED
     *           y2            %Coord;        #REQUIRED
     *           x3            %Coord;        #REQUIRED
     *           y3            %Coord;        #REQUIRED
     *           layer         %Layer;        #REQUIRED
     *           dtype         %DimensionType; "parallel"
     *           width         %Dimension;    "0.13"
     *           extwidth      %Dimension;    "0"
     *           extlength     %Dimension;    "0"
     *           extoffset     %Dimension;    "0"
     *           textsize      %Dimension;    #REQUIRED
     *           textratio     %Int;          "8"
     *           unit          %GridUnit;     "mm"
     *           precision     %Int;          "2"
     *           visible       %Bool;         "no"
     *           grouprefs     IDREFS         #IMPLIED
     *           >
     */
    ECOORD       x1;
    ECOORD       y1;
    ECOORD       x2;
    ECOORD       y2;
    ECOORD       x3;
    ECOORD       y3;
    opt_ecoord   textsize;
    int          layer;
    opt_wxString dimensionType;
    opt_double   width;
    opt_double   extwidth;
    opt_double   extlength;
    opt_double   extoffset;
    opt_int      textratio;
    opt_wxString unit;
    opt_int      precision;
    opt_bool     visible;

    // TODO add grouprefs

    EDIMENSION( wxXmlNode* aDimension, IO_BASE* aIo = nullptr );
};


/// Eagle text element
struct ETEXT : public EAGLE_BASE
{
    /*
     * <!ELEMENT text (#PCDATA)>
     * <!ATTLIST text
     *           x             %Coord;        #REQUIRED
     *           y             %Coord;        #REQUIRED
     *           size          %Dimension;    #REQUIRED
     *           layer         %Layer;        #REQUIRED
     *           font          %TextFont;     "proportional"
     *           ratio         %Int;          "8"
     *           rot           %Rotation;     "R0"
     *           align         %Align;        "bottom-left"
     *           distance      %Int;          "50"
     *           grouprefs     IDREFS         #IMPLIED
     *           >
     */
    wxString     text;
    ECOORD       x;
    ECOORD       y;
    ECOORD       size;
    int          layer;
    opt_wxString font;
    opt_double   ratio;
    opt_erot     rot;

    enum {          // for align
        CENTER = 0,
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
    opt_int distance;

    // TODO add grouprefs

    ETEXT( wxXmlNode* aText, IO_BASE* aIo = nullptr );

    /// Calculate text size based on font type and size
    VECTOR2I ConvertSize() const;
};


/**
 * Parse an Eagle frame element.
 */
struct EFRAME : public EAGLE_BASE
{
    /*
     * <!ELEMENT frame EMPTY>
     * <!ATTLIST frame
     *           x1            %Coord;       #REQUIRED
     *           y1            %Coord;       #REQUIRED
     *           x2            %Coord;       #REQUIRED
     *           y2            %Coord;       #REQUIRED
     *           columns       %Int;         #REQUIRED
     *           rows          %Int;         #REQUIRED
     *           layer         %Layer;       #REQUIRED
     *           border-left   %Bool;        "yes"
     *           border-top    %Bool;        "yes"
     *           border-right  %Bool;        "yes"
     *           border-bottom %Bool;        "yes"
     *           grouprefs     IDREFS         #IMPLIED
     *           >
     */
    ECOORD   x1;
    ECOORD   y1;
    ECOORD   x2;
    ECOORD   y2;
    int      columns;
    int      rows;
    int      layer;
    opt_bool border_left;
    opt_bool border_top;
    opt_bool border_right;
    opt_bool border_bottom;

    EFRAME( wxXmlNode* aFrameNode, IO_BASE* aIo = nullptr );
};


/// Structure holding common properties for through-hole and SMD pads
struct EPAD_COMMON : public EAGLE_BASE
{
    wxString   name;
    ECOORD     x, y;
    opt_erot   rot;
    opt_bool   stop;
    opt_bool   thermals;

    EPAD_COMMON( wxXmlNode* aPad, IO_BASE* aIo = nullptr );
};


/// Eagle thru hole pad
struct EPAD : public EPAD_COMMON
{
    /*
     * <!ELEMENT pad EMPTY>
     * <!ATTLIST pad
     *           name          %String;       #REQUIRED
     *           x             %Coord;        #REQUIRED
     *           y             %Coord;        #REQUIRED
     *           drill         %Dimension;    #REQUIRED
     *           diameter      %Dimension;    "0"
     *           shape         %PadShape;     "round"
     *           rot           %Rotation;     "R0"
     *           stop          %Bool;         "yes"
     *           thermals      %Bool;         "yes"
     *           first         %Bool;         "no"
     *           >
     */
    opt_ecoord drill;
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

    EPAD( wxXmlNode* aPad, IO_BASE* aIo = nullptr );
};


/// Eagle SMD pad
struct ESMD : public EPAD_COMMON
{
    /*
     * <!ELEMENT smd EMPTY>
     * <!ATTLIST smd
     *           name          %String;       #REQUIRED
     *           x             %Coord;        #REQUIRED
     *           y             %Coord;        #REQUIRED
     *           dx            %Dimension;    #REQUIRED
     *           dy            %Dimension;    #REQUIRED
     *           layer         %Layer;        #REQUIRED
     *           roundness     %Int;          "0"
     *           rot           %Rotation;     "R0"
     *           stop          %Bool;         "yes"
     *           thermals      %Bool;         "yes"
     *           cream         %Bool;         "yes"
     *           >
     */
    ECOORD   dx;
    ECOORD   dy;
    int      layer;
    opt_int  roundness;
    opt_bool cream;

    ESMD( wxXmlNode* aSMD, IO_BASE* aIo = nullptr );
};


/// Eagle pin element
struct EPIN : public EAGLE_BASE
{
    /*
     * <!ELEMENT pin EMPTY>
     * <!ATTLIST pin
     *           name          %String;       #REQUIRED
     *           x             %Coord;        #REQUIRED
     *           y             %Coord;        #REQUIRED
     *           visible       %PinVisible;   "both"
     *           length        %PinLength;    "long"
     *           direction     %PinDirection; "io"
     *           function      %PinFunction;  "none"
     *           swaplevel     %Int;          "0"
     *           rot           %Rotation;     "R0"
     *           >
     */
    wxString name;
    ECOORD   x;
    ECOORD   y;

    opt_wxString visible;
    opt_wxString length;
    opt_wxString direction;
    opt_wxString function;
    opt_int      swaplevel;
    opt_erot     rot;

    EPIN( wxXmlNode* aPin, IO_BASE* aIo = nullptr );
};


/// Eagle polygon, without vertices which are parsed as needed
struct EPOLYGON : public EAGLE_BASE
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
        ESOLID,
        EHATCH,
        ECUTOUT,
    };

    int        pour;
    opt_ecoord isolate;
    opt_bool   orphans;
    opt_bool   thermals;
    opt_int    rank;

    std::vector<std::unique_ptr<EVERTEX>> vertices;

    // TODO add grouprefs

    EPOLYGON( wxXmlNode* aPolygon, IO_BASE* aIo = nullptr );
};


/// Eagle hole element
struct EHOLE : public EAGLE_BASE
{
    /*
     * <!ELEMENT hole EMPTY>
     * <!ATTLIST hole
     *           x             %Coord;        #REQUIRED
     *           y             %Coord;        #REQUIRED
     *           drill         %Dimension;    #REQUIRED
     *           grouprefs     IDREFS         #IMPLIED
     *           >
     */
    ECOORD x;
    ECOORD y;
    ECOORD drill;

    EHOLE( wxXmlNode* aHole, IO_BASE* aIo = nullptr );
};


struct EVARIANT : public EAGLE_BASE
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
    wxString     name;
    opt_bool     populate;
    opt_wxString value;
    opt_wxString technology;

    EVARIANT( wxXmlNode* aVariant, IO_BASE* aIo = nullptr );
};


struct EPINMAP : public EAGLE_BASE
{
    /*
     * <!ELEMENT pinmap EMPTY>
     * <!ATTLIST pinmap
     *           gate          %String;       #REQUIRED
     *           pin           %String;       #REQUIRED
     *           pinorder      %String;       #REQUIRED
     *           >
     */
    wxString gate;
    wxString pin;
    wxString pinorder;

    EPINMAP( wxXmlNode* aPinMap, IO_BASE* aIo = nullptr );
};


struct EPINMAPPING : public EAGLE_BASE
{
    /*
     * <!ELEMENT pinmapping (pinmap+)>
     * <!ATTLIST pinmapping
     *           isusermap     %Bool;         "no"
     *           iddevicewide  %Bool;         "yes"
     *           spiceprefix   %String;       ""
     *           >
     */
    std::vector<std::unique_ptr<EPINMAP>> pinmaps;
    opt_bool                              isusermap;
    opt_bool                              iddevicewide;
    opt_wxString                          spiceprefix;

    EPINMAPPING( wxXmlNode* aPinMap, IO_BASE* aIo = nullptr );
};


struct EMODEL : public EAGLE_BASE
{
    /*
     * <!ELEMENT model (#PCDATA)>
     * <!ATTLIST model
     *           name          %String;       #REQUIRED
     *           >
     */
    wxString name;
    wxString model;

    EMODEL( wxXmlNode* aModel, IO_BASE* aIo = nullptr );
};


struct ESPICE : public EAGLE_BASE
{
    /*
     * <!ELEMENT spice (pinmapping, model)>
     */
    std::unique_ptr<EPINMAPPING> pinmapping;
    std::unique_ptr<EMODEL>      model;

    ESPICE( wxXmlNode* aSpice, IO_BASE* aIo = nullptr );
};


/// Eagle element element
struct EELEMENT : public EAGLE_BASE
{
    /*
     * <!ELEMENT element (attribute*, variant*)>
     *           <!-- variant* is accepted only for compatibility with EAGLE 6.x files -->
     * <!ATTLIST element
     *           name          %String;       #REQUIRED
     *           library       %String;       #REQUIRED
     *           library_urn   %Urn;          ""
     *           package       %String;       #REQUIRED
     *           package3d_urn %Urn;          ""
     *           override_package3d_urn %Urn; ""
     *           override_package_urn %Urn;    ""
     *           override_locally_modified %Bool; "no"
     *           value         %String;       #REQUIRED
     *           x             %Coord;        #REQUIRED
     *           y             %Coord;        #REQUIRED
     *           locked        %Bool;         "no"
     *           populate      %Bool;         "yes"
     *           smashed       %Bool;         "no"
     *           rot           %Rotation;     "R0"
     *           grouprefs     IDREFS         #IMPLIED
     *           >
     *           <!-- library_urn: Only in parts from online libraries -->
     */
    std::map<wxString, std::unique_ptr<EATTR>>    attributes;
    std::map<wxString, std::unique_ptr<EVARIANT>> variants;

    wxString     name;
    wxString     library;
    opt_eurn     library_urn;
    wxString     package;
    opt_wxString package3d_urn;
    opt_wxString override_package3d_urn;
    opt_bool     override_locally_modified;
    wxString     value;
    ECOORD       x;
    ECOORD       y;
    opt_bool     locked;
    opt_bool     smashed;
    opt_erot     rot;

    // TODO add grouprefs

    EELEMENT( wxXmlNode* aElement, IO_BASE* aIo = nullptr );
};


struct ELAYER : public EAGLE_BASE
{
    /*
     * <!ELEMENT layer EMPTY>
     * <!ATTLIST layer
     *           number        %Layer;        #REQUIRED
     *           name          %String;       #REQUIRED
     *           color         %Int;          #REQUIRED
     *           fill          %Int;          #REQUIRED
     *           visible       %Bool;         "yes"
     *           active        %Bool;         "yes"
     *           >
     */
    int      number;
    wxString name;
    int      color;
    int      fill;
    opt_bool visible;
    opt_bool active;

    ELAYER( wxXmlNode* aLayer, IO_BASE* aIo = nullptr );
};


struct EAGLE_LAYER : public EAGLE_BASE
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


struct EGATE : public EAGLE_BASE
{
    /*
     * <!ELEMENT gate EMPTY>
     * <!ATTLIST gate
     *           name          %String;       #REQUIRED
     *           symbol        %String;       #REQUIRED
     *           x             %Coord;        #REQUIRED
     *           y             %Coord;        #REQUIRED
     *           addlevel      %GateAddLevel; "next"
     *           swaplevel     %Int;          "0"
     *           >
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

    EGATE( wxXmlNode* aGate, IO_BASE* aIo = nullptr );
};


struct EPART : public EAGLE_BASE
{
    /*
     * <!ELEMENT part (attribute*, variant*, spice?)>
     * <!ATTLIST part
     *           name          %String;       #REQUIRED
     *           library       %String;       #REQUIRED
     *           library_urn   %Urn;          ""
     *           deviceset     %String;       #REQUIRED
     *           device        %String;       #REQUIRED
     *           package3d_urn %Urn;          ""
     *           override_package3d_urn %Urn; ""
     *           override_package_urn %Urn;    ""
     *           override_locally_modified %Bool; "no"
     *           technology    %String;       ""
     *           value         %String;       #IMPLIED
     *           >
     *           <!-- library_urn: Only in parts from online libraries -->
     */
    std::map<wxString, std::unique_ptr<EATTR>>    attributes;
    std::map<wxString, std::unique_ptr<EVARIANT>> variants;
    std::unique_ptr<ESPICE>                       spice;

    wxString     name;
    wxString     library;
    opt_eurn     libraryUrn;
    wxString     deviceset;
    wxString     device;
    opt_wxString package3d_urn;
    opt_wxString override_package3d_urn;
    opt_wxString override_package_urn;
    opt_bool     override_locally_modified;
    opt_wxString technology;
    opt_wxString value;

    EPART( wxXmlNode* aPart, IO_BASE* aIo = nullptr );
};


struct EINSTANCE : public EAGLE_BASE
{
    /*
     * <!ELEMENT instance (attribute)*>
     * <!ATTLIST instance
     *           part          %String;       #REQUIRED
     *           gate          %String;       #REQUIRED
     *           x             %Coord;        #REQUIRED
     *           y             %Coord;        #REQUIRED
     *           smashed       %Bool;         "no"
     *           rot           %Rotation;     "R0"
     *           grouprefs     IDREFS         #IMPLIED
     *           >
     *           <!-- rot: Only 0, 90, 180 or 270 -->
     */

    wxString part;
    wxString gate;
    ECOORD   x;
    ECOORD   y;
    opt_bool smashed;
    opt_erot rot;

    // TODO: add grouprefs

    std::map<wxString, std::unique_ptr<EATTR>> attributes;

    EINSTANCE( wxXmlNode* aInstance, IO_BASE* aIo = nullptr );
};


struct ECONNECT : public EAGLE_BASE
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
    wxString     gate;
    wxString     pin;
    wxString     pad;
    opt_wxString contactroute;

    ECONNECT( wxXmlNode* aConnect, IO_BASE* aIo = nullptr );
};


struct ETECHNOLOGY : public EAGLE_BASE
{
    /*
     * <!ELEMENT technology (attribute)*>
     * <!ATTLIST technology
     *           name          %String;       #REQUIRED
     *           >
     */
    wxString name;

    std::vector<std::unique_ptr<EATTR>> attributes;

    ETECHNOLOGY( wxXmlNode* aTechnology, IO_BASE* aIo = nullptr );
};


struct EPACKAGE3DINST : public EAGLE_BASE
{
    /*
     * <!ELEMENT package3dinstance EMPTY>
     * <!ATTLIST package3dinstance
     *           package3d_urn %Urn;          #REQUIRED
     *           >
     */
    wxString package3d_urn;

    EPACKAGE3DINST( wxXmlNode* aPackage3dInst, IO_BASE* aIo = nullptr );
};


struct EDEVICE : public EAGLE_BASE
{
    /*
     * <!ELEMENT device (connects?, package3dinstances?, technologies?)>
     * <!ATTLIST device
     *           name          %String;       ""
     *           package       %String;       #IMPLIED
     *           >
     */
    wxString     name;
    opt_wxString package;

    std::vector<std::unique_ptr<ECONNECT>>         connects;
    std::vector < std::unique_ptr<EPACKAGE3DINST>> package3dinstances;
    std::vector < std::unique_ptr<ETECHNOLOGY>>    technologies;

    EDEVICE( wxXmlNode* aDevice, IO_BASE* aIo = nullptr );
};


struct EDEVICE_SET : public EAGLE_BASE
{
    /*
     * <!ELEMENT deviceset (description?, gates, devices, spice?)>
     * <!ATTLIST deviceset
     *           name                     %String;    #REQUIRED
     *           urn                      %Urn;       ""
     *           locally_modified         %Bool;      "no"
     *           prefix                   %String;    ""
     *           uservalue                %Bool;      "no"
     *           library_version          %Int;       ""
     *           library_locally_modified %Bool;      "no"
     *           >
     *           <!-- library_version and library_locally_modified: Only in managed libraries
     *                inside boards or schematics -->
     */

    wxString     name;
    opt_eurn     urn;
    opt_bool     locally_modified;
    opt_wxString prefix;
    opt_bool     uservalue;
    opt_int      library_version;
    opt_bool     library_locally_modified;

    std::optional<EDESCRIPTION>                description;
    std::map<wxString, std::unique_ptr<EGATE>> gates;
    std::vector<std::unique_ptr<EDEVICE>>      devices;
    std::optional<ESPICE>                      spice;

    EDEVICE_SET( wxXmlNode* aDeviceSet, IO_BASE* aIo = nullptr );
};


struct ECLASS : public EAGLE_BASE
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

    wxString number;
    wxString name;
    opt_ecoord width;
    opt_ecoord drill;

    std::map<wxString, ECOORD> clearanceMap;

    ECLASS( wxXmlNode* aClass, IO_BASE* aIo = nullptr );
};


struct EPORT : public EAGLE_BASE
{
    /*
     * <!ELEMENT port EMPTY>
     * <!ATTLIST port
     *           name          %String;       #REQUIRED
     *           side          %String;       #REQUIRED
     *           coord         %Coord;        #REQUIRED
     *           direction     %PortDirection; "io"
     *           >
     *
     * The eagle.dtd is incorrect for the EPORT side attribute.  It is not an integer, it is a
     * string that defines the side of the module rectangle the port is located.  Valid values
     * are "top", "bottom", "right", and "left".
     */
    wxString     name;
    wxString     side;
    ECOORD       coord;
    opt_wxString direction;

    EPORT( wxXmlNode* aPort, IO_BASE* aIo = nullptr );
};


struct EVARIANTDEF : public EAGLE_BASE
{
    /*
     * <!ELEMENT variantdef EMPTY>
     * <!ATTLIST variantdef
     *           name          %String;       #REQUIRED
     *           current       %Bool;         "no"
     *           >
     */
    wxString name;
    opt_bool current;

    EVARIANTDEF( wxXmlNode* aVariantDef, IO_BASE* aIo = nullptr );
};


struct ESCHEMATIC_GROUP : public EAGLE_BASE
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
    wxString     name;
    opt_bool     selectable;
    opt_ecoord   width;
    opt_ecoord   titleSize;
    opt_wxString titleFont;
    opt_wxString wireStyle;
    opt_bool     showAnnotations;
    opt_int      layer;
    opt_wxString grouprefs;

    std::optional<EDESCRIPTION>         description;
    std::vector<std::unique_ptr<EATTR>> attributes;

    ESCHEMATIC_GROUP( wxXmlNode* aSchematicGroup, IO_BASE* aIo = nullptr );
};


struct EPLAIN : public EAGLE_BASE
{
    /*
     * <!ELEMENT plain (polygon | wire | text | dimension | circle | spline | rectangle |
     *                  frame | hole)*>
     */

    std::vector<std::unique_ptr<EPOLYGON>>   polygons;
    std::vector<std::unique_ptr<EWIRE>>      wires;
    std::vector<std::unique_ptr<ETEXT>>      texts;
    std::vector<std::unique_ptr<EDIMENSION>> dimensions;
    std::vector<std::unique_ptr<ECIRCLE>>    circles;
    std::vector<std::unique_ptr<ESPLINE>>    splines;
    std::vector<std::unique_ptr<ERECT>>      rectangles;
    std::vector<std::unique_ptr<EFRAME>>     frames;
    std::vector<std::unique_ptr<EHOLE>>      holes;

    EPLAIN( wxXmlNode* aPlain, IO_BASE* aIo = nullptr );
};


struct EMODULEINST : public EAGLE_BASE
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

    wxString     name;
    wxString     moduleinst;
    opt_wxString moduleVariant;
    ECOORD       x;
    ECOORD       y;
    opt_int      offset;
    opt_bool     smashed;
    opt_erot     rotation;

    EMODULEINST( wxXmlNode* aModuleInst, IO_BASE* aIo = nullptr );
};


struct EPINREF : public EAGLE_BASE
{
    /*
     * <!ELEMENT pinref EMPTY>
     * <!ATTLIST pinref
     *           part          %String;       #REQUIRED
     *           gate          %String;       #REQUIRED
     *           pin           %String;       #REQUIRED
     *           >
     */
    wxString part;
    wxString gate;
    wxString pin;

    EPINREF( wxXmlNode* aPinRef, IO_BASE* aIo = nullptr );
};


struct EPORTREF : public EAGLE_BASE
{
    /*
     * <!ELEMENT portref EMPTY>
     * <!ATTLIST portref
     *           moduleinst    %String;       #REQUIRED
     *           port          %String;       #REQUIRED
     *           >
     */
    wxString moduleinst;
    wxString port;

    EPORTREF( wxXmlNode* aPortRef, IO_BASE* aIo = nullptr );
};


struct EPROBE : public EAGLE_BASE
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
    ECOORD       x;
    ECOORD       y;
    double       size;
    int          layer;
    opt_wxString font;
    int          ratio;
    opt_erot     rot;
    opt_bool     xref;

    // TODO add grouprefs

    EPROBE( wxXmlNode* aProbe, IO_BASE* aIo = nullptr );
};


struct ESEGMENT : public EAGLE_BASE
{
    /*
     * <!ELEMENT segment (pinref | portref | wire | junction | label | probe)*>
     *           <!-- 'pinref' and 'junction' are only valid in a <net> context -->
     */
    std::vector<std::unique_ptr<EPINREF>>   pinRefs;
    std::vector<std::unique_ptr<EPORTREF>>  portRefs;
    std::vector<std::unique_ptr<EWIRE>>     wires;
    std::vector<std::unique_ptr<EJUNCTION>> junctions;
    std::vector<std::unique_ptr<ELABEL>>    labels;
    std::vector<std::unique_ptr<EPROBE>>    probes;

    ESEGMENT( wxXmlNode* aSegment, IO_BASE* aIo = nullptr );
};


struct EBUS : public EAGLE_BASE
{
    /*
     * <!ELEMENT bus (segment)*>
     * <!ATTLIST bus
     *           name          %String;       #REQUIRED
     *           >
     */

    wxString name;
    std::vector<std::unique_ptr<ESEGMENT>> segments;

    EBUS( wxXmlNode* aBus, IO_BASE* aIo = nullptr );
};


struct ESHEET : public EAGLE_BASE
{
    /*
     * <!ELEMENT sheet (description?, plain?, moduleinsts?, instances?, busses?, nets?)>
     */

    std::optional<EDESCRIPTION>                      description;
    std::unique_ptr<EPLAIN>                          plain;
    std::map<wxString, std::unique_ptr<EMODULEINST>> moduleinsts;
    std::vector<std::unique_ptr<EINSTANCE>>          instances;
    std::vector<std::unique_ptr<EBUS>>               busses;
    std::vector<std::unique_ptr<ENET>>               nets;

    ESHEET( wxXmlNode* aSheet, IO_BASE* aIo = nullptr );
};


struct EMODULE : public EAGLE_BASE
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
    wxString     name;
    opt_wxString prefix;
    ECOORD       dx;
    ECOORD       dy;

    std::optional<EDESCRIPTION>                           description;
    std::map<wxString, std::unique_ptr<EPORT>>            ports;
    std::map<wxString, std::unique_ptr<EVARIANTDEF>>      variantdefs;
    std::map<wxString, std::unique_ptr<ESCHEMATIC_GROUP>> groups;
    std::map<wxString, std::unique_ptr<EPART>>            parts;
    std::vector<std::unique_ptr<ESHEET>>                  sheets;

    EMODULE( wxXmlNode* aModule, IO_BASE* aIo = nullptr );
};


struct ENOTE : public EAGLE_BASE
{
    /*
     * <!ELEMENT note (#PCDATA)>
     * <!ATTLIST note
     *           version       %Real;         #REQUIRED
     *           severity      %Severity;     #REQUIRED
     *           >
     *           <!-- version: The EAGLE program version that introduced this compatibility note -->
     */
    double   version;
    wxString severity;
    wxString note;

    ENOTE( wxXmlNode* aNote, IO_BASE* aIo = nullptr );
};


struct ECOMPATIBILITY : public EAGLE_BASE
{
    /*
     * <!ELEMENT compatibility (note)*>
     */
    std::vector<std::unique_ptr<ENOTE>> notes;

    ECOMPATIBILITY( wxXmlNode* aCompatibility, IO_BASE* aIo = nullptr );
};


struct ESETTING : public EAGLE_BASE
{
    /*
     * <!ELEMENT setting EMPTY>
     * <!ATTLIST setting
     *           alwaysvectorfont  %Bool;         #IMPLIED
     *           verticaltext      %VerticalText; "up"
     *           keepoldvectorfont %Bool;         "no"
     *           >
     */
    opt_bool     alwaysvectorfont;
    opt_wxString verticaltext;
    opt_bool     keepoldvectorfont;

    ESETTING( wxXmlNode* aSetting, IO_BASE* aIo = nullptr );
};


struct EGRID : public EAGLE_BASE
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
    opt_double   distance;
    opt_wxString unitdist;
    opt_wxString unit;
    opt_wxString style;
    opt_int      multiple;
    opt_bool     display;
    opt_double   altdistance;
    opt_wxString altunitdist;
    opt_wxString altunit;

    EGRID( wxXmlNode* aGrid, IO_BASE* aIo = nullptr );
};


struct EFILTER : public EAGLE_BASE
{
    /*
     * <!ELEMENT filter EMPTY>
     * <!ATTLIST filter
     *           name          %String;       #REQUIRED
     *           expression    %String;       #REQUIRED
     *           >
     */
    wxString name;
    wxString expression;

    EFILTER( wxXmlNode* aGrid, IO_BASE* aIo = nullptr );
};


struct EPACKAGE : public EAGLE_BASE
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
    wxString     name;
    opt_eurn     urn;
    opt_bool     locally_modified;
    opt_int      library_version;
    opt_bool     library_locally_modified;

    std::optional<EDESCRIPTION>              description;
    std::vector<std::unique_ptr<EPOLYGON>>   polygons;
    std::vector<std::unique_ptr<EWIRE>>      wires;
    std::vector<std::unique_ptr<ETEXT>>      texts;
    std::vector<std::unique_ptr<EDIMENSION>> dimensions;
    std::vector<std::unique_ptr<ECIRCLE>>    circles;
    std::vector<std::unique_ptr<ERECT>>      rectangles;
    std::vector<std::unique_ptr<EFRAME>>     frames;
    std::vector<std::unique_ptr<EHOLE>>      holes;
    std::vector<std::unique_ptr<EPAD>>       thtpads;
    std::vector<std::unique_ptr<ESMD>>       smdpads;

    EPACKAGE( wxXmlNode* aPackage, IO_BASE* aIo = nullptr );
};


struct EPACKAGEINSTANCE : public EAGLE_BASE
{
    /*
     * <!ELEMENT packageinstance EMPTY>
     * <!ATTLIST packageinstance
     *           name          %String;       #REQUIRED
     *           >
     */
    wxString name;

    EPACKAGEINSTANCE( wxXmlNode* aPackageInstance, IO_BASE* aIo = nullptr );
};


struct EPACKAGE3D : public EAGLE_BASE
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
    wxString name;
    EURN     urn;
    wxString type;
    opt_int  library_version;
    opt_bool library_locally_modified;

    std::optional<EDESCRIPTION>                    description;
    std::vector<std::unique_ptr<EPACKAGEINSTANCE>> packageinstances;

    EPACKAGE3D( wxXmlNode* aPackage3d, IO_BASE* aIo = nullptr );
};


struct ESYMBOL : public EAGLE_BASE
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

    wxString     name;
    opt_eurn     urn;
    opt_bool     locally_modified;
    opt_int      library_version;
    opt_bool     library_locally_modified;

    std::optional<EDESCRIPTION>              description;
    std::vector<std::unique_ptr<EPOLYGON>>   polygons;
    std::vector<std::unique_ptr<EWIRE>>      wires;
    std::vector<std::unique_ptr<ETEXT>>      texts;
    std::vector<std::unique_ptr<EDIMENSION>> dimensions;
    std::vector<std::unique_ptr<EPIN>>       pins;
    std::vector<std::unique_ptr<ECIRCLE>>    circles;
    std::vector<std::unique_ptr<ERECT>>      rectangles;
    std::vector<std::unique_ptr<EFRAME>>     frames;

    ESYMBOL( wxXmlNode* aSymbol, IO_BASE* aIo = nullptr );
};


struct ELIBRARY : public EAGLE_BASE
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
    wxString     name;
    opt_eurn     urn;

    std::optional<EDESCRIPTION>                      description;
    std::map<wxString, std::unique_ptr<EPACKAGE>>    packages;
    std::map<wxString, std::unique_ptr<EPACKAGE3D>>  packages3d;
    std::map<wxString, std::unique_ptr<ESYMBOL>>     symbols;
    std::map<wxString, std::unique_ptr<EDEVICE_SET>> devicesets;

    /**
     * Fetch the fully unique library name.
     *
     * @return the unique library name.
     */
    wxString GetName() const;
    ELIBRARY( wxXmlNode* aLibrary, IO_BASE* aIo = nullptr );
};


struct EAPPROVED : public EAGLE_BASE
{
    /*
     * <!ELEMENT approved EMPTY>
     * <!ATTLIST approved
     *           hash          %String;       #REQUIRED
     *           >
     */
    wxString hash;

    EAPPROVED( wxXmlNode* aApproved, IO_BASE* aIo = nullptr );
};


struct ESCHEMATIC : public EAGLE_BASE
{
    /*
     * <!ELEMENT schematic (description?, libraries?, attributes?, variantdefs?, classes?,
     *           modules?, groups?, parts?, sheets?, errors?)>
     * <!ATTLIST schematic
     *           xreflabel     %String;       #IMPLIED
     *           xrefpart      %String;       #IMPLIED
     *           >
     */
    opt_wxString xreflabel;
    opt_wxString xrefpart;

    std::optional<EDESCRIPTION>                           description;
    std::map<wxString, std::unique_ptr<ELIBRARY>>         libraries;
    std::map<wxString, std::unique_ptr<EATTR>>            attributes;
    std::map<wxString, std::unique_ptr<EVARIANTDEF>>      variantdefs;
    std::map<wxString, std::unique_ptr<ECLASS>>           classes;
    std::map<wxString, std::unique_ptr<EMODULE>>          modules;
    std::map<wxString, std::unique_ptr<ESCHEMATIC_GROUP>> groups;
    std::map<wxString, std::unique_ptr<EPART>>            parts;
    std::vector<std::unique_ptr<ESHEET>>                  sheets;
    std::vector<std::unique_ptr<EAPPROVED>>               errors;

    ESCHEMATIC( wxXmlNode* aSchematic, IO_BASE* aIo = nullptr );
};


struct EDRAWING : public EAGLE_BASE
{
    /*
     * <!ELEMENT drawing (settings?, grid?, filters?, layers, (library | schematic | board))>
     */
    std::vector<std::unique_ptr<ESETTING>> settings;
    std::optional<EGRID>                   grid;
    std::vector<std::unique_ptr<EFILTER>>  filters;
    std::vector<std::unique_ptr<ELAYER>>   layers;
    std::optional<ESCHEMATIC>              schematic;
    std::optional<ELIBRARY>                library;
    // std::optional<std::unique_ptr<EBOARD>> board;

    EDRAWING( wxXmlNode* aDrawing, IO_BASE* aIo = nullptr );
};


struct EAGLE_DOC : public EAGLE_BASE
{
    /*
     * <!ELEMENT eagle (compatibility?, drawing, compatibility?)>
     * <!ATTLIST eagle
     *           version       %Real;         #REQUIRED
     *           >
     *           <!-- version: The EAGLE program version that generated this file, in the
     *                form V.RR -->
     */

    /**
     * The Eagle XML file version.
     *
     * @note Even though the Eagle XML file claims the version is a %%Real(floating point number),
     *       this is not the case.  The version string in the XML file is a major.minor.micro
     *       format so it's just parsed a string.
     */
    wxString version;

    std::unique_ptr<EDRAWING>     drawing;
    std::optional<ECOMPATIBILITY> compatibility;

    EAGLE_DOC( wxXmlNode* aEagleDoc, IO_BASE* aIo = nullptr );
};


#endif // _EAGLE_PARSER_H_
