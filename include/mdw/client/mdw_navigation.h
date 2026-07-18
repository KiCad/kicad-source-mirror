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
 * @file mdw_navigation.h
 * @brief Workspace folder navigation and item-listing options.
 */

#ifndef MDW_NAVIGATION_H
#define MDW_NAVIGATION_H

#include <wx/string.h>

struct MDW_FOLDER
{
    wxString folder_id;
    wxString display_name;
    wxString parent_folder_id;
    bool     has_children = false;
};


/**
 * One page of a list. page_size 0 asks for the backend's default and an empty page_token starts the
 * list. Only page_token may change while a list is paged through.
 */
struct MDW_PAGE
{
    unsigned page_size = 0;
    wxString page_token;
};


/// The most pages a list reads, enough for any real list and a bound on a backend that keeps answering with
/// another page token
constexpr int MDW_MAX_PAGES = 20;


#endif // MDW_NAVIGATION_H
