

// This file describes the early phases of some new classes which may
// eventually be used to implement a distributed library system.

// Designer and copyright holder: Dick Hollenbeck <dick@softplc.com>


typedef std::string STRING;
typedef std::vector< STRING >  STRINGS;


const STRING StrEmpty = "";


/**
 * Class PART
 * will have to be unified with what Wayne is doing.  I want a separate copy
 * here until I can get the state management correct.  Since a PART only lives
 * within a cache called a LIBRARY, its constructor is private (only a LIBRARY
 * can instantiate one), and it exists in various states of freshness and
 * completeness relative to the LIBRARY_SOURCE within the LIBRARY.
 */
class PART
{
    /// LIBRARY class has great license to modify what's in here, nobody else does.
    /// Modification is done through the LIBRARY so it can track the state of the
    /// PART and take action as needed.  Actually most of the modification will
    /// be done by PARTS_LIST, a class derived from LIBRARY.
    friend class LIBRARY;


    /// a private constructor, only a LIBRARY can instantiate one.
    PART() {}


protected:      // not likely to have descendants, but protected none-the-less.

    LIBRARY*    owner;      ///< which LIBRARY am I a part of (pun if you want)
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
 * <p>
 * For now, the Library URI types needed to support the various types can be one of those
 * shown below, which are typical of each type:
 * <ul>
 * <li> "file://C:/mylibdir"
 * <li> "file://home/user/kicadwork/jtagboard.sch"
 * <li> "svn://kicad.org/partlib/trunk"
 * <li> "http://kicad.org/partlib"
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
 * Class LIBRARY_SOURCE
 * is an abstract class from which implementation specific LIBRARY_SOURCEs
 * may be derived, one for each kind of library type allowed in the library table.
 * The class name stems from the fact that this interface only provides READ ONLY
 * functions.
 */
class LIBRARY_SOURCE
{
    friend class LIBRARY;   ///< only the LIBRARY uses these functions.

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
     */
    virtual void ReadParts( STRINGS* aResults, const STRINGS& aPartNames ) throw( IO_ERROR ) = 0;

    /**
     * Function GetCategories
     * fetches all categories present in the library source into @a aResults
     */
    virtual void GetCategories( STRINGS* aResults ) throw( IO_ERROR ) = 0;

    /**
     * Function GetCategoricalPartNames
     * fetches all the part names for @a aCategory, which was returned by GetCategories().
     *
     * @param aCategory is a subdividing navigator within the library source, but may default to empty
     *  which will be taken to mean all categories.
     */
    virtual void GetCategoricalPartNames( STRINGS* aResults, const STRING& aCategory=StrEmpty ) throw( IO_ERROR ) = 0;

    /**
     * Function GetRevisions
     * fetches all revisions for @a aPartName into @a aResults.  Revisions are strings
     * like "rev12", "rev279", and are library source agnostic.  These
     */
    virtual void GetRevisions( STRINGS* aResults, const STRING& aPartName ) throw( IO_ERROR ) = 0;

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
     */
    virtual void FindParts( STRINGS* aResults, const STRING& aQuery ) throw( IO_ERROR ) = 0;

protected:
    STRING      sourceType;
    STRING      sourceURI;
};


/**
 * Class LIBRARY_SINK
 * is an abstract class from which implementation specific LIBRARY_SINKs
 * may be derived, one for each kind of library type in the library table that
 * supports writing.  The class name stems from the fact that this interface
 * only provides WRITE functions.
 */
class LIBRARY_SINK
{
    friend class LIBRARY;   ///< only the LIBRARY uses these functions.

protected:                  ///< derived classes must implement

    /**
     * Function WritePart
     * saves the part to non-volatile storage. @a aPartName may have the revision
     * portion present.  If it is not present, and a overwrite of an existhing
     * part is done, then LIBRARY::ReloadPart() must be called on this same part
     * and all parts that inherit it must be reparsed.
     */
    virtual void WritePart( const STRING& aPartName, const STRING& aSExpression ) throw ( IO_ERROR ) = 0;


protected:
    STRING      sinkType;
    STRING      sinkURI;
};


/**
 * Class LIBS
 * houses a handful of functions that manage all the RAM resident LIBRARYs, and
 * provide for a global part lookup function, GetPart(), which can be the basis
 * of cross LIBRARY hyperlink.
 */
class LIBS
{
    /**
     * Function GetPart
     * finds and loads a PART, and parses it.  As long as the part is
     * accessible in any LIBRARY_SOURCE, opened or not opened, this function
     * will find it and load it into its containing LIBRARY, even if that means
     * having to load a new LIBRARY as given in the library table.
     */
    static PART* GetPart( const LPID& aLogicalPartID ) throw ( IO_ERROR );

    /**
     * Function GetLIBRARY
     * is first a lookup function and then if needed, a factory function.
     * If aLogicalLibraryName has been opened, then return the already opened
     * LIBRARY.  If not, then instantiate the library and fill the initial
     * library PARTs (unparsed) and categories, and add it to LIB::libraries
     * for future reference.
     */
    static LIBRARY* GetLibrary( const STRING& aLogicalLibraryName ) throw( IO_ERROR );

    /**
     * Function GetOpenedLibraryNames
     * returns the logical library names of LIBRARYs that are already opened.
     * @see LPID::GetLogicalLibraries()
     */
    static STRINGS GetOpendedLogicalLibraryNames();

    /**
     * Function CloseLibrary
     * closes an open library @a aLibrary and removes it from LIBS::libraries.
     */
    static void CloseLibrary( LIBRARY* aLibrary ) throw( IO_ERROR );


private:

    /// collection of LIBRARYs, searchable by logical name.
    static std::map< STRING, LIBRARY* > libraries;      // owns the LIBRARYs.
};


/**
 * Class LIBRARY
 * is a cache of parts, and because the LIBRARY_SOURCE is abstracted, there
 * should be no need to extend from this class in any case except for the
 * PARTS_LIST.
 */
class LIBRARY
{
    friend class LIBS;      ///< the LIBRARY factory is LIBS::GetLibrary()

protected:  // constructor is not public, called from LIBS only.

    /**
     * Constructor LIBRARY
     * is not public and is only called from LIBS::GetLibrary()
     *
     * @param aLogicalLibrary is the name of a well know logical library, and is
     *  known because it already exists in the library table.
     *
     * @param aLibrarySource is an open LIBRARY_SOURCE whose ownership is
     *          given over to this LIBRARY.
     *
     * @param aLibrarySink is an open LIBRARY_SINK whose ownership is given over
     *          to this LIBRARY, and it is normally NULL.
     */
     LIBRARY( const STRING& aLogicalLibrary, LIBRARY_SOURCE* aSource, LIBRARY_SINK* aSink ) :
        name( aLogicalLibrary ),
        source( aSource ),
        sink( aSink )
    {
    }

    ~LIBRARY()
    {
        delete source;
        delete sink;
    }


public:

    /**
     * Function HasSink
     * returns true if this library has write/save capability.  Most LIBARARYs
     * are read only, and all remote ones are.
     */
    bool HasSave()  { return sink != NULL; }


    //-----<use delegates: source and sink>---------------------------------

    /**
     * Function GetPart
     * returns a PART given @a aPartName, such as "passives/R".
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
     * fetches all categories of parts within this LIBRARY into @a aResults.
     */
    void GetCategories( STRINGS* aResults ) throw( IO_ERROR ) = 0;

    /**
     * Function GetCategoricalPartName
     * fetches the part names for @a aCategory into @a aResults, and at the same time
     * creates cache entries for the very same parts if they do not already exist
     * in this LIBRARY cache.
     */
    void GetCategoricalPartNames( STRINGS* aResults, const STRING& aCategory=StrEmpty ) throw( IO_ERROR ) = 0;

    //-----<.use delegates: source and sink>--------------------------------

    /**
     * Function WritePart
     * saves the part to non-volatile storage. @a aPartName may have the revision
     * portion present.  If it is not present, and a overwrite of an existing
     * part is done, then all parts that inherit it must be reparsed.
     * This is why most library sources are read only.  An exception is the PARTS_LIST,
     * not to be confused with a LIBRARY based on a parts list in another schematic.
     * The PARTS_LIST is in the the schematic being edited and is by definition the
     * last to inherit, so editing in the current schematic's PARTS_LIST should be harmless.
     * There can be some self referential issues that mean all the parts in the PARTS_LIST
     * have to reparsed.
     */
    virtual void WritePart( PART* aPart ) throw ( IO_ERROR ) = 0;

    virtual void SetPartBody( PART* aPart, const STRING& aSExpression ) throw ( IO_ERROR );

    /**
     * Function GetRevisions
     * returns the revisions of @a aPartName that are present in this LIBRARY.
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
     *  here is an s-expression that uses (and ..) and (or ..) operators. For example
     *  "(and (footprint 0805)(value 33ohm)(category passives))"
     */
    STRINGS FindParts( const STRING& aQuery ) throw( IO_ERROR ) = 0
    {
        // run the query on the cached data first for any PARTS which are fully
        // parsed (i.e. cached), then on the LIBRARY_SOURCE to find any that
        // are not fully parsed, then unify the results.
    }

private:

    STRING              fetched;    ///< scratch, used to fetch things, grows to worst case size.

    LIBARARY_SOURCE*    source;
    LIBRARY_SINK*       sink;

    STRING              name;
    STRING              libraryType;
    STRING              libraryURI;

    STRINGS             categories;

    typedef std::map<STRING, PART*> PARTS;

    PARTS               parts;
};

// EOF
