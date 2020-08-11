/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Joshua Redstone redstone at gmail.com
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <class_group.h>
#include <confirm.h>
#include <msgpanel.h>
#include <view/view.h>

GROUP::GROUP( BOARD* parent ) : BOARD_ITEM( (BOARD_ITEM*) parent, PCB_GROUP_T )
{
}


bool GROUP::AddItem( BOARD_ITEM* item )
{
    return m_items.insert( item ).second;
}

bool GROUP::RemoveItem( const BOARD_ITEM* item )
{
    return m_items.erase( const_cast<BOARD_ITEM*>( item ) ) == 1;
}

wxPoint GROUP::GetPosition() const
{
    return GetBoundingBox().Centre();
}

void GROUP::SetPosition( const wxPoint& newpos )
{
    wxPoint delta = newpos - GetPosition();

    for( auto member : m_items )
    {
        member->SetPosition( member->GetPosition() + delta );
    }
}

EDA_ITEM* GROUP::Clone() const
{
    // Use copy constructor to get the same uuid and other fields
    GROUP* newGroup = new GROUP( *this );
    return newGroup;
}

GROUP* GROUP::DeepClone() const
{
    // Use copy constructor to get the same uuid and other fields
    GROUP* newGroup = new GROUP( *this );
    newGroup->m_items.clear();

    for( auto member : m_items )
    {
        if( member->Type() == PCB_GROUP_T )
        {
            newGroup->AddItem( static_cast<GROUP*>( member )->DeepClone() );
        }
        else
        {
            newGroup->AddItem( static_cast<BOARD_ITEM*>( member->Clone() ) );
        }
    }

    return newGroup;
}


GROUP* GROUP::DeepDuplicate() const
{
    GROUP* newGroup = static_cast<GROUP*>( this->Duplicate() );
    newGroup->m_items.clear();

    for( auto member : m_items )
    {
        if( member->Type() == PCB_GROUP_T )
        {
            newGroup->AddItem( static_cast<GROUP*>( member )->DeepDuplicate() );
        }
        else
        {
            newGroup->AddItem( static_cast<BOARD_ITEM*>( member->Duplicate() ) );
        }
    }

    return newGroup;
}


void GROUP::SwapData( BOARD_ITEM* aImage )
{
    assert( aImage->Type() == PCB_GROUP_T );

    std::swap( *( (GROUP*) this ), *( (GROUP*) aImage ) );
}

#if 0
void GROUP::TransformShapeWithClearanceToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                                                  int aClearanceValue, int aError = ARC_LOW_DEF, bool ignoreLineWidth = false ) const
{
}
const BOX2I GROUP::ViewBBox() const
{
    return GetBoundingBox();
}

#endif

bool GROUP::HitTest( const wxPoint& aPosition, int aAccuracy ) const
{
    EDA_RECT rect = GetBoundingBox();
    return rect.Inflate( aAccuracy ).Contains( aPosition );
}

bool GROUP::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    EDA_RECT arect = aRect;
    arect.Inflate( aAccuracy );

    EDA_RECT bbox = GetBoundingBox();

    if( aContained )
        return arect.Contains( bbox );
    else
    {
        // If the rect does not intersect the bounding box, skip any tests
        if( !aRect.Intersects( bbox ) )
            return false;

        for( BOARD_ITEM* member : m_items )
        {
            if( member->HitTest( arect, false, 0 ) )
                return true;
        }

        // No items were hit
        return false;
    }
}

const EDA_RECT GROUP::GetBoundingBox() const
{
    EDA_RECT area;
    bool     isFirst = true;

    for( BOARD_ITEM* item : m_items )
    {
        if( isFirst )
        {
            area    = item->GetBoundingBox();
            isFirst = false;
        }
        else
        {
            area.Merge( item->GetBoundingBox() );
        }
    }
    area.Inflate( Millimeter2iu( 0.25 ) ); // Give a min size to the area

    return area;
}

SEARCH_RESULT GROUP::Visit( INSPECTOR inspector, void* testData, const KICAD_T scanTypes[] )
{
    for( const KICAD_T* stype = scanTypes; *stype != EOT; ++stype )
    {
        // If caller wants to inspect my type
        if( *stype == Type() )
        {
            if( SEARCH_RESULT::QUIT == inspector( this, testData ) )
                return SEARCH_RESULT::QUIT;
        }
    }

    return SEARCH_RESULT::CONTINUE;
}

LSET GROUP::GetLayerSet() const
{
    LSET aSet;

    for( BOARD_ITEM* item : m_items )
    {
        aSet |= item->GetLayerSet();
    }
    return aSet;
}

void GROUP::ViewGetLayers( int aLayers[], int& aCount ) const
{
    // What layer to put bounding box on?  change in class_group.cpp
    std::unordered_set<int> layers = { LAYER_ANCHOR }; // for bounding box

    for( BOARD_ITEM* item : m_items )
    {
        int member_layers[KIGFX::VIEW::VIEW_MAX_LAYERS], member_layers_count;
        item->ViewGetLayers( member_layers, member_layers_count );

        for( int i = 0; i < member_layers_count; i++ )
            layers.insert( member_layers[i] );
    }

    aCount = layers.size();
    int i  = 0;

    for( int layer : layers )
        aLayers[i++] = layer;
}

unsigned int GROUP::ViewGetLOD( int aLayer, KIGFX::VIEW* aView ) const
{
    if( aView->IsLayerVisible( LAYER_ANCHOR ) )
        return 0;

    return std::numeric_limits<unsigned int>::max();
}

void GROUP::Move( const wxPoint& aMoveVector )
{
    wxPoint newpos = GetPosition() + aMoveVector;
    SetPosition( newpos );
}

void GROUP::Rotate( const wxPoint& aRotCentre, double aAngle )
{
    for( BOARD_ITEM* item : m_items )
    {
        item->Rotate( aRotCentre, aAngle );
    }
}

void GROUP::Flip( const wxPoint& aCentre, bool aFlipLeftRight )
{
    for( BOARD_ITEM* item : m_items )
    {
        item->Flip( aCentre, aFlipLeftRight );
    }
}

wxString GROUP::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    if( m_name.empty() )
    {
        return wxString::Format( _( "Anonymous group %s with %ld members" ), m_Uuid.AsString(), m_items.size() );
    }
    return wxString::Format( _( "Group \"%s\" with %ld members" ), m_name, m_items.size() );
}

BITMAP_DEF GROUP::GetMenuImage() const
{
    return module_xpm;
}

void GROUP::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    aList.emplace_back( _( "Group" ), m_name.empty() ? _( "Anonymous" ) :
                        wxString::Format( _( "\"%s\"" ), m_name ), DARKCYAN );
    aList.emplace_back( _( "Members" ), wxString::Format( _( "%ld" ), m_items.size() ), BROWN );
}

void GROUP::RunOnChildren( const std::function<void( BOARD_ITEM* )>& aFunction )
{
    try
    {
        for( BOARD_ITEM* item : m_items )
            aFunction( item );
    }
    catch( std::bad_function_call& )
    {
        DisplayError( NULL, wxT( "Error running GROUP::RunOnChildren" ) );
    }
}

void GROUP::RunOnDescendants( const std::function<void( BOARD_ITEM* )>& aFunction )
{
    try
    {
        for( BOARD_ITEM* item : m_items )
        {
            aFunction( item );
            if( item->Type() == PCB_GROUP_T )
                static_cast<GROUP*>( item )->RunOnDescendants( aFunction );
        }
    }
    catch( std::bad_function_call& )
    {
        DisplayError( NULL, wxT( "Error running GROUP::RunOnDescendants" ) );
    }
}
