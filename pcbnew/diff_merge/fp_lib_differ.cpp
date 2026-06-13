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

#include "fp_lib_differ.h"

#include <footprint.h>
#include <io/io_utils.h>
#include <io/io_base.h>
#include <pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h>
#include <richio.h>
#include <trace_helpers.h>

#include <wx/log.h>


namespace KICAD_DIFF
{

FP_LIB_DIFFER::FP_LIB_DIFFER( const FOOTPRINT_MAP& aBefore, const FOOTPRINT_MAP& aAfter, const wxString& aPath ) :
        m_before( aBefore ),
        m_after( aAfter ),
        m_path( aPath )
{
}


FP_LIB_DIFFER::~FP_LIB_DIFFER() = default;


std::pair<std::vector<std::unique_ptr<FOOTPRINT>>, FP_LIB_DIFFER::FOOTPRINT_MAP>
FP_LIB_DIFFER::LoadLibrary( const wxString& aPrettyPath )
{
    std::vector<std::unique_ptr<FOOTPRINT>> owners;
    FOOTPRINT_MAP                           map;

    PCB_IO_KICAD_SEXPR io;
    wxArrayString      names;

    // Throws on corrupt/unreadable libraries; callers distinguish failures
    // from a deliberately-empty .pretty directory.
    io.FootprintEnumerate( names, aPrettyPath, false, nullptr );

    // Per-footprint loads propagate too; previously swallowing them let a
    // bilaterally-corrupt footprint vanish from the diff entirely.
    for( const wxString& name : names )
    {
        std::unique_ptr<FOOTPRINT> owner( io.FootprintLoad( aPrettyPath, name, false, nullptr ) );

        if( !owner )
        {
            wxLogTrace( traceDiffMerge, wxT( "FP_LIB_DIFFER: '%s' returned null on load" ), name );
            continue;
        }

        map[name] = owner.get();
        owners.push_back( std::move( owner ) );
    }

    return { std::move( owners ), std::move( map ) };
}


DOCUMENT_DIFF FP_LIB_DIFFER::Diff()
{
    return DiffLibraryByName(
            m_before, m_after, m_path, wxS( "pretty" ), wxS( "FOOTPRINT" ), m_options,
            []( const FOOTPRINT* aFp ) { return aFp->GetBoundingBox( false ); },
            []( const FOOTPRINT* aA, const FOOTPRINT* aB ) { return !( *aA == *aB ); } );
}

} // namespace KICAD_DIFF
