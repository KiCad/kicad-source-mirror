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

#include <diff_merge/merge_dispatch.h>

#include <cli/exit_codes.h>
#include <kiface_base.h>
#include <kiface_ids.h>
#include <kiway.h>


namespace KICAD_DIFF
{

namespace
{
/// PCB and footprint kinds live in the pcbnew kiface; schematic and symbol
/// kinds in eeschema.
KIWAY::FACE_T faceForKind( DOC_KIND aKind )
{
    switch( aKind )
    {
    case DOC_KIND::SCH:
    case DOC_KIND::SYM_LIB:
        return KIWAY::FACE_SCH;
    default:
        return KIWAY::FACE_PCB;
    }
}
} // namespace


int DispatchMerge( KIWAY& aKiway, DOC_KIND aKind, const wxString& aAncestor, const wxString& aOurs,
                   const wxString& aTheirs, const wxString& aOutput, bool aInteractive,
                   bool aSingleFile, REPORTER* aReporter )
{
    typedef int ( *MERGE_FN )( int, const wxString&, const wxString&, const wxString&,
                               const wxString&, bool, bool, REPORTER* );

    KIFACE* kiface = aKiway.KiFACE( faceForKind( aKind ) );

    if( !kiface )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    MERGE_FN fn = reinterpret_cast<MERGE_FN>( kiface->IfaceOrAddress( KIFACE_MERGE_DOCUMENT ) );

    if( !fn )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    return fn( static_cast<int>( aKind ), aAncestor, aOurs, aTheirs, aOutput, aInteractive,
               aSingleFile, aReporter );
}


int DispatchOpenDiffDialog( KIWAY& aKiway, DOC_KIND aKind, const wxString& aFileA,
                            const wxString& aFileB, const wxString& aLabelA, const wxString& aLabelB,
                            wxWindow* aParent, REPORTER* aReporter )
{
    typedef int ( *OPEN_FN )( int, const wxString&, const wxString&, const wxString&,
                              const wxString&, wxWindow*, REPORTER* );

    KIFACE* kiface = aKiway.KiFACE( faceForKind( aKind ) );

    if( !kiface )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    OPEN_FN fn = reinterpret_cast<OPEN_FN>( kiface->IfaceOrAddress( KIFACE_OPEN_DIFF_DIALOG ) );

    if( !fn )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    return fn( static_cast<int>( aKind ), aFileA, aFileB, aLabelA, aLabelB, aParent, aReporter );
}

} // namespace KICAD_DIFF
