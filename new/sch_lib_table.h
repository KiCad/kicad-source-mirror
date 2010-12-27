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
#include <boost/ptr_container/ptr_set.hpp>
#include <sch_lib.h>

class SCH_LIB_TABLE_LEXER;      // outside namespace SCH, since make_lexer() Functions.cmake can't do namespace

namespace SCH {

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

        bool operator<( const ROW& other ) const
        {
            return logicalName < other.logicalName;
        }

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

#if defined(DEBUG)
        void Show() const
        {
            printf( "(lib (logical \"%s\")(type \"%s\")(full_uri \"%s\")(options \"%s\"))\n",
                logicalName.c_str(), libType.c_str(), fullURI.c_str(), options.c_str() );
        }
#endif

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

#if defined(DEBUG)
    void Show() const
    {
        printf("(lib_table\n" );
        for( ROWS_CITER it = rows.begin();  it != rows.end();  ++it )
            it->Show();
        printf(")\n" );
    }
#endif

    typedef boost::ptr_set<ROW>     ROWS;
    typedef ROWS::iterator          ROWS_ITER;
    typedef ROWS::const_iterator    ROWS_CITER;

private:

    ROWS    rows;
};

}   // namespace SCH

#endif  // SCH_LIB_TABLE_H_
