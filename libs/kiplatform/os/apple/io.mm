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