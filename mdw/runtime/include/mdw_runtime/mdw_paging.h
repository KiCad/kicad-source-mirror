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

/**
 * @file mdw_paging.h
 * @brief Stateless continuation tokens for a PageRequest.
 *
 * A token is "offset:size:snapshot", where the snapshot is whatever identifies the listing being paged, such as
 * a commit id or a hash of its rows. The backend keeps nothing between pages, and a token from another page size
 * or another listing is refused.
 */

#ifndef MDW_PAGING_H
#define MDW_PAGING_H

#include <api/mdw/backend_base.pb.h>
#include <api/workspace/workspace_types.pb.h>

#include <cstddef>
#include <string>

/**
 * The rows [aBegin, aEnd) that aPage asks for out of aCount rows of the listing aSnapshot names, and in
 * aNextToken the token for the page after, empty after the last. WE_INVALID_PARAMETER for a token of another
 * query, WE_VERSION_CONFLICT when the listing changed since the token was issued.
 */
kiapi::workspace::WorkspaceError MdwPageRange( const kiapi::mdw::PageRequest& aPage, std::size_t aCount,
                                               const std::string& aSnapshot, std::size_t& aBegin, std::size_t& aEnd,
                                               std::string& aNextToken );

#endif // MDW_PAGING_H
