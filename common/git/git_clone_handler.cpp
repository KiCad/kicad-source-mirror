/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.TXT for contributors.
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

#include "git_clone_handler.h"

#include <git/kicad_git_common.h>

#include <git2.h>
#include <wx/filename.h>

GIT_CLONE_HANDLER::GIT_CLONE_HANDLER() :  KIGIT_COMMON( nullptr )
{}

GIT_CLONE_HANDLER::~GIT_CLONE_HANDLER()
{
    if( m_repo )
        git_repository_free( m_repo );
}


bool GIT_CLONE_HANDLER::PerformClone()
{
    wxFileName clonePath( m_clonePath );

    if( !clonePath.DirExists() )
    {
        if( !clonePath.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) )
        {
            AddErrorString( wxString::Format( _( "Could not create directory '%s'" ), m_clonePath ) );
            return false;
        }
    }

    git_clone_options cloneOptions;
    git_clone_init_options( &cloneOptions, GIT_CLONE_OPTIONS_VERSION );
    cloneOptions.checkout_opts.checkout_strategy = GIT_CHECKOUT_SAFE;
    cloneOptions.checkout_opts.progress_cb = clone_progress_cb;
    cloneOptions.checkout_opts.progress_payload = this;
    cloneOptions.fetch_opts.callbacks.transfer_progress = transfer_progress_cb;
    cloneOptions.fetch_opts.callbacks.credentials = credentials_cb;
    cloneOptions.fetch_opts.callbacks.payload = this;

    m_testedTypes = 0;

    if( git_clone( &m_repo, m_URL.ToStdString().c_str(), m_clonePath.ToStdString().c_str(), &cloneOptions ) != 0 )
    {
        AddErrorString( wxString::Format( _( "Could not clone repository '%s'" ), m_URL ) );
        return false;
    }

    if( m_progressReporter )
        m_progressReporter->Hide();

    m_previousProgress = 0;

    return true;
}


void GIT_CLONE_HANDLER::UpdateProgress( int aCurrent, int aTotal, const wxString& aMessage )
{
    ReportProgress( aCurrent, aTotal, aMessage );
}
