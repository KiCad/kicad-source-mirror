/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2010 Kicad Developers, see change_log.txt for contributors.
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

class SWEET_LEXER;
class PART_PARSER;


namespace SCH {

class LPID;

/**
 * Enum PartBit
 * is a set of bit positions that can be used to create flag bits within
 * PART::contains to indicate what state the PART is in and what it contains, i.e.
 * whether the PART has been parsed, and what the PART contains, categorically.
 */
enum PartBit
{
    PARSED,     ///< have parsed this part already, otherwise 'body' text must be parsed
    EXTENDS,    ///< saw "extends" keyword, inheriting from another PART
    VALUE,
    ANCHOR,
    REFERENCE,
    FOOTPRINT,
    DATASHEET,
    MODEL,
    KEYWORDS,
};


/// Function PB
/// is a PartBit shifter for PART::contains field.
static inline const int PB( PartBit oneBitOnly )
{
    return ( 1 << oneBitOnly );
}


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
    friend class ::PART_PARSER;

protected:      // not likely to have C++ descendants, but protected none-the-less.

    /// a protected constructor, only a LIB can instantiate a PART.
    PART( LIB* aOwner, const STRING& aPartNameAndRev );

    /**
     * Function inherit
     * is a specialized assignment function that copies a specific subset, enough
     * to fulfill the requirements of the Sweet s-expression language.
     */
    void inherit( const PART& aBasePart );


    //PART( LIB* aOwner );

    LIB*        owner;      ///< which LIB am I a part of (pun if you want)
    int         contains;   ///< has bits from Enum PartParts

    STRING      partNameAndRev;   ///< example "passives/R[/revN..]", immutable.

    LPID*       extends;    ///< of base part, NULL if none, otherwise I own it.
    PART*       base;       ///< which PART am I extending, if any.  no ownership.

    /// encapsulate the old version deletion, take ownership of @a aLPID
    void setExtends( LPID* aLPID );

    /// s-expression text for the part, initially empty, and read in as this part
    /// actually becomes cached in RAM.
    STRING      body;

//    bool        cachedRevisions;    ///< allows lazy loading of revision of this same part name

    // 3 separate lists for speed:

    /// A property list.
    //PROPERTIES  properties;

    /// A drawing list for graphics
    //DRAWINGS    drawings;

    /// A pin list
    //PINS        pins;

    /// Alternate body forms.
    //ALTERNATES  alternates;

    // lots of other stuff, like the mandatory properties, but no units, since we went with dimensionless

public:

    ~PART();

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
     * @param aLexer is an instance of SWEET_LEXER, rewound at the first line.
     *
     * @param aLibTable is the LIB_TABLE view that is in effect for inheritance,
     *  and comes from the big containing SCHEMATIC object.
     */
    void Parse( SWEET_LEXER* aLexer, LIB_TABLE* aTable ) throw( IO_ERROR );

/*
    void SetBody( const STR_UTF& aSExpression )
    {
        body = aSExpression;
    }
*/
};

}   // namespace PART

#endif  // SCH_PART_
