/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef _LIB_TABLE_BASE_H_
#define _LIB_TABLE_BASE_H_

#include <map>
#include <boost/ptr_container/ptr_vector.hpp>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <project.h>
#include <richio.h>
#include <kicommon.h>

class OUTPUTFORMATTER;
class LIB_TABLE_LEXER;
class LIB_ID;
class LIB_TABLE_ROW;
class LIB_TABLE_GRID;
class LIB_TABLE;
class IO_ERROR;
class wxWindow;


typedef boost::ptr_vector< LIB_TABLE_ROW > LIB_TABLE_ROWS;
typedef LIB_TABLE_ROWS::iterator           LIB_TABLE_ROWS_ITER;
typedef LIB_TABLE_ROWS::const_iterator     LIB_TABLE_ROWS_CITER;


/**
 * Allows boost pointer containers to make clones of the data stored in them.  Since they
 * store pointers the data is cloned.  Copying and assigning pointers would cause ownership
 * issues if the standard C++ containers were used.
 */
KICOMMON_API LIB_TABLE_ROW* new_clone( const LIB_TABLE_ROW& aRow );


/**
 * LIB_TABLE_IO abstracts the file I/O operations for the library table
 * loading and saving.
 *
 * Normally, this is file-based-reading, but that's not a requirement.
 */
class KICOMMON_API LIB_TABLE_IO
{
public:
    virtual ~LIB_TABLE_IO() = default;

    /**
     * Create a reader for the given URI.
     *
     * This can return nullptr, for example if the URI is not a file or
     * is not readable.
     */
    virtual std::unique_ptr<LINE_READER> GetReader( const wxString& aURI ) const = 0;

    /**
     * Check if the given URI is writable.
     */
    virtual bool CanSaveToUri( const wxString& aURI ) const = 0;

    /**
     * Compare two URIs for equivalence.
     *
     * For example, two URIs that point to the same file should be considered equivalent,
     * even if they are not string-wise equal (e.g. symlinks)
     */
    virtual bool UrisAreEquivalent( const wxString& aURI1, const wxString& aURI2 ) const = 0;

    /**
     * Save the given table to the given URI.
     */
    virtual std::unique_ptr<OUTPUTFORMATTER> GetWriter( const wxString& aURI ) const = 0;
};


/**
 * Implementations of LIB_TABLE_IO for file-based I/O.
 *
 * This is the default implementation for real usage.
 */
class KICOMMON_API FILE_LIB_TABLE_IO : public LIB_TABLE_IO
{
public:
    FILE_LIB_TABLE_IO() = default;

    std::unique_ptr<LINE_READER> GetReader( const wxString& aURI ) const override;

    bool CanSaveToUri( const wxString& aURI ) const override;

    bool UrisAreEquivalent( const wxString& aURI1, const wxString& aURI2 ) const override;

    std::unique_ptr<OUTPUTFORMATTER> GetWriter( const wxString& aURI ) const override;
};


/**
 * Hold a record identifying a library accessed by the appropriate plug in object in the
 * #LIB_TABLE.  This is an abstract base class from which to derive library specific rows.
 */
class KICOMMON_API LIB_TABLE_ROW
{
public:
    LIB_TABLE_ROW() :
        enabled( true ),
        visible( true ),
        m_loaded( false ),
        m_parent( nullptr )
    {
    }

    virtual ~LIB_TABLE_ROW()
    {
    }

    LIB_TABLE_ROW( const wxString& aNick, const wxString& aURI, const wxString& aOptions,
                   const wxString& aDescr = wxEmptyString, LIB_TABLE* aParent = nullptr ) :
        nickName( aNick ),
        description( aDescr ),
        enabled( true ),
        visible( true ),
        m_loaded( false ),
        m_parent( aParent )
    {
        SetOptions( aOptions );
        SetFullURI( aURI );
    }

    bool operator==( const LIB_TABLE_ROW& r ) const;

    bool operator!=( const LIB_TABLE_ROW& r ) const { return !( *this == r ); }

    /**
     * @return the logical name of this library table row.
     */
    const wxString& GetNickName() const { return nickName; }

    /**
     * Change the logical name of this library, useful for an editor.
     */
    void SetNickName( const wxString& aNickName ) { nickName = aNickName; }

    /**
     * @return true if the library was loaded without error
     */
    bool GetIsLoaded() const { return m_loaded; }

    /**
     * Mark the row as being a loaded library
     */
    void SetLoaded( bool aLoaded ) { m_loaded = aLoaded; };

    /**
     * @return the enabled status of this library row
     */
    bool GetIsEnabled() const { return enabled; }

    /**
     * Change the enabled status of this library
     */
    void SetEnabled( bool aEnabled = true ) { enabled = aEnabled; }

    bool GetIsVisible() const { return visible; }

    void SetVisible( bool aVisible = true ) { visible = aVisible; }

    virtual bool LibraryExists() const = 0;

    virtual bool Refresh() { return false; }

    /**
     * Return the type of library represented by this row.
     */
    virtual const wxString GetType() const = 0;

    /**
     * Change the type of library represented by this row that must be implemented in the
     * derived object to provide the library table row type.
     */
    virtual void SetType( const wxString& aType ) = 0;

    virtual bool SupportsSettingsDialog() const { return false; }

    virtual void ShowSettingsDialog( wxWindow* aParent ) const {}

    /**
     * Return the full location specifying URI for the LIB, either in original UI form or
     * in environment variable expanded form.
     *
     * @param aSubstituted Tells if caller wanted the substituted form, else not.
     */
    const wxString GetFullURI( bool aSubstituted = false ) const;

    /**
     * Change the full URI for the library.
     */
    void SetFullURI( const wxString& aFullURI );

    /**
     * Return the options string, which may hold a password or anything else needed to
     * instantiate the underlying library plugin.
     */
    const wxString& GetOptions() const          { return options; }

    /**
     * Change the library options strings.
     */
    void SetOptions( const wxString& aOptions );

    /**
     * Return the description of the library referenced by this row.
     */
    const wxString& GetDescr() const            { return description; }

    /**
     * Change the description of the library referenced by this row.
     */
    void SetDescr( const wxString& aDescr )     { description = aDescr; }

    LIB_TABLE* GetParent() const { return m_parent; }

    void SetParent( LIB_TABLE* aParent ) { m_parent = aParent; }

    std::mutex& GetMutex() { return m_loadMutex; }

    /**
     * Return the constant #PROPERTIES for this library (#LIB_TABLE_ROW).  These are
     * the "options" in a table.
     */
    const std::map<std::string, UTF8>& GetProperties() const     { return properties; }

    /**
     * Serialize this object as utf8 text to an #OUTPUTFORMATTER, and tries to
     * make it look good using multiple lines and indentation.
     *
     * @param out is an #OUTPUTFORMATTER
     * @param nestLevel is the indentation level to base all lines of the output.
     *                  Actual indentation will be 2 spaces for each nestLevel.
     */
    void Format( OUTPUTFORMATTER* out, int nestLevel ) const;

    LIB_TABLE_ROW* clone() const
    {
        return do_clone();
    }

protected:
    LIB_TABLE_ROW( const LIB_TABLE_ROW& aRow ) :
        nickName( aRow.nickName ),
        uri_user( aRow.uri_user ),
        options( aRow.options ),
        description( aRow.description ),
        enabled( aRow.enabled ),
        visible( aRow.visible ),
        m_loaded( aRow.m_loaded ),
        m_parent( aRow.m_parent ),
        properties( aRow.properties )
    {
    }

    void operator=( const LIB_TABLE_ROW& aRow );

private:
    virtual LIB_TABLE_ROW* do_clone() const = 0;

    void setProperties( const std::map<std::string, UTF8>& aProperties );

private:
    wxString          nickName;
    wxString          uri_user;           ///< what user entered from UI or loaded from disk
    wxString          options;
    wxString          description;

    bool              enabled  = true;    ///< Whether the LIB_TABLE_ROW is enabled
    bool              visible  = true;    ///< Whether the LIB_TABLE_ROW is visible in choosers
    bool              m_loaded = false;   ///< Whether the LIB_TABLE_ROW is loaded
    LIB_TABLE*        m_parent;           ///< Pointer to the table this row lives in (maybe null)

    std::map<std::string, UTF8> properties;

    std::mutex        m_loadMutex;
};


/**
 * Manage #LIB_TABLE_ROW records (rows), and can be searched based on library nickname.
 *
 * <p>
 * This class owns the <b>library table</b>, which is like fstab in concept and maps
 * logical library name to the library URI, type, and options. It is heavily based on
 * the SWEET parser work done by Dick Hollenbeck and can be seen in new/sch_lib_table.h.
 * A library table has the following columns:
 * <ul>
 * <li> Logical Library Name (Nickname)
 * <li> Library Type, used to determine which plugin to load to access the library.
 * <li> Library URI.  The full URI to the library source, form dependent on Type.
 * <li> Options, used for as yet to be defined information such as user names or passwords
 * </ul>
 * </p>
 * <p>
 * The Library Type can be one of:
 * <ul>
 * <li> "file"
 * <li> "ftp"
 * <li> "http"
 * </ul>
 * </p>
 * <p>
 * For now, the Library URI types needed to support the various types can be one of those
 * shown below, which are typical of each type:
 * <ul>
 * <li> "file://C:/mylibdir"
 * <li> "ftp://kicad.org/partlib/trunk"
 * <li> "http://kicad.org/partlib"
 * </ul>
 * </p>
 * <p>
 * The library table is built up from several additive entries (table fragments), and the
 * final table is a (conceptual) merging of the table fragments. Two anticipated sources
 * of the entries are a personal table saved in the KiCad configuration and a project
 * resident table that resides in project file.  The project footprint table entries are
 * considered a higher priority in the final dynamically assembled library table.  An row
 * in the project file contribution to the library table takes precedence over the personal
 * table if there is a collision of logical library names.  Otherwise, the entries simply
 * combine without issue to make up the applicable library table.
 * </p>
 *
 * @author Wayne Stambaugh
 */
class KICOMMON_API LIB_TABLE : public PROJECT::_ELEM
{
public:
    /**
     * Parse the #LIB_TABLE_LEXER s-expression library table format into the appropriate
     * #LIB_TABLE_ROW objects.
     *
     * @param aLexer is the lexer to parse.
     *
     * @throw IO_ERROR if an I/O error occurs during parsing.
     * @throw PARSER_ERROR if the lexer format to parse is invalid.
     * @throw boost::bad_pointer if an any attempt to add an invalid pointer to the
     *                           boost::ptr_vector.
     * @throw boost::bad_index if an index outside the row table bounds is accessed.
     */
    virtual void Parse( LIB_TABLE_LEXER* aLexer ) = 0;

    /**
     * Generate the table in s-expression format to \a aOutput with an indentation level
     * of \a aIndentLevel.
     *
     * @param aOutput is the #OUTPUTFORMATTER to format the table into.
     * @param aIndentLevel is the indentation level (2 spaces) to indent.
     *
     * @throw IO_ERROR if an I/O error occurs during output.
     * @throw boost::interprocess::lock_except if separate process attempt to access the table.
     */
    virtual void Format( OUTPUTFORMATTER* aOutput, int aIndentLevel ) const = 0;

    /**
     * Build a library table by pre-pending this table fragment in front of \a aFallBackTable.
     * Loading of this table fragment is done by using Parse().
     *
     * @param aFallBackTable is another LIB_TABLE which is searched only when
     *                       a row is not found in this table.  No ownership is
     *                       taken of aFallBackTable.
     * @param aTableIo is the I/O object to use for the table data. nullptr
     *                 means use the default file-based I/O.
     */
    LIB_TABLE( LIB_TABLE*                    aFallBackTable = nullptr,
               std::unique_ptr<LIB_TABLE_IO> aTableIo = nullptr );

    virtual ~LIB_TABLE();

    /**
     * Compares this table against another.
     *
     * This compares the row *contents* against each other.
     * Any fallback tables are not checked.
     */
    bool operator==( const LIB_TABLE& r ) const
    {
        if( m_rows.size() == r.m_rows.size() )
        {
            unsigned i;

            for( i = 0; i < m_rows.size() && m_rows[i] == r.m_rows[i];  ++i )
                ;

            if( i == m_rows.size() )
                return true;
        }

        return false;
    }

    bool operator!=( const LIB_TABLE& r ) const  { return !( *this == r ); }

    /**
     * Get the number of rows contained in the table
     */
    unsigned GetCount() const
    {
        return m_rows.size();
    }

    /**
     * Get the 'n'th #LIB_TABLE_ROW object
     * @param  aIndex index of row (must exist: from 0 to GetCount() - 1)
     * @return        reference to the row
     */
    LIB_TABLE_ROW& At( unsigned aIndex )
    {
        return m_rows[aIndex];
    }

    /**
     * @copydoc At()
     */
    const LIB_TABLE_ROW& At( unsigned aIndex ) const
    {
        return m_rows[aIndex];
    }

    /**
     * Return true if the table is empty.
     *
     * @param aIncludeFallback is used to determine if the fallback table should be
     *                         included in the test.
     *
     * @return true if the footprint library table is empty.
     */
    bool IsEmpty( bool aIncludeFallback = true );

    /**
     * @return the library description from @a aNickname, or an empty string
     *         if @a aNickname does not exist.
     */
    const wxString GetDescription( const wxString& aNickname );

    /**
     * Test for the existence of \a aNickname in the library table.
     *
     * @param aCheckEnabled if true will only return true for enabled libraries
     * @return true if a library \a aNickname exists in the table.
     */
    bool HasLibrary( const wxString& aNickname, bool aCheckEnabled = false ) const;

    /**
     * Test for the existence of \a aPath in the library table.
     *
     * @param aCheckEnabled if true will only return true for enabled libraries
     * @return true if a library \a aNickname exists in the table.
     */
    bool HasLibraryWithPath( const wxString& aPath ) const;

    /**
     * Return the logical library names, all of them that are pertinent to
     * a look up done on this LIB_TABLE.
     */
    std::vector<wxString> GetLogicalLibs();

    /**
     * Return the full URI of the library mapped to \a aLibNickname.
     */
    wxString GetFullURI( const wxString& aLibNickname, bool aExpandEnvVars = true ) const;

    /**
     * Adds \a aRow if it does not already exist or if doReplace is true.  If doReplace
     * is not true and the key for aRow already exists, the function fails and returns false.
     *
     * The key for the table is the nickName, and all in this table must be unique.
     *
     * @param aRow is the new row to insert, or to forcibly add if doReplace is true.
     * @param doReplace if true, means insert regardless of whether aRow's key already
     *                  exists.  If false, then fail if the key already exists.
     *
     * @return bool - true if the operation succeeded.
     */
    bool InsertRow( LIB_TABLE_ROW* aRow, bool doReplace = false );

    /**
     * Removes a row from the table and frees the pointer
     * @param aRow is the row to remove
     * @return true if the row was found (and removed)
     */
    bool RemoveRow( const LIB_TABLE_ROW* aRow );

    /**
     * Replaces the Nth row with the given new row
     * @return true if successful
     */
    bool ReplaceRow( size_t aIndex, LIB_TABLE_ROW* aRow );

    /**
     * Moves a row within the table
     * @param aIndex is the current index of the row to move
     * @param aOffset is the number of positions to move it by in the table
     * @return true if the move resulted in a change
     */
    bool ChangeRowOrder( size_t aIndex, int aOffset );

    /**
     * Takes ownership of another list of rows; the original list will be freed
     */
    void TransferRows( LIB_TABLE_ROWS& aRowsList );

    /**
     * @return a #LIB_TABLE_ROW pointer if \a aURI is found in this table or in any chained
     *         fallBack table fragments, else NULL.
     */
    const LIB_TABLE_ROW* FindRowByURI( const wxString& aURI );

    /**
     * Load the library table using the path defined by \a aFileName aFallBackTable.
     *
     * @param aFileName contains the full path to the s-expression file.
     *
     * @throw IO_ERROR if an error occurs attempting to load the footprint library
     *                 table.
     */
    void Load( const wxString& aFileName );

    /**
     * Write this library table to \a aFileName in s-expression form.
     *
     * @param aFileName is the name of the file to write to.
     */
    void Save( const wxString& aFileName ) const;

    /**
     * Parses \a aOptionsList and places the result into a #PROPERTIES object which is
     * returned.  If the options field is empty, then the returned PROPERTIES will be
     * a NULL pointer.
     *
     * <p>
     * Typically aOptionsList comes from the "options" field within a LIB_TABLE_ROW and
     * the format is simply a comma separated list of name value pairs. e.g.:
     * [name1[=value1][|name2[=value2]]] etc.  When using the UI to create or edit
     * a library table, this formatting is handled for you.
     * </p>
     */
    static std::map<std::string, UTF8> ParseOptions( const std::string& aOptionsList );

    /**
     * Returns a list of options from the aProperties parameter.
     *
     * The name=value pairs will be separated with the '|' character.  The =value portion may
     * not be present.  You might expect something like "name1=value1|name2=value2|flag_me".
     * Notice that flag_me does not have a value.  This is ok.
     *
     * @param aProperties is the PROPERTIES to format or NULL.  If NULL the returned
     *                    string will be empty.
     */
    static UTF8 FormatOptions( const std::map<std::string, UTF8>* aProperties );

    /**
     * Returns the version number (0 if unset)
     *
     * @return integer version number read from table
     */
    int GetVersion() const
    {
        return m_version;
    }

protected:
    /*
     * Do not make this public.  It MUST be called with a lock already in place.
     */
    void clear();

    /**
     * Return a #LIB_TABLE_ROW if \a aNickname is found in this table or in any chained
     * fallBack table fragment, else NULL.
     *
     * @param aNickname is the name of the library table entry to find.
     * @param aCheckIfEnabled is a flag to check if the library table entry is enabled.
     * @return a pointer to the #LIB_TABLE_ROW found.
     */
    LIB_TABLE_ROW* findRow( const wxString& aNickname, bool aCheckIfEnabled = false ) const;

    /**
     * Performs the mechanics of inserting a row, but without locking or reindexing.
     */
    bool doInsertRow( LIB_TABLE_ROW* aRow, bool doReplace = false );

    /**
     * Updates the env vars from older version of KiCad, provided they do not currently
     * resolve to anything
     *
     * @return True if the tables were modified
     */
    bool migrate();

    /*
     * Do not make this public.  It MUST be called with a lock already in place.
     */
    void reindex();

protected:
    // Injected I/O interface
    std::unique_ptr<LIB_TABLE_IO> m_io;

    LIB_TABLE* m_fallBack;

    /// Versioning to handle importing old tables
    mutable int m_version;

    /// Owning set of rows.
    // TODO: This should really be private; but the lib table grids re-use it
    //       (without using m_rowsMap).
    LIB_TABLE_ROWS m_rows;

    /// this is a non-owning index into the LIB_TABLE_ROWS table
    std::map<wxString, LIB_TABLE_ROWS_ITER> m_rowsMap;

    /// Mutex to protect access to the rows vector
    mutable std::shared_mutex m_mutex;
};

#endif  // _LIB_TABLE_BASE_H_
