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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <pcb_drill_map.h>

#include <api/api_enums.h>
#include <api/api_utils.h>
#include <api/board/board_types.pb.h>

#include <base_units.h>
#include <board.h>
#include <board_design_settings.h>
#include <drill/drill_chart_template.h>
#include <drill/drill_enumerator.h>
#include <i18n_utility.h>
#include <properties/property_mgr.h>
#include <view/view.h>
#include <widgets/msgpanel.h>


PCB_DRILL_MAP::PCB_DRILL_MAP( BOARD_ITEM* aParent ) :
        BOARD_ITEM( aParent, PCB_DRILL_MAP_T ),
        m_symbolSize( DRILL_SYMBOL_PROFILE().GetSymbolSize() ),
        m_allSpans( true ),
        m_outlineSlots( true ),
        m_guideCross( false ),
        m_outlineCacheGeneration( 0 )
{
    if( BOARD* board = GetBoard() )
        m_symbolSize = board->GetDesignSettings().GetDrillSymbolProfile().GetSymbolSize();
}


PCB_DRILL_MAP::PCB_DRILL_MAP( const PCB_DRILL_MAP& aOther ) :
        BOARD_ITEM( aOther ),
        m_offset( aOther.m_offset ),
        m_symbolSize( aOther.m_symbolSize ),
        m_span( aOther.m_span ),
        m_allSpans( aOther.m_allSpans ),
        m_outlineSlots( aOther.m_outlineSlots ),
        m_guideCross( aOther.m_guideCross ),
        m_outlineCacheGeneration( 0 )
{
}


void PCB_DRILL_MAP::Rotate( const VECTOR2I& aCentre, const EDA_ANGLE& aAngle )
{
    // The marks sit at their own holes and the offset is a displacement from each one, so
    // there is no geometry here to turn
}


void PCB_DRILL_MAP::Flip( const VECTOR2I& aCentre, FLIP_DIRECTION aDirection )
{
    // The offset is a displacement of every mark from its own hole, so there is nothing to
    // mirror. The side the map documents is still the side it belongs on
    const BOARD* board = GetBoard();

    SetLayer( board ? board->FlipLayer( GetLayer() ) : ::FlipLayer( GetLayer() ) );

}


void PCB_DRILL_MAP::SetSymbolSize( int aSize )
{
    m_symbolSize = std::max( aSize, 1 );
}


int PCB_DRILL_MAP::GetSymbolExtent() const
{
    const BOARD* board = GetBoard();
    const int    width = board ? board->GetDesignSettings().GetDrillSymbolProfile().GetSymbolWidth() : 0;

    // Numeric marks can reach three character sizes either side of their hole.
    return 3 * m_symbolSize + 2 * width;
}


const BOX2I PCB_DRILL_MAP::GetBoundingBox() const
{
    BOX2I box;

    if( const BOARD* board = GetBoard() )
    {
        // Holes and the board outline together, so dragging the map snaps against the edge
        // cuts rather than against whatever hole happens to be outermost
        box = board->DrillSymbolCache()->m_HoleExtent;
        box.Inflate( GetSymbolExtent() );
        box.Merge( board->GetBoardEdgesBoundingBox() );
    }

    box.Move( m_offset );

    return box;
}


std::shared_ptr<const SHAPE_POLY_SET> PCB_DRILL_MAP::GetBoardOutlines() const
{
    std::lock_guard<std::mutex> lock( m_outlineCacheMutex );

    const BOARD* board = GetBoard();

    if( !board )
        return std::make_shared<const SHAPE_POLY_SET>();

    const uint64_t generation = board->GetBoardOutlineGeneration();

    if( m_outlineCache && m_outlineCacheGeneration == generation && m_outlineCacheOffset == m_offset )
        return m_outlineCache;

    std::shared_ptr<SHAPE_POLY_SET> rebuilt = std::make_shared<SHAPE_POLY_SET>();

    if( const_cast<BOARD*>( board )->GetBoardPolygonOutlines( *rebuilt, false ) )
        rebuilt->Move( m_offset );
    else
        rebuilt->RemoveAllContours();

    m_outlineCache = rebuilt;
    m_outlineCacheGeneration = generation;
    m_outlineCacheOffset = m_offset;

    return m_outlineCache;
}


void RefreshDrillMapOutlines( const BOARD& aBoard, KIGFX::VIEW* aView )
{
    if( !aView )
        return;

    for( const BOARD_ITEM* item : aBoard.Drawings() )
    {
        if( item->Type() == PCB_DRILL_MAP_T )
            aView->Update( item, KIGFX::GEOMETRY | KIGFX::REPAINT );
    }
}


bool PCB_DRILL_MAP::HitTest( const VECTOR2I& aPosition, int aAccuracy ) const
{
    const BOARD* board = GetBoard();

    if( !board )
        return false;

    // Only the marks are clickable. The bounding box spans the whole board, so treating it as
    // the hit area would make the map answer for every click on the design.
    const VECTOR2I  where = aPosition - m_offset;
    const int       reach = GetSymbolExtent() / 2 + aAccuracy;
    const std::shared_ptr<const DRILL_SYMBOL_CACHE> cache = board->DrillSymbolCache();

    for( const auto& [itemId, entries] : cache->m_ByItem )
    {
        for( const DRILL_SYMBOL_ENTRY& entry : entries )
        {
            if( !m_allSpans && !( entry.m_Span == m_span ) )
                continue;

            if( std::abs( entry.m_Position.x - where.x ) <= reach
                && std::abs( entry.m_Position.y - where.y ) <= reach )
            {
                return true;
            }
        }
    }

    // The outline is drawn and plotted as the map's own artwork, so a map on a board with an
    // outline but no holes would otherwise be visible and unselectable
    return GetBoardOutlines()->PointOnEdge( aPosition, aAccuracy );
}


bool PCB_DRILL_MAP::HitTest( const BOX2I& aRect, bool aContained, int aAccuracy ) const
{
    BOX2I rect = aRect;
    rect.Inflate( aAccuracy );

    if( aContained )
        return rect.Contains( GetBoundingBox() );

    return rect.Intersects( GetBoundingBox() );
}


std::vector<int> PCB_DRILL_MAP::ViewGetLayers() const
{
    // Its own layer only. The selection outline is drawn there, in the selected colour, and
    // the overlay would draw it a second time
    return { GetLayer() };
}


wxString PCB_DRILL_MAP::GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const
{
    return wxString::Format( _( "Drill Map on %s" ), GetLayerName() );
}


void PCB_DRILL_MAP::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    aList.emplace_back( _( "Drill Map" ), wxEmptyString );
    aList.emplace_back( _( "Layer" ), GetLayerName() );
    aList.emplace_back( _( "Spans" ), m_allSpans ? _( "All" ) : _( "Single" ) );
}


void PCB_DRILL_MAP::swapData( BOARD_ITEM* aImage )
{
    wxCHECK_RET( aImage && aImage->Type() == Type(), wxT( "Cannot swap data with invalid map." ) );

    PCB_DRILL_MAP* other = static_cast<PCB_DRILL_MAP*>( aImage );

    std::swap( m_layer, other->m_layer );
    std::swap( m_isLocked, other->m_isLocked );
    std::swap( m_offset, other->m_offset );
    std::swap( m_symbolSize, other->m_symbolSize );
    std::swap( m_span, other->m_span );
    std::swap( m_allSpans, other->m_allSpans );
    std::swap( m_outlineSlots, other->m_outlineSlots );
    std::swap( m_guideCross, other->m_guideCross );
}


double PCB_DRILL_MAP::Similarity( const BOARD_ITEM& aOther ) const
{
    if( aOther.Type() != Type() )
        return 0.0;

    const PCB_DRILL_MAP& other = static_cast<const PCB_DRILL_MAP&>( aOther );

    return other.GetLayer() == GetLayer() ? 1.0 : 0.5;
}


bool PCB_DRILL_MAP::operator==( const BOARD_ITEM& aOther ) const
{
    if( aOther.Type() != Type() )
        return false;

    const PCB_DRILL_MAP& other = static_cast<const PCB_DRILL_MAP&>( aOther );

    // m_span included. Without it the merge driver treats a retargeted map as unchanged
    return m_offset == other.m_offset && m_symbolSize == other.m_symbolSize && m_allSpans == other.m_allSpans
           && m_span == other.m_span
           && m_outlineSlots == other.m_outlineSlots && m_guideCross == other.m_guideCross
           && GetLayer() == other.GetLayer();
}


void PCB_DRILL_MAP::Serialize( google::protobuf::Any& aContainer ) const
{
    using namespace kiapi::board;
    types::DrillMap map;

    map.mutable_id()->set_value( m_Uuid.AsStdString() );
    map.set_layer( ToProtoEnum<PCB_LAYER_ID, types::BoardLayer>( GetLayer() ) );
    kiapi::common::PackVector2( *map.mutable_position(), GetPosition() );
    map.set_locked( IsLocked() ? kiapi::common::types::LockedState::LS_LOCKED
                               : kiapi::common::types::LockedState::LS_UNLOCKED );

    map.set_all_spans( m_allSpans );

    types::DrillSpan* span = map.mutable_span();
    span->set_start_layer( ToProtoEnum<PCB_LAYER_ID, types::BoardLayer>( m_span.DrillStartLayer() ) );
    span->set_end_layer( ToProtoEnum<PCB_LAYER_ID, types::BoardLayer>( m_span.DrillEndLayer() ) );
    span->set_is_backdrill( m_span.m_IsBackdrill );
    span->set_is_non_plated( m_span.m_IsNonPlatedFile );

    map.set_outline_slots( m_outlineSlots );
    map.set_guide_cross( m_guideCross );
    kiapi::common::PackDistance( *map.mutable_symbol_size(), m_symbolSize );

    aContainer.PackFrom( map );
}


bool PCB_DRILL_MAP::Deserialize( const google::protobuf::Any& aContainer )
{
    using namespace kiapi::board;
    types::DrillMap map;

    if( !aContainer.UnpackTo( &map ) )
    {
        return false;
    }

    const PCB_LAYER_ID layer = FromProtoEnum<PCB_LAYER_ID>( map.layer() );

    // Copper, silkscreen, mask, paste, adhesive, Edge.Cuts, Margin and courtyard are all
    // manufacturing inputs that hole symbols would corrupt rather than document
    if( !DrillDocumentationLayers().Contains( layer ) )
    {
        return false;
    }

    SetUuidDirect( KIID( map.id().value() ) );
    SetLayer( layer );
    SetPosition( kiapi::common::UnpackVector2( map.position() ) );
    SetLocked( map.locked() == kiapi::common::types::LockedState::LS_LOCKED );

    m_allSpans = map.all_spans();

    if( map.has_span() )
    {
        m_span = DRILL_SPAN( FromProtoEnum<PCB_LAYER_ID>( map.span().start_layer() ),
                             FromProtoEnum<PCB_LAYER_ID>( map.span().end_layer() ),
                             map.span().is_backdrill(), map.span().is_non_plated() );
    }

    m_outlineSlots = map.outline_slots();
    m_guideCross = map.guide_cross();

    if( map.has_symbol_size() && map.symbol_size().value_nm() > 0 )
        SetSymbolSize( kiapi::common::UnpackDistance( map.symbol_size() ) );

    return true;
}


int PCB_DRILL_MAP::GetSpanChoice() const
{
    if( m_allSpans || !GetBoard() )
        return -1;

    const std::vector<DRILL_SPAN> spans = EnumerateDrillSpans( *GetBoard() );
    const auto it = std::find( spans.begin(), spans.end(), m_span );

    // A span the stackup no longer has reads as every span rather than as an empty map
    if( it == spans.end() )
        return -1;

    return static_cast<int>( it - spans.begin() );
}


void PCB_DRILL_MAP::SetSpanChoice( int aIndex )
{
    if( aIndex < 0 || !GetBoard() )
    {
        m_allSpans = true;
        return;
    }

    const std::vector<DRILL_SPAN> spans = EnumerateDrillSpans( *GetBoard() );

    if( aIndex >= static_cast<int>( spans.size() ) )
    {
        m_allSpans = true;
        return;
    }

    m_allSpans = false;
    m_span = spans[aIndex];
}


static struct PCB_DRILL_MAP_DESC
{
    PCB_DRILL_MAP_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( PCB_DRILL_MAP );

        propMgr.AddTypeCast( new TYPE_CAST<PCB_DRILL_MAP, BOARD_ITEM> );
        propMgr.InheritsAfter( TYPE_HASH( PCB_DRILL_MAP ), TYPE_HASH( BOARD_ITEM ) );

        const wxString mapProps = _( "Drill Map Properties" );

        // Relative, not absolute. This is how far the marks are slid from their holes, so the
        // drawing origin has nothing to say about it
        propMgr.AddProperty( new PROPERTY<PCB_DRILL_MAP, int>( _HKI( "Offset X" ),
                    &PCB_DRILL_MAP::SetOffsetX, &PCB_DRILL_MAP::GetOffsetX,
                    PROPERTY_DISPLAY::PT_COORD, ORIGIN_TRANSFORMS::REL_X_COORD ),
                    mapProps );

        propMgr.AddProperty( new PROPERTY<PCB_DRILL_MAP, int>( _HKI( "Offset Y" ),
                    &PCB_DRILL_MAP::SetOffsetY, &PCB_DRILL_MAP::GetOffsetY,
                    PROPERTY_DISPLAY::PT_COORD, ORIGIN_TRANSFORMS::REL_Y_COORD ),
                    mapProps );

        propMgr.AddProperty( new PROPERTY<PCB_DRILL_MAP, int>( _HKI( "Symbol Size" ),
                    &PCB_DRILL_MAP::SetSymbolSize, &PCB_DRILL_MAP::GetSymbolSize,
                    PROPERTY_DISPLAY::PT_SIZE ),
                    mapProps );

        propMgr.Mask( TYPE_HASH( PCB_DRILL_MAP ), TYPE_HASH( BOARD_ITEM ), _HKI( "Position X" ) );
        propMgr.Mask( TYPE_HASH( PCB_DRILL_MAP ), TYPE_HASH( BOARD_ITEM ), _HKI( "Position Y" ) );

        // Choices come from the board's stackup, so they are filled in by the properties panel
        propMgr.AddProperty( new PROPERTY_ENUM<PCB_DRILL_MAP, int>( _HKI( "Hole Span" ),
                    &PCB_DRILL_MAP::SetSpanChoice, &PCB_DRILL_MAP::GetSpanChoice ),
                    mapProps );

        propMgr.AddProperty( new PROPERTY<PCB_DRILL_MAP, bool>( _HKI( "Outline Slots" ),
                    &PCB_DRILL_MAP::SetOutlineSlots, &PCB_DRILL_MAP::GetOutlineSlots ),
                    mapProps );

        propMgr.AddProperty( new PROPERTY<PCB_DRILL_MAP, bool>( _HKI( "Guide Cross" ),
                    &PCB_DRILL_MAP::SetGuideCross, &PCB_DRILL_MAP::GetGuideCross ),
                    mapProps );
    }
} _PCB_DRILL_MAP_DESC;
