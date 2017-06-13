/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012-2017 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2012-2017 KiCad Developers, see change_log.txt for contributors.
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

#include <boost/interprocess/exceptions.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/noncopyable.hpp>

#include <project.h>
#include <properties.h>
#include <richio.h>


#define FP_LATE_ENVVAR  1           ///< late=1/early=0 environment variable expansion

class OUTPUTFORMATTER;
class LIB_TABLE_LEXER;
class LIB_ID;
class LIB_TABLE_ROW;
class LIB_TABLE_GRID;
class IO_ERROR;


typedef boost::ptr_vector< LIB_TABLE_ROW > LIB_TABLE_ROWS;
typedef LIB_TABLE_ROWS::iterator           LIB_TABLE_ROWS_ITER;
typedef LIB_TABLE_ROWS::const_iterator     LIB_TABLE_ROWS_CITER;


/**
 * Function new_clone
 *
 * Allows boost pointer containers to make clones of the data stored in them.  Since they
 * store pointers the data is cloned.  Copying and assigning pointers would cause ownership
 * issues if the standard C++ containers were used.
 */
LIB_TABLE_ROW* new_clone( const LIB_TABLE_ROW& aRow );


/**
 * Class LIB_TABLE_ROW
 *
 * holds a record identifying a library accessed by the appropriate plug in object in the
 * #LIB_TABLE.  This is an abstract base class from which to derive library specific rows.
 */
class LIB_TABLE_ROW : boost::noncopyable
{
public:
    LIB_TABLE_ROW()
    {
    }

    virtual ~LIB_TABLE_ROW()
    {
    }

    LIB_TABLE_ROW( const wxString& aNick, const wxString& aURI, const wxString& aOptions,
                   const wxString& aDescr = wxEmptyString ) :
        nickName( aNick ),
        description( aDescr )
    {
        properties.reset();
        SetOptions( aOptions );
        SetFullURI( aURI );
    }

    bool operator==( const LIB_TABLE_ROW& r ) const;

    bool operator!=( const LIB_TABLE_ROW& r ) const   { return !( *this == r ); }

    /**
     * Function GetNickName
     *
     * @return the logical name of this library table row.
     */
    const wxString& GetNickName() const         { return nickName; }

    /**
     * Function SetNickName
     *
     * changes the logical name of this library, useful for an editor.
     */
    void SetNickName( const wxString& aNickName ) { nickName = aNickName; }

    /**
     * Function GetType
     *
     * is a pure virtual function that returns the type of LIB represented by this row.
     */
    virtual const wxString GetType() const = 0;

    /**
     * Function SetType
     *
     * is a pure virtual function changes the type represented by this row that must
     * be implemented in the derived object to provide the library table row type.
     */
    virtual void SetType( const wxString& aType ) = 0;

    /**
     * Function GetFullURI
     *
     * returns the full location specifying URI for the LIB, either in original
     * UI form or in environment variable expanded form.
     *
     * @param aSubstituted Tells if caller wanted the substituted form, else not.
     */
    const wxString GetFullURI( bool aSubstituted = false ) const;

    /**
     * Function SetFullURI
     *
     * changes the full URI for the library.
     */
    void SetFullURI( const wxString& aFullURI );

    /**
     * Function GetOptions
     *
     * returns the options string, which may hold a password or anything else needed to
     * instantiate the underlying LIB_SOURCE.
     */
    const wxString& GetOptions() const          { return options; }

    /**
     * Function SetOptions
     */
    void SetOptions( const wxString& aOptions );

    /**
     * Function GetDescr
     *
     * returns the description of the library referenced by this row.
     */
    const wxString& GetDescr() const            { return description; }

    /**
     * Function SetDescr
     *
     * changes the description of the library referenced by this row.
     */
    void SetDescr( const wxString& aDescr )     { description = aDescr; }

    /**
     * Function GetProperties
     *
     * returns the constant PROPERTIES for this library (LIB_TABLE_ROW).  These are
     * the "options" in a table.
     */
    const PROPERTIES* GetProperties() const     { return properties.get(); }

    /**
     * Function Format
     *
     * serializes this object as utf8 text to an OUTPUTFORMATTER, and tries to
     * make it look good using multiple lines and indentation.
     *
     * @param out is an #OUTPUTFORMATTER
     * @param nestLevel is the indentation level to base all lines of the output.
     *                  Actual indentation will be 2 spaces for each nestLevel.
     */
    void Format( OUTPUTFORMATTER* out, int nestLevel ) const;

    static void Parse( std::unique_ptr< LIB_TABLE_ROW >& aRow, LIB_TABLE_LEXER* in );

    LIB_TABLE_ROW* clone() const
    {
        return do_clone();
    }

protected:
    LIB_TABLE_ROW( const LIB_TABLE_ROW& aRow ) :
        nickName( aRow.nickName ),
        uri_user( aRow.uri_user ),
#if !FP_LATE_ENVVAR
        uri_expanded( aRow.uri_expanded ),
#endif
        options( aRow.options ),
        description( aRow.description )
    {
        if( aRow.properties )
            properties.reset( new PROPERTIES( *aRow.properties.get() ) );
        else
            properties.reset();
    }

    void operator=( const LIB_TABLE_ROW& aRow );

private:
    virtual LIB_TABLE_ROW* do_clone() const = 0;

    void setProperties( PROPERTIES* aProperties );

    wxString          nickName;
    wxString          uri_user;           ///< what user entered from UI or loaded from disk

#if !FP_LATE_ENVVAR
    wxString          uri_expanded;       ///< from ExpandSubstitutions()
#endif

    wxString          options;
    wxString          description;

    std::unique_ptr< PROPERTIES > properties;
};


/**
 * Class LIB_TABLE
 *
 * holds #LIB_TABLE_ROW records (rows), and can be searched based on library nickname.
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
class LIB_TABLE : public PROJECT::_ELEM
{
    friend class DIALOG_FP_LIB_TABLE;
    friend class LIB_TABLE_GRID;

public:

    /**
     * Function Parse
     *
     * Parses the \a #LIB_TABLE_LEXER s-expression library table format into the appropriate
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
     * Generate the table in s-expression format to \a aOutput with an indention level
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
     * Constructor LIB_TABLE
     * builds a library table by pre-pending this table fragment in front of
     * @a aFallBackTable.  Loading of this table fragment is done by using Parse().
     *
     * @param aFallBackTable is another LIB_TABLE which is searched only when
     *                       a row is not found in this table.  No ownership is
     *                       taken of aFallBackTable.
     */
    LIB_TABLE( LIB_TABLE* aFallBackTable = NULL );

    virtual ~LIB_TABLE();

    /// Delete all rows.
    void Clear()
    {
        rows.clear();
        nickIndex.clear();
    }

    bool operator==( const LIB_TABLE& r ) const
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

    bool operator!=( const LIB_TABLE& r ) const  { return !( *this == r ); }

    int GetCount()       { return rows.size(); }

    LIB_TABLE_ROW* At( int aIndex ) { return &rows[aIndex]; }

    /**
     * Function IsEmpty
     *
     * @param aIncludeFallback is used to determine if the fallback table should be
     *                         included in the test.
     *
     * @return true if the footprint library table is empty.
     */
    bool IsEmpty( bool aIncludeFallback = true );

    /**
     * Function GetDescription
     *
     * @return the library description from @a aNickname, or an empty string
     *         if @a aNickname does not exist.
     */
    const wxString GetDescription( const wxString& aNickname );

    /**
     * Function GetLogicalLibs
     *
     * returns the logical library names, all of them that are pertinent to
     * a look up done on this LIB_TABLE.
     */
    std::vector<wxString> GetLogicalLibs();

    /**
     * Function InsertRow
     *
     * adds aRow if it does not already exist or if doReplace is true.  If doReplace
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
     * Function FindRowByURI
     *
     * @return a #LIB_TABLE_ROW pointer if \a aURI is found in this table or in any chained
     *         fallBack table fragments, else NULL.
     */
    const LIB_TABLE_ROW* FindRowByURI( const wxString& aURI );

    /**
     * Function Load
     *
     * loads the library table using the path defined by \a aFileName aFallBackTable.
     *
     * @param aFileName contains the full path to the s-expression file.
     *
     * @throw IO_ERROR if an error occurs attempting to load the footprint library
     *                 table.
     */
    void Load( const wxString& aFileName );

    /**
     * Function Save
     *
     * writes this library table to \a aFileName in s-expression form.
     *
     * @param aFileName is the name of the file to write to.
     */
    void Save( const wxString& aFileName ) const;

    /**
     * Search the paths all of the #LIB_TABLE_ROWS of the #LIB_TABLE and add all of the
     * environment variable substitutions to \a aEnvVars.
     *
     * This will only add unique environment variables to the list.  Duplicates are ignored.
     *
     * @param aEnvVars is the array to load the environment variables.
     *
     * @return the number of unique environment variables found in the table.
     */
    size_t GetEnvVars( wxArrayString& aEnvVars ) const;

    /**
     * Function ParseOptions
     *
     * parses @a aOptionsList and places the result into a PROPERTIES object which is
     * returned.  If the options field is empty, then the returned PROPERTIES will be
     * a NULL pointer.
     * <p>
     * Typically aOptionsList comes from the "options" field within a LIB_TABLE_ROW and
     * the format is simply a comma separated list of name value pairs. e.g.:
     * [name1[=value1][|name2[=value2]]] etc.  When using the UI to create or edit
     * a library table, this formatting is handled for you.
     * </p>
     */
    static PROPERTIES* ParseOptions( const std::string& aOptionsList );

    /**
     * Function FormatOptions
     *
     * returns a list of options from the aProperties parameter.  The name=value
     * pairs will be separated with the '|' character.  The =value portion may not
     * be present.  You might expect something like "name1=value1|name2=value2|flag_me".
     * Notice that flag_me does not have a value.  This is ok.
     *
     * @param aProperties is the PROPERTIES to format or NULL.  If NULL the returned
     *                    string will be empty.
     */
    static UTF8 FormatOptions( const PROPERTIES* aProperties );

    /**
     * Function ExpandSubstitutions
     *
     * replaces any environment variable references with their values and is here to fully
     * embellish the TABLE_ROW::uri in a platform independent way.  This enables library
     * tables to have platform dependent environment variables in them, allowing for a
     * uniform table across platforms.
     */
    static const wxString ExpandSubstitutions( const wxString& aString );

protected:

    /**
     * Function findRow
     * returns a LIB_TABLE_ROW if aNickname is found in this table or in any chained
     * fallBack table fragment, else NULL.
     */
    LIB_TABLE_ROW* findRow( const wxString& aNickname ) const;

    void reindex()
    {
        nickIndex.clear();

        for( LIB_TABLE_ROWS_ITER it = rows.begin(); it != rows.end(); ++it )
            nickIndex.insert( INDEX_VALUE( it->GetNickName(), it - rows.begin() ) );
    }

    void ensureIndex()
    {
        // The dialog lib table editor may not maintain the nickIndex.
        // Lazy indexing may be required.  To handle lazy indexing, we must enforce
        // that "nickIndex" is either empty or accurate, but never inaccurate.
        if( !nickIndex.size() )
            reindex();
    }

    LIB_TABLE_ROWS rows;

    /// this is a non-owning index into the LIB_TABLE_ROWS table
    typedef std::map<wxString,int>      INDEX;              // "int" is std::vector array index
    typedef INDEX::iterator             INDEX_ITER;
    typedef INDEX::const_iterator       INDEX_CITER;
    typedef INDEX::value_type           INDEX_VALUE;

    /// this particular key is the nickName within each row.
    INDEX nickIndex;

    LIB_TABLE* fallBack;
};

#endif  // _LIB_TABLE_BASE_H_
