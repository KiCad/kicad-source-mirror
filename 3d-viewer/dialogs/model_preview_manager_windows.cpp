/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-3.0.html
 */

#include <model_preview_manager.h>
#include <model_preview_manager_windows_internal.h>

#include <wx/filename.h>

#include <windows.h>
#include <winver.h>

#include <array>
#include <vector>

#pragma comment( lib, "version.lib" )

namespace
{
constexpr wchar_t                       HANDLER_CLSID[] = L"{072BD71D-CB70-4CC8-8F08-D0A763A17C2E}";
constexpr wchar_t                       HANDLER_APPID[] = L"{6D2B5079-2F0B-48DD-AB7F-97CEC514D30B}";
constexpr wchar_t                       PREVIEW_HANDLER_IID[] = L"{8895B1C6-B41F-4C1C-A562-0D564250836F}";
constexpr wchar_t                       REQUIREMENT_ID[] = L"windows-preview-handler-v1";
constexpr wchar_t                       HANDLER_NAME[] = L"kicad_3d_model_previewer.dll";
// Must match fileTypes[] in the preview handler's dllmain.cpp, which writes the registration.
constexpr std::array<const wchar_t*, 9> EXTENSIONS = { L".stp",  L".stpz", L".step", L".stepz", L".igs",
                                                       L".iges", L".wrl",  L".wrz",  L".x3d" };

enum class READ_RESULT
{
    FOUND,
    MISSING,
    READ_ERROR
};

READ_RESULT ReadRegistryString( HKEY aRoot, const wxString& aKey, const wchar_t* aValueName, wxString& aValue )
{
    std::array<wchar_t, 2048> value;
    DWORD                     size = static_cast<DWORD>( value.size() * sizeof( wchar_t ) );
    LONG                      result = RegGetValueW( aRoot, aKey.wc_str(), aValueName,
                                                     RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ | RRF_NOEXPAND, nullptr,
                                                     value.data(), &size );

    if( result == ERROR_FILE_NOT_FOUND )
        return READ_RESULT::MISSING;

    if( result != ERROR_SUCCESS )
        return READ_RESULT::READ_ERROR;

    aValue = value.data();

    return READ_RESULT::FOUND;
}


READ_RESULT ReadString( const wxString& aKey, const wchar_t* aValueName, wxString& aValue )
{
    return ReadRegistryString( HKEY_CURRENT_USER, aKey, aValueName, aValue );
}


READ_RESULT ReadClassString( const wxString& aKey, const wchar_t* aValueName, wxString& aValue )
{
    return ReadRegistryString( HKEY_CLASSES_ROOT, aKey, aValueName, aValue );
}

wxString BundledHandlerPath()
{
    std::array<wchar_t, 32768> path;
    DWORD                      length = GetModuleFileNameW( nullptr, path.data(), static_cast<DWORD>( path.size() ) );

    if( length == 0 || length == path.size() )
        return {};

    wxFileName executable( path.data() );
    return wxFileName( executable.GetPath(), HANDLER_NAME ).GetFullPath();
}

wxString ExpandRegistryPath( const wxString& aPath )
{
    DWORD required = ExpandEnvironmentStringsW( aPath.wc_str(), nullptr, 0 );

    if( required == 0 )
        return aPath;

    std::vector<wchar_t> expanded( required );

    if( ExpandEnvironmentStringsW( aPath.wc_str(), expanded.data(), required ) != required )
        return aPath;

    return expanded.data();
}

wxString FileVersion( const wxString& aPath )
{
    DWORD ignored = 0;
    DWORD size = GetFileVersionInfoSizeW( aPath.wc_str(), &ignored );

    if( !size )
        return {};

    std::vector<BYTE> data( size );
    VS_FIXEDFILEINFO* info = nullptr;
    UINT              infoSize = 0;

    if( !GetFileVersionInfoW( aPath.wc_str(), 0, size, data.data() )
        || !VerQueryValueW( data.data(), L"\\", reinterpret_cast<void**>( &info ), &infoSize ) || !info )
        return {};

    return wxString::Format( wxS( "%u.%u.%u.%u" ), HIWORD( info->dwFileVersionMS ), LOWORD( info->dwFileVersionMS ),
                             HIWORD( info->dwFileVersionLS ), LOWORD( info->dwFileVersionLS ) );
}
} // namespace


MODEL_PREVIEW_STATUS GetPlatformModelPreviewStatus()
{
    WINDOWS_MODEL_PREVIEW_SNAPSHOT snapshot;
    snapshot.requirementId = REQUIREMENT_ID;
    wxString                       bundledPath = BundledHandlerPath();

    if( bundledPath.empty() || !wxFileName::FileExists( bundledPath ) )
    {
        return ClassifyWindowsModelPreviewStatus( snapshot );
    }

    snapshot.bundledPresent = true;
    snapshot.bundledVersion = FileVersion( bundledPath );
    snapshot.requirementId = wxString::Format( wxS( "%ls-%s" ), REQUIREMENT_ID, snapshot.bundledVersion );

    bool missingAssociation = false;
    bool foreignOwner = false;

    for( const wchar_t* extension : EXTENSIONS )
    {
        wxString owner;
        wxString mergedKey = wxString::Format( L"%ls\\shellex\\%ls", extension, PREVIEW_HANDLER_IID );

        switch( ReadClassString( mergedKey, nullptr, owner ) )
        {
        case READ_RESULT::MISSING:
            missingAssociation = true;
            break;

        case READ_RESULT::READ_ERROR:
            snapshot.associations = MODEL_PREVIEW_OBSERVATION::UNREADABLE;
            return ClassifyWindowsModelPreviewStatus( snapshot );

        case READ_RESULT::FOUND:
            foreignOwner = foreignOwner || owner.CmpNoCase( HANDLER_CLSID ) != 0;
            break;
        }
    }

    if( foreignOwner )
    {
        snapshot.foreignOwner = true;
        return ClassifyWindowsModelPreviewStatus( snapshot );
    }

    wxString    registeredPath;
    wxString    inprocKey = wxString::Format( L"Software\\Classes\\CLSID\\%ls\\InprocServer32", HANDLER_CLSID );
    wxString    mergedInprocKey = wxString::Format( L"CLSID\\%ls\\InprocServer32", HANDLER_CLSID );
    READ_RESULT pathResult = ReadClassString( mergedInprocKey, nullptr, registeredPath );

    if( pathResult == READ_RESULT::READ_ERROR )
    {
        snapshot.module = MODEL_PREVIEW_OBSERVATION::UNREADABLE;
        return ClassifyWindowsModelPreviewStatus( snapshot );
    }

    if( pathResult == READ_RESULT::MISSING )
    {
        snapshot.module = MODEL_PREVIEW_OBSERVATION::MISSING;
        return ClassifyWindowsModelPreviewStatus( snapshot );
    }

    registeredPath = ExpandRegistryPath( registeredPath );
    wxFileName registeredFile( registeredPath );
    wxFileName bundledFile( bundledPath );
    registeredFile.Normalize( wxPATH_NORM_ABSOLUTE | wxPATH_NORM_DOTS | wxPATH_NORM_CASE | wxPATH_NORM_LONG );
    bundledFile.Normalize( wxPATH_NORM_ABSOLUTE | wxPATH_NORM_DOTS | wxPATH_NORM_CASE | wxPATH_NORM_LONG );

    if( registeredFile.GetFullPath().CmpNoCase( bundledFile.GetFullPath() ) != 0 )
    {
        snapshot.module = MODEL_PREVIEW_OBSERVATION::PRESENT;
        snapshot.foreignOwner = true;
        return ClassifyWindowsModelPreviewStatus( snapshot );
    }

    snapshot.module = MODEL_PREVIEW_OBSERVATION::PRESENT;
    snapshot.sameModule = true;

    wxString registeredVersion;
    READ_RESULT registeredVersionResult = ReadString( inprocKey, L"KiCadHandlerVersion", registeredVersion );

    if( registeredVersionResult == READ_RESULT::READ_ERROR )
    {
        snapshot.module = MODEL_PREVIEW_OBSERVATION::UNREADABLE;
        return ClassifyWindowsModelPreviewStatus( snapshot );
    }

    snapshot.registeredVersion = registeredVersion;

    for( const wchar_t* extension : EXTENSIONS )
    {
        wxString    owner;
        wxString    key = wxString::Format( L"Software\\Classes\\%ls\\shellex\\%ls", extension, PREVIEW_HANDLER_IID );
        READ_RESULT ownerResult = ReadString( key, nullptr, owner );

        if( ownerResult == READ_RESULT::READ_ERROR )
        {
            snapshot.associations = MODEL_PREVIEW_OBSERVATION::UNREADABLE;
            return ClassifyWindowsModelPreviewStatus( snapshot );
        }

        if( ownerResult == READ_RESULT::FOUND && owner.CmpNoCase( HANDLER_CLSID ) != 0 )
        {
            snapshot.foreignOwner = true;
            return ClassifyWindowsModelPreviewStatus( snapshot );
        }
    }

    wxString    surrogate;
    wxString    appIdKey = wxString::Format( L"Software\\Classes\\AppID\\%ls", HANDLER_APPID );
    READ_RESULT appIdResult = ReadString( appIdKey, L"DllSurrogate", surrogate );

    if( appIdResult == READ_RESULT::READ_ERROR )
    {
        snapshot.surrogate = MODEL_PREVIEW_OBSERVATION::UNREADABLE;
        return ClassifyWindowsModelPreviewStatus( snapshot );
    }

    wxString    clsidAppId;
    wxString    threadingModel;
    wxString    clsidKey = wxString::Format( L"Software\\Classes\\CLSID\\%ls", HANDLER_CLSID );
    READ_RESULT clsidAppIdResult = ReadString( clsidKey, L"AppID", clsidAppId );
    READ_RESULT threadingResult = ReadString( inprocKey, L"ThreadingModel", threadingModel );

    if( clsidAppIdResult == READ_RESULT::READ_ERROR || threadingResult == READ_RESULT::READ_ERROR )
    {
        snapshot.classMetadata = MODEL_PREVIEW_OBSERVATION::UNREADABLE;
        return ClassifyWindowsModelPreviewStatus( snapshot );
    }

    wxString    previewHandlerName;
    READ_RESULT previewListResult = ReadString( wxS( "Software\\Microsoft\\Windows\\CurrentVersion\\PreviewHandlers" ),
                                                HANDLER_CLSID, previewHandlerName );

    if( previewListResult == READ_RESULT::READ_ERROR )
    {
        snapshot.previewList = MODEL_PREVIEW_OBSERVATION::UNREADABLE;
        return ClassifyWindowsModelPreviewStatus( snapshot );
    }

    snapshot.associations =
            missingAssociation ? MODEL_PREVIEW_OBSERVATION::MISSING : MODEL_PREVIEW_OBSERVATION::PRESENT;
    snapshot.surrogate = appIdResult == READ_RESULT::FOUND && !surrogate.empty() ? MODEL_PREVIEW_OBSERVATION::PRESENT
                                                                                 : MODEL_PREVIEW_OBSERVATION::MALFORMED;
    snapshot.classMetadata = clsidAppIdResult == READ_RESULT::FOUND && threadingResult == READ_RESULT::FOUND
                                             && clsidAppId.CmpNoCase( HANDLER_APPID ) == 0
                                             && threadingModel.CmpNoCase( wxS( "Apartment" ) ) == 0
                                     ? MODEL_PREVIEW_OBSERVATION::PRESENT
                                     : MODEL_PREVIEW_OBSERVATION::MALFORMED;
    snapshot.previewList = previewListResult == READ_RESULT::FOUND && !previewHandlerName.empty()
                                   ? MODEL_PREVIEW_OBSERVATION::PRESENT
                                   : MODEL_PREVIEW_OBSERVATION::MALFORMED;
    return ClassifyWindowsModelPreviewStatus( snapshot );
}


bool RepairPlatformModelPreviewRegistration( wxString& aError )
{
    MODEL_PREVIEW_STATUS status = GetPlatformModelPreviewStatus();

    if( !status.canRepairRegistration )
    {
        aError = status.detail;
        return false;
    }

    wxString handlerPath = BundledHandlerPath();
    HMODULE  module = LoadLibraryExW( handlerPath.wc_str(), nullptr,
                                      LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS );

    if( !module )
    {
        aError = wxString::Format( wxS( "Unable to load the Windows preview handler (error %lu)." ), GetLastError() );
        return false;
    }

    using REGISTER_SERVER = HRESULT( STDAPICALLTYPE* )();
    REGISTER_SERVER registerServer = reinterpret_cast<REGISTER_SERVER>( GetProcAddress( module, "DllRegisterServer" ) );
    HRESULT         result = registerServer ? registerServer() : HRESULT_FROM_WIN32( ERROR_PROC_NOT_FOUND );
    FreeLibrary( module );

    if( FAILED( result ) )
    {
        aError = wxString::Format( wxS( "Windows preview registration failed (0x%08lx)." ),
                                   static_cast<unsigned long>( result ) );
        return false;
    }

    MODEL_PREVIEW_STATUS repaired = GetPlatformModelPreviewStatus();

    if( repaired.component != MODEL_PREVIEW_COMPONENT_STATE::AVAILABLE
        || repaired.enablement != MODEL_PREVIEW_ENABLEMENT_STATE::ENABLED )
    {
        aError = repaired.detail;
        return false;
    }

    aError.clear();
    return true;
}
