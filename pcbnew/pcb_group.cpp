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
#include "pcb_group.h"

#include <bitmaps.h>
#include <eda_draw_frame.h>
#include <geometry/shape_compound.h>
#include <board.h>
#include <board_item.h>
#include <confirm.h>
#include <footprint.h>
#include <pcb_generator.h>
#include <string_utils.h>
#include <widgets/msgpanel.h>
#include <view/view.h>
#include <api/api_enums.h>
#include <api/api_utils.h>
#include <api/api_pcb_utils.h>
#include <api/board/board_types.pb.h>
#include <google/protobuf/any.pb.h>

#include <wx/debug.h>

PCB_GROUP::PCB_GROUP( BOARD_ITEM* aParent ) :
        BOARD_ITEM( aParent, PCB_GROUP_T )
{
}


PCB_GROUP::PCB_GROUP( BOARD_ITEM* aParent, KICAD_T idtype, PCB_LAYER_ID aLayer ) :
        BOARD_ITEM( aParent, idtype, aLayer )
{
}

void PCB_GROUP::Serialize( google::protobuf::Any &aContainer ) const
{
    using namespace kiapi::board::types;
    Group group;

    group.mutable_id()->set_value( m_Uuid.AsStdString() );
    group.set_name( GetName().ToUTF8() );

    for( EDA_ITEM* item : GetItems() )
    {
        kiapi::common::types::KIID* itemId = group.add_items();
        itemId->set_value( item->m_Uuid.AsStdString() );
    }

    aContainer.PackFrom( group );
}


bool PCB_GROUP::Deserialize( const google::protobuf::Any &aContainer )
{
    kiapi::board::types::Group group;

    if( !aContainer.UnpackTo( &group ) )
        return false;

    const_cast<KIID&>( m_Uuid ) = KIID( group.id().value() );
    SetName( wxString( group.name().c_str(), wxConvUTF8 ) );

    BOARD* board = GetBoard();

    if( !board )
        return false;

    for( const kiapi::common::types::KIID& itemId : group.items() )
    {
        KIID id( itemId.value() );

        if( BOARD_ITEM* item = board->ResolveItem( id, true ) )
            AddItem( item );
    }

    return true;
}

std::unordered_set<BOARD_ITEM*> PCB_GROUP::GetBoardItems() const
{
    std::unordered_set<BOARD_ITEM*> items;

    for( EDA_ITEM* item : m_items )
    {
        if( item->IsBOARD_ITEM() )
            items.insert( static_cast<BOARD_ITEM*>( item ) );
    }

    return items;
}


/*
 * @return if not in the footprint editor and aItem is in a footprint, returns the
 * footprint's parent group. Otherwise, returns the aItem's parent group.
 */
EDA_GROUP* getClosestGroup( BOARD_ITEM* aItem, bool isFootprintEditor )
{
    if( !isFootprintEditor && aItem->GetParent() && aItem->GetParent()->Type() == PCB_FOOTPRINT_T )
        return aItem->GetParent()->GetParentGroup();
    else
        return aItem->GetParentGroup();
}


/// Returns the top level group inside the aScope group, or nullptr
EDA_GROUP* getNestedGroup( BOARD_ITEM* aItem, EDA_GROUP* aScope, bool isFootprintEditor )
{
    EDA_GROUP* group = getClosestGroup( aItem, isFootprintEditor );

    if( group == aScope )
        return nullptr;

    while( group && group->AsEdaItem()->GetParentGroup() && group->AsEdaItem()->GetParentGroup() != aScope )
        group = group->AsEdaItem()->GetParentGroup();

    return group;
}


EDA_GROUP* PCB_GROUP::TopLevelGroup( BOARD_ITEM* aItem, EDA_GROUP* aScope, bool isFootprintEditor )
{
    return getNestedGroup( aItem, aScope, isFootprintEditor );
}


bool PCB_GROUP::WithinScope( BOARD_ITEM* aItem, PCB_GROUP* aScope, bool isFootprintEditor )
{
    EDA_GROUP* group = getClosestGroup( aItem, isFootprintEditor );

    if( group && group == aScope )
        return true;

    EDA_GROUP* nested = getNestedGroup( aItem, aScope, isFootprintEditor );

    return nested && nested->AsEdaItem()->GetParentGroup() && ( nested->AsEdaItem()->GetParentGroup() == aScope );
}


VECTOR2I PCB_GROUP::GetPosition() const
{
    return GetBoundingBox().Centre();
}


void PCB_GROUP::SetPosition( const VECTOR2I& aNewpos )
{
    VECTOR2I delta = aNewpos - GetPosition();

    Move( delta );
}


void PCB_GROUP::SetLocked( bool aLockState )
{
    BOARD_ITEM::SetLocked( aLockState );

    RunOnChildren(
            [&]( BOARD_ITEM* child )
            {
                child->SetLocked( aLockState );
            },
            RECURSE_MODE::NO_RECURSE );
}


EDA_ITEM* PCB_GROUP::Clone() const
{
    // Use copy constructor to get the same uuid and other fields
    PCB_GROUP* newGroup = new PCB_GROUP( *this );
    return newGroup;
}


PCB_GROUP* PCB_GROUP::DeepClone() const
{
    // Use copy constructor to get the same uuid and other fields
    PCB_GROUP* newGroup = new PCB_GROUP( *this );
    newGroup->m_items.clear();

    for( EDA_ITEM* member : m_items )
    {
        if( member->Type() == PCB_GROUP_T )
            newGroup->AddItem( static_cast<PCB_GROUP*>( member )->DeepClone() );
        else if( member->Type() == PCB_GENERATOR_T )
            newGroup->AddItem( static_cast<PCB_GENERATOR*>( member )->DeepClone() );
        else
            newGroup->AddItem( static_cast<BOARD_ITEM*>( member->Clone() ) );
    }

    return newGroup;
}


PCB_GROUP* PCB_GROUP::DeepDuplicate( bool addToParentGroup, BOARD_COMMIT* aCommit ) const
{
    PCB_GROUP* newGroup = static_cast<PCB_GROUP*>( Duplicate( addToParentGroup, aCommit ) );
    newGroup->m_items.clear();

    for( EDA_ITEM* member : m_items )
    {
        if( member->Type() == PCB_GROUP_T )
            newGroup->AddItem( static_cast<PCB_GROUP*>( member )->DeepDuplicate( IGNORE_PARENT_GROUP ) );
        else
            newGroup->AddItem( static_cast<BOARD_ITEM*>( member )->Duplicate( IGNORE_PARENT_GROUP ) );
    }

    return newGroup;
}


void PCB_GROUP::swapData( BOARD_ITEM* aImage )
{
    assert( aImage->Type() == PCB_GROUP_T );
    PCB_GROUP* image = static_cast<PCB_GROUP*>( aImage );

    std::swap( *this, *image );

    // A group doesn't own its children (they're owned by the board), so undo doesn't do a
    // deep clone when making an image.  However, it's still safest to update the parentGroup
    // pointers of the group's children. We must do it in the right order in case any of the
    // children are shared (ie: image first, "this" second so that any shared children end up
    // with "this").
    image->RunOnChildren(
            [&]( BOARD_ITEM* child )
            {
                child->SetParentGroup( image );
            },
            RECURSE_MODE::NO_RECURSE );

    RunOnChildren(
            [&]( BOARD_ITEM* child )
            {
                child->SetParentGroup( this );
            },
            RECURSE_MODE::NO_RECURSE );
}


bool PCB_GROUP::HitTest( const VECTOR2I& aPosition, int aAccuracy ) const
{
    // Groups are selected by promoting a selection of one of their children
    return false;
}


bool PCB_GROUP::HitTest( const BOX2I& aRect, bool aContained, int aAccuracy ) const
{
    // Groups are selected by promoting a selection of one of their children
    return false;
}


bool PCB_GROUP::HitTest( const SHAPE_LINE_CHAIN& aPoly, bool aContained ) const
{
    // Groups are selected by promoting a selection of one of their children
    return false;
}


const BOX2I PCB_GROUP::GetBoundingBox() const
{
    BOX2I bbox;

    for( EDA_ITEM* item : m_items )
    {
        if( item->Type() == PCB_FOOTPRINT_T )
            bbox.Merge( static_cast<FOOTPRINT*>( item )->GetBoundingBox( true ) );
        else
            bbox.Merge( item->GetBoundingBox() );
    }

    bbox.Inflate( pcbIUScale.mmToIU( 0.25 ) ); // Give a min size to the bbox

    return bbox;
}


std::shared_ptr<SHAPE> PCB_GROUP::GetEffectiveShape( PCB_LAYER_ID aLayer, FLASHING aFlash ) const
{
    std::shared_ptr<SHAPE_COMPOUND> shape = std::make_shared<SHAPE_COMPOUND>();

    for( BOARD_ITEM* item : GetBoardItems() )
        shape->AddShape( item->GetEffectiveShape( aLayer, aFlash )->Clone() );

    return shape;
}


INSPECT_RESULT PCB_GROUP::Visit( INSPECTOR aInspector, void* aTestData,
                                 const std::vector<KICAD_T>& aScanTypes )
{
    for( KICAD_T scanType : aScanTypes )
    {
        if( scanType == Type() )
        {
            if( INSPECT_RESULT::QUIT == aInspector( this, aTestData ) )
                return INSPECT_RESULT::QUIT;
        }
    }

    return INSPECT_RESULT::CONTINUE;
}


LSET PCB_GROUP::GetLayerSet() const
{
    LSET aSet;

    for( EDA_ITEM* item : m_items )
        aSet |= static_cast<BOARD_ITEM*>( item )->GetLayerSet();

    return aSet;
}


bool PCB_GROUP::IsOnLayer( PCB_LAYER_ID aLayer ) const
{
    // A group is on a layer if any item is on the layer
    for( EDA_ITEM* item : m_items )
    {
        if( static_cast<BOARD_ITEM*>( item )->IsOnLayer( aLayer ) )
            return true;
    }

    return false;
}


std::vector<int> PCB_GROUP::ViewGetLayers() const
{
    return { LAYER_ANCHOR };
}


double PCB_GROUP::ViewGetLOD( int aLayer, const KIGFX::VIEW* aView ) const
{
    if( aView->IsLayerVisible( LAYER_ANCHOR ) )
        return LOD_SHOW;

    return LOD_HIDE;
}


void PCB_GROUP::Move( const VECTOR2I& aMoveVector )
{
    for( EDA_ITEM* member : m_items )
        static_cast<BOARD_ITEM*>( member )->Move( aMoveVector );
}


void PCB_GROUP::Rotate( const VECTOR2I& aRotCentre, const EDA_ANGLE& aAngle )
{
    for( EDA_ITEM* item : m_items )
        static_cast<BOARD_ITEM*>( item )->Rotate( aRotCentre, aAngle );
}


void PCB_GROUP::Flip( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection )
{
    for( EDA_ITEM* item : m_items )
        static_cast<BOARD_ITEM*>( item )->Flip( aCentre, aFlipDirection );
}


void PCB_GROUP::Mirror( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection )
{
    for( EDA_ITEM* item : m_items )
        static_cast<BOARD_ITEM*>( item )->Mirror( aCentre, aFlipDirection );
}


wxString PCB_GROUP::GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const
{
    if( m_name.empty() )
        return wxString::Format( _( "Unnamed Group, %zu members" ), m_items.size() );
    else
        return wxString::Format( _( "Group '%s', %zu members" ), m_name, m_items.size() );
}


BITMAPS PCB_GROUP::GetMenuImage() const
{
    return BITMAPS::module;
}


void PCB_GROUP::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    aList.emplace_back( _( "Group" ), m_name.empty() ? _( "<unnamed>" ) : m_name );
    aList.emplace_back( _( "Members" ), wxString::Format( wxT( "%zu" ), m_items.size() ) );

    if( aFrame->GetName() == PCB_EDIT_FRAME_NAME && IsLocked() )
        aList.emplace_back( _( "Status" ), _( "Locked" ) );
}


bool PCB_GROUP::Matches( const EDA_SEARCH_DATA& aSearchData, void* aAuxData ) const
{
    return EDA_ITEM::Matches( UnescapeString( GetName() ), aSearchData );
}


void PCB_GROUP::RunOnChildren( const std::function<void( BOARD_ITEM* )>& aFunction, RECURSE_MODE aMode ) const
{
    try
    {
        for( BOARD_ITEM* item : GetBoardItems() )
        {
            aFunction( item );

            if( aMode == RECURSE_MODE::RECURSE && ( item->Type() == PCB_GROUP_T || item->Type() == PCB_GENERATOR_T ) )
            {
                item->RunOnChildren( aFunction, RECURSE_MODE::RECURSE );
            }
        }
    }
    catch( std::bad_function_call& )
    {
        wxFAIL_MSG( wxT( "Error calling function in PCB_GROUP::RunOnChildren" ) );
    }
}


bool PCB_GROUP::operator==( const BOARD_ITEM& aBoardItem ) const
{
    if( aBoardItem.Type() != Type() )
        return false;

    const PCB_GROUP& other = static_cast<const PCB_GROUP&>( aBoardItem );

    return *this == other;
}


bool PCB_GROUP::operator==( const PCB_GROUP& aOther ) const
{
    if( m_items.size() != aOther.m_items.size() )
        return false;

    // The items in groups are in unordered sets hashed by the pointer value, so we need to
    // order them by UUID (EDA_ITEM_SET) to compare
    EDA_ITEM_SET itemSet( m_items.begin(), m_items.end() );
    EDA_ITEM_SET otherItemSet( aOther.m_items.begin(), aOther.m_items.end() );

    for( auto it1 = itemSet.begin(), it2 = otherItemSet.begin(); it1 != itemSet.end(); ++it1, ++it2 )
    {
        // Compare UUID instead of the items themselves because we only care if the contents
        // of the group has changed, not which elements in the group have changed
        if( ( *it1 )->m_Uuid != ( *it2 )->m_Uuid )
            return false;
    }

    return true;
}


double PCB_GROUP::Similarity( const BOARD_ITEM& aOther ) const
{
    if( aOther.Type() != Type() )
        return 0.0;

    const PCB_GROUP& other = static_cast<const PCB_GROUP&>( aOther );

    double similarity = 0.0;

    for( EDA_ITEM* item : m_items )
    {
        for( EDA_ITEM* otherItem : other.m_items )
        {
            similarity += static_cast<BOARD_ITEM*>( item )->Similarity( *static_cast<BOARD_ITEM*>( otherItem ) );
        }
    }

    return similarity / m_items.size();
}


static struct PCB_GROUP_DESC
{
    PCB_GROUP_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( PCB_GROUP );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_GROUP, BOARD_ITEM> );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_GROUP, EDA_GROUP> );
        propMgr.InheritsAfter( TYPE_HASH( PCB_GROUP ), TYPE_HASH( BOARD_ITEM ) );
        propMgr.InheritsAfter( TYPE_HASH( PCB_GROUP ), TYPE_HASH( EDA_GROUP ) );

        propMgr.Mask( TYPE_HASH( PCB_GROUP ), TYPE_HASH( BOARD_ITEM ), _HKI( "Position X" ) );
        propMgr.Mask( TYPE_HASH( PCB_GROUP ), TYPE_HASH( BOARD_ITEM ), _HKI( "Position Y" ) );
        propMgr.Mask( TYPE_HASH( PCB_GROUP ), TYPE_HASH( BOARD_ITEM ), _HKI( "Layer" ) );

        const wxString groupTab = _HKI( "Group Properties" );

        propMgr.AddProperty(
                new PROPERTY<EDA_GROUP, wxString>( _HKI( "Name" ), &PCB_GROUP::SetName, &PCB_GROUP::GetName ),
                groupTab );
    }
} _PCB_GROUP_DESC;
