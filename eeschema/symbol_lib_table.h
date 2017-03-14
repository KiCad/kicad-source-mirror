/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016-2017 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2016-2017 KiCad Developers, see change_log.txt for contributors.
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

#ifndef _SYMBOL_LIB_TABLE_H_
#define _SYMBOL_LIB_TABLE_H_

#include <lib_table_base.h>
#include <sch_io_mgr.h>

class LIB_PART;

/**
 * Class SYMBOL_LIB_TABLE_ROW
 *
 * holds a record identifying a library accessed by the appropriate footprint library #PLUGIN
 * object in the #SYMBOL_LIB_TABLE.
 */
class SYMBOL_LIB_TABLE_ROW : public LIB_TABLE_ROW
{
    friend class SYMBOL_LIB_TABLE;

public:
    typedef SCH_IO_MGR::SCH_FILE_T LIB_T;

    SYMBOL_LIB_TABLE_ROW( const wxString& aNick, const wxString& aURI, const wxString& aType,
                          const wxString& aOptions, const wxString& aDescr = wxEmptyString ) :
        LIB_TABLE_ROW( aNick, aURI, aOptions, aDescr )
    {
        SetType( aType );
    }

    SYMBOL_LIB_TABLE_ROW() :
        type( SCH_IO_MGR::SCH_KICAD )
    {
    }

    bool operator==( const SYMBOL_LIB_TABLE_ROW& aRow ) const;

    bool operator!=( const SYMBOL_LIB_TABLE_ROW& aRow ) const   { return !( *this == aRow ); }

    /**
     * Function GetType
     *
     * returns the type of symbol library table represented by this row.
     */
    const wxString GetType() const override         { return SCH_IO_MGR::ShowType( type ); }

    /**
     * Function SetType
     *
     * changes the type represented by this row.
     */
    void SetType( const wxString& aType ) override;

protected:
    SYMBOL_LIB_TABLE_ROW( const SYMBOL_LIB_TABLE_ROW& aRow ) :
        LIB_TABLE_ROW( aRow ),
        type( aRow.type )
    {
    }

private:

    virtual LIB_TABLE_ROW* do_clone() const override
    {
        return new SYMBOL_LIB_TABLE_ROW( *this );
    }

    void setPlugin( SCH_PLUGIN* aPlugin )
    {
        plugin.set( aPlugin );
    }

    SCH_PLUGIN::SCH_PLUGIN_RELEASER  plugin;
    LIB_T                            type;
};


class SYMBOL_LIB_TABLE : public LIB_TABLE
{
public:

    virtual void Parse( LIB_TABLE_LEXER* aLexer ) override;

    virtual void Format( OUTPUTFORMATTER* aOutput, int aIndentLevel ) const override;

    /**
     * Constructor SYMBOL_LIB_TABLE
     *
     * builds a footprint library table by pre-pending this table fragment in front of
     * @a aFallBackTable.  Loading of this table fragment is done by using Parse().
     *
     * @param aFallBackTable is another SYMBOL_LIB_TABLE which is searched only when
     *                       a row is not found in this table.  No ownership is
     *                       taken of aFallBackTable.
     */
    SYMBOL_LIB_TABLE( SYMBOL_LIB_TABLE* aFallBackTable = NULL );

    /**
     * Function FindRow
     *
     * returns an SYMBOL_LIB_TABLE_ROW if \a aNickName is found in this table or in any chained
     * fallBack table fragment.  The #PLUGIN is loaded and attached to the "plugin" field
     * of the #SYMBOL_LIB_TABLE_ROW if not already loaded.
     *
     * @throw IO_ERROR if \a aNickName cannot be found.
     */
    const SYMBOL_LIB_TABLE_ROW* FindRow( const wxString& aNickName ) throw( IO_ERROR );

    //-----<PLUGIN API SUBSET, REBASED ON aNickname>---------------------------

    /**
     * Function EnumerateSymbolLib
     *
     * returns a list of symbol alias names contained within the library given by @a aNickname.
     *
     * @param aNickname is a locator for the "library", it is a "name" in LIB_TABLE_ROW.
     * @param aAliasNames is a reference to an array for the alias names
     *
     * @throw IO_ERROR if the library cannot be found or loaded.
     */
    void EnumerateSymbolLib( const wxString& aNickname, wxArrayString& aAliasNames );

    /**
     * Function LoadSymbol
     *
     * loads a #LIB_ALIAS having @a aAliasName from the library given by @a aNickname.
     * The actual symbol can be retreaved from the LIB_ALIAS::GetPart() method.
     *
     * @param aNickname is a locator for the "library", it is a "name" in #LIB_TABLE_ROW
     *
     * @param aAliasName is the name of the #LIB_ALIAS to load.
     *
     * @return  LIB_ALIAS* - if found we own it, else NULL if not found.
     *
     * @throw   IO_ERROR if the library cannot be found or read.  No exception
     *                   is thrown in the case where aAliasName cannot be found.
     */
    LIB_ALIAS* LoadSymbol( const wxString& aNickname, const wxString& aAliasName );

    /**
     * Enum SAVE_T
     *
     * is the set of return values from SaveSymbol() below.
     */
    enum SAVE_T
    {
        SAVE_OK,
        SAVE_SKIPPED,
    };

    /**
     * Function SaveSymbol
     *
     * will write @a aSymbol to an existing library given by @a aNickname.  If a #LIB_PART
     * by the same name already exists or there are any conflicting alias names, the new
     * #LIB_PART will silently overwrite any existing aliases and/or part becaue libraries
     * cannot have duplicate alias names.  It is the responsibility of the caller to check
     * the library for conflicts before saving.
     *
     * @param aNickname is a locator for the "library", it is a "name" in LIB_TABLE_ROW
     *
     * @param aSymbol is what to store in the library. The library owns the symbol after this
     *                call.
     *
     * @param aOverwrite when true means overwrite any existing symbol by the same name,
     *                   else if false means skip the write and return SAVE_SKIPPED.
     *
     * @return SAVE_T - SAVE_OK or SAVE_SKIPPED.  If error saving, then IO_ERROR is thrown.
     *
     * @throw IO_ERROR if there is a problem saving.
     */
    SAVE_T SaveSymbol( const wxString& aNickname, const LIB_PART* aSymbol,
                       bool aOverwrite = true );

    /**
     * Function DeleteSymbol
     *
     * deletes the @a aSymbolName from the library given by @a aNickname.
     *
     * @param aNickname is a locator for the "library", it is a "name" in LIB_TABLE_ROW.
     *
     * @param aSymbolName is the name of a symbol to delete from the specified library.
     *
     * @throw IO_ERROR if there is a problem finding the footprint or the library, or deleting it.
     */
    void DeleteSymbol( const wxString& aNickname, const wxString& aSymboltName );

    /**
     * Function DeleteAlias
     *
     * deletes @a aAliasName from the library at @a aLibraryPath.
     *
     * If @a aAliasName refers the the root #LIB_PART object, the part is renamed to
     * the next or previous #LIB_ALIAS in the #LIB_PART if one exists.  If the #LIB_ALIAS
     * is the last alias referring to the root #LIB_PART, the #LIB_PART is also removed
     * from the library.
     *
     * @param aNickname is a locator for the "library", it is a "name" in LIB_TABLE_ROW
     *
     * @param aAliasName is the name of a #LIB_ALIAS to delete from the specified library.
     *
     * @throw IO_ERROR if there is a problem finding the alias or the library or deleting it.
     */
    void DeleteAlias( const wxString& aNickname, const wxString& aAliasName );

    /**
     * Function IsSymbolLibWritable
     *
     * returns true if the library given by @a aNickname is writable.  (Often
     * system libraries are read only because of where they are installed.)
     *
     * @throw IO_ERROR if no library at @a aNickname exists.
     */
    bool IsSymbolLibWritable( const wxString& aNickname );

    void DeleteSymbolLib( const wxString& aNickname );

    void CreateSymbolLib( const wxString& aNickname );

    //-----</PLUGIN API SUBSET, REBASED ON aNickname>---------------------------

    /**
     * Function LoadSymboldWithOptionalNickname
     * loads a #LIB_PART having @a aFootprintId with possibly an empty librarynickname.
     *
     * @param aId the library  nickname and name of the symbol to load.
     *
     * @return  LIB_PART* - if found the library owns it, else NULL if not found.
     *
     * @throw   IO_ERROR if the library cannot be found or read.  No exception
     *                   is thrown in the case where aId cannot be found.
     * @throw   PARSE_ERROR if @a atId is not parsed OK.
     */
    LIB_ALIAS* LoadSymbolWithOptionalNickname( const LIB_ID& aId )
        throw( IO_ERROR, PARSE_ERROR, boost::interprocess::lock_exception );

    /**
     * Function LoadGlobalTable
     *
     * loads the global symbol library table into \a aTable.
     *
     * This probably should be move into the application object when KiCad is changed
     * to a single process application.  This is the least painful solution for the
     * time being.
     *
     * @param aTable the #SYMBOL_LIB_TABLE object to load.
     * @return true if the global library table exists and is loaded properly.
     * @throw IO_ERROR if an error occurs attempting to load the symbol library table.
     */
    static bool LoadGlobalTable( SYMBOL_LIB_TABLE& aTable )
        throw (IO_ERROR, PARSE_ERROR, boost::interprocess::lock_exception );

    /**
     * Function GetGlobalTableFileName
     *
     * @return the platform specific global symbol library path and file name.
     */
    static wxString GetGlobalTableFileName();

    /**
     * Function GlobalPathEnvVarVariableName
     *
     * returns the name of the environment variable used to hold the directory of locally
     * installed "KiCad sponsored" system symbol libraries.  These can be either legacy
     * or sweet format.  The only thing special about this particular environment variable
     * is that it is set automatically by KiCad on program start up, <b>if</b> it is not
     * set already in the environment.
     */
    static const wxString GlobalPathEnvVariableName();

    static SYMBOL_LIB_TABLE& GetGlobalLibTable() { return m_globalLibTable; }

private:
    static SYMBOL_LIB_TABLE m_globalLibTable;   // There can be only one.
};


#endif  // _SYMBOL_LIB_TABLE_H_
