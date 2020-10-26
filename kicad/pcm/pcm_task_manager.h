/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Andrew Lutsenko, anlutsenko at gmail dot com
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef PCM_TASK_MANAGER_H_
#define PCM_TASK_MANAGER_H_

#include "dialogs/dialog_pcm_progress.h"
#include "pcm.h"
#include "pcm_data.h"
#include "sync_queue.h"
#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <nlohmann/json-schema.hpp>
#include <widgets/wx_progress_reporters.h>
#include <wx/string.h>


typedef std::function<void()> PCM_TASK;


/**
 * @brief Helper class that handles package (un)installation
 *
 * Package state changes are first enqueued using DownloadAndInstall/Uninstall methods
 * and then applied using RunQueue().
 *
 * RunQueue() is multithreaded for better experience.
 */
class PCM_TASK_MANAGER
{
public:
    PCM_TASK_MANAGER( std::shared_ptr<PLUGIN_CONTENT_MANAGER> pcm ) : m_pcm( pcm ){};

    /**
     * @brief Enqueue package download and installation
     *
     * Enqueues a download task for a given package version.
     *
     * Download task fetches the package archive and if successful enqueues an installation task.
     * Installation task verifies sha256 hash if specified, extracts the package, removes the
     * downloaded archive and marks package as installed.
     *
     * Both tasks report their state independently to a progress dialog.
     *
     * @param aPackage package metadata
     * @param aVersion version to be installed
     * @param aRepositoryId id of the source repository
     */
    void DownloadAndInstall( const PCM_PACKAGE& aPackage, const wxString& aVersion,
                             const wxString& aRepositoryId );

    /**
     * @brief Enqueue package uninstallation
     *
     * Enqueues uninstallation task that removes all package files and marks package
     * as uninstalled.
     *
     * @param aPackage package metadata
     */
    void Uninstall( const PCM_PACKAGE& aPackage );

    /**
     * @brief Run queue of pending actions
     *
     * This method spawns 2 threads to concurrently run tasks in download and install
     * queues until they are drained.
     *
     * Download queue feeds into install queue so the install thread keeps running until
     * download thread indicated that it's finished AND all installs are processed.
     *
     * @param aParent parent dialog for progress window
     */
    void RunQueue( wxWindow* aParent );

    /**
     * @brief Installs package from an archive file on disk
     *
     * Unlike DownloadAndInstall/Uninstall methods this one immediately extracts the package
     * and marks it as installed.
     *
     * @param aParent parent dialog for progress window
     * @param aFilePath path to the archive file
     */
    void InstallFromFile( wxWindow* aParent, const wxString& aFilePath );

private:
    /**
     * @brief Download URL to a file
     *
     * @param aFilePath path to file
     * @param aUrl URL to download
     * @return int CURLE error code
     */
    int downloadFile( const wxString& aFilePath, const wxString& aUrl );

    /**
     * @brief Extract package archive
     *
     * @param aFilePath path to the archive
     * @param aPackageId id of the package
     * @return true if archive was extracted successfuly
     */
    bool extract( const wxString& aFilePath, const wxString& aPackageId );

    /**
     * @brief Delete all package files
     *
     * @param aPackageId id of the package
     */
    void deletePackageDirectories( const wxString& aPackageId );

    DIALOG_PCM_PROGRESS*                    m_reporter;
    SYNC_QUEUE<PCM_TASK>                    m_download_queue;
    SYNC_QUEUE<PCM_TASK>                    m_install_queue;
    std::shared_ptr<PLUGIN_CONTENT_MANAGER> m_pcm;
};


#endif // PCM_TASK_MANAGER_H_
