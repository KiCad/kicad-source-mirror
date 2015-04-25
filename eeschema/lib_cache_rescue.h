/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Chris Pavlina <pavlina.chris@gmail.com>
 * Copyright (C) 2015 KiCad Developers, see change_log.txt for contributors.
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

#ifndef _LIB_CACHE_RESCUE_H_
#define _LIB_CACHE_RESCUE_H_

/* This code handles the case where an old schematic has parts that have
 * changed in the system libraries, such that their pins no longer line up.
 * The function of note is a member of SCH_EDIT_FRAME, defined thus:
 *
 * bool SCH_EDIT_FRAME::RescueCacheConflicts( bool aSilentIfNone );
 *
 * When this is called, a list of component names referring to conflicting
 * symbols is compiled. If this list is empty, then the function displays
 * a notification and returns (if aSilentIfNone is true, the notification is
 * silenced).
 *
 * The user is then prompted to select which parts he would like to rescue.
 * Any remaining after he's through are rescued: they are renamed to avoid
 * further conflicts, and then they are copied into a new library. The
 * schematic components are updated to link to these new names, the library
 * is saved, and the library is added to the project at the top of the
 * search path.
 */

#include <vector>
#include <wx/string.h>

class LIB_PART;
class SCH_COMPONENT;

class RESCUE_CANDIDATE
{
public:
    wxString   requested_name;
    LIB_PART*  cache_candidate;
    LIB_PART*  lib_candidate;
};

class RESCUE_LOG
{
public:
    SCH_COMPONENT*  component;
    wxString        old_name;
    wxString        new_name;
};

#endif // _LIB_CACHE_RESCUE_H_
