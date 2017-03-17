/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 NBEE Embedded Systems SL, Miguel Angel Ajo <miguelangel@ajo.es>
 * Copyright (C) 2013-2017 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#ifndef __PCBNEW_SCRIPTING_HELPERS_H
#define __PCBNEW_SCRIPTING_HELPERS_H

#include <wxPcbStruct.h>
#include <io_mgr.h>
/* we could be including all these methods as static in a class, but
 * we want plain pcbnew.<method_name> access from python */

#ifndef SWIG
void    ScriptingSetPcbEditFrame( PCB_EDIT_FRAME* aPCBEdaFrame );

#endif

BOARD*  GetBoard();

BOARD*  LoadBoard( wxString& aFileName, IO_MGR::PCB_FILE_T aFormat );
BOARD*  LoadBoard( wxString& aFileName );

bool    SaveBoard( wxString& aFileName, BOARD* aBoard, IO_MGR::PCB_FILE_T aFormat );
bool    SaveBoard( wxString& aFileName, BOARD* aBoard );

void    Refresh();
void    WindowZoom( int xl, int yl, int width, int height );

#endif
