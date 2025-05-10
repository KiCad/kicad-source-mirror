/*
* This program source code file is part of KiCad, a free EDA CAD application.
*
* Copyright The KiCad Developers, see AUTHORS.txt for contributors.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, you may find one here:
* http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
* or you may search the http://www.gnu.org website for the version 2 license,
* or you may write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#include <wbemidl.h>
#include <cstdio>
#define _WIN32_DCOM
#include <comdef.h>


#include <stdexcept>
#include <string>
#include <vector>

#include <kiplatform/sysinfo.h>

#include <algorithm>
#include <sstream>
#include <vector>

#include <wx/string.h>
#include <wx/msw/registry.h>

#include "d3d9.h"
#include "dxgi.h"


namespace KIPLATFORM
{
namespace
{
    /*
     * Helper functions from https://stackoverflow.com/questions/6284524/bstr-to-stdstring-stdwstring-and-vice-versa
     * Licensed CC BY-SA 3.0
     */
    std::string ConvertWCSToMBS( const wchar_t* pstr, long wslen )
    {
        int len = ::WideCharToMultiByte( CP_ACP, 0, pstr, wslen, NULL, 0, NULL, NULL );

        std::string dblstr( len, '\0' );
        len = ::WideCharToMultiByte( CP_ACP, 0 /* no flags */, pstr,
                                     wslen /* not necessary NULL-terminated */, &dblstr[0], len, NULL,
                                     NULL /* no default char */ );

        return dblstr;
    }


    std::string ConvertBSTRToMBS( BSTR bstr )
    {
        int wslen = ::SysStringLen( bstr );
        return ConvertWCSToMBS( (wchar_t*) bstr, wslen );
    }


    BSTR ConvertMBSToBSTR( const std::string& str )
    {
        int wsLen = ::MultiByteToWideChar( CP_ACP, 0 /* no flags */, str.data(), str.length(), NULL,
                                           0 );

        BSTR wsData = ::SysAllocStringLen( NULL, wsLen );
        if( ::MultiByteToWideChar( CP_ACP, 0 /* no flags */,
                                   str.data(),
                                   str.length(),
                                   wsData,
                                   wsLen ) == 0 )
        {
            // conversion failure
            ::SysFreeString( wsData );
            wsData = NULL;
        }

        return wsData;
    }
}


SYSINFO::SYSINFO()
{
    openWmi();
}


SYSINFO::~SYSINFO()
{
    freeWmi();
}


HRESULT SYSINFO::openWmi()
{
    HRESULT hres;

    // We assume wxwidgets is handling calling of CoInitializeEx and CoInitializeSecurity
    hres = CoCreateInstance( CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator,
                             (LPVOID*) &m_pLoc );

    if( FAILED( hres ) )
    {
        throw std::runtime_error( "Failed to create IWbemLocator object." );
        return hres;
    }

    // Connect to the root\cimv2 namespace with
    // the current user and obtain pointer pSvc
    // to make IWbemServices calls.
    hres = m_pLoc->ConnectServer( ConvertMBSToBSTR( "ROOT\\CIMV2" ), // Object path of WMI namespace
                                  NULL,                      // User name. NULL = current user
                                  NULL,                      // User password. NULL = current
                                  0,                         // Locale. NULL indicates current
                                  0,                         // Security flags.
                                  0,                         // Authority (for example, Kerberos)
                                  0,                         // Context object
                                  &m_pSvc                    // pointer to IWbemServices proxy
    );

    if( FAILED( hres ) )
    {
        m_pLoc->Release();
        throw std::runtime_error( "Could not connect to root\\cimv2." );
        return hres;
    }

    hres = CoSetProxyBlanket( m_pSvc,                      // Indicates the proxy to set
                              RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
                              RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
                              NULL,                        // Server principal name
                              RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx
                              RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
                              NULL,                        // client identity
                              EOAC_NONE                    // proxy capabilities
    );

    if( FAILED( hres ) )
    {
        m_pSvc->Release();
        m_pLoc->Release();
        throw std::runtime_error( "Could not set proxy blanket." );
        return hres;
    }

    return hres;
}


HRESULT SYSINFO::freeWmi()
{
    HRESULT hres = 0;

    if( m_pSvc != nullptr )
    {
        hres = m_pSvc->Release();
    }

    if( m_pLoc != nullptr )
    {
        hres = m_pLoc->Release();
    }

    if( m_pEnumerator != nullptr )
    {
        hres = m_pEnumerator->Release();
    }

    return hres;
}


std::string SYSINFO::improveDriverVersion( const std::string& aManufacturer,
                                           const std::string& aVersion )
{
    if( aManufacturer.find( "NVIDIA" ) != std::string::npos
        || aManufacturer.find( "Nvidia" ) != std::string::npos )
    {
        std::istringstream       versionStream( aVersion );
        std::string              s;
        std::vector<std::string> versionBits;
        while( getline( versionStream, s, '.' ) )
        {
            versionBits.push_back( s );
        }

        if( versionBits.size() == 4 )
        {
            unsigned int numChars2 = versionBits[2].length();
            unsigned int numChars3 = versionBits[2].length();
            if( numChars2 >= 1 && numChars3 >= 2 )
            {
                if( numChars3 < 4 )
                {
                    versionBits[3].insert( 0, ( 4 - versionBits[3].size() ), '0' );
                    numChars3 = 4;
                }
            }

            std::string major1 = versionBits[2].substr( numChars2 - 1, numChars2 - 1 );
            std::string major2 = versionBits[3].substr( 0, 1 );
            std::string major = major1 + major2;
            std::string minor = versionBits[3].substr( 2, numChars3 - 1 );


            return major + "." + minor;
        }
    }

    return aVersion;
}


bool SYSINFO::GetMemoryInfo( MEMORY_INFO& aMemoryInfo )
{
    MEMORYSTATUSEX statex;

    statex.dwLength = sizeof( statex );

    if( GlobalMemoryStatusEx( &statex ) != 0 )
    {
        aMemoryInfo.Usage         = statex.dwMemoryLoad;
        aMemoryInfo.TotalPhysical = statex.ullTotalPhys;
        aMemoryInfo.FreePhysical  = statex.ullAvailPhys;
        aMemoryInfo.TotalPaging   = statex.ullTotalPageFile;
        aMemoryInfo.FreePaging    = statex.ullAvailPageFile;
        aMemoryInfo.TotalVirtual  = statex.ullTotalVirtual;
        aMemoryInfo.FreeVirtual   = statex.ullAvailVirtual;

        return true;
    }

    return false;
}


bool SYSINFO::GetCPUInfo( std::vector<CPU_INFO>& aCpuInfos )
{
    HRESULT hres;
    DWORD   returned = 0;

    IWbemClassObject* processors[2];
    hres = m_pSvc->ExecQuery(
            ConvertMBSToBSTR( "WQL" ), ConvertMBSToBSTR( "SELECT * FROM Win32_Processor" ),
            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, 0, &m_pEnumerator );

    if( SUCCEEDED( hres ) )
    {
        hres = m_pEnumerator->Next( WBEM_INFINITE, 2, processors, &returned );
        if( FAILED( hres ) || returned == 0 )
        {
            return false;
        }
    }

    VARIANT var;
    VariantInit( &var );

    std::string chRetValue = "";
    for( UINT ctrlIndex = 0; ctrlIndex < returned; ctrlIndex++ )
    {
        IWbemClassObject* processor = processors[ctrlIndex];

        CPU_INFO cpuInfo;
        hres = processor->Get( ConvertMBSToBSTR( "Name" ), 0, &var, NULL, NULL );
        if( SUCCEEDED( hres ) )
        {
            variantToString( &var, chRetValue );
            cpuInfo.Name = chRetValue;

            VariantClear( &var );
            processor->Get( ConvertMBSToBSTR( "Manufacturer" ), 0, &var, NULL, NULL );
            variantToString( &var, chRetValue );
            cpuInfo.Manufacturer = chRetValue;

            VariantClear( &var );
            processor->Get( ConvertMBSToBSTR( "NumberOfCores" ), 0, &var, NULL, NULL );
            variantToString( &var, chRetValue );
            long long cores = atoll( chRetValue.c_str() );
            cpuInfo.NumberCores = cores;

            VariantClear( &var );
            processor->Get( ConvertMBSToBSTR( "NumberOfLogicalProcessors" ), 0, &var, NULL, NULL );
            variantToString( &var, chRetValue );
            long long logical = atoll( chRetValue.c_str() );
            cpuInfo.NumberLogical = logical;


            aCpuInfos.push_back( cpuInfo );
        }

        VariantClear( &var );
    }


    return true;
}


bool SYSINFO::getVersionFromDXRegistry( int64_t aAdapterLuid, std::string& aDriverVersion )
{
    wxString baseKeyName = "SOFTWARE\\Microsoft\\DirectX";
    wxRegKey key( wxRegKey::HKLM, baseKeyName );
    HKEY    tmpKey;

    if( key.HasSubkeys() )
    {
        wxString adapterGuid;
        long     index = 0;
        for( bool cont = key.GetFirstKey( adapterGuid, index ); cont;
                cont   = key.GetNextKey( adapterGuid, index ) )
        {
            wxString subKeyName = baseKeyName + "\\" + adapterGuid;

            LSTATUS status;
            status = ::RegOpenKeyEx(
                    (HKEY) HKEY_LOCAL_MACHINE, subKeyName.t_str(), 0, KEY_READ, &tmpKey );

            ULONGLONG adapterLuid;
            DWORD dwType, dwSize = sizeof(ULONGLONG);
            status = ::RegQueryValueEx((HKEY)tmpKey, L"AdapterLuid",
                0,
                &dwType, (LPBYTE)&adapterLuid, &dwSize);

            if (status != ERROR_SUCCESS)
            {
                ::RegCloseKey(tmpKey);
                continue;
            }

            //access error, target ended up weirdly the wrong type
            if ( dwType != REG_QWORD_LITTLE_ENDIAN && dwType != REG_QWORD ) {
                ::RegCloseKey(tmpKey);
                continue;
            }

            if (adapterLuid != aAdapterLuid)
            {
                ::RegCloseKey(tmpKey);
                continue;
            }

            ULONGLONG driverVersion;
            status = ::RegQueryValueEx((HKEY)tmpKey, L"DriverVersion",
                0,
                &dwType, (LPBYTE)&driverVersion, &dwSize);

            if (status != ERROR_SUCCESS)
            {
                ::RegCloseKey(tmpKey);
                continue;
            }

            //access error, target ended up weirdly the wrong type
            if ( dwType != REG_QWORD_LITTLE_ENDIAN && dwType != REG_QWORD ) {
                ::RegCloseKey(tmpKey);
                continue;
            }

            std::stringstream fmt;
            fmt << ( ( driverVersion >> 48 ) & 0xFFFF ) << "."
                << ( ( driverVersion >> 32 ) & 0xFFFF ) << "."
                << std::to_string( ( driverVersion >> 16 ) & 0xFFFF ) << "."
                << std::to_string( driverVersion & 0xFFFF );
            aDriverVersion = fmt.str();

            ::RegCloseKey(tmpKey);
            return true;
        }
    }

    return false;
}


bool SYSINFO::gpuFromDirectX( std::vector<GPU_INFO>& aGpuInfos )
{
    typedef HRESULT( WINAPI * LPCREATEDXGIFACTORY )( REFIID, void** );
    LPCREATEDXGIFACTORY pCreateDXGIFactory = NULL;
    HMODULE             hDXGI              = NULL;

    hDXGI = LoadLibrary( L"dxgi.dll" );
    //
    if( NULL == hDXGI )
    {
        return false;
    }

    IID iid_IDXGIFactory;

    //to avoid having to link dxgi.dll, we also hardcode the guid of the interface
    IIDFromString( L"{7b7166ec-21c7-44ae-b21a-c9ae321ae369}", &iid_IDXGIFactory );

    IDXGIAdapter* pAdapter = NULL;
    pCreateDXGIFactory     = (LPCREATEDXGIFACTORY) GetProcAddress( hDXGI, "CreateDXGIFactory" );
    if( pCreateDXGIFactory )
    {
        IDXGIFactory* pDXGIFactory;
        HRESULT       hr = pCreateDXGIFactory( iid_IDXGIFactory, (void**) &pDXGIFactory );
        if( SUCCEEDED( hr ) )
        {
            UINT adapter = 0;
            while( pDXGIFactory->EnumAdapters( adapter, &pAdapter ) != DXGI_ERROR_NOT_FOUND )
            {
                DXGI_ADAPTER_DESC desc;
                hr = pAdapter->GetDesc( &desc );
                if( SUCCEEDED( hr ) )
                {
                    // Microsoft Basic Render Driver that will always show up
                    if( desc.VendorId == 5140 && desc.DeviceId == 140 )
                    {
                        adapter++;
                        continue;
                    }
                    GPU_INFO gpuInfo;

                    char descriptionBuffer[260];
                    char defaultChar = ' ';
                    WideCharToMultiByte( CP_ACP, 0, desc.Description, -1, descriptionBuffer,
                            sizeof( descriptionBuffer ), &defaultChar, NULL );

                    gpuInfo.Name = descriptionBuffer;
                    // Names are generally formatted with Manu<space>Model
                    gpuInfo.Manufacturer = gpuInfo.Name.substr( 0, gpuInfo.Name.find( " " ) );
                    ;
                    gpuInfo.MemorySize = desc.DedicatedVideoMemory;

                    int64_t luid = ( ( (int64_t) desc.AdapterLuid.HighPart ) << 32 )
                                   | desc.AdapterLuid.LowPart;

                    if( getVersionFromDXRegistry( luid, gpuInfo.DriverVersion ) )
                    {
                        improveDriverVersion( gpuInfo.Manufacturer, gpuInfo.DriverVersion );
                    }

                    aGpuInfos.push_back( gpuInfo );
                }

                adapter++;
            }

            pDXGIFactory->Release();
        }
    }

    FreeLibrary( hDXGI );

    return true;
}


bool SYSINFO::GetGPUInfo( std::vector<GPU_INFO>& aGpuInfos )
{
    if (gpuFromDirectX(aGpuInfos))
    {
        return true;
    }

    return false;
}


void SYSINFO::variantToString( const LPVARIANT aVar, std::string& aReturnString ) const
{
    HRESULT hr;
    char    returnBuffer[256];

    switch( aVar->vt )
    {
    case VT_BSTR:
    {
        aReturnString = ConvertBSTRToMBS( aVar->bstrVal );
    }
    break;
    case VT_BOOL:
    {
        if( VARIANT_TRUE == aVar->boolVal )
            aReturnString = "true";
        else
            aReturnString = "false";
    }
    break;
    case VT_I4:
    {
        sprintf_s( returnBuffer, sizeof( returnBuffer ) - 1, "%u", aVar->uintVal );

        aReturnString = returnBuffer;
    }
    break;
    case VT_UI1:
    {
        sprintf_s( returnBuffer, sizeof( returnBuffer ) - 1, "%u", aVar->uintVal );
        aReturnString = returnBuffer;
    }
    break;
    case VT_UI4:
    {
        sprintf_s( returnBuffer, sizeof( returnBuffer ) - 1, "%u", aVar->uintVal );
        aReturnString = returnBuffer;
    }
    break;
    case VT_BSTR | VT_ARRAY:
    {
        VARIANT* raw;
        hr = SafeArrayAccessData( aVar->parray, (void**) &raw );

        const _bstr_t bstr( raw[0] );
        hr = SafeArrayUnaccessData( aVar->parray );

        aReturnString = ConvertBSTRToMBS( bstr );
    }
    break;
    case VT_I4 | VT_ARRAY:
    {
        BYTE HUGEP* pBuf;
        LONG        low, high;
        SafeArrayGetLBound( aVar->parray, 1, &low );
        SafeArrayGetUBound( aVar->parray, 1, &high );

        hr = SafeArrayAccessData( aVar->parray, (void HUGEP**) &pBuf );
        hr = SafeArrayUnaccessData( aVar->parray );
        std::string strTmp;
        high = std::min( high, (long) MAX_PATH * 2 - 1 );
        for( LONG i = low; i <= high; ++i )
        {
            sprintf_s( returnBuffer, sizeof( returnBuffer ) - 1, "%02X", pBuf[i] );
            aReturnString += returnBuffer;
        }
    }
    break;
    default: break;
    }
}

} // namespace KIPLATFORM
