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

#pragma once

#include <mutex>

class FOOTPRINT_LIBRARY_ADAPTER;
class PROJECT;
class S3D_CACHE;
class FILENAME_RESOLVER;

class PROJECT_PCB
{
public:
    static FOOTPRINT_LIBRARY_ADAPTER* FootprintLibAdapter( PROJECT* aProject );

    /**
     * Return a pointer to an instance of the 3D cache manager.
     *
     * An instance is created and initialized if appropriate.
     *
     * @return a pointer to an instance of the 3D cache manager or NULL on failure.
     */
    static S3D_CACHE* Get3DCacheManager( PROJECT* aProject, bool updateProjDir = false );

    static void Cleanup3DCache( PROJECT* aProject );

    /// Accessor for 3D path resolver
    static FILENAME_RESOLVER* Get3DFilenameResolver( PROJECT* aProject );

private:
    PROJECT_PCB() {}

    /// Used to synchronise access to FootprintLibAdapter
    static std::mutex s_libAdapterMutex;
};
