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
 

#ifndef SYMBOL_LIBRARY_MANAGER_ADAPTER_H
#define SYMBOL_LIBRARY_MANAGER_ADAPTER_H

#include <future>
#include <lib_id.h>
#include <libraries/library_manager.h>
#include <sch_io/sch_io.h>

class LIB_SYMBOL;
class PROJECT;


enum class LOAD_STATUS
{
    INVALID,
    LOADING,
    LOADED,
    ERROR
};

struct LIB_STATUS
{
    LOAD_STATUS load_status = LOAD_STATUS::INVALID;
    std::optional<LIBRARY_ERROR> error;
};


/**
 * Data associated with a loaded symbol library
 */
struct LIB_DATA
{
    std::unique_ptr<SCH_IO> plugin;
    const LIBRARY_TABLE_ROW* row = nullptr;
    std::mutex mutex;
    LIB_STATUS status;

    int modify_hash;
    std::vector<wxString> available_fields_cache;
};

/**
 * A descriptor for a sub-library (supported by database and http libraries)
 */
struct SUB_LIBRARY
{
    wxString nickname;
    wxString description;
};


/**
 * An interface to the global shared library manager that is schematic-specific
 * and linked to one project in particular.  This is what can return actual concrete
 * schematic library content (symbols).
 */
class SYMBOL_LIBRARY_MANAGER_ADAPTER : public LIBRARY_MANAGER_ADAPTER
{
public:
    static const char* PropPowerSymsOnly;
    static const char* PropNonPowerSymsOnly;

public:
    SYMBOL_LIBRARY_MANAGER_ADAPTER( LIBRARY_MANAGER& aManager );

    LIBRARY_TABLE_TYPE Type() const override { return LIBRARY_TABLE_TYPE::SYMBOL; }

    void ProjectChanged() override;

    /**
     * Loads all available symbol libraries in the background
     */
    void AsyncLoad();

    /// Returns async load progress between 0.0 and 1.0, or nullopt if load is not in progress
    std::optional<float> AsyncLoadProgress() const;

    void BlockUntilLoaded();

    /// Loads or reloads the given library, if it exists
    std::optional<LIB_STATUS> LoadOne( const wxString& aNickname );

    std::optional<wxString> FindLibraryByURI( const wxString& aURI ) const;

    /// Returns a list of library nicknames that are available (skips any that failed to load)
    std::vector<wxString> GetLibraryNames() const;

    std::optional<wxString> GetLibraryDescription( const wxString& aNickname ) const;

    /**
     * Test for the existence of \a aNickname in the library tables.
     *
     * @param aCheckEnabled if true will only return true for enabled libraries
     * @return true if a library \a aNickname exists in the loaded tables.
     */
    bool HasLibrary( const wxString& aNickname, bool aCheckEnabled = false ) const;

    /// Returns the status of a loaded library, or nullopt if the library hasn't been loaded (yet)
    std::optional<LIB_STATUS> GetLibraryStatus( const wxString& aNickname ) const;

    /// Returns a list of all library nicknames and their status (even if they failed to load)
    std::vector<std::pair<wxString, LIB_STATUS>> GetLibraryStatuses() const;

    /// Returns a list of additional (non-mandatory) symbol fields present in the given library
    std::vector<wxString> GetAvailableExtraFields( const wxString& aNickname );

    bool SupportsSubLibraries( const wxString& aNickname ) const;

    std::vector<SUB_LIBRARY> GetSubLibraries( const wxString& aNickname ) const;

    bool SupportsConfigurationDialog( const wxString& aNickname ) const;

    void ShowConfigurationDialog( const wxString& aNickname, wxWindow* aParent ) const;

    enum class SYMBOL_TYPE
    {
        ALL_SYMBOLS,
        POWER_ONLY
    };

    std::vector<LIB_SYMBOL*> GetSymbols( const wxString& aNickname,
                                         SYMBOL_TYPE aType = SYMBOL_TYPE::ALL_SYMBOLS );

    std::vector<wxString> GetSymbolNames( const wxString& aNickname,
                                          SYMBOL_TYPE aType = SYMBOL_TYPE::ALL_SYMBOLS);
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

    bool IsLibraryLoaded( const wxString& aNickname );

    std::optional<LIBRARY_ERROR> LibraryError( const wxString& aNickname ) const;

    // TODO(JE) is this the right API still?
    void DeleteLibrary( const wxString& aNickname );

    // TODO(JE) is this the right API still?
    void CreateLibrary( const wxString& aNickname );

    static std::optional<SCH_IO_MGR::SCH_FILE_T> ParseLibType( const wxString& aType );

    int GetModifyHash() const override;

    bool IsWritable( const wxString& aNickname ) const override;

protected:

    void doPreload() override {}

private:

    static wxString getUri( const LIBRARY_TABLE_ROW* aRow );

    LIBRARY_RESULT<LIB_DATA*> loadIfNeeded( const wxString& aNickname );

    std::optional<const LIB_DATA*> fetchIfLoaded( const wxString& aNickname ) const;

    std::optional<LIB_DATA*> fetchIfLoaded( const wxString& aNickname );

    /// Aborts any async load in progress; blocks until fully done aborting
    void abortLoad();

    // The actual library content is held in an associated SCH_IO plugin
    // TODO(JE) should this be an expected<LIB_ROW> so we can store the
    // error result if a lib can't be loaded instead of retrying the load every time
    // content is requested?
    std::map<wxString, LIB_DATA> m_libraries;

    std::mutex m_libraries_mutex;

    std::atomic_bool m_abort;
    std::vector<std::future<void>> m_futures;

    std::atomic<size_t> m_loadCount;
    size_t m_loadTotal;

    // The global libraries, potentially shared between multiple different open
    // projects, each of which has their own instance of this adapter class
    static std::map<wxString, LIB_DATA> GlobalLibraries;

    static std::mutex GlobalLibraryMutex;
};

#endif //SYMBOL_LIBRARY_MANAGER_ADAPTER_H
