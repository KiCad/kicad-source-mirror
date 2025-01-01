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
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <wx/tarstrm.h>
#include <wx/wfstream.h>
#include <wx/zstream.h>

#include <asset_archive.h>


ASSET_ARCHIVE::ASSET_ARCHIVE( const wxString& aFilePath, bool aLoadNow ) :
        m_filePath( aFilePath )
{
    if( aLoadNow )
        Load();
}


bool ASSET_ARCHIVE::Load()
{
    // We don't support hot-reloading yet
    if( !m_fileInfoCache.empty() )
        return false;

    wxFFileInputStream zipFile( m_filePath );

    if( !zipFile.IsOk() )
        return false;

    wxZlibInputStream stream( zipFile, wxZLIB_GZIP );
    wxTarInputStream tarStream( stream );
    wxTarEntry* entry;

    // Avoid realloc while reading: we're not going to get better than 2:1 compression
    m_cache.resize( 2 * zipFile.GetLength() );

    size_t offset = 0;

    while( ( entry = tarStream.GetNextEntry() ) != nullptr )
    {
        if( entry->IsDir() )
        {
            delete entry;
            continue;
        }

        size_t length = entry->GetSize();

        FILE_INFO fi;
        fi.offset = offset;
        fi.length = length;

        if( offset + length > m_cache.size() )
            m_cache.resize( m_cache.size() * 2 );

        tarStream.Read( &m_cache[offset], length );

        m_fileInfoCache[entry->GetName()] = fi;

        offset += length;

        delete entry;
    }

    m_cache.resize( offset );

    return true;
}


long ASSET_ARCHIVE::GetFileContents( const wxString& aFilePath, const unsigned char* aDest,
                                     size_t aMaxLen )
{
    wxFAIL_MSG( wxS( "Unimplemented" ) );
    return 0;
}


long ASSET_ARCHIVE::GetFilePointer( const wxString& aFilePath, const unsigned char** aDest )
{
    if( aFilePath.IsEmpty() )
        return -1;

    wxASSERT( aDest );

    if( !m_fileInfoCache.count( aFilePath ) )
        return -1;

    const FILE_INFO& fi = m_fileInfoCache.at( aFilePath );

    *aDest = &m_cache[fi.offset];

    return fi.length;
}
