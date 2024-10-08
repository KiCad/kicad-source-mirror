/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Joshua Redstone redstone at gmail.com
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <bitmaps.h>
#include <eda_draw_frame.h>
#include <geometry/shape_compound.h>
#include <board.h>
#include <board_item.h>
#include <footprint.h>
#include <pcb_generator.h>
#include <pcb_group.h>
#include <confirm.h>
#include <widgets/msgpanel.h>
#include <view/view.h>

#include <wx/debug.h>

PCB_GROUP::PCB_GROUP( BOARD_ITEM* aParent ) :
        BOARD_ITEM( aParent, PCB_GROUP_T )
{
}


PCB_GROUP::PCB_GROUP( BOARD_ITEM* aParent, KICAD_T idtype, PCB_LAYER_ID aLayer ) :
        BOARD_ITEM( aParent, idtype, aLayer )
{
}


bool PCB_GROUP::IsGroupableType( KICAD_T aType )
{
    switch ( aType )
    {
    case PCB_FOOTPRINT_T:
    case PCB_PAD_T:
    case PCB_SHAPE_T:
    case PCB_REFERENCE_IMAGE_T:
    case PCB_FIELD_T:
    case PCB_TEXT_T:
    case PCB_TEXTBOX_T:
    case PCB_TABLE_T:
    case PCB_GROUP_T:
    case PCB_GENERATOR_T:
    case PCB_TRACE_T:
    case PCB_VIA_T:
    case PCB_ARC_T:
    case PCB_DIMENSION_T:
    case PCB_DIM_ALIGNED_T:
    case PCB_DIM_LEADER_T:
    case PCB_DIM_CENTER_T:
    case PCB_DIM_RADIAL_T:
    case PCB_DIM_ORTHOGONAL_T:
    case PCB_ZONE_T:
        return true;
    default:
        return false;
    }
}


bool PCB_GROUP::AddItem( BOARD_ITEM* aItem )
{
    wxCHECK_MSG( IsGroupableType( aItem->Type() ), false,
            wxT( "Invalid item type added to group: " ) + aItem->GetTypeDesc() );

    // Items can only be in one group at a time
    if( aItem->GetParentGroup() )
        aItem->GetParentGroup()->RemoveItem( aItem );

    m_items.insert( aItem );
    aItem->SetParentGroup( this );
    return true;
}


bool PCB_GROUP::RemoveItem( BOARD_ITEM* aItem )
{
    // Only clear the item's group field if it was inside this group
    if( m_items.erase( aItem ) == 1 )
    {
        aItem->SetParentGroup( nullptr );
        return true;
    }

    return false;
}


void PCB_GROUP::RemoveAll()
{
    for( BOARD_ITEM* item : m_items )
        item->SetParentGroup( nullptr );

    m_items.clear();
}


/*
 * @return if not in the footprint editor and aItem is in a footprint, returns the
 * footprint's parent group. Otherwise, returns the aItem's parent group.
 */
PCB_GROUP* getClosestGroup( BOARD_ITEM* aItem, bool isFootprintEditor )
{
    if( !isFootprintEditor && aItem->GetParent() && aItem->GetParent()->Type() == PCB_FOOTPRINT_T )
        return aItem->GetParent()->GetParentGroup();
    else
        return aItem->GetParentGroup();
}


/// Returns the top level group inside the aScope group, or nullptr
PCB_GROUP* getNestedGroup( BOARD_ITEM* aItem, PCB_GROUP* aScope, bool isFootprintEditor )
{
    PCB_GROUP* group = getClosestGroup( aItem, isFootprintEditor );

    if( group == aScope )
        return nullptr;

    while( group && group->GetParentGroup() && group->GetParentGroup() != aScope )
    {
        if( group->GetParent()->Type() == PCB_FOOTPRINT_T && isFootprintEditor )
            break;

        group = group->GetParentGroup();
    }

    return group;
}


PCB_GROUP* PCB_GROUP::TopLevelGroup( BOARD_ITEM* aItem, PCB_GROUP* aScope, bool isFootprintEditor )
{
    return getNestedGroup( aItem, aScope, isFootprintEditor );
}


bool PCB_GROUP::WithinScope( BOARD_ITEM* aItem, PCB_GROUP* aScope, bool isFootprintEditor )
{
    PCB_GROUP* group = getClosestGroup( aItem, isFootprintEditor );

    if( group && group == aScope )
        return true;

    PCB_GROUP* nested = getNestedGroup( aItem, aScope, isFootprintEditor );

    return nested && nested->GetParentGroup() && nested->GetParentGroup() == aScope;
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
            } );
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

    for( BOARD_ITEM* member : m_items )
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


PCB_GROUP* PCB_GROUP::DeepDuplicate() const
{
    PCB_GROUP* newGroup = static_cast<PCB_GROUP*>( Duplicate() );
    newGroup->m_items.clear();

    for( BOARD_ITEM* member : m_items )
    {
        if( member->Type() == PCB_GROUP_T )
            newGroup->AddItem( static_cast<PCB_GROUP*>( member )->DeepDuplicate() );
        else
            newGroup->AddItem( static_cast<BOARD_ITEM*>( member->Duplicate() ) );
    }

    return newGroup;
}


void PCB_GROUP::swapData( BOARD_ITEM* aImage )
{
    assert( aImage->Type() == PCB_GROUP_T );

    std::swap( *( (PCB_GROUP*) this ), *( (PCB_GROUP*) aImage ) );
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


const BOX2I PCB_GROUP::GetBoundingBox() const
{
    BOX2I bbox;

    for( BOARD_ITEM* item : m_items )
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

    for( BOARD_ITEM* item : m_items )
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

    for( BOARD_ITEM* item : m_items )
        aSet |= item->GetLayerSet();

    return aSet;
}


bool PCB_GROUP::IsOnLayer( PCB_LAYER_ID aLayer ) const
{
    // A group is on a layer if any item is on the layer
    for( BOARD_ITEM* item : m_items )
    {
        if( item->IsOnLayer( aLayer ) )
            return true;
    }

    return false;
}


void PCB_GROUP::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount = 1;
    aLayers[0] = LAYER_ANCHOR;
}


double PCB_GROUP::ViewGetLOD( int aLayer, KIGFX::VIEW* aView ) const
{
    if( aView->IsLayerVisible( LAYER_ANCHOR ) )
        return 0.0;

    return std::numeric_limits<double>::max();
}


void PCB_GROUP::Move( const VECTOR2I& aMoveVector )
{
    for( BOARD_ITEM* member : m_items )
        member->Move( aMoveVector );
}


void PCB_GROUP::Rotate( const VECTOR2I& aRotCentre, const EDA_ANGLE& aAngle )
{
    for( BOARD_ITEM* item : m_items )
        item->Rotate( aRotCentre, aAngle );
}


void PCB_GROUP::Flip( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection )
{
    for( BOARD_ITEM* item : m_items )
        item->Flip( aCentre, aFlipDirection );
}


void PCB_GROUP::Mirror( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection )
{
    for( BOARD_ITEM* item : m_items )
        item->Mirror( aCentre, aFlipDirection );
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


void PCB_GROUP::RunOnChildren( const std::function<void( BOARD_ITEM* )>& aFunction ) const
{
    try
    {
        for( BOARD_ITEM* item : m_items )
            aFunction( item );
    }
    catch( std::bad_function_call& )
    {
        wxFAIL_MSG( wxT( "Error calling function in PCB_GROUP::RunOnChildren" ) );
    }
}


void PCB_GROUP::RunOnDescendants( const std::function<void( BOARD_ITEM* )>& aFunction,
                                  int aDepth ) const
{
    // Avoid freezes with infinite recursion
    if( aDepth > 20 )
        return;

    try
    {
        for( BOARD_ITEM* item : m_items )
        {
            aFunction( item );

            if( item->Type() == PCB_GROUP_T || item->Type() == PCB_GENERATOR_T )
                item->RunOnDescendants( aFunction, aDepth + 1 );
        }
    }
    catch( std::bad_function_call& )
    {
        wxFAIL_MSG( wxT( "Error calling function in PCB_GROUP::RunOnDescendants" ) );
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

    for( BOARD_ITEM* item : m_items )
    {
        for( BOARD_ITEM* otherItem : other.m_items )
        {
            similarity += item->Similarity( *otherItem );
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
        propMgr.InheritsAfter( TYPE_HASH( PCB_GROUP ), TYPE_HASH( BOARD_ITEM ) );

        propMgr.Mask( TYPE_HASH( PCB_GROUP ), TYPE_HASH( BOARD_ITEM ), _HKI( "Position X" ) );
        propMgr.Mask( TYPE_HASH( PCB_GROUP ), TYPE_HASH( BOARD_ITEM ), _HKI( "Position Y" ) );
        propMgr.Mask( TYPE_HASH( PCB_GROUP ), TYPE_HASH( BOARD_ITEM ), _HKI( "Layer" ) );

        const wxString groupTab = _HKI( "Group Properties" );

        propMgr.AddProperty( new PROPERTY<PCB_GROUP, wxString>( _HKI( "Name" ),
                    &PCB_GROUP::SetName, &PCB_GROUP::GetName ),
                    groupTab );
    }
} _PCB_GROUP_DESC;
