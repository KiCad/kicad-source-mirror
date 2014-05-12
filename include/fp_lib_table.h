/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-12 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2012 KiCad Developers, see change_log.txt for contributors.
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

#ifndef FP_LIB_TABLE_H_
#define FP_LIB_TABLE_H_

#include <macros.h>
#include <vector>
#include <map>
#include <io_mgr.h>
#include <project.h>

#define FP_LATE_ENVVAR  1           ///< late=1/early=0 environment variable expansion

class wxFileName;
class OUTPUTFORMATTER;
class MODULE;
class FP_LIB_TABLE_LEXER;
class FPID;
class NETLIST;
class REPORTER;
class SEARCH_STACK;

/**
 * Class FP_LIB_TABLE
 * holds FP_LIB_TABLE::ROW records (rows), and can be searched based on library nickName.
 * <p>
 * This class owns the <b>footprint library table</b>, which is like fstab in concept and maps
 * logical library name to the library URI, type, and options. It is heavily based on the SWEET
 * parser work done by Dick Hollenbeck and can be seen in new/sch_lib_table.h.  A footprint
 * library table had  the following columns:
 * <ul>
 * <li> Logical Library Name (Nickname)
 * <li> Library Type, used to determine which plugin to load to access the library.
 * <li> Library URI.  The full URI to the library source, form dependent on Type.
 * <li> Options, used for as yet to be defined information such as user names or passwords
 * </ul>
 * <p>
 * The Library Type can be one of:
 * <ul>
 * <li> "file"
 * <li> "ftp"
 * <li> "http"
 * </ul>
 * <p>
 * For now, the Library URI types needed to support the various types can be one of those
 * shown below, which are typical of each type:
 * <ul>
 * <li> "file://C:/mylibdir"
 * <li> "ftp://kicad.org/partlib/trunk"
 * <li> "http://kicad.org/partlib"
 * </ul>
 * <p>
 * The footprint library table is built up from several additive entries (table fragments),
 * and the final table is a (conceptual) merging of the table fragments. Two
 * anticipated sources of the entries are a personal table saved in the KiCad configuration
 * and a project resident table that resides in project file.  The project footprint table
 * entries are considered a higher priority in the final dynamically assembled library table.
 * An row in the project file contribution to the library table takes precedence over the
 * personal table if there is a collision of logical library names.  Otherwise, the entries
 * simply combine without issue to make up the applicable library table.
 *
 * @author Wayne Stambaugh
 */
class FP_LIB_TABLE : public PROJECT::_ELEM
{
    friend class DIALOG_FP_LIB_TABLE;

public:

    /**
     * Class ROW
     * holds a record identifying a footprint library accessed by the appropriate #PLUGIN
     * object in the #FP_LIB_TABLE.
     */
    class ROW
    {
        friend class FP_LIB_TABLE;
        friend class DIALOG_FP_LIB_TABLE;

    public:

        typedef IO_MGR::PCB_FILE_T   LIB_T;

        ROW() :
            type( IO_MGR::KICAD ),
            properties( 0 )
        {
        }

        ROW( const wxString& aNick, const wxString& aURI, const wxString& aType,
             const wxString& aOptions, const wxString& aDescr = wxEmptyString ) :
            nickName( aNick ),
            description( aDescr ),
            properties( 0 )
        {
            SetOptions( aOptions ),
            SetFullURI( aURI );
            SetType( aType );
        }

        ROW( const ROW& a );

        ~ROW()
        {
            delete properties;
        }

        ROW& operator=( const ROW& r );

        /// Used in DIALOG_FP_LIB_TABLE for detecting an edit.
        bool operator==( const ROW& r ) const;

        bool operator!=( const ROW& r ) const   { return !( *this == r ); }

        //-----<accessors>------------------------------------------------------

        /**
         * Function GetNickName
         * returns the short name of this library table row.
         */
        const wxString& GetNickName() const         { return nickName; }

        /**
         * Function SetNickName
         * changes the logical name of this library, useful for an editor.
         */
        void SetNickName( const wxString& aNickName ) { nickName = aNickName; }

        /**
         * Function GetType
         * returns the type of LIB represented by this row.
         */
        const wxString GetType() const              { return IO_MGR::ShowType( type ); }

        /**
         * Function SetType
         * changes the type represented by this row.
         */
        void SetType( const wxString& aType );

        /**
         * Function GetFullURI
         * returns the full location specifying URI for the LIB, either in original
         * UI form or in environment variable expanded form.
         *
         * @param aSubstituted Tells if caller wanted the substituted form, else not.
         */
        const wxString GetFullURI( bool aSubstituted = false ) const;

        /**
         * Function SetFullURI
         * changes the full URI for the library.
         */
        void SetFullURI( const wxString& aFullURI );

        /**
         * Function GetOptions
         * returns the options string, which may hold a password or anything else needed to
         * instantiate the underlying LIB_SOURCE.
         */
        const wxString& GetOptions() const          { return options; }

        /**
         * Function SetOptions
         */
        void SetOptions( const wxString& aOptions )
        {
            options = aOptions;

            // set PROPERTIES* from options
            setProperties( ParseOptions( TO_UTF8( aOptions ) ) );
        }

        /**
         * Function GetDescr
         * returns the description of the library referenced by this row.
         */
        const wxString& GetDescr() const            { return description; }

        /**
         * Function SetDescr
         * changes the description of the library referenced by this row.
         */
        void SetDescr( const wxString& aDescr )     { description = aDescr; }

        /**
         * Function GetProperties
         * returns the constant PROPERTIES for this library (ROW).  These are
         * the "options" in a table.
         */
        const PROPERTIES* GetProperties() const     { return properties; }

        //-----</accessors>-----------------------------------------------------

        /**
         * Function Format
         * serializes this object as utf8 text to an OUTPUTFORMATTER, and tries to
         * make it look good using multiple lines and indentation.
         * @param out is an #OUTPUTFORMATTER
         * @param nestLevel is the indentation level to base all lines of the output.
         *   Actual indentation will be 2 spaces for each nestLevel.
         */
        void Format( OUTPUTFORMATTER* out, int nestLevel ) const
            throw( IO_ERROR );

    private:

        /**
         * Function setProperties
         * sets this ROW's PROPERTIES by taking ownership of @a aProperties.
         * @param aProperties ownership is given over to this ROW.
         */
        void setProperties( const PROPERTIES* aProperties )
        {
            delete properties;
            properties = aProperties;
        }

        void setPlugin( PLUGIN* aPlugin )
        {
            plugin.set( aPlugin );
        }

        wxString        nickName;
        wxString        uri_user;           ///< what user entered from UI or loaded from disk

#if !FP_LATE_ENVVAR
        wxString        uri_expanded;       ///< from ExpandSubstitutions()
#endif

        LIB_T           type;
        wxString        options;
        wxString        description;

        const PROPERTIES*   properties;
        PLUGIN::RELEASER    plugin;
    };

    /**
     * Constructor FP_LIB_TABLE
     * builds a library table by pre-pending this table fragment in front of
     * @a aFallBackTable.  Loading of this table fragment is done by using Parse().
     *
     * @param aFallBackTable is another FP_LIB_TABLE which is searched only when
     *                       a row is not found in this table.  No ownership is
     *                       taken of aFallBackTable.
     */
    FP_LIB_TABLE( FP_LIB_TABLE* aFallBackTable = NULL );

    ~FP_LIB_TABLE();

    /// Delete all rows.
    void Clear()
    {
        rows.clear();
        nickIndex.clear();
    }

    bool operator==( const FP_LIB_TABLE& r ) const
    {
        if( rows.size() == r.rows.size() )
        {
            unsigned i;
            for( i = 0; i < rows.size() && rows[i] == r.rows[i];  ++i )
                ;

            if( i == rows.size() )
                return true;
        }

        return false;
    }

    bool operator!=( const FP_LIB_TABLE& r ) const  { return !( *this == r ); }

    int     GetCount()                              { return rows.size(); }

    ROW&    At( int aIndex )                        { return rows[aIndex]; }

    /**
     * Function Parse
     * fills this table fragment from information in the input stream \a aParser, which
     * is a DSNLEXER customized for the grammar needed to describe instances of this object.
     * The entire textual element spec is <br>
     *
     * <pre>
     * (fp_lib_table
     *   (lib (name LOGICAL)(descr DESCRIPTION)(uri FULL_URI)(type TYPE)(options OPTIONS))
     *   (lib (name LOGICAL)(descr DESCRIPTION)(uri FULL_URI)(type TYPE)(options OPTIONS))
     *   (lib (name LOGICAL)(descr DESCRIPTION)(uri FULL_URI)(type TYPE)(options OPTIONS))
     *  )
     * </pre>
     *
     * When this function is called, the input token stream given by \a aParser
     * is assumed to be positioned at the '^' in the following example, i.e. just
     * after the identifying keyword and before the content specifying stuff.
     * <br>
     * (lib_table ^ (....) )
     *
     * @param aParser is the input token stream of keywords and symbols.
     */
    void Parse( FP_LIB_TABLE_LEXER* aParser ) throw( IO_ERROR, PARSE_ERROR );

    /**
     * Function ParseOptions
     * parses @a aOptionsList and places the result into a PROPERTIES object
     * which is returned.  If the options field is empty, then the returned PROPERTIES
     * will be a NULL pointer.
     * <p>
     * Typically aOptionsList comes from the "options" field within a ROW and
     * the format is simply a comma separated list of name value pairs. e.g.:
     * [name1[=value1][|name2[=value2]]] etc.  When using the UI to create or edit
     * a fp lib table, this formatting is handled for you.
     */
    static PROPERTIES* ParseOptions( const std::string& aOptionsList );

    /**
     * Function FormatOptions
     * returns a list of options from the aProperties parameter.  The name=value
     * pairs will be separted with the '|' character.  The =value portion may not
     * be present.  You might expect something like "name1=value1|name2=value2|flag_me".
     * Notice that flag_me does not have a value.  This is ok.
     *
     * @param aProperties is the PROPERTIES to format or NULL.  If NULL the returned
     *  string will be empty.
     */
    static UTF8 FormatOptions( const PROPERTIES* aProperties );

    /**
     * Function Format
     * serializes this object as utf8 text to an #OUTPUTFORMATTER, and tries to
     * make it look good using multiple lines and indentation.
     *
     * @param out is an #OUTPUTFORMATTER
     * @param nestLevel is the indentation level to base all lines of the output.
     *   Actual indentation will be 2 spaces for each nestLevel.
     */
    void Format( OUTPUTFORMATTER* out, int nestLevel ) const throw( IO_ERROR );

    /**
     * Function GetLogicalLibs
     * returns the logical library names, all of them that are pertinent to
     * a lookup done on this FP_LIB_TABLE.
     */
    std::vector<wxString> GetLogicalLibs();

    //-----<PLUGIN API SUBSET, REBASED ON aNickname>---------------------------

    /**
     * Function FootprintEnumerate
     * returns a list of footprint names contained within the library given by
     * @a aNickname.
     *
     * @param aNickname is a locator for the "library", it is a "name"
     *     in FP_LIB_TABLE::ROW
     *
     * @return wxArrayString - is the array of available footprint names inside
     *   a library
     *
     * @throw IO_ERROR if the library cannot be found, or footprint cannot be loaded.
     */
    wxArrayString FootprintEnumerate( const wxString& aNickname );

    /**
     * Function FootprintLoad
     * loads a footprint having @a aFootprintName from the library given by @a aNickname.
     *
     * @param aNickname is a locator for the "library", it is a "name"
     *     in FP_LIB_TABLE::ROW
     *
     * @param aFootprintName is the name of the footprint to load.
     *
     * @return  MODULE* - if found caller owns it, else NULL if not found.
     *
     * @throw   IO_ERROR if the library cannot be found or read.  No exception
     *          is thrown in the case where aFootprintName cannot be found.
     */
    MODULE* FootprintLoad( const wxString& aNickname, const wxString& aFootprintName );

    /**
     * Enum SAVE_T
     * is the set of return values from FootprintSave() below.
     */
    enum SAVE_T
    {
        SAVE_OK,
        SAVE_SKIPPED,
    };

    /**
     * Function FootprintSave
     * will write @a aFootprint to an existing library given by @a aNickname.
     * If a footprint by the same name already exists, it is replaced.
     *
     * @param aNickname is a locator for the "library", it is a "name"
     *     in FP_LIB_TABLE::ROW
     *
     * @param aFootprint is what to store in the library. The caller continues
     *    to own the footprint after this call.
     *
     * @param aOverwrite when true means overwrite any existing footprint by the
     *  same name, else if false means skip the write and return SAVE_SKIPPED.
     *
     * @return SAVE_T - SAVE_OK or SAVE_SKIPPED.  If error saving, then IO_ERROR is thrown.
     *
     * @throw IO_ERROR if there is a problem saving.
     */
    SAVE_T FootprintSave( const wxString& aNickname, const MODULE* aFootprint, bool aOverwrite = true );

    /**
     * Function FootprintDelete
     * deletes the @a aFootprintName from the library given by @a aNickname.
     *
     * @param aNickname is a locator for the "library", it is a "name"
     *     in FP_LIB_TABLE::ROW
     *
     * @param aFootprintName is the name of a footprint to delete from the specified library.
     *
     * @throw IO_ERROR if there is a problem finding the footprint or the library, or deleting it.
     */
    void FootprintDelete( const wxString& aNickname, const wxString& aFootprintName );

    /**
     * Function IsFootprintLibWritable
     * returns true iff the library given by @a aNickname is writable.  (Often
     * system libraries are read only because of where they are installed.)
     *
     * @throw IO_ERROR if no library at aLibraryPath exists.
     */
    bool IsFootprintLibWritable( const wxString& aNickname );

    void FootprintLibDelete( const wxString& aNickname );

    void FootprintLibCreate( const wxString& aNickname );

    //-----</PLUGIN API SUBSET, REBASED ON aNickname>---------------------------

    /**
     * Function FootprintLoadWithOptionalNickname
     * loads a footprint having @a aFootprintId with possibly an empty nickname.
     *
     * @param aFootprintId the [nickname] & fooprint name of the footprint to load.
     *
     * @return  MODULE* - if found caller owns it, else NULL if not found.
     *
     * @throw   IO_ERROR if the library cannot be found or read.  No exception
     *          is thrown in the case where aFootprintName cannot be found.
     * @throw   PARSE_ERROR if @a aFootprintId is not parsed OK.
     */
    MODULE* FootprintLoadWithOptionalNickname( const FPID& aFootprintId )
        throw( IO_ERROR, PARSE_ERROR );

    /**
     * Function GetDescription
     * returns the library desicription from @a aNickname, or an empty string
     * if aNickname does not exist.
     */
    const wxString GetDescription( const wxString& aNickname );

    /**
     * Function InsertRow
     * adds aRow if it does not already exist or if doReplace is true.  If doReplace
     * is not true and the key for aRow already exists, the function fails and returns false.
     * The key for the table is the nickName, and all in this table must be unique.
     * @param aRow is the new row to insert, or to forcibly add if doReplace is true.
     * @param doReplace if true, means insert regardless of whether aRow's key already
     *  exists.  If false, then fail if the key already exists.
     * @return bool - true if the operation succeeded.
     */
    bool InsertRow( const ROW& aRow, bool doReplace = false );

    /**
     * Function FindRow
     * returns a ROW if aNickName is found in this table or in any chained
     * fallBack table fragment.  The PLUGIN is loaded and attached
     * to the "plugin" field of the ROW if not already loaded.
     *
     * @throw IO_ERROR if aNickName cannot be found.
     */
    const ROW* FindRow( const wxString& aNickName ) throw( IO_ERROR );

    /**
     * Function FindRowByURI
     * returns a #FP_LIB_TABLE::ROW if aURE is found in this table or in any chained
     * fallBack table fragments, else NULL.
     */
    const ROW* FindRowByURI( const wxString& aURI );

    /**
     * Function IsEmpty
     * @param aIncludeFallback is used to determine if the fallback table should be
     *                         included in the test.
     * @return true if the footprint library table is empty.
     */
    bool IsEmpty( bool aIncludeFallback = true );

    /**
     * Function ExpandSubstitutions
     * replaces any environment variable references with their values and is
     * here to fully embellish the ROW::uri in a platform independent way.
     * This enables (fp_lib_table)s to have platform dependent environment
     * variables in them, allowing for a uniform table across platforms.
     */
    static const wxString ExpandSubstitutions( const wxString& aString );

    /**
     * Function LoadGlobalTable
     * loads the global footprint library table into \a aTable.
     *
     * This probably should be move into the application object when KiCad is changed
     * to a single process application.  This is the least painful solution for the
     * time being.
     *
     * @param aTable the #FP_LIB_TABLE object to load.
     * @return true if the global library table exists and is loaded properly.
     * @throw IO_ERROR if an error occurs attempting to load the footprint library
     *                 table.
     */
    static bool LoadGlobalTable( FP_LIB_TABLE& aTable ) throw (IO_ERROR, PARSE_ERROR );

    /**
     * Function GetGlobalTableFileName
     * @return the platform specific global footprint library path and file name.
     */
    static wxString GetGlobalTableFileName();

#if 0
    /**
     * Function GetFileName
     * @return the footprint library file name.
     */
    static const wxString GetFileName();
#endif

    /**
     * Function GlobalPathEnvVarVariableName
     * returns the name of the environment variable used to hold the directory of
     * locally installed "KiCad sponsored" system footprint libraries.  These can
     * be either legacy or pretty format.  The only thing special about this
     * particular environment variable is that it is set automatically by
     * KiCad on program startup, <b>iff</b> it is not set already in the environment.
     */
    static const wxString GlobalPathEnvVariableName();

    /**
     * Function Load
     * loads the footprint library table using the path defined in \a aFileName with
     * \a aFallBackTable.
     *
     * @param aFileName contains the full path to the s-expression file.
     *
     * @throw IO_ERROR if an error occurs attempting to load the footprint library
     *                 table.
     */
    void Load( const wxString& aFileName ) throw( IO_ERROR );

    /**
     * Function Save
     * writes this table to aFileName in s-expression form.
     * @param aFileName is the name of the file to write to.
     */
    void Save( const wxString& aFileName ) const throw( IO_ERROR );

protected:

    /**
     * Function findRow
     * returns a ROW if aNickname is found in this table or in any chained
     * fallBack table fragment, else NULL.
     */
    ROW* findRow( const wxString& aNickname ) const;

    void reindex()
    {
        nickIndex.clear();

        for( ROWS_CITER it = rows.begin();  it != rows.end();  ++it )
            nickIndex.insert( INDEX_VALUE( it->nickName, it - rows.begin() ) );
    }

    void ensureIndex()
    {
        // The dialog lib table editor may not maintain the nickIndex.
        // Lazy indexing may be required.  To handle lazy indexing, we must enforce
        // that "nickIndex" is either empty or accurate, but never inaccurate.
        if( !nickIndex.size() )
            reindex();
    }

    typedef std::vector<ROW>            ROWS;
    typedef ROWS::iterator              ROWS_ITER;
    typedef ROWS::const_iterator        ROWS_CITER;

    ROWS            rows;

    /// this is a non-owning index into the ROWS table
    typedef std::map<wxString,int>      INDEX;              // "int" is std::vector array index
    typedef INDEX::iterator             INDEX_ITER;
    typedef INDEX::const_iterator       INDEX_CITER;
    typedef INDEX::value_type           INDEX_VALUE;

    /// this particular key is the nickName within each row.
    INDEX           nickIndex;

    FP_LIB_TABLE*   fallBack;
};


extern FP_LIB_TABLE GFootprintTable;        // KIFACE scope.

#endif  // FP_LIB_TABLE_H_
