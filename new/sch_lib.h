/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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

#ifndef SCH_LIB_H_
#define SCH_LIB_H_

#include <utf8.h>
#include <richio.h>
#include <import_export.h>


namespace SCH {

class LPID;
class PART;
class LIB_TABLE;

/**
 * Class LIB_SOURCE
 * is an abstract class from which implementation specific LIB_SOURCEs
 * may be derived, one for each kind of library type allowed in the library table.
 * The class name stems from the fact that this interface only provides READ ONLY
 * functions.
 *
 * @author Dick Hollenbeck
 */
class LIB_SOURCE
{
    friend class LIB;       ///< the LIB uses these functions.

protected:                  ///< derived classes must implement

    /**
     * Function GetSourceType
     * returns the library table entry's type for this library source.
     */
    const STRING& GetSourceType()   { return sourceType; }

    /**
     * Function GetSourceURI
     * returns absolute location of the library source.
     */
    const STRING& GetSourceURI()    { return sourceURI; }

    //-----<abstract for implementors>---------------------------------------

    /**
     * Function ReadPart
     * fetches @a aPartName's s-expression into @a aResult after clear()ing aResult.
     */
    virtual void ReadPart( STR_UTF* aResult, const STRING& aPartName, const STRING& aRev = "" ) = 0;

    /**
     * Function ReadParts
     * fetches the s-expressions for each part given in @a aPartNames, into @a aResults,
     * honoring the array indices respectfully.
     * @param aPartNames is a list of part names, one name per list element.  If a part name
     *        does not have a version string, then the most recent version is fetched.
     * @param aResults receives the s-expressions
     */
    virtual void ReadParts( STR_UTFS* aResults, const STRINGS& aPartNames ) = 0;

    /**
     * Function GetCategories
     * fetches all categories present in the library source into @a aResults
     */
    virtual void GetCategories( STRINGS* aResults ) = 0;

    /**
     * Function GetCategoricalPartNames
     * fetches all the part names for @a aCategory, which was returned by GetCategories().
     *
     * @param aCategory is a subdividing navigator within the library source,
     *  but may default to empty which will be taken to mean all categories.
     *
     * @param aResults is a place to put the fetched result, one category per STRING.
     */
    virtual void GetCategoricalPartNames( STRINGS* aResults, const STRING& aCategory="" ) = 0;

    /**
     * Function GetRevisions
     * fetches all revisions for @a aPartName into @a aResults.  Revisions are strings
     * like "rev12", "rev279", and are library source agnostic.  These do not have to be
     * in a contiguous order, but the first 3 characters must be "rev" and subsequent
     * characters must consist of at least one decimal digit.  If the LIB_SOURCE
     * does not support revisions, it is allowed to return a single "" string as
     * the only result.  This means aPartName is present in the libsource, only once
     * without a revision.  This is a special case.
     */
    virtual void GetRevisions( STRINGS* aResults, const STRING& aPartName ) = 0;

    /**
     * Function FindParts
     * fetches part names for all parts matching the criteria given in @a
     * aQuery, into @a aResults.  The query string is designed to be easily marshalled,
     * i.e. serialized, so that long distance queries can be made with minimal overhead.
     * The library source needs to have an intelligent friend on the other end if
     * the actual library data is remotely located, otherwise it will be too slow
     * to honor this portion of the API contract.
     *
     * @param aQuery is a string holding a domain specific query language expression.
     *  One candidate here is an s-expression that uses (and ..) and (or ..) operators
     *  and uses them as RPN. For example "(and (footprint 0805)(value 33ohm)(category passives))".
     *  The UI can shield the user from this if it wants.
     *
     * @param aResults is a place to put the fetched part names, one part per STRING.
     */
    virtual void FindParts( STRINGS* aResults, const STRING& aQuery ) = 0;

    //-----</abstract for implementors>--------------------------------------


protected:
    STRING      sourceType;
    STRING      sourceURI;
};


/**
 * Class LIB_SINK
 * is an abstract class from which implementation specific LIB_SINKs
 * may be derived, one for each kind of library type in the library table that
 * supports writing.  The class name stems from the fact that this interface
 * only provides WRITE functions.
 *
 * @author Dick Hollenbeck
 */
class LIB_SINK
{
    friend class LIB;       ///< only the LIB uses these functions.

protected:                  ///< derived classes must implement

    /**
     * Function GetSinkType
     * returns the library table entry's type for this library sink.
     */
    const STRING& GetSinkType()   { return sinkType; }

    /**
     * Function GetSinkURI
     * returns absolute location of the library sink.
     */
    const STRING& GetSinkURI()    { return sinkURI; }

    /**
     * Function WritePart
     * saves the part to non-volatile storage. @a aPartName may have the revision
     * portion present.  If it is not present, and a overwrite of an existhing
     * part is done, then LIB::ReloadPart() must be called on this same part
     * and all parts that inherit it must be reparsed.
     * @return STRING - if the LIB_SINK support revision numbering, then return a
     *   revision name that was next in the sequence, e.g. "rev22", else "".
     */
    virtual STRING WritePart( const STRING& aPartName, const STRING& aSExpression ) = 0;

protected:
    STRING      sinkType;
    STRING      sinkURI;
};


class PARTS;


/**
 * Class LIB
 * is a cache of parts, and because the LIB_SOURCE is abstracted, there
 * should be no need to extend from this class in any case except for the
 * PARTS_LIST.
 *
 * @author Dick Hollenbeck
 */
class MY_API LIB
{
    friend class LIB_TABLE;    ///< protected constructor, LIB_TABLE may construct

protected:  // constructor is not public, called from LIB_TABLE only.

    /**
     * Constructor LIB
     * is not public and is only called from class LIB_TABLE
     *
     * @param aLogicalLibrary is the name of a well known logical library, and is
     *  known because it already exists in the library table.
     *
     * @param aSource is an open LIB_SOURCE whose ownership is
     *          given over to this LIB.
     *
     * @param aSink is an open LIB_SINK whose ownership is given over
     *          to this LIB, and it is normally NULL.
     */
    LIB( const STRING& aLogicalLibrary, LIB_SOURCE* aSource, LIB_SINK* aSink = NULL );

public:

    ~LIB();

    /**
     * Function HasSink
     * returns true if this library has write/save capability.  Most LIBs
     * are read only.
     */
    bool HasSink()  { return sink != NULL; }

    /**
     * Function LogicalName
     * returns the logical name of this LIB.
     */
    STRING LogicalName();

    //-----<use delegates: source and sink>---------------------------------

    /**
     * Function LookupPart
     * returns a PART given @a aPartName, such as "passives/R".  No ownership
     * is given to the PART, it stays in the cache that is this LIB.
     *
     * @param aLPID is the part to lookup.  The logicalLibName can be empty in it
     *    since yes, we know which LIB is in play.
     *
     * @param aLibTable is the LIB_TABLE view that is in effect for inheritance,
     *  and comes from the big containing SCHEMATIC object.
     *
     * @return PART* - The desired PART and will never be NULL.  No ownership is
     *  given to caller.  PARTs always reside in the cache that is a LIB.
     *
     * @throw IO_ERROR if the part cannot be found or loaded.
     */
    PART* LookupPart( const LPID& aLPID, LIB_TABLE* aLibTable );

    /**
     * Function ReloadPart
     * will reload the part assuming the library source has a changed content
     * for it.
     */
    void ReloadPart( PART* aPart );

    /**
     * Function GetCategories
     * returns all categories of parts within this LIB into @a aResults.
     */
    STRINGS GetCategories();

    /**
     * Function GetCategoricalPartNames
     * returns the part names for @a aCategory, and at the same time
     * creates cache entries for the very same parts if they do not already exist
     * in this LIB (i.e. cache).
     */
    STRINGS GetCategoricalPartNames( const STRING& aCategory = "" );


    //-----<.use delegates: source and sink>--------------------------------

    /**
     * Function WritePart
     * saves the part to non-volatile storage and returns the next new revision
     * name in the sequence established by the LIB_SINK.
     */
    STRING WritePart( PART* aPart );

    void SetPartBody( PART* aPart, const STRING& aSExpression );

    /**
     * Function GetRevisions
     * returns the revisions of @a aPartName that are present in this LIB.
     * The returned STRINGS will look like "rev1", "rev2", etc.
     */
    STRINGS GetRevisions( const STRING& aPartName );

    /**
     * Function FindParts
     * returns part names for all parts matching the criteria given in @a
     * aQuery, into @a aResults.  The query string is designed to be easily marshalled,
     * i.e. serialized, so that long distance queries can be made with minimal overhead.
     * The library source needs to have an intelligent friend on the other end if
     * the actual library data is remotely located, otherwise it will be too slow
     * to honor this portion of the API contract.
     *
     * @param aQuery is a string holding a domain specific language expression.  One candidate
     *  here is an RPN s-expression that uses (and ..) and (or ..) operators. For example
     *  "(and (footprint 0805)(value 33ohm)(category passives))"
     */
    STRINGS FindParts( const STRING& aQuery )
    {
        // run the query on the cached data first for any PARTS which are fully
        // parsed (i.e. cached), then on the LIB_SOURCE to find any that
        // are not fully parsed, then unify the results.

        return STRINGS();
    }

#if defined(DEBUG)
    static void Test( int argc, char** argv );
#endif

protected:

    STR_UTF             fetch;          // scratch, used to fetch things, grows to worst case size.
    STR_UTFS            vfetch;         // scratch, used to fetch things.

    STRING              logicalName;
    LIB_SOURCE*         source;
    LIB_SINK*           sink;

    STRINGS             categories;
    bool                cachedCategories;   /// < is true only after reading categories


    /** parts are in various states of readiness:
     *  1) not even loaded (if cachedParts is false)
     *  2) present, but without member 'body' having been read() yet.
     *  3) body has been read, but not parsed yet.
     *  4) parsed and inheritance if any has been applied.
     */
    PARTS*              parts;

    /**
     * Function lookupPart
     * looks up a PART, returns NULL if cannot find in source.  Does not parse
     * the part.  Does not even load the part's Sweet string.   No ownership
     * is given to the PART, it stays in the cache that is this LIB.
     *
     * @throw IO_ERROR if there is some kind of communications error reading
     *  the original list of parts.
     *
     * @return PART* - the cached PART, or NULL if not found.  No ownership transferred.
     */
    const PART* lookupPart( const LPID& aLPID );
};


}   // namespace SCH

#endif  // SCH_LIB_H_
