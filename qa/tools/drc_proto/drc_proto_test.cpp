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

#include <string>

#include <common.h>
#include <core/profile.h>

#include <wx/cmdline.h>
#include <wx/init.h>

#include <properties/property_mgr.h>

#include <pgm_base.h>

#include "drc_proto.h"

int main( int argc, char** argv )
{
    wxInitialize( argc, argv );

    Pgm().InitPgm( true );

    PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
    propMgr.Rebuild();

    if( argc < 2 )
    {
        printf("usage: %s <project-file/board-file> [drc-rules-file]\n", argv[0] );
        Pgm().Destroy();
        wxUninitialize();
        return -1;
    }

    PROJECT_CONTEXT project = loadKicadProject( argv[1], argv[2] ? wxString( argv[2] ) : std::optional<wxString>() );

    // This causes some glib warnings on GTK3 (http://trac.wxwidgets.org/ticket/18274)
    // but without it, Valgrind notices a lot of leaks from WX

    runDRCProto( project );

    Pgm().Destroy();

    wxUninitialize();

    return 0;
}