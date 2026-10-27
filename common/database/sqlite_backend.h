/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#ifndef KICAD_SQLITE_BACKEND_H
#define KICAD_SQLITE_BACKEND_H

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <database/database_backend.h>

#include "db_worker_thread.h"

struct SQLITE_DB;


/**
 * Reads a SQLite database file directly, read only and with normal locking so that edits made by other
 * programs show on the next read.  Every SQLite call runs on a worker thread, so a file on a share that
 * stopped answering costs the caller a bounded wait, not a hung program.
 */
class SQLITE_BACKEND : public DATABASE_BACKEND
{
public:
    /**
     * @param aPath the database file, as UTF-8
     * @param aTimeoutSeconds how long SQLite waits on a lock; the caller never waits more than this plus 10 s
     */
    SQLITE_BACKEND( std::string aPath, int aTimeoutSeconds );

    ~SQLITE_BACKEND() override;

    bool Connect() override;

    void Disconnect() override;

    bool IsConnected() const override { return m_connected; }

    bool ListTables( std::vector<std::string>& aTables ) override;

    std::optional<DB_TABLE_INFO> DescribeTable( const std::string& aTable ) override;


    bool ReadTable( const DB_TABLE_INFO& aTable, const std::vector<std::string>& aColumns, size_t aMaxRows,
                    std::vector<std::string>& aLabels, const DB_ROW_SINK& aSink ) override;

    std::string GetLastError() const override { return m_lastError; }

    DB_FAILURE GetLastFailure() const override { return m_lastFailure; }

    /**
     * Builds the read-only SQLite URI for a file.  Separators are made forward slashes and everything but
     * unreserved characters is percent-encoded as UTF-8.  A drive letter gives file:/C:/..., and a UNC path
     * gives file:////server/share/..., since SQLite accepts only an empty authority.
     *
     * @param aWindowsPath true when backslashes separate directories, drive letters and UNC names apply
     */
    static std::string FileUri( const std::string& aPath, bool aWindowsPath );

    /// @return true for \\server\share and \\?\UNC\server\share paths, with either kind of slash
    static bool IsUncPath( const std::string& aPath );

    /// @return true if the file lives on a network file system; false when that cannot be told
    static bool IsNetworkPath( const std::string& aPath );

    /**
     * Decides from the text of /proc/self/mountinfo whether aPath lies on a FUSE mount that reaches a remote
     * machine.  Only the known remote subtypes (sshfs, rclone, s3fs, gcsfuse, smbnetfs, google-drive-ocamlfuse
     * and gvfsd-fuse) count, since other FUSE file systems are local and keep their memory maps.
     */
    static bool IsRemoteFuseMount( const std::string& aMountInfo, const std::string& aPath );

private:
    std::chrono::milliseconds callerBound() const;

    /// Runs aJob on the worker, recording why it did not finish as the failure
    template <typename R>
    std::optional<R> runOnWorker( std::function<R( DB_JOB_CONTEXT& )> aJob, const DB_ROW_SINK* aSink );

    /// Forgets the connection and closes it on a worker thread, never on the caller
    void dropConnection();

    void fail( DB_FAILURE aFailure, std::string aError );

    /// Records a failure of a call that ran, and drops the connection if the failure means it cannot be trusted
    void failCall( DB_FAILURE aFailure, std::string aMessage );

    /// Fails with a message and returns false unless a connection is open
    bool requireConnection();

    std::string m_path;
    int         m_timeout;

    std::unique_ptr<DB_WORKER_THREAD> m_worker;
    std::shared_ptr<SQLITE_DB>        m_db;

    std::atomic<bool> m_connected{ false };

    std::string m_lastError;
    DB_FAILURE  m_lastFailure = DB_FAILURE::UNKNOWN;
};

#endif // KICAD_SQLITE_BACKEND_H
