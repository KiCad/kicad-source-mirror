/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Ian McInerney <Ian.S.McInerney at ieee.org>
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

#include <kiplatform/environment.h>

#import <Cocoa/Cocoa.h>
#include <wx/osx/core/cfstring.h>
#include <wx/filefn.h>
#include <wx/stdpaths.h>


void KIPLATFORM::ENV::Init()
{
    // Disable the automatic window tabbing OSX does
    [NSWindow setAllowsAutomaticWindowTabbing:NO];

    // No tasks for this platform
}


bool KIPLATFORM::ENV::MoveToTrash( const wxString& aPath, wxString& aError )
{
    bool     isDirectory = wxDirExists( aPath );
    NSURL*   url = [NSURL fileURLWithPath:wxCFStringRef( aPath ).AsNSString() isDirectory:isDirectory];
    NSError* err = nullptr;

    BOOL result = [[NSFileManager defaultManager] trashItemAtURL:url resultingItemURL:nil error:&err];

    // Extract the error string if the operation failed
    if( result == NO )
    {
        NSString* errmsg;

        if( err.localizedRecoverySuggestion == nil )
        {
            errmsg = err.localizedFailureReason;
        }
        else
        {
            errmsg = [err.localizedFailureReason stringByAppendingFormat:@"\n\n%@",
                      err.localizedRecoverySuggestion];
        }

        aError = wxCFStringRef::AsString( (CFStringRef) errmsg );
        return false;
    }

    return true;
}


bool KIPLATFORM::ENV::IsNetworkPath( const wxString& aPath )
{
    // placeholder, we "nerf" behavior if its a network path so return false by default
    return false;
}


wxString KIPLATFORM::ENV::GetDocumentsPath()
{
    return wxStandardPaths::Get().GetDocumentsDir();
}


wxString KIPLATFORM::ENV::GetUserConfigPath()
{
    return wxStandardPaths::Get().GetUserConfigDir();
}


wxString KIPLATFORM::ENV::GetUserDataPath()
{
    return wxStandardPaths::Get().GetUserDataDir();
}


wxString KIPLATFORM::ENV::GetUserLocalDataPath()
{
    return wxStandardPaths::Get().GetUserLocalDataDir();
}


wxString KIPLATFORM::ENV::GetUserCachePath()
{
    NSURL* url = [[NSFileManager defaultManager] URLForDirectory:NSCachesDirectory
                                                 inDomain:NSUserDomainMask
                                                 appropriateForURL:nil
                                                 create:NO error:nil];

    return wxCFStringRef::AsString( ( CFStringRef) url.path );
}


bool KIPLATFORM::ENV::GetSystemProxyConfig( const wxString& aURL, PROXY_CONFIG& aCfg )
{
    return false;
}


bool KIPLATFORM::ENV::VerifyFileSignature( const wxString& aPath )
{
    return true;
}


wxString KIPLATFORM::ENV::GetAppUserModelId()
{
    return wxEmptyString;
}


void KIPLATFORM::ENV::SetAppDetailsForWindow( wxWindow* aWindow, const wxString& aRelaunchCommand,
                                              const wxString& aRelaunchDisplayName )
{
}


wxString KIPLATFORM::ENV::GetCommandLineStr()
{
    return wxEmptyString;
}


void KIPLATFORM::ENV::AddToRecentDocs( const wxString& aPath )
{
}