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

#ifndef CONSTRAINT_COPY_H
#define CONSTRAINT_COPY_H

#include <map>
#include <set>
#include <vector>

#include <kiid.h>
#include <pcb_item_containers.h>

class PCB_SELECTION;
class PCB_CONSTRAINT;


/**
 * Shared logic deciding which constraints follow items through copy paste or duplicate
 *
 * A constraint references members by KIID so it travels only when every member is in the copy
 * otherwise paste would resolve to a mix of new and stale items Copy and duplicate both filter
 * on this rule so it lives here once
 */


/// The KIIDs of every selected item and all of its descendants, defining the scope a constraint
/// must fall entirely within to travel with a copy.
std::set<KIID> CollectConstraintScopeIds( const PCB_SELECTION& aSelection );


/// True when @p aConstraint has members and every one of them is present in @p aScopeIds, so the
/// whole relation is being copied rather than left half dangling.
bool ConstraintFullySelected( const PCB_CONSTRAINT* aConstraint, const std::set<KIID>& aScopeIds );


/**
 * Clone the constraints in @p aSource that a duplicate should carry
 *
 * @p aIdMap maps each original item KIID to its duplicate KIID a constraint whose members are all
 * keys is cloned with a fresh UUID and remapped to the duplicates one referencing an item outside
 * the map is dropped Caller owns the returned constraints
 */
std::vector<PCB_CONSTRAINT*> CloneFullySelectedConstraints( const CONSTRAINTS& aSource,
                                                            const std::map<KIID, KIID>& aIdMap );

#endif // CONSTRAINT_COPY_H
