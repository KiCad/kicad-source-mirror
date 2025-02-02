/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Mark Roszko <mark.roszko@gmail.com>
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

#ifndef NOTIFICATIONS_MANAGER_H
#define NOTIFICATIONS_MANAGER_H

#include <kicommon.h>
#include <functional>
#include <vector>

class wxString;
class KISTATUSBAR;
struct NOTIFICATION;
class NOTIFICATIONS_LIST;
class wxWindow;
class wxCloseEvent;


struct KICOMMON_API NOTIFICATION
{
public:
    wxString title;       ///< Title of the notification.
    wxString description; ///< Additional message displayed under title.
    wxString href;        ///< URL if any to link to for details.
    wxString key;         ///< Unique key to find a notification.
    wxString date;        ///< Date notification will display.
};


class KICOMMON_API NOTIFICATIONS_MANAGER
{
public:
    NOTIFICATIONS_MANAGER();

    /**
     * Create a notification with the given parameters or updates an existing one with the same key.
     *
     * @param aKey is a unique key for the notification, this allows removing or updating the same
     *        notification.
     * @param aTitle is the displayed title for the event.
     * @param aDescription is the text that displays underneath the title and has slightly more info
     *        them later programatically in case a notification is no longer required.
     * @param aHref is link to external or internal content.
     */
    void CreateOrUpdate( const wxString& aKey, const wxString& aTitle, const wxString& aDescription,
                         const wxString& aHref = wxEmptyString );

    /**
     * Remove a notification by key.
     *
     * @param aKey is the unique key to locate.
     */
    void Remove( const wxString& aKey );

    /**
     * Load notifications stored from disk.
     */
    void Load();

    /**
     * Save notifications to disk.
     */
    void Save();

    /**
     * Show the notification list.
     */
    void ShowList( wxWindow* aParent, wxPoint aPos );

    /**
     * Add a status bar for handling.
     */
    void RegisterStatusBar( KISTATUSBAR* aStatusBar );

    /**
     * Remove status bar from handling.
     */
    void UnregisterStatusBar( KISTATUSBAR* aStatusBar );

private:
    /**
     * Handle removing the shown list window from our list of shown windows.
     */
    void onListWindowClosed( wxCloseEvent& aEvent );

private:
    /// Current stack of notifications.
    std::vector<NOTIFICATION>        m_notifications;

    /// Currently shown notification lists.
    std::vector<NOTIFICATIONS_LIST*> m_shownDialogs;

    /// Status bars registered for updates.
    std::vector<KISTATUSBAR*>        m_statusBars;

    /// The cached file path to read/write notifications on disk.
    wxFileName                       m_destFileName;
};

#endif
