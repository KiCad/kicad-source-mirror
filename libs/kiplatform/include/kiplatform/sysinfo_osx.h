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

#ifndef SYSINFO_GTK_H
#define SYSINFO_GTK_H

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

    };
}

#endif // SYSINFO_GTK_H