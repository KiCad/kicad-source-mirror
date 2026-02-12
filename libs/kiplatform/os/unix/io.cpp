/*
* This program source code file is part of KiCad, a free EDA CAD application.
*
* Copyright (C) 2023 Mark Roszko <mark.roszko@gmail.com>
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

#include <kiplatform/io.h>

#include <wx/crt.h>
#include <wx/string.h>
#include <wx/filename.h>

#include <climits>
#include <dirent.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

FILE* KIPLATFORM::IO::SeqFOpen( const wxString& aPath, const wxString& aMode )
{
    FILE* fp = wxFopen( aPath, aMode );

    if( fp )
    {
        if( posix_fadvise( fileno( fp ), 0, 0, POSIX_FADV_SEQUENTIAL ) != 0 )
        {
            fclose( fp );
            fp = nullptr;
        }
    }

    return fp;
}

bool KIPLATFORM::IO::DuplicatePermissions( const wxString &aSrc, const wxString &aDest )
{
    struct stat sourceStat;
    if( stat( aSrc.fn_str(), &sourceStat ) == 0 )
    {
        mode_t permissions = sourceStat.st_mode & ( S_IRWXU | S_IRWXG | S_IRWXO );
        if( chmod( aDest.fn_str(), permissions ) == 0 )
        {
            return true;
        }
        else
        {
            // Handle error
            return false;
        }
    }
    else
    {
        // Handle error
        return false;
    }
}

bool KIPLATFORM::IO::MakeWriteable( const wxString& aFilePath )
{
    struct stat fileStat;
    if( stat( aFilePath.fn_str(), &fileStat ) == 0 )
    {
        // Add user write permission to existing permissions
        mode_t newPermissions = fileStat.st_mode | S_IWUSR;
        if( chmod( aFilePath.fn_str(), newPermissions ) == 0 )
        {
            return true;
        }
    }
    return false;
}

bool KIPLATFORM::IO::IsFileHidden( const wxString& aFileName )
{
    wxFileName fn( aFileName );

    return fn.GetName().StartsWith( wxT( "." ) );
}


void KIPLATFORM::IO::LongPathAdjustment( wxFileName& aFilename )
{
    // no-op
}


long long KIPLATFORM::IO::TimestampDir( const wxString& aDirPath, const wxString& aFilespec )
{
    long long timestamp = 0;

    std::string pattern( aFilespec.fn_str() );
    std::string dir_path( aDirPath.fn_str() );

    DIR* dir = opendir( dir_path.c_str() );

    if( dir )
    {
        for( dirent* dir_entry = readdir( dir ); dir_entry; dir_entry = readdir( dir ) )
        {
            // FNM_PERIOD skips dotfiles (hidden files), FNM_CASEFOLD for case-insensitive match
            if( fnmatch( pattern.c_str(), dir_entry->d_name, FNM_CASEFOLD | FNM_PERIOD ) != 0 )
                continue;

            std::string entry_path = dir_path + '/' + dir_entry->d_name;
            struct stat entry_stat;

            if( lstat( entry_path.c_str(), &entry_stat ) == 0 )
            {
                // Follow symlinks to get the actual file's timestamp
                if( S_ISLNK( entry_stat.st_mode ) )
                {
                    char    buffer[PATH_MAX + 1];
                    ssize_t pathLen = readlink( entry_path.c_str(), buffer, PATH_MAX );

                    if( pathLen > 0 )
                    {
                        struct stat linked_stat;
                        buffer[pathLen] = '\0';
                        std::string linked_path = dir_path + '/' + buffer;

                        if( lstat( linked_path.c_str(), &linked_stat ) == 0 )
                            entry_stat = linked_stat;
                    }
                }

                if( S_ISREG( entry_stat.st_mode ) )
                {
                    timestamp += entry_stat.st_mtime * 1000;
                    timestamp += entry_stat.st_size;
                }
            }
            else
            {
                // If we couldn't stat the file, use the name hash
                timestamp += (signed) std::hash<std::string>{}( std::string( dir_entry->d_name ) );
            }
        }

        closedir( dir );
    }

    return timestamp;
}


KIPLATFORM::IO::MAPPED_FILE::MAPPED_FILE( const wxString& aFileName )
{
    int fd = open( aFileName.fn_str(), O_RDONLY );

    if( fd < 0 )
    {
        throw std::runtime_error( std::string( "Cannot open file: " )
                                  + aFileName.ToStdString() );
    }

    struct stat st;

    if( fstat( fd, &st ) != 0 )
    {
        close( fd );
        throw std::runtime_error( std::string( "Cannot stat file: " )
                                  + aFileName.ToStdString() );
    }

    m_size = static_cast<size_t>( st.st_size );

    if( m_size == 0 )
    {
        close( fd );
        return;
    }

    void* ptr = mmap( nullptr, m_size, PROT_READ, MAP_PRIVATE, fd, 0 );
    close( fd );

    if( ptr == MAP_FAILED )
    {
        readIntoBuffer( aFileName );
        return;
    }

    madvise( ptr, m_size, MADV_SEQUENTIAL );
    m_data = static_cast<const uint8_t*>( ptr );
    m_isMapped = true;
}


KIPLATFORM::IO::MAPPED_FILE::~MAPPED_FILE()
{
    if( m_isMapped && m_data )
        munmap( const_cast<uint8_t*>( m_data ), m_size );
}
