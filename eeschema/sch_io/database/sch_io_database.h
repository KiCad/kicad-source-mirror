/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Jon Evans <jon@craftyjon.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef SCH_IO_DATABASE_H_
#define SCH_IO_DATABASE_H_

#include <database/database_connection.h>
#include <sch_io/sch_io.h>
#include <sch_io/sch_io_mgr.h>
#include <wildcards_and_files_ext.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <optional>
#include <random>
#include <shared_mutex>
#include <thread>
#include <unordered_set>


class LIB_ID;
class LIBRARY_MANAGER_ADAPTER;
class DATABASE_LIB_SETTINGS;
struct DATABASE_LIB_TABLE;
struct DATABASE_SOURCE;


/**
 * A KiCad database library provides both symbol and footprint metadata, so there are "shim" plugins
 * on both the symbol and footprint side of things that expose the database contents to the
 * schematic and board editors.  The architecture of these is slightly different from the other
 * plugins because the backing file is just a configuration file rather than something that
 * contains symbol or footprint data.
 */
class SCH_IO_DATABASE : public SCH_IO
{
    friend class SCH_IO_DATABASE_CYCLE_DETECTION_FIXTURE;

public:

    SCH_IO_DATABASE();
    virtual ~SCH_IO_DATABASE();

    const IO_BASE::IO_FILE_DESC GetLibraryDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( _HKI( "KiCad database library files" ),
                                      { FILEEXT::DatabaseLibraryFileExtension } );
    }

    int GetModifyHash() const override { return m_modifyHash; }

    void EnumerateSymbolLib( wxArrayString&    aSymbolNameList,
                             const wxString&   aLibraryPath,
                             const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    void EnumerateSymbolLib( std::vector<LIB_SYMBOL*>& aSymbolList,
                             const wxString&           aLibraryPath,
                             const std::map<std::string, UTF8>*         aProperties = nullptr ) override;

    void CheckLibrary( const wxString& aLibraryPath,
                       const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    LIB_SYMBOL* LoadSymbol( const wxString& aLibraryPath, const wxString& aAliasName,
                            const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    bool SupportsSubLibraries() const override { return true; }

    void GetSubLibraryNames( std::vector<wxString>& aNames ) override;

    void GetAvailableSymbolFields( std::vector<wxString>& aNames ) override;

    void GetDefaultSymbolFields( std::vector<wxString>& aNames ) override;

    // Database libraries can never be written using the symbol editing API
    bool IsLibraryWritable( const wxString& aLibraryPath ) override { return false; }

    void SetLibraryManagerAdapter( SYMBOL_LIBRARY_ADAPTER* aAdapter ) override
    {
        m_adapter = aAdapter;
    }

    bool SupportsConfigurationDialog() const override { return true; }

    DIALOG_SHIM* CreateConfigurationDialog( wxWindow* aParent, const wxString& aLibraryPath ) override;

    /// Ends the waits of a load in progress and of any that starts before ResumeLoad(), so closing a project does not
    /// wait out a locked database
    void CancelLoad() override;

    void ResumeLoad() noexcept override;

    DATABASE_LIB_SETTINGS* Settings() const { return m_settings.get(); }

    /// @return the symbol name a row is listed under, or empty if the row has no key
    static wxString SymbolNameForRow( const DATABASE_LIB_TABLE& aTable, bool aGloballyUniqueKeys,
                                      const DATABASE_CONNECTION::ROW& aRow, std::string* aRawKey = nullptr );

    /// A connection built from settings, or the reason none could be built
    struct CONNECTION_RESULT
    {
        /// Null when no backend exists for the source type
        std::unique_ptr<DATABASE_CONNECTION> connection;

        /// Why connection is null
        DB_FAILURE failure = DB_FAILURE::UNKNOWN;

        /// The message naming the problem when connection is null
        std::string error;
    };

    /**
     * Opens a connection described by aSettings, expanding ${CWD} in a connection string to the
     * directory holding the settings file.  The connection may be disconnected; check IsConnected().
     *
     * @param aConnectNow false defers connecting to the caller's Connect(), e.g. on a worker thread
     * @param aBaseDir replaces the settings file's directory for ${CWD}; used while the file has no path yet
     */
    static CONNECTION_RESULT CreateConnection( const DATABASE_LIB_SETTINGS& aSettings, bool aConnectNow = true,
                                               const wxString& aBaseDir = wxEmptyString );

    /// A problem found while previewing, tagged with the setting that caused it
    struct PREVIEW_ISSUE
    {
        enum class FIELD
        {
            KEY,          ///< The row as a whole, such as colliding keys or a row that could not be loaded
            REPEATED_KEY, ///< A key that more than one row has
            SYMBOLS,      ///< The symbols column
            PIN_MAP       ///< The pin map column
        };

        FIELD    field;
        wxString message;
    };

    using PREVIEW_ISSUES = std::vector<PREVIEW_ISSUE>;

    /**
     * Builds symbols from already-fetched rows exactly as the library would name and load them,
     * without opening a connection.  References into database libraries are not resolved, so a
     * preview never issues queries beyond the rows it was given.
     *
     * @param aLibs resolves the symbols column; may be null for metadata-only symbols
     * @param aIssues receives per-row problems such as malformed pin maps; may be null
     */
    static std::vector<std::unique_ptr<LIB_SYMBOL>>
    BuildPreviewSymbols( SYMBOL_LIBRARY_ADAPTER* aLibs, const DATABASE_LIB_TABLE& aTable, bool aGloballyUniqueKeys,
                         const std::vector<DATABASE_CONNECTION::ROW>& aRows, PREVIEW_ISSUES* aIssues );

private:
    typedef std::vector<std::pair<const DATABASE_LIB_TABLE*, std::vector<DATABASE_CONNECTION::ROW>>> TABLE_RESULT_LIST;

    void cacheLib( const std::shared_ptr<DATABASE_CONNECTION>& aConn );

    size_t computeSignature( const TABLE_RESULT_LIST& aTableResults ) const;

    bool  materializeCache( const TABLE_RESULT_LIST& aTableResults,
                            std::map<wxString, std::unique_ptr<LIB_SYMBOL>>& aSymbolCache,
                            std::map<wxString, std::pair<std::string, std::string>>& aSanitizedNameMap );

    void ensureSettings( const wxString& aSettingsPath );

    /// Loads the settings file on a thread that can be abandoned, so a share that went away cannot hold the caller
    /// past the wait or past a cancel; throws when the file could not be reached
    void loadSettingsBounded( int aTimeoutSeconds );

    /// Why the last connect() attempt failed, for the caller to report from its own copy
    struct CONNECT_STATUS
    {
        DB_FAILURE failure = DB_FAILURE::UNKNOWN;
        wxString   error;
    };

    /// Copies the connection under the cache lock so the caller keeps it alive across a reconnect
    std::shared_ptr<DATABASE_CONNECTION> currentConnection();

    /// Returns a live connection or throws
    std::shared_ptr<DATABASE_CONNECTION> ensureConnection();

    /// Connects, and waits out a file another instance is opening at the same moment when the library never opened
    CONNECT_STATUS connect();

    /// One attempt at connect(), with m_connectMutex held
    CONNECT_STATUS connectOnce();

    /// @return true once the plugin is closing, so no wait should outlast the call that notices
    bool connectionStopped() const;

    /// Wraps a backend in a connection, or reports why there is none
    static CONNECTION_RESULT connectionFor( DB_BACKEND_RESULT&& aBackend, bool aConnectNow );

    std::unique_ptr<LIB_SYMBOL> loadSymbolFromRow( const wxString& aSymbolName,
                                                   const DATABASE_LIB_TABLE& aTable,
                                                   const DATABASE_CONNECTION::ROW& aRow );

    static std::optional<bool> boolFromAny( const std::any& aVal );

    /// Resolves one entry of a row's symbols column through the library adapter
    LIB_SYMBOL* resolveSourceSymbol( const LIB_ID& aId );

    /// Reports to the preview's issue list when there is one, otherwise to the reporter
    void reportIssue( PREVIEW_ISSUE::FIELD aField, const wxString& aMessage, SEVERITY aSeverity ) const;

    /// Set on the throwaway instance behind BuildPreviewSymbols()
    bool m_previewMode = false;

    /// Collects the problems found while building a preview; null outside BuildPreviewSymbols()
    PREVIEW_ISSUES* m_previewIssues = nullptr;

    SYMBOL_LIBRARY_ADAPTER* m_adapter;

    std::unique_ptr<DATABASE_LIB_SETTINGS> m_settings;

    /// Generally will be null if no valid connection is established
    std::shared_ptr<DATABASE_CONNECTION> m_conn;

    std::set<wxString> m_customFields;

    std::set<wxString> m_defaultShownFields;

    std::map<wxString, std::unique_ptr<LIB_SYMBOL>> m_nameToSymbolcache;
    std::map<wxString, std::pair<std::string, std::string>> m_sanitizedNameMap;

    long long m_cacheTimestamp;

    /// True once the LIB_SYMBOL cache has been materialized at least once.
    std::atomic<bool> m_cachePopulated{ false };

    int m_modifyHash = 0;

    /// Signature of the raw database rows at last materialization; used to skip rebuilding the
    /// LIB_SYMBOL cache when a re-query returns identical data.
    size_t m_cacheSignature;

    /// LIB_IDs whose resolution is in flight on the calling thread, used to break self-referential
    /// cycles where a row's Symbols column points back into the same database library (issue #24249).
    /// The recursion is synchronous, so the chain is per thread; sharing the set would let concurrent
    /// loads of the same symbol report a false cycle and would race on the container itself.
    static thread_local std::unordered_set<wxString> s_inProgressLoads;

    /// Re-entrancy guard for cacheLib(), tripped when a self-referential load routes back through
    /// the adapter into LoadSymbol mid-build.
    bool m_inCacheLib = false;

    void startBackgroundRefresh();
    void stopBackgroundRefresh();
    void backgroundRefreshWorker();

    /// Busy retries past this attempt stop doubling the window
    static constexpr int MAX_BUSY_ATTEMPT = 5;

    /**
     * @return the wait before retrying a refresh that found the database locked, drawn from the upper half of a
     *         window that starts at 3 s and doubles with each attempt up to max_age
     */
    static std::chrono::milliseconds busyRetryWait( long long aMaxAgeSeconds, int aAttempt, std::minstd_rand& aRng );

    /// The race this covers lasts milliseconds, so a file in use for longer has a real holder and is reported
    static constexpr std::chrono::seconds FIRST_CONNECT_BUDGET{ 3 };

    std::thread       m_refreshThread;
    std::atomic<bool> m_refreshRunning{ false };
    std::atomic<bool> m_stopping{ false };
    std::atomic<bool> m_loadCancelled{ false }; ///< Set by CancelLoad(), cleared by ResumeLoad()

    std::condition_variable m_refreshCV;
    std::mutex              m_refreshMutex;
    std::shared_mutex       m_cacheMutex;
    std::mutex              m_connectMutex;
    std::mutex              m_symbolLoadMutex;
};

#endif //SCH_IO_DATABASE_H_
