/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2019 CERN
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

#include "pns_arc.h"
#include "pns_node.h"
#include "pns_utils.h"

namespace PNS {


const SHAPE_LINE_CHAIN ARC::Hull( int aClearance, int aWalkaroundThickness, int aLayer ) const
{
    return ArcHull( m_arc, aClearance, aWalkaroundThickness );
}


ARC* ARC::Clone() const
{
    ARC* a = new ARC( m_arc, m_net );

    a->m_parent = m_parent;
    a->m_sourceItem = m_sourceItem;

    a->m_movable = m_movable;
    a->m_layers = m_layers;
    a->m_marker = m_marker;
    a->m_rank = m_rank;
    a->m_routable = m_routable;

    return a;
}


OPT_BOX2I ARC::ChangedArea( const ARC* aOther ) const
{
    BOX2I tmp = Shape( -1 )->BBox();
    tmp.Merge( aOther->Shape( -1 )->BBox() );
    return tmp;
}

}
