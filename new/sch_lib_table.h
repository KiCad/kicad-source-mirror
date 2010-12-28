/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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

#ifndef SCH_LIB_TABLE_H_
#define SCH_LIB_TABLE_H_

#include <string>
#include <boost/ptr_container/ptr_map.hpp>
#include <sch_lib.h>

class SCH_LIB_TABLE_LEXER;      // outside namespace SCH, since make_lexer() Functions.cmake can't do namespace
class OUTPUTFORMATTER;

namespace SCH {

class LPID;
class PART;

/**
 * Class LIB_TABLE
 * holds LIB_TABLE::ROW records, and can be searched in a very high speed
 * way based on logical library name.
 *
 * @author Dick Hollenbeck
 */
class LIB_TABLE
{
public:

    /**
     * Class ROW
     * holds a record identifying a LIB in the LIB_TABLE.
     */
    class ROW
    {
        friend class LIB_TABLE;

    public:

        /**
         * Function GetLogicalName
         * returns the logical name of this library table entry.
         */
        const STRING&   GetLogicalName() const
        {
            return logicalName;
        }

        /**
         * Function GetType
         * returns the type of LIB represented by this record.
         */
        const STRING&   GetType() const
        {
            return libType;
        }

        /**
         * Function GetFullURI
         * returns the full location specifying URI for the LIB.
         */
        const STRING&   GetFullURI() const
        {
            return fullURI;
        }

        /**
         * Function GetOptions
         * returns the options string, which may hold a password or anything else needed to
         * instantiate the underlying LIB_SOURCE.
         */
        const STRING&   GetOptions() const
        {
            return options;
        }

        ~ROW()
        {
            delete lib;
        }

        /**
         * Function Format
         * serializes this object as utf8 text to an OUTPUTFORMATTER, and tries to
         * make it look good using multiple lines and indentation.
         * @param out is an OUTPUTFORMATTER
         * @param nestLevel is the indentation level to base all lines of the output.
         *   Actual indentation will be 2 spaces for each nestLevel.
         */
        void Format( OUTPUTFORMATTER* out, int nestLevel ) const
            throw( IO_ERROR );

    protected:

        ROW( LIB_TABLE* aOwner ) :
            owner( aOwner ),
            lib( 0 )
        {}

        /**
         * Function SetLogicalName
         * changes the logical name of this library, useful for an editor.
         */
        void    SetLogicalName( const STRING& aLogicalName )
        {
            logicalName = aLogicalName;
        }

        /**
         * Function SetType
         * changes the type represented by this record.
         */
        void    SetType( const STRING& aType )
        {
            libType = aType;
        }

        /**
         * Function SetFullURI
         * changes the full URI for the library, useful from a library table editor.
         */
        void    SetFullURI( const STRING& aFullURI )
        {
            fullURI = aFullURI;
        }

        /**
         * Function SetOptions
         * changes the options string for this record, and is useful from
         * the library table editor.
         */
        void    SetOptions( const STRING& aOptions )
        {
            options = aOptions;
        }

    private:
        LIB_TABLE*  owner;
        STRING      logicalName;
        STRING      libType;
        STRING      fullURI;
        STRING      options;

        LIB*        lib;
    };


    /**
     * Constructor LIB_TABLE
     * builds a library table from an s-expression form of the library table.
     * @param aFallBackTable is another LIB_TABLE which is searched only when
     *   a record is not found in this table.  No ownership is taken of aFallBackTable.
     */
    LIB_TABLE( LIB_TABLE* aFallBackTable = NULL );

    /**
     * Function Parse
     * fills this object from information in the input stream \a aLexer, which
     * is a DSNLEXER customized for the grammar needed to describe instances of this object.
     * The entire textual element spec is <br>
     * (lib_table (logical _yourfieldname_)(value _yourvalue_) visible))
     *
     * <pre>
     * (lib_table
     *   (lib (logical "LOGICAL")(type "TYPE")(fullURI "FULL_URI")(options "OPTIONS"))
     *   (lib (logical "LOGICAL")(type "TYPE")(fullURI "FULL_URI")(options "OPTIONS"))
     *   (lib (logical "LOGICAL")(type "TYPE")(fullURI "FULL_URI")(options "OPTIONS"))
     * </pre>
     *
     * When this function is called, the input token stream given by \a aLexer
     * is assumed to be positioned at the '^' in the following example, i.e. just after the
     * identifying keyword and before the content specifying stuff.<br>
     * (lib_table ^ (....) )
     *
     * @param aSpec is the input token stream of keywords and symbols.
     */
    void Parse( SCH_LIB_TABLE_LEXER* aLexer ) throw( IO_ERROR );

    /**
     * Function Format
     * serializes this object as utf8 text to an OUTPUTFORMATTER, and tries to
     * make it look good using multiple lines and indentation.
     * @param out is an OUTPUTFORMATTER
     * @param nestLevel is the indentation level to base all lines of the output.
     *   Actual indentation will be 2 spaces for each nestLevel.
     */
    void Format( OUTPUTFORMATTER* out, int nestLevel ) const throw( IO_ERROR );

    /**
     * Function GetPart
     * finds and loads a PART, and parses it.  As long as the part is
     * accessible in any LIB_SOURCE, opened or not opened, this function
     * will find it and load it into its containing LIB, even if that means
     * having to open a LIB in this table that was not previously opened.
     */
    PART* GetPart( const LPID& aLogicalPartID ) throw( IO_ERROR );

    /**
     * Function GetLogicalLibs
     * returns the logical library names, all of them that are in pertinent to
     * a lookup done on this LIB_TABLE.
     */
    STRINGS GetLogicalLibs();

    //----<read accessors>----------------------------------------------------
    // the returning of a const STRING* tells if not found, but might be too
    // promiscuous?

    /**
     * Function GetLibURI
     * returns the full library path from a logical library name.
     * @param aLogicalLibraryName is the short name for the library of interest.
     * @return const STRING* - or NULL if not found.
     */
    const STRING* GetLibURI( const STRING& aLogicalLibraryName ) const
    {
        const ROW* row = FindRow( aLogicalLibraryName );
        return row ? &row->fullURI : 0;
    }

    /**
     * Function GetLibType
     * returns the type of a logical library.
     * @param aLogicalLibraryName is the short name for the library of interest.
     * @return const STRING* - or NULL if not found.
     */
    const STRING* GetLibType( const STRING& aLogicalLibraryName ) const
    {
        const ROW* row = FindRow( aLogicalLibraryName );
        return row ? &row->libType : 0;
    }

    /**
     * Function GetLibOptions
     * returns the options string for \a aLogicalLibraryName.
     * @param aLogicalLibraryName is the short name for the library of interest.
     * @return const STRING* - or NULL if not found.
     */
    const STRING* GetLibOptions( const STRING& aLogicalLibraryName ) const
    {
        const ROW* row = FindRow( aLogicalLibraryName );
        return row ? &row->options : 0;
    }

    //----</read accessors>---------------------------------------------------

#if defined(DEBUG)
    /// implement the tests in here so we can honor the priviledge levels of the
    /// accessors, something difficult to do from int main(int, char**)
    void Test();
#endif

protected:  // only a table editor can use these

    /**
     * Function InsertRow
     * adds aRow if it does not already exist or if doReplace is true.  If doReplace
     * is not true and the key for aRow already exists, the function fails and returns false.
     * The key for the table is the logicalName, and all in this table must be unique.
     * @param aRow is the new row to insert, or to forcibly add if doReplace is true.
     * @param doReplace if true, means insert regardless of whether aRow's key already
     *  exists.  If false, then fail if the key already exists.
     * @return bool - true if the operation succeeded.
     */
    bool InsertRow( std::auto_ptr<ROW>& aRow, bool doReplace = false );

    /**
     * Function FindRow
     * returns a ROW* if aLogicalName is found in this table or in fallBack, else NULL.
     */
    const ROW* FindRow( const STRING& aLogicalName ) const;

private:

    typedef boost::ptr_map<STRING, ROW>     ROWS;
    typedef ROWS::iterator                  ROWS_ITER;
    typedef ROWS::const_iterator            ROWS_CITER;
//  typedef std::pair<ROWS_ITER, bool>      ROW_PAIR;

    ROWS        rows;
    LIB_TABLE*  fallBack;
};

}   // namespace SCH

#endif  // SCH_LIB_TABLE_H_
