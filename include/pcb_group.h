/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Joshua Redstone redstone at gmail.com
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file class_pcb_group.h
 * @brief Class to handle a set of BOARD_ITEMs.
 */

#ifndef CLASS_PCB_GROUP_H_
#define CLASS_PCB_GROUP_H_

#include <board_commit.h>
#include <board_item.h>
#include <unordered_set>

namespace KIGFX
{
class VIEW;
}

/**
 * A set of BOARD_ITEMs (i.e., without duplicates).
 *
 * The group parent is always board, not logical parent group. The group is transparent
 * container - e.g., its position is derived from the position of its members.  A selection
 * containing a group implicitly contains its members. However other operations on sets of
 * items, like committing, updating the view, etc the set is explicit.
 */
class PCB_GROUP : public BOARD_ITEM
{
public:
    PCB_GROUP( BOARD_ITEM* aParent );

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && PCB_GROUP_T == aItem->Type();
    }

    wxString GetClass() const override
    {
        return wxT( "PCB_GROUP" );
    }

    wxString GetName() const { return m_name; }
    void SetName( wxString aName ) { m_name = aName; }

    std::unordered_set<BOARD_ITEM*>& GetItems()
    {
        return m_items;
    }

    const std::unordered_set<BOARD_ITEM*>& GetItems() const
    {
        return m_items;
    }

    /**
     * Add item to group. Does not take ownership of item.
     *
     * @return true if item was added (false if item belongs to a different group).
     */
    bool AddItem( BOARD_ITEM* aItem );

    /**
     * Remove item from group.
     *
     * @return true if item was removed (false if item was not in the group).
     */
    bool RemoveItem( BOARD_ITEM* aItem );

    void RemoveAll();

    /*
     * Search for highest level group containing item.
     *
     * @param aScope restricts the search to groups within the group scope.
     * @param aFootprintEditor true if we should stop promoting at the footprint level
     * @return group containing item, if it exists, otherwise, NULL
     */
    static PCB_GROUP* TopLevelGroup( BOARD_ITEM* aItem, PCB_GROUP* aScope, bool aFootprintEditor );

    static bool WithinScope( BOARD_ITEM* item, PCB_GROUP* scope );

#if defined( DEBUG )
    void Show( int nestLevel, std::ostream& os ) const override
    {
        ShowDummy( os );
    }
#endif

    ///< @copydoc EDA_ITEM::GetPosition
    wxPoint GetPosition() const override;

    ///< @copydoc EDA_ITEM::SetPosition
    void SetPosition( const wxPoint& aNewpos ) override;

    ///< @copydoc BOARD_ITEM::GetLayerSet
    LSET GetLayerSet() const override;

    ///< @copydoc BOARD_ITEM::SetLayer
    void SetLayer( PCB_LAYER_ID aLayer ) override
    {
        wxFAIL_MSG( "groups don't support layer SetLayer" );
    }

    ///< @copydoc EDA_ITEM::Clone
    EDA_ITEM* Clone() const override;

    /*
     * Clone() this and all descendants
     */
    PCB_GROUP* DeepClone() const;

    /*
     * Duplicate() this and all descendants
     */
    PCB_GROUP* DeepDuplicate() const;

    ///< @copydoc BOARD_ITEM::SwapData
    void SwapData( BOARD_ITEM* aImage ) override;

    ///< @copydoc BOARD_ITEM::IsOnLayer
    bool IsOnLayer( PCB_LAYER_ID aLayer ) const override;

    ///< @copydoc EDA_ITEM::HitTest
    bool HitTest( const wxPoint& aPosition, int aAccuracy = 0 ) const override;

    ///< @copydoc EDA_ITEM::HitTest
    bool HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy = 0 ) const override;

    ///< @copydoc EDA_ITEM::GetBoundingBox
    const EDA_RECT GetBoundingBox() const override;

    ///< @copydoc EDA_ITEM::Visit
    SEARCH_RESULT Visit( INSPECTOR aInspector, void* aTestData,
                         const KICAD_T aScanTypes[] ) override;

    ///< @copydoc VIEW_ITEM::ViewGetLayers
    void ViewGetLayers( int aLayers[], int& aCount ) const override;

    ///< @copydoc VIEW_ITEM::ViewGetLOD
    double ViewGetLOD( int aLayer, KIGFX::VIEW* aView ) const override;

    ///< @copydoc BOARD_ITEM::Move
    void Move( const wxPoint& aMoveVector ) override;

    ///< @copydoc BOARD_ITEM::Rotate
    void Rotate( const wxPoint& aRotCentre, double aAngle ) override;

    ///< @copydoc BOARD_ITEM::Flip
    void Flip( const wxPoint& aCentre, bool aFlipLeftRight ) override;

    ///< @copydoc EDA_ITEM::GetSelectMenuText
    wxString GetSelectMenuText( EDA_UNITS aUnits ) const override;

    ///< @copydoc EDA_ITEM::GetMenuImage
    BITMAP_DEF GetMenuImage() const override;

    ///< @copydoc EDA_ITEM::GetMsgPanelInfo
    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

    /**
     * Invoke a function on all members of the group.
     *
     * @note This function should not add or remove items to the group.
     *
     * @param aFunction is the function to be invoked.
     */
    void RunOnChildren( const std::function<void ( BOARD_ITEM* )>& aFunction ) const;

    /**
     * Invoke a function on all descendants of the group.
     *
     * @note This function should not add or remove items to the group or descendant groups.
     * @param aFunction is the function to be invoked.
     */
    void RunOnDescendants( const std::function<void( BOARD_ITEM* )>& aFunction ) const;

private:
    std::unordered_set<BOARD_ITEM*> m_items;     // Members of the group
    wxString                        m_name;      // Optional group name
};

#endif // CLASS_PCB_GROUP_H_
