/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include <import_net_names.h>
#include <board.h>
#include <board_design_settings.h>
#include <project/net_settings.h>
#include <reporter.h>


bool ApplyImportedNetNameMap( BOARD& aBoard, const std::map<wxString, wxString>& aNames,
                              REPORTER& aReporter )
{
    if( !aBoard.RenameNets( aNames, aReporter ) )
        return false;

    if( std::shared_ptr<NET_SETTINGS> settings = aBoard.GetDesignSettings().m_NetSettings )
        settings->RenameNets( aNames );

    return true;
}
