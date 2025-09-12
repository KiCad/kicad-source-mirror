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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef GIT_INIT_HANDLER_H
#define GIT_INIT_HANDLER_H

#include <git/git_repo_mixin.h>
#include <import_export.h>
#include <wx/string.h>

enum class InitResult
{
    Success,
    AlreadyExists,
    Error
};

struct RemoteConfig
{
    wxString url;
    wxString username;
    wxString password;
    wxString sshKey;
    KIGIT_COMMON::GIT_CONN_TYPE connType;
};

class APIEXPORT GIT_INIT_HANDLER : public KIGIT_REPO_MIXIN
{
public:
    GIT_INIT_HANDLER( KIGIT_COMMON* aCommon );
    virtual ~GIT_INIT_HANDLER();

    /**
     * Check if a directory is already a git repository
     * @param aPath Directory path to check
     * @return True if directory contains a git repository
     */
    bool IsRepository( const wxString& aPath );

    /**
     * Initialize a new git repository in the specified directory
     * @param aPath Directory path where to initialize the repository
     * @return InitResult indicating success, already exists, or error
     */
    InitResult InitializeRepository( const wxString& aPath );

    /**
     * Set up a remote for the repository
     * @param aConfig Remote configuration parameters
     * @return True on success, false on error
     */
    bool SetupRemote( const RemoteConfig& aConfig );

    void UpdateProgress( int aCurrent, int aTotal, const wxString& aMessage ) override;
};

#endif // GIT_INIT_HANDLER_H