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

#ifndef KIGIT_TRIVIAL_MERGE_BASE_H
#define KIGIT_TRIVIAL_MERGE_BASE_H

#include <git/kigit_merge_blob_utils.h>


/**
 * CRTP base for libgit2 merge drivers that resolve only the trivial 3-way
 * cases inline and defer real conflicts to the mergetool. Each DERIVED
 * instantiation yields a distinct static Apply() entry point, so the three
 * drivers register as separate libgit2 drivers from one shared Merge() body.
 *
 * The non-trivial path writes the ours blob through so the working-tree file
 * stays parseable while git records GIT_EMERGECONFLICT.
 */
template <typename DERIVED>
class KIGIT_TRIVIAL_MERGE_BASE
{
public:
    KIGIT_TRIVIAL_MERGE_BASE( git_merge_driver_source* aSource, git_buf* aBuf ) :
            m_mergeDriver( aSource ),
            m_result( aBuf )
    {}

    int Merge()
    {
        KIGIT::MERGE_BLOBS blobs;

        if( int rc = KIGIT::LoadMergeBlobs( m_mergeDriver, blobs ); rc != 0 )
            return rc;

        int rc = 0;

        if( KIGIT::TryTrivialMerge( blobs, m_result, &rc ) )
            return rc;

        if( KIGIT::WriteToGitBuf( m_result, blobs.ours ) != 0 )
            return -1;

        return GIT_EMERGECONFLICT;
    }

    static int Apply( const git_merge_driver_source* aSrc, const char** aPathOut,
                      unsigned int* aModeOut, git_buf* aMergedOut )
    {
        return KIGIT::ApplyMergeDriver<DERIVED>( aSrc, aPathOut, aModeOut, aMergedOut );
    }

private:
    git_merge_driver_source* m_mergeDriver;
    git_buf*                 m_result;
};

#endif // KIGIT_TRIVIAL_MERGE_BASE_H
