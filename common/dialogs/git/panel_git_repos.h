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

#ifndef PANEL_GIT_REPOS_H
#define PANEL_GIT_REPOS_H

#include <git/panel_git_repos_base.h>
#include <widgets/wx_grid.h>

class PANEL_GIT_REPOS : public PANEL_GIT_REPOS_BASE
{
public:
    PANEL_GIT_REPOS( wxWindow* parent );
    ~PANEL_GIT_REPOS() override;

    void ResetPanel() override;

    bool TransferDataFromWindow() override;
    bool TransferDataToWindow() override;

    enum COLS
    {
        COL_ACTIVE = 0,
        COL_NAME,
        COL_PATH,
        COL_STATUS,
        COL_AUTH_TYPE,
        COL_USERNAME,
        COL_PASSWORD,
        COL_SSH_KEY,
        COL_SSH_PATH
    };

private:
    void onDefaultClick( wxCommandEvent& event ) override;
    void onEnableGitClick( wxCommandEvent& event ) override;

};

#endif // PANEL_GIT_REPOS_H