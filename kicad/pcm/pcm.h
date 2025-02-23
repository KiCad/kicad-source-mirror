/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Andrew Lutsenko, anlutsenko at gmail dot com
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PCM_H_
#define PCM_H_

#include "core/wx_stl_compat.h"
#include "pcm_data.h"
#include <json_schema_validator.h>
#include "widgets/wx_progress_reporters.h"
#include <functional>
#include <iostream>
#include <map>

#include <thread>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <wx/wx.h>


///< Contains list of all valid directories that get extracted from a package archive
const std::unordered_set<wxString> PCM_PACKAGE_DIRECTORIES( {
        "plugins",
        "footprints",
        "3dmodels",
        "symbols",
        "resources",
        "colors",
        "templates",
        "scripts"
} );


///< Package states
///< Package is available if it is not installed and there is a compatible version
///< Package is unavailable if it is not installed and there are no compatible versions
///< Pending states are intermediary when (un)installation is scheduled but not yet performed
enum PCM_PACKAGE_STATE
{
    PPS_AVAILABLE = 0,
    PPS_UNAVAILABLE = 1,
    PPS_INSTALLED = 2,
    PPS_PENDING_INSTALL = 3,
    PPS_PENDING_UNINSTALL = 4,
    PPS_UPDATE_AVAILABLE = 5,
    PPS_PENDING_UPDATE = 6,
};


///< Package actions
enum PCM_PACKAGE_ACTION
{
    PPA_INSTALL = 0,
    PPA_UNINSTALL = 1,
    PPA_UPDATE = 2,
};


typedef std::vector<std::pair<wxString, wxString>>            STRING_PAIR_LIST;
typedef std::vector<std::tuple<wxString, wxString, wxString>> STRING_TUPLE_LIST;


struct BACKGROUND_JOB;
class DIALOG_PCM;


/**
 * @brief Main class of Plugin and Content Manager subsystem
 *
 * This class handles logistics of repository management, caching, json validation,
 * tracking installed packages and provides some utility methods.
 *
 * Repository caching is done in $KICADX_3RD_PARTY/cache directory with each
 * repository storing it's metadata, packages and optionally resource files under
 * it's id subdirectory.
 *
 * Repository id is a prefix of sha256 of it's main url.
 *
 * JSON schema file is expected to be in $KICAD_DATA/schemas directory
 *
 * Installed packages are stored in <user_settings>/installed_packages.json
 * If that file is missing PCM will try to reconstruct it from existing
 * directory structure inside $KICADX_3RD_PARTY but package descriptions
 * and other metadata will be lost.
 */
class PLUGIN_CONTENT_MANAGER
{
public:
    PLUGIN_CONTENT_MANAGER( std::function<void( int )> aAvailableUpdateCallbac );
    ~PLUGIN_CONTENT_MANAGER();

    /**
     * @brief Saves metadata of installed packages to disk
     *
     * Path is <user settings>/installed_packages.json
     */
    void SaveInstalledPackages();

    /**
     * @brief Fetches repository metadata from given url
     *
     * @param aUrl URL of the repository
     * @param aRepository fetched repository metadata
     * @param aReporter progress reporter dialog to use for download
     * @return true if successful
     * @return false if URL could not be downloaded or result could not be parsed
     */
    bool FetchRepository( const wxString& aUrl, PCM_REPOSITORY& aRepository,
                          PROGRESS_REPORTER* aReporter );

    /**
     * @brief Validates json against a specific definition in the PCM schema
     *
     * @param aJson JSON object to validate
     * @param aUri JSON URI of a definition to validate against, default is root
     * @throws std::invalid_argument on validation failure
     */
    void ValidateJson( const nlohmann::json&     aJson,
                       const nlohmann::json_uri& aUri = nlohmann::json_uri( "#" ) ) const;

    /**
     * @brief Verifies SHA256 hash of a binary stream
     *
     * @param aStream input stream
     * @param aHash sha256 lowercase hex string
     * @return true if hash matches
     */
    bool VerifyHash( std::istream& aStream, const wxString& aHash ) const;

    /**
     * @brief Set list of repositories
     *
     * Discards cache of repositories that were previously cached but are not
     * on the new list of repositories.
     *
     * @param aRepositories list of <URL, name> pairs of repositories
     */
    void SetRepositoryList( const STRING_PAIR_LIST& aRepositories );

    /**
     * @brief Discard in-memory and on-disk cache of a repository
     *
     * @param aRepositoryId id of the repository
     */
    void DiscardRepositoryCache( const wxString& aRepositoryId );

    /**
     * @brief Mark package as installed
     *
     * @param aPackage package metadata
     * @param aVersion installed package version
     * @param aRepositoryId id of the source repository or empty estring if
     *                      installed from a local file
     */
    void MarkInstalled( const PCM_PACKAGE& aPackage, const wxString& aVersion,
                        const wxString& aRepositoryId );

    /**
     * @brief Mark package as uninstalled
     *
     * @param aPackage package metadata
     */
    void MarkUninstalled( const PCM_PACKAGE& aPackage );

    /**
     * @brief Get list of repositories
     *
     * @return const STRING_TUPLE_LIST& list of repositories in <id, URL, name> tuple format
     */
    const STRING_TUPLE_LIST& GetRepositoryList() const { return m_repository_list; }

    /**
     * @brief Cache specified repository packages and other metadata
     *
     * This method fetches latest repository metadata, checks if there is cache on disk,
     * compares it's last update timestamp to repository metadata and redownloads packages
     * list if necessary.
     * Then it checks sha256 if provided and parses the package list.
     * Parsed packages metadata is stored in memory.
     *
     * Process is repeated with resources file except it is just stored on disk and
     * any errors at this stage are ignored.
     *
     * @param aRepositoryId id of the repository to cache
     * @return true if packages metadata was cached successfully
     */
    bool CacheRepository( const wxString& aRepositoryId );

    /**
     * @brief Get the packages metadata from a previously cached repository
     *
     * This should only be called after a successful CacheRepository call
     *
     * @param aRepositoryId id of the repository
     * @return list of package metadata objects
     */
    const std::vector<PCM_PACKAGE>& GetRepositoryPackages( const wxString& aRepositoryId ) const;

    /**
     * @brief Get list of installed packages
     *
     * @return vector of PCM_INSTALLATION_ENTRY objects
     */
    const std::vector<PCM_INSTALLATION_ENTRY> GetInstalledPackages() const;

    /**
     * @brief Get the current version of an installed package
     *
     * @param aPackageId id of the package
     * @return current version
     * @throws std::out_of_range if package with given id is not installed
     */
    const wxString& GetInstalledPackageVersion( const wxString& aPackageId ) const;

    ///< Returns current 3rd party directory path
    const wxString& Get3rdPartyPath() const { return m_3rdparty_path; };

    /**
     * @brief Get current state of the package
     *
     * @param aRepositoryId repository id
     * @param aPackageId package id
     * @return PCM_PACKAGE_STATE
     */
    PCM_PACKAGE_STATE GetPackageState( const wxString& aRepositoryId, const wxString& aPackageId );

    /**
     * @brief Returns pinned status of a package
     *
     * @param aPackageId package id
     * @return true if package is installed and is pinned
     * @return false if package is not installed or not pinned
     */
    bool IsPackagePinned( const wxString& aPackageId ) const;

    /**
     * @brief Set the pinned status of a package
     *
     * no-op for not installed packages
     *
     * @param aPackageId package id
     * @param aPinned pinned status
     */
    void SetPinned( const wxString& aPackageId, const bool aPinned );

    /**
     * @brief Get the preferred package update version or empty string if there is none
     *
     * Works only for installed packages and returns highest compatible version greater
     * than currently installed that is at the same or higher (numerically lower)
     * version stability level.
     *
     * @param aPackage package
     * @return package version string
     */
    const wxString GetPackageUpdateVersion( const PCM_PACKAGE& aPackage );

    /**
     * @brief Downloads url to an output stream
     *
     * @param aUrl URL to download
     * @param aOutput output stream
     * @param aReporter progress dialog to use
     * @param aSizeLimit maximum download size, 0 for unlimited
     * @return true if download was successful
     * @return false if download failed or was too large
     */
    bool DownloadToStream( const wxString& aUrl, std::ostream* aOutput,
                           PROGRESS_REPORTER* aReporter,
                           const size_t       aSizeLimit = DEFAULT_DOWNLOAD_MEM_LIMIT );

    /**
     * @brief Get the approximate measure of how much given package matches the search term
     *
     * @param aPackage package metadata object
     * @param aSearchTerm search term
     * @return int search rank, higher number means better match, 0 means no match
     */
    int GetPackageSearchRank( const PCM_PACKAGE& aPackage, const wxString& aSearchTerm );

    /**
     * @brief Get the icon bitmaps for repository packages
     *
     * Repository package icons are taken from repository's resources zip file
     *
     * @param aRepositoryId id of the repository
     * @return map of package id -> bitmap
     */
    std::unordered_map<wxString, wxBitmap>
    GetRepositoryPackageBitmaps( const wxString& aRepositoryId );

    /**
     * @brief Get the icon bitmaps for installed packages
     *
     * Icons for installed packages are taken from package extracted files in
     * $KICADX_3RD_PARTY/resources/<packageid> directories
     *
     * @return map of package id -> bitmap
     */
    std::unordered_map<wxString, wxBitmap> GetInstalledPackageBitmaps();

    /**
     * @brief Set the Dialog Window
     *
     * PCM can effectively run in "silent" mode with a background thread that
     * reports to kicad manager window status bar. Setting valid window pointer here
     * will switch it to GUI mode with WX_PROGRESS_DIALOG popup for downloads.
     *
     * @param aDialog parent dialog for progress window
     */
    void SetDialogWindow( DIALOG_PCM* aDialog ) { m_dialog = aDialog; };

    /**
     * @brief Runs a background update thread that checks for new package versions
     */
    void RunBackgroundUpdate();

    /**
     * @brief Interrupts and joins() the update thread
     */
    void StopBackgroundUpdate();

    /**
     * @brief Stores 3rdparty path from environment variables
     */
    void ReadEnvVar();

    /**
     * @brief Parses version strings and calculates compatibility
     *
     * This should be called after loading package metadata from repository or from
     * installation entries
     *
     * @param aPackage package metadata object
     */
    static void PreparePackage( PCM_PACKAGE& aPackage );

private:
    ///< Default download limit of 10 Mb to not use too much memory
    static constexpr size_t DEFAULT_DOWNLOAD_MEM_LIMIT = 10 * 1024 * 1024;

    /**
     * @brief Downloads packages metadata to in memory stream, verifies hash and attempts to parse it
     *
     * @param aUrl URL of the packages metadata
     * @param aHash optional sha256 hash
     * @param aPackages resulting packages metadata list
     * @param aReporter progress dialog to use for download
     * @return true if packages were successfully downloaded, verified and parsed
     */
    bool fetchPackages( const wxString& aUrl, const std::optional<wxString>& aHash,
                        std::vector<PCM_PACKAGE>& aPackages, PROGRESS_REPORTER* aReporter );

    /**
     * @brief Get the cached repository metadata
     *
     * @param aRepositoryId id of the repository
     * @return const PCM_REPOSITORY&
     */
    const PCM_REPOSITORY& getCachedRepository( const wxString& aRepositoryId ) const;

    /**
     * @brief Updates metadata of installed packages from freshly fetched repo
     *
     * This completely replaces all fields including description.
     * Only exception is versions field, if currently installed version is missing
     * from the repo metadata it is manually added back in to correctly display in the
     * installed packages.
     *
     * @param aRepositoryId
     */
    void updateInstalledPackagesMetadata( const wxString& aRepositoryId );

    ///< Returns current UTC timestamp
    time_t getCurrentTimestamp() const;

    DIALOG_PCM*                                  m_dialog;
    std::unique_ptr<JSON_SCHEMA_VALIDATOR>       m_schema_validator;
    wxString                                     m_3rdparty_path;
    wxString                                     m_cache_path;
    std::unordered_map<wxString, PCM_REPOSITORY> m_repository_cache;
    STRING_TUPLE_LIST                            m_repository_list; // (id, name, url) tuples
    // Using sorted map to keep order of entries in installed list stable
    std::map<wxString, PCM_INSTALLATION_ENTRY> m_installed;
    const static std::tuple<int, int, int>     m_kicad_version;
    std::function<void( int )>                 m_availableUpdateCallback;
    std::thread                                m_updateThread;

    std::shared_ptr<BACKGROUND_JOB>              m_updateBackgroundJob;
};

#endif // PCM_H_
