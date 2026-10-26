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

#include <diff_merge/kicad_differ.h>
#include <ki_exception.h>
#include <progress_reporter.h>

#include <wx/filename.h>
#include <wx/intl.h>


namespace KICAD_DIFF
{

KICAD_DIFFER::~KICAD_DIFFER() = default;


void ReportDiffProgress( KICAD_DIFFER& aDiffer, PROGRESS_REPORTER* aProgress )
{
    if( !aProgress )
        return;

    aProgress->AdvancePhase( _( "Comparing..." ) );

    KICAD_DIFFER::OPTIONS options = aDiffer.GetOptions();

    options.progress = [aProgress]( double aFraction )
    {
        aProgress->SetCurrentProgress( aFraction );

        // Stops the pairing at once, which otherwise runs on for seconds after Cancel
        if( !aProgress->KeepRefreshing() )
            THROW_IO_CANCELLED();
    };

    aDiffer.SetOptions( options );
}


void KeepLoading( PROGRESS_REPORTER* aProgress )
{
    if( aProgress && !aProgress->KeepRefreshing() )
        THROW_IO_CANCELLED();
}


wxString LoadingMessage( const wxString& aFile, const wxString& aLabel )
{
    const wxString name = wxFileName( aFile ).GetFullName();

    if( aLabel.IsEmpty() )
        return wxString::Format( _( "Loading %s..." ), name );

    // Versions labels already start with the file name
    if( aLabel.StartsWith( name + wxS( " " ) ) )
        return wxString::Format( _( "Loading %s..." ), aLabel );

    return wxString::Format( _( "Loading %s (%s)..." ), name, aLabel );
}

} // namespace KICAD_DIFF
