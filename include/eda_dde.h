/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2014 KiCad Developers, see CHANGELOG.TXT for contributors.
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

/**
 * @file eda_dde.h
 * @brief DDE server & client.
 */

#ifndef EDA_DDE_H_
#define EDA_DDE_H_

#include <wx/socket.h>


// TCP/IP ports used by Pcbnew and Eeschema respectively.

///< Pcbnew listens on this port for commands from Eeschema
#define KICAD_PCB_PORT_SERVICE_NUMBER   4242

///< Eeschema listens on this port for commands from Pcbnew
#define KICAD_SCH_PORT_SERVICE_NUMBER   4243


#define MSG_TO_PCB                      KICAD_PCB_PORT_SERVICE_NUMBER
#define MSG_TO_SCH                      KICAD_SCH_PORT_SERVICE_NUMBER

wxSocketServer* CreateServer( wxWindow * window, int port, bool local = true );
bool SendCommand( int port, const char* cmdline );

#endif    // EDA_DDE_H_
