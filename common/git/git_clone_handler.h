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

    void SetRemote( const wxString& aRemote ) { GetCommon()->SetRemote( aRemote ); }

    void UpdateProgress( int aCurrent, int aTotal, const wxString& aMessage ) override;

private:
    wxString m_branch;
    wxString m_clonePath;
};


#endif
