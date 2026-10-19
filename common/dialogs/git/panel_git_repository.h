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
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PANEL_GIT_REPOSITORY_H
#define PANEL_GIT_REPOSITORY_H

#include <dialogs/git/git_transfer.h>
#include <dialogs/git/panel_git_repository_base.h>

class EDA_BASE_FRAME;
class GIT_ADVANCED_DIALOG;


/**
 * Show the repository's author and open the shared identity editor.
 */
class PANEL_GIT_REPOSITORY : public PANEL_GIT_REPOSITORY_BASE
{
public:
    PANEL_GIT_REPOSITORY( wxWindow* aParent, GIT_ADVANCED_DIALOG* aDialog, EDA_BASE_FRAME* aFrame );

    /// Read the repository identity on a worker and update the display.
    void Populate();

private:
    void onEditAuthor();

    GIT_ADVANCED_DIALOG* m_dialog;
    EDA_BASE_FRAME*      m_frame;

    KIGIT_QUERY m_configQuery;
    KIGIT_QUERY m_editQuery;
};

#endif // PANEL_GIT_REPOSITORY_H
