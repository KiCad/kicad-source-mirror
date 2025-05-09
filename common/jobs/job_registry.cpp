/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Mark Roszko <mark.roszko@gmail.com>
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

#include <jobs/job_registry.h>

bool JOB_REGISTRY::Add( const wxString& aName, JOB_REGISTRY_ENTRY entry, bool aDeprecated )
{
    entry.deprecated = aDeprecated;

    REGISTRY_MAP_T& registry = getRegistry();

    if( registry.find( aName ) != registry.end() )
        return false;

    registry[aName] = entry;
    return true;
}


KIWAY::FACE_T JOB_REGISTRY::GetKifaceType( const wxString& aName )
{
    REGISTRY_MAP_T& registry = getRegistry();
    if( registry.find( aName ) == registry.end() )
    {
        return KIWAY::KIWAY_FACE_COUNT;
    }

    return registry[aName].kifaceType;
}


JOB_REGISTRY::REGISTRY_MAP_T& JOB_REGISTRY::getRegistry()
{
    static REGISTRY_MAP_T map;
    return map;
}