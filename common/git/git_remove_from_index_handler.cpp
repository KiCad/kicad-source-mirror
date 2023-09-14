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

#include <wx/string.h>
#include <wx/log.h>

#include "git_remove_from_index_handler.h"

GIT_REMOVE_FROM_INDEX_HANDLER::GIT_REMOVE_FROM_INDEX_HANDLER( git_repository* aRepository )
{
    m_repository = aRepository;
    m_filesToRemove.clear();
}

GIT_REMOVE_FROM_INDEX_HANDLER::~GIT_REMOVE_FROM_INDEX_HANDLER()
{
}

bool GIT_REMOVE_FROM_INDEX_HANDLER::RemoveFromIndex( const wxString& aFilePath )
{
    // Test if file is currently in the index

    git_index* index = nullptr;
    size_t at_pos = 0;

    if( git_repository_index( &index, m_repository ) != 0 )
    {
        wxLogError( "Failed to get repository index" );
        return false;
    }

    if( git_index_find( &at_pos, index, aFilePath.ToUTF8().data() ) != 0 )
    {
        git_index_free( index );
        wxLogError( "Failed to find index entry for %s", aFilePath );
        return false;
    }

    git_index_free( index );

    m_filesToRemove.push_back( aFilePath );
    return true;
}

void GIT_REMOVE_FROM_INDEX_HANDLER::PerformRemoveFromIndex()
{
    for( auto& file : m_filesToRemove )
    {
        git_index* index = nullptr;
        git_oid oid;

        if( git_repository_index( &index, m_repository ) != 0 )
        {
            wxLogError( "Failed to get repository index" );
            return;
        }

        if( git_index_remove_bypath( index, file.ToUTF8().data() ) != 0 )
        {
            wxLogError( "Failed to remove index entry for %s", file );
            return;
        }

        if( git_index_write( index ) != 0 )
        {
            wxLogError( "Failed to write index" );
            return;
        }

        if( git_index_write_tree( &oid, index ) != 0 )
        {
            wxLogError( "Failed to write index tree" );
            return;
        }

        git_index_free( index );
    }
}