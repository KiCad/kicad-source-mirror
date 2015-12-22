/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 2015 KiCad Developers, see CHANGELOG.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
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

#include <kicad_curl/kicad_curl.h>

bool KICAD_CURL::Init()
{
    if ( curl_global_init( CURL_GLOBAL_ALL ) != CURLE_OK )
    {
        return false;
    }
    else
    {
        m_initialized = true;
        return true;
    }
}


void KICAD_CURL::Cleanup()
{
    if( m_initialized )
        curl_global_cleanup();
}


std::string KICAD_CURL::GetVersion()
{
    return std::string( curl_version() );
}


bool KICAD_CURL::m_initialized = false;