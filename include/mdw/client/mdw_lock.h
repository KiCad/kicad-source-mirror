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

/**
 * @file mdw_lock.h
 * @brief Per-user exclusive lock state.
 *
 * A lock is independent of the lifecycle category, so an item can be EDITABLE and locked by a
 * teammate, or RESTRICTED and unlocked.
 */

#ifndef MDW_LOCK_H
#define MDW_LOCK_H

#include <mdw/client/mdw_types.h>

#include <wx/string.h>
#include <wx/datetime.h>

/// Last-known per-user lock state for an item
struct MDW_LOCK_INFO
{
    bool       is_locked = false;
    wxString   owner_id;
    wxString   owner_display_name;
    wxDateTime lock_timestamp;
    bool       locked_by_me = false;
    bool       advisory = false;        ///< The backend accepts another user's check-in while this is held
    bool       known = true;            ///< False before the backend's claims were read, so a free item may be held
};


#endif // MDW_LOCK_H
