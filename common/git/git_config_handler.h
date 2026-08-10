/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef GIT_CONFIG_HANDLER_H
#define GIT_CONFIG_HANDLER_H

#include <git/git_repo_mixin.h>
#include <import_export.h>
#include <settings/common_settings.h>
#include <wx/string.h>

struct GitUserConfig
{
    wxString authorName;
    wxString authorEmail;
    bool hasName = false;
    bool hasEmail = false;
};

struct git_config;
struct git_repository;

namespace KIGIT
{
/**
 * Read a git config value, decoded as UTF-8.
 *
 * @return true when the key is set
 */
APIEXPORT bool ReadConfigString( git_config* aConfig, const char* aKey, wxString& aValue );

/**
 * Read the author set in a repository's own config and the one set in the global, XDG or system config.
 * A null repository reads only the global levels.
 * A value counts as set only when it is not empty, and the has flags say so.
 */
APIEXPORT void ReadAuthorConfig( git_repository* aRepo, GitUserConfig& aLocal, GitUserConfig& aGlobal );

/**
 * Resolve the author recorded for new commits.
 *
 * The repository's own config wins, so a Set Author for one repository is honored. Next comes the preference
 * when it is not set to the default author, then the global git config, then the workspace account. The OS user
 * name is not recorded as an author, callers may offer it as a prefill.
 *
 * @param aPrefs the author preference
 * @param aLocal the values set in the repository's own config
 * @param aGlobal the values set in the global, XDG or system config
 * @param aAccountName the workspace account's display name, empty when there is none
 * @param aAccountEmail the workspace account's email, empty when there is none
 * @return the author, whose has flags say which values were found
 */
APIEXPORT GitUserConfig ResolveAuthor( const COMMON_SETTINGS::GIT& aPrefs, const GitUserConfig& aLocal,
                                       const GitUserConfig& aGlobal, const wxString& aAccountName = wxEmptyString,
                                       const wxString& aAccountEmail = wxEmptyString );

/**
 * Whether aAfter changes the author ResolveAuthor gives against aBefore. The default mode ignores the stored name
 * and email, so a change to them alone counts only while the preference is custom.
 */
APIEXPORT bool AuthorPreferenceChanged( const COMMON_SETTINGS::GIT& aBefore, const COMMON_SETTINGS::GIT& aAfter );
} // namespace KIGIT

class APIEXPORT GIT_CONFIG_HANDLER : public KIGIT_REPO_MIXIN
{
public:
    GIT_CONFIG_HANDLER( KIGIT_COMMON* aCommon );
    virtual ~GIT_CONFIG_HANDLER();

    /**
     * Get the author (name and email) that new commits use, resolved by KIGIT::ResolveAuthor
     * @return GitUserConfig structure with author information
     */
    GitUserConfig GetUserConfig();
};

#endif // GIT_CONFIG_HANDLER_H
