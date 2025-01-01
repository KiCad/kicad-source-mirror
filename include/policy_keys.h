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


#ifndef POLICY_KEYS_H_
#define POLICY_KEYS_H_

#include <cstdint>

#define POLICY_KEY_DATACOLLECTION wxT( "DataCollection" )
#define POLICY_KEY_PCM wxT( "PluginAndContentManager" )
#define POLICY_KEY_REQUESTS_CURL_REVOKE wxT( "curl\\SslRevoke" )

enum class POLICY_CURL_SSL_REVOKE : std::uint32_t
{
	DEFAULT = 0,
	BEST_EFFORT = 1,
	NONE = 2
};

#endif // POLICY_KEYS_H_