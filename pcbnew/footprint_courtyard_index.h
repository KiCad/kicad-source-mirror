/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers.
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

#ifndef FOOTPRINT_COURTYARD_INDEX_H
#define FOOTPRINT_COURTYARD_INDEX_H

#include <functional>

#include <geometry/rtree.h>
#include <math/box2.h>

class BOARD;
class FOOTPRINT;

/**
 * Spatial index over footprint courtyard bounding boxes.
 *
 * intersectsCourtyard()-style rule predicates otherwise scan every footprint on the board for
 * each item under test, which is O(items x footprints) and dominates DRC on dense boards with
 * courtyard rules.  Indexing the courtyards lets those predicates visit only the footprints whose
 * courtyard can actually reach the item, while the precise per-side collision test downstream is
 * unchanged.
 */
class FOOTPRINT_COURTYARD_INDEX
{
public:
    /// Build the index from the board's footprint courtyards (which DRC has already cached).
    explicit FOOTPRINT_COURTYARD_INDEX( const BOARD* aBoard );

    /// Visit every footprint whose courtyard bounding box overlaps aBox.  The visitor returns
    /// true to keep searching and false to stop early, mirroring a linear scan's short-circuit.
    void QueryOverlapping( const BOX2I& aBox,
                           const std::function<bool( FOOTPRINT* )>& aVisitor ) const;

private:
    RTree<FOOTPRINT*, int, 2, double> m_tree;
};

#endif // FOOTPRINT_COURTYARD_INDEX_H
