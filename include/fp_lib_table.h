/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
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

#ifndef FP_LIB_TABLE_H_
#define FP_LIB_TABLE_H_

#include <lib_table_base.h>
#include <pcb_io/pcb_io_mgr.h>

class FOOTPRINT;
class FP_LIB_TABLE_GRID;
class PCB_IO;


/**
 * Hold a record identifying a library accessed by the appropriate footprint library #PLUGIN
 * object in the #FP_LIB_TABLE.
 */
class FP_LIB_TABLE_ROW : public LIB_TABLE_ROW
{
public:
    FP_LIB_TABLE_ROW( const wxString& aNick, const wxString& aURI, const wxString& aType,
                      const wxString& aOptions, const wxString& aDescr = wxEmptyString ) :
        LIB_TABLE_ROW( aNick, aURI, aOptions, aDescr )
    {
        SetType( aType );
    }

    FP_LIB_TABLE_ROW() :
        type( PCB_IO_MGR::KICAD_SEXP )
    {
    }

    bool operator==( const FP_LIB_TABLE_ROW& aRow ) const;

    bool operator!=( const FP_LIB_TABLE_ROW& aRow ) const   { return !( *this == aRow ); }

    /**
     * return the type of footprint library table represented by this row.
     */
    const wxString GetType() const override         { return PCB_IO_MGR::ShowType( type ); }

    /**
     * Change the type represented by this row.
     */
    void SetType( const wxString& aType ) override;

    bool LibraryExists() const override;

    PCB_IO_MGR::PCB_FILE_T GetFileType() { return type; }

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

    void setPlugin( PCB_IO* aPlugin )
    {
        plugin.reset( aPlugin );
    }

    friend class FP_LIB_TABLE;

private:
    IO_RELEASER<PCB_IO>    plugin;
    PCB_IO_MGR::PCB_FILE_T type;
};


class FP_LIB_TABLE : public LIB_TABLE
{
public:
    PROJECT::ELEM ProjectElementType() override { return PROJECT::ELEM::FPTBL; }

    virtual void Parse( LIB_TABLE_LEXER* aLexer ) override;

    virtual void Format( OUTPUTFORMATTER* aOutput, int aIndentLevel ) const override;

    /**
     * Build a footprint library table by pre-pending this table fragment in front of
     * @a aFallBackTable.  Loading of this table fragment is done by using Parse().
     *
     * @param aFallBackTable is another FP_LIB_TABLE which is searched only when a row
     *                       is not found in this table.  No ownership is taken of
     *                       \a aFallBackTable.
     */
    FP_LIB_TABLE( FP_LIB_TABLE* aFallBackTable = nullptr );

    bool operator==( const FP_LIB_TABLE& aFpTable ) const;

    bool operator!=( const FP_LIB_TABLE& r ) const  { return !( *this == r ); }

    /**
     * Return an #FP_LIB_TABLE_ROW if \a aNickName is found in this table or in any chained
     * fall back table fragment.
     *
     * If \a aCheckIfEnabled is true, the library will be ignored even if it is disabled.
     * Otherwise, the row found will be returned even if entry is disabled.
     *
     * The #PLUGIN is loaded and attached to the "plugin" field of the #FP_LIB_TABLE_ROW if
     * not already loaded.
     *
     * @param aNickName is the name of library nickname to find.
     * @param aCheckIfEnabled is the flag to check if the library found is enabled.
     * @return the library \a NickName if found.
     * @throw IO_ERROR if \a aNickName cannot be found.
     */
    const FP_LIB_TABLE_ROW* FindRow( const wxString& aNickName, bool aCheckIfEnabled = false );

    /**
     * Return a list of footprint names contained within the library given by @a aNickname.
     *
     * @param aFootprintNames is the list to fill with the footprint names found in \a aNickname
     * @param aNickname is a locator for the "library", it is a "name" in LIB_TABLE_ROW.
     * @param aBestEfforts if true, don't throw on errors.
     *
     * @throw IO_ERROR if the library cannot be found, or footprint cannot be loaded.
     */
    void FootprintEnumerate( wxArrayString& aFootprintNames, const wxString& aNickname,
                             bool aBestEfforts );

    /**
     * Generate a hashed timestamp representing the last-mod-times of the library indicated
     * by \a aNickname, or all libraries if \a aNickname is NULL.
     */
    long long GenerateTimestamp( const wxString* aNickname );

    /**
     * Load a footprint having @a aFootprintName from the library given by @a aNickname.
     *
     * @param aNickname is a locator for the "library", it is a "name" in #LIB_TABLE_ROW.
     * @param aFootprintName is the name of the footprint to load.
     * @param aKeepUUID = true to keep initial items UUID, false to set new UUID
     *                   normally true if loaded in the footprint editor, false
     *                   if loaded in the board editor. Used only in kicad_plugin
     * @return  the footprint if found caller owns it, else NULL if not found.
     *
     * @throw   IO_ERROR if the library cannot be found or read.  No exception
     *          is thrown in the case where aFootprintName cannot be found.
     */
    FOOTPRINT* FootprintLoad( const wxString& aNickname, const wxString& aFootprintName,
                              bool aKeepUUID = false );

    /**
     * Indicates whether or not the given footprint already exists in the given library.
     */
    bool FootprintExists( const wxString& aNickname, const wxString& aFootprintName );

    /**
     * A version of #FootprintLoad() for use after #FootprintEnumerate() for more efficient
     * cache management.
     *
     * The return value is const to allow it to return a reference to a cached item.
     */
    const FOOTPRINT* GetEnumeratedFootprint( const wxString& aNickname,
                                             const wxString& aFootprintName );
    /**
     * The set of return values from FootprintSave() below.
     */
    enum SAVE_T
    {
        SAVE_OK,
        SAVE_SKIPPED,
    };

    /**
     * Write @a aFootprint to an existing library given by @a aNickname.
     *
     * If a footprint by the same name already exists, it is replaced.
     *
     * @param aNickname is a locator for the "library", it is a "name" in #LIB_TABLE_ROW.
     * @param aFootprint is what to store in the library. The caller continues to own the
     *                   footprint after this call.
     * @param aOverwrite when true means overwrite any existing footprint by the same name,
     *                   else if false means skip the write and return SAVE_SKIPPED.
     * @return #SAVE_OK or #SAVE_SKIPPED.  If error saving, then #IO_ERROR is thrown.
     *
     * @throw IO_ERROR if there is a problem saving.
     */
    SAVE_T FootprintSave( const wxString& aNickname, const FOOTPRINT* aFootprint,
                          bool aOverwrite = true );

    /**
     * Delete the @a aFootprintName from the library given by @a aNickname.
     *
     * @param aNickname is a locator for the "library", it is a "name" in #LIB_TABLE_ROW.
     * @param aFootprintName is the name of a footprint to delete from the specified library.
     *
     * @throw IO_ERROR if there is a problem finding the footprint or the library, or deleting it.
     */
    void FootprintDelete( const wxString& aNickname, const wxString& aFootprintName );

    /**
     * Return true if the library given by @a aNickname is writable.
     *
     * Often system libraries are read only because of where they are installed.
     *
     * @throw IO_ERROR if no library at aLibraryPath exists.
     */
    bool IsFootprintLibWritable( const wxString& aNickname );

    void FootprintLibDelete( const wxString& aNickname );

    void FootprintLibCreate( const wxString& aNickname );

    /**
     * Load a footprint having @a aFootprintId with possibly an empty nickname.
     *
     * @param aFootprintId the [nickname] and footprint name of the footprint to load.
     * @param aKeepUUID = true to keep initial items UUID, false to set new UUID
     *                   normally true if loaded in the footprint editor, false
     *                   if loaded in the board editor
     *                   used only in kicad_plugin
     * @return  the #FOOTPRINT if found caller owns it, else NULL if not found.
     *
     * @throw   IO_ERROR if the library cannot be found or read.  No exception is
     *                   thrown in the case where \a aFootprintName cannot be found.
     * @throw   PARSE_ERROR if @a aFootprintId is not parsed OK.
     */
    FOOTPRINT* FootprintLoadWithOptionalNickname( const LIB_ID& aFootprintId,
                                                  bool aKeepUUID = false );

    /**
     * Load the global footprint library table into \a aTable.
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
    static bool LoadGlobalTable( FP_LIB_TABLE& aTable );

    /**
     * @return the platform specific global footprint library path and file name.
     */
    static wxString GetGlobalTableFileName();

    /**
     * Return the name of the environment variable used to hold the directory of
     * locally installed "KiCad sponsored" system footprint libraries.
     *
     *These can be either legacy or pretty format.  The only thing special about this
     * particular environment variable is that it is set automatically by KiCad on
     * program start up, <b>if</b> it is not set already in the environment.
     */
    static const wxString GlobalPathEnvVariableName();

private:
    friend class FP_LIB_TABLE_GRID;
};


extern FP_LIB_TABLE GFootprintTable;        // KIFACE scope.

#endif  // FP_LIB_TABLE_H_
