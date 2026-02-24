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

#pragma once

#include <string>
#include <cstdint>

/**
 * @namespace TEXT_EVAL_VCS
 * VCS (Version Control System) utility functions for text evaluation.
 * These functions provide generic VCS operations that are currently implemented
 * for Git repositories, but can be extended to support other VCS systems in the future.
 */
namespace TEXT_EVAL_VCS
{
/**
 * Get the current HEAD commit identifier (hash).
 *
 * @param aPath Optional file path for file-specific queries. Defaults to "." for repo HEAD.
 * @param aLength Optional hash length (4-40). Defaults to 40 for full SHA-1.
 * @return Commit hash as hex string, or empty if not in a VCS repository.
 */
std::string GetCommitHash( const std::string& aPath = ".", int aLength = 40 );

/**
 * Get the nearest tag/label from HEAD.
 *
 * @param aMatch Optional glob pattern to filter tags. Defaults to "*" for all tags.
 * @param aAnyTags If true, includes lightweight tags. If false, annotated tags only.
 * @return Tag name, or empty string if no matching tag found.
 */
std::string GetNearestTag( const std::string& aMatch = "", bool aAnyTags = false );

/**
 * Get the number of commits since the nearest matching tag.
 *
 * @param aMatch Optional glob pattern to filter tags.
 * @param aAnyTags If true, includes lightweight tags. If false, annotated tags only.
 * @return Distance in commits, or 0 if no tag found or at a tag.
 */
int GetDistanceFromTag( const std::string& aMatch = "", bool aAnyTags = false );

/**
 * Check if the repository has uncommitted changes.
 *
 * @param aIncludeUntracked If true, untracked files count as dirty. If false, only staged/modified.
 * @return true if repository is dirty, false if clean or not in a VCS repository.
 */
bool IsDirty( bool aIncludeUntracked = false );

/**
 * Get the author name of the HEAD commit.
 *
 * @param aPath Optional file path for file-specific queries.
 * @return Author name, or empty string if not available.
 */
std::string GetAuthor( const std::string& aPath = "." );

/**
 * Get the author email of the HEAD commit.
 *
 * @param aPath Optional file path for file-specific queries.
 * @return Author email, or empty string if not available.
 */
std::string GetAuthorEmail( const std::string& aPath = "." );

/**
 * Get the committer name of the HEAD commit.
 *
 * @param aPath Optional file path for file-specific queries.
 * @return Committer name, or empty string if not available.
 */
std::string GetCommitter( const std::string& aPath = "." );

/**
 * Get the committer email of the HEAD commit.
 *
 * @param aPath Optional file path for file-specific queries.
 * @return Committer email, or empty string if not available.
 */
std::string GetCommitterEmail( const std::string& aPath = "." );

/**
 * Get the current branch name.
 *
 * @return Branch name (e.g., "main", "develop"), or empty string if detached or not in VCS.
 */
std::string GetBranch();

/**
 * Get the commit timestamp (Unix time) of the HEAD commit.
 *
 * @param aPath Optional file path for file-specific queries.
 * @return Unix timestamp in seconds, or 0 if not available.
 */
int64_t GetCommitTimestamp( const std::string& aPath = "." );

/**
 * Get the commit date of the HEAD commit as a timestamp string.
 *
 * @param aPath Optional file path for file-specific queries.
 * @return Timestamp as string (to be formatted by caller), or empty string if not available.
 */
std::string GetCommitDate( const std::string& aPath = "." );

} // namespace TEXT_EVAL_VCS
