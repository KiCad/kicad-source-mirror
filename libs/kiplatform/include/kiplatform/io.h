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
* You should have received a copy of the GNU General Public License along
* with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef KIPLATFORM_IO_H_
#define KIPLATFORM_IO_H_

#include <stdio.h>
#include <cstddef>
#include <cstdint>
#include <vector>

class wxString;
class wxFileName;

namespace KIPLATFORM
{
namespace IO
{
    /**
     * RAII wrapper for memory-mapped file I/O.
     *
     * Uses mmap on POSIX and MapViewOfFile on Windows. Falls back to reading
     * the entire file into a heap buffer if memory mapping is unavailable.
     */
    class MAPPED_FILE
    {
    public:
        explicit MAPPED_FILE( const wxString& aFileName );
        ~MAPPED_FILE();

        MAPPED_FILE( const MAPPED_FILE& ) = delete;
        MAPPED_FILE& operator=( const MAPPED_FILE& ) = delete;

        const uint8_t* Data() const { return m_data; }
        size_t         Size() const { return m_size; }

    private:
        void readIntoBuffer( const wxString& aFileName );

        const uint8_t* m_data = nullptr;
        size_t         m_size = 0;

#ifdef _WIN32
        void*          m_fileHandle = nullptr;
        void*          m_mapHandle = nullptr;
#else
        bool           m_isMapped = false;
#endif

        std::vector<uint8_t> m_fallbackBuffer;
    };

    /**
     * Buffer size for file I/O operations on cloud-synced folders.
     *
     * Cloud sync services like Google Drive, OneDrive, and Dropbox can report stale file sizes
     * during seek operations immediately after writing. Using a 512KB buffer reduces the number
     * of I/O operations and allows the cloud sync driver to flush data more reliably before
     * subsequent reads. This value was determined empirically to eliminate sync issues.
     */
    static constexpr size_t CLOUD_SYNC_BUFFER_SIZE = 512 * 1024;
    /**
     * Opens the file like fopen but sets flags (if available) for sequential read hinting.
     * Only use this variant of fopen if the file is truely going to be read sequentially only
     * otherwise you may encounter performance penalities.
     *
     * Windows in particular is a little ulgy to set the sequential scan flag compared
     * to say linux and it's posix_fadvise
     */
    FILE* SeqFOpen( const wxString& aPath, const wxString& mode );

    /**
     * Duplicates the file security data from one file to another ensuring that they are
     * the same between both.  This assumes that the user has permission to set #aDest
     * @return true if the process was successful
     */
    bool DuplicatePermissions( const wxString& aSrc, const wxString& aDest );

    /**
     * Ensures that a file has write permissions.
     * This is useful after copying files that may have been read-only.
     * @param aFilePath path to the file to make writeable
     * @return true if the process was successful
     */
    bool MakeWriteable( const wxString& aFilePath );

    /**
    * Helper function to determine the status of the 'Hidden' file attribute.
    * @return true if the file attribut is set.
    */
    bool IsFileHidden( const wxString& aFileName );

    /**
     * Adjusts a filename to be a long path compatible.
     * This is a no-op on non-Windows platforms.
     */
    void LongPathAdjustment( wxFileName& aFilename );

    /**
     * Computes a hash of modification times and sizes for files matching a pattern.
     * Used for cache invalidation by detecting changes to library directories.
     *
     * @param aDirPath  Directory to search
     * @param aFilespec Wildcard pattern to match (e.g. "*.kicad_mod")
     * @return Hash value that changes when any matching file is modified
     */
    long long TimestampDir( const wxString& aDirPath, const wxString& aFilespec );

    /**
     * Flushes user-space buffers for @p aFp and forces the kernel/filesystem to commit
     * the file's data blocks to stable storage.
     *
     * POSIX uses fflush + fsync. macOS uses fflush + fcntl F_FULLFSYNC, which is stronger
     * than fsync on APFS/HFS+ (the kernel flushes the drive's own write cache). Windows
     * uses fflush + FlushFileBuffers.
     *
     * @param aFp open FILE*; must remain open across the call.
     * @return true if every layer reported success.
     */
    bool FlushToDisk( FILE* aFp );

    /**
     * Forces a directory entry's metadata to stable storage. Without this, a rename()
     * that successfully returns can still be lost on power loss because the directory
     * inode isn't yet committed to the journal.
     *
     * POSIX opens the directory read-only and calls fsync on its fd. Windows returns
     * true without action since NTFS metadata journaling handles this automatically.
     *
     * @param aDirPath directory containing the files just renamed.
     * @return true on success or on platforms where the call is a no-op.
     */
    bool FlushDirectory( const wxString& aDirPath );

    /**
     * Atomically replaces @p aDst with @p aSrc. Both paths must live on the same
     * filesystem; cross-device renames are not atomic.
     *
     * POSIX uses rename(2), which is atomic w.r.t. concurrent opens. Windows uses
     * MoveFileExW with MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH and falls
     * back to ReplaceFileW for cases (antivirus, indexer, share-locked) that reject
     * MoveFileEx. A brief retry loop absorbs transient ERROR_SHARING_VIOLATION and
     * ERROR_ACCESS_DENIED on Windows.
     *
     * On success @p aSrc no longer exists; on failure it is left in place so the
     * caller can clean up.
     *
     * @param aSrc     source path, typically a sibling temp file of aDst.
     * @param aDst     destination path.
     * @param aError   optional out-parameter populated with a human-readable message.
     * @return true if the rename committed atomically.
     */
    bool AtomicRename( const wxString& aSrc, const wxString& aDst, wxString* aError = nullptr );

    /**
     * Returns a unique sibling path of @p aTargetPath suitable as an atomic-save temp
     * file. The sibling lives in the same directory so rename() stays atomic. Uniqueness
     * is provided by process pid plus a process-wide atomic counter.
     */
    wxString MakeSiblingTempPath( const wxString& aTargetPath );

    /**
     * Opens a fresh sibling temp file next to @p aTargetPath with exclusive-create
     * semantics (POSIX O_CREAT|O_EXCL, Windows CREATE_NEW). A pre-existing file at the
     * chosen temp path is never opened or truncated; if the candidate is taken the
     * helper retries with a new counter value.
     *
     * The returned FILE* is opened with @p aMode ("wb" typically). On success
     * @p aTempPathOut receives the path actually created. On failure the FILE* is
     * nullptr and @p aError (if provided) holds a human-readable message.
     *
     * Callers own the FILE* and must fclose/remove the temp file as appropriate.
     */
    FILE* OpenUniqueSiblingTempFile( const wxString& aTargetPath, const wxString& aMode,
                                     wxString* aTempPathOut, wxString* aError = nullptr );

    /**
     * If @p aPath is a symlink on POSIX, returns the canonical path of its referent so
     * atomic-save operations replace the underlying file rather than the link itself.
     * On Windows and when @p aPath is not a symlink (or does not yet exist) the input
     * is returned unchanged.
     */
    wxString ResolveSymlinkTarget( const wxString& aPath );

    /**
     * Opaque snapshot of filesystem attributes that MakeWriteable may alter and that the
     * atomic rename sequence does not preserve by itself. On Windows this holds the
     * FILE_ATTRIBUTE_READONLY / FILE_ATTRIBUTE_HIDDEN bits (which SetFileSecurity does
     * not carry). On POSIX this is an uncaptured placeholder since DuplicatePermissions
     * + rename already preserve mode.
     */
    struct TARGET_ATTRS
    {
        std::uint32_t value    = 0;
        bool          captured = false;
    };

    /**
     * Captures attributes of an existing @p aPath that must survive an atomic rename.
     * Windows reads GetFileAttributesW and stores READONLY/HIDDEN bits. POSIX returns
     * an uncaptured snapshot. Call before any mutation of the target (MakeWriteable).
     */
    TARGET_ATTRS CaptureTargetAttributes( const wxString& aPath );

    /**
     * Re-applies attributes previously captured by @ref CaptureTargetAttributes.
     *
     * Safe to call on either the happy path (restores HIDDEN/READONLY bits to the new
     * file produced by the rename, which DuplicatePermissions would not have carried) or
     * the failure path (rolls back a MakeWriteable mutation on the original target).
     *
     * @return true if the attributes were re-applied or the snapshot is a no-op; false
     *         only if the OS rejected the attribute change.
     */
    bool ApplyTargetAttributes( const wxString& aPath, const TARGET_ATTRS& aAttrs );

    /**
     * Completes an atomic save. Assumes @p aTempPath has been fully written, fsynced,
     * and closed. Clears any read-only/hidden attributes on an existing @p aTargetPath,
     * duplicates its permissions onto the temp file, atomically renames @p aTempPath
     * onto @p aTargetPath, and fsyncs the containing directory so the rename itself is
     * durable across power loss.
     *
     * On failure @p aTempPath is left in place for the caller to clean up.
     *
     * @param aTempPath   already-closed source temp file.
     * @param aTargetPath final destination path.
     * @param aError      optional human-readable failure message.
     * @return true on durable commit.
     */
    bool CommitTempFile( const wxString& aTempPath, const wxString& aTargetPath,
                         wxString* aError = nullptr );

    /**
     * Writes @p aData to @p aTargetPath via a sibling temp file, fsyncs the data and
     * directory, and atomically replaces the target. A crash or power loss at any
     * point leaves @p aTargetPath either byte-identical to its prior contents or
     * byte-identical to @p aData -- never truncated or partially written.
     *
     * If @p aTargetPath already exists its permissions are duplicated onto the
     * replacement, and any read-only/hidden attributes are cleared first so cloud
     * sync services don't block the rename.
     *
     * @param aTargetPath final destination path.
     * @param aData       pointer to bytes to write.
     * @param aSize       number of bytes in aData.
     * @param aError      optional out-parameter populated with a human-readable
     *                    message on failure.
     * @return true on durable commit of aData to aTargetPath.
     */
    bool AtomicWriteFile( const wxString& aTargetPath, const void* aData, size_t aSize,
                          wxString* aError = nullptr );
} // namespace IO
} // namespace KIPLATFORM

#endif // KIPLATFORM_IO_H_
