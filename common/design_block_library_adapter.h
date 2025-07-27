/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Jon Evans <jon@craftyjon.com>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 

#ifndef DESIGN_BLOCK_LIBRARY_ADAPTER_H
#define DESIGN_BLOCK_LIBRARY_ADAPTER_H

#include <kicommon.h>
#include <lib_id.h>
#include <libraries/library_manager.h>

class DESIGN_BLOCK;
class DESIGN_BLOCK_IO;
class PROJECT;


class KICOMMON_API DESIGN_BLOCK_LIBRARY_ADAPTER : public LIBRARY_MANAGER_ADAPTER
{
public:
    DESIGN_BLOCK_LIBRARY_ADAPTER( LIBRARY_MANAGER& aManager );

    LIBRARY_TABLE_TYPE Type() const override { return LIBRARY_TABLE_TYPE::DESIGN_BLOCK; }

    static wxString GlobalPathEnvVariableName();

    void AsyncLoad() override;

    /// @return all the design blocks in the given library, if it exists and is loaded (or an empty list)
    std::vector<DESIGN_BLOCK*> GetDesignBlocks( const wxString& aNickname );

    /// @return all the names of design blocks in the given library, if it exists and is loaded (or an empty list)
    std::vector<wxString> GetDesignBlockNames( const wxString& aNickname );

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
    DESIGN_BLOCK* LoadDesignBlock( const wxString& aNickname, const wxString& aDesignBlockName,
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
    // TODO(JE) library tables -- do we still need this?
    const DESIGN_BLOCK* GetEnumeratedDesignBlock( const wxString& aNickname, const wxString& aDesignBlockName );

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
    SAVE_T SaveDesignBlock( const wxString& aNickname, const DESIGN_BLOCK* aDesignBlock,
                            bool aOverwrite = true );

    /**
     * Delete the @a aDesignBlockName from the library given by @a aNickname.
     *
     * @param aNickname is a locator for the "library", it is a "name" in #LIB_TABLE_ROW.
     * @param aDesignBlockName is the name of a design block to delete from the specified library.
     *
     * @throw IO_ERROR if there is a problem finding the design block or the library, or deleting
     *        it.
     */
    void DeleteDesignBlock( const wxString& aNickname, const wxString& aDesignBlockName );

    /**
     * Return true if the library given by @a aNickname is writable.
     *
     * Often system libraries are read only because of where they are installed.
     *
     * @throw IO_ERROR if no library at aLibraryPath exists.
     */
    bool IsDesignBlockLibWritable( const wxString& aNickname );

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

    std::optional<LIBRARY_ERROR> LibraryError( const wxString& aNickname ) const;

protected:

    std::map<wxString, LIB_DATA>& globalLibs() override { return GlobalLibraries; }
    std::map<wxString, LIB_DATA>& globalLibs() const override { return GlobalLibraries; }
    std::mutex& globalLibsMutex() override { return GlobalLibraryMutex; }

    LIBRARY_RESULT<IO_BASE*> createPlugin( const LIBRARY_TABLE_ROW* row ) override;

private:

    /// Helper to cast the ABC plugin in the LIB_DATA* to a concrete plugin
    static DESIGN_BLOCK_IO* plugin( const LIB_DATA* aRow );

    // The global libraries, potentially shared between multiple different open
    // projects, each of which has their own instance of this adapter class
    static std::map<wxString, LIB_DATA> GlobalLibraries;

    static std::mutex GlobalLibraryMutex;
};



#endif //DESIGN_BLOCK_LIBRARY_ADAPTER_H
