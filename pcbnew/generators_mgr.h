/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Alex Shvartzkop <dudesuchamazing@gmail.com>
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

#ifndef GENERATORS_MGR_H_
#define GENERATORS_MGR_H_

#include <cstdint>
#include <config.h>
#include <vector>
#include <map>
#include <memory>
#include <wx/string.h>

class PCB_GENERATOR;


///< Unique type identifier
using TYPE_ID = size_t;

/**
 * A factory which returns an instance of a #PCB_GENERATOR.
 */
class GENERATORS_MGR
{
public:
    struct ENTRY
    {
        wxString                              m_type;
        wxString                              m_displayName;
        std::function<PCB_GENERATOR*( void )> m_createFunc;
    };

    static GENERATORS_MGR& Instance();

    /**
     * Associate a type string to display name and create function.
     *
     * @param aTypeStr is the type identifier string.
     * @param aName is the display name.
     * @param aCreateFunc is the create function.
     */
    void Register( const wxString& aTypeStr, const wxString& aName,
                   std::function<PCB_GENERATOR*( void )> aCreateFunc );

    PCB_GENERATOR* CreateFromType( const wxString& aTypeStr );

    /**
     * Static helper to register a generator.
     * T must define static members GENERATOR_TYPE and DISPLAY_NAME
     */
    template <typename T>
    struct REGISTER
    {
        REGISTER()
        {
            GENERATORS_MGR::Instance().Register( T::GENERATOR_TYPE, T::DISPLAY_NAME,
                                                 []
                                                 {
                                                     return new T;
                                                 } );
        }
    };

private:
    std::map<wxString, ENTRY> m_registry;
};


#endif // GENERATORS_MGR_H_
