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
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#pragma once

#include <wx/crt.h>
#include <wx/string.h>

#include <string>


namespace CLI
{

template <typename Format>
bool ParseDiffOutputFormat( const std::string& aText, Format& aOut )
{
    if( aText == "json" )
    {
        aOut = Format::JSON;
        return true;
    }

    if( aText == "text" )
    {
        aOut = Format::TEXT;
        return true;
    }

    if( aText == "png" )
    {
        aOut = Format::PNG;
        return true;
    }

    if( aText == "svg" )
    {
        aOut = Format::SVG;
        return true;
    }

    return false;
}


template <typename Format>
bool DiffOutputFormatRequiresOutputPath( Format aFormat )
{
    return aFormat == Format::PNG || aFormat == Format::SVG;
}


inline void ReportInvalidDiffOutputFormat( const std::string& aText )
{
    wxFprintf( stderr, _( "Invalid output format '%s' (expected 'json', 'text', 'png', or 'svg')\n" ),
               wxString::FromUTF8( aText ) );
}

} // namespace CLI
