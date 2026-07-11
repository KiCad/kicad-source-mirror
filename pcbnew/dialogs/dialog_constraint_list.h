/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
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

#ifndef DIALOG_CONSTRAINT_LIST_H_
#define DIALOG_CONSTRAINT_LIST_H_

#include <functional>
#include <vector>

#include <dialogs/dialog_constraint_list_base.h>

class BOARD;
class PCB_BASE_FRAME;
class PCB_CONSTRAINT;


/**
 * Lists every geometric constraint on the board with its type and diagnostic state, the way the
 * canvas cannot (constraints have no geometry).  The caller supplies callbacks to highlight a
 * constraint's members and to remove a constraint, so this dialog stays free of board mutation
 * and tool dependencies.
 */
class DIALOG_CONSTRAINT_LIST : public DIALOG_CONSTRAINT_LIST_BASE
{
public:
    DIALOG_CONSTRAINT_LIST( PCB_BASE_FRAME* aParent, BOARD* aBoard,
                            std::function<void( PCB_CONSTRAINT* )> aHighlight,
                            std::function<void( PCB_CONSTRAINT* )> aRemove );

    /// Rebuild the rows from the board's current constraints.
    void Populate();

private:
    PCB_CONSTRAINT* selectedConstraint() const;

    void onRowActivated( wxListEvent& aEvent ) override;
    void onDelete( wxCommandEvent& aEvent ) override;

    PCB_BASE_FRAME*                        m_parentFrame;
    BOARD*                                 m_board;
    std::vector<PCB_CONSTRAINT*>           m_rows;   ///< Row index -> constraint.
    std::function<void( PCB_CONSTRAINT* )> m_highlight;
    std::function<void( PCB_CONSTRAINT* )> m_remove;
};

#endif // DIALOG_CONSTRAINT_LIST_H_
