/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2026 KiCad Developers, see AUTHORS.txt for contributors.
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

// The git workspace backend. It never links kicommon, so it can never reach the platform vault

#include "git_backend.h"

#include <mdw_runtime/mdw_event_publisher.h>


int main( int argc, char** argv )
{
    MDW_GIT_LIBGIT2     libgit2;
    MDW_BACKEND_LAUNCH  launch = MDW_ParseLaunch( argc, argv );
    MDW_BACKEND_SERVER  server;
    MDW_EVENT_PUBLISHER publisher( launch.channel_token );
    MDW_GIT_BACKEND     backend( server, &publisher );

    backend.Register();

    return server.Run( launch, MDW_GIT_BACKEND::Descriptor(), &publisher );
}
