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

#ifndef _FP_LIB_TABLE_H_
#define _FP_LIB_TABLE_H_

#include <macros.h>

#include <vector>
#include <map>

#include <wx/grid.h>
#include <fp_lib_id.h>


class OUTPUTFORMATTER;


class MODULE;

/**
 * Class FP_LIB_TABLE
 * holds FP_LIB_TABLE::ROW records, and can be searched based on logical library name.
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
class FP_LIB_TABLE : public wxGridTableBase
{
public:

    /**
     * Class ROW
     * holds a record identifying a footprint library accessed by the appropriate #PLUGIN
     * object in the #FP_LIB_TABLE.
     */
    class ROW
    {
        friend class FP_LIB_TABLE;

    public:

        /**
         * Function GetNickName
         * returns the short name of this library table row.
         */
        const wxString& GetNickName() const
        {
            return nickName;
        }

        /**
         * Function GetType
         * returns the type of LIB represented by this record.
         */
        const wxString& GetType() const
        {
            return type;
        }

        /**
         * Function GetFullURI
         * returns the full location specifying URI for the LIB.
         */
        const wxString& GetFullURI() const
        {
            return uri;
        }

        /**
         * Function GetOptions
         * returns the options string, which may hold a password or anything else needed to
         * instantiate the underlying LIB_SOURCE.
         */
        const wxString& GetOptions() const
        {
            return options;
        }

        ~ROW()
        {
            // delete lib;
        }

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

    protected:

        ROW( FP_LIB_TABLE* aOwner ) :
            owner( aOwner )
            // lib( 0 )
        {}

        /**
         * Function SetNickName
         * changes the logical name of this library, useful for an editor.
         */
        void SetNickName( const wxString& aNickName )
        {
            nickName = aNickName;
        }

        /**
         * Function SetType
         * changes the type represented by this record.
         */
        void SetType( const wxString& aType )
        {
            type = aType;
        }

        /**
         * Function SetFullURI
         * changes the full URI for the library, useful from a library table editor.
         */
        void SetFullURI( const wxString& aFullURI )
        {
            uri = aFullURI;
        }

        /**
         * Function SetOptions
         * changes the options string for this record, and is useful from
         * the library table editor.
         */
        void SetOptions( const wxString& aOptions )
        {
            options = aOptions;
        }

    private:
        FP_LIB_TABLE*   owner;
        wxString        nickName;
        wxString        type;
        wxString        uri;
        wxString        options;

        /*
        PLUGIN*         lib;        ///< ownership of the loaded LIB is here
        */
    };


    /**
     * Constructor FP_LIB_TABLE
     * builds a library table by pre-pending this table fragment in front of
     * @a aFallBackTable.  Loading of this table fragment is done by using Parse().
     *
     * @param aFallBackTable is another FP_LIB_TABLE which is searched only when
     *                       a record is not found in this table.  No ownership is
     *                       taken of aFallBackTable.
     */
    FP_LIB_TABLE( FP_LIB_TABLE* aFallBackTable = NULL );

    /**
     * Function Parse
     * fills this table fragment from information in the input stream \a aParser, which
     * is a DSNLEXER customized for the grammar needed to describe instances of this object.
     * The entire textual element spec is <br>
     *
     * <pre>
     * (fp_lib_table
     *   (lib (name LOGICAL)(type TYPE)(uri FULL_URI)(options OPTIONS))
     *   (lib (name LOGICAL)(type TYPE)(uri FULL_URI)(options OPTIONS))
     *   (lib (name LOGICAL)(type TYPE)(uri FULL_URI)(options OPTIONS))
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
     * Function Format
     * serializes this object as utf8 text to an #OUTPUTFORMATTER, and tries to
     * make it look good using multiple lines and indentation.
     *
     * @param out is an #OUTPUTFORMATTER
     * @param nestLevel is the indentation level to base all lines of the output.
     *   Actual indentation will be 2 spaces for each nestLevel.
     */
    void Format( OUTPUTFORMATTER* out, int nestLevel ) const throw( IO_ERROR );

#if 0
    /**
     * Function LookupPart
     * finds and loads a MODULE, and parses it.  As long as the part is
     * accessible in any LIB_SOURCE, opened or not opened, this function
     * will find it and load it into its containing LIB, even if that means
     * having to open a LIB in this table that was not previously opened.
     *
     * @param aFootprintId The fully qualified name of the footprint to look up.
     *
     * @return MODULE* - this will never be NULL, and no ownership is transferred because
     *  all MODULEs live in LIBs.  You only get to point to them in some LIB. If the MODULE
     *  cannot be found, then an exception is thrown.
     *
     * @throw IO_ERROR if any problem occurs or if the footprint cannot be found.
     */
    MODULE* LookupFootprint( const FP_LIB_ID& aFootprintId ) throw( IO_ERROR );
#endif


    /**
     * Function GetLogicalLibs
     * returns the logical library names, all of them that are pertinent to
     * a lookup done on this FP_LIB_TABLE.
     */
    std::vector<wxString> GetLogicalLibs();


    //-----<wxGridTableBase overloads>-------------------------------------------

    int         GetNumberRows () { return rows.size(); }
    int         GetNumberCols () { return 4; }

    wxString    GetValue( int aRow, int aCol )
    {
        if( unsigned( aRow ) < rows.size() )
        {
            const ROW&  r  = rows[aRow];

            switch( aCol )
            {
            case 0:     return r.GetNickName();
            case 1:     return r.GetType();
            case 2:     return r.GetFullURI();
            case 3:     return r.GetOptions();
            default:
                ;       // fall thru to wxEmptyString
            }
        }

        return wxEmptyString;
    }

    void    SetValue( int aRow, int aCol, const wxString &aValue )
    {
        if( aCol == 0 )
        {
            // when the nickname is changed, there's careful work to do, including
            // ensuring uniqueness of the nickname.
        }
        else if( unsigned( aRow ) < rows.size() )
        {
            ROW&  r  = rows[aRow];

            switch( aCol )
            {
            case 1:     return r.SetType( aValue  );
            case 2:     return r.SetFullURI( aValue );
            case 3:     return r.SetOptions( aValue );
            }
        }
    }

    //-----</wxGridTableBase overloads>------------------------------------------


    //----<read accessors>----------------------------------------------------
    // the returning of a const wxString* tells if not found, but might be too
    // promiscuous?

    /**
     * Function GetURI
     * returns the full library path from a logical library name.
     * @param aLogicalLibraryName is the short name for the library of interest.
     * @return const wxString* - or NULL if not found.
     */
    const wxString* GetURI( const wxString& aLogicalLibraryName ) const
    {
        const ROW* row = FindRow( aLogicalLibraryName );
        return row ? &row->uri : 0;
    }

    /**
     * Function GetType
     * returns the type of a logical library.
     * @param aLogicalLibraryName is the short name for the library of interest.
     * @return const wxString* - or NULL if not found.
     */
    const wxString* GetType( const wxString& aLogicalLibraryName ) const
    {
        const ROW* row = FindRow( aLogicalLibraryName );
        return row ? &row->type : 0;
    }

    /**
     * Function GetLibOptions
     * returns the options string for \a aLogicalLibraryName.
     * @param aLogicalLibraryName is the short name for the library of interest.
     * @return const wxString* - or NULL if not found.
     */
    const wxString* GetLibOptions( const wxString& aLogicalLibraryName ) const
    {
        const ROW* row = FindRow( aLogicalLibraryName );
        return row ? &row->options : 0;
    }

    //----</read accessors>---------------------------------------------------

#if 1 || defined(DEBUG)
    /// implement the tests in here so we can honor the privilege levels of the
    /// accessors, something difficult to do from int main(int, char**)
    void Test();
#endif

protected:  // only a table editor can use these

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
     * returns a #ROW* if aNickName is found in this table or in any chained
     * fallBack table fragment, else NULL.
     */
    ROW* FindRow( const wxString& aNickName ) const;

private:

#if 0   // lets see what we need.
    /**
     * Function lookupLib
     * finds or loads a LIB based on @a aLogicalPartID or @a aFallBackLib.
     * If the LIB is already loaded then it is returned as is, else it is loaded.
     *
     * @param aLogicalPartID holds the partName and may also hold the logicalLibName.  If
     *  logicalLibName is empty, then @a aFallBackLib should not be NULL.
     *
     * @param aFallBackLib is used only if aLogicalPartID has an empty logicalLibName.
     *  This is for the case when an LPID has no logicalLibName because the LPID is using
     *  a partName from the same LIB as was the referring content.
     *
     * @return PLUGIN* - this will never be NULL, and no ownership is transfered because
     *  all LIBs live in the FP_LIB_TABLEs.  You only get to point to them in some FP_LIB_TABLE.
     *  If the LIB cannot be found, then an exception is thrown.
     *
     * @throw IO_ERROR if any problem occurs or if the LIB cannot be found or cannot be loaded.
     */
    PLUGIN* lookupLib( const FP_LIB_ID& aLogicalPartID ) throw( IO_ERROR );

    /**
     * Function loadLib
     * loads a LIB using information in @a aRow.  Call only if LIB not
     * already loaded.
     */
    void loadLib( ROW* aRow ) throw( IO_ERROR );
#endif

    typedef std::vector<ROW>            ROWS;
    typedef ROWS::iterator              ROWS_ITER;
    typedef ROWS::const_iterator        ROWS_CITER;

    ROWS           rows;

    /// this is a non-owning index into the ROWS table
    typedef std::map<wxString,int>      INDEX;              // "int" is std::vector array index
    typedef INDEX::iterator             INDEX_ITER;
    typedef INDEX::const_iterator       INDEX_CITER;
    typedef INDEX::value_type           INDEX_VALUE;


    /// the particular key is the nickName within each row.
    INDEX           nickIndex;

    FP_LIB_TABLE*  fallBack;
};

#endif  // _FP_LIB_TABLE_H_
