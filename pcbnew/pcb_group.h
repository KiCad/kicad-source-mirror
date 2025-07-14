/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Joshua Redstone redstone at gmail.com
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
 * @file pcb_group.h
 * @brief Class to handle a set of BOARD_ITEMs.
 */

#ifndef CLASS_PCB_GROUP_H_
#define CLASS_PCB_GROUP_H_

#include <board_commit.h>
#include <board_item.h>
#include <eda_group.h>
#include <lset.h>
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
class PCB_GROUP : public BOARD_ITEM, public EDA_GROUP
{
public:
    PCB_GROUP( BOARD_ITEM* aParent );

    void Serialize( google::protobuf::Any &aContainer ) const override;
    bool Deserialize( const google::protobuf::Any &aContainer ) override;

    EDA_ITEM* AsEdaItem() override { return this; }

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && PCB_GROUP_T == aItem->Type();
    }

    wxString GetClass() const override
    {
        return wxT( "PCB_GROUP" );
    }

    std::unordered_set<BOARD_ITEM*> GetBoardItems() const;

    /*
     * Search for highest level group inside of aScope, containing item.
     *
     * @param aScope restricts the search to groups within the group scope.
     * @param isFootprintEditor true if we should stop promoting at the footprint level
     * @return group inside of aScope, containing item, if exists, otherwise, nullptr
     */
    static EDA_GROUP* TopLevelGroup( BOARD_ITEM* aItem, EDA_GROUP* aScope, bool isFootprintEditor );

    static bool WithinScope( BOARD_ITEM* aItem, PCB_GROUP* aScope, bool isFootprintEditor );

    double Similarity( const BOARD_ITEM& aOther ) const override;

    bool operator==( const PCB_GROUP& aOther ) const;
    bool operator==( const BOARD_ITEM& aBoardItem ) const override;

#if defined( DEBUG )
    void Show( int nestLevel, std::ostream& os ) const override
    {
        ShowDummy( os );
    }
#endif

    /// @copydoc EDA_ITEM::GetPosition
    VECTOR2I GetPosition() const override;

    /// @copydoc EDA_ITEM::SetPosition
    void SetPosition( const VECTOR2I& aNewpos ) override;

    /// @copydoc BOARD_ITEM::GetLayerSet
    LSET GetLayerSet() const override;

    /// @copydoc BOARD_ITEM::SetLayer
    void SetLayer( PCB_LAYER_ID aLayer ) override
    {
        // NOP
    }

    bool IsOnCopperLayer() const override
    {
        // A group might have members on a copper layer, but isn't itself on any layer.
        return false;
    }

    void SetLocked( bool aLocked ) override;

    /// @copydoc EDA_ITEM::Clone
    EDA_ITEM* Clone() const override;

    /*
     * Clone this and all descendants
     */
    PCB_GROUP* DeepClone() const;

    /*
     * Duplicate this and all descendants
     *
     * @param addToParentGroup if the original is part of a group then the new member will also
     *                         be added to said group
     */
    PCB_GROUP* DeepDuplicate( bool addToParentGroup, BOARD_COMMIT* aCommit = nullptr ) const;

    /// @copydoc BOARD_ITEM::IsOnLayer
    bool IsOnLayer( PCB_LAYER_ID aLayer ) const override;

    /// @copydoc EDA_ITEM::HitTest
    bool HitTest( const VECTOR2I& aPosition, int aAccuracy = 0 ) const override;

    /// @copydoc EDA_ITEM::HitTest
    bool HitTest( const BOX2I& aRect, bool aContained, int aAccuracy = 0 ) const override;

    /// @copydoc EDA_ITEM::HitTest
    bool HitTest( const SHAPE_LINE_CHAIN& aPoly, bool aContained ) const override;

    /// @copydoc EDA_ITEM::GetBoundingBox
    const BOX2I GetBoundingBox() const override;

    // @copydoc BOARD_ITEM::GetEffectiveShape
    std::shared_ptr<SHAPE> GetEffectiveShape( PCB_LAYER_ID aLayer = UNDEFINED_LAYER,
                                              FLASHING aFlash = FLASHING::DEFAULT ) const override;

    /// @copydoc EDA_ITEM::Visit
    INSPECT_RESULT Visit( INSPECTOR aInspector, void* aTestData,
                          const std::vector<KICAD_T>& aScanTypes ) override;

    /// @copydoc VIEW_ITEM::ViewGetLayers
    std::vector<int> ViewGetLayers() const override;

    /// @copydoc VIEW_ITEM::ViewGetLOD
    double ViewGetLOD( int aLayer, const KIGFX::VIEW* aView ) const override;

    /// @copydoc BOARD_ITEM::Move
    void Move( const VECTOR2I& aMoveVector ) override;

    /// @copydoc BOARD_ITEM::Rotate
    void Rotate( const VECTOR2I& aRotCentre, const EDA_ANGLE& aAngle ) override;

    /// @copydoc BOARD_ITEM::Flip
    void Flip( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection ) override;

    /// @copydoc BOARD_ITEM::Mirror
    void Mirror( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection ) override;

    /// @copydoc EDA_ITEM::GetItemDescription
    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const override;

    /// @copydoc EDA_ITEM::GetMenuImage
    BITMAPS GetMenuImage() const override;

    /// @copydoc EDA_ITEM::GetMsgPanelInfo
    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

    /// @copydoc EDA_ITEM::Matches
    bool Matches( const EDA_SEARCH_DATA& aSearchData, void* aAuxData ) const override;

    ///< @copydoc BOARD_ITEM::RunOnChildren
    void RunOnChildren( const std::function<void( BOARD_ITEM* )>& aFunction, RECURSE_MODE aMode ) const override;

protected:
    PCB_GROUP( BOARD_ITEM* aParent, KICAD_T idtype, PCB_LAYER_ID aLayer = F_Cu );

    /// @copydoc BOARD_ITEM::swapData
    void swapData( BOARD_ITEM* aImage ) override;
};

#endif // CLASS_PCB_GROUP_H_
