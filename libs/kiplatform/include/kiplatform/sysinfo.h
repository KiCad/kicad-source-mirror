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
* along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef SYSINFO_H
#define SYSINFO_H

#include <cstdint>
#include <string>
#include <vector>

namespace KIPLATFORM
{
struct GPU_INFO
{
public:
    std::string Name;
    long long   MemorySize;
    std::string DriverVersion;
    std::string Manufacturer;
};

struct CPU_INFO
{
public:
    std::string Name;
    std::string Manufacturer;
    uint32_t    NumberCores;
    uint32_t    NumberLogical;
};

struct MEMORY_INFO
{
public:
    uint32_t Usage;
    uint64_t TotalPhysical;
    uint64_t FreePhysical;
    uint64_t TotalPaging;
    uint64_t FreePaging;
    uint64_t TotalVirtual;
    uint64_t FreeVirtual;
};

class SYSINFO_BASE
{
public:
    /**
     * Retrieve GPU info for the system
     *
     * @param aGPUInfos is an vector for GPU_INFO objects to be stored in
     *
     * @return true if info was fetchable, false if failure to retrieve info
     */
    virtual bool GetGPUInfo( std::vector<GPU_INFO>& aGpuInfos ) = 0;

    /**
     * Retrieve CPU info for the system
     *
     * @param aCPUInfos is an vector for CPU_INFO objects to be stored in
     *
     * @return true if info was fetchable, false if failure to retrieve info
     */
    virtual bool GetCPUInfo( std::vector<CPU_INFO>& aCpuInfos ) = 0;

    /**
     * Retrieve memory info for the system
     *
     * @param aMemoryInfo is an object to be populate with the retrieved system memory info
     *
     * @return true if info was fetchable, false if failure to retrieve info
     */
    virtual bool GetMemoryInfo( MEMORY_INFO& aMemoryInfo )      = 0;
};
} // namespace KIPLATFORM

#if defined( _WIN32 )
#include <kiplatform/sysinfo_windows.h>
#elif defined( __unix__ ) || defined( __unix )
#include <kiplatform/sysinfo_gtk.h>
#elif defined( __APPLE__ )
#include <kiplatform/sysinfo_osx.h>
#else

#endif
#endif // SYSINFO_H