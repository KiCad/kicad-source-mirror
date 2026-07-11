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

#pragma once

#include <functional>

class BOARD;
class PCB_CONSTRAINT;
class UNITS_PROVIDER;
class wxListCtrl;
struct BOARD_CONSTRAINT_DIAGNOSTICS;


/// Columns of a geometric-constraint list, shared by the dockable panel and the footprint dialog.
enum CONSTRAINT_LIST_COL
{
    CONSTRAINT_COL_ITEM_1 = 0,
    CONSTRAINT_COL_ITEM_2,
    CONSTRAINT_COL_TYPE,
    CONSTRAINT_COL_STATE,
    CONSTRAINT_COL_COUNT
};


/// Add the four constraint columns (Item 1 / Item 2 / Constraint / State) to a report-mode list.
void AddConstraintListColumns( wxListCtrl* aList );


/**
 * Fill @p aList with one row per constraint on @p aBoard (board-owned then footprint-owned), each
 * row showing up to two member cells, the type with its value in @p aUnits, and the diagnostic
 * state taken from the caller-supplied @p aDiag (so the board is solved once and shared, not
 * re-solved here).  @p aOnRow is invoked with the inserted row index and constraint so the caller
 * can record its own row->constraint mapping (a KIID or a raw pointer).  The list is cleared first.
 */
void PopulateConstraintList( wxListCtrl* aList, BOARD* aBoard, UNITS_PROVIDER* aUnits,
                             const BOARD_CONSTRAINT_DIAGNOSTICS& aDiag,
                             const std::function<void( long, PCB_CONSTRAINT* )>& aOnRow );
