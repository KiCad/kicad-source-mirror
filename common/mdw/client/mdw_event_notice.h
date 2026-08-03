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
 * @file mdw_event_notice.h
 * Which backend events belong in the notification bell. Pure, so it needs no frame.
 */

#ifndef MDW_EVENT_NOTICE_H
#define MDW_EVENT_NOTICE_H

#include <kicommon.h>
#include <mdw/client/mdw_events.h>

#include <optional>
#include <wx/string.h>

/// A bell entry. A newer notice with the same key replaces the older one.
struct MDW_NOTICE
{
    wxString key;
    wxString title;
    wxString message;
};

/**
 * The notice an event earns, or nothing when the event is state a live surface already shows. Only
 * an unlock that took away this user's own claim, a check-in and a lifecycle change are actionable.
 * aProjectSource is the locator of the open project's source, empty when there is none. Another
 * source, such as a team library, only ever earns the claim notice.
 *
 * The key is mdw-<source>-<item> with no event type, so one item holds one notice at a time.
 */
KICOMMON_API std::optional<MDW_NOTICE> MdwEventNotice( const MDW_EVENT& aEvent, const wxString& aProjectSource );

#endif // MDW_EVENT_NOTICE_H
