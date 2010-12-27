
namespace SCH {

/** @mainpage

This file describes the design of a new Distributed Library System for Kicad's
EESCHEMA. Many of the concepts can be adapted with modest modification to PCBNEW
also, in the future.

@author Dick Hollenbeck <dick@softplc.com>

@date   October-December 2010

@section intr_sec Introduction

Schematic <b>parts</b> are frequently needed to complete a circuit design
schematic. Computer data entry of parts can be a rate limiting step in the
design of an overall PCB. Having ready made access to all needed parts in a
design significantly improves the productivity of a circuit designer. Sharing
parts within an organization is one step in the right direction, but there is
opportunity to share across organizational boundaries to improve productivity
even more. Using a part that someone else in another organization has already
entered into the computer can eliminate the first data input process for that
part. The more complicated the part and the board, the larger the positive
impact on productivity because the larger the time savings.

<p> Sharing parts within an organization is best done by directly accessing a
known internal source for those parts, say on a company network. Sharing parts
across organizational boundaries is best done using the Internet in real-time.
Having the ability to search for a part based on arbitrary search criteria can
speed up the pace at which new parts are found and used.

<p> Electronic component manufacturers need and look for ways to differentiate
their products from their competitors. With this Distributed Library System
facility in Kicad, one way for manufacturers to differentiate themselves and
their parts is to publish a part library on the Internet and save their
customers the work of doing the data entry of the part into the Kicad design
system.

<p> Maintaining a comprehensive part library is a fairly labor intensive
activity. New parts come into the market everyday. By being able to publish a
superior library on the Internet, it may be possible to make a for profit
business out of doing this.  The Kicad eco-system would benefit should this
happen, and there could even be competition between such businesses.  Or there
can be library specializations or niches.

<p> Often a found part is close to what is needed but not exactly what is
needed. This Distributed Library System design incorporates the concept of part
inheritance using a part description language called <b>Sweet</b>. Sweet is
based on s-expression syntax. Inheritance is the ability to incrementally change
an existing part without completely re-designing it. It is sometimes easier to
modify an existing part than it is to create the new part entirely from scratch.

<p> This Distributed Library System design will have the capability to
significantly benefit the Kicad eco-system, and that should mean expanding the
numbers of users and contributors to the project, and hopefully making for a
better Kicad tool-set for all.


@section definitions Definitions

Only new terms or changes in the definition of terms are given here.

<dl>

<dt>S-Expression</dt><dd>This is a syntactical textual envelop in the same vain as
XML. It may be used to express any number of domain specific grammars. It uses
parentheses to indicate the start and end of an element. A domain specific
grammar is a set of rules that dictate what keywords may be used, and in what
context. A grammar also establishes the allowed places and types of constants.
There can be any number of grammars which all use s-expressions to hold the
individual elements within the grammar. A grammar is at a higher level than
s-expressions, in the same way that a sentence will have grammatical rules which
are at a higher level than the rules used to spell words. Technically, grammars
nest within grammars. So once you are inside a grammatical element, it will have
its own set of rules as to which nested elements it may hold, and once you enter
one of those nested elements, then that nested element's grammar pertains,
etc.<p> In the case of the grammar for a part, the grammar itself is being given
the name Sweet. The name does not extend to the grammar for the schematic,
only the part grammar.</dd>

<dt>Schematic</dt><dd>This consists of one or more sheets and will be different
in three ways from existing schematics. <ul>

    <li>All sheets will be in one file, thus the entire schematic is in one file.

    <li>The schematic file will have its own s-expression grammar.

    <li> There will be a <b>parts list</b> within the schematic, and within the
    parts list will be <b>all</b> the parts for the schematic. yes <b>all</b> of
    them.  See class PARTS_LIST.</ul>

Within the sheets of the schematic will be components.</dd>

<dt>Component</dt><dd>A component is an instantiated part. The keyword for
component is (comp). A component does not have any of its own properties other
than: <ul> <li>rerence designator <li>part pointer or reference into the parts
list <li>location <li>rotation <li>stuff i.e. DNS or do stuff the part
<li>visual textual effect overrides </ul> Note that the (comp) may not have any
properties or fields of its own, and that it may not exist without a
corresponding part in the parts_list. A reason for this is to ensure that a
BOM can be made simply from the parts_list.</dd>

<dt>Component, again for good measure.</dt><dd>A component is an instantiation
of a part. A component exists within a schematic which has a parts list
containing the part from which the component is instantiated. A component has a
unique reference designator, part ref, its own location, orientation,
stuff/DNS, and text attributes but <b>not</b> its own text fields/strings (other
than reference designator). The part which is instantiated must exist in the
parts list of the same schematic.</dd>

<dt>Inheritance</dt><dd>Is the ability to mimic form and function from another
entity. In our case we use it only for parts. One part may "inherit from" or
"extend" another single part.</dd>

<dt>Part</dt><dd>A part is a symbolic schematic circuit element found within an
EESCHEMA library (or within a parts list). It is re-usable and may be
instantiated more than once within a schematic. For it to be instantiated, it
must be copied or inherited into the parts list of the instantiating schematic.
If inherited into the parts list, then only a concise reference is needed into
the originating library. If instead it is copied into the parts list, then the
part is fully autonomous and need have no reference to its original copy.</dd>

<dt>Parts List</dt><dd>A parts list, keyword (parts_list), is an entirely new
construct. It exists within a schematic and is the complete set of parts used
within a particular schematic.  Each schematic has exactly one parts list
contained within it. A parts list is also a library source and a library sink
for the current schematic. A parts list in any schematic may also be a library
source for any other schematic, but not a library sink. The parts list construct
makes it almost wholly unnecessary to write to other types of library
sinks.</dd>

<dt>Library</dt><dd>A library is no longer a file. It is a memory cache of
parts, consistent with the normal definition of memory cache. Each library is
backed up with a <b>library source</b>. In rare cases, some libraries may also
have a <b>library sink</b>.</dd>

<dt>Library Source</dt><dd>A library source is an abstract read only repository
of parts. The repository itself might exist on the moon. The difference between
a library source and a library sink is that a source is a readable entity.</dd>

<dt>Library Sink</dt><dd>A library sink is an abstract place that parts can be
written to for future reading. The difference between a library source and a
library sink is that a library sink is a writable entity.</dd>

<dt>Symbol</dt><dd>The term "symbol" is not used in a specific way in this
document. There is no symbol in any of the grammars, so use of it on the
developers list will not be understood without explanation. Of course it is
possible to have multiple parts all extend a common base part, and you can think
of the base part as having most if not all the graphical lines for any
derivatives. But we do not need to use the term symbol to describe that
relationship, the term "part" is sufficient.</dd>

<dt>LPID</dt><dd>This stand for "Logical Part ID", and is a reference to any
part within any known library. The term "logical" is used because the contained
library name is logical, not a full library name. The LPID consists of 3 main
portions: logical library name, part name, and revision number.</dd>

<dt>Library Table</dt><dd>This is a lookup table that maps a logical library
name (i.e. a short name) into a fully specified library name and library type.
An applicable library table consists of rows from (at least) two sources:<ol>
<li>A schematic resident library table.
<li>A personal library table.
</ol>

These rows from the two sources are conceptually concatonated (although they may
not be physically concatonated in the implementation, TBD). The schematic
resident rows take presedence over the personal library table if there are
logical library names duplicately defined. (Or we will simply ask that any remote
(i.e. public) libraries use uppercase first letters in logical names, TBD.)

<p> Eventually there will be an external publicly available internet based
logical library table also, but this will need to be glued down at a hard coded
URL that we have control over. The internet based library table allows us to
advertise remote libraries without having to issue an update to Kicad.</dd>

<dt>Query Language</dt><dd>This is a means of searching for something that is
contained within a container. Since some library sources are remote, it is
important to be able to ask the library source for a part that matches some
criteria, for performance reasons.</dd>

</dl>



@section changes Required Changes

In order fulfill the vision embodied by this Distributed Library System design,
it will be necessary to change many APIs and file formats within EESCHEMA. In
fact, the entire schematic file format will be new, based on s-expressions,
the schematic grammar, and the Sweet language for parts.

Here are some of the changes required: <ul>

<li> All sheets which make up a schematic will go into a single s-expression
file. The multiple sheet support will still exist, but all the sheets for a
single schematic are all in a single file.

<li> A "library" is a collection of "parts". The unit of retrieval from a
library is a part as a textual string in the Sweet language. Sweet is a
particular "grammar" expressed in s-expression form, and can be used to fully
describe parts. Because EESCHEMA does not actually see a "library file",
(remember, EESCHEMA can only ask for a part), the actual file format for a
library is no longer pertinent nor visible to the core of EESCHEMA. The unit of
retrieval from the API is the part, so EESCHEMA gets an entire part s-expression
and must then parse it as a RAM resident Sweet string.

<li>EESCHEMA knows of no library files, instead there is a library API which
abstracts the actual part storage strategy used within any library
implementation. The API can be implemented by anyone wanting to provide a
library under a given storage strategy. The API provides a storage strategy
abstraction in classes LIB_SOURCE and LIB_SINK. The actual storage strategy used
in any particular library implementation is not technically part of the
conceptual core of EESCHEMA. This is an important concept to grasp. Eventually
the library implementations may be jetisoned into a plug-in structure, but
initially they are statically linked into EESCHEMA. Should the plug-in strategy
ever get done, the boundary of the plug-in interface will remain the C++ library
API as given here (mostly in class LIB_SOURCE). The only reason to introduce a
plug-in design is to allow proprietary closed source library implementations,
and this could eventually come about if a part vendor wanted to provide one for
the Kicad project. If a Texas Instruments type of company wants to maintain a
Kicad library, we will be positioned to accommodate them. Until then, the
LIB_SOURCE implementations can be statically linked into EESCHEMA and there is
no conceptual disruption either way.

<li> Most library implementations are read only. There are only two library
types that are writable, the "dir" type, and the "parts list". All other types
are read only from the perspective of the API. Stuffing those read only
libraries and maintaining them will be done using the normal update mechanisms
pertinent to the library's respective type of repository. The most common place
to do incremental enhancements to a part before using it is not in the external
library, but now in the parts list with this new design.

<li> The design will support classical clipboard usage. The part in the Sweet
language can be placed onto the clipboard for use by other applications and
instances of EESCHEMA. Eventually larger blocks of components may also be
supported on the clipboard, since the Sweet language allows these blocks to be
descributed textually in a very readable fashion. (Clipboard support beyond part
manipulation is not currently in this revision of the design however, it can be
a future separate enhancement. Perhaps someday complete sheets may be passed
through the clipboard.)

<li> The cumulative set of required changes are significant, and are tantamount
to saying that EESCHEMA will need its part handling foundations re-written. A
conversion program will convert everything over to the new architecture. The
conversion program can simplify things by simply putting all schematic parts
into a parts list within each schematic.

<li> An Internet connection is required to use some of the library sources. It
will be possible to omit these library sources and run Kicad by doing a
configuration change. Eventually, some library sources will spring up and will
not technically be part of the Kicad project, so they will remain remote, but
fully usable to those with an internet connection and permission from the
library source's owner.

<li>By far, even as radical as the distributed library concept is, complete with
remote access, the most significant conceptual change is the introduction of the
<b>parts list</b>. This is a special library that exists in a schematic, and is
the complete record of all parts used within that same schematic. It is
impossible to put a component into a schematic without that component's part
first existing within the parts list of that schematic.

<li> Because of inheritance, multi-body-form parts, alternate body styles, see
also references, and other needs, it is necessary to have a <b>part reference
mechanism</b>. A component has to reference a part, the one it "is". A part has
to be able to reference another part, either in the same library or elsewhere.
Enter the Logical Part ID, or LPID to serve this need. An LPID consists of a
logical library name, a part name, and an optional revision. It is used to
reference parts, from anywhere. If the reference is from a sheet's component,
then the logical library name of the LPID is not needed. Why? Well if you've
been paying attention you know. A comp can only be based on a part that exists
within the parts_list of the same schematic in which it resides. Likewise, a
part within any library that references another part in that <b>same</b> library
will also omit the logical library name from the LPID, and it must omit it. Why?
Well because it makes renaming the library easier, for one. Two, the logical
library name is only a lookup key into a "library table". The library table maps
the logical library name into an actual library source [and sink, iff writable].
See LIB_SOURCE and LIB_SINK.

<p> In the case of the component referencing the part that it "is", there is no
revision number allowed in the LPID. This is because that reference is to the
part in the parts list, and the parts list only holds a single revision of any
part (where "revision" is what you understand from version control systems).

<li> There needs to be a new query language designed and each library source
needs to support it. See LIB_SOURCE::FindParts() for a brief description of one.

</ul>


@section philosophy Design Philosophies

<p> Class names are chosen to be as concise as possible. Separate namespaces can be
used should these same class names be needed in both EESCHEMA and PCBNEW (later).
However, this design does not yet address PCBNEW.  Suggested namespaces are
SCH for EESCHEMA, and PCB for PCBNEW.

<p> Since most if not all the APIs deal with file or non-volatile storage, only
8 bit string types are used. For international strings, UTF-8 is used, and
that is what is currently in use within the Kicad file storage formats.

<p> The typedef <b>STRINGS</b> is used frequently as a holder for multiple
std::strings. After some research, I chose std::dequeue<STRING> to hold a list of
STRINGs. I thought it best when considering overall speed, memory
fragmentation, memory efficiency, and speed of insertion and expansion.

<p> A part description language is introduced called <b>(Sweet)</b>. It supports
inheritance and its syntax is based on s-expressions.

<p> Since a part can be based on another part using inheritance, it is important
to understand the idea of library dependencies. A part in one library can be
dependent on another part in another library, or on another part in the same
library as itself. There are several library sources, some far away and some
very close to the schematic. The closest library to the schematic is the
<b>(parts list)</b> class PARTS_LIST. Circular dependencies are not allowed. All
dependencies must be resolvable in a straight forward way. This means that a
part in a remote library cannot be dependent on any part which is not always
resolvable.

<p> NRVO described:
    http://msdn.microsoft.com/en-us/library/ms364057%28VS.80%29.aspx

Even with NRVO provided by most C++ compilers, I don't see it being as lean as
having class LIB keep expanded members STRING fetch and STRINGS vfetch for the
aResults values. But at the topmost API, client convenience is worth a minor
sacrifice in speed, so the topmost API does return these complex string objects
for convenience. So there is a different strategy underneath the hood than what
is used on the hood ornament. When aResults pointer is passed as an argument, I
won't refer to this as 'returning' a value, but rather 'fetching' a result to
distinguish between the two strategies.


@section architecture Architecture

This document set later shows some <b>library sources</b> derived from class
LIB_SOURCE. A library source is the backing to a library. The class name for a
library in the new design is LIB.

<p>
Show architecture here.

<a href="../drawing.png" > Click here to see an architectural drawing.</a>

*/



/**
 * \defgroup string_types STRING Types
 * Provide some string types for use within the API.
 * @{
 */

typedef std::string STRING;
typedef std::dequeue<STRING>    STRINGS;

//typedef std::vector<wxString>   WSTRINGS;


const STRING StrEmpty = "";

/** @} string_types STRING Types */

/**
 * \defgroup exception_types Exception Types
 * Provide some exception types for use within the API.
 * @{
 */


/** @} exception_types Exception Types */



/**
 * Class LPID (aka GUID)
 * is a Logical Part ID and consists of various portions much like a URI.  It
 * relies heavily on a logical library name to hide where actual physical library
 * sources reside.  Its static functions serve as managers of the "library table" to
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
 * library name to library URI, type, and options. It has the following columns:
 * <ul>
 * <li> Logical Library Name
 * <li> Library Type
 * <li> Library URI.  The full URI to the library source, form dependent on Type.
 * <li> Options, used for access, such as password
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
 * The applicable library table is built up from several additive rows (table fragments),
 * and the final table is a merging of the table fragments. Two anticipated sources of
 * the rows are a personal table, and a schematic resident table.  The schematic
 * resident table rows are considered a higher priority in the final dynamically
 * assembled library table. A row in the schematic contribution to the library table
 * will take precedence over the personal table if there is a collision on logical
 * library name, otherwise the rows simply combine without issue to make up the
 * applicable library table.
 */
class LPID  // aka GUID
{
public:
    /**
     * Constructor LPID
     * takes aLPID string and parses it.  A typical LPID string uses a logical
     * library name followed by a part name.
     * e.g.: "kicad:passives/R/rev2", or
     * e.g.: "mylib:R33"
     */
    LPID( const STRING& aLPID ) throw( PARSE_ERROR );

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
     * @param aSchematic provides access to the full library table inclusive
     *  of the schematic contribution, or may be NULL to exclude the schematic rows.
     */
    static STRINGS GetLogicalLibraries( SCHEMATIC* aSchematic=NULL );

    /**
     * Function GetLibraryURI
     * returns the full library path from a logical library name.
     * @param aLogicalLibraryName is the short name for the library of interest.
     * @param aSchematic provides access to the full library table inclusive
     *  of the schematic contribution, or may be NULL to exclude the schematic rows.
     */
    static STRING  GetLibraryURI( const STRING& aLogicalLibraryName,
                        SCHEMATIC* aSchematic=NULL ) const;

    /**
     * Function GetLibraryType
     * returns the type of a logical library.
     * @param aLogicalLibraryName is the short name for the library of interest.
     * @param aSchematic provides access to the full library table inclusive
     *  of the schematic contribution, or may be NULL to exclude the schematic rows.
     */
    static STRING GetLibraryType( const STRING& aLogicalLibraryName,
                        SCHEMATIC* aSchematic=NULL ) const;

    /**
     * Function GetOptions
     * returns the options string for \a aLogicalLibraryName.
     * @param aLogicalLibraryName is the short name for the library of interest.
     * @param aSchematic provides access to the full library table inclusive
     *  of the schematic contribution, or may be NULL to exclude the schematic rows.
     */
    static STRING GetPassword( const STRING& aLogicalLibraryName,
                        SCHEMATIC* aSchematic=NULL ) const;
};


/**
 * Class SVN_LIB_SOURCE
 * implements a LIB_SOURCE in a subversion repository.
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
 * Class SCHEMATIC_LIB_SOURCE
 * implements a LIB_SOURCE in by reading a parts list from schematic file
 * unrelated to the schematic currently being edited.
 */
class SCHEMATIC_LIB_SOURCE : public LIB_SOURCE
{
    friend class LIBS;   ///< constructor the LIB uses these functions.

protected:

    /**
     * Constructor SCHEMATIC_LIB_SOURCE( const STRING& aSchematicFile )
     * sets up a LIB_SOURCE using aSchematicFile which is a full path and filename
     * for a schematic not related to the schematic being editing in
     * this EESCHEMA session.
     * @see LIBS::GetLibrary().
     *
     * @param aSchematicFile is a full path and filename.  Example:
     *  "/home/user/kicadproject/design.sch"
     */
    SCHEMATIC_LIB_SOURCE( const STRING& aSchematicFile ) throws( IO_ERROR );
};


/**
 * Class PARTS_LIST
 * is a LIB which resides in a SCHEMATIC, and it is a table model for a
 * spread sheet both.  When columns are added or removed to/from the spreadsheet,
 * this is adding or removing fields/properties to/from ALL the contained PARTS.
 */
class PARTS_LIST : public LIB
{
public:

    /**
     * Function GetModel
     * returns a spreadsheet table model that allows both reading and writing to
     * rows in a spreadsheet.  The UI holds the actual screen widgets, but
     * this is the table model, i.e. the PARTS_LIST is.
     */
    SPREADSHEET_TABLE_MODEL*    GetModel();
};



} // namespace SCH


// EOF
