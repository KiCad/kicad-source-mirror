/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#pragma once

#include <CoreFoundation/CoreFoundation.h>

#include <kiplatform/environment.h>

namespace KIPLATFORM
{
    namespace ENV
    {
        /**
         * Resolves the proxy needed to reach the given url from a CFNetwork proxy settings
         * dictionary, in the form returned by CFNetworkCopySystemProxySettings
         *
         * @param aURL The target url whose proxy should be resolved
         * @param aSettings The proxy settings dictionary to resolve against
         * @param aCfg The proxy config struct that will be populated
         *
         * @return True if a proxy is required and was resolved
         */
        bool GetProxyConfigForSettings( const wxString& aURL, CFDictionaryRef aSettings,
                                        PROXY_CONFIG& aCfg );
    }
}
