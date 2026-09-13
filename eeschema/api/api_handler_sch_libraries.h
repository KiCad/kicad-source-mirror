/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Jon Evans <jon@craftyjon.com>
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KICAD_API_HANDLER_SCH_LIBRARIES_H
#define KICAD_API_HANDLER_SCH_LIBRARIES_H

#include <api/api_handler_libraries.h>


class API_HANDLER_SCH_LIBRARIES : public API_HANDLER_LIBRARIES
{
public:
    API_HANDLER_SCH_LIBRARIES() : API_HANDLER_LIBRARIES( LIBRARY_TABLE_TYPE::SYMBOL ) {}

protected:
    LIBRARY_MANAGER_ADAPTER* adapterForProject( PROJECT& aProject ) const override;

    std::vector<wxString> getItemNames( LIBRARY_MANAGER_ADAPTER& aAdapter,
                                        const wxString& aNickname ) const override;
};

#endif //KICAD_API_HANDLER_SCH_LIBRARIES_H
