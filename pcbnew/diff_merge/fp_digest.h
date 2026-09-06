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

#ifndef FP_DIGEST_H
#define FP_DIGEST_H

#include <wx/string.h>

class FOOTPRINT;


namespace KICAD_DIFF
{

enum class FOOTPRINT_DIGEST
{
    PARITY, ///< What the library parity check compares, so a placed copy matches its library footprint
    MEMBER  ///< Everything the library file carries, for telling one release of the footprint from another
};


/**
 * The release digest of a footprint, "v1:" and a SHA-256 in hex. It covers
 * what FOOTPRINT::FootprintNeedsUpdate compares for DRC, in the library's own frame.
 *
 * Equal digests mean that check finds no difference. It can still find none where the digests differ,
 * since it allows small tolerances that an exact hash cannot, as after a rotation off a quarter turn.
 *
 * MEMBER covers instead everything the library file carries apart from UUIDs, so a keywords edit counts.
 */
wxString FootprintDigest( const FOOTPRINT& aFootprint, FOOTPRINT_DIGEST aScope = FOOTPRINT_DIGEST::PARITY );

} // namespace KICAD_DIFF

#endif // FP_DIGEST_H
