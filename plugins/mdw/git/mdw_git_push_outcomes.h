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

/**
 * @file mdw_git_push_outcomes.h
 * @brief What a user's pushes to one remote taught about writing there.
 *
 * A server's hook or ACL refuses a push only when it is tried. An outcome seen in this process stands while the
 * credentials stay as they were. The last outcome is also kept, best effort, in a file the backend's processes
 * share, holding nothing derived from a credential. A later process reports it only as unknown, so a refusal that
 * a new token has since cured never locks the user out. Users with no key in their account share one record.
 */

#ifndef MDW_GIT_PUSH_OUTCOMES_H
#define MDW_GIT_PUSH_OUTCOMES_H

#include <api/workspace/workspace_types.pb.h>

#include <cstdint>
#include <filesystem>
#include <map>
#include <mutex>
#include <string>


class MDW_GIT_PUSH_OUTCOMES
{
public:
    struct OUTCOME
    {
        kiapi::workspace::PermissionState state = kiapi::workspace::PS_UNKNOWN;
        bool                              refusedBefore = false; ///< An earlier process saw a refusal
    };

    /// An empty aFile keeps nothing between processes
    explicit MDW_GIT_PUSH_OUTCOMES( std::filesystem::path aFile );

    /// The outcome of aUser's pushes to aRemote seen under credentials generation aGeneration
    OUTCOME Get( const std::string& aRemote, const std::string& aUser, std::uint64_t aGeneration ) const;

    /// Record that a push by aUser to aRemote under aGeneration was accepted or refused
    void Set( const std::string& aRemote, const std::string& aUser, std::uint64_t aGeneration,
              kiapi::workspace::PermissionState aState );

private:
    std::filesystem::path m_file;

    mutable std::mutex                                                                          m_mutex;
    std::map<std::string, std::pair<std::uint64_t, kiapi::workspace::PermissionState>> m_seen;
};

#endif // MDW_GIT_PUSH_OUTCOMES_H
