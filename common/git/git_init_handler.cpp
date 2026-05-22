/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#include "git_init_handler.h"
#include "git_backend.h"
#include <git/kicad_git_common.h>
#include <git/kicad_git_memory.h>
#include <trace_helpers.h>
#include <set>
#include <wx/ffile.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/tokenzr.h>


// Seed the project .gitignore with KiCad-generated paths that should never be
// committed (local history, backups, autosaves, lock files, footprint cache,
// project-local settings which may hold remote URLs and usernames).
// Append-only: existing entries are preserved.
static void ensureProjectGitignore( const wxString& aProjectPath )
{
    static const wxString kicadEntries[] = {
        wxS( ".history/" ),     wxS( "*-backups/" ), wxS( "_autosave-*" ),
        wxS( "fp-info-cache" ), wxS( "~*.lck" ),     wxS( "*.kicad_prl" ),
    };

    wxFileName         ignoreFile( aProjectPath, wxS( ".gitignore" ) );
    std::set<wxString> existing;
    bool               fileExists = ignoreFile.FileExists();
    bool               hasTrailingNewline = true;

    if( fileExists )
    {
        wxFFile  in( ignoreFile.GetFullPath(), wxT( "r" ) );
        wxString contents;

        if( !in.IsOpened() || !in.ReadAll( &contents ) )
            return;

        hasTrailingNewline = contents.empty() || contents.EndsWith( wxS( "\n" ) );

        wxStringTokenizer tok( contents, wxS( "\n" ) );

        while( tok.HasMoreTokens() )
        {
            wxString line = tok.GetNextToken();
            line.Trim().Trim( false );

            if( !line.empty() && !line.StartsWith( wxS( "#" ) ) )
                existing.insert( line );
        }
    }

    wxString missing;

    for( const wxString& entry : kicadEntries )
    {
        if( existing.find( entry ) == existing.end() )
            missing += entry + wxS( "\n" );
    }

    if( missing.empty() )
        return;

    wxFFile out( ignoreFile.GetFullPath(), wxT( "a" ) );

    if( !out.IsOpened() )
        return;

    if( !fileExists )
        out.Write( wxS( "# KiCad-generated files and directories.\n" ) );
    else if( !hasTrailingNewline )
        out.Write( wxS( "\n" ) );

    out.Write( missing );
}


GIT_INIT_HANDLER::GIT_INIT_HANDLER( KIGIT_COMMON* aCommon ) : KIGIT_REPO_MIXIN( aCommon )
{}


GIT_INIT_HANDLER::~GIT_INIT_HANDLER()
{}


bool GIT_INIT_HANDLER::IsRepository( const wxString& aPath )
{
    return GetGitBackend()->IsRepository( this, aPath );
}


InitResult GIT_INIT_HANDLER::InitializeRepository( const wxString& aPath )
{
    InitResult result = GetGitBackend()->InitializeRepository( this, aPath );

    if( result == InitResult::Success )
        ensureProjectGitignore( aPath );

    return result;
}


bool GIT_INIT_HANDLER::SetupRemote( const RemoteConfig& aConfig )
{
    return GetGitBackend()->SetupRemote( this, aConfig );
}


void GIT_INIT_HANDLER::UpdateProgress( int aCurrent, int aTotal, const wxString& aMessage )
{
    ReportProgress( aCurrent, aTotal, aMessage );
}
