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
 * @file mdw_lifecycle.h
 * @brief Lifecycle transition requests and results.
 *
 * The framework consults only the four universal categories. A transition
 * targets a backend custom state; the backend reports the resulting category.
 */

#ifndef MDW_LIFECYCLE_H
#define MDW_LIFECYCLE_H

#include <kicommon.h>
#include <mdw/client/mdw_types.h>

#include <gal/color4d.h>

#include <wx/string.h>

#include <vector>

struct MDW_REVISION;

/// True when items in aCategory refuse writes, which is what a save warning keys on
KICOMMON_API bool MdwCategoryBlocksWrites( MDW_LIFECYCLE_CATEGORY aCategory );

/// The translated name of aCategory, as the status pill and save warnings show it
KICOMMON_API wxString MdwCategoryName( MDW_LIFECYCLE_CATEGORY aCategory );

/// "Rev 3" for a release, a baseline's label ("Baseline 3") as given, and empty for anything else
KICOMMON_API wxString MdwRevisionText( const MDW_REVISION& aRevision );

/// The pill colour for aCategory: green editable, amber pending review, red restricted, blue released
KICOMMON_API KIGFX::COLOR4D MdwCategoryColor( MDW_LIFECYCLE_CATEGORY aCategory );


#endif // MDW_LIFECYCLE_H
