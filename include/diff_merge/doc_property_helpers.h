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

#ifndef KICAD_DIFF_DOC_PROPERTY_HELPERS_H
#define KICAD_DIFF_DOC_PROPERTY_HELPERS_H

#include <diff_merge/kicad_diff_types.h>

#include <page_info.h>

#include <utility>
#include <vector>


namespace KICAD_DIFF
{

/**
 * Append `DOC_PROP_PAGE_FORMAT` and/or `DOC_PROP_PAGE_ORIENTATION` deltas to
 * `aDeltas` when the two `PAGE_INFO` structs differ on either field.
 *
 * Shared between `SCH_DIFFER` (root + per-sheet paper diff) and
 * `PCB_DIFFER` so the property-name + DIFF_VALUE encoding can't drift
 * between document types as the paper-info struct evolves.
 *
 * Emit order is fixed: page-format delta appended first, page-orientation
 * delta second. Downstream marker rendering and diff JSON serialization
 * rely on this stable ordering to produce deterministic output across
 * runs.
 */
inline void AppendPaperDeltas( std::vector<PROPERTY_DELTA>& aDeltas, const PAGE_INFO& aBefore,
                               const PAGE_INFO& aAfter )
{
    if( aBefore.GetType() != aAfter.GetType() )
    {
        PROPERTY_DELTA d;
        d.name   = DOC_PROP_PAGE_FORMAT;
        d.before = DIFF_VALUE::FromEnum( static_cast<int>( aBefore.GetType() ),
                                         aBefore.GetTypeAsString().ToStdString() );
        d.after  = DIFF_VALUE::FromEnum( static_cast<int>( aAfter.GetType() ),
                                         aAfter.GetTypeAsString().ToStdString() );
        aDeltas.push_back( std::move( d ) );
    }

    if( aBefore.IsPortrait() != aAfter.IsPortrait() )
    {
        PROPERTY_DELTA d;
        d.name   = DOC_PROP_PAGE_ORIENTATION;
        d.before = DIFF_VALUE::FromBool( aBefore.IsPortrait() );
        d.after  = DIFF_VALUE::FromBool( aAfter.IsPortrait() );
        aDeltas.push_back( std::move( d ) );
    }
}

} // namespace KICAD_DIFF

#endif // KICAD_DIFF_DOC_PROPERTY_HELPERS_H
