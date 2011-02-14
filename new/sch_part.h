
#ifndef SCH_PART_H_
#define SCH_PART_H_

#include <sch_lib.h>


//-----<temporary home for PART sub objects, move after stable>------------------

typedef     wxPoint     POINT;

#include <wx/gdicmn.h>
#include <vector>

namespace SCH {

class PART;
class SWEET_PARSER;


class   BASE_GRAPHIC
{
    friend class PART;
    friend class SWEET_PARSER;

protected:
    PART*   owner;

public:
    BASE_GRAPHIC( PART* aOwner ) :
        owner( aOwner )
    {}

    virtual ~BASE_GRAPHIC() {}
};


class POLY_LINE : BASE_GRAPHIC
{
    friend class PART;
    friend class SWEET_PARSER;

protected:
    double              width;
    std::vector<POINT>  pts;

public:
    POLY_LINE( PART* aOwner ) :
        BASE_GRAPHIC( aOwner )
    {}
};


};


//-----</temporary home for PART sub objects, move after stable>-----------------


namespace SCH {

typedef std::vector< BASE_GRAPHIC* >    GRAPHICS;

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

protected:      // not likely to have C++ descendants, but protected none-the-less.

    /// a protected constructor, only a LIB can instantiate a PART.
    PART( LIB* aOwner, const STRING& aPartNameAndRev );

    /**
     * Function destroy
     * clears out this object, deleting all graphics, all fields, all properties,
     * etc.
     */
    void clear();

    /**
     * Function inherit
     * is a specialized assignment function that copies a specific subset, enough
     * to fulfill the requirements of the Sweet s-expression language.
     */
    void inherit( const PART& aBasePart );

    POINT           anchor;

    //PART( LIB* aOwner );

    LIB*            owner;      ///< which LIB am I a part of (pun if you want)
    int             contains;   ///< has bits from Enum PartParts

    STRING          partNameAndRev;   ///< example "passives/R[/revN..]", immutable.

    LPID*           extends;    ///< of base part, NULL if none, otherwise I own it.
    PART*           base;       ///< which PART am I extending, if any.  no ownership.

    /// encapsulate the old version deletion, take ownership of @a aLPID
    void setExtends( LPID* aLPID );

    /// s-expression text for the part, initially empty, and read in as this part
    /// actually becomes cached in RAM.
    STRING          body;

//    bool        cachedRevisions;    ///< allows lazy loading of revision of this same part name

    // 3 separate lists for speed:

    /// A property list.
    //PROPERTIES    properties;

    /// A drawing list for graphics
    GRAPHICS        graphics;

    /// A pin list
    //PINS        pins;

    /// Alternate body forms.
    //ALTERNATES  alternates;

    // lots of other stuff, like the mandatory properties, but no units, since we went with dimensionless

public:

    virtual ~PART();

    PART& operator=( const PART& other );

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

/*
    void SetBody( const STR_UTF& aSExpression )
    {
        body = aSExpression;
    }
*/
};

}   // namespace PART

#endif  // SCH_PART_
