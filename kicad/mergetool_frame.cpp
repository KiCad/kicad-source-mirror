/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
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

#include "mergetool_frame.h"

#include <base_units.h>
#include <bitmaps/bitmap_types.h>
#include <cli/exit_codes.h>
#include <diff_merge/merge_dispatch.h>
#include <kiway.h>
#include <reporter.h>
#include <wildcards_and_files_ext.h>

#include <wx/crt.h>
#include <wx/filename.h>
#include <wx/msgdlg.h>


namespace
{
bool supportsMergetoolKind( KICAD_DIFF::DOC_KIND aKind )
{
    switch( aKind )
    {
    case KICAD_DIFF::DOC_KIND::PCB:
    case KICAD_DIFF::DOC_KIND::SCH:
    case KICAD_DIFF::DOC_KIND::SYM_LIB:
    case KICAD_DIFF::DOC_KIND::FOOTPRINT:
        return true;

    default:
        return false;
    }
}
} // namespace


MERGETOOL_FRAME::MERGETOOL_FRAME( KIWAY* aKiway, wxWindow* aParent,
                                  const MERGETOOL_PATHS& aPaths ) :
        KIWAY_PLAYER( aKiway, aParent, FRAME_MERGETOOL,
                      _( "KiCad Merge Tool" ), wxDefaultPosition,
                      wxWindow::FromDIP( wxSize( 400, 120 ), nullptr ),
                      wxDEFAULT_FRAME_STYLE | wxFRAME_NO_TASKBAR,
                      wxT( "MergeToolFrame" ), unityScale ),
        m_paths( aPaths )
{
}


MERGETOOL_FRAME::~MERGETOOL_FRAME() = default;


void MERGETOOL_FRAME::doCloseWindow()
{
    // No persistent state to flush; the JOB writes the merged output
    // directly. The frame is just the modal-dialog host.
}


int MERGETOOL_FRAME::RunMerge()
{
    const KICAD_DIFF::DOC_KIND kind = KICAD_DIFF::DocKindFromExtension( m_paths.output );

    if( !supportsMergetoolKind( kind ) )
    {
        wxMessageBox( wxString::Format( _( "No mergetool handler for output '%s'.\n"
                                           "Output extension must be .kicad_pcb, .kicad_sch, "
                                           ".kicad_sym, or .kicad_mod." ),
                                        m_paths.output ),
                      _( "KiCad Merge Tool" ), wxOK | wxICON_ERROR, this );
        return CLI::EXIT_CODES::ERR_ARGS;
    }

    // Interactive: the merge opens DIALOG_KICAD_MERGE_3WAY for unresolved conflicts.
    WX_STRING_REPORTER reporter;
    const bool         singleFile = kind == KICAD_DIFF::DOC_KIND::FOOTPRINT;
    int                exitCode = KICAD_DIFF::DispatchMerge( Kiway(), kind, m_paths.ancestor,
                                                            m_paths.ours, m_paths.theirs,
                                                            m_paths.output, /* interactive */ true,
                                                            singleFile, &reporter );

    if( reporter.HasMessage() )
    {
        // Surface the kiface's diagnostics to the terminal so a `git mergetool`
        // invocation can see what happened even when the dialog was dismissed.
        wxFprintf( stderr, wxT( "%s" ), reporter.GetMessages() );
    }

    return exitCode;
}

