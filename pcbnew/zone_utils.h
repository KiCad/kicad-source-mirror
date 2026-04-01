/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#pragma once

#include <memory>
#include <vector>

class BOARD;
class PROGRESS_REPORTER;
class ZONE;


/**
 * Merges zones with identical outlines and nets on different layers into single multi-layer zones.
 *
 * @param aZones is the zones to merge. Ownership of all the zones is taken (some will be returned)
 *
 * @return the merged zones. Ownership of all the returned zones is transferred to the caller.
 */
std::vector<std::unique_ptr<ZONE>> MergeZonesWithSameOutline( std::vector<std::unique_ptr<ZONE>>&& aZones );

/**
 * Automatically assign zone priorities based on connectivity analysis of overlapping regions.
 *
 * For each pair of overlapping zones, counts pads and vias per-net in the intersection area.
 * The zone whose net has more items in the overlap gets higher priority. When item counts are
 * within 20% of the larger count, the smaller zone gets higher priority. Overlap analysis for
 * each pair runs in parallel via the KiCad thread pool.
 *
 * @param aBoard the board whose zone priorities will be reassigned
 * @param aReporter optional progress reporter
 * @return true if any priorities were changed
 */
bool AutoAssignZonePriorities( BOARD* aBoard, PROGRESS_REPORTER* aReporter = nullptr );
