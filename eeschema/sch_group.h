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
 * @file sch_group.h
 * @brief Class to handle a set of SCH_ITEMs.
 */

#ifndef CLASS_SCH_GROUP_H_
#define CLASS_SCH_GROUP_H_

#include <eda_group.h>
#include <sch_commit.h>
#include <sch_item.h>
#include <lset.h>
#include <unordered_set>

namespace KIGFX
{
class VIEW;
}

/**
 * A set of SCH_ITEMs (i.e., without duplicates).
 *
 * The group parent is always sheet, not logical parent group. The group is transparent
 * container - e.g., its position is derived from the position of its members.  A selection
 * containing a group implicitly contains its members. However other operations on sets of
 * items, like committing, updating the view, etc the set is explicit.
 */
class SCH_GROUP : public SCH_ITEM, public EDA_GROUP
{
public:
    SCH_GROUP();

    SCH_GROUP( SCH_ITEM* aParent );

    SCH_GROUP( SCH_SCREEN* aParent );

    EDA_ITEM* AsEdaItem() override { return this; }

    static inline bool ClassOf( const EDA_ITEM* aItem ) { return aItem && SCH_GROUP_T == aItem->Type(); }

    wxString GetClass() const override { return wxT( "SCH_GROUP" ); }

    std::unordered_set<SCH_ITEM*> GetSchItems() const;

    /*
     * Search for highest level group inside of aScope, containing item.
     *
     * @param aScope restricts the search to groups within the group scope.
     * @param isSymbolEditor true if we should stop promoting at the symbol level
     * @return group inside of aScope, containing item, if exists, otherwise, nullptr
     */
    static EDA_GROUP* TopLevelGroup( SCH_ITEM* aItem, EDA_GROUP* aScope, bool isSymbolEditor );

    static bool WithinScope( SCH_ITEM* aItem, SCH_GROUP* aScope, bool isSymbolEditor );

    double Similarity( const SCH_ITEM& aOther ) const override;

    bool operator==( const SCH_GROUP& aOther ) const;
    bool operator==( const SCH_ITEM& aSchItem ) const override;

#if defined( DEBUG )
    void Show( int nestLevel, std::ostream& os ) const override { ShowDummy( os ); }
#endif

    /// @copydoc EDA_ITEM::GetPosition
    VECTOR2I GetPosition() const override;

    /// @copydoc EDA_ITEM::SetPosition
    void SetPosition( const VECTOR2I& aNewpos ) override;

    /// @copydoc EDA_ITEM::Clone
    EDA_ITEM* Clone() const override;

    /*
     * Clone this and all descendants
     */
    SCH_GROUP* DeepClone() const;

    /*
     * Duplicate this and all descendants
     *
     * @param addToParentGroup if the original is part of a group then the new member will also
     *                         be added to said group
     */
    SCH_GROUP* DeepDuplicate( bool addToParentGroup, SCH_COMMIT* aCommit = nullptr ) const;

    /// @copydoc EDA_ITEM::HitTest
    bool HitTest( const VECTOR2I& aPosition, int aAccuracy = 0 ) const override;

    /// @copydoc EDA_ITEM::HitTest
    bool HitTest( const BOX2I& aRect, bool aContained, int aAccuracy = 0 ) const override;

    /// @copydoc EDA_ITEM::GetBoundingBox
    const BOX2I GetBoundingBox() const override;

    /// @copydoc EDA_ITEM::Visit
    INSPECT_RESULT Visit( INSPECTOR aInspector, void* aTestData,
                          const std::vector<KICAD_T>& aScanTypes ) override;

    /// @copydoc VIEW_ITEM::ViewGetLayers
    std::vector<int> ViewGetLayers() const override;

    /// @copydoc VIEW_ITEM::ViewGetLOD
    double ViewGetLOD( int aLayer, const KIGFX::VIEW* aView ) const override;

    /// @copydoc SCH_ITEM::Move
    void Move( const VECTOR2I& aMoveVector ) override;

    /// @copydoc SCH_ITEM::Rotate
    void Rotate( const VECTOR2I& aCenter, bool aRotateCCW ) override;

    /// @copydoc SCH_ITEM::MirrorHorizontally
    void MirrorHorizontally( int aCenter ) override;

    /// @copydoc SCH_ITEM::MirrorVertically
    void MirrorVertically( int aCenter ) override;

    void Plot( PLOTTER* aPlotter, bool aBackground, const SCH_PLOT_OPTS& aPlotOpts,
               int aUnit, int aBodyStyle, const VECTOR2I& aOffset, bool aDimmed ) override;

    /// @copydoc EDA_ITEM::GetItemDescription
    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const override;

    /// @copydoc EDA_ITEM::GetMenuImage
    BITMAPS GetMenuImage() const override;

    /// @copydoc EDA_ITEM::GetMsgPanelInfo
    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

    /// @copydoc EDA_ITEM::Matches
    bool Matches( const EDA_SEARCH_DATA& aSearchData, void* aAuxData ) const override;

    ///< @copydoc SCH_ITEM::RunOnChildren
    void RunOnChildren( const std::function<void( SCH_ITEM* )>& aFunction, RECURSE_MODE aMode ) override;

    /// @copydoc SCH_ITEM::swapData
    void swapData( SCH_ITEM* aImage ) override;
};

#endif
