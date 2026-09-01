/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef LIBRARY_DIGEST_H
#define LIBRARY_DIGEST_H

#include <picosha2.h>

#include <wx/string.h>

#include <string>


namespace KICAD_DIFF
{

/// "v1:" and the SHA-256 of aText in hex. Backends store digests and match them as strings, so a change
/// to what a digest covers needs a new scheme prefix.
inline wxString LibraryDigest( const std::string& aText )
{
    return wxS( "v1:" ) + wxString::FromUTF8( picosha2::hash256_hex_string( aText ) );
}

} // namespace KICAD_DIFF

#endif // LIBRARY_DIGEST_H
