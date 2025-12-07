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
 

#ifndef FOOTPRINT_LIBRARY_ADAPTER_H
#define FOOTPRINT_LIBRARY_ADAPTER_H

#include <lib_id.h>
#include <libraries/library_manager.h>
#include <pcb_io/pcb_io.h>
#include <project.h>

class FOOTPRINT;


/**
 * An interface to the global shared library manager that is schematic-specific
 * and linked to one project in particular.  This is what can return actual concrete
 * schematic library content (symbols).
 */
class FOOTPRINT_LIBRARY_ADAPTER : public LIBRARY_MANAGER_ADAPTER
{
public:
    FOOTPRINT_LIBRARY_ADAPTER( LIBRARY_MANAGER& aManager );

    LIBRARY_TABLE_TYPE Type() const override { return LIBRARY_TABLE_TYPE::FOOTPRINT; }

    static wxString GlobalPathEnvVariableName();

    void AsyncLoad() override;

    /// Loads or reloads the given library, if it exists
    std::optional<LIB_STATUS> LoadOne( LIB_DATA* aLib ) override;

    /// Loads or reloads the given library, if it exists
    std::optional<LIB_STATUS> LoadOne( const wxString& aNickname );

    /// Returns the status of a loaded library, or nullopt if the library hasn't been loaded (yet)
    std::optional<LIB_STATUS> GetLibraryStatus( const wxString& aNickname ) const override;

    /**
     * Retrieves a list of footprints contained in a given loaded library
     * @param aNickname is the library to query
     * @param aBestEfforts if true, don't throw on errors, just return a smaller or empty list.
     * @return a list of footprints contained in the given library
     */
    std::vector<FOOTPRINT*> GetFootprints( const wxString& aNickname, bool aBestEfforts = false );

    /**
     * Retrieves a list of footprint names contained in a given loaded library
     * @param aNickname is the library to query
     * @param aBestEfforts if true, don't throw on errors, just return a smaller or empty list.
     * @return a list of names of footprints contained in the given library
     */
    std::vector<wxString> GetFootprintNames( const wxString& aNickname, bool aBestEfforts = false );

    /**
     * Generates a filesystem timestamp / hash value for library(ies)
     * @param aNickname is an optional specific library to timestamp.  If nullptr,
     *                  a timestamp will be calculated for all libraries in the table.
     * @return a value that can be used to determine if libraries have changed on disk
     */
    long long GenerateTimestamp( const wxString* aNickname );

    bool FootprintExists( const wxString& aNickname, const wxString& aName );

    /**
     * Load a #FOOTPRINT having @a aName from the library given by @a aNickname.
     *
     * @param aNickname is a locator for the "library", it is a "name" in #LIB_TABLE_ROW
     * @param aName is the name of the #FOOTPRINT to load.
     * @param aKeepUUID = true to keep initial items UUID, false to set new UUID
     *                   normally true if loaded in the footprint editor, false
     *                   if loaded in the board editor. Make sense only in kicad_plugin
     * @return the footprint if found or NULL if not found.
     * @throw IO_ERROR if the library cannot be found or read.  No exception
     *                 is thrown in the case where \a aNickname cannot be found.
     */
    FOOTPRINT* LoadFootprint( const wxString& aNickname, const wxString& aName, bool aKeepUUID );

    FOOTPRINT* LoadFootprint( const LIB_ID& aLibId, bool aKeepUUID )
    {
        return LoadFootprint( aLibId.GetLibNickname(), aLibId.GetLibItemName(), aKeepUUID );
    }

    /**
     * Load a footprint having @a aFootprintId with possibly an empty nickname.
     *
     * @param aFootprintId the [nickname] and name of the design block to load.
     * @param aKeepUUID = true to keep initial items UUID, false to set new UUID
     *                   normally true if loaded in the footprint editor, false
     *                   if loaded in the board editor
     *                   used only in kicad_plugin
     * @return  the #FOOTPRINT if found caller owns it, else NULL if not found.
     *
     * @throw   IO_ERROR if the library cannot be found or read.  No exception is
     *                   thrown in the case where \a aFootprintId cannot be found.
     * @throw   PARSE_ERROR if @a aFootprintId is not parsed OK.
     */
    FOOTPRINT* LoadFootprintWithOptionalNickname( const LIB_ID& aFootprintId, bool aKeepUUID );

    // TODO(JE) library tables - hoist out?
    /**
     * The set of return values from SaveSymbol() below.
     */
    enum SAVE_T
    {
        SAVE_OK,
        SAVE_SKIPPED,
    };

    /**
     * Write @a aFootprint to an existing library given by @a aNickname.
     *
     * If a #FOOTPRINT by the same name already exists or there are any conflicting alias
     * names, the new #FOOTPRINT will silently overwrite any existing aliases and/or part
     * because libraries cannot have duplicate alias names.  It is the responsibility of
     * the caller to check the library for conflicts before saving.
     *
     * @param aNickname is a locator for the "library", it is a "name" in LIB_TABLE_ROW
     * @param aFootprint is what to store in the library. The library owns the footprint after this
     *                   call.
     * @param aOverwrite when true means overwrite any existing symbol by the same name,
     *                   else if false means skip the write and return SAVE_SKIPPED.
     * @return SAVE_T - SAVE_OK or SAVE_SKIPPED.  If error saving, then IO_ERROR is thrown.
     * @throw IO_ERROR if there is a problem saving the footprint.
     */
    SAVE_T SaveFootprint( const wxString& aNickname, const FOOTPRINT* aFootprint,
                          bool aOverwrite = true );

    /**
     * Deletes the @a aFootprintName from the library given by @a aNickname.
     *
     * @param aNickname is a locator for the "library", it is a "name" in LIB_TABLE_ROW.
     * @param aFootprintName is the name of a footprint to delete from the specified library.
     * @throw IO_ERROR if there is a problem finding the footprint or the library, or deleting it.
     */
    void DeleteFootprint( const wxString& aNickname, const wxString& aFootprintName );

    /**
     * Return true if the library given by @a aNickname is writable.
     *
     * It is possible that some footprint libraries are read only because of where they are
     * installed.
     *
     * @param aNickname is the library nickname in the footprint library table.
     * @throw IO_ERROR if no library at @a aNickname exists.
     */
    bool IsFootprintLibWritable( const wxString& aNickname );

protected:

    std::map<wxString, LIB_DATA>& globalLibs() override { return GlobalLibraries; }
    std::map<wxString, LIB_DATA>& globalLibs() const override { return GlobalLibraries; }
    std::mutex& globalLibsMutex() override { return GlobalLibraryMutex; }

    LIBRARY_RESULT<IO_BASE*> createPlugin( const LIBRARY_TABLE_ROW* row ) override;

    IO_BASE* plugin( const LIB_DATA* aRow ) override { return pcbplugin( aRow ); }

private:
    /// Helper to cast the ABC plugin in the LIB_DATA* to a concrete plugin
    static PCB_IO* pcbplugin( const LIB_DATA* aRow );

    // The global libraries, potentially shared between multiple different open
    // projects, each of which has their own instance of this adapter class
    static std::map<wxString, LIB_DATA> GlobalLibraries;

    static std::mutex GlobalLibraryMutex;
};

#endif //FOOTPRINT_LIBRARY_ADAPTER_H
