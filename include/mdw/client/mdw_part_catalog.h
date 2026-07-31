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
 * @file mdw_part_catalog.h
 * Managed parts (CG_PARTS), with lookup, search and the parts that use a library member.
 */

#ifndef MDW_CLIENT_PART_CATALOG_H
#define MDW_CLIENT_PART_CATALOG_H

#include <kicommon.h>
#include <mdw/client/mdw_client_types.h>
#include <mdw/client/mdw_op.h>

#include <memory>
#include <vector>

class MDW_SOURCE;

enum class MDW_MEMBER_KIND
{
    UNKNOWN   = 0,
    SYMBOL    = 1,
    FOOTPRINT = 2
};

/// A library member a part revision uses. member.path_hint is "nickname:item". member.item_id is empty when no
/// library of the source holds it.
struct MDW_PART_MEMBER
{
    MDW_MEMBER_KIND kind = MDW_MEMBER_KIND::UNKNOWN;
    MDW_ITEM_REF    member;
    wxString        revision_id;            ///< Empty for its latest release
};

struct MDW_MANUFACTURER_PART
{
    wxString manufacturer;
    wxString mpn;
};

struct MDW_PART_INFO
{
    MDW_ITEM_REF                       part;
    wxString                           part_number;
    wxString                           name;
    wxString                           description;
    wxString                           revision_id;
    wxString                           revision_label;
    MDW_LIFECYCLE_STATE                state;
    std::vector<MDW_PART_MEMBER>       members;
    std::vector<MDW_MANUFACTURER_PART> manufacturer_parts;
    wxString                           url;

    wxString DisplayDescription() const { return description.IsEmpty() ? name : description; }
};

/// Every part using one member. More than one is shown as a choice; none is preferred.
struct MDW_PART_CANDIDATES
{
    MDW_ITEM_REF               member;
    std::vector<MDW_PART_INFO> candidates;
};

class KICOMMON_API MDW_PART_CATALOG
{
public:
    explicit MDW_PART_CATALOG( std::shared_ptr<MDW_SOURCE> aSource );

    MDW_OP Get( const MDW_ITEM_REF& aPart, const wxString& aRevisionId, MDW_DONE<MDW_PART_INFO> aDone );
    MDW_OP Search( const wxString& aText, const MDW_PAGE& aPage, MDW_DONE<MDW_PAGED<MDW_PART_INFO>> aDone );

    /// At most 1000 members; one result per member, in order
    MDW_OP Resolve( const std::vector<MDW_ITEM_REF>& aMembers,
                    MDW_DONE<std::vector<MDW_PART_CANDIDATES>> aDone );

private:
    std::shared_ptr<MDW_SOURCE> m_source;
};

#endif // MDW_CLIENT_PART_CATALOG_H
