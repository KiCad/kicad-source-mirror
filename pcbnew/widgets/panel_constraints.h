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

#ifndef PANEL_CONSTRAINTS_H_
#define PANEL_CONSTRAINTS_H_

#include <vector>

#include <kiid.h>
#include <widgets/panel_constraints_base.h>

class PCB_BASE_EDIT_FRAME;
class CONSTRAINT_EDIT_TOOL;
struct BOARD_CONSTRAINT_DIAGNOSTICS;
class wxMouseEvent;


/**
 * Dockable panel listing the board's geometric constraints (issue #2329).
 *
 * Each row shows the constrained items in their own columns (up to two), the constraint type
 * (with its value, in display units, when it has one), and the diagnostic state.  Clicking an
 * item cell highlights that item on the canvas; double-clicking a valued row opens its value
 * dialog.  Geometry-free constraints can't be picked on the canvas, so this is the place to see,
 * locate and delete them.
 */
class PANEL_CONSTRAINTS : public PANEL_CONSTRAINTS_BASE
{
public:
    PANEL_CONSTRAINTS( PCB_BASE_EDIT_FRAME* aFrame );

    /// Rebuild the rows from the board's current constraints (solving the board diagnosis here).
    void RefreshList();

    /// Rebuild the rows using a caller-supplied diagnosis, so the board is solved only once when
    /// the tool refreshes several views together.
    void RefreshList( const BOARD_CONSTRAINT_DIAGNOSTICS& aDiag );

    /// Select and reveal the row for @p aConstraint (e.g. when its badge is clicked on canvas).
    void SelectConstraint( const KIID& aConstraint );

private:
    /// The owning constraint tool, which performs all board mutation and selection; the panel only
    /// displays rows (holding KIIDs) and raises intents to it.
    CONSTRAINT_EDIT_TOOL* constraintTool() const;

    /// The KIID for a row, or niluuid if out of range.
    const KIID& rowConstraint( long aRow ) const;

    /// The list column under @p aPos within row @p aRow, or -1.  Uses cell rectangles because the
    /// HitTest subitem out-param is unreliable on the generic (GTK) list control.
    long columnAt( long aRow, const wxPoint& aPos ) const;

    void onLeftDown( wxMouseEvent& aEvent );
    void onRowActivated( wxListEvent& aEvent ) override;
    void onDelete( wxCommandEvent& aEvent ) override;
    void onRefresh( wxCommandEvent& aEvent ) override;

    PCB_BASE_EDIT_FRAME* m_frame;
    std::vector<KIID>    m_rows;
};

#endif // PANEL_CONSTRAINTS_H_
