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

#ifndef LIBRARY_MANAGER_H
#define LIBRARY_MANAGER_H

#include <future>
#include <memory>
#include <tl/expected.hpp>

#include <kicommon.h>
#include <libraries/library_table.h>
#include <io/io_base.h>


struct KICOMMON_API LIBRARY_ERROR
{
    wxString message;
};


template<typename ResultType>
using LIBRARY_RESULT = tl::expected<ResultType, LIBRARY_ERROR>;

class LIBRARY_MANAGER_ADAPTER;
class PROJECT;


class KICOMMON_API LIBRARY_MANAGER
{
public:
    LIBRARY_MANAGER();

    ~LIBRARY_MANAGER();

    static wxString DefaultGlobalTablePath( LIBRARY_TABLE_TYPE aType );

    static bool IsTableValid( const wxString& aPath );

    /// @return true if all required global tables are present on disk and valid
    static bool GlobalTablesValid();

    /// @return a list of global tables that are not valid (or an empty list if GlobalTablesValid() returns true)
    static std::vector<LIBRARY_TABLE_TYPE> InvalidGlobalTables();

    static bool CreateGlobalTable( LIBRARY_TABLE_TYPE aType, bool aPopulateDefaultLibraries );

    /// (Re)loads the global library tables in the given list, or all tables if no list is given
    void LoadGlobalTables( std::initializer_list<LIBRARY_TABLE_TYPE> aTablesToLoad = {} );

    /// Notify all adapters that the project has changed
    void ProjectChanged();

    void RegisterAdapter( LIBRARY_TABLE_TYPE aType,
                          std::unique_ptr<LIBRARY_MANAGER_ADAPTER>&& aAdapter );

    std::optional<LIBRARY_MANAGER_ADAPTER*> Adapter( LIBRARY_TABLE_TYPE aType ) const;

    /**
     * Retrieves a given table; creating a new empty project table if a valid project is
     * loaded and the given table type doesn't exist in the project.
     * @param aType determines which type of table to return
     * @param aScope determines whether to return a global or project table
     * @return the given table if it exists (which should be the case unless a project table is
     *         requested and there is no valid project loaded)
     */
    std::optional<LIBRARY_TABLE*> Table( LIBRARY_TABLE_TYPE aType,
                                         LIBRARY_TABLE_SCOPE aScope );

    /**
     * Returns a flattened list of libraries of the given type
     * @param aType determines which type of libraries to return (symbol, footprint, ...)
     * @param aIncludeInvalid will include the nicknames of libraries even if they could not be
     *                        loaded for some reason (file not found, etc)
     * @return a list of library nicknames (the first part of a LIB_ID)
     */
    std::vector<LIBRARY_TABLE_ROW*> Rows( LIBRARY_TABLE_TYPE aType,
                                          LIBRARY_TABLE_SCOPE aScope = LIBRARY_TABLE_SCOPE::BOTH,
                                          bool aIncludeInvalid = false ) const;

    /**
     *
     * @param aType determines which type of libraries to return (symbol, footprint, ...)
     * @param aScope determines whether to search project, global, or both library tables
     * @param aNickname is the library nickname to retrieve
     * @return the row, or a nullopt if it does not exist
     */
    std::optional<LIBRARY_TABLE_ROW*> GetRow( LIBRARY_TABLE_TYPE aType,
                                               const wxString &aNickname,
                                               LIBRARY_TABLE_SCOPE aScope =
                                                       LIBRARY_TABLE_SCOPE::BOTH ) const;

    std::optional<LIBRARY_TABLE_ROW*> FindRowByURI( LIBRARY_TABLE_TYPE aType,
                                                    const wxString &aUri,
                                                    LIBRARY_TABLE_SCOPE aScope =
                                                            LIBRARY_TABLE_SCOPE::BOTH ) const;

    void LoadProjectTables( const wxString& aProjectPath );

    LIBRARY_RESULT<void> Save( LIBRARY_TABLE* aTable ) const;

    /**
     * Return the full location specifying URI for the LIB, either in original UI form or
     * in environment variable expanded form.
     *
     * @param aType determines which tables will be searched for the library
     * @param aNickname is the library to look up
     * @param aSubstituted Tells if caller wanted the substituted form, else not.
     * @return the URI for the given library, or nullopt if the nickname is not a valid library
     */
    std::optional<wxString> GetFullURI( LIBRARY_TABLE_TYPE aType, const wxString& aNickname,
                                        bool aSubstituted = false ) const;

    static wxString GetFullURI( const LIBRARY_TABLE_ROW* aRow, bool aSubstituted = false );

    static wxString ExpandURI( const wxString& aShortURI, const PROJECT& aProject );

    static bool UrisAreEquivalent( const wxString& aURI1, const wxString& aURI2 );

private:
    void loadTables( const wxString& aTablePath, LIBRARY_TABLE_SCOPE aScope,
                     std::vector<LIBRARY_TABLE_TYPE> aTablesToLoad = {} );

    void loadNestedTables( LIBRARY_TABLE& aTable );

    static wxString tableFileName( LIBRARY_TABLE_TYPE aType );

    void createEmptyTable( LIBRARY_TABLE_TYPE aType, LIBRARY_TABLE_SCOPE aScope );

    std::map<LIBRARY_TABLE_TYPE, std::unique_ptr<LIBRARY_TABLE>> m_tables;

    /// Map of full URI to table object for tables that are referenced by global or project tables
    std::map<wxString, std::unique_ptr<LIBRARY_TABLE>> m_childTables;

    // TODO: support multiple projects
    std::map<LIBRARY_TABLE_TYPE, std::unique_ptr<LIBRARY_TABLE>> m_projectTables;

    std::map<LIBRARY_TABLE_TYPE, std::unique_ptr<LIBRARY_MANAGER_ADAPTER>> m_adapters;

    static const std::map<LIBRARY_TABLE_TYPE, const std::string&> m_typeToFilenameMap;
};

/// Status of a library load managed by a library adapter
enum class LOAD_STATUS
{
    INVALID,
    LOADING,
    LOADED,
    ERROR
};

/// The overall status of a loaded or loading library
struct KICOMMON_API LIB_STATUS
{
    LOAD_STATUS load_status = LOAD_STATUS::INVALID;
    std::optional<LIBRARY_ERROR> error;
};


/// Storage for an actual loaded library (including library content owned by the plugin)
struct KICOMMON_API LIB_DATA
{
    std::unique_ptr<IO_BASE> plugin;
    const LIBRARY_TABLE_ROW* row = nullptr;
    std::mutex mutex;
    LIB_STATUS status;

    int modify_hash;
    std::vector<wxString> available_fields_cache;
};


/**
 * The interface used by the classes that actually can load IO plugins for the
 * different parts of KiCad and return concrete types (symbols, footprints, etc)
 */
class KICOMMON_API LIBRARY_MANAGER_ADAPTER
{
public:
    /**
     * Constructs a type-specific adapter into the library manager.  The code for these
     * generally resides in app-specific libraries (eeschema/pcbnew for example) rather than
     * being in kicommon.
     *
     * @param aManager should usually be Pgm().GetLibraryManager() except in QA tests
     */
    LIBRARY_MANAGER_ADAPTER( LIBRARY_MANAGER& aManager );

    virtual ~LIBRARY_MANAGER_ADAPTER();

    LIBRARY_MANAGER& Manager() const { return m_manager; }

    /// The type of library table this adapter works with
    virtual LIBRARY_TABLE_TYPE Type() const = 0;

    /// Retrieves the global library table for this adapter type
    LIBRARY_TABLE* GlobalTable() const;

    /// Retrieves the project library table for this adapter type, or nullopt if one doesn't exist
    std::optional<LIBRARY_TABLE*> ProjectTable() const;

    std::optional<wxString> FindLibraryByURI( const wxString& aURI ) const;

    /// Returns a list of library nicknames that are available (skips any that failed to load)
    std::vector<wxString> GetLibraryNames() const;

    /**
     * Test for the existence of \a aNickname in the library tables.
     *
     * @param aCheckEnabled if true will only return true for enabled libraries
     * @return true if a library \a aNickname exists in the loaded tables.
     */
    bool HasLibrary( const wxString& aNickname, bool aCheckEnabled = false ) const;

    /// Deletes the given library from disk if it exists; returns true if deleted
    bool DeleteLibrary( const wxString& aNickname );

    std::optional<wxString> GetLibraryDescription( const wxString& aNickname ) const;

    /// Like LIBRARY_MANAGER::Rows but filtered to the LIBRARY_TABLE_TYPE of this adapter
    std::vector<LIBRARY_TABLE_ROW *> Rows(
        LIBRARY_TABLE_SCOPE aScope = LIBRARY_TABLE_SCOPE::BOTH,
        bool aIncludeInvalid = false ) const;

    /// Like LIBRARY_MANAGER::GetRow but filtered to the LIBRARY_TABLE_TYPE of this adapter
    std::optional<LIBRARY_TABLE_ROW *> GetRow( const wxString &aNickname,
        LIBRARY_TABLE_SCOPE aScope = LIBRARY_TABLE_SCOPE::BOTH ) const;

    /// Like LIBRARY_MANAGER::FindRowByURI but filtered to the LIBRARY_TABLE_TYPE of this adapter
    std::optional<LIBRARY_TABLE_ROW*> FindRowByURI( const wxString &aUri,
                                                    LIBRARY_TABLE_SCOPE aScope = LIBRARY_TABLE_SCOPE::BOTH ) const;

    /// Notify the adapter that the active project has changed
    virtual void ProjectChanged();

    /// Notify the adapter that the global library tables have changed
    void GlobalTablesChanged( std::initializer_list<LIBRARY_TABLE_TYPE> aChangedTables = {} );

    /// Loads all available libraries for this adapter type in the background
    virtual void AsyncLoad() = 0;

    /// Returns async load progress between 0.0 and 1.0, or nullopt if load is not in progress
    std::optional<float> AsyncLoadProgress() const;

    void BlockUntilLoaded();

    bool IsLibraryLoaded( const wxString& aNickname );

    /// Return true if the given nickname exists and is not a read-only library
    virtual bool IsWritable( const wxString& aNickname ) const { return false; }

protected:
    virtual std::map<wxString, LIB_DATA>& globalLibs() = 0;
    virtual std::map<wxString, LIB_DATA>& globalLibs() const = 0;
    virtual std::mutex& globalLibsMutex() = 0;

    static wxString getUri( const LIBRARY_TABLE_ROW* aRow );

    std::optional<const LIB_DATA*> fetchIfLoaded( const wxString& aNickname ) const;

    std::optional<LIB_DATA*> fetchIfLoaded( const wxString& aNickname );

    /// Fetches a loaded library, triggering a load of that library if it isn't loaded yet
    LIBRARY_RESULT<LIB_DATA*> loadIfNeeded( const wxString& aNickname );

    /// Aborts any async load in progress; blocks until fully done aborting
    void abortLoad();

    /// Creates a concrete plugin for the given row
    virtual LIBRARY_RESULT<IO_BASE*> createPlugin( const LIBRARY_TABLE_ROW* row ) = 0;

    LIBRARY_MANAGER& m_manager;

    // The actual library content is held in an associated IO plugin
    // TODO(JE) should this be an expected<LIB_ROW> so we can store the
    // error result if a lib can't be loaded instead of retrying the load every time
    // content is requested?
    std::map<wxString, LIB_DATA> m_libraries;

    std::mutex m_libraries_mutex;

    std::atomic_bool m_abort;
    std::vector<std::future<void>> m_futures;

    std::atomic<size_t> m_loadCount;
    size_t m_loadTotal;
};


#endif //LIBRARY_MANAGER_H
