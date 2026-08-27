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

#ifndef DRILL_ENUMERATOR_H
#define DRILL_ENUMERATOR_H

#include <vector>

#include <drill/drill_operation.h>

class BOARD;
class PAD;


/**
 * The one place that decides what the board's holes are.
 *
 * Every drill consumer goes through here so that no two of them can disagree about the
 * board. Results come back in board order. Callers wanting the drill writers' ordering sort
 * afterwards.
 */
std::vector<DRILL_OPERATION> EnumerateDrillOperations( const BOARD& aBoard,
                                                       const DRILL_QUERY& aQuery );

/**
 * Whether a pad's hole is a slot rather than a round drill.
 *
 * One definition, because the drill writers, the chart and the IPC-2581 and ODB++ exporters
 * each used to decide this for themselves and did not agree. A pad whose drill shape is
 * circular but whose X and Y differ was a round hole to the drill files and a slot to the
 * exporters.
 */
bool IsDrillSlot( const PAD& aPad );

#endif // DRILL_ENUMERATOR_H
