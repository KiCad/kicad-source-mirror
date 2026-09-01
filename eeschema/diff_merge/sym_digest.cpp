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

#include "sym_digest.h"

#include <diff_merge/library_digest.h>
#include <diff_merge/sym_item_diff.h>
#include <lib_symbol.h>
#include <mdw/client/mdw_types.h>
#include <richio.h>
#include <sch_symbol.h>
#include <widgets/mdw_part_controller.h>
#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr_lib_cache.h>


namespace KICAD_DIFF
{

namespace
{
    wxString digestOf( LIB_SYMBOL& aPart )
    {
        if( SCH_FIELD* lifecycle = aPart.FindFieldCaseInsensitive( wxString::FromUTF8( MDW_LIFECYCLE_FIELD ) ) )
            aPart.RemoveField( lifecycle );

        STRING_FORMATTER formatter;
        SCH_IO_KICAD_SEXPR_LIB_CACHE::SaveSymbol( &aPart, formatter );

        return LibraryDigest( formatter.GetString() );
    }
} // namespace


wxString SymbolDigest( const LIB_SYMBOL& aSymbol, SYMBOL_DIGEST aScope )
{
    if( aScope == SYMBOL_DIGEST::MEMBER )
        return LibraryDigest( SavedSymbolText( aSymbol ) );

    std::unique_ptr<LIB_SYMBOL> part = aSymbol.Flatten();

    if( !part )
        return wxEmptyString;

    return digestOf( *part );
}


void SetReleaseDigests( MDW_RELEASE_ITEM& aItem, const LIB_SYMBOL& aSymbol )
{
    aItem.digest = SymbolDigest( aSymbol );
    aItem.memberDigest = SymbolDigest( aSymbol, SYMBOL_DIGEST::MEMBER );
}


wxString PlacedSymbolDigest( const SCH_SYMBOL& aPlaced )
{
    const std::unique_ptr<LIB_SYMBOL>& cached = aPlaced.GetLibSymbolRef();

    if( !cached )
        return wxEmptyString;

    std::unique_ptr<LIB_SYMBOL> part = cached->Flatten();

    if( !part )
        return wxEmptyString;

    // The cache prefixes the nickname and renames coexisting revisions, so name it as its library does
    LIB_ID libId( wxEmptyString, aPlaced.GetLibId().GetUniStringLibItemName() );
    part->SetLibId( libId );
    part->SetName( libId.GetUniStringLibItemName() );

    return digestOf( *part );
}

} // namespace KICAD_DIFF
