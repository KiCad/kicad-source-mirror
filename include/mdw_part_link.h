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
 * @file mdw_part_link.h
 * @brief The MDW_Part field a placed symbol carries once a workspace action links it to a part.
 *
 * It holds identity only, the part's source locator, its part id and the part revision placed. Labels and
 * states are read from the source when shown, so the field never goes stale. Only placing a workspace part
 * and Assign Part write it; it is hidden on the canvas.
 */

#ifndef MDW_PART_LINK_H
#define MDW_PART_LINK_H

#include <kicommon.h>
#include <mdw/client/mdw_part.h>

#include <wx/string.h>

#include <optional>


constexpr const char MDW_PART_FIELD[] = "MDW_Part";

/// The field text, which separates the source locator, part id and revision id with '|' and escapes '%' and '|'
KICOMMON_API wxString MdwFormatPartField( const MDW_PART_IDENTITY& aPart );

/// The part aText names, or nothing when it is not a field this wrote, as after a user edited it by hand
KICOMMON_API std::optional<MDW_PART_IDENTITY> MdwParsePartField( const wxString& aText );

#endif // MDW_PART_LINK_H
