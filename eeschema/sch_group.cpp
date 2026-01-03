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
#include <bitmaps.h>
#include <eda_draw_frame.h>
#include <eda_group.h>
#include <geometry/shape_compound.h>
#include <sch_item.h>
#include <sch_group.h>
#include <sch_screen.h>
#include <sch_symbol.h>
#include <symbol.h>
#include <confirm.h>
#include <widgets/msgpanel.h>
#include <view/view.h>

#include <wx/debug.h>

SCH_GROUP::SCH_GROUP() : SCH_ITEM( nullptr, SCH_GROUP_T )
{
    SetLayer( LAYER_GROUP );
}

SCH_GROUP::SCH_GROUP( SCH_ITEM* aParent ) : SCH_ITEM( aParent, SCH_GROUP_T )
{
    SetLayer( LAYER_GROUP );
}

SCH_GROUP::SCH_GROUP( SCH_SCREEN* aParent ) : SCH_ITEM( aParent, SCH_GROUP_T )
{
    SetLayer( LAYER_GROUP );
}

std::unordered_set<SCH_ITEM*> SCH_GROUP::GetSchItems() const
{
    std::unordered_set<SCH_ITEM*> items;

    for( EDA_ITEM* item : m_items )
    {
        if( item->IsSCH_ITEM() )
            items.insert( static_cast<SCH_ITEM*>( item ) );
    }

    return items;
}


/*
 * @return if not in the symbol editor and aItem is in a symbol, returns the
 * symbol's parent group. Otherwise, returns the aItem's parent group.
 */
EDA_GROUP* getClosestGroup( SCH_ITEM* aItem, bool isSymbolEditor )
{
    if( !isSymbolEditor && aItem->GetParent() && aItem->GetParent()->Type() == SCH_SYMBOL_T )
        return static_cast<SCH_SYMBOL*>( aItem->GetParent() )->GetParentGroup();
    else
        return aItem->GetParentGroup();
}


/// Returns the top level group inside the aScope group, or nullptr
EDA_GROUP* getNestedGroup( SCH_ITEM* aItem, EDA_GROUP* aScope, bool isSymbolEditor )
{
    EDA_GROUP* group = getClosestGroup( aItem, isSymbolEditor );

    if( group == aScope )
        return nullptr;

    while( group && group->AsEdaItem()->GetParentGroup() && group->AsEdaItem()->GetParentGroup() != aScope )
    {
        if( group->AsEdaItem()->GetParent()->Type() == LIB_SYMBOL_T && isSymbolEditor )
            break;

        group = group->AsEdaItem()->GetParentGroup();
    }

    return group;
}


EDA_GROUP* SCH_GROUP::TopLevelGroup( SCH_ITEM* aItem, EDA_GROUP* aScope, bool isSymbolEditor )
{
    return getNestedGroup( aItem, aScope, isSymbolEditor );
}


bool SCH_GROUP::WithinScope( SCH_ITEM* aItem, SCH_GROUP* aScope, bool isSymbolEditor )
{
    EDA_GROUP* group = getClosestGroup( aItem, isSymbolEditor );

    if( group && group == aScope )
        return true;

    EDA_GROUP* nested = getNestedGroup( aItem, aScope, isSymbolEditor );

    return nested && nested->AsEdaItem()->GetParentGroup() && nested->AsEdaItem()->GetParentGroup() == aScope;
}


VECTOR2I SCH_GROUP::GetPosition() const
{
    return GetBoundingBox().Centre();
}


void SCH_GROUP::SetPosition( const VECTOR2I& aNewpos )
{
    VECTOR2I delta = aNewpos - GetPosition();

    Move( delta );
}


EDA_ITEM* SCH_GROUP::Clone() const
{
    // Use copy constructor to get the same uuid and other fields
    SCH_GROUP* newGroup = new SCH_GROUP( *this );
    return newGroup;
}


SCH_GROUP* SCH_GROUP::DeepClone() const
{
    // Use copy constructor to get the same uuid and other fields
    SCH_GROUP* newGroup = new SCH_GROUP( *this );
    newGroup->m_items.clear();

    for( EDA_ITEM* member : m_items )
    {
        if( member->Type() == SCH_GROUP_T )
            newGroup->AddItem( static_cast<SCH_GROUP*>( member )->DeepClone() );
        else
            newGroup->AddItem( static_cast<SCH_ITEM*>( member->Clone() ) );
    }

    return newGroup;
}


SCH_GROUP* SCH_GROUP::DeepDuplicate( bool addToParentGroup, SCH_COMMIT* aCommit ) const
{
    SCH_GROUP* newGroup = static_cast<SCH_GROUP*>( Duplicate( addToParentGroup, aCommit ) );
    newGroup->m_items.clear();

    for( EDA_ITEM* member : m_items )
    {
        if( member->Type() == SCH_GROUP_T )
            newGroup->AddItem( static_cast<SCH_GROUP*>( member )->DeepDuplicate( IGNORE_PARENT_GROUP ) );
        else
            newGroup->AddItem( static_cast<SCH_ITEM*>( member )->Duplicate( IGNORE_PARENT_GROUP ) );
    }

    return newGroup;
}


void SCH_GROUP::swapData( SCH_ITEM* aImage )
{
    assert( aImage->Type() == SCH_GROUP_T );
    SCH_GROUP* image = static_cast<SCH_GROUP*>( aImage );

    std::swap( m_items, image->m_items );
    std::swap( m_name, image->m_name );
    std::swap( m_designBlockLibId, image->m_designBlockLibId );

    // A group doesn't own its children (they're owned by the schematic), so undo doesn't do a
    // deep clone when making an image.  However, it's still safest to update the parentGroup
    // pointers of the group's children. We must do it in the right order in case any of the
    // children are shared (ie: image first, "this" second so that any shared children end up
    // with "this").
    image->RunOnChildren(
            [&]( SCH_ITEM* child )
            {
                child->SetParentGroup( image );
            },
            RECURSE_MODE::NO_RECURSE );

    RunOnChildren(
            [&]( SCH_ITEM* child )
            {
                child->SetParentGroup( this );
            },
            RECURSE_MODE::NO_RECURSE );
}


bool SCH_GROUP::HitTest( const VECTOR2I& aPosition, int aAccuracy ) const
{
    // Groups are selected by promoting a selection of one of their children
    return false;
}


bool SCH_GROUP::HitTest( const BOX2I& aRect, bool aContained, int aAccuracy ) const
{
    // Groups are selected by promoting a selection of one of their children
    return false;
}


const BOX2I SCH_GROUP::GetBoundingBox() const
{
    BOX2I bbox;

    for( EDA_ITEM* item : m_items )
    {
        if( item->Type() == SCH_SYMBOL_T || item->Type() == LIB_SYMBOL_T )
            bbox.Merge( static_cast<SYMBOL*>( item )->GetBoundingBox() );
        else
            bbox.Merge( item->GetBoundingBox() );
    }

    bbox.Inflate( schIUScale.MilsToIU( 10 ) );

    return bbox;
}


INSPECT_RESULT SCH_GROUP::Visit( INSPECTOR aInspector, void* aTestData,
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


std::vector<int> SCH_GROUP::ViewGetLayers() const
{
    return { LAYER_SCHEMATIC_ANCHOR };
}


double SCH_GROUP::ViewGetLOD( int aLayer, const KIGFX::VIEW* aView ) const
{
    if( aView->IsLayerVisible( LAYER_SCHEMATIC_ANCHOR ) )
        return LOD_SHOW;

    return LOD_HIDE;
}


void SCH_GROUP::Move( const VECTOR2I& aMoveVector )
{
    for( EDA_ITEM* member : m_items )
    {
        EDA_ITEM_FLAGS flags = member->GetFlags();

        if( member->Type() == SCH_LINE_T )
            member->SetFlags( STARTPOINT | ENDPOINT );

        static_cast<SCH_ITEM*>( member )->Move( aMoveVector );

        member->SetFlags( flags );
    }
}


void SCH_GROUP::Rotate( const VECTOR2I& aCenter, bool aRotateCCW )
{
    for( EDA_ITEM* member : m_items )
    {
        EDA_ITEM_FLAGS flags = member->GetFlags();

        if( member->Type() == SCH_LINE_T )
            member->SetFlags( STARTPOINT | ENDPOINT );

        static_cast<SCH_ITEM*>( member )->Rotate( aCenter, aRotateCCW );

        member->SetFlags( flags );
    }
}


void SCH_GROUP::MirrorHorizontally( int aCenter )
{
    for( EDA_ITEM* member : m_items )
    {
        EDA_ITEM_FLAGS flags = member->GetFlags();

        if( member->Type() == SCH_LINE_T )
            member->SetFlags( STARTPOINT | ENDPOINT );

        static_cast<SCH_ITEM*>( member )->MirrorHorizontally( aCenter );

        member->SetFlags( flags );
    }
}


void SCH_GROUP::MirrorVertically( int aCenter )
{
    for( EDA_ITEM* member : m_items )
    {
        EDA_ITEM_FLAGS flags = member->GetFlags();

        if( member->Type() == SCH_LINE_T )
            member->SetFlags( STARTPOINT | ENDPOINT );

        static_cast<SCH_ITEM*>( member )->MirrorVertically( aCenter );

        member->SetFlags( flags );
    }
}


void SCH_GROUP::Plot( PLOTTER* aPlotter, bool aBackground, const SCH_PLOT_OPTS& aPlotOpts,
                      int aUnit, int aBodyStyle, const VECTOR2I& aOffset, bool aDimmed )
{
    // TODO: should we plot the name & border of named groups?
}


wxString SCH_GROUP::GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const
{
    if( m_name.empty() )
        return wxString::Format( _( "Unnamed Group, %zu members" ), m_items.size() );
    else
        return wxString::Format( _( "Group '%s', %zu members" ), m_name, m_items.size() );
}


BITMAPS SCH_GROUP::GetMenuImage() const
{
    return BITMAPS::module;
}


void SCH_GROUP::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    aList.emplace_back( _( "Group" ), m_name.empty() ? _( "<unnamed>" ) : m_name );
    aList.emplace_back( _( "Members" ), wxString::Format( wxT( "%zu" ), m_items.size() ) );
}


bool SCH_GROUP::Matches( const EDA_SEARCH_DATA& aSearchData, void* aAuxData ) const
{
    return EDA_ITEM::Matches( UnescapeString( GetName() ), aSearchData );
}


void SCH_GROUP::RunOnChildren( const std::function<void( SCH_ITEM* )>& aFunction, RECURSE_MODE aMode )
{
    try
    {
        for( EDA_ITEM* item : m_items )
        {
            aFunction( static_cast<SCH_ITEM*>( item ) );

            if( item->Type() == SCH_GROUP_T && aMode == RECURSE_MODE::RECURSE )
                static_cast<SCH_GROUP*>( item )->RunOnChildren( aFunction, RECURSE_MODE::RECURSE );
        }
    }
    catch( std::bad_function_call& )
    {
        wxFAIL_MSG( wxT( "Error calling function in SCH_GROUP::RunOnChildren" ) );
    }
}


bool SCH_GROUP::operator==( const SCH_ITEM& aSchItem ) const
{
    if( aSchItem.Type() != Type() )
        return false;

    const SCH_GROUP& other = static_cast<const SCH_GROUP&>( aSchItem );

    return *this == other;
}


bool SCH_GROUP::operator==( const SCH_GROUP& aOther ) const
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


double SCH_GROUP::Similarity( const SCH_ITEM& aOther ) const
{
    if( aOther.Type() != Type() )
        return 0.0;

    const SCH_GROUP& other = static_cast<const SCH_GROUP&>( aOther );

    double similarity = 0.0;

    for( EDA_ITEM* item : m_items )
    {
        for( EDA_ITEM* otherItem : other.m_items )
        {
            similarity += static_cast<SCH_ITEM*>( item )->Similarity( *static_cast<SCH_ITEM*>( otherItem ) );
        }
    }

    return similarity / m_items.size();
}


static struct SCH_GROUP_DESC
{
    SCH_GROUP_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( SCH_GROUP );
        propMgr.AddTypeCast( new TYPE_CAST<SCH_GROUP, SCH_ITEM> );
        propMgr.AddTypeCast( new TYPE_CAST<SCH_GROUP, EDA_GROUP> );
        propMgr.InheritsAfter( TYPE_HASH( SCH_GROUP ), TYPE_HASH( SCH_ITEM ) );
        propMgr.InheritsAfter( TYPE_HASH( SCH_GROUP ), TYPE_HASH( EDA_GROUP ) );

        propMgr.Mask( TYPE_HASH( SCH_GROUP ), TYPE_HASH( SCH_ITEM ), _HKI( "Position X" ) );
        propMgr.Mask( TYPE_HASH( SCH_GROUP ), TYPE_HASH( SCH_ITEM ), _HKI( "Position Y" ) );

        const wxString groupTab = _HKI( "Group Properties" );

        propMgr.AddProperty(
                new PROPERTY<EDA_GROUP, wxString>( _HKI( "Name" ), &SCH_GROUP::SetName, &SCH_GROUP::GetName ),
                groupTab );
    }
} _SCH_GROUP_DESC;
