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

#ifndef EDA_GROUP_H
#define EDA_GROUP_H

#include <eda_item.h>
#include <lib_id.h>
#include <lset.h>
#include <unordered_set>

namespace KIGFX
{
class VIEW;
}

/**
 * A set of EDA_ITEMs (i.e., without duplicates).
 *
 * The group parent is always board/sheet, not logical parent group. The group is transparent
 * container - e.g., its position is derived from the position of its members.  A selection
 * containing a group implicitly contains its members. However other operations on sets of
 * items, like committing, updating the view, etc the set is explicit.
 */
class EDA_GROUP
{
public:
    virtual EDA_ITEM* AsEdaItem() = 0;
    virtual ~EDA_GROUP() = default;

    wxString GetName() const { return m_name; }
    void     SetName( const wxString& aName ) { m_name = aName; }

    std::unordered_set<EDA_ITEM*>& GetItems() { return m_items; }
    const std::unordered_set<EDA_ITEM*>& GetItems() const { return m_items; }

    /**
     * Add item to group. Does not take ownership of item.
     */
    void AddItem( EDA_ITEM* aItem );

    /**
     * Remove item from group.
     */
    void RemoveItem( EDA_ITEM* aItem );
    void RemoveAll();

    KIID_VECT_LIST GetGroupMemberIds() const;

    bool HasDesignBlockLink() const { return m_designBlockLibId.IsValid(); }

    void SetDesignBlockLibId( const LIB_ID& aLibId ) { m_designBlockLibId = aLibId; }
    const LIB_ID& GetDesignBlockLibId() const { return m_designBlockLibId; }

protected:
    std::unordered_set<EDA_ITEM*> m_items;             // Members of the group (no ownership)
    wxString                      m_name;              // Optional group name
    LIB_ID                        m_designBlockLibId;  // Optional link to a design block
};

#endif // CLASS_PCB_GROUP_H_
