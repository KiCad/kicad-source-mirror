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

#ifndef GIT_CLONE_HANDLER_H_
#define GIT_CLONE_HANDLER_H_

#include "kicad_git_common.h"
#include <import_export.h>
#include "git_repo_mixin.h"
#include "git_progress.h"

class APIEXPORT GIT_CLONE_HANDLER : public KIGIT_REPO_MIXIN
{
public:
    GIT_CLONE_HANDLER( KIGIT_COMMON* aCommon );
    ~GIT_CLONE_HANDLER();

    bool PerformClone();

    void SetBranch( const wxString& aBranch ) { m_branch = aBranch; }
    wxString GetBranch() const { return m_branch; }

    void SetClonePath( const wxString& aPath ) { m_clonePath = aPath; }
    wxString GetClonePath() const { return m_clonePath; }

    void SetRemote( const wxString& aRemote ) { GetCommon()->m_remote = aRemote; }

    void UpdateProgress( int aCurrent, int aTotal, const wxString& aMessage ) override;

private:
    wxString m_branch;
    wxString m_clonePath;
};


#endif
