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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef CASE_INSENSITIVE_MAP_H
#define CASE_INSENSITIVE_MAP_H

#include <wx/string.h>

#include <map>

namespace DETAIL
{
struct CASE_INSENSITIVE_COMPARER final
{
    bool operator()( const wxString& aLhs, const wxString& aRhs ) const noexcept
    {
        return aLhs.CmpNoCase( aRhs ) < 0;
    }
};
} // namespace DETAIL

template <typename ValueType>
using CASE_INSENSITIVE_MAP = std::map<wxString, ValueType, DETAIL::CASE_INSENSITIVE_COMPARER>;

#endif