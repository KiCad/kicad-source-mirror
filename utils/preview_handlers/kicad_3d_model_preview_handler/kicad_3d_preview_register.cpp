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

#include <kicad_3d_preview_register.h>
#include <objbase.h>
#include <strsafe.h>
#include <vector>
#include <winver.h>

#include <string>

#pragma region Registry Helper Functions

HRESULT SetPerUserClassValue( PCWSTR aPszSubKey, PCWSTR aPszValueName, PCWSTR aPszData, DWORD aDwType = REG_SZ )
{
    HRESULT hr = S_OK;
    HKEY    hKey = NULL;

    hr = HRESULT_FROM_WIN32( RegCreateKeyExW( HKEY_CURRENT_USER, aPszSubKey, 0, NULL, REG_OPTION_NON_VOLATILE,
                                              KEY_WRITE, NULL, &hKey, NULL ) );

    if( SUCCEEDED( hr ) )
    {
        if( aPszData != NULL )
        {
            DWORD cbData = ( lstrlenW( aPszData ) + 1 ) * sizeof( *aPszData );
            hr = HRESULT_FROM_WIN32( RegSetValueExW( hKey, aPszValueName, 0, aDwType,
                                                     reinterpret_cast<const BYTE*>( aPszData ), cbData ) );
        }

        RegCloseKey( hKey );
    }

    return hr;
}

HRESULT GetPerUserClassValue( PCWSTR aPszSubKey, PCWSTR aPszValueName, PWSTR aPszData, DWORD aCbData )
{
    return HRESULT_FROM_WIN32(
            RegGetValueW( HKEY_CURRENT_USER, aPszSubKey, aPszValueName, RRF_RT_REG_SZ, nullptr, aPszData, &aCbData ) );
}

#pragma endregion

HRESULT RegisterInprocServer( PCWSTR aPszModule, const CLSID& aClsid, PCWSTR aPszFriendlyName, PCWSTR aPszThreadModel,
                              const GUID& aAppId )
{
    if( aPszModule == NULL || aPszThreadModel == NULL )
    {
        return E_INVALIDARG;
    }

    HRESULT hr;

    wchar_t szCLSID[MAX_PATH];
    StringFromGUID2( aClsid, szCLSID, ARRAYSIZE( szCLSID ) );

    wchar_t szAppID[MAX_PATH];
    StringFromGUID2( aAppId, szAppID, ARRAYSIZE( szAppID ) );

    wchar_t szSubkey[MAX_PATH];

    hr = StringCchPrintf( szSubkey, ARRAYSIZE( szSubkey ), L"Software\\Classes\\CLSID\\%s\\InprocServer32", szCLSID );

    if( SUCCEEDED( hr ) )
    {
        std::wstring mergedKey = std::wstring( L"CLSID\\" ) + szCLSID + L"\\InprocServer32";
        wchar_t      mergedModule[MAX_PATH];
        DWORD        mergedSize = sizeof( mergedModule );
        LONG         mergedResult = RegGetValueW( HKEY_CLASSES_ROOT, mergedKey.c_str(), nullptr, RRF_RT_REG_SZ, nullptr,
                                                  mergedModule, &mergedSize );

        if( mergedResult == ERROR_SUCCESS && _wcsicmp( mergedModule, aPszModule ) != 0 )
            return HRESULT_FROM_WIN32( ERROR_ALREADY_EXISTS );

        if( mergedResult != ERROR_SUCCESS && mergedResult != ERROR_FILE_NOT_FOUND )
            return HRESULT_FROM_WIN32( mergedResult );

        wchar_t registeredModule[MAX_PATH];
        HRESULT readResult =
                GetPerUserClassValue( szSubkey, nullptr, registeredModule, sizeof( registeredModule ) );

        if( SUCCEEDED( readResult ) && _wcsicmp( registeredModule, aPszModule ) != 0 )
            return HRESULT_FROM_WIN32( ERROR_ALREADY_EXISTS );

        if( FAILED( readResult ) && readResult != HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ) )
            return readResult;
    }

    hr = StringCchPrintf( szSubkey, ARRAYSIZE( szSubkey ), L"Software\\Classes\\CLSID\\%s", szCLSID );

    if( SUCCEEDED( hr ) )
    {
        hr = SetPerUserClassValue( szSubkey, NULL, aPszFriendlyName );

        if( SUCCEEDED( hr ) )
        {
            hr = SetPerUserClassValue( szSubkey, L"AppID", szAppID );
        }

        if( SUCCEEDED( hr ) )
        {
            hr = StringCchPrintf( szSubkey, ARRAYSIZE( szSubkey ), L"Software\\Classes\\CLSID\\%s\\InprocServer32",
                                  szCLSID );
            if( SUCCEEDED( hr ) )
            {
                hr = SetPerUserClassValue( szSubkey, NULL, aPszModule );

                if( SUCCEEDED( hr ) )
                {
                    hr = SetPerUserClassValue( szSubkey, L"ThreadingModel", aPszThreadModel );
                }

                if( SUCCEEDED( hr ) )
                {
                    DWORD versionHandle = 0;
                    DWORD versionSize = GetFileVersionInfoSizeW( aPszModule, &versionHandle );
                    std::vector<BYTE> versionData( versionSize );
                    VS_FIXEDFILEINFO* versionInfo = nullptr;
                    UINT              infoSize = 0;

                    if( versionSize && GetFileVersionInfoW( aPszModule, 0, versionSize, versionData.data() )
                        && VerQueryValueW( versionData.data(), L"\\", reinterpret_cast<void**>( &versionInfo ),
                                           &infoSize )
                        && versionInfo )
                    {
                        wchar_t version[64];
                        hr = StringCchPrintfW( version, ARRAYSIZE( version ), L"%u.%u.%u.%u",
                                               HIWORD( versionInfo->dwFileVersionMS ),
                                               LOWORD( versionInfo->dwFileVersionMS ),
                                               HIWORD( versionInfo->dwFileVersionLS ),
                                               LOWORD( versionInfo->dwFileVersionLS ) );

                        if( SUCCEEDED( hr ) )
                            hr = SetPerUserClassValue( szSubkey, L"KiCadHandlerVersion", version );
                    }
                    else
                    {
                        hr = HRESULT_FROM_WIN32( ERROR_RESOURCE_DATA_NOT_FOUND );
                    }
                }
            }
        }
    }

    // Create a new prevhost AppID so that this always runs in its own
    // isolated process.
    if( SUCCEEDED( hr ) )
    {
        hr = StringCchPrintf( szSubkey, ARRAYSIZE( szSubkey ), L"Software\\Classes\\AppID\\%s", szAppID );

        if( SUCCEEDED( hr ) )
        {
            hr = SetPerUserClassValue( szSubkey, L"DllSurrogate", L"%SystemRoot%\\system32\\prevhost.exe",
                                             REG_EXPAND_SZ );
        }
    }

    return hr;
}

HRESULT UnregisterInprocServer( PCWSTR aPszModule, const CLSID& aClsid, const GUID& aAppId )
{
    if( !aPszModule )
        return E_INVALIDARG;

    HRESULT hr = S_OK;

    wchar_t szCLSID[MAX_PATH];
    StringFromGUID2( aClsid, szCLSID, ARRAYSIZE( szCLSID ) );

    wchar_t szAppID[MAX_PATH];
    StringFromGUID2( aAppId, szAppID, ARRAYSIZE( szAppID ) );

    wchar_t szSubkey[MAX_PATH];

    hr = StringCchPrintf( szSubkey, ARRAYSIZE( szSubkey ), L"Software\\Classes\\CLSID\\%s\\InprocServer32", szCLSID );

    if( SUCCEEDED( hr ) )
    {
        wchar_t registeredModule[MAX_PATH];
        hr = GetPerUserClassValue( szSubkey, nullptr, registeredModule, sizeof( registeredModule ) );

        if( hr == HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ) )
            return S_OK;

        if( SUCCEEDED( hr ) && _wcsicmp( registeredModule, aPszModule ) != 0 )
            return HRESULT_FROM_WIN32( ERROR_NOT_OWNER );
    }

    if( SUCCEEDED( hr ) )
    {
        hr = StringCchPrintf( szSubkey, ARRAYSIZE( szSubkey ), L"Software\\Classes\\CLSID\\%s", szCLSID );

        if( SUCCEEDED( hr ) )
        {
            hr = HRESULT_FROM_WIN32( RegDeleteTreeW( HKEY_CURRENT_USER, szSubkey ) );

            if( hr == HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ) )
                hr = S_OK;
        }
    }

    if( SUCCEEDED( hr ) )
    {
        hr = StringCchPrintf( szSubkey, ARRAYSIZE( szSubkey ), L"Software\\Classes\\AppID\\%s", szAppID );

        if( SUCCEEDED( hr ) )
        {
            hr = HRESULT_FROM_WIN32( RegDeleteTreeW( HKEY_CURRENT_USER, szSubkey ) );

            if( hr == HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ) )
                hr = S_OK;
        }
    }

    return hr;
}

HRESULT RegisterShellExtPreviewHandler( PCWSTR aPszFileTypes[], size_t aPszFileTypesCount, const CLSID& aClsid,
                                        PCWSTR aPszDescription )
{
    if( aPszFileTypes == NULL || aPszFileTypesCount == 0 )
    {
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;

    wchar_t szCLSID[MAX_PATH];
    StringFromGUID2( aClsid, szCLSID, ARRAYSIZE( szCLSID ) );

    for( size_t i = 0; i < aPszFileTypesCount; ++i )
    {
        std::wstring mergedKey = std::wstring( aPszFileTypes[i] )
                                 + L"\\shellex\\{8895b1c6-b41f-4c1c-a562-0d564250836f}";
        wchar_t mergedClsid[MAX_PATH];
        DWORD   mergedSize = sizeof( mergedClsid );
        LONG mergedResult = RegGetValueW( HKEY_CLASSES_ROOT, mergedKey.c_str(), nullptr, RRF_RT_REG_SZ, nullptr,
                                          mergedClsid, &mergedSize );

        if( mergedResult == ERROR_SUCCESS && _wcsicmp( mergedClsid, szCLSID ) != 0 )
            return HRESULT_FROM_WIN32( ERROR_ALREADY_EXISTS );

        if( mergedResult != ERROR_SUCCESS && mergedResult != ERROR_FILE_NOT_FOUND )
            return HRESULT_FROM_WIN32( mergedResult );
    }

    for( size_t i = 0; i < aPszFileTypesCount; ++i )
    {
        PCWSTR  pszFileType = aPszFileTypes[i];
        wchar_t szSubkey[MAX_PATH];

        hr = StringCchPrintf( szSubkey, ARRAYSIZE( szSubkey ),
                              L"Software\\Classes\\%s\\shellex\\{8895b1c6-b41f-4c1c-a562-0d564250836f}", pszFileType );

        if( SUCCEEDED( hr ) )
        {
            wchar_t currentClsid[MAX_PATH];
            HRESULT readResult = GetPerUserClassValue( szSubkey, nullptr, currentClsid, sizeof( currentClsid ) );

            if( SUCCEEDED( readResult ) && _wcsicmp( currentClsid, szCLSID ) != 0 )
                return HRESULT_FROM_WIN32( ERROR_ALREADY_EXISTS );

            if( FAILED( readResult ) && readResult != HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ) )
                return readResult;

            hr = SetPerUserClassValue( szSubkey, nullptr, szCLSID );
        }

        if( FAILED( hr ) )
            return hr;
    }

    // Windows uses this list to enumerate registered preview handlers.
    if( SUCCEEDED( hr ) )
    {
        HKEY hKey = NULL;

        hr = HRESULT_FROM_WIN32( RegCreateKeyExW( HKEY_CURRENT_USER,
                                                  L"Software\\Microsoft\\Windows\\CurrentVersion\\PreviewHandlers", 0,
                                                  NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL ) );

        if( SUCCEEDED( hr ) )
        {
            DWORD cbData = ( aPszDescription == NULL )
                                   ? 0
                                   : ( lstrlenW( aPszDescription ) + 1 ) * sizeof( *aPszDescription );
            hr = HRESULT_FROM_WIN32( RegSetValueExW( hKey, szCLSID, 0, REG_SZ,
                                                     reinterpret_cast<const BYTE*>( aPszDescription ), cbData ) );

            RegCloseKey( hKey );
        }
    }

    return hr;
}


HRESULT UnregisterShellExtPreviewHandler( PCWSTR aPszFileType, const CLSID& aClsid )
{
    if( aPszFileType == NULL )
    {
        return E_INVALIDARG;
    }

    HRESULT hr;

    wchar_t szCLSID[MAX_PATH];
    StringFromGUID2( aClsid, szCLSID, ARRAYSIZE( szCLSID ) );

    wchar_t szSubkey[MAX_PATH];

    hr = StringCchPrintf( szSubkey, ARRAYSIZE( szSubkey ),
                          L"Software\\Classes\\%s\\shellex\\{8895b1c6-b41f-4c1c-a562-0d564250836f}", aPszFileType );

    if( SUCCEEDED( hr ) )
    {
        wchar_t currentClsid[MAX_PATH];
        hr = GetPerUserClassValue( szSubkey, nullptr, currentClsid, sizeof( currentClsid ) );

        if( hr == HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ) )
            hr = S_OK;
        else if( SUCCEEDED( hr ) && _wcsicmp( currentClsid, szCLSID ) != 0 )
            hr = S_OK;
        else if( SUCCEEDED( hr ) )
            hr = HRESULT_FROM_WIN32( RegDeleteKeyW( HKEY_CURRENT_USER, szSubkey ) );
    }

    if( SUCCEEDED( hr ) )
    {
        HKEY hKey = NULL;

        hr = HRESULT_FROM_WIN32( RegOpenKeyExW( HKEY_CURRENT_USER,
                                                L"Software\\Microsoft\\Windows\\CurrentVersion\\PreviewHandlers", 0,
                                                KEY_WRITE, &hKey ) );
        if( SUCCEEDED( hr ) )
        {
            hr = HRESULT_FROM_WIN32( RegDeleteValue( hKey, szCLSID ) );
            if( hr == HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ) )
            {
                hr = S_OK;
            }

            RegCloseKey( hKey );
        }
        else if( hr == HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ) )
        {
            hr = S_OK;
        }
    }

    return hr;
}
