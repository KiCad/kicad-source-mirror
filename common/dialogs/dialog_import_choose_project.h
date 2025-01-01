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

#ifndef DIALOG_IMPORT_CHOOSE_PROJECT_H
#define DIALOG_IMPORT_CHOOSE_PROJECT_H

#include "dialog_import_choose_project_base.h"
#include <io/common/plugin_common_choose_project.h>


class DIALOG_IMPORT_CHOOSE_PROJECT : public DIALOG_IMPORT_CHOOSE_PROJECT_BASE
{
public:
    DIALOG_IMPORT_CHOOSE_PROJECT( wxWindow*                               aParent,
                                  const std::vector<IMPORT_PROJECT_DESC>& aProjectDesc );

    /**
     * Create and show a dialog (modal) and returns the data from it after completion. If the
     * dialog is closed or cancel is pressed, returns an empty vector.
     *
     * @param aParent Parent window for the invoked dialog.
     * @param aProjectDesc are project descriptors.
     */
    static std::vector<IMPORT_PROJECT_DESC>
    RunModal( wxWindow* aParent, const std::vector<IMPORT_PROJECT_DESC>& aProjectDesc );

    void onItemActivated( wxListEvent& event ) override;

    void onClose( wxCloseEvent& event ) override;

    std::vector<IMPORT_PROJECT_DESC> GetProjects();

private:
    std::vector<IMPORT_PROJECT_DESC> m_project_desc;
};

#endif // DIALOG_IMPORT_CHOOSE_PROJECT_H
