/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023, 2024 KiCad Developers, see AUTHORS.TXT for contributors.
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

#include "git_revert_handler.h"

#include <wx/log.h>
#include <wx/string.h>

#include <trace_helpers.h>


GIT_REVERT_HANDLER::GIT_REVERT_HANDLER( git_repository* aRepository )
{
    m_repository = aRepository;
}


GIT_REVERT_HANDLER::~GIT_REVERT_HANDLER()
{
}


bool GIT_REVERT_HANDLER::Revert( const wxString& aFilePath )
{
    m_filesToRevert.push_back( aFilePath );
    return true;
}


static void checkout_progress_cb( const char *path, size_t cur, size_t tot, void *payload )
{
    wxLogTrace( traceGit, wxS( "checkout_progress_cb: %s %zu/%zu" ), path, cur, tot );
}


static int checkout_notify_cb( git_checkout_notify_t why, const char *path,
                               const git_diff_file *baseline,
                               const git_diff_file *target,
                               const git_diff_file *workdir, void *payload )
{
    GIT_REVERT_HANDLER* handler = static_cast<GIT_REVERT_HANDLER*>(payload);

    if( why & ( GIT_CHECKOUT_NOTIFY_CONFLICT | GIT_CHECKOUT_NOTIFY_IGNORED
              | GIT_CHECKOUT_NOTIFY_UPDATED ) )
        handler->PushFailedFile( path );

    return 0;
}


void GIT_REVERT_HANDLER::PerformRevert()
{
    git_object* head_commit = NULL;
    git_checkout_options opts;
    git_checkout_init_options( &opts, GIT_CHECKOUT_OPTIONS_VERSION );

    // Get the HEAD commit
    if( git_revparse_single( &head_commit, m_repository, "HEAD" ) != 0 )
    {
        // Handle error. If we cannot get the HEAD, then there's no point proceeding.
        return;
    }

    opts.checkout_strategy = GIT_CHECKOUT_FORCE;
    char** paths =  new char*[m_filesToRevert.size()];

    for( size_t ii = 0; ii < m_filesToRevert.size(); ii++ )
    {
        // Set paths to the specific file
        paths[ii] = wxStrdup( m_filesToRevert[ii].ToUTF8() );
    }

    git_strarray arr = { paths, m_filesToRevert.size() };

    opts.paths = arr;
    opts.progress_cb = checkout_progress_cb;
    opts.notify_cb = checkout_notify_cb;
    opts.notify_payload = static_cast<void*>( this );

    // Attempt to checkout the file(s)
    if( git_checkout_tree(m_repository, head_commit, &opts ) != 0 )
    {
        const git_error *e = git_error_last();

        if( e )
        {
            wxLogTrace( traceGit, wxS( "Checkout failed: %d: %s" ), e->klass, e->message );
        }
    }

    // Free the HEAD commit
    for( size_t ii = 0; ii < m_filesToRevert.size(); ii++ )
        delete( paths[ii] );

    delete[] paths;

    git_object_free( head_commit );
}

