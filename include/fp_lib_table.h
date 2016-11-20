/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012-2016 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2012-2016 KiCad Developers, see change_log.txt for contributors.
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

#include <lib_table_base.h>
#include <io_mgr.h>

class MODULE;
class FP_TBL_MODEL;

/**
 * Class FP_LIB_TABLE_ROW
 *
 * holds a record identifying a library accessed by the appropriate footprint library #PLUGIN
 * object in the #FP_LIB_TABLE.
 */
class FP_LIB_TABLE_ROW : public LIB_TABLE_ROW
{
    friend class FP_LIB_TABLE;

public:
    typedef IO_MGR::PCB_FILE_T LIB_T;

    FP_LIB_TABLE_ROW( const wxString& aNick, const wxString& aURI, const wxString& aType,
                      const wxString& aOptions, const wxString& aDescr = wxEmptyString ) :
        LIB_TABLE_ROW( aNick, aURI, aOptions, aDescr )
    {
        SetType( aType );
    }

    FP_LIB_TABLE_ROW() :
        type( IO_MGR::KICAD )
    {
    }

    bool operator==( const FP_LIB_TABLE_ROW& aRow ) const;

    bool operator!=( const FP_LIB_TABLE_ROW& aRow ) const   { return !( *this == aRow ); }

    /**
     * Function GetType
     *
     * returns the type of footprint library table represented by this row.
     */
    const wxString GetType() const override         { return IO_MGR::ShowType( type ); }

    /**
     * Function SetType
     *
     * changes the type represented by this row.
     */
    void SetType( const wxString& aType ) override;

protected:
    FP_LIB_TABLE_ROW( const FP_LIB_TABLE_ROW& aRow ) :
        LIB_TABLE_ROW( aRow ),
        type( aRow.type )
    {
    }

private:

    virtual LIB_TABLE_ROW* do_clone() const override
    {
        return new FP_LIB_TABLE_ROW( *this );
    }

    void setPlugin( PLUGIN* aPlugin )
    {
        plugin.set( aPlugin );
    }

    PLUGIN::RELEASER  plugin;
    LIB_T             type;
};


class FP_LIB_TABLE : public LIB_TABLE
{
public:

    virtual void Parse( LIB_TABLE_LEXER* aLexer ) throw() override;

    virtual void Format( OUTPUTFORMATTER* out, int nestLevel ) const throw() override;

    /**
     * Constructor FP_LIB_TABLE
     *
     * builds a footprint library table by pre-pending this table fragment in front of
     * @a aFallBackTable.  Loading of this table fragment is done by using Parse().
     *
     * @param aFallBackTable is another FP_LIB_TABLE which is searched only when
     *                       a row is not found in this table.  No ownership is
     *                       taken of aFallBackTable.
     */
    FP_LIB_TABLE( FP_LIB_TABLE* aFallBackTable = NULL );

    /**
     * Function FindRow
     *
     * returns an FP_LIB_TABLE_ROW if \a aNickName is found in this table or in any chained
     * fallBack table fragment.  The #PLUGIN is loaded and attached to the "plugin" field
     * of the #FP_LIB_TABLE_ROW if not already loaded.
     *
     * @throw IO_ERROR if \a aNickName cannot be found.
     */
    const FP_LIB_TABLE_ROW* FindRow( const wxString& aNickName ) throw( IO_ERROR );

    //-----<PLUGIN API SUBSET, REBASED ON aNickname>---------------------------

    /**
     * Function FootprintEnumerate
     * returns a list of footprint names contained within the library given by
     * @a aNickname.
     *
     * @param aNickname is a locator for the "library", it is a "name" in LIB_TABLE_ROW.
     *
     * @return wxArrayString - is the array of available footprint names inside a library
     *
     * @throw IO_ERROR if the library cannot be found, or footprint cannot be loaded.
     */
    wxArrayString FootprintEnumerate( const wxString& aNickname );

    /**
     * Function FootprintLoad
     *
     * loads a footprint having @a aFootprintName from the library given by @a aNickname.
     *
     * @param aNickname is a locator for the "library", it is a "name" in #LIB_TABLE_ROW
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
     *
     * will write @a aFootprint to an existing library given by @a aNickname.
     * If a footprint by the same name already exists, it is replaced.
     *
     * @param aNickname is a locator for the "library", it is a "name" in LIB_TABLE_ROW
     *
     * @param aFootprint is what to store in the library. The caller continues to own the
     *                   footprint after this call.
     *
     * @param aOverwrite when true means overwrite any existing footprint by the same name,
     *                   else if false means skip the write and return SAVE_SKIPPED.
     *
     * @return SAVE_T - SAVE_OK or SAVE_SKIPPED.  If error saving, then IO_ERROR is thrown.
     *
     * @throw IO_ERROR if there is a problem saving.
     */
    SAVE_T FootprintSave( const wxString& aNickname, const MODULE* aFootprint,
                          bool aOverwrite = true );

    /**
     * Function FootprintDelete
     *
     * deletes the @a aFootprintName from the library given by @a aNickname.
     *
     * @param aNickname is a locator for the "library", it is a "name" in LIB_TABLE_ROW.
     *
     * @param aFootprintName is the name of a footprint to delete from the specified library.
     *
     * @throw IO_ERROR if there is a problem finding the footprint or the library, or deleting it.
     */
    void FootprintDelete( const wxString& aNickname, const wxString& aFootprintName );

    /**
     * Function IsFootprintLibWritable
     *
     * returns true if the library given by @a aNickname is writable.  (Often
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
     * @param aFootprintId the [nickname] & footprint name of the footprint to load.
     *
     * @return  MODULE* - if found caller owns it, else NULL if not found.
     *
     * @throw   IO_ERROR if the library cannot be found or read.  No exception
     *          is thrown in the case where aFootprintName cannot be found.
     * @throw   PARSE_ERROR if @a aFootprintId is not parsed OK.
     */
    MODULE* FootprintLoadWithOptionalNickname( const LIB_ID& aFootprintId )
        throw( IO_ERROR, PARSE_ERROR, boost::interprocess::lock_exception );

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
    static bool LoadGlobalTable( FP_LIB_TABLE& aTable )
        throw (IO_ERROR, PARSE_ERROR, boost::interprocess::lock_exception );

    /**
     * Function GetGlobalTableFileName
     *
     * @return the platform specific global footprint library path and file name.
     */
    static wxString GetGlobalTableFileName();

    /**
     * Function GlobalPathEnvVarVariableName
     *
     * returns the name of the environment variable used to hold the directory of
     * locally installed "KiCad sponsored" system footprint libraries.  These can
     * be either legacy or pretty format.  The only thing special about this
     * particular environment variable is that it is set automatically by
     * KiCad on program start up, <b>if</b> it is not set already in the environment.
     */
    static const wxString GlobalPathEnvVariableName();
};


extern FP_LIB_TABLE GFootprintTable;        // KIFACE scope.

#endif  // FP_LIB_TABLE_H_
