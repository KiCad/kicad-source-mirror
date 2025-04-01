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
    virtual ~EDA_GROUP();

    wxString GetName() const { return m_name; }
    void     SetName( const wxString& aName ) { m_name = aName; }

    std::unordered_set<EDA_ITEM*>& GetItems() { return m_items; }

    const std::unordered_set<EDA_ITEM*>& GetItems() const { return m_items; }

    /**
     * Add item to group. Does not take ownership of item.
     *
     * @return true if item was added (false if item belongs to a different group).
     */
    virtual bool AddItem( EDA_ITEM* aItem ) = 0;

    /**
     * Remove item from group.
     *
     * @return true if item was removed (false if item was not in the group).
     */
    virtual bool RemoveItem( EDA_ITEM* aItem ) = 0;

    virtual void RemoveAll() = 0;

    /*
     * Clone() this and all descendants
     */
    virtual EDA_GROUP* DeepClone() const = 0;

    /*
     * Duplicate() this and all descendants
     */
    virtual EDA_GROUP* DeepDuplicate() const = 0;

    /**
     * Check if the proposed type can be added to a group
     * @param aType KICAD_T type to check
     * @return true if the type can belong to a group, false otherwise
     */
    //virtual static bool IsGroupableType( KICAD_T aType );

protected:
    std::unordered_set<EDA_ITEM*> m_items; // Members of the group
    wxString                      m_name;  // Optional group name
};

#endif // CLASS_PCB_GROUP_H_
