/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
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
#include <core/mirror.h>
#include <macros.h>
#include <pcb_edit_frame.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <base_units.h>
#include <geometry/shape_compound.h>
#include <pcb_shape.h>
#include <pcb_painter.h>

PCB_SHAPE::PCB_SHAPE( BOARD_ITEM* aParent, KICAD_T aItemType, SHAPE_T aShapeType ) :
    BOARD_ITEM( aParent, aItemType ),
    EDA_SHAPE( aShapeType, pcbIUScale.mmToIU( DEFAULT_LINE_WIDTH ), FILL_T::NO_FILL )
{
}


PCB_SHAPE::PCB_SHAPE( BOARD_ITEM* aParent, SHAPE_T shapetype ) :
    BOARD_ITEM( aParent, PCB_SHAPE_T ),
    EDA_SHAPE( shapetype, pcbIUScale.mmToIU( DEFAULT_LINE_WIDTH ), FILL_T::NO_FILL )
{
}


PCB_SHAPE::~PCB_SHAPE()
{
}


bool PCB_SHAPE::IsType( const std::vector<KICAD_T>& aScanTypes ) const
{
    if( BOARD_ITEM::IsType( aScanTypes ) )
        return true;

    bool sametype = false;

    for( KICAD_T scanType : aScanTypes )
    {
        if( scanType == PCB_LOCATE_BOARD_EDGE_T )
            sametype = m_layer == Edge_Cuts;
        else if( scanType == PCB_SHAPE_LOCATE_ARC_T )
            sametype = m_shape == SHAPE_T::ARC;
        else if( scanType == PCB_SHAPE_LOCATE_CIRCLE_T )
            sametype = m_shape == SHAPE_T::CIRCLE;
        else if( scanType == PCB_SHAPE_LOCATE_RECT_T )
            sametype = m_shape == SHAPE_T::RECT;
        else if( scanType == PCB_SHAPE_LOCATE_SEGMENT_T )
            sametype = m_shape == SHAPE_T::SEGMENT;
        else if( scanType == PCB_SHAPE_LOCATE_POLY_T )
            sametype = m_shape == SHAPE_T::POLY;
        else if( scanType == PCB_SHAPE_LOCATE_BEZIER_T )
            sametype = m_shape == SHAPE_T::BEZIER;

        if( sametype )
            return true;
    }

    return false;
}


const VECTOR2I PCB_SHAPE::GetFocusPosition() const
{
    // For some shapes return the visual center, but for not filled polygonal shapes,
    // the center is usually far from the shape: a point on the outline is better

    switch( m_shape )
    {
    case SHAPE_T::CIRCLE:
        if( !IsFilled() )
            return VECTOR2I( GetCenter().x + GetRadius(), GetCenter().y );
        else
            return GetCenter();

    case SHAPE_T::RECT:
        if( !IsFilled() )
            return GetStart();
        else
            return GetCenter();

    case SHAPE_T::POLY:
        if( !IsFilled() )
        {
            VECTOR2I pos = GetPolyShape().Outline(0).CPoint(0);
            return VECTOR2I( pos.x, pos.y );
        }
        else
        {
            return GetCenter();
        }

    case SHAPE_T::ARC:
        return GetArcMid();

    case SHAPE_T::BEZIER:
        return GetStart();

    default:
        return GetCenter();
    }
}


std::vector<VECTOR2I> PCB_SHAPE::GetCorners() const
{
    std::vector<VECTOR2I> pts;

    if( GetShape() == SHAPE_T::RECT )
    {
        pts = GetRectCorners();
    }
    else if( GetShape() == SHAPE_T::POLY )
    {
        for( int ii = 0; ii < GetPolyShape().OutlineCount(); ++ii )
        {
            for( const VECTOR2I& pt : GetPolyShape().Outline( ii ).CPoints() )
                pts.emplace_back( pt );
        }
    }
    else
    {
        UNIMPLEMENTED_FOR( SHAPE_T_asString() );
    }

    while( pts.size() < 4 )
        pts.emplace_back( pts.back() + VECTOR2I( 10, 10 ) );

    return pts;
}


void PCB_SHAPE::Move( const VECTOR2I& aMoveVector )
{
    move( aMoveVector );
}


void PCB_SHAPE::Scale( double aScale )
{
    scale( aScale );
}


void PCB_SHAPE::NormalizeRect()
{
    if( m_shape == SHAPE_T::RECT )
    {
        VECTOR2I start = GetStart();
        VECTOR2I end = GetEnd();

        BOX2I rect( start, end - start );
        rect.Normalize();

        SetStart( rect.GetPosition() );
        SetEnd( rect.GetEnd() );
    }
}


void PCB_SHAPE::Rotate( const VECTOR2I& aRotCentre, const EDA_ANGLE& aAngle )
{
    rotate( aRotCentre, aAngle );
}


void PCB_SHAPE::Flip( const VECTOR2I& aCentre, bool aFlipLeftRight )
{
    flip( aCentre, aFlipLeftRight );

    SetLayer( FlipLayer( GetLayer(), GetBoard()->GetCopperLayerCount() ) );
}


void PCB_SHAPE::Mirror( const VECTOR2I& aCentre, bool aMirrorAroundXAxis )
{
    // Mirror an edge of the footprint. the layer is not modified
    // This is a footprint shape modification.

    switch( GetShape() )
    {
    case SHAPE_T::ARC:
    case SHAPE_T::SEGMENT:
    case SHAPE_T::RECT:
    case SHAPE_T::CIRCLE:
    case SHAPE_T::BEZIER:
        if( aMirrorAroundXAxis )
        {
            MIRROR( m_start.y, aCentre.y );
            MIRROR( m_end.y, aCentre.y );
            MIRROR( m_arcCenter.y, aCentre.y );
            MIRROR( m_bezierC1.y, aCentre.y );
            MIRROR( m_bezierC2.y, aCentre.y );
        }
        else
        {
            MIRROR( m_start.x, aCentre.x );
            MIRROR( m_end.x, aCentre.x );
            MIRROR( m_arcCenter.x, aCentre.x );
            MIRROR( m_bezierC1.x, aCentre.x );
            MIRROR( m_bezierC2.x, aCentre.x );
        }

        if( GetShape() == SHAPE_T::ARC )
            std::swap( m_start, m_end );

        if( GetShape() == SHAPE_T::BEZIER )
            RebuildBezierToSegmentsPointsList( GetWidth() );

        break;

    case SHAPE_T::POLY:
        m_poly.Mirror( !aMirrorAroundXAxis, aMirrorAroundXAxis, aCentre );
        break;

    default:
        UNIMPLEMENTED_FOR( SHAPE_T_asString() );
    }
}


double PCB_SHAPE::ViewGetLOD( int aLayer, KIGFX::VIEW* aView ) const
{
    constexpr double HIDE = std::numeric_limits<double>::max();
    constexpr double SHOW = 0.0;

    KIGFX::PCB_PAINTER*  painter = static_cast<KIGFX::PCB_PAINTER*>( aView->GetPainter() );
    KIGFX::PCB_RENDER_SETTINGS* renderSettings = painter->GetSettings();

    if( aLayer == LAYER_LOCKED_ITEM_SHADOW )
    {
        // Hide shadow if the main layer is not shown
        if( !aView->IsLayerVisible( m_layer ) )
            return HIDE;

        // Hide shadow on dimmed tracks
        if( renderSettings->GetHighContrast() )
        {
            if( m_layer != renderSettings->GetPrimaryHighContrastLayer() )
                return HIDE;
        }
    }

    if( FOOTPRINT* parent = GetParentFootprint() )
    {
        if( parent->GetLayer() == F_Cu && !aView->IsLayerVisible( LAYER_MOD_FR ) )
            return HIDE;

        if( parent->GetLayer() == B_Cu && !aView->IsLayerVisible( LAYER_MOD_BK ) )
            return HIDE;
    }

    return SHOW;
}


void PCB_SHAPE::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    if( aFrame->GetName() == PCB_EDIT_FRAME_NAME )
    {
        if( FOOTPRINT* parent = GetParentFootprint() )
            aList.emplace_back( _( "Footprint" ), parent->GetReference() );
    }

    aList.emplace_back( _( "Type" ), _( "Drawing" ) );

    if( aFrame->GetName() == PCB_EDIT_FRAME_NAME && IsLocked() )
        aList.emplace_back( _( "Status" ), _( "Locked" ) );

    ShapeGetMsgPanelInfo( aFrame, aList );

    aList.emplace_back( _( "Layer" ), GetLayerName() );
}


wxString PCB_SHAPE::GetItemDescription( UNITS_PROVIDER* aUnitsProvider ) const
{
    return wxString::Format( _( "%s on %s" ), ShowShape(), GetLayerName() );
}


BITMAPS PCB_SHAPE::GetMenuImage() const
{
    if( GetParentFootprint() )
        return BITMAPS::show_mod_edge;
    else
        return BITMAPS::add_dashed_line;
}


EDA_ITEM* PCB_SHAPE::Clone() const
{
    return new PCB_SHAPE( *this );
}


const BOX2I PCB_SHAPE::ViewBBox() const
{
    BOX2I return_box = EDA_ITEM::ViewBBox();

    // Inflate the bounding box by just a bit more for safety.
    return_box.Inflate( GetWidth() );

    return return_box;
}


std::shared_ptr<SHAPE> PCB_SHAPE::GetEffectiveShape( PCB_LAYER_ID aLayer, FLASHING aFlash ) const
{
    return std::make_shared<SHAPE_COMPOUND>( MakeEffectiveShapes() );
}


void PCB_SHAPE::swapData( BOARD_ITEM* aImage )
{
    PCB_SHAPE* image = dynamic_cast<PCB_SHAPE*>( aImage );
    assert( image );

    SwapShape( image );

    // Swap params not handled by SwapShape( image )
    std::swap( m_layer, image->m_layer );
    std::swap( m_isKnockout, image->m_isKnockout );
    std::swap( m_isLocked, image->m_isLocked );
    std::swap( m_flags, image->m_flags );
    std::swap( m_parent, image->m_parent );
    std::swap( m_forceVisible, image->m_forceVisible );
}


bool PCB_SHAPE::cmp_drawings::operator()( const BOARD_ITEM* aFirst,
                                          const BOARD_ITEM* aSecond ) const
{
    if( aFirst->Type() != aSecond->Type() )
        return aFirst->Type() < aSecond->Type();

    if( aFirst->GetLayer() != aSecond->GetLayer() )
        return aFirst->GetLayer() < aSecond->GetLayer();

    if( aFirst->Type() == PCB_SHAPE_T )
    {
        const PCB_SHAPE* dwgA = static_cast<const PCB_SHAPE*>( aFirst );
        const PCB_SHAPE* dwgB = static_cast<const PCB_SHAPE*>( aSecond );

        if( dwgA->GetShape() != dwgB->GetShape() )
            return dwgA->GetShape() < dwgB->GetShape();
    }

    return aFirst->m_Uuid < aSecond->m_Uuid;
}


void PCB_SHAPE::TransformShapeToPolygon( SHAPE_POLY_SET& aBuffer, PCB_LAYER_ID aLayer,
                                         int aClearance, int aError, ERROR_LOC aErrorLoc,
                                         bool ignoreLineWidth ) const
{
    EDA_SHAPE::TransformShapeToPolygon( aBuffer, aClearance, aError, aErrorLoc, ignoreLineWidth );
}


static struct PCB_SHAPE_DESC
{
    PCB_SHAPE_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( PCB_SHAPE );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_SHAPE, BOARD_ITEM> );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_SHAPE, EDA_SHAPE> );
        propMgr.InheritsAfter( TYPE_HASH( PCB_SHAPE ), TYPE_HASH( BOARD_ITEM ) );
        propMgr.InheritsAfter( TYPE_HASH( PCB_SHAPE ), TYPE_HASH( EDA_SHAPE ) );

        auto layerProperty = new PROPERTY_ENUM<PCB_SHAPE, PCB_LAYER_ID, BOARD_ITEM>(
                _HKI( "Layer" ), &PCB_SHAPE::SetLayer, &PCB_SHAPE::GetLayer );

        propMgr.ReplaceProperty( TYPE_HASH( BOARD_ITEM ), _HKI( "Layer" ), layerProperty );

        // Only polygons have meaningful Position properties.
        // On other shapes, these are duplicates of the Start properties.
        auto isPolygon =
                []( INSPECTABLE* aItem ) -> bool
                {
                    if( PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( aItem ) )
                        return shape->GetShape() == SHAPE_T::POLY;

                    return false;
                };

        propMgr.OverrideAvailability( TYPE_HASH( PCB_SHAPE ), TYPE_HASH( BOARD_ITEM ),
                                      _HKI( "Position X" ), isPolygon );
        propMgr.OverrideAvailability( TYPE_HASH( PCB_SHAPE ), TYPE_HASH( BOARD_ITEM ),
                                      _HKI( "Position Y" ), isPolygon );
    }
} _PCB_SHAPE_DESC;
