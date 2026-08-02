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

#include <api/api_enums.h>
#include <api/api_utils.h>
#include <api/board/board_types.pb.h>
#include <bitmaps.h>
#include <board.h>
#include <board_design_settings.h>
#include <pcb_griditem.h>
#include <base_units.h>
#include <settings/color_settings.h>
#include <settings/settings_manager.h>
#include <trigo.h>
#include <view/view.h>
#include <i18n_utility.h>
#include <geometry/shape_circle.h>
#include <geometry/shape_line_chain.h>
#include <geometry/geometry_utils.h> // KIGEOM::BoxHitTest
#include <eda_draw_frame.h>
#include <pcb_shape.h>
#include <geometry/shape_null.h>
#include <properties/property_mgr.h>
#include <properties/property.h>

#include <google/protobuf/any.pb.h>

PCB_GRIDITEM::PCB_GRIDITEM( BOARD_ITEM* aParent ) :
        BOARD_ITEM( aParent, PCB_GRIDITEM_T )
{
    m_type = PCB_GRIDITEM_TYPE::CARTESIAN;
    m_extent = VECTOR2I( pcbIUScale.mmToIU( 5 ), pcbIUScale.mmToIU( 5 ) );
    m_spacing = VECTOR2I( pcbIUScale.mmToIU( 1 ), pcbIUScale.mmToIU( 1 ) );
    m_orientation = ANGLE_0;
    m_phiExtent = EDA_ANGLE( 360, DEGREES_T );
    m_phiSpacing = EDA_ANGLE( 10, DEGREES_T );
}


GRID_GEOMETRY PCB_GRIDITEM::AsGridGeometry() const
{
    GRID_GEOMETRY g;
    g.origin = VECTOR2D( m_pos );
    g.orientation = m_orientation.AsRadians();
    g.priority = m_priority;

    switch( m_type )
    {
    case PCB_GRIDITEM_TYPE::POLAR:
        g.kind = GRID_GEOMETRY::KIND::POLAR;
        g.pitch = VECTOR2D( m_spacing.x, m_phiSpacing.AsRadians() );
        g.extent = VECTOR2D( m_extent.x, m_phiExtent.AsRadians() );
        break;

    case PCB_GRIDITEM_TYPE::CARTESIAN:
        g.kind = GRID_GEOMETRY::KIND::CARTESIAN;
        g.pitch = VECTOR2D( m_spacing );
        g.extent = VECTOR2D( m_extent );
        break;

    default: wxFAIL_MSG( wxT( "AsGridGeometry: unhandled PCB_GRIDITEM_TYPE" ) ); break;
    }

    return g;
}


void PCB_GRIDITEM::Serialize( google::protobuf::Any& aContainer ) const
{
    using namespace kiapi::board::types;

    GridItem grid;

    grid.mutable_id()->set_value( m_Uuid.AsStdString() );

    kiapi::common::PackVector2( *grid.mutable_position(), m_pos );
    grid.mutable_orientation()->set_value_degrees( m_orientation.AsDegrees() );

    switch( m_type )
    {
    case PCB_GRIDITEM_TYPE::CARTESIAN:
    {
        CartesianGridItemAttributes* cartesian = grid.mutable_cartesian();

        kiapi::common::PackVector2( *cartesian->mutable_extent(), m_extent );
        kiapi::common::PackVector2( *cartesian->mutable_spacing(), m_spacing );
        break;
    }

    case PCB_GRIDITEM_TYPE::POLAR:
    {
        PolarGridItemAttributes* polar = grid.mutable_polar();

        kiapi::common::PackDistance( *polar->mutable_radius_extent(), m_extent.x );
        kiapi::common::PackDistance( *polar->mutable_radius_spacing(), m_spacing.x );
        polar->mutable_phi_extent()->set_value_degrees( m_phiExtent.AsDegrees() );
        polar->mutable_phi_spacing()->set_value_degrees( m_phiSpacing.AsDegrees() );
        break;
    }

    default: wxFAIL_MSG( wxT( "Serialize: unhandled PCB_GRIDITEM_TYPE" ) ); break;
    }

    grid.set_priority( m_priority );
    grid.set_tick_interval( m_tickInterval );

    grid.mutable_affects()->set_cursor( m_affects.cursor );
    grid.mutable_affects()->set_routing( m_affects.routing );
    grid.mutable_affects()->set_placement( m_affects.placement );

    grid.set_locked( IsLocked() ? kiapi::common::types::LockedState::LS_LOCKED
                                : kiapi::common::types::LockedState::LS_UNLOCKED );

    aContainer.PackFrom( grid );
}


bool PCB_GRIDITEM::Deserialize( const google::protobuf::Any& aContainer )
{
    using namespace kiapi::board::types;

    GridItem grid;

    if( !aContainer.UnpackTo( &grid ) )
        return false;

    SetUuidDirect( KIID( grid.id().value() ) );

    SetPosition( kiapi::common::UnpackVector2( grid.position() ) );
    SetOrientationDegrees( grid.orientation().value_degrees() );

    // setters enforce range sanitation
    if( grid.has_polar() )
    {
        SetGridItemType( PCB_GRIDITEM_TYPE::POLAR );
        SetRadiusExtent( kiapi::common::UnpackDistance( grid.polar().radius_extent() ) );
        SetRadiusSpacing( kiapi::common::UnpackDistance( grid.polar().radius_spacing() ) );
        SetPhiExtentDegrees( grid.polar().phi_extent().value_degrees() );
        SetPhiSpacingDegrees( grid.polar().phi_spacing().value_degrees() );
    }
    else
    {
        SetGridItemType( PCB_GRIDITEM_TYPE::CARTESIAN );
        SetExtent( kiapi::common::UnpackVector2( grid.cartesian().extent() ) );
        SetSpacing( kiapi::common::UnpackVector2( grid.cartesian().spacing() ) );
    }

    SetAssignedPriority( grid.priority() );
    SetTickInterval( grid.tick_interval() );

    if( grid.has_affects() )
    {
        m_affects.cursor = grid.affects().cursor();
        m_affects.routing = grid.affects().routing();
        m_affects.placement = grid.affects().placement();
    }

    SetLocked( grid.locked() == kiapi::common::types::LockedState::LS_LOCKED );

    return true;
}


std::vector<int> PCB_GRIDITEM::ViewGetLayers() const
{
    if( IsLocked() )
        return { LAYER_GRIDITEMS, LAYER_LOCKED_ITEM_SHADOW };

    return { LAYER_GRIDITEMS };
}


double PCB_GRIDITEM::ViewGetLOD( int aLayer, const KIGFX::VIEW* aView ) const
{
    // Hide the locked shadow when grid items themselves are not shown
    if( aLayer == LAYER_LOCKED_ITEM_SHADOW && !aView->IsLayerVisibleCached( LAYER_GRIDITEMS ) )
        return LOD_HIDE;

    return LOD_SHOW;
}


void PCB_GRIDITEM::Rotate( const VECTOR2I& aRotCentre, const EDA_ANGLE& aAngle )
{
    RotatePoint( m_pos, aRotCentre, aAngle );
    m_orientation += aAngle;
    m_orientation.Normalize();
}


void PCB_GRIDITEM::Flip( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection )
{
    if( aFlipDirection == FLIP_DIRECTION::LEFT_RIGHT )
    {
        m_pos.x = aCentre.x - ( m_pos.x - aCentre.x );
        m_orientation = ANGLE_180 - m_orientation;
    }
    else
    {
        m_pos.y = aCentre.y - ( m_pos.y - aCentre.y );
        m_orientation = -m_orientation;
    }

    m_orientation.Normalize();
}


bool PCB_GRIDITEM::HitTest( const VECTOR2I& aPosition, int aAccuracy ) const
{
    // Use the rotated shape (rect/wedge) rather than the axis-aligned bounding box.
    return AsGridGeometry().Contains( VECTOR2D( aPosition ), (double) aAccuracy );
}


SHAPE_LINE_CHAIN PCB_GRIDITEM::buildOutlineWorld() const
{
    SHAPE_LINE_CHAIN outline;

    auto pushWorld = [&]( VECTOR2I aLocal )
    {
        RotatePoint( aLocal, m_orientation );
        outline.Append( m_pos + aLocal );
    };

    switch( m_type )
    {
    case PCB_GRIDITEM_TYPE::POLAR:
    {
        const int    r = m_extent.x;
        const double phiMax = m_phiExtent.AsRadians();

        if( phiMax + 1e-9 < 2 * M_PI )
            pushWorld( VECTOR2I( 0, 0 ) ); // wedge apex

        constexpr int kArcSegments = 32;

        for( int i = 0; i <= kArcSegments; ++i )
        {
            const double phi = phiMax * i / kArcSegments;
            pushWorld( VECTOR2I( KiROUND( r * std::cos( phi ) ), KiROUND( r * std::sin( phi ) ) ) );
        }
        break;
    }

    case PCB_GRIDITEM_TYPE::CARTESIAN:
        pushWorld( VECTOR2I( -m_extent.x, -m_extent.y ) );
        pushWorld( VECTOR2I( m_extent.x, -m_extent.y ) );
        pushWorld( VECTOR2I( m_extent.x, m_extent.y ) );
        pushWorld( VECTOR2I( -m_extent.x, m_extent.y ) );
        break;

    default: wxFAIL_MSG( wxT( "buildOutlineWorld: unhandled PCB_GRIDITEM_TYPE" ) ); break;
    }

    outline.SetClosed( true );
    return outline;
}


bool PCB_GRIDITEM::HitTest( const BOX2I& aRect, bool aContained, int aAccuracy ) const
{
    BOX2I arect = aRect;
    arect.Inflate( aAccuracy );

    const SHAPE_LINE_CHAIN outline = buildOutlineWorld();

    // polygon inside an axis-aligned box  <=>  polygon.BBox inside the box.
    if( aContained )
        return arect.Contains( outline.BBox() );

    return KIGEOM::BoxHitTest( outline, arect, false );
}


const BOX2I PCB_GRIDITEM::GetBoundingBox() const
{
    return buildOutlineWorld().BBox();
}


std::shared_ptr<SHAPE> PCB_GRIDITEM::GetEffectiveShape( PCB_LAYER_ID, FLASHING, DRC_CONSTRAINT_T ) const
{
    // No board geometry - return a null shape.
    return std::make_shared<SHAPE_NULL>();
}


wxString PCB_GRIDITEM::GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const
{
    return _( "Grid Item" );
}


BITMAPS PCB_GRIDITEM::GetMenuImage() const
{
    return BITMAPS::grid_select;
}


EDA_ITEM* PCB_GRIDITEM::Clone() const
{
    return new PCB_GRIDITEM( *this );
}


void PCB_GRIDITEM::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    aList.emplace_back( _( "Grid Item" ), wxEmptyString );

    wxString typeStr;

    switch( GetGridItemType() )
    {
    case PCB_GRIDITEM_TYPE::CARTESIAN: typeStr = wxT( "xy" ); break;
    case PCB_GRIDITEM_TYPE::POLAR: typeStr = wxT( "polar" ); break;
    default: wxFAIL_MSG( wxT( "GetMsgPanelInfo: unhandled PCB_GRIDITEM_TYPE" ) );
    }

    aList.emplace_back( _( "Type" ), typeStr );
}


double PCB_GRIDITEM::Similarity( const BOARD_ITEM& aOther ) const
{
    if( aOther.Type() != Type() )
        return 0.0;

    const PCB_GRIDITEM& other = static_cast<const PCB_GRIDITEM&>( aOther );

    double similarity = 1.0;

    if( m_type != other.m_type )
        similarity *= 0.9;
    if( m_pos != other.m_pos )
        similarity *= 0.9;
    if( m_orientation != other.m_orientation )
        similarity *= 0.9;
    if( m_extent != other.m_extent )
        similarity *= 0.9;
    if( m_spacing != other.m_spacing )
        similarity *= 0.9;
    if( m_phiExtent != other.m_phiExtent )
        similarity *= 0.9;
    if( m_phiSpacing != other.m_phiSpacing )
        similarity *= 0.9;
    if( m_priority != other.m_priority )
        similarity *= 0.9;
    if( m_tickInterval != other.m_tickInterval )
        similarity *= 0.9;
    if( !( m_affects == other.m_affects ) )
        similarity *= 0.9;

    return similarity;
}


bool PCB_GRIDITEM::operator==( const PCB_GRIDITEM& aOther ) const
{
    return m_type == aOther.m_type && m_pos == aOther.m_pos && m_orientation == aOther.m_orientation
           && m_extent == aOther.m_extent && m_spacing == aOther.m_spacing && m_phiExtent == aOther.m_phiExtent
           && m_phiSpacing == aOther.m_phiSpacing && m_priority == aOther.m_priority
           && m_tickInterval == aOther.m_tickInterval && m_affects == aOther.m_affects;
}


bool PCB_GRIDITEM::operator==( const BOARD_ITEM& aOther ) const
{
    if( aOther.Type() != Type() )
        return false;

    return *this == static_cast<const PCB_GRIDITEM&>( aOther );
}


void PCB_GRIDITEM::swapData( BOARD_ITEM* aImage )
{
    assert( aImage->Type() == PCB_GRIDITEM_T );

    std::swap( *( (PCB_GRIDITEM*) this ), *( (PCB_GRIDITEM*) aImage ) );
}


PCB_GRIDITEM* FindActiveGridAt( const BOARD& aBoard, const VECTOR2I& aPos, PCB_GRIDITEM_ROLE aRole )
{
    PCB_GRIDITEM* best = nullptr;
    GRID_GEOMETRY bestGeom;

    for( BOARD_ITEM* item : aBoard.Drawings() )
    {
        if( item->Type() != PCB_GRIDITEM_T )
            continue;

        PCB_GRIDITEM* grid = static_cast<PCB_GRIDITEM*>( item );

        // Selected = edit mode; exclude to avoid self-snap.
        if( grid->IsSelected() )
            continue;

        bool applies = false;

        switch( aRole )
        {
        case PCB_GRIDITEM_ROLE::CURSOR: applies = grid->Affects().cursor; break;
        case PCB_GRIDITEM_ROLE::ROUTING: applies = grid->Affects().routing; break;
        case PCB_GRIDITEM_ROLE::PLACEMENT: applies = grid->Affects().placement; break;
        }

        if( !applies )
            continue;

        if( !grid->HitTestArea( aPos ) )
            continue;

        const GRID_GEOMETRY geom = grid->AsGridGeometry();

        if( !best || geom.TakesPrecedenceOver( bestGeom ) )
        {
            best = grid;
            bestGeom = geom;
        }
    }

    return best;
}


ENUM_TO_WXANY( PCB_GRIDITEM_TYPE );


static struct PCB_GRIDITEM_DESC
{
    PCB_GRIDITEM_DESC()
    {
        ENUM_MAP<PCB_GRIDITEM_TYPE>& gridTypeEnum = ENUM_MAP<PCB_GRIDITEM_TYPE>::Instance();

        if( gridTypeEnum.Choices().GetCount() == 0 )
        {
            gridTypeEnum.Map( PCB_GRIDITEM_TYPE::CARTESIAN, _HKI( "Cartesian" ) )
                    .Map( PCB_GRIDITEM_TYPE::POLAR, _HKI( "Polar" ) );
        }

        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( PCB_GRIDITEM );
        propMgr.InheritsAfter( TYPE_HASH( PCB_GRIDITEM ), TYPE_HASH( BOARD_ITEM ) );

        // Grid items live on every layer; the inherited Layer property is meaningless.
        propMgr.OverrideAvailability( TYPE_HASH( PCB_GRIDITEM ), TYPE_HASH( BOARD_ITEM ), _HKI( "Layer" ),
                                      []( INSPECTABLE* )
                                      {
                                          return false;
                                      } );

        const auto isCartesian = []( INSPECTABLE* aItem ) -> bool
        {
            if( PCB_GRIDITEM* g = dynamic_cast<PCB_GRIDITEM*>( aItem ) )
                return g->GetGridItemType() == PCB_GRIDITEM_TYPE::CARTESIAN;

            return false;
        };

        const auto isPolar = []( INSPECTABLE* aItem ) -> bool
        {
            if( PCB_GRIDITEM* g = dynamic_cast<PCB_GRIDITEM*>( aItem ) )
                return g->GetGridItemType() == PCB_GRIDITEM_TYPE::POLAR;

            return false;
        };

        propMgr.AddProperty( new PROPERTY_ENUM<PCB_GRIDITEM, PCB_GRIDITEM_TYPE>(
                _HKI( "Grid Type" ), &PCB_GRIDITEM::SetGridItemType, &PCB_GRIDITEM::GetGridItemType ) );

        propMgr.AddProperty( new PROPERTY<PCB_GRIDITEM, double>(
                _HKI( "Orientation" ), &PCB_GRIDITEM::SetOrientationDegrees, &PCB_GRIDITEM::GetOrientationDegrees,
                PROPERTY_DISPLAY::PT_DEGREE ) );

        auto extentX = new PROPERTY<PCB_GRIDITEM, int>( _HKI( "Extent X" ), &PCB_GRIDITEM::SetExtentX,
                                                        &PCB_GRIDITEM::GetExtentX, PROPERTY_DISPLAY::PT_SIZE );
        extentX->SetAvailableFunc( isCartesian );
        propMgr.AddProperty( extentX );

        auto extentY = new PROPERTY<PCB_GRIDITEM, int>( _HKI( "Extent Y" ), &PCB_GRIDITEM::SetExtentY,
                                                        &PCB_GRIDITEM::GetExtentY, PROPERTY_DISPLAY::PT_SIZE );
        extentY->SetAvailableFunc( isCartesian );
        propMgr.AddProperty( extentY );

        auto spacingX = new PROPERTY<PCB_GRIDITEM, int>( _HKI( "Spacing X" ), &PCB_GRIDITEM::SetSpacingX,
                                                         &PCB_GRIDITEM::GetSpacingX, PROPERTY_DISPLAY::PT_SIZE );
        spacingX->SetAvailableFunc( isCartesian );
        propMgr.AddProperty( spacingX );

        auto spacingY = new PROPERTY<PCB_GRIDITEM, int>( _HKI( "Spacing Y" ), &PCB_GRIDITEM::SetSpacingY,
                                                         &PCB_GRIDITEM::GetSpacingY, PROPERTY_DISPLAY::PT_SIZE );
        spacingY->SetAvailableFunc( isCartesian );
        propMgr.AddProperty( spacingY );

        auto radiusMax = new PROPERTY<PCB_GRIDITEM, int>( _HKI( "Radius Extent" ), &PCB_GRIDITEM::SetRadiusExtent,
                                                          &PCB_GRIDITEM::GetRadiusExtent, PROPERTY_DISPLAY::PT_SIZE );
        radiusMax->SetAvailableFunc( isPolar );
        propMgr.AddProperty( radiusMax );

        auto radiusStep = new PROPERTY<PCB_GRIDITEM, int>( _HKI( "Radius Spacing" ), &PCB_GRIDITEM::SetRadiusSpacing,
                                                           &PCB_GRIDITEM::GetRadiusSpacing, PROPERTY_DISPLAY::PT_SIZE );
        radiusStep->SetAvailableFunc( isPolar );
        propMgr.AddProperty( radiusStep );

        auto phiMax =
                new PROPERTY<PCB_GRIDITEM, double>( _HKI( "Phi Extent" ), &PCB_GRIDITEM::SetPhiExtentDegrees,
                                                    &PCB_GRIDITEM::GetPhiExtentDegrees, PROPERTY_DISPLAY::PT_DEGREE );
        phiMax->SetAvailableFunc( isPolar );
        propMgr.AddProperty( phiMax );

        auto phiStep =
                new PROPERTY<PCB_GRIDITEM, double>( _HKI( "Phi Spacing" ), &PCB_GRIDITEM::SetPhiSpacingDegrees,
                                                    &PCB_GRIDITEM::GetPhiSpacingDegrees, PROPERTY_DISPLAY::PT_DEGREE );
        phiStep->SetAvailableFunc( isPolar );
        propMgr.AddProperty( phiStep );

        propMgr.AddProperty( new PROPERTY<PCB_GRIDITEM, unsigned>(
                _HKI( "Priority" ), &PCB_GRIDITEM::SetAssignedPriority, &PCB_GRIDITEM::GetAssignedPriority ) );

        propMgr.AddProperty( new PROPERTY<PCB_GRIDITEM, unsigned>(
                _HKI( "Tick Interval" ), &PCB_GRIDITEM::SetTickInterval, &PCB_GRIDITEM::GetTickInterval ) );

        propMgr.AddProperty( new PROPERTY<PCB_GRIDITEM, bool>(
                _HKI( "Affects Cursor Snap" ), &PCB_GRIDITEM::SetAffectsCursor, &PCB_GRIDITEM::GetAffectsCursor ) );

        propMgr.AddProperty( new PROPERTY<PCB_GRIDITEM, bool>(
                _HKI( "Affects Routing" ), &PCB_GRIDITEM::SetAffectsRouting, &PCB_GRIDITEM::GetAffectsRouting ) );

        propMgr.AddProperty( new PROPERTY<PCB_GRIDITEM, bool>(
                _HKI( "Affects Placement" ), &PCB_GRIDITEM::SetAffectsPlacement, &PCB_GRIDITEM::GetAffectsPlacement ) );
    }
} _PCB_GRIDITEM_DESC;
