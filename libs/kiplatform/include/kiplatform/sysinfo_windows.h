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

#ifndef SYSINFO_WINDOWS_H
#define SYSINFO_WINDOWS_H

struct IWbemLocator;
struct IWbemServices;
struct IEnumWbemClassObject;
struct IWbemClassObject;

namespace KIPLATFORM
{
    class SYSINFO : SYSINFO_BASE
    {
    public:
        SYSINFO();
        ~SYSINFO();

        bool GetCPUInfo( std::vector<CPU_INFO>& aCpuInfos ) override;
        bool GetGPUInfo( std::vector<GPU_INFO>& aGpuInfos ) override;
        bool GetMemoryInfo( MEMORY_INFO& aRamInfo ) override;

    private:
        HRESULT openWmi();
        HRESULT freeWmi();

        /**
         * Fetches gpu info from directx and registry
         * WMI unforunately has a uint32 max value issue for reporting gpu memory, meanwhile directx does not have this issue
         *
         * We only lightly poke directx to get the info we need and don't actually use it
         */
        bool gpuFromDirectX( std::vector<GPU_INFO>& aGpuInfos );

        /**
         * Extracts the driver version for an dapter from the directx section of the registry
         *
         * @param aAdapterLuid is the unique adapter (gpu) id to look for in the registry
         * @param aDriverVersion is the string variable that will be updated
         *
         * @return True if successfully retrieved the driver version
         */
        bool getVersionFromDXRegistry( int64_t aAdapterLuid, std::string& aDriverVersion );

        /**
         * Attempts to convert the Windows driver version string to the vendor version string if possible
         *
         * @param aManufacturer is the name of the vendor
         * @param aVersion is the windows driver version string to convert
         *
         * @return Vendor version string
         */
        std::string improveDriverVersion( const std::string& aManufacturer, const std::string& aVersion );

        /**
         * Converts a win32 LPVARIANT to a string
         *
         * @param aVar is the LPVARIANT to be converted
         * @param aReturnString is the string to store the variant within
         *
         * @return LPVARIANT as string
         */
        void variantToString( const LPVARIANT aVar, std::string& aReturnString ) const;

        IWbemLocator*         m_pLoc = nullptr;
        IWbemServices*        m_pSvc = nullptr;
        IEnumWbemClassObject* m_pEnumerator = nullptr;
        IWbemClassObject*     m_pClassObject = nullptr;
    };
}

#endif // SYSINFO_WINDOWS_H