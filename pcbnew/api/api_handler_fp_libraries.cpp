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

#include <api/api_handler_fp_libraries.h>

#include <footprint_library_adapter.h>
#include <project_pcb.h>


LIBRARY_MANAGER_ADAPTER* API_HANDLER_FP_LIBRARIES::adapterForProject( PROJECT& aProject ) const
{
    return PROJECT_PCB::FootprintLibAdapter( &aProject );
}


std::vector<wxString> API_HANDLER_FP_LIBRARIES::getItemNames( LIBRARY_MANAGER_ADAPTER& aAdapter,
                                                              const wxString& aNickname ) const
{
    return static_cast<FOOTPRINT_LIBRARY_ADAPTER&>( aAdapter ).GetFootprintNames( aNickname, true );
}
