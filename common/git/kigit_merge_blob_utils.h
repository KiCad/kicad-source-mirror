/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 */

#ifndef KIGIT_MERGE_BLOB_UTILS_H
#define KIGIT_MERGE_BLOB_UTILS_H

#include <kicommon.h>

#include <git2.h>
#include <git2/sys/merge.h>

#include <string>


namespace KIGIT
{

/**
 * Copy a libgit2 blob's raw bytes into a std::string. Returns an empty string
 * if the blob is null.
 */
KICOMMON_API std::string BlobToString( git_blob* aBlob );


/**
 * Allocate a libgit2-owned buffer big enough for `aContent` and copy the
 * bytes plus a trailing NUL. Uses git_buf_grow so libgit2 disposes the buffer
 * correctly via git_buf_dispose.
 *
 * Returns 0 on success, -1 on allocation failure.
 */
KICOMMON_API int WriteToGitBuf( git_buf* aBuf, const std::string& aContent );


/// Decoded ancestor/ours/theirs blob contents for a 3-way merge driver.
struct MERGE_BLOBS
{
    std::string ancestor;   ///< Empty when there is no common ancestor.
    std::string ours;
    std::string theirs;
};


/**
 * Look up the ancestor/ours/theirs blobs of a merge-driver source and decode
 * them into @p aBlobs. The blobs are freed before returning (only the strings
 * are kept), so callers need no cleanup.
 *
 * Returns 0 on success, GIT_PASSTHROUGH when ours/theirs are missing, or
 * GIT_ENOTFOUND when a blob lookup fails.
 */
KICOMMON_API int LoadMergeBlobs( const git_merge_driver_source* aSource, MERGE_BLOBS& aBlobs );


/**
 * Resolve the trivial 3-way cases (identical sides, or one side unchanged from
 * the ancestor). On a trivial hit the resolved content is written to @p aResult,
 * @p aRc is set to the WriteToGitBuf return code, and true is returned. Returns
 * false when a real merge is required.
 */
KICOMMON_API bool TryTrivialMerge( const MERGE_BLOBS& aBlobs, git_buf* aResult, int* aRc );


/**
 * Shared libgit2 merge-driver `apply` callback shim. Constructs @p DRIVER over
 * the source, runs its Merge(), and on success or conflict copies ours' path
 * and mode to the outputs. @p DRIVER must expose a `(git_merge_driver_source*,
 * git_buf*)` constructor and an `int Merge()` method.
 */
template <typename DRIVER>
int ApplyMergeDriver( const git_merge_driver_source* aSrc, const char** aPathOut,
                      unsigned int* aModeOut, git_buf* aMergedOut )
{
    if( !aSrc || !aPathOut || !aModeOut || !aMergedOut )
        return -1;

    DRIVER driver( const_cast<git_merge_driver_source*>( aSrc ), aMergedOut );
    int    rc = driver.Merge();

    if( rc == 0 || rc == GIT_EMERGECONFLICT )
    {
        if( const git_index_entry* oursEntry = git_merge_driver_source_ours( aSrc ) )
        {
            *aPathOut = oursEntry->path;
            *aModeOut = oursEntry->mode;
        }
    }

    return rc;
}

} // namespace KIGIT

#endif // KIGIT_MERGE_BLOB_UTILS_H
