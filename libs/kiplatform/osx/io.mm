/*
* This program source code file is part of KiCad, a free EDA CAD application.
*
* Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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