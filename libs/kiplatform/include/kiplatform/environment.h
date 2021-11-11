/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Ian McInerney <Ian.S.McInerney at ieee.org>
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <wx/string.h>

namespace KIPLATFORM
{
    namespace ENV
    {
        /**
         * Perform environment initialization tasks. These tasks are called during the wxApp
         * constructor and therefore won't have access to the underlying OS application.
         */
        void Init();

        /**
         * Move the specified file/directory to the trash bin/recycle bin.
         *
         * @param aPath is the absolute path of the file/directory to move to the trash
         * @param aError is the error message saying why the operation failed
         *
         * @return true if the operation succeeds, false if it fails (see the contents of aError)
         */
        bool MoveToTrash( const wxString& aPath, wxString& aError );

        /**
         * Determines if a given path is a network shared file apth
         * On Windows for example, any form of path is accepted drive map or UNC
         *
         * @param aPath is any kind of file path to be tested
         *
         * @return true if given path is on a network location
         */
        bool IsNetworkPath( const wxString& aPath );


        /**
         * Retrieves the operating system specific path for a user's documents
         *
         * @return User documents path
         */
        wxString GetDocumentsPath();

        /**
         * Retrieves the operating system specific path for a user's configuration store
         *
         * @return User config path
         */
        wxString GetUserConfigPath();

        /**
         * Retrieves the operating system specific path for user's application cache
         *
         * @return User cache path
         */
        wxString GetUserCachePath();

        struct PROXY_CONFIG
        {
            wxString host;
            wxString username;
            wxString password;
        };

        /**
         * Retrieves platform level proxying requirements to reach the given url
         *
         * @param aURL The target url we will be requesting over http
         * @param aCfg The proxy config struct that will be populated
         *
         * @return True if successful fetched proxy info
         */
        bool GetSystemProxyConfig( const wxString& aURL, PROXY_CONFIG& aCfg );
    }
}
