/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Wayne Stambaugh <stambaughw@gmail.com>
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

#ifndef _SYMBOL_LIB_TABLE_H_
#define _SYMBOL_LIB_TABLE_H_

#include <lib_table_base.h>
#include <sch_io/sch_io.h>
#include <sch_io/sch_io_mgr.h>
#include <lib_id.h>

//class LIB_SYMBOL;
class SYMBOL_LIB_TABLE_GRID;
class DIALOG_SYMBOL_LIB_TABLE;


/**
 * Hold a record identifying a symbol library accessed by the appropriate symbol library
 * #SCH_IO object in the #SYMBOL_LIB_TABLE.
 */
class SYMBOL_LIB_TABLE_ROW : public LIB_TABLE_ROW
{
public:
    typedef SCH_IO_MGR::SCH_FILE_T LIB_T;

    SYMBOL_LIB_TABLE_ROW( const wxString& aNick, const wxString& aURI, const wxString& aType,
                          const wxString& aOptions = wxEmptyString,
                          const wxString& aDescr = wxEmptyString ) :
        LIB_TABLE_ROW( aNick, aURI, aOptions, aDescr )
    {
        SetType( aType );
        SetEnabled( true );
    }

    SYMBOL_LIB_TABLE_ROW() :
        type( SCH_IO_MGR::SCH_KICAD )
    {
        SetEnabled( true );
    }

    bool operator==( const SYMBOL_LIB_TABLE_ROW& aRow ) const;

    bool operator!=( const SYMBOL_LIB_TABLE_ROW& aRow ) const   { return !( *this == aRow ); }

    LIB_T SchLibType() const { return type; }

    /**
     * Return the type of symbol library table represented by this row.
     */
    const wxString GetType() const override         { return SCH_IO_MGR::ShowType( type ); }

    /**
     * Change the schematic plugin type represented by this row.
     */
    void SetType( const wxString& aType ) override;

    /**
     * Attempt to reload the library.
     *
     * @return true if a reload was required.
     * @throw IO_ERROR if the reload was unsuccessful.
     */
    bool Refresh() override;

    bool SupportsSubLibraries() const { return plugin ? plugin->SupportsSubLibraries() : false; }

    bool SupportsSettingsDialog() const override
    {
        // Only database libraries have dialog-configurable options at the moment
        return type == SCH_IO_MGR::SCH_FILE_T::SCH_DATABASE;
    }

    void ShowSettingsDialog( wxWindow* aWindow ) const override;

    void GetSubLibraryNames( std::vector<wxString>& aNames ) const;

    wxString GetSubLibraryDescription( const wxString& aName ) const;

    /**
     * @see SCH_IO::GetAvailableSymbolFields
     */
    void GetAvailableSymbolFields( std::vector<wxString>& aNames ) const
    {
        if( plugin )
            plugin->GetAvailableSymbolFields( aNames );
    }

    /**
     * @see SCH_IO::GetDefaultSymbolFields
     */
    void GetDefaultSymbolFields( std::vector<wxString>& aNames ) const
    {
        if( plugin )
            plugin->GetDefaultSymbolFields( aNames );
    }

    bool LibraryExists() const override
    {
        if( plugin )
            return plugin->CanReadLibrary( GetFullURI( true ) );

        return false;
    }

    SCH_IO_MGR::SCH_FILE_T GetFileType() { return type; }

protected:
    SYMBOL_LIB_TABLE_ROW( const SYMBOL_LIB_TABLE_ROW& aRow ) :
        LIB_TABLE_ROW( aRow ),
        type( aRow.type )
    {
        SetEnabled( aRow.GetIsEnabled() );
    }

private:
    friend class SYMBOL_LIB_TABLE;

    virtual LIB_TABLE_ROW* do_clone() const override
    {
        return new SYMBOL_LIB_TABLE_ROW( *this );
    }

    void setPlugin( SCH_IO* aPlugin )
    {
        plugin.reset( aPlugin );
    }

    IO_RELEASER<SCH_IO> plugin;
    LIB_T               type;
};


class SYMBOL_LIB_TABLE : public LIB_TABLE
{
public:
    PROJECT::ELEM ProjectElementType() override { return PROJECT::ELEM::SYMBOL_LIB_TABLE; }

    static const char* PropPowerSymsOnly;
    static const char* PropNonPowerSymsOnly;

    virtual void Parse( LIB_TABLE_LEXER* aLexer ) override;

    virtual void Format( OUTPUTFORMATTER* aOutput, int aIndentLevel ) const override;

    /**
     * Build a symbol library table by pre-pending this table fragment in front of
     * @a aFallBackTable.  Loading of this table fragment is done by using Parse().
     *
     * @param aFallBackTable is another SYMBOL_LIB_TABLE which is searched only when
     *                       a row is not found in this table.  No ownership is
     *                       taken of aFallBackTable.
     */
    SYMBOL_LIB_TABLE( SYMBOL_LIB_TABLE* aFallBackTable = nullptr );

    /**
     * Return an SYMBOL_LIB_TABLE_ROW if \a aNickName is found in this table or in any chained
     * fallBack table fragment.
     *
     * The #SCH_IO is loaded and attached to the "plugin" fieldf the #SYMBOL_LIB_TABLE_ROW if
     * not already loaded.
     *
     * @param aNickName is the name of the row to find.
     * @param aCheckIfEnabled is a flag to verify if the table entry is enabled or disabled.
     * @return the row found or NULL if \a aNickName was not found.
     */
    SYMBOL_LIB_TABLE_ROW* FindRow( const wxString& aNickName, bool aCheckIfEnabled = false );

    int GetModifyHash();

    //-----<PLUGIN API SUBSET, REBASED ON aNickname>---------------------------

    /**
     * Return a list of symbol alias names contained within the library given by @a aNickname.
     *
     * @param aNickname is a locator for the "library", it is a "name" in LIB_TABLE_ROW.
     * @param aAliasNames is a reference to an array for the alias names.
     * @param aPowerSymbolsOnly is a flag to enumerate only power symbols.
     * @throw IO_ERROR if the library cannot be found or loaded.
     */
    void EnumerateSymbolLib( const wxString& aNickname, wxArrayString& aAliasNames,
                             bool aPowerSymbolsOnly = false );

    void LoadSymbolLib( std::vector<LIB_SYMBOL*>& aAliasList, const wxString& aNickname,
                        bool aPowerSymbolsOnly = false );

    /**
     * Load a #LIB_SYMBOL having @a aName from the library given by @a aNickname.
     *
     * @param aNickname is a locator for the "library", it is a "name" in #LIB_TABLE_ROW
     * @param aName is the name of the #LIB_SYMBOL to load.
     * @param aFlatten set to true to flatten derived parts.
     * @return the symbol alias if found or NULL if not found.
     * @throw IO_ERROR if the library cannot be found or read.  No exception
     *                 is thrown in the case where \a aNickname cannot be found.
     */
    LIB_SYMBOL* LoadSymbol( const wxString& aNickname, const wxString& aName );

    LIB_SYMBOL* LoadSymbol( const LIB_ID& aLibId )
    {
        return LoadSymbol( aLibId.GetLibNickname(), aLibId.GetLibItemName() );
    }

    /**
     * The set of return values from SaveSymbol() below.
     */
    enum SAVE_T
    {
        SAVE_OK,
        SAVE_SKIPPED,
    };

    /**
     * Write @a aSymbol to an existing library given by @a aNickname.
     *
     * If a #LIB_SYMBOL by the same name already exists or there are any conflicting alias
     * names, the new #LIB_SYMBOL will silently overwrite any existing aliases and/or part
     * because libraries cannot have duplicate alias names.  It is the responsibility of
     * the caller to check the library for conflicts before saving.
     *
     * @param aNickname is a locator for the "library", it is a "name" in LIB_TABLE_ROW
     * @param aSymbol is what to store in the library. The library owns the symbol after this
     *                call.
     * @param aOverwrite when true means overwrite any existing symbol by the same name,
     *                   else if false means skip the write and return SAVE_SKIPPED.
     * @return SAVE_T - SAVE_OK or SAVE_SKIPPED.  If error saving, then IO_ERROR is thrown.
     * @throw IO_ERROR if there is a problem saving the symbol.
     */
    SAVE_T SaveSymbol( const wxString& aNickname, const LIB_SYMBOL* aSymbol,
                       bool aOverwrite = true );

    /**
     * Deletes the @a aSymbolName from the library given by @a aNickname.
     *
     * @param aNickname is a locator for the "library", it is a "name" in LIB_TABLE_ROW.
     * @param aSymbolName is the name of a symbol to delete from the specified library.
     * @throw IO_ERROR if there is a problem finding the footprint or the library, or deleting it.
     */
    void DeleteSymbol( const wxString& aNickname, const wxString& aSymbolName );

    /**
     * Return true if the library given by @a aNickname is writable.
     *
     * It is possible that some symbols libraries are read only because of where they are
     * installed.
     *
     * @param aNickname is the library nickname in the symbol library table.
     * @throw IO_ERROR if no library at @a aNickname exists.
     */
    bool IsSymbolLibWritable( const wxString& aNickname );

    /**
     * Return true if the library given by @a aNickname was successfully loaded.
     *
     * @param aNickname is the library nickname in the symbol library table.
     * @throw IO_ERROR if no library at @a aNickname exists.
     */
    bool IsSymbolLibLoaded( const wxString& aNickname );

    void DeleteSymbolLib( const wxString& aNickname );

    void CreateSymbolLib( const wxString& aNickname );

    //-----</PLUGIN API SUBSET, REBASED ON aNickname>---------------------------

    /**
     * Load a #LIB_SYMBOL having @a aFootprintId with possibly an empty library nickname.
     *
     * @param aId the library nickname and name of the symbol to load.
     * @return  the library symbol if found (the library owns it) or NULL if not found.
     * @throw IO_ERROR if the library cannot be found or read.  No exception
     *                 is thrown in the case where aId cannot be found.
     * @throw PARSE_ERROR if @a aId is not parsed OK.
     */
    LIB_SYMBOL* LoadSymbolWithOptionalNickname( const LIB_ID& aId );

    /**
     * Load the global symbol library table into \a aTable.
     *
     * This probably should be move into the application object when KiCad is changed
     * to a single process application.  This is the least painful solution for the
     * time being.
     *
     * @param aTable the #SYMBOL_LIB_TABLE object to load.
     * @return true if the global library table exists and is loaded properly.
     * @throw IO_ERROR if an error occurs attempting to load the symbol library table.
     */
    static bool LoadGlobalTable( SYMBOL_LIB_TABLE& aTable );

    /**
     *
     * Fetch the global symbol library table file name.
     *
     * @return the platform specific global symbol library path and file name.
     */
    static wxString GetGlobalTableFileName();

    /**
     * Return the name of the environment variable used to hold the directory of locally
     * installed "KiCad sponsored" system symbol libraries.
     *
     * These can be either legacy or sweet format.  The only thing special about this
     * particular environment variable is that it is set automatically by KiCad on
     * program start up, <b>if</b> it is not set already in the environment.
     */
    static const wxString GlobalPathEnvVariableName();

    static SYMBOL_LIB_TABLE& GetGlobalLibTable();

    static const wxString GetSymbolLibTableFileName();

   /**
     * Compares this table against another.
     * This compares the row *contents* against each other.
     */
    bool operator==( const SYMBOL_LIB_TABLE& aOther ) const;

    bool operator!=( const SYMBOL_LIB_TABLE& aOther ) const { return !( *this == aOther ); }


private:
    friend class SYMBOL_LIB_TABLE_GRID;
    friend class PANEL_SYM_LIB_TABLE;

    static int m_modifyHash;     ///< helper for GetModifyHash()
};


#endif  // _SYMBOL_LIB_TABLE_H_
