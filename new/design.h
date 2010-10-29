


// Designer and copyright holder: Dick Hollenbeck <dick@softplc.com>


/** @mainpage

This file describes the design of new C++ classes which may
be used to implement a distributed library system for EESCHEMA, and with
some modification, PCBNEW also.

@author Dick Hollenbeck <dick@softplc.com>

@date   October 2010

@section intr_sec Introduction

This is the introduction.


@section summary Summary

This is the summary.

*/



/**
 * \defgroup STRING Types
 * Provide some string types for use within the API.
 * @{
 */

typedef std::string STRING;

/**
 * Type STRING_TOKS
 * documents a STRING which holds a sequence of s-expressions suitable for parsing
 * with DSNLEXER.  This can either be a sequence of DSN_SYMBOLs or a sequence of
 * fully parenthesis delimited s-expressions.  There are 2 types: <ol>
 * <li> R C R33 "quoted-name" J2
 * <li> (part R ())(part C ())
 * </ol>
 * Notice that in the 1st example, there are 5 tokens in sequence, and in the
 * 2nd example there are two top most s-expressions in sequence.  So the counts
 * in these are 5 and 2 respectively.
 */
typedef STRING      STRING_TOKS;


typedef std::vector< STRING >  STRINGS;


const STRING StrEmpty = "";

/** @} STRING Types */


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
    /// LIB class has great license to modify what's in here, nobody else does.
    /// Modification is done through the LIB so it can track the state of the
    /// PART and take action as needed.  Actually most of the modification will
    /// be done by PARTS_LIST, a class derived from LIB.
    friend class LIB;


    /// a private constructor, only a LIB can instantiate one.
    PART() {}


protected:      // not likely to have descendants, but protected none-the-less.

    LIB*    owner;      ///< which LIB am I a part of (pun if you want)
    STRING      extends;    ///< LPID of base part

    STRING      name;       ///< example "passives/R", immutable.

    /// s-expression text for the part, initially empty, and read in as this part
    /// actually becomes cached in RAM.
    STRING      body;

    // lots of other stuff.


public:

    /**
     * Function Inherit
     * is a specialized assignment function that copies a specific subset, enough
     * to fulfill the requirements of the sweet s-expression language.
     */
    void Inherit( const PART& aBasePart );

};


/**
 * Class LPID (aka GUID)
 * is a Logical Part ID and consists of various portions much like a URI.  It
 * relies heavily on a logical library name to hide where actual physical library
 * sources reside.  Its static functions serve as managers of the library table to
 * map logical library names to actual library sources.
 * <p>
 * Example LPID string:
 * "kicad:passives/R/rev6".
 * <p>
 * <ul>
 * <li> "kicad" is the logical library name.
 * <li> "passives" is the category.
 * <li> "passives/R" is the partname.
 * <li> "rev6" is the revision number, which is optional.  If missing then its
 *      delimiter should also not be present.
 * </ul>
 * <p>
 * This class owns the <b>library table</b>, which is like fstab in concept and maps logical
 * library name to library URI, type, and password. It has the following columns:
 * <ul>
 * <li> Logical Library Name
 * <li> Library Type
 * <li> Library URI
 * <li> Password
 * </ul>
 * <p>
 * For now, the Library Type can be one of:
 * <ul>
 * <li> "dir"
 * <li> "schematic"  i.e. a parts list from another schematic.
 * <li> "subversion"
 * <li> "bazaar"
 * <li> "http"
 * </ul>
 * <p>
 * For now, the Library URI types needed to support the various types can be one of those
 * shown below, which are typical of each type:
 * <ul>
 * <li> "file://C:/mylibdir"
 * <li> "file://home/user/kicadwork/jtagboard.sch"
 * <li> "svn://kicad.org/partlib/trunk"
 * <li> "http://kicad.org/partlib"
 * </ul>
 * <p>
 * The library table is built up from several sources, and is a contatonation
 * of those sources.
 */
class LPID  // aka GUID
{
    /**
     * Constructor LPID
     * takes aLPID string and parses it.  A typical LPID string uses a logical
     * library name followed by a part name.
     * e.g.: "kicad:passives/R/rev2", or
     * e.g.: "me:R33"
     */
    LPID( const STRING& aLPID = StrEmpty ) throw( PARSE_ERROR );

    /**
     * Function GetLogLib
     * returns the logical library portion of a LPID.  There is not Set accessor
     * for this portion since it comes from the library table and is considered
     * read only here.
     */
    STRING  GetLogLib() const;

    /**
     * Function GetCategory
     * returns the category of this part id, "passives" in the example at the
     * top of the class description.
     */
    STRING  GetCategory() const;

    /**
     * Function SetCategory
     * overrides the category portion of the LPID to @a aCategory and is typically
     * either the empty string or a single word like "passives".
     */
    void SetCategory( const STRING& aCategory );

    /**
     * Function GetRevision
     * returns the revision portion of the LPID or StrEmpty if none.
     */
    STRING GetRevision() const;

    /**
     * Function SetRevision
     * overrides the revision portion of the LPID to @a aRevision and must
     * be in the form "rev<num>" where "<num>" is "1", "2", etc.
     */
    void SetRevision( const STRING& aRevision );

    /**
     * Function GetFullText
     * returns the full text of the LPID.
     */
    STRING  GetFullText() const;


    //-----<statics>-----------------------------------------------------

    /**
     * Function GetLogicalLibraries
     * returns the logical library names, all of them that are in the
     * library table.
     */
    static STRINGS GetLogicalLibraries();

    /**
     * Function GetLibraryURI
     * returns the full library path from a logical library name.
     */
    static STRING  GetLibraryURI( const STRING& aLogicalLibraryName ) const;

    /**
     * Function GetLibraryType
     * returns the type of a logical library.
     */
    static STRING GetLibraryType( const STRING& aLogicalLibraryName ) const;

    /**
     * Function GetPassword
     * returns the password for this type of a logical library.
     */
    static STRING GetPassword( const STRING& aLogicalLibraryName ) const;
};


/**
 * Class LIB_SOURCE
 * is an abstract class from which implementation specific LIB_SOURCEs
 * may be derived, one for each kind of library type allowed in the library table.
 * The class name stems from the fact that this interface only provides READ ONLY
 * functions.
 */
class LIB_SOURCE
{
    friend class LIBS;      ///< the LIB factory is LIBS::GetLibrary()
    friend class LIB;       ///< the LIB uses these functions.

protected:                  ///< derived classes must implement

    /*
        NRVO described:
        http://msdn.microsoft.com/en-us/library/ms364057%28VS.80%29.aspx

        Even with NRVO provided by the compilers, I don't see it being as lean as
        having the LIBARY keep an expanded member STRING for the aResult value. So I
        am heading towards passing STRING* aResult and STRINGS* aResults. Rather
        than returning a STRING.  When the pointer to a results buffer is passeed,
        I won't refer to this as returning a value, but rather 'fetching' a result.
    */

    /**
     * Function GetSourceType
     * retuns type library table entry's type for library source.
     */
    const STRING& GetSourceType()  { return sourceType ; }

    /**
     * Function ReadPart
     * fetches @a aPartName's s-expression into @a aResult after clear()ing aResult.
     */
    virtual void ReadPart( STRING* aResult, const STRING& aPartName, const STRING& aRev=StrEmpty ) throw( IO_ERROR ) = 0;

    /**
     * Function ReadParts
     * fetches the s-expressions for each part given in @a aPartNames, into @a aResults,
     * honoring the array indices respectfully.
     * @param aPartNames is a list of part names, one name per vector element.
     * @param aResults receives the s-expressions
     */
    virtual void ReadParts( STRING_TOKS* aResults, const STRINGS& aPartNames ) throw( IO_ERROR ) = 0;

    /**
     * Function GetCategories
     * fetches all categories present in the library source into @a aResults
     */
    virtual void GetCategories( STRING_TOKS* aResults ) throw( IO_ERROR ) = 0;

    /**
     * Function GetCategoricalPartNames
     * fetches all the part names for @a aCategory, which was returned by GetCategories().
     *
     * @param aCategory is a subdividing navigator within the library source, but may default to empty
     *  which will be taken to mean all categories.
     *
     * @param aResults is a place to put the fetched result, one category per STRING.
     */
    virtual void GetCategoricalPartNames( STRING_TOKS* aResults, const STRING& aCategory=StrEmpty ) throw( IO_ERROR ) = 0;

    /**
     * Function GetRevisions
     * fetches all revisions for @a aPartName into @a aResults.  Revisions are strings
     * like "rev12", "rev279", and are library source agnostic.  These
     */
    virtual void GetRevisions( STRING_TOKS* aResults, const STRING& aPartName ) throw( IO_ERROR ) = 0;

    /**
     * Function FindParts
     * fetches part names for all parts matching the criteria given in @a
     * aQuery, into @a aResults.  The query string is designed to be easily marshalled,
     * i.e. serialized, so that long distance queries can be made with minimal overhead.
     * The library source needs to have an intelligent friend on the other end if
     * the actual library data is remotely located, otherwise it will be too slow
     * to honor this portion of the API contract.
     *
     * @param aQuery is a string holding a domain specific language expression.  One candidate
     *  here is an s-expression that uses (and ..) and (or ..) operators. For example
     *  "(and (footprint 0805)(value 33ohm)(category passives))"
     *
     * @param aResults is a place to put the fetched part names, one part per STRING.
     */
    virtual void FindParts( STRING_TOKS* aResults, const STRING& aQuery ) throw( IO_ERROR ) = 0;

protected:
    STRING      sourceType;
    STRING      sourceURI;
};


/**
 * Class DIR_LIB_SOURCE
 * implements a LIB_SOURCE in a file system directory.
 */
class DIR_LIB_SOURCE : public LIB_SOURCE
{
    friend class LIBS;   ///< LIBS::GetLib() can construct one.

protected:

    /**
     * Constructor DIR_LIB_SOURCE( const STRING& aDirectoryPath )
     * sets up a LIB_SOURCE using aDirectoryPath in a file system.
     * @see LIBS::GetLibrary().
     *
     * @param aDirectoryPath is a full pathname of a directory which contains
     *  the library source of part files.  Examples might be "C:\kicad_data\mylib" or
     *  "/home/designer/mylibdir".
     */
    DIR_LIB_SOURCE( const STRING& aDirectoryPath ) throws( IO_ERROR );
};


/**
 * Class SVN_LIB_SOURCE
 * implements a LIB_SOURCE in a file system directory.
 */
class SVN_LIB_SOURCE : public LIB_SOURCE
{
    friend class LIBS;   ///< constructor the LIB uses these functions.

protected:

    /**
     * Constructor SVN_LIB_SOURCE( const STRING& aSvnURL )
     * sets up a LIB_SOURCE using aSvnURI which points to a subversion
     * repository.
     * @see LIBS::GetLibrary().
     *
     * @param aSvnURL is a full URL of a subversion repo directory.  Example might
     *  be "svn://kicad.org/repos/library/trunk"
     */
    SVN_LIB_SOURCE( const STRING& aSvnURL ) throws( IO_ERROR );
};


/**
 * Class PARTS_LIST_LIB_SOURCE
 * implements a LIB_SOURCE in on a schematic file.
 */
class PARTS_LIST_LIB_SOURCE : public LIB_SOURCE
{
    friend class LIBS;   ///< constructor the LIB uses these functions.

protected:

    /**
     * Constructor PARTS_LIST_LIB_SOURCE( const STRING& aSchematicFile )
     * sets up a LIB_SOURCE using aSchematicFile which is a full path and filename
     * for a schematic not related to the schematic being editing in
     * this EESCHEMA session.
     * @see LIBS::GetLibrary().
     *
     * @param aSchematicFile is a full path and filename.  Example:
     *  "/home/user/kicadproject/design.sch"
     */
    PARTS_LIST_LIB_SOURCE( const STRING& aSchematicFile ) throws( IO_ERROR );
};


/**
 * Class LIB_SINK
 * is an abstract class from which implementation specific LIB_SINKs
 * may be derived, one for each kind of library type in the library table that
 * supports writing.  The class name stems from the fact that this interface
 * only provides WRITE functions.
 */
class LIB_SINK
{
    friend class LIB;   ///< only the LIB uses these functions.

protected:                  ///< derived classes must implement

    /**
     * Function WritePart
     * saves the part to non-volatile storage. @a aPartName may have the revision
     * portion present.  If it is not present, and a overwrite of an existhing
     * part is done, then LIB::ReloadPart() must be called on this same part
     * and all parts that inherit it must be reparsed.
     */
    virtual void WritePart( const STRING& aPartName, const STRING& aSExpression ) throw ( IO_ERROR ) = 0;


protected:
    STRING      sinkType;
    STRING      sinkURI;
};


/**
 * Class LIBS
 * houses a handful of functions that manage all the RAM resident LIBs, and
 * provide for a global part lookup function, GetPart(), which can be the basis
 * of a cross LIB hyperlink.
 */
class LIBS
{
public:

    /**
     * Function GetPart
     * finds and loads a PART, and parses it.  As long as the part is
     * accessible in any LIB_SOURCE, opened or not opened, this function
     * will find it and load it into its containing LIB, even if that means
     * having to load a new LIB as given in the library table.
     */
    static PART* GetPart( const LPID& aLogicalPartID ) throw ( IO_ERROR );

    /**
     * Function GetLib
     * is first a lookup function and then if needed, a factory function.
     * If aLogicalLibraryName has been opened, then return the already opened
     * LIB.  If not, then instantiate the library and fill the initial
     * library PARTs (unparsed) and categories, and add it to LIB::libraries
     * for future reference.
     */
    static LIB* GetLib( const STRING& aLogicalLibraryName ) throw( IO_ERROR );

    /**
     * Function GetOpenedLibNames
     * returns the logical library names of LIBs that are already opened.
     * @see LPID::GetLogicalLibraries()
     */
    static STRINGS GetOpendedLogicalLibNames();

    /**
     * Function CloseLibrary
     * closes an open library @a aLibrary and removes it from class LIBS.
     */
    static void CloseLibrary( LIB* aLibrary ) throw( IO_ERROR );


private:

    /// collection of LIBs, searchable by logical name.
    static std::map< STRING, LIB* > libraries;      // owns the LIBs.
};


/**
 * Class LIB
 * is a cache of parts, and because the LIB_SOURCE is abstracted, there
 * should be no need to extend from this class in any case except for the
 * PARTS_LIST.
 */
class LIB
{
    friend class LIBS;      ///< the LIB factory is LIBS::GetLibrary()

protected:  // constructor is not public, called from LIBS only.

    /**
     * Constructor LIB
     * is not public and is only called from LIBS::GetLib()
     *
     * @param aLogicalLibrary is the name of a well know logical library, and is
     *  known because it already exists in the library table.
     *
     * @param aSource is an open LIB_SOURCE whose ownership is
     *          given over to this LIB.
     *
     * @param aSink is an open LIB_SINK whose ownership is given over
     *          to this LIB, and it is normally NULL.
     */
    LIB( const STRING& aLogicalLibrary, LIB_SOURCE* aSource, LIB_SINK* aSink=NULL ) :
        name( aLogicalLibrary ),
        source( aSource ),
        sink( aSink )
    {
    }

    ~LIB()
    {
        delete source;
        delete sink;
    }


public:

    /**
     * Function HasSink
     * returns true if this library has write/save capability.  Most LIBs
     * are read only.
     */
    bool HasSave()  { return sink != NULL; }


    //-----<use delegates: source and sink>---------------------------------

    /**
     * Function GetPart
     * returns a PART given @a aPartName, such as "passives/R".
     * @param aPartName is local to this LIB and does not have the logical
     *  library name prefixed.
     */
    const PART* GetPart( const STRING& aPartName ) throw( IO_ERROR );


    /**
     * Function ReloadPart
     * will reload the part assuming the library source has a changed content
     * for it.
     */
    void ReloadPart( PART* aPart ) throw( IO_ERROR );

    /**
     * Function GetCategories
     * fetches all categories of parts within this LIB into @a aResults.
     */
    STRINGS GetCategories() throw( IO_ERROR ) = 0;

    /**
     * Function GetCategoricalPartNames
     * returns the part names for @a aCategory, and at the same time
     * creates cache entries for the very same parts if they do not already exist
     * in this LIB (i.e. cache).
     */
    STRINGS GetCategoricalPartNames( const STRING& aCategory=StrEmpty ) throw( IO_ERROR ) = 0;

    //-----<.use delegates: source and sink>--------------------------------

    /**
     * Function WritePart
     * saves the part to non-volatile storage. @a aPartName may have the revision
     * portion present.  If it is not present, and a overwrite of an existing
     * part is done, then all parts that inherit it must be reparsed.
     * This is why most library sources are read only.  An exception is the PARTS_LIST,
     * not to be confused with a LIB based on a parts list in another schematic.
     * The PARTS_LIST is in the the schematic being edited and is by definition the
     * last to inherit, so editing in the current schematic's PARTS_LIST should be harmless.
     * There can be some self referential issues that mean all the parts in the PARTS_LIST
     * have to reparsed.
     */
    virtual void WritePart( PART* aPart ) throw ( IO_ERROR ) = 0;

    virtual void SetPartBody( PART* aPart, const STRING& aSExpression ) throw ( IO_ERROR );

    /**
     * Function GetRevisions
     * returns the revisions of @a aPartName that are present in this LIB.
     * The returned STRINGS will look like "rev1", "rev2", etc.
     */
    STRINGS GetRevisions( const STRING& aPartName ) throw( IO_ERROR ) = 0;

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
    STRINGS FindParts( const STRING& aQuery ) throw( IO_ERROR ) = 0
    {
        // run the query on the cached data first for any PARTS which are fully
        // parsed (i.e. cached), then on the LIB_SOURCE to find any that
        // are not fully parsed, then unify the results.
    }

private:

    STRING              fetch;      ///< scratch, used to fetch things, grows to worst case size.

    LIB_SOURCE*         source;
    LIB_SINK*           sink;

    STRING              name;
    STRING              libraryType;
    STRING              libraryURI;

    STRINGS             categories;

    typedef boost::ptr_vector<PART>     PARTS;

    PARTS               parts;

    std::vector<PART*>  orderByName;
};

// EOF
