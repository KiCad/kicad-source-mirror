/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2010 KiCad Developers, see change_log.txt for contributors.
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


#ifndef SCH_PART_H_
#define SCH_PART_H_

#include <sch_lib.h>
#include <sch_lib_table.h>
#include <sch_lpid.h>
//#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/ptr_container/ptr_map.hpp>


#define INTERNAL_PER_LOGICAL        10000   ///< no. internal units per logical unit


/**
 * Function InternalToLogical
 * converts an internal coordinate to a logical coordinate.  Logical coordinates
 * are defined as the standard distance between pins being equal to one.
 * Internal coordinates are currently INTERNAL_PER_LOGICAL times that.
 */
static inline double InternalToLogical( int aCoord )
{
    return double( aCoord ) / INTERNAL_PER_LOGICAL;
}


/**
 * Function LogicalToInternal
 * converts a logical coordinate to an internal coordinate.  Logical coordinates
 * are defined as the standard distance between pins being equal to one.
 * Internal coordinates are currently INTERNAL_PER_LOGICAL times that.
 */
static inline int LogicalToInternal( double aCoord )
{
    return int( aCoord * INTERNAL_PER_LOGICAL );
}

static inline int WidthToInternal( double aWidth )
{
    // sweet line widths are a "percent of a logical unit"
    return LogicalToInternal( aWidth ) / 100;
}

static inline double InternalToWidth( int aWidth )
{
    // sweet line widths are a "percent of a logical unit"
    return InternalToLogical( aWidth ) * 100;
}

static inline int FontzToInternal( double aFontSize )
{
    // sweet font sizes are deci-pins
    return LogicalToInternal( aFontSize ) / 10;
}

static inline double InternalToFontz( int aFontSize )
{
    // sweet font sizes are deci-pins
    return InternalToLogical( aFontSize ) * 10;
}


//-----<temporary home for PART sub objects, move after stable>------------------

#include <wx/gdicmn.h>
#include <deque>
#include <vector>
#include <set>
#include <sweet_lexer.h>

class OUTPUTFORMATTER;

/// Control Bits for Format() functions
#define CTL_OMIT_NL     (1<<0)          ///< omit new line in Format()s.

namespace SCH {

class PART;
class SWEET_PARSER;
class PROPERTY;

class POINT : public wxPoint
{
public:
    POINT( int x, int y ) :
        wxPoint( x, y )
    {}

    POINT( const POINT& r ) :
        wxPoint( r )
    {}

    POINT() :
        wxPoint()
    {}

    // assume assignment operator is inherited.
};

};

/// a set of pin padnames that are electrically equivalent for a PART.
typedef std::set< wxString >            MERGE_SET;

/// The key is the VISIBLE_PIN from
/// (pin_merge VISIBLE_PIN (hide HIDDEN_PIN1 HIDDEN_PIN2...))
typedef boost::ptr_map< wxString, MERGE_SET >  MERGE_SETS;


/**
 * Class FONTZ
 * is the size of a font, and comes with a constructor which initializes
 * height and width to special values which defer font size decision to
 * a higher control.
 */
class FONTZ
{
public:

#define FONTZ_DEFAULT       -1      ///< when size defers to higher control

    FONTZ() :
        height( FONTZ_DEFAULT ),
        width(  FONTZ_DEFAULT )
    {}

    int     height;
    int     width;
};


typedef float   ANGLE;
typedef int     STROKE;             ///< will be a class someday, currently only line width


namespace SCH {

class FONT
{
    friend class PART;
    friend class SWEET_PARSER;

protected:
    wxString        name;           ///< name or other id such as number, TBD
    FONTZ           size;

    bool            italic;
    bool            bold;

public:
    FONT() :
        italic( false ),
        bold( false )
    {}

    void Format( OUTPUTFORMATTER* aFormatter, int aNestLevel, int aControlBits ) const;

    // trust compiler to write its own assignment operator for this class OK.
};


struct TEXT_EFFECTS
{
    POINT       pos;
    ANGLE       angle;
    FONT        font;
    bool        isVisible;

    PROPERTY*   property;       ///< only used from a COMPONENT, specifies PROPERTY in PART
    wxString    propName;       ///< only used from a COMPONENT, specifies PROPERTY in PART

    TEXT_EFFECTS() :
        angle( 0 ),
        isVisible( false ),
        property( 0 )
    {}

    void Format( OUTPUTFORMATTER* aFormatter, int aNestLevel, int aControlBits ) const;

    // trust compiler to write its own assignment operator for this class OK.
};


#define STROKE_DEFAULT      -1          ///< defer line width decision to higher control

#define FILL_TYPE_DEFAULT   PR::T_none  ///< fillType defaut

class BASE_GRAPHIC
{
    friend class PART;
    friend class SWEET_PARSER;

protected:
    PART*       owner;
    PART*       birthplace;         ///< at which PART in inheritance chain was 'this' added

public:
    BASE_GRAPHIC( PART* aOwner ) :
        owner( aOwner ),
        birthplace( aOwner )
    {}

    virtual ~BASE_GRAPHIC() {}

    /**
     * Function Clone
     * invokes the copy constructor on a heap allocated object of this same
     * type and creates a deep copy of 'this' into it
     * @param aOwner is the owner of the returned, new object.
     */
    virtual BASE_GRAPHIC* Clone( PART* aOwner ) const = 0;

    static const char* ShowFill( int aFillType )
    {
        return SWEET_LEXER::TokenName( PR::T( aFillType ) );
    }

    /**
     * Function Format
     * outputs this object to @a aFormatter in s-expression form.
     */
    virtual void Format( OUTPUTFORMATTER* aFormatter, int aNestLevel, int aControlBits ) const
    {}
};


typedef std::deque<POINT>  POINTS;

class POLY_LINE : public BASE_GRAPHIC
{
    friend class PART;
    friend class SWEET_PARSER;

protected:
    STROKE      stroke;
    int         fillType;       // T_none, T_filled, or T_transparent
    POINTS      pts;

    void formatContents( OUTPUTFORMATTER* aFormatter, int aNestLevel, int aControlBits ) const;

public:
    POLY_LINE( PART* aOwner ) :
        BASE_GRAPHIC( aOwner ),
        stroke( STROKE_DEFAULT ),
        fillType( PR::T_none )
    {
    }

    void Format( OUTPUTFORMATTER* aFormatter, int aNestLevel, int aControlBits ) const;

    BASE_GRAPHIC* Clone( PART* aOwner ) const
    {
        POLY_LINE* n = new POLY_LINE( *this );
        n->owner = aOwner;
        return n;
    }
};


class BEZIER : public POLY_LINE
{
    friend class PART;
    friend class SWEET_PARSER;

public:
    BEZIER( PART* aOwner ) :
        POLY_LINE( aOwner )
    {
        stroke    = STROKE_DEFAULT;
        fillType  = PR::T_none;
    }

    void Format( OUTPUTFORMATTER* aFormatter, int aNestLevel, int aControlBits ) const;

    BASE_GRAPHIC* Clone( PART* aOwner ) const
    {
        BEZIER* n = new BEZIER( *this );
        n->owner = aOwner;
        return n;
    }
};


class RECTANGLE : public BASE_GRAPHIC
{
    friend class PART;
    friend class SWEET_PARSER;

protected:
    STROKE      stroke;
    int         fillType;       // T_none, T_filled, or T_transparent
    POINT       start;
    POINT       end;

public:
    RECTANGLE( PART* aOwner ) :
        BASE_GRAPHIC( aOwner ),
        stroke( STROKE_DEFAULT ),
        fillType( FILL_TYPE_DEFAULT )
    {
    }

    void Format( OUTPUTFORMATTER* aFormatter, int aNestLevel, int aControlBits ) const;

    BASE_GRAPHIC* Clone( PART* aOwner ) const
    {
        RECTANGLE* n = new RECTANGLE( *this );
        n->owner = aOwner;
        return n;
    }
};


class CIRCLE : public BASE_GRAPHIC
{
    friend class PART;
    friend class SWEET_PARSER;

protected:
    POINT       center;
    int         radius;
    STROKE      stroke;
    int         fillType;       // T_none, T_filled, or T_transparent

public:
    CIRCLE( PART* aOwner ) :
        BASE_GRAPHIC( aOwner ),
        radius( LogicalToInternal( 0.5 ) ),
        stroke( STROKE_DEFAULT ),
        fillType( FILL_TYPE_DEFAULT )
    {
    }

    void Format( OUTPUTFORMATTER* aFormatter, int aNestLevel, int aControlBits ) const;

    BASE_GRAPHIC* Clone( PART* aOwner ) const
    {
        CIRCLE* n = new CIRCLE( *this );
        n->owner = aOwner;
        return n;
    }
};


class ARC : public BASE_GRAPHIC
{
    friend class PART;
    friend class SWEET_PARSER;

protected:
    POINT       pos;
    STROKE      stroke;
    int         fillType;       // T_none, T_filled, or T_transparent
    int         radius;
    POINT       start;
    POINT       end;

public:
    ARC( PART* aOwner ) :
        BASE_GRAPHIC( aOwner ),
        stroke( STROKE_DEFAULT ),
        fillType( FILL_TYPE_DEFAULT ),
        radius( LogicalToInternal( 0.5 ) )
    {
    }

    void Format( OUTPUTFORMATTER* aFormatter, int aNestLevel, int aControlBits ) const;

    BASE_GRAPHIC* Clone( PART* aOwner ) const
    {
        ARC* n = new ARC( *this );
        n->owner = aOwner;
        return n;
    }
};


class GR_TEXT : public BASE_GRAPHIC
{
    friend class PART;
    friend class SWEET_PARSER;

protected:
    POINT       pos;
    ANGLE       angle;

    int         fillType;       ///< T_none, T_filled, or T_transparent

    int         hjustify;       ///< T_center, T_right, or T_left
    int         vjustify;       ///< T_center, T_top, or T_bottom

    bool        isVisible;
    wxString    text;
    FONT        font;

public:
    GR_TEXT( PART* aOwner ) :
        BASE_GRAPHIC( aOwner ),
        angle( 0 ),
        fillType( PR::T_filled ),
        hjustify( PR::T_left ),
        vjustify( PR::T_bottom ),
        isVisible( true )
    {}

    static const char* ShowJustify( int aJustify )
    {
        return SWEET_LEXER::TokenName( PR::T( aJustify ) );
    }

    void Format( OUTPUTFORMATTER* aFormatter, int aNestLevel, int aControlBits ) const;

    BASE_GRAPHIC* Clone( PART* aOwner ) const
    {
        GR_TEXT* n = new GR_TEXT( *this );
        n->owner = aOwner;
        return n;
    }
};


class PROPERTY : public BASE_GRAPHIC
{
    friend class PART;
    friend class SWEET_PARSER;

protected:
    wxString        name;
    wxString        text;
    TEXT_EFFECTS*   effects;

    void clear()
    {
        delete effects;
        effects = 0;

        name = wxEmptyString;
        text = wxEmptyString;
    }

public:
    PROPERTY( PART* aOwner, const wxChar* aName = wxT( "" ) ) :
        BASE_GRAPHIC( aOwner ),
        name( aName ),
        effects( 0 )
    {}

    PROPERTY( const PROPERTY& r ) :
        BASE_GRAPHIC( NULL ),
        effects( 0 )
    {
        // use assignment operator
        *this = r;
    }

    PROPERTY& operator = ( const PROPERTY& r );     // @todo

    ~PROPERTY()
    {
        clear();
    }

    /**
     * Function Effects
     * returns a pointer to the TEXT_EFFECTS object for this PROPERTY, and optionally
     * will lazily allocate one if it did not exist previously.
     * @param doAlloc if true, means do an allocation of a new TEXT_EFFECTS if one
     * currently does not exist, otherwise return NULL if non-existent.
     */
    TEXT_EFFECTS*   EffectsLookup();
    TEXT_EFFECTS*   Effects() const
    {
        return effects;
    }

    void Format( OUTPUTFORMATTER* aFormatter, int aNestLevel, int aControlBits ) const;

    BASE_GRAPHIC* Clone( PART* aOwner ) const
    {
        PROPERTY* n = new PROPERTY( *this );
        n->owner = aOwner;
        return n;
    }
};


struct PINTEXT
{
    wxString    text;
    FONT        font;
    bool        isVisible;

    PINTEXT() :
        isVisible( true )
    {}

    void Format( OUTPUTFORMATTER* aFormatter, const char* aElement, int aNestLevel, int aControlBits ) const;
};


#define PIN_LEN_DEFAULT     -1          ///< use standard pin length for given type
#define PIN_SHAPE_DEFAULT   PR::T_line  ///< use standard pin shape
#define PIN_CONN_DEFAULT    PR::T_in    ///< use standard pin connection type

class PIN : public BASE_GRAPHIC
{
    friend class PART;
    friend class SWEET_PARSER;

public:
    PIN( PART* aOwner ) :
        BASE_GRAPHIC( aOwner ),
        angle( 0 ),
        connectionType( PIN_CONN_DEFAULT ),
        shape( PIN_SHAPE_DEFAULT ),
        length( PIN_LEN_DEFAULT ),
        isVisible( true )
    {}

    ~PIN();

    const char* ShowType() const
    {
        return SWEET_LEXER::TokenName( PR::T( connectionType ) );
    }

    const char* ShowShape() const
    {
        return SWEET_LEXER::TokenName( PR::T( shape ) );
    }

    void Format( OUTPUTFORMATTER* aFormatter, int aNestLevel, int aControlBits ) const;

    BASE_GRAPHIC* Clone( PART* aOwner ) const
    {
        PIN* n = new PIN( *this );
        n->owner = aOwner;
        return n;
    }

protected:
    POINT       pos;
    ANGLE       angle;

    PINTEXT     pad;
    PINTEXT     signal;

    int         connectionType;     ///< T_in, T_out, T_inout, T_tristate, T_passive, T_unspecified,
                                    ///< T_power_in, T_power_out, T_open_collector, T_open_emitter, or T_unconnected.

    int         shape;              ///< T_none, T_line, T_inverted, T_clock, T_inverted_clk, T_input_low, T_clock_low,
                                    ///< T_falling_edge, T_non_logic.

    int         length;             ///< length of pin in internal units
    bool        isVisible;          ///< pin is visible

    wxString    pin_merge;          ///< pad of (pin_merge ...) that I am a member of, else empty if none
};


/**
 * Class PART_REF
 * is an LPID with a pointer to the "looked up" PART, which is looked up lazily.
 */
class PART_REF : public LPID
{
public:
    PART_REF() :
        LPID(),
        part(0)
    {}

    /**
     * Constructor LPID
     * takes aLPID string and parses it.  A typical LPID string uses a logical
     * library name followed by a part name.
     * e.g.: "kicad:passives/R/rev2", or
     * e.g.: "mylib:R33"
     */
    PART_REF( const STRING& aLPID ) throw( PARSE_ERROR ) :
        LPID( aLPID ),
        part(0)
    {
    }

    /**
     * Function Lookup
     * returns the PART that this LPID refers to.  Never returns NULL, because
     * instead an exception would be thrown.
     * @throw IO_ERROR if any problem occurs or if the part cannot be found.
     */
    PART* Lookup( LIB_TABLE* aLibTable, LIB* aFallBackLib )
    {
        if( !part )
        {
            part = aLibTable->LookupPart( *this, aFallBackLib );
        }
        return part;
    }

protected:
    PART*   part;               ///< The looked-up PART,
                                ///< no ownership (duh, PARTs are always owned by a LIB)
};

typedef std::vector<PART_REF>   PART_REFS;

}  // namespace SCH


//-----</temporary home for PART sub objects, move after stable>-----------------


typedef std::set< wxString >            KEYWORDS;

namespace SCH {

typedef std::vector< BASE_GRAPHIC* >    GRAPHICS;
typedef std::vector< PROPERTY* >        PROPERTIES;

typedef std::vector< PIN* >             PINS;
typedef std::vector< PIN* >             PIN_LIST;       ///< no ownership, used for searches


class LPID;
class SWEET_PARSER;


/**
 * Class PART
 * will have to be unified with what Wayne is doing.  I want a separate copy

 * here until I can get the state management correct.  Since a PART only lives
 * within a cache called a LIB, its constructor is private (only a LIB
 * can instantiate one), and it exists in various states of freshness and
 * completeness relative to the LIB_SOURCE within the LIB.
 */
class PART
{
    friend class LIB;           // is the owner of all PARTS, afterall
    friend class SWEET_PARSER;

public:

    /**
     * Enum PROP_ID
     * is the set of "mandatory" properties within a PART.  These are used by
     * class PART as array indices into PART::mandatory[].
     */
    enum  PROP_ID
    {
        REFERENCE,      ///< reference prefix, a template for instantiation at COMPONENT level
        VALUE,          ///< value, e.g. "3.3K"
        FOOTPRINT,      ///< name of PCB module, e.g. "16DIP300"
        DATASHEET,      ///< URI of datasheet
        MODEL,          ///< spice model name
        END             ///< array sentinel, not a valid index
    };

    virtual ~PART();

    PART& operator = ( const PART& other );

    /**
     * Function Owner
     * returns the LIB* owner of this part.
     */
    LIB* Owner()  { return owner; }

    /**
     * Function Parse
     * translates a Sweet string into a binary form that is represented
     * by the normal fields of this class.  Parse is expected to call Inherit()
     * if this part extends any other.
     *
     * @param aParser is an instance of SWEET_PARSER, rewound at the first line.
     *
     * @param aLibTable is the LIB_TABLE "view" that is in effect for inheritance,
     *  and comes from the big containing SCHEMATIC object.
     */
    void Parse( SWEET_PARSER* aParser, LIB_TABLE* aLibTable ) throw( IO_ERROR, PARSE_ERROR );

    /**
     * Function Format
     * outputs this PART in UTF8 encoded s-expression format to @a aFormatter.
     * @param aFormatter is the output sink to write to.
     * @param aNestLevel is the initial indent level
     * @param aControlBits are bit flags ORed together which control how the output
     *  is done.
     */
    void Format( OUTPUTFORMATTER* aFormatter, int aNestLevel, int aControlBits = 0 ) const;

    /**
     * Function PropDelete
     * deletes the property with aPropertyName if found and returns true, else false
     * if not found.
     */
    bool PropDelete( const wxString& aPropertyName );

    /**
     * Function Field
     * returns a pointer to one of the mandatory properties, or NULL
     * if non-existent.  Use FieldLookup() to potentially allocate it.
     */
    PROPERTY*   Field( PROP_ID aPropertyId ) const
    {
        wxASSERT( unsigned(aPropertyId) < unsigned(END) );
        return mandatory[aPropertyId];
    }

    /**
     * Function FieldLookup
     * returns a pointer to one of the mandatory properties, which is lazily
     * constructed by this function if need be.
     * @param aPropertyId tells which field.
     */
    PROPERTY* FieldLookup( PROP_ID aPropertyId );

    /**
     * Function PinFindByPad
     * finds a PIN based on aPad or returns NULL if not found.
     * @param aPad is the pin to find
     * @return PIN* - the found PIN or NULL if not found.
     */
    PIN* PinFindByPad( const wxString& aPad )
    {
        PINS::iterator it = pinFindByPad( aPad );
        return it != pins.end() ? *it : NULL;
    }

    /**
     * Function PinsFindBySignal
     * fetches all the pins matching aSignal into aResults.
     */
    void PinsFindBySignal( PIN_LIST* aResults, const wxString& aSignal );

    /**
     * Function PinDelete
     * deletes the pin with aPad if found and returns true, else false
     * if not found.
     */
    bool PinDelete( const wxString& aPad );


/*
    void SetValue( const wxString& aValue )
    {
        value = aValue;
    }
    const wxString& GetValue()
    {
        return value;
    }

    void SetFootprint( const wxString& aFootprint )
    {
        footprint = aFootprint;
    }
    const wxString& GetFootprint()
    {
        return footprint;
    }

    void SetModel( const wxString& aModel )
    {
        model = aModel;
    }
    const wxString& GetModel()
    {
        return model;
    }
*/

/*
    void SetBody( const STR_UTF& aSExpression )
    {
        body = aSExpression;
    }
*/


protected:      // not likely to have C++ descendants, but protected none-the-less.

    /// a protected constructor, only a LIB can instantiate a PART.
    PART( LIB* aOwner, const STRING& aPartNameAndRev );

    /**
     * Function destroy
     * clears out this object, deleting everything that this PART owns and
     * initializing values back to a state as if the object was just constructed
     * empty.
     */
    void clear();

    /**
     * Function inherit
     * is a specialized assignment function that copies a specific subset, enough
     * to fulfill the requirements of the Sweet s-expression language.
     */
    void inherit( const PART& aBasePart );

    /**
     * Function propertyFind
     * searches for aPropertyName and returns a PROPERTIES::iterator which
     * is the found item or properties.end() if not found.
     */
    PROPERTIES::iterator propertyFind( const wxString& aPropertyName );

    /**
     * Function pinFindByPad
     * searches for a PIN with aPad and returns a PROPERTIES::iterator which
     * is the found item or pins.end() if not found.
     */
    PINS::iterator pinFindByPad( const wxString& aPad );

    LIB*            owner;      ///< which LIB am I a part of (pun if you want)
    int             contains;   ///< has bits from Enum PartParts

    STRING          partNameAndRev;   ///< example "passives/R[/revN..]", immutable.

    LPID*           extends;    ///< of base part, NULL if none, otherwise I own it.
    const PART*     base;       ///< which PART am I extending, if any.  no ownership.

    POINT           anchor;

    /// encapsulate the old version deletion, take ownership of @a aLPID
    void setExtends( LPID* aLPID );

    /// s-expression text for the part, initially empty, and read in as this part
    /// actually becomes cached in RAM.
    STRING          body;

    /// mandatory properties, aka fields.  Index into mandatory[] is PROP_ID.
    PROPERTY*       mandatory[END];

    /**
     * Member properties
     * holds the non-mandatory properties.
     */
    PROPERTIES      properties;

    /**
     * Member graphics
     * owns : POLY_LINE, RECTANGLE, CIRCLE, ARC, BEZIER, and GR_TEXT objects.
     */
    GRAPHICS        graphics;

    /**
     * Member pins
     * owns all the PINs in pins.
     */
    PINS            pins;

    /// Alternate body forms.
    PART_REFS       alternates;

    ///  searching aids
    KEYWORDS        keywords;

    /**
     * A pin_merge set is a set of pins that are all electrically equivalent
     * and whose anchor pin is the only one visible.  The visible pin is the
     * key in the MERGE_SETS boost::ptr_map::map
     */
    MERGE_SETS      pin_merges;
};

}   // namespace PART

#endif  // SCH_PART_
