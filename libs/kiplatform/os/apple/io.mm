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

#include <kiplatform/io.h>

#import <Foundation/Foundation.h>

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
    return wxFopen( aPath, aMode );
}


bool KIPLATFORM::IO::DuplicatePermissions(const wxString& sourceFilePath, const wxString& destFilePath)
{
    NSString *sourcePath = [NSString stringWithUTF8String:sourceFilePath.utf8_str()];
    NSString *destPath = [NSString stringWithUTF8String:destFilePath.utf8_str()];

    NSFileManager *fileManager = [NSFileManager defaultManager];

    NSError *error;
    NSDictionary *sourceAttributes = [fileManager attributesOfItemAtPath:sourcePath error:&error];

    if( !sourceAttributes )
    {
        NSLog(@"Error retrieving source file attributes: %@", error);
        return false;
    }

    NSNumber *permissions = sourceAttributes[NSFilePosixPermissions];

    if (permissions == nil)
    {
        return false;
    }

    if ([fileManager setAttributes:@{NSFilePosixPermissions: permissions} ofItemAtPath:destPath error:&error])
    {
        return true;
    }
    else
    {
        NSLog(@"Error assigning permissions: %@", error);
        return false;
    }
}

bool KIPLATFORM::IO::MakeWriteable( const wxString& aFilePath )
{
    NSString *path = [NSString stringWithUTF8String:aFilePath.utf8_str()];
    NSFileManager *fileManager = [NSFileManager defaultManager];
    
    NSError *error;
    NSDictionary *attributes = [fileManager attributesOfItemAtPath:path error:&error];
    
    if( !attributes )
    {
        NSLog(@"Error retrieving file attributes: %@", error);
        return false;
    }
    
    NSNumber *permissions = attributes[NSFilePosixPermissions];
    
    if( permissions == nil )
    {
        return false;
    }
    
    // Add user write permission (S_IWUSR = 0200)
    unsigned short currentPerms = [permissions unsignedShortValue];
    unsigned short newPerms = currentPerms | 0200;
    
    if( [fileManager setAttributes:@{NSFilePosixPermissions: @(newPerms)} 
                        ofItemAtPath:path 
                               error:&error] )
    {
        return true;
    }
    else
    {
        NSLog(@"Error setting permissions: %@", error);
        return false;
    }
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
