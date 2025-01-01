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

#include <core/version_compare.h>
#include <kiplatform/drivers.h>

#import <Foundation/Foundation.h>

#define MIN_MAC_VERSION "10.8.0"

bool KIPLATFORM::DRIVERS::Valid3DConnexionDriverVersion()
{
    NSFileManager *fileManager = [NSFileManager defaultManager];
    NSArray *searchPaths = NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSLocalDomainMask, YES);
    NSString *libraryDirectory = searchPaths.firstObject;
    NSString *frameworkPath = [libraryDirectory stringByAppendingPathComponent:@"Frameworks/3DconnexionNavlib.framework"];

    NSBundle *frameworkBundle = [NSBundle bundleWithPath:frameworkPath];
    NSDictionary *infoDictionary = [frameworkBundle infoDictionary];
    NSString *version = infoDictionary[@"CFBundleShortVersionString"];

    if( !version )
        return false;

    return compareVersionStrings( version.UTF8String, MIN_MAC_VERSION );
}