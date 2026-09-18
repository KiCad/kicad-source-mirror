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


#include <kicad_3d_preview_class_factory.h>
#include <kicad_3d_preview_register.h>
#include <objbase.h>
#include <shlwapi.h>
#include <new>

#define SZ_CLSID_KiCad3DPreviewHandler L"{072BD71D-CB70-4CC8-8F08-D0A763A17C2E}"
#define SZ_APPID_KiCad3DPreviewHandler L"{6d2b5079-2f0b-48dd-ab7f-97cec514d30b}"
#define SZ_KiCad3DPreviewHandler L"KICAD 3D Preview Handler"

// Keep in step with the extensions S3D::ImportModel sniffs and with EXTENSIONS in
// model_preview_manager_windows.cpp, which verifies the registration this list creates.
PCWSTR fileTypes[] = { L".stp",  L".stpz", L".step", L".stepz", L".igs",
                       L".iges", L".wrl",  L".wrz",  L".x3d" };
size_t fileTypeCount = ARRAYSIZE( fileTypes );

const CLSID CLSID_KiCad3DPreviewHandler = { 0x72bd71d, 0xcb70, 0x4cc8, 0x8f, 0x8, 0xd0, 0xa7, 0x63, 0xa1, 0x7c, 0x2e };
const CLSID APPID_KiCad3DPreviewHandler = {
    0x6d2b5079, 0x2f0b, 0x48dd, 0xab, 0x7f, 0x97, 0xce, 0xc5, 0x14, 0xd3, 0x0b
};

long g_cRefModule = 0;

HINSTANCE          g_hInst = NULL;

STDAPI_( BOOL ) DllMain( HINSTANCE hInstance, DWORD dwReason, void* )
{
    if( dwReason == DLL_PROCESS_ATTACH )
    {
        g_hInst = hInstance;
        DisableThreadLibraryCalls( hInstance );

    }

    return TRUE;
}

STDAPI DllCanUnloadNow()
{
    return ( InterlockedCompareExchange( &g_cRefModule, 0, 0 ) == 0 ) ? S_OK : S_FALSE;
}

STDAPI DllGetClassObject( REFCLSID clsid, REFIID riid, void** ppv )
{
    if( !ppv )
        return E_POINTER;

    *ppv = nullptr;

    HRESULT hr = CLASS_E_CLASSNOTAVAILABLE;

    if( IsEqualCLSID( CLSID_KiCad3DPreviewHandler, clsid ) )
    {
        hr = E_OUTOFMEMORY;

        KICAD_3D_PREVIEW_CLASS_FACTORY* pKiCad3dPreviewClassFactory =
                new ( std::nothrow ) KICAD_3D_PREVIEW_CLASS_FACTORY();
        if( pKiCad3dPreviewClassFactory )
        {
            hr = pKiCad3dPreviewClassFactory->QueryInterface( riid, ppv );
            pKiCad3dPreviewClassFactory->Release();
        }
    }

    return hr;
}

STDAPI DllRegisterServer()
{
    HRESULT hr = S_OK;

    wchar_t szModule[MAX_PATH];
    DWORD   moduleLength = GetModuleFileName( g_hInst, szModule, ARRAYSIZE( szModule ) );
    if( moduleLength == 0 || moduleLength == ARRAYSIZE( szModule ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        return hr;
    }

    hr = RegisterInprocServer( szModule, CLSID_KiCad3DPreviewHandler, SZ_KiCad3DPreviewHandler, L"Apartment",
                               APPID_KiCad3DPreviewHandler );
    if( SUCCEEDED( hr ) )
    {
        hr = RegisterShellExtPreviewHandler( fileTypes, fileTypeCount, CLSID_KiCad3DPreviewHandler,
                                             SZ_KiCad3DPreviewHandler );

        // Registration validates every existing provider before writing.  Keep
        // an existing class registration if an extension is owned elsewhere.
    }

    return hr;
}

STDAPI DllUnregisterServer()
{
    HRESULT hr = S_OK;

    wchar_t szModule[MAX_PATH];
    DWORD   moduleLength = GetModuleFileName( g_hInst, szModule, ARRAYSIZE( szModule ) );
    if( moduleLength == 0 || moduleLength == ARRAYSIZE( szModule ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        return hr;
    }

    // Keep another installation's extension bindings intact.
    hr = UnregisterInprocServer( szModule, CLSID_KiCad3DPreviewHandler, APPID_KiCad3DPreviewHandler );

    for( size_t i = 0; i < fileTypeCount && SUCCEEDED( hr ); ++i )
    {
        PCWSTR pszFileType = fileTypes[i];
        hr = UnregisterShellExtPreviewHandler( pszFileType, CLSID_KiCad3DPreviewHandler );
    }

    return hr;
}
