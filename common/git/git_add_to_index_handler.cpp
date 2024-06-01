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

#include "git_add_to_index_handler.h"

#include <iterator>

#include <wx/string.h>
#include <wx/log.h>

GIT_ADD_TO_INDEX_HANDLER::GIT_ADD_TO_INDEX_HANDLER( git_repository* aRepository )
{
    m_repository = aRepository;
    m_filesToAdd.clear();
}

GIT_ADD_TO_INDEX_HANDLER::~GIT_ADD_TO_INDEX_HANDLER()
{
}


bool GIT_ADD_TO_INDEX_HANDLER::AddToIndex( const wxString& aFilePath )
{
    // Test if file is currently in the index

    git_index* index = nullptr;
    size_t at_pos = 0;

    if( git_repository_index( &index, m_repository ) != 0 )
    {
        wxLogError( "Failed to get repository index" );
        return false;
    }

    if( git_index_find( &at_pos, index, aFilePath.ToUTF8().data() ) == GIT_OK )
    {
        git_index_free( index );
        wxLogError( "%s already in index", aFilePath );
        return false;
    }

    git_index_free( index );

    // Add file to index if not already there
    m_filesToAdd.push_back( aFilePath );

    return true;
}


bool GIT_ADD_TO_INDEX_HANDLER::PerformAddToIndex()
{
    git_index* index = nullptr;

    m_filesFailedToAdd.clear();

    if( git_repository_index( &index, m_repository ) != 0 )
    {
        wxLogError( "Failed to get repository index" );
        std::copy( m_filesToAdd.begin(), m_filesToAdd.end(), std::back_inserter( m_filesFailedToAdd ) );
        return false;
    }

    for( auto& file : m_filesToAdd )
    {
        if( git_index_add_bypath( index, file.ToUTF8().data() ) != 0 )
        {
            wxLogError( "Failed to add %s to index", file );
            m_filesFailedToAdd.push_back( file );
            continue;
        }
    }


    if( git_index_write( index ) != 0 )
    {
        wxLogError( "Failed to write index" );
        m_filesFailedToAdd.clear();
        std::copy( m_filesToAdd.begin(), m_filesToAdd.end(), std::back_inserter( m_filesFailedToAdd ) );
        git_index_free( index );
        return false;
    }

    git_index_free( index );

    return true;
}