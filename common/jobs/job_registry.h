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

#pragma once

#include <jobs/job.h>
#include <kiway.h>

struct KICOMMON_API JOB_REGISTRY_ENTRY
{
public:
    KIWAY::FACE_T         kifaceType;
    std::function<JOB*()> createFunc;
    wxString              title;
    bool                  deprecated = false;
};

class KICOMMON_API JOB_REGISTRY
{
public:
    typedef std::unordered_map<wxString, JOB_REGISTRY_ENTRY> REGISTRY_MAP_T;

    static bool Add( const wxString& aName, JOB_REGISTRY_ENTRY entry, bool aDeprecated = false );

    static KIWAY::FACE_T GetKifaceType( const wxString& aName );

    template <typename T>
    static T* CreateInstance( const wxString& aName )
    {
        REGISTRY_MAP_T& registry = getRegistry();

        if( registry.find( aName ) == registry.end() )
            return nullptr;

        return registry[aName].createFunc();
    }

    static const REGISTRY_MAP_T& GetRegistry() {
        return getRegistry();
    }

private:
    // Use Meyer's singleton to prevent SIOF
    static REGISTRY_MAP_T& getRegistry();
};

#define REGISTER_JOB( job_name, title, face, T ) bool job_name##_entry = JOB_REGISTRY::Add( #job_name, \
                                                            { face, []()                               \
                                                            {                                          \
                                                                return new T();                        \
                                                            },                                         \
                                                            title } )
// newline required to appease warning for REGISTER_JOB macro

#define REGISTER_DEPRECATED_JOB( job_name, title, face, T ) bool job_name##_entry = JOB_REGISTRY::Add( #job_name, \
                                                            { face, []()                                          \
                                                            {                                                     \
                                                                return new T();                                   \
                                                            },                                                    \
                                                            title }, true )
// newline required to appease warning for REGISTER_DEPRECATED_JOB macro