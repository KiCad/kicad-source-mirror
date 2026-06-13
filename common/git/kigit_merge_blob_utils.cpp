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
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <git/kigit_merge_blob_utils.h>
#include <git/kicad_git_memory.h>

#include <git2/buffer.h>
#include <git2/deprecated.h>

#include <cstring>


namespace KIGIT
{

std::string BlobToString( git_blob* aBlob )
{
    if( !aBlob )
        return {};

    const char*  raw  = static_cast<const char*>( git_blob_rawcontent( aBlob ) );
    std::size_t  size = git_blob_rawsize( aBlob );
    return std::string( raw, size );
}


int WriteToGitBuf( git_buf* aBuf, const std::string& aContent )
{
    if( !aBuf )
        return -1;

    if( git_buf_grow( aBuf, aContent.size() + 1 ) != 0 || !aBuf->ptr )
        return -1;

    std::memcpy( aBuf->ptr, aContent.data(), aContent.size() );
    aBuf->ptr[aContent.size()] = '\0';
    aBuf->size                  = aContent.size();
    return 0;
}


int LoadMergeBlobs( const git_merge_driver_source* aSource, MERGE_BLOBS& aBlobs )
{
    git_repository*        repo     = const_cast<git_repository*>( git_merge_driver_source_repo( aSource ) );
    const git_index_entry* ancestor = git_merge_driver_source_ancestor( aSource );
    const git_index_entry* ours     = git_merge_driver_source_ours( aSource );
    const git_index_entry* theirs   = git_merge_driver_source_theirs( aSource );

    if( !ours || !theirs )
        return GIT_PASSTHROUGH;

    git_blob* ancestorBlob = nullptr;
    git_blob* oursBlob     = nullptr;
    git_blob* theirsBlob   = nullptr;

    if( ancestor && git_blob_lookup( &ancestorBlob, repo, &ancestor->id ) != 0 )
        return GIT_ENOTFOUND;

    GitBlobPtr ancestorPtr( ancestorBlob );

    if( git_blob_lookup( &oursBlob, repo, &ours->id ) != 0 )
        return GIT_ENOTFOUND;

    GitBlobPtr oursPtr( oursBlob );

    if( git_blob_lookup( &theirsBlob, repo, &theirs->id ) != 0 )
        return GIT_ENOTFOUND;

    GitBlobPtr theirsPtr( theirsBlob );

    aBlobs.ancestor = ancestorBlob ? BlobToString( ancestorBlob ) : std::string{};
    aBlobs.ours     = BlobToString( oursBlob );
    aBlobs.theirs   = BlobToString( theirsBlob );

    return 0;
}


bool TryTrivialMerge( const MERGE_BLOBS& aBlobs, git_buf* aResult, int* aRc )
{
    if( aBlobs.ours == aBlobs.theirs )
    {
        *aRc = WriteToGitBuf( aResult, aBlobs.ours );
        return true;
    }

    if( !aBlobs.ancestor.empty() && aBlobs.ancestor == aBlobs.ours )
    {
        *aRc = WriteToGitBuf( aResult, aBlobs.theirs );
        return true;
    }

    if( !aBlobs.ancestor.empty() && aBlobs.ancestor == aBlobs.theirs )
    {
        *aRc = WriteToGitBuf( aResult, aBlobs.ours );
        return true;
    }

    return false;
}

} // namespace KIGIT
