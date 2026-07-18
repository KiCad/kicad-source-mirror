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
 * @file mdw_search.h
 * Backend item search (CG_SEARCH).
 */

#ifndef MDW_CLIENT_SEARCH_H
#define MDW_CLIENT_SEARCH_H

#include <kicommon.h>
#include <mdw/client/mdw_client_types.h>
#include <mdw/client/mdw_item.h>
#include <mdw/client/mdw_op.h>

#include <api/workspace/workspace_types.pb.h>

#include <memory>
#include <vector>

class MDW_SOURCE;

/// Equality on one field
struct MDW_SEARCH_FILTER
{
    wxString field_id;
    wxString value;

    bool operator==( const MDW_SEARCH_FILTER& ) const = default;
};

/// Filters are AND-ed. Empty text and no filters list every item, most recently changed first.
struct MDW_SEARCH_QUERY
{
    wxString                       text;
    std::vector<MDW_SEARCH_FILTER> filters;

    bool operator==( const MDW_SEARCH_QUERY& ) const = default;
};

class KICOMMON_API MDW_SEARCH
{
public:
    explicit MDW_SEARCH( std::shared_ptr<MDW_SOURCE> aSource );

    /// A field type this build does not know reads as AT_STRING
    MDW_OP Fields( MDW_DONE<std::vector<kiapi::workspace::SearchField>> aDone );
    MDW_OP Run( const MDW_SEARCH_QUERY& aQuery, const MDW_PAGE& aPage, MDW_DONE<MDW_PAGED<MDW_ITEM_INFO>> aDone );

private:
    std::shared_ptr<MDW_SOURCE> m_source;
};

#endif // MDW_CLIENT_SEARCH_H
