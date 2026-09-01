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

#ifndef SYM_DIGEST_H
#define SYM_DIGEST_H

#include <wx/string.h>

class LIB_SYMBOL;
class SCH_SYMBOL;
struct MDW_RELEASE_ITEM;


namespace KICAD_DIFF
{

enum class SYMBOL_DIGEST
{
    PARITY, ///< What ERC compares, so a placed copy matches its library symbol
    MEMBER  ///< Everything the library file saves, for telling one release of the symbol from another
};


/**
 * The release digest of a library symbol, "v1:" and a SHA-256 in hex. It covers
 * the flattened symbol as the library writer formats it, so a parent's edit counts toward each derived
 * symbol, and leaves out the Lifecycle field. Empty when a derived symbol's parent is missing.
 *
 * Equal digests mean ERC finds no difference between the symbols. ERC can still find none where the
 * digests differ, since it skips attributes such as field styles and pin names that the digest covers.
 *
 * The MEMBER digest covers the symbol unflattened, as its library saves it, so it also tells which parent a
 * derived symbol names.
 */
wxString SymbolDigest( const LIB_SYMBOL& aSymbol, SYMBOL_DIGEST aScope = SYMBOL_DIGEST::PARITY );

/// Give aItem the digests a release of aSymbol records, both its PARITY and its MEMBER digest
void SetReleaseDigests( MDW_RELEASE_ITEM& aItem, const LIB_SYMBOL& aSymbol );

/// The digest of the library copy a placed symbol carries, which matches its library symbol's while
/// that copy is unchanged. The placed symbol's own field values and position do not count.
wxString PlacedSymbolDigest( const SCH_SYMBOL& aPlaced );

} // namespace KICAD_DIFF

#endif // SYM_DIGEST_H
