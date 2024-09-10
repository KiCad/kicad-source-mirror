/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012-2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef DESIGN_BLOCK_LIB_TABLE_H_
#define DESIGN_BLOCK_LIB_TABLE_H_

#include <kicommon.h>
#include <lib_table_base.h>
#include <design_block_io.h>
#include <design_block_info_impl.h>

class DESIGN_BLOCK;
class DESIGN_BLOCK_LIB_TABLE_GRID;

/**
 * Hold a record identifying a library accessed by the appropriate design block library #PLUGIN
 * object in the #DESIGN_BLOCK_LIB_TABLE.
 */
class KICOMMON_API DESIGN_BLOCK_LIB_TABLE_ROW : public LIB_TABLE_ROW
{
public:
    DESIGN_BLOCK_LIB_TABLE_ROW( const wxString& aNick, const wxString& aURI, const wxString& aType,
                                const wxString& aOptions, const wxString& aDescr = wxEmptyString ) :
            LIB_TABLE_ROW( aNick, aURI, aOptions, aDescr )
    {
        SetType( aType );
    }

    DESIGN_BLOCK_LIB_TABLE_ROW() : type( DESIGN_BLOCK_IO_MGR::KICAD_SEXP ) {}

    bool operator==( const DESIGN_BLOCK_LIB_TABLE_ROW& aRow ) const;

    bool operator!=( const DESIGN_BLOCK_LIB_TABLE_ROW& aRow ) const { return !( *this == aRow ); }

    /**
     * return the type of design block library table represented by this row.
     */
    const wxString GetType() const override { return DESIGN_BLOCK_IO_MGR::ShowType( type ); }

    /**
     * Change the type represented by this row.
     */
    void SetType( const wxString& aType ) override;

    DESIGN_BLOCK_IO_MGR::DESIGN_BLOCK_FILE_T GetFileType() { return type; }

protected:
    DESIGN_BLOCK_LIB_TABLE_ROW( const DESIGN_BLOCK_LIB_TABLE_ROW& aRow ) :
            LIB_TABLE_ROW( aRow ), type( aRow.type )
    {
    }

private:
    virtual LIB_TABLE_ROW* do_clone() const override
    {
        return new DESIGN_BLOCK_LIB_TABLE_ROW( *this );
    }

    void setPlugin( DESIGN_BLOCK_IO* aPlugin ) { plugin.reset( aPlugin ); }

    friend class DESIGN_BLOCK_LIB_TABLE;

private:
    IO_RELEASER<DESIGN_BLOCK_IO>             plugin;
    DESIGN_BLOCK_IO_MGR::DESIGN_BLOCK_FILE_T type;
};


class KICOMMON_API DESIGN_BLOCK_LIB_TABLE : public LIB_TABLE
{
public:
    PROJECT::ELEM ProjectElementType() override { return PROJECT::ELEM::DESIGN_BLOCK_LIB_TABLE; }

    virtual void Parse( LIB_TABLE_LEXER* aLexer ) override;

    virtual void Format( OUTPUTFORMATTER* aOutput, int aIndentLevel ) const override;

    /**
     * Build a design block library table by pre-pending this table fragment in front of
     * @a aFallBackTable.  Loading of this table fragment is done by using Parse().
     *
     * @param aFallBackTable is another DESIGN_BLOCK_LIB_TABLE which is searched only when a row
     *                       is not found in this table.  No ownership is taken of
     *                       \a aFallBackTable.
     */
    DESIGN_BLOCK_LIB_TABLE( DESIGN_BLOCK_LIB_TABLE* aFallBackTable = nullptr );

    bool operator==( const DESIGN_BLOCK_LIB_TABLE& aFpTable ) const;

    bool operator!=( const DESIGN_BLOCK_LIB_TABLE& r ) const { return !( *this == r ); }

    /**
     * Return an #DESIGN_BLOCK_LIB_TABLE_ROW if \a aNickName is found in this table or in any chained
     * fall back table fragment.
     *
     * If \a aCheckIfEnabled is true, the library will be ignored even if it is disabled.
     * Otherwise, the row found will be returned even if entry is disabled.
     *
     * The #PLUGIN is loaded and attached to the "plugin" field of the #DESIGN_BLOCK_LIB_TABLE_ROW if
     * not already loaded.
     *
     * @param aNickName is the name of library nickname to find.
     * @param aCheckIfEnabled is the flag to check if the library found is enabled.
     * @return the library \a NickName if found.
     * @throw IO_ERROR if \a aNickName cannot be found.
     */
    const DESIGN_BLOCK_LIB_TABLE_ROW* FindRow( const wxString& aNickName,
                                               bool            aCheckIfEnabled = false );

    /**
     * Return a list of design block names contained within the library given by @a aNickname.
     *
     * @param aDesignBlockNames is the list to fill with the design block names found in \a aNickname
     * @param aNickname is a locator for the "library", it is a "name" in LIB_TABLE_ROW.
     * @param aBestEfforts if true, don't throw on errors.
     *
     * @throw IO_ERROR if the library cannot be found, or design block cannot be loaded.
     */
    void DesignBlockEnumerate( wxArrayString& aDesignBlockNames, const wxString& aNickname,
                               bool aBestEfforts );

    /**
     * Generate a hashed timestamp representing the last-mod-times of the library indicated
     * by \a aNickname, or all libraries if \a aNickname is NULL.
     */
    long long GenerateTimestamp( const wxString* aNickname );

    /**
     * If possible, prefetches the specified library (e.g. performing downloads). Does not parse.
     * Threadsafe.
     *
     * This is a no-op for libraries that cannot be prefetched.
     *
     * @param aNickname is a locator for the library; it is a name in LIB_TABLE_ROW.
     *
     * @throw IO_ERROR if there is an error prefetching the library.
     */
    void PrefetchLib( const wxString& aNickname );

    /**
     * Load a design block having @a aDesignBlockName from the library given by @a aNickname.
     *
     * @param aNickname is a locator for the "library", it is a "name" in #LIB_TABLE_ROW.
     * @param aDesignBlockName is the name of the design block to load.
     * @param aKeepUUID = true to keep initial items UUID, false to set new UUID
     *                   normally true if loaded in the design block editor, false
     *                   if loaded in the board editor. Used only in kicad_plugin
     * @return  the design block if found caller owns it, else NULL if not found.
     *
     * @throw   IO_ERROR if the library cannot be found or read.  No exception
     *          is thrown in the case where aDesignBlockName cannot be found.
     */
    DESIGN_BLOCK* DesignBlockLoad( const wxString& aNickname, const wxString& aDesignBlockName,
                                   bool aKeepUUID = false );

    /**
     * Indicates whether or not the given design block already exists in the given library.
     */
    bool DesignBlockExists( const wxString& aNickname, const wxString& aDesignBlockName );

    /**
     * A version of #DesignBlockLoad() for use after #DesignBlockEnumerate() for more efficient
     * cache management.
     *
     * The return value is const to allow it to return a reference to a cached item.
     */
    const DESIGN_BLOCK* GetEnumeratedDesignBlock( const wxString& aNickname,
                                                  const wxString& aDesignBlockName );
    /**
     * The set of return values from DesignBlockSave() below.
     */
    enum SAVE_T
    {
        SAVE_OK,
        SAVE_SKIPPED,
    };

    /**
     * Write @a aDesignBlock to an existing library given by @a aNickname.
     *
     * If a design block by the same name already exists, it is replaced.
     *
     * @param aNickname is a locator for the "library", it is a "name" in #LIB_TABLE_ROW.
     * @param aDesignBlock is what to store in the library. The caller continues to own the
     *                   design block after this call.
     * @param aOverwrite when true means overwrite any existing design block by the same name,
     *                   else if false means skip the write and return SAVE_SKIPPED.
     * @return #SAVE_OK or #SAVE_SKIPPED.  If error saving, then #IO_ERROR is thrown.
     *
     * @throw IO_ERROR if there is a problem saving.
     */
    SAVE_T DesignBlockSave( const wxString& aNickname, const DESIGN_BLOCK* aDesignBlock,
                            bool aOverwrite = true );

    /**
     * Delete the @a aDesignBlockName from the library given by @a aNickname.
     *
     * @param aNickname is a locator for the "library", it is a "name" in #LIB_TABLE_ROW.
     * @param aDesignBlockName is the name of a design block to delete from the specified library.
     *
     * @throw IO_ERROR if there is a problem finding the design block or the library, or deleting it.
     */
    void DesignBlockDelete( const wxString& aNickname, const wxString& aDesignBlockName );

    /**
     * Return true if the library given by @a aNickname is writable.
     *
     * Often system libraries are read only because of where they are installed.
     *
     * @throw IO_ERROR if no library at aLibraryPath exists.
     */
    bool IsDesignBlockLibWritable( const wxString& aNickname );

    void DesignBlockLibDelete( const wxString& aNickname );

    void DesignBlockLibCreate( const wxString& aNickname );

    /**
     * Load a design block having @a aDesignBlockId with possibly an empty nickname.
     *
     * @param aDesignBlockId the [nickname] and name of the design block to load.
     * @param aKeepUUID = true to keep initial items UUID, false to set new UUID
     *                   normally true if loaded in the design block editor, false
     *                   if loaded in the board editor
     *                   used only in kicad_plugin
     * @return  the #DESIGN_BLOCK if found caller owns it, else NULL if not found.
     *
     * @throw   IO_ERROR if the library cannot be found or read.  No exception is
     *                   thrown in the case where \a aDesignBlockName cannot be found.
     * @throw   PARSE_ERROR if @a aDesignBlockId is not parsed OK.
     */
    DESIGN_BLOCK* DesignBlockLoadWithOptionalNickname( const LIB_ID& aDesignBlockId,
                                                       bool          aKeepUUID = false );

    /**
     * Load the global design block library table into \a aTable.
     *
     * This probably should be move into the application object when KiCad is changed
     * to a single process application.  This is the least painful solution for the
     * time being.
     *
     * @param aTable the #DESIGN_BLOCK_LIB_TABLE object to load.
     * @return true if the global library table exists and is loaded properly.
     * @throw IO_ERROR if an error occurs attempting to load the design block library
     *                 table.
     */
    static bool LoadGlobalTable( DESIGN_BLOCK_LIB_TABLE& aTable );

    static DESIGN_BLOCK_LIB_TABLE& GetGlobalLibTable();

    static DESIGN_BLOCK_LIST_IMPL& GetGlobalList();

    /**
     * @return the platform specific global design block library path and file name.
     */
    static wxString GetGlobalTableFileName();

    /**
     * Return the name of the environment variable used to hold the directory of
     * locally installed "KiCad sponsored" system design block libraries.
     *
     *These can be either legacy or pretty format.  The only thing special about this
     * particular environment variable is that it is set automatically by KiCad on
     * program start up, <b>if</b> it is not set already in the environment.
     */
    static const wxString GlobalPathEnvVariableName();

private:
    friend class DESIGN_BLOCK_LIB_TABLE_GRID;
};

#endif // DESIGN_BLOCK_LIB_TABLE_H_
