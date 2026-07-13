/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2026 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file mdw_file_io.h
 * @brief Crash-safe file creation and cross-process locking for backends.
 *
 * New files are created exclusively, so an existing name, a planted symlink included, is never opened or
 * truncated.
 */

#ifndef MDW_FILE_IO_H
#define MDW_FILE_IO_H

#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <system_error>

/// The whole of aPath, or nothing when it cannot be read
std::optional<std::string> MDW_ReadFile( const std::filesystem::path& aPath );

/**
 * Replace aPath with aData through a new sibling renamed over it, so a reader never sees half a file. A file being
 * replaced keeps its permissions and a new one is created with aPosixMode.
 */
std::error_code MDW_ReplaceFile( const std::filesystem::path& aPath, const std::string& aData,
                                 unsigned aPosixMode = 0666 );

/**
 * MDW_ReplaceFile() that asks aCommit, once the new file is written and just before the rename, whether to go on.
 * When aCommit answers false, aPath is left alone and the call answers std::errc::operation_canceled. Checking aPath
 * there leaves only the rename to race, so a write landing between that check and the rename is still lost.
 */
std::error_code MDW_ReplaceFileIf( const std::filesystem::path& aPath, const std::string& aData,
                                   const std::function<bool()>& aCommit, unsigned aPosixMode = 0666 );

/**
 * An exclusive lock on aPath, created when missing, held until destruction. It excludes other
 * processes and other threads of this one, through flock on POSIX and LockFileEx on Windows.
 * With aWait false a lock held elsewhere is not waited for and Locked() is false.
 */
class MDW_FILE_LOCK
{
public:
    explicit MDW_FILE_LOCK( const std::filesystem::path& aPath, bool aWait = true );
    ~MDW_FILE_LOCK();

    MDW_FILE_LOCK( const MDW_FILE_LOCK& ) = delete;
    MDW_FILE_LOCK& operator=( const MDW_FILE_LOCK& ) = delete;

    bool Locked() const { return m_locked; }

    /// Not locked only because another holder has it, so waiting can succeed
    bool Busy() const { return m_busy; }

private:
    struct IMPL;
    std::unique_ptr<IMPL> m_impl;
    bool                  m_locked = false;
    bool                  m_busy = false;
};

#endif // MDW_FILE_IO_H
