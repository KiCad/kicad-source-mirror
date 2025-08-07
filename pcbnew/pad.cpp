/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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

#include <base_units.h>
#include <bitmaps.h>
#include <math/util.h>      // for KiROUND
#include <eda_draw_frame.h>
#include <geometry/shape_circle.h>
#include <geometry/shape_segment.h>
#include <geometry/shape_simple.h>
#include <geometry/shape_rect.h>
#include <geometry/shape_compound.h>
#include <geometry/shape_null.h>
#include <geometry/geometry_utils.h>
#include <layer_range.h>
#include <string_utils.h>
#include <i18n_utility.h>
#include <view/view.h>
#include <board.h>
#include <board_connected_item.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <lset.h>
#include <pad.h>
#include <pad_utils.h>
#include <pcb_shape.h>
#include <connectivity/connectivity_data.h>
#include <eda_units.h>
#include <convert_basic_shapes_to_polygon.h>
#include <widgets/msgpanel.h>
#include <pcb_painter.h>
#include <properties/property_validators.h>
#include <wx/log.h>
#include <api/api_enums.h>
#include <api/api_utils.h>
#include <api/api_pcb_utils.h>
#include <api/board/board_types.pb.h>

#include <memory>
#include <macros.h>
#include <magic_enum.hpp>
#include <drc/drc_item.h>
#include "kiface_base.h"
#include "pcbnew_settings.h"

#include <pcb_group.h>
#include <gal/graphics_abstraction_layer.h>
#include <pin_type.h>

using KIGFX::PCB_PAINTER;
using KIGFX::PCB_RENDER_SETTINGS;


PAD::PAD( FOOTPRINT* parent ) :
    BOARD_CONNECTED_ITEM( parent, PCB_PAD_T ),
    m_padStack( this )
{
    VECTOR2I& drill = m_padStack.Drill().size;
    m_padStack.SetSize( { EDA_UNIT_UTILS::Mils2IU( pcbIUScale, 60 ),
                          EDA_UNIT_UTILS::Mils2IU( pcbIUScale, 60 ) },
                        PADSTACK::ALL_LAYERS );
    drill.x = drill.y = EDA_UNIT_UTILS::Mils2IU( pcbIUScale, 30 );       // Default drill size 30 mils.
    m_lengthPadToDie      = 0;
    m_delayPadToDie = 0;

    if( m_parent && m_parent->Type() == PCB_FOOTPRINT_T )
        m_pos = GetParent()->GetPosition();

    SetShape( F_Cu, PAD_SHAPE::CIRCLE );          // Default pad shape is PAD_CIRCLE.
    SetAnchorPadShape( F_Cu, PAD_SHAPE::CIRCLE ); // Default shape for custom shaped pads
                                                  // is PAD_CIRCLE.
    SetDrillShape( PAD_DRILL_SHAPE::CIRCLE );     // Default pad drill shape is a circle.
    m_attribute           = PAD_ATTRIB::PTH;      // Default pad type is plated through hole
    SetProperty( PAD_PROP::NONE );                // no special fabrication property

    // Parameters for round rect only:
    m_padStack.SetRoundRectRadiusRatio( 0.25, F_Cu );  // from IPC-7351C standard

    // Parameters for chamfered rect only:
    m_padStack.SetChamferRatio( 0.2, F_Cu );
    m_padStack.SetChamferPositions( RECT_NO_CHAMFER, F_Cu );

    // Set layers mask to default for a standard thru hole pad.
    m_padStack.SetLayerSet( PTHMask() );

    SetSubRatsnest( 0 );                       // used in ratsnest calculations

    SetDirty();
    m_effectiveBoundingRadius = 0;

    for( PCB_LAYER_ID layer : LAYER_RANGE( F_Cu, B_Cu, BoardCopperLayerCount() ) )
        m_zoneLayerOverrides[layer] = ZLO_NONE;

    m_lastGalZoomLevel = 0.0;
}


PAD::PAD( const PAD& aOther ) :
    BOARD_CONNECTED_ITEM( aOther.GetParent(), PCB_PAD_T ),
    m_padStack( this )
{
    PAD::operator=( aOther );

    const_cast<KIID&>( m_Uuid ) = aOther.m_Uuid;
}


PAD& PAD::operator=( const PAD &aOther )
{
    BOARD_CONNECTED_ITEM::operator=( aOther );

    ImportSettingsFrom( aOther );
    SetPadToDieLength( aOther.GetPadToDieLength() );
    SetPadToDieDelay( aOther.GetPadToDieDelay() );
    SetPosition( aOther.GetPosition() );
    SetNumber( aOther.GetNumber() );
    SetPinType( aOther.GetPinType() );
    SetPinFunction( aOther.GetPinFunction() );
    SetSubRatsnest( aOther.GetSubRatsnest() );
    m_effectiveBoundingRadius = aOther.m_effectiveBoundingRadius;

    return *this;
}


void PAD::CopyFrom( const BOARD_ITEM* aOther )
{
    wxCHECK( aOther && aOther->Type() == PCB_PAD_T, /* void */ );
    *this = *static_cast<const PAD*>( aOther );
}


void PAD::Serialize( google::protobuf::Any &aContainer ) const
{
    using namespace kiapi::board::types;
    Pad pad;

    pad.mutable_id()->set_value( m_Uuid.AsStdString() );
    kiapi::common::PackVector2( *pad.mutable_position(), GetPosition() );
    pad.set_locked( IsLocked() ? kiapi::common::types::LockedState::LS_LOCKED
                               : kiapi::common::types::LockedState::LS_UNLOCKED );
    PackNet( pad.mutable_net() );
    pad.set_number( GetNumber().ToUTF8() );
    pad.set_type( ToProtoEnum<PAD_ATTRIB, PadType>( GetAttribute() ) );
    pad.mutable_pad_to_die_length()->set_value_nm( GetPadToDieLength() );
    pad.mutable_pad_to_die_delay()->set_value_as( GetPadToDieDelay() );

    google::protobuf::Any padStackMsg;
    m_padStack.Serialize( padStackMsg );
    padStackMsg.UnpackTo( pad.mutable_pad_stack() );

    if( GetLocalClearance().has_value() )
        pad.mutable_copper_clearance_override()->set_value_nm( *GetLocalClearance() );

    aContainer.PackFrom( pad );
}


bool PAD::Deserialize( const google::protobuf::Any &aContainer )
{
    kiapi::board::types::Pad pad;

    if( !aContainer.UnpackTo( &pad ) )
        return false;

    const_cast<KIID&>( m_Uuid ) = KIID( pad.id().value() );
    SetPosition( kiapi::common::UnpackVector2( pad.position() ) );
    UnpackNet( pad.net() );
    SetLocked( pad.locked() == kiapi::common::types::LockedState::LS_LOCKED );
    SetAttribute( FromProtoEnum<PAD_ATTRIB>( pad.type() ) );
    SetNumber( wxString::FromUTF8( pad.number() ) );
    SetPadToDieLength( pad.pad_to_die_length().value_nm() );
    SetPadToDieDelay( pad.pad_to_die_delay().value_as() );

    google::protobuf::Any padStackWrapper;
    padStackWrapper.PackFrom( pad.pad_stack() );
    m_padStack.Deserialize( padStackWrapper );

    SetLayer( m_padStack.StartLayer() );

    if( pad.has_copper_clearance_override() )
        SetLocalClearance( pad.copper_clearance_override().value_nm() );
    else
        SetLocalClearance( std::nullopt );

    return true;
}


void PAD::ClearZoneLayerOverrides()
{
    std::unique_lock<std::mutex> cacheLock( m_zoneLayerOverridesMutex );

    for( PCB_LAYER_ID layer : LAYER_RANGE( F_Cu, B_Cu, BoardCopperLayerCount() ) )
        m_zoneLayerOverrides[layer] = ZLO_NONE;
}


const ZONE_LAYER_OVERRIDE& PAD::GetZoneLayerOverride( PCB_LAYER_ID aLayer ) const
{
    std::unique_lock<std::mutex> cacheLock( m_zoneLayerOverridesMutex );

    static const ZONE_LAYER_OVERRIDE defaultOverride = ZLO_NONE;
    auto it = m_zoneLayerOverrides.find( aLayer );
    return it != m_zoneLayerOverrides.end() ? it->second : defaultOverride;
}


void PAD::SetZoneLayerOverride( PCB_LAYER_ID aLayer, ZONE_LAYER_OVERRIDE aOverride )
{
    std::unique_lock<std::mutex> cacheLock( m_zoneLayerOverridesMutex );
    m_zoneLayerOverrides[aLayer] = aOverride;
}


bool PAD::CanHaveNumber() const
{
    // Aperture pads don't get a number
    if( IsAperturePad() )
        return false;

    // NPTH pads don't get numbers
    if( GetAttribute() == PAD_ATTRIB::NPTH )
        return false;

    return true;
}


bool PAD::IsLocked() const
{
    if( GetParent() && GetParent()->IsLocked() )
        return true;

    return BOARD_ITEM::IsLocked();
};


bool PAD::SharesNetTieGroup( const PAD* aOther ) const
{
    FOOTPRINT* parentFp = GetParentFootprint();

    if( parentFp && parentFp->IsNetTie() && aOther->GetParentFootprint() == parentFp )
    {
        std::map<wxString, int> padToNetTieGroupMap = parentFp->MapPadNumbersToNetTieGroups();
        int thisNetTieGroup = padToNetTieGroupMap[ GetNumber() ];
        int otherNetTieGroup = padToNetTieGroupMap[ aOther->GetNumber() ];

        return thisNetTieGroup >= 0 && thisNetTieGroup == otherNetTieGroup;
    }

    return false;
}


bool PAD::IsNoConnectPad() const
{
    return m_pinType.Contains( wxT( "no_connect" ) );
}


bool PAD::IsFreePad() const
{
    return GetShortNetname().StartsWith( wxT( "unconnected-(" ) )
            && m_pinType == wxT( "free" );
}


LSET PAD::PTHMask()
{
    static LSET saved = LSET::AllCuMask() | LSET( { F_Mask, B_Mask } );
    return saved;
}


LSET PAD::SMDMask()
{
    static LSET saved( { F_Cu, F_Paste, F_Mask } );
    return saved;
}


LSET PAD::ConnSMDMask()
{
    static LSET saved( { F_Cu, F_Mask } );
    return saved;
}


LSET PAD::UnplatedHoleMask()
{
    static LSET saved = LSET( { F_Cu, B_Cu, F_Mask, B_Mask } );
    return saved;
}


LSET PAD::ApertureMask()
{
    static LSET saved( { F_Paste } );
    return saved;
}


bool PAD::IsFlipped() const
{
    FOOTPRINT* parent = GetParentFootprint();

    return ( parent && parent->GetLayer() == B_Cu );
}


PCB_LAYER_ID PAD::GetLayer() const
{
    return BOARD_ITEM::GetLayer();
}


PCB_LAYER_ID PAD::GetPrincipalLayer() const
{
    if( m_attribute == PAD_ATTRIB::SMD || m_attribute == PAD_ATTRIB::CONN || GetLayerSet().none() )
        return m_layer;
    else
        return GetLayerSet().Seq().front();

}


bool PAD::FlashLayer( const LSET& aLayers ) const
{
    for( PCB_LAYER_ID layer : aLayers )
    {
        if( FlashLayer( layer ) )
            return true;
    }

    return false;
}


bool PAD::FlashLayer( int aLayer, bool aOnlyCheckIfPermitted ) const
{
    if( aLayer == UNDEFINED_LAYER )
        return true;

    // Sometimes this is called with GAL layers and should just return true
    if( aLayer > PCB_LAYER_ID_COUNT )
        return true;

    PCB_LAYER_ID layer = static_cast<PCB_LAYER_ID>( aLayer );

    if( !IsOnLayer( layer ) )
        return false;

    if( GetAttribute() == PAD_ATTRIB::NPTH && IsCopperLayer( aLayer ) )
    {
        if( GetShape( layer ) == PAD_SHAPE::CIRCLE && GetDrillShape() == PAD_DRILL_SHAPE::CIRCLE )
        {
            if( GetOffset( layer ) == VECTOR2I( 0, 0 ) && GetDrillSize().x >= GetSize( layer ).x )
                return false;
        }
        else if( GetShape( layer ) == PAD_SHAPE::OVAL
                 && GetDrillShape() == PAD_DRILL_SHAPE::OBLONG )
        {
            if( GetOffset( layer ) == VECTOR2I( 0, 0 )
                    && GetDrillSize().x >= GetSize( layer ).x
                    && GetDrillSize().y >= GetSize( layer ).y )
            {
                return false;
            }
        }
    }

    if( LSET::FrontBoardTechMask().test( aLayer ) )
        aLayer = F_Cu;
    else if( LSET::BackBoardTechMask().test( aLayer ) )
        aLayer = B_Cu;

    if( GetAttribute() == PAD_ATTRIB::PTH && IsCopperLayer( aLayer ) )
    {
        PADSTACK::UNCONNECTED_LAYER_MODE mode = m_padStack.UnconnectedLayerMode();

        if( mode == PADSTACK::UNCONNECTED_LAYER_MODE::KEEP_ALL )
            return true;

        // Plated through hole pads need copper on the top/bottom layers for proper soldering
        // Unless the user has removed them in the pad dialog
        if( mode == PADSTACK::UNCONNECTED_LAYER_MODE::START_END_ONLY )
        {
            return aLayer == m_padStack.Drill().start || aLayer == m_padStack.Drill().end;
        }

        if( mode == PADSTACK::UNCONNECTED_LAYER_MODE::REMOVE_EXCEPT_START_AND_END
            && IsExternalCopperLayer( aLayer ) )
        {
            return true;
        }

        if( const BOARD* board = GetBoard() )
        {
            if( GetZoneLayerOverride( layer ) == ZLO_FORCE_FLASHED )
            {
                return true;
            }
            else if( aOnlyCheckIfPermitted )
            {
                return true;
            }
            else
            {
                // Must be static to keep from raising its ugly head in performance profiles
                static std::initializer_list<KICAD_T> nonZoneTypes = { PCB_TRACE_T, PCB_ARC_T,
                                                                       PCB_VIA_T, PCB_PAD_T };

                return board->GetConnectivity()->IsConnectedOnLayer( this, aLayer, nonZoneTypes );
            }
        }
    }

    return true;
}


void PAD::SetDrillSizeX( const int aX )
{
    m_padStack.Drill().size.x = aX;

    if( GetDrillShape() == PAD_DRILL_SHAPE::CIRCLE )
        SetDrillSizeY( aX );

    SetDirty();
}


void PAD::SetDrillShape( PAD_DRILL_SHAPE aShape )
{
    m_padStack.Drill().shape = aShape;

    if( aShape == PAD_DRILL_SHAPE::CIRCLE )
        SetDrillSizeY( GetDrillSizeX() );

    m_shapesDirty = true;
}


int PAD::GetRoundRectCornerRadius( PCB_LAYER_ID aLayer ) const
{
    return m_padStack.RoundRectRadius( aLayer );
}


void PAD::SetRoundRectCornerRadius( PCB_LAYER_ID aLayer, double aRadius )
{
    m_padStack.SetRoundRectRadius( aRadius, aLayer );
}


void PAD::SetRoundRectRadiusRatio( PCB_LAYER_ID aLayer, double aRadiusScale )
{
    m_padStack.SetRoundRectRadiusRatio( std::clamp( aRadiusScale, 0.0, 0.5 ), aLayer );

    SetDirty();
}


void PAD::SetFrontRoundRectRadiusRatio( double aRadiusScale )
{
    wxASSERT_MSG( m_padStack.Mode() == PADSTACK::MODE::NORMAL,
                  "Set front radius only meaningful for normal padstacks" );

    m_padStack.SetRoundRectRadiusRatio( std::clamp( aRadiusScale, 0.0, 0.5 ), F_Cu );
    SetDirty();
}


void PAD::SetFrontRoundRectRadiusSize( int aRadius )
{
    const VECTOR2I size = m_padStack.Size( F_Cu );
    const int      minSize = std::min( size.x, size.y );
    const double   newRatio = aRadius / double( minSize );

    SetFrontRoundRectRadiusRatio( newRatio );
}


int PAD::GetFrontRoundRectRadiusSize() const
{
    const VECTOR2I size = m_padStack.Size( F_Cu );
    const int      minSize = std::min( size.x, size.y );
    const double   ratio = GetFrontRoundRectRadiusRatio();

    return KiROUND( ratio * minSize );
}


void PAD::SetChamferRectRatio( PCB_LAYER_ID aLayer, double aChamferScale )
{
    m_padStack.SetChamferRatio( aChamferScale, aLayer );

    SetDirty();
}


const std::shared_ptr<SHAPE_POLY_SET>& PAD::GetEffectivePolygon( PCB_LAYER_ID aLayer,
                                                                 ERROR_LOC aErrorLoc ) const
{
    if( m_polyDirty[ aErrorLoc ] )
        BuildEffectivePolygon( aErrorLoc );

    aLayer = Padstack().EffectiveLayerFor( aLayer );

    return m_effectivePolygons[ aLayer ][ aErrorLoc ];
}


std::shared_ptr<SHAPE> PAD::GetEffectiveShape( PCB_LAYER_ID aLayer, FLASHING flashPTHPads ) const
{
    if( aLayer == Edge_Cuts )
    {
        std::shared_ptr<SHAPE_COMPOUND> effective_compund = std::make_shared<SHAPE_COMPOUND>();

        if( GetAttribute() == PAD_ATTRIB::PTH || GetAttribute() == PAD_ATTRIB::NPTH )
        {
            effective_compund->AddShape( GetEffectiveHoleShape() );
            return effective_compund;
        }
        else
        {
            effective_compund->AddShape( std::make_shared<SHAPE_NULL>() );
            return effective_compund;
        }
    }

    if( GetAttribute() == PAD_ATTRIB::PTH )
    {
        bool flash;
        std::shared_ptr<SHAPE_COMPOUND> effective_compund = std::make_shared<SHAPE_COMPOUND>();

        if( flashPTHPads == FLASHING::NEVER_FLASHED )
            flash = false;
        else if( flashPTHPads == FLASHING::ALWAYS_FLASHED )
            flash = true;
        else
            flash = FlashLayer( aLayer );

        if( !flash )
        {
            if( GetAttribute() == PAD_ATTRIB::PTH )
            {
                effective_compund->AddShape( GetEffectiveHoleShape() );
                return effective_compund;
            }
            else
            {
                effective_compund->AddShape( std::make_shared<SHAPE_NULL>() );
                return effective_compund;
            }
        }
    }

    if( m_shapesDirty )
        BuildEffectiveShapes();

    aLayer = Padstack().EffectiveLayerFor( aLayer );

    wxCHECK_MSG( m_effectiveShapes.contains( aLayer ), nullptr,
                 wxString::Format( wxT( "Missing shape in PAD::GetEffectiveShape for layer %s." ),
                                   magic_enum::enum_name( aLayer ) ) );
    wxCHECK_MSG( m_effectiveShapes.at( aLayer ), nullptr,
                 wxString::Format( wxT( "Null shape in PAD::GetEffectiveShape for layer %s." ),
                                   magic_enum::enum_name( aLayer ) ) );

    return m_effectiveShapes[aLayer];
}


std::shared_ptr<SHAPE_SEGMENT> PAD::GetEffectiveHoleShape() const
{
    if( m_shapesDirty )
        BuildEffectiveShapes();

    return m_effectiveHoleShape;
}


int PAD::GetBoundingRadius() const
{
    if( m_polyDirty[ ERROR_OUTSIDE ] )
        BuildEffectivePolygon( ERROR_OUTSIDE );

    return m_effectiveBoundingRadius;
}


void PAD::BuildEffectiveShapes() const
{
    std::lock_guard<std::mutex> RAII_lock( m_shapesBuildingLock );

    // If we had to wait for the lock then we were probably waiting for someone else to
    // finish rebuilding the shapes.  So check to see if they're clean now.
    if( !m_shapesDirty )
        return;

    m_effectiveBoundingBox = BOX2I();

    Padstack().ForEachUniqueLayer(
            [&]( PCB_LAYER_ID aLayer )
            {
                const SHAPE_COMPOUND& layerShape = buildEffectiveShape( aLayer );
                m_effectiveBoundingBox.Merge( layerShape.BBox() );
            } );

    // Hole shape
    m_effectiveHoleShape = nullptr;

    VECTOR2I half_size = m_padStack.Drill().size / 2;
    int      half_width;
    VECTOR2I half_len;

    if( m_padStack.Drill().shape == PAD_DRILL_SHAPE::CIRCLE )
    {
        half_width = half_size.x;
    }
    else
    {
        half_width = std::min( half_size.x, half_size.y );
        half_len = VECTOR2I( half_size.x - half_width, half_size.y - half_width );
    }

    RotatePoint( half_len, GetOrientation() );

    m_effectiveHoleShape = std::make_shared<SHAPE_SEGMENT>( m_pos - half_len, m_pos + half_len,
                                                            half_width * 2 );
    m_effectiveBoundingBox.Merge( m_effectiveHoleShape->BBox() );

    // All done
    m_shapesDirty = false;
}


const SHAPE_COMPOUND& PAD::buildEffectiveShape( PCB_LAYER_ID aLayer ) const
{
    m_effectiveShapes[aLayer] = std::make_shared<SHAPE_COMPOUND>();

    auto add = [this, aLayer]( SHAPE* aShape )
               {
                   m_effectiveShapes[aLayer]->AddShape( aShape );
               };

    VECTOR2I  shapePos = ShapePos( aLayer ); // Fetch only once; rotation involves trig
    PAD_SHAPE effectiveShape = GetShape( aLayer );
    const VECTOR2I& size = m_padStack.Size( aLayer );

    if( effectiveShape == PAD_SHAPE::CUSTOM )
        effectiveShape = GetAnchorPadShape( aLayer );

    switch( effectiveShape )
    {
    case PAD_SHAPE::CIRCLE:
        add( new SHAPE_CIRCLE( shapePos, size.x / 2 ) );
        break;

    case PAD_SHAPE::OVAL:
        if( size.x == size.y ) // the oval pad is in fact a circle
        {
            add( new SHAPE_CIRCLE( shapePos, size.x / 2 ) );
        }
        else
        {
            VECTOR2I half_size = size / 2;
            int     half_width = std::min( half_size.x, half_size.y );
            VECTOR2I half_len( half_size.x - half_width, half_size.y - half_width );
            RotatePoint( half_len, GetOrientation() );
            add( new SHAPE_SEGMENT( shapePos - half_len, shapePos + half_len, half_width * 2 ) );
        }

        break;

    case PAD_SHAPE::RECTANGLE:
    case PAD_SHAPE::TRAPEZOID:
    case PAD_SHAPE::ROUNDRECT:
    {
        int r = ( effectiveShape == PAD_SHAPE::ROUNDRECT ) ? GetRoundRectCornerRadius( aLayer ) : 0;
        VECTOR2I half_size( size.x / 2, size.y / 2 );
        VECTOR2I trap_delta( 0, 0 );

        if( r )
        {
            half_size -= VECTOR2I( r, r );

            // Avoid degenerated shapes (0 length segments) that always create issues
            // For roundrect pad very near a circle, use only a circle
            const int min_len = pcbIUScale.mmToIU( 0.0001 );

            if( half_size.x < min_len && half_size.y < min_len )
            {
                add( new SHAPE_CIRCLE( shapePos, r ) );
                break;
            }
        }
        else if( effectiveShape == PAD_SHAPE::TRAPEZOID )
        {
            trap_delta = m_padStack.TrapezoidDeltaSize( aLayer ) / 2;
        }

        SHAPE_LINE_CHAIN corners;

        corners.Append( -half_size.x - trap_delta.y,  half_size.y + trap_delta.x );
        corners.Append(  half_size.x + trap_delta.y,  half_size.y - trap_delta.x );
        corners.Append(  half_size.x - trap_delta.y, -half_size.y + trap_delta.x );
        corners.Append( -half_size.x + trap_delta.y, -half_size.y - trap_delta.x );

        corners.Rotate( GetOrientation() );
        corners.Move( shapePos );

        // GAL renders rectangles faster than 4-point polygons so it's worth checking if our
        // body shape is a rectangle.
        if( corners.PointCount() == 4
                &&
                ( ( corners.CPoint( 0 ).y == corners.CPoint( 1 ).y
                    && corners.CPoint( 1 ).x == corners.CPoint( 2 ).x
                    && corners.CPoint( 2 ).y == corners.CPoint( 3 ).y
                    && corners.CPoint( 3 ).x == corners.CPoint( 0 ).x )
                    ||
                ( corners.CPoint( 0 ).x == corners.CPoint( 1 ).x
                    && corners.CPoint( 1 ).y == corners.CPoint( 2 ).y
                    && corners.CPoint( 2 ).x == corners.CPoint( 3 ).x
                    && corners.CPoint( 3 ).y == corners.CPoint( 0 ).y )
                )
            )
        {
            int width  = std::abs( corners.CPoint( 2 ).x - corners.CPoint( 0 ).x );
            int height = std::abs( corners.CPoint( 2 ).y - corners.CPoint( 0 ).y );
            VECTOR2I pos( std::min( corners.CPoint( 2 ).x, corners.CPoint( 0 ).x ),
                          std::min( corners.CPoint( 2 ).y, corners.CPoint( 0 ).y ) );

            add( new SHAPE_RECT( pos, width, height ) );
        }
        else
        {
            add( new SHAPE_SIMPLE( corners ) );
        }

        if( r )
        {
            add( new SHAPE_SEGMENT( corners.CPoint( 0 ), corners.CPoint( 1 ), r * 2 ) );
            add( new SHAPE_SEGMENT( corners.CPoint( 1 ), corners.CPoint( 2 ), r * 2 ) );
            add( new SHAPE_SEGMENT( corners.CPoint( 2 ), corners.CPoint( 3 ), r * 2 ) );
            add( new SHAPE_SEGMENT( corners.CPoint( 3 ), corners.CPoint( 0 ), r * 2 ) );
        }
    }
        break;

    case PAD_SHAPE::CHAMFERED_RECT:
    {
        SHAPE_POLY_SET outline;

        TransformRoundChamferedRectToPolygon( outline, shapePos, GetSize( aLayer ),
                                              GetOrientation(), GetRoundRectCornerRadius( aLayer ),
                                              GetChamferRectRatio( aLayer ),
                                              GetChamferPositions( aLayer ), 0, GetMaxError(),
                                              ERROR_INSIDE );

        add( new SHAPE_SIMPLE( outline.COutline( 0 ) ) );
    }
        break;

    default:
        wxFAIL_MSG( wxT( "PAD::buildEffectiveShapes: Unsupported pad shape: PAD_SHAPE::" )
                    + wxString( std::string( magic_enum::enum_name( effectiveShape ) ) ) );
        break;
    }

    if( GetShape( aLayer ) == PAD_SHAPE::CUSTOM )
    {
        for( const std::shared_ptr<PCB_SHAPE>& primitive : m_padStack.Primitives( aLayer ) )
        {
            if( !primitive->IsProxyItem() )
            {
                for( SHAPE* shape : primitive->MakeEffectiveShapes() )
                {
                    shape->Rotate( GetOrientation() );
                    shape->Move( shapePos );
                    add( shape );
                }
            }
        }
    }

    return *m_effectiveShapes[aLayer];
}


void PAD::BuildEffectivePolygon( ERROR_LOC aErrorLoc ) const
{
    std::lock_guard<std::mutex> RAII_lock( m_polyBuildingLock );

    // Only calculate this once, not for both ERROR_INSIDE and ERROR_OUTSIDE
    bool doBoundingRadius = aErrorLoc == ERROR_OUTSIDE;

    // If we had to wait for the lock then we were probably waiting for someone else to
    // finish rebuilding the shapes.  So check to see if they're clean now.
    if( !m_polyDirty[ aErrorLoc ] )
        return;

    Padstack().ForEachUniqueLayer(
        [&]( PCB_LAYER_ID aLayer )
        {
            // Polygon
            std::shared_ptr<SHAPE_POLY_SET>& effectivePolygon = m_effectivePolygons[ aLayer ][ aErrorLoc ];

            effectivePolygon = std::make_shared<SHAPE_POLY_SET>();
            TransformShapeToPolygon( *effectivePolygon, aLayer, 0, GetMaxError(), aErrorLoc );
        } );

    if( doBoundingRadius )
    {
        m_effectiveBoundingRadius = 0;

        Padstack().ForEachUniqueLayer(
            [&]( PCB_LAYER_ID aLayer )
            {
                std::shared_ptr<SHAPE_POLY_SET>& effectivePolygon = m_effectivePolygons[ aLayer ][ aErrorLoc ];

                for( int cnt = 0; cnt < effectivePolygon->OutlineCount(); ++cnt )
                {
                    const SHAPE_LINE_CHAIN& poly = effectivePolygon->COutline( cnt );

                    for( int ii = 0; ii < poly.PointCount(); ++ii )
                    {
                        int dist = KiROUND( ( poly.CPoint( ii ) - m_pos ).EuclideanNorm() );
                        m_effectiveBoundingRadius = std::max( m_effectiveBoundingRadius, dist );
                    }
                }
            } );

        m_effectiveBoundingRadius = std::max( m_effectiveBoundingRadius, KiROUND( GetDrillSizeX() / 2.0 ) );
        m_effectiveBoundingRadius = std::max( m_effectiveBoundingRadius, KiROUND( GetDrillSizeY() / 2.0 ) );
    }

    // All done
    m_polyDirty[ aErrorLoc ] = false;
}


const BOX2I PAD::GetBoundingBox() const
{
    if( m_shapesDirty )
        BuildEffectiveShapes();

    return m_effectiveBoundingBox;
}


// Thermal spokes are built on the bounding box, so we must have a layer-specific version
const BOX2I PAD::GetBoundingBox( PCB_LAYER_ID aLayer ) const
{
    return buildEffectiveShape( aLayer ).BBox();
}


void PAD::SetAttribute( PAD_ATTRIB aAttribute )
{
    if( m_attribute != aAttribute )
    {
        m_attribute = aAttribute;

        LSET& layerMask = m_padStack.LayerSet();

        switch( aAttribute )
        {
        case PAD_ATTRIB::PTH:
            // Plump up to all copper layers
            layerMask |= LSET::AllCuMask();
            break;

        case PAD_ATTRIB::SMD:
        case PAD_ATTRIB::CONN:
        {
            // Trim down to no more than one copper layer
            LSET copperLayers = layerMask & LSET::AllCuMask();

            if( copperLayers.count() > 1 )
            {
                layerMask &= ~LSET::AllCuMask();

                if( copperLayers.test( B_Cu ) )
                    layerMask.set( B_Cu );
                else
                    layerMask.set( copperLayers.Seq().front() );
            }

            // No hole
            m_padStack.Drill().size = VECTOR2I( 0, 0 );
            break;
        }

        case PAD_ATTRIB::NPTH:
            // No number; no net
            m_number = wxEmptyString;
            SetNetCode( NETINFO_LIST::UNCONNECTED );
            break;
        }
    }

    SetDirty();
}


void PAD::SetFrontShape( PAD_SHAPE aShape )
{
    const bool wasRoundable = PAD_UTILS::PadHasMeaningfulRoundingRadius( *this, F_Cu );

    m_padStack.SetShape( aShape, F_Cu );

    const bool isRoundable = PAD_UTILS::PadHasMeaningfulRoundingRadius( *this, F_Cu );

    // If we have become roundable, set a sensible rounding default using the IPC rules.
    if( !wasRoundable && isRoundable )
    {
        const double ipcRadiusRatio = PAD_UTILS::GetDefaultIpcRoundingRatio( *this, F_Cu );
        m_padStack.SetRoundRectRadiusRatio( ipcRadiusRatio, F_Cu );
    }

    SetDirty();
}


void PAD::SetProperty( PAD_PROP aProperty )
{
    m_property = aProperty;

    SetDirty();
}


void PAD::SetOrientation( const EDA_ANGLE& aAngle )
{
    m_padStack.SetOrientation( aAngle );
    SetDirty();
}


void PAD::SetFPRelativeOrientation( const EDA_ANGLE& aAngle )
{
    if( FOOTPRINT* parentFP = GetParentFootprint() )
        SetOrientation( aAngle + parentFP->GetOrientation() );
    else
        SetOrientation( aAngle );
}


EDA_ANGLE PAD::GetFPRelativeOrientation() const
{
    if( FOOTPRINT* parentFP = GetParentFootprint() )
        return GetOrientation() - parentFP->GetOrientation();
    else
        return GetOrientation();
}


void PAD::Flip( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection )
{
    MIRROR( m_pos, aCentre, aFlipDirection );

    m_padStack.ForEachUniqueLayer(
        [&]( PCB_LAYER_ID aLayer )
        {
            MIRROR( m_padStack.Offset( aLayer ), VECTOR2I{ 0, 0 }, aFlipDirection );
            MIRROR( m_padStack.TrapezoidDeltaSize( aLayer ), VECTOR2I{ 0, 0 }, aFlipDirection );
        } );

    SetFPRelativeOrientation( -GetFPRelativeOrientation() );

    auto mirrorBitFlags = []( int& aBitfield, int a, int b )
                          {
                              bool temp = aBitfield & a;

                              if( aBitfield & b )
                                  aBitfield |= a;
                              else
                                  aBitfield &= ~a;

                              if( temp )
                                  aBitfield |= b;
                              else
                                  aBitfield &= ~b;
                          };

    Padstack().ForEachUniqueLayer(
        [&]( PCB_LAYER_ID aLayer )
        {
            if( aFlipDirection == FLIP_DIRECTION::LEFT_RIGHT )
            {
                mirrorBitFlags( m_padStack.ChamferPositions( aLayer ), RECT_CHAMFER_TOP_LEFT,
                                RECT_CHAMFER_TOP_RIGHT );
                mirrorBitFlags( m_padStack.ChamferPositions( aLayer ), RECT_CHAMFER_BOTTOM_LEFT,
                                RECT_CHAMFER_BOTTOM_RIGHT );
            }
            else
            {
                mirrorBitFlags( m_padStack.ChamferPositions( aLayer ), RECT_CHAMFER_TOP_LEFT,
                                RECT_CHAMFER_BOTTOM_LEFT );
                mirrorBitFlags( m_padStack.ChamferPositions( aLayer ), RECT_CHAMFER_TOP_RIGHT,
                                RECT_CHAMFER_BOTTOM_RIGHT );
            }
        } );

    // Flip padstack geometry
    int copperLayerCount = BoardCopperLayerCount();

    m_padStack.FlipLayers( copperLayerCount );

    // Flip pads layers after padstack geometry
    LSET flipped;

    for( PCB_LAYER_ID layer : m_padStack.LayerSet() )
        flipped.set( GetBoard()->FlipLayer( layer ) );

    SetLayerSet( flipped );

    // Flip the basic shapes, in custom pads
    FlipPrimitives( aFlipDirection );

    SetDirty();
}


void PAD::FlipPrimitives( FLIP_DIRECTION aFlipDirection )
{
    Padstack().ForEachUniqueLayer(
        [&]( PCB_LAYER_ID aLayer )
        {
            for( std::shared_ptr<PCB_SHAPE>& primitive : m_padStack.Primitives( aLayer ) )
            {
                // Ensure the primitive parent is up to date. Flip uses GetBoard() that
                // imply primitive parent is valid
                primitive->SetParent(this);
                primitive->Flip( VECTOR2I( 0, 0 ), aFlipDirection );
            }
        } );

    SetDirty();
}


VECTOR2I PAD::ShapePos( PCB_LAYER_ID aLayer ) const
{
    VECTOR2I loc_offset = m_padStack.Offset( aLayer );

    if( loc_offset.x == 0 && loc_offset.y == 0 )
        return m_pos;

    RotatePoint( loc_offset, GetOrientation() );

    VECTOR2I shape_pos = m_pos + loc_offset;

    return shape_pos;
}


bool PAD::IsOnCopperLayer() const
{
    if( GetAttribute() == PAD_ATTRIB::NPTH )
    {
        // NPTH pads have no plated hole cylinder.  If their annular ring size is 0 or
        // negative, then they have no annular ring either.
        bool hasAnnularRing = true;

        Padstack().ForEachUniqueLayer(
            [&]( PCB_LAYER_ID aLayer )
            {
                switch( GetShape( aLayer ) )
                {
                case PAD_SHAPE::CIRCLE:
                    if( m_padStack.Offset( aLayer ) == VECTOR2I( 0, 0 )
                        && m_padStack.Size( aLayer ).x <= m_padStack.Drill().size.x )
                    {
                        hasAnnularRing = false;
                    }

                    break;

                case PAD_SHAPE::OVAL:
                    if( m_padStack.Offset( aLayer ) == VECTOR2I( 0, 0 )
                        && m_padStack.Size( aLayer ).x <= m_padStack.Drill().size.x
                        && m_padStack.Size( aLayer ).y <= m_padStack.Drill().size.y )
                    {
                        hasAnnularRing = false;
                    }

                    break;

                default:
                    // We could subtract the hole polygon from the shape polygon for these, but it
                    // would be expensive and we're probably well out of the common use cases....
                    break;
                }
            } );

            if( !hasAnnularRing )
                return false;
    }

    return ( GetLayerSet() & LSET::AllCuMask() ).any();
}


std::optional<int> PAD::GetLocalClearance( wxString* aSource ) const
{
    if( m_padStack.Clearance().has_value() && aSource )
        *aSource = _( "pad" );

    return m_padStack.Clearance();
}


std::optional<int> PAD::GetClearanceOverrides( wxString* aSource ) const
{
    if( m_padStack.Clearance().has_value() )
        return GetLocalClearance( aSource );

    if( FOOTPRINT* parentFootprint = GetParentFootprint() )
        return parentFootprint->GetClearanceOverrides( aSource );

    return std::optional<int>();
}


int PAD::GetOwnClearance( PCB_LAYER_ID aLayer, wxString* aSource ) const
{
    DRC_CONSTRAINT c;

    if( GetBoard() && GetBoard()->GetDesignSettings().m_DRCEngine )
    {
        BOARD_DESIGN_SETTINGS& bds = GetBoard()->GetDesignSettings();

        if( GetAttribute() == PAD_ATTRIB::NPTH )
            c = bds.m_DRCEngine->EvalRules( HOLE_CLEARANCE_CONSTRAINT, this, nullptr, aLayer );
        else
            c = bds.m_DRCEngine->EvalRules( CLEARANCE_CONSTRAINT, this, nullptr, aLayer );
    }

    if( c.Value().HasMin() )
    {
        if( aSource )
            *aSource = c.GetName();

        return c.Value().Min();
    }

    return 0;
}


int PAD::GetSolderMaskExpansion( PCB_LAYER_ID aLayer ) const
{
    // Pads defined only on mask layers (and perhaps on other tech layers) use the shape
    // defined by the pad settings only.  ALL other pads, even those that don't actually have
    // any copper (such as NPTH pads with holes the same size as the pad) get mask expansion.
    if( ( m_padStack.LayerSet() & LSET::AllCuMask() ).none() )
        return 0;

    if( IsFrontLayer( aLayer ) )
        aLayer = F_Mask;
    else if( IsBackLayer( aLayer ) )
        aLayer = B_Mask;
    else
        return 0;

    std::optional<int> margin;

    if( GetBoard() && GetBoard()->GetDesignSettings().m_DRCEngine )
    {
        DRC_CONSTRAINT              constraint;
        std::shared_ptr<DRC_ENGINE> drcEngine = GetBoard()->GetDesignSettings().m_DRCEngine;

        constraint = drcEngine->EvalRules( SOLDER_MASK_EXPANSION_CONSTRAINT, this, nullptr, aLayer );

        if( constraint.m_Value.HasOpt() )
            margin = constraint.m_Value.Opt();
    }
    else
    {
        margin = m_padStack.SolderMaskMargin( aLayer );

        if( !margin.has_value() )
        {
            if( FOOTPRINT* parentFootprint = GetParentFootprint() )
                margin = parentFootprint->GetLocalSolderMaskMargin();
        }
    }

    int marginValue = margin.value_or( 0 );

    PCB_LAYER_ID cuLayer = ( aLayer == B_Mask ) ? B_Cu : F_Cu;

    // ensure mask have a size always >= 0
    if( marginValue < 0 )
    {
        int minsize = -std::min( m_padStack.Size( cuLayer ).x, m_padStack.Size( cuLayer ).y ) / 2;

        if( marginValue < minsize )
            marginValue = minsize;
    }

    return marginValue;
}


VECTOR2I PAD::GetSolderPasteMargin( PCB_LAYER_ID aLayer ) const
{
    // Pads defined only on mask layers (and perhaps on other tech layers) use the shape
    // defined by the pad settings only.  ALL other pads, even those that don't actually have
    // any copper (such as NPTH pads with holes the same size as the pad) get paste expansion.
    if( ( m_padStack.LayerSet() & LSET::AllCuMask() ).none() )
        return VECTOR2I( 0, 0 );

    if( IsFrontLayer( aLayer ) )
        aLayer = F_Paste;
    else if( IsBackLayer( aLayer ) )
        aLayer = B_Paste;
    else
        return VECTOR2I( 0, 0 );

    std::optional<int>    margin;
    std::optional<double> mratio;

    if( GetBoard() && GetBoard()->GetDesignSettings().m_DRCEngine )
    {
        DRC_CONSTRAINT              constraint;
        std::shared_ptr<DRC_ENGINE> drcEngine = GetBoard()->GetDesignSettings().m_DRCEngine;

        constraint = drcEngine->EvalRules( SOLDER_PASTE_ABS_MARGIN_CONSTRAINT, this, nullptr, aLayer );

        if( constraint.m_Value.HasOpt() )
            margin = constraint.m_Value.Opt();

        constraint = drcEngine->EvalRules( SOLDER_PASTE_REL_MARGIN_CONSTRAINT, this, nullptr, aLayer );

        if( constraint.m_Value.HasOpt() )
            mratio = constraint.m_Value.Opt() / 1000.0;
    }
    else
    {
        margin = m_padStack.SolderPasteMargin( aLayer );
        mratio = m_padStack.SolderPasteMarginRatio( aLayer );

        if( !margin.has_value() )
        {
            if( FOOTPRINT* parentFootprint = GetParentFootprint() )
                margin = parentFootprint->GetLocalSolderPasteMargin();
        }

        if( !mratio.has_value() )
        {
            if( FOOTPRINT* parentFootprint = GetParentFootprint() )
                mratio = parentFootprint->GetLocalSolderPasteMarginRatio();
        }
    }

    PCB_LAYER_ID cuLayer = ( aLayer == B_Paste ) ? B_Cu : F_Cu;
    VECTOR2I padSize = m_padStack.Size( cuLayer );

    VECTOR2I pad_margin;
    pad_margin.x = margin.value_or( 0 ) + KiROUND( padSize.x * mratio.value_or( 0 ) );
    pad_margin.y = margin.value_or( 0 ) + KiROUND( padSize.y * mratio.value_or( 0 ) );

    // ensure paste have a size always >= 0
    if( m_padStack.Shape( aLayer ) != PAD_SHAPE::CUSTOM )
    {
        if( pad_margin.x < -padSize.x / 2 )
            pad_margin.x = -padSize.x / 2;

        if( pad_margin.y < -padSize.y / 2 )
            pad_margin.y = -padSize.y / 2;
    }

    return pad_margin;
}


ZONE_CONNECTION PAD::GetZoneConnectionOverrides( wxString* aSource ) const
{
    ZONE_CONNECTION connection = m_padStack.ZoneConnection().value_or( ZONE_CONNECTION::INHERITED );

    if( connection != ZONE_CONNECTION::INHERITED )
    {
        if( aSource )
            *aSource = _( "pad" );
    }

    if( connection == ZONE_CONNECTION::INHERITED )
    {
        if( FOOTPRINT* parentFootprint = GetParentFootprint() )
            connection = parentFootprint->GetZoneConnectionOverrides( aSource );
    }

    return connection;
}


int PAD::GetLocalSpokeWidthOverride( wxString* aSource ) const
{
    if( m_padStack.ThermalSpokeWidth().has_value() && aSource )
        *aSource = _( "pad" );

    return m_padStack.ThermalSpokeWidth().value_or( 0 );
}


int PAD::GetLocalThermalGapOverride( wxString* aSource ) const
{
    if( m_padStack.ThermalGap().has_value() && aSource )
        *aSource = _( "pad" );

    return GetLocalThermalGapOverride().value_or( 0 );
}


void PAD::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    wxString   msg;
    FOOTPRINT* parentFootprint = static_cast<FOOTPRINT*>( m_parent );

    if( aFrame->GetName() == PCB_EDIT_FRAME_NAME )
    {
        if( parentFootprint )
            aList.emplace_back( _( "Footprint" ), parentFootprint->GetReference() );
    }

    aList.emplace_back( _( "Pad" ), m_number );

    if( !GetPinFunction().IsEmpty() )
        aList.emplace_back( _( "Pin Name" ), GetPinFunction() );

    if( !GetPinType().IsEmpty() )
        aList.emplace_back( _( "Pin Type" ), GetPinType() );

    if( aFrame->GetName() == PCB_EDIT_FRAME_NAME )
    {
        aList.emplace_back( _( "Net" ), UnescapeString( GetNetname() ) );

        aList.emplace_back( _( "Resolved Netclass" ),
                            UnescapeString( GetEffectiveNetClass()->GetHumanReadableName() ) );

        if( IsLocked() )
            aList.emplace_back( _( "Status" ), _( "Locked" ) );
    }

    if( GetAttribute() == PAD_ATTRIB::SMD || GetAttribute() == PAD_ATTRIB::CONN )
        aList.emplace_back( _( "Layer" ), layerMaskDescribe() );

    if( aFrame->GetName() == FOOTPRINT_EDIT_FRAME_NAME )
    {
        if( GetAttribute() == PAD_ATTRIB::SMD )
        {
            // TOOD(JE) padstacks
            const std::shared_ptr<SHAPE_POLY_SET>& poly = GetEffectivePolygon( PADSTACK::ALL_LAYERS );
            double area = poly->Area();

            aList.emplace_back( _( "Area" ), aFrame->MessageTextFromValue( area, true, EDA_DATA_TYPE::AREA ) );
        }
    }

    // Show the pad shape, attribute and property
    wxString props = ShowPadAttr();

    if( GetProperty() != PAD_PROP::NONE )
        props += ',';

    switch( GetProperty() )
    {
    case PAD_PROP::NONE:                                            break;
    case PAD_PROP::BGA:            props += _( "BGA" );             break;
    case PAD_PROP::FIDUCIAL_GLBL:  props += _( "Fiducial global" ); break;
    case PAD_PROP::FIDUCIAL_LOCAL: props += _( "Fiducial local" );  break;
    case PAD_PROP::TESTPOINT:      props += _( "Test point" );      break;
    case PAD_PROP::HEATSINK:       props += _( "Heat sink" );       break;
    case PAD_PROP::CASTELLATED:    props += _( "Castellated" );     break;
    case PAD_PROP::MECHANICAL:     props += _( "Mechanical" );      break;
    }

    // TODO(JE) How to show complex padstack info in the message panel
    aList.emplace_back( ShowPadShape( PADSTACK::ALL_LAYERS ), props );

    PAD_SHAPE padShape = GetShape( PADSTACK::ALL_LAYERS );
    VECTOR2I padSize = m_padStack.Size( PADSTACK::ALL_LAYERS );

    if( ( padShape == PAD_SHAPE::CIRCLE || padShape == PAD_SHAPE::OVAL )
            && padSize.x == padSize.y )
    {
        aList.emplace_back( _( "Diameter" ), aFrame->MessageTextFromValue( padSize.x ) );
    }
    else
    {
        aList.emplace_back( _( "Width" ), aFrame->MessageTextFromValue( padSize.x ) );
        aList.emplace_back( _( "Height" ), aFrame->MessageTextFromValue( padSize.y ) );
    }

    EDA_ANGLE fp_orient = parentFootprint ? parentFootprint->GetOrientation() : ANGLE_0;
    EDA_ANGLE pad_orient = GetOrientation() - fp_orient;
    pad_orient.Normalize180();

    if( !fp_orient.IsZero() )
        msg.Printf( wxT( "%g(+ %g)" ), pad_orient.AsDegrees(), fp_orient.AsDegrees() );
    else
        msg.Printf( wxT( "%g" ), GetOrientation().AsDegrees() );

    aList.emplace_back( _( "Rotation" ), msg );

    if( GetPadToDieLength() )
    {
        aList.emplace_back( _( "Length in Package" ),
                            aFrame->MessageTextFromValue( GetPadToDieLength() ) );
    }

    const VECTOR2I& drill = m_padStack.Drill().size;

    if( drill.x > 0 || drill.y > 0 )
    {
        if( GetDrillShape() == PAD_DRILL_SHAPE::CIRCLE )
        {
            aList.emplace_back( _( "Hole" ),
                                wxString::Format( wxT( "%s" ),
                                                  aFrame->MessageTextFromValue( drill.x ) ) );
        }
        else
        {
            aList.emplace_back( _( "Hole X / Y" ),
                                wxString::Format( wxT( "%s / %s" ),
                                                  aFrame->MessageTextFromValue( drill.x ),
                                                  aFrame->MessageTextFromValue( drill.y ) ) );
        }
    }

    wxString source;
    int      clearance = GetOwnClearance( UNDEFINED_LAYER, &source );

    if( !source.IsEmpty() )
    {
        aList.emplace_back( wxString::Format( _( "Min Clearance: %s" ),
                                              aFrame->MessageTextFromValue( clearance ) ),
                            wxString::Format( _( "(from %s)" ),
                                              source ) );
    }
#if 0
    // useful for debug only
    aList.emplace_back( wxT( "UUID" ), m_Uuid.AsString() );
#endif
}


bool PAD::HitTest( const VECTOR2I& aPosition, int aAccuracy ) const
{
    VECTOR2I delta = aPosition - GetPosition();
    int      boundingRadius = GetBoundingRadius() + aAccuracy;

    if( delta.SquaredEuclideanNorm() > SEG::Square( boundingRadius ) )
        return false;

    bool contains = false;

    Padstack().ForEachUniqueLayer(
            [&]( PCB_LAYER_ID l )
            {
                if( contains )
                    return;

                if( GetEffectivePolygon( l, ERROR_INSIDE )->Contains( aPosition, -1, aAccuracy ) )
                    contains = true;
            } );

    contains |= GetEffectiveHoleShape()->Collide( aPosition, aAccuracy );

    return contains;
}


bool PAD::HitTest( const BOX2I& aRect, bool aContained, int aAccuracy ) const
{
    BOX2I arect = aRect;
    arect.Normalize();
    arect.Inflate( aAccuracy );

    BOX2I bbox = GetBoundingBox();

    if( aContained )
    {
        return arect.Contains( bbox );
    }
    else
    {
        // Fast test: if aRect is outside the polygon bounding box,
        // rectangles cannot intersect
        if( !arect.Intersects( bbox ) )
            return false;

        bool hit = false;

        Padstack().ForEachUniqueLayer(
                [&]( PCB_LAYER_ID aLayer )
                {
                    if( hit )
                        return;

                    const std::shared_ptr<SHAPE_POLY_SET>& poly = GetEffectivePolygon( aLayer, ERROR_INSIDE );

                    int count = poly->TotalVertices();

                    for( int ii = 0; ii < count; ii++ )
                    {
                        VECTOR2I vertex = poly->CVertex( ii );
                        VECTOR2I vertexNext = poly->CVertex( ( ii + 1 ) % count );

                        // Test if the point is within aRect
                        if( arect.Contains( vertex ) )
                        {
                            hit = true;
                            break;
                        }

                        // Test if this edge intersects aRect
                        if( arect.Intersects( vertex, vertexNext ) )
                        {
                            hit = true;
                            break;
                        }
                    }
                } );

        if( !hit )
        {
            SHAPE_RECT rect( arect );
            hit |= GetEffectiveHoleShape()->Collide( &rect );
        }

        return hit;
    }
}


bool PAD::HitTest( const SHAPE_LINE_CHAIN& aPoly, bool aContained ) const
{
    SHAPE_COMPOUND effectiveShape;

    // Add padstack shapes
    Padstack().ForEachUniqueLayer(
            [&]( PCB_LAYER_ID aLayer )
            {
                effectiveShape.AddShape( GetEffectiveShape( aLayer ) );
            } );

    // Add hole shape
    effectiveShape.AddShape( GetEffectiveHoleShape() );

    return KIGEOM::ShapeHitTest( aPoly, effectiveShape, aContained );
}


int PAD::Compare( const PAD* aPadRef, const PAD* aPadCmp )
{
    int diff;

    if( ( diff = static_cast<int>( aPadRef->m_attribute ) - static_cast<int>( aPadCmp->m_attribute ) ) != 0 )
        return diff;

    return PADSTACK::Compare( &aPadRef->Padstack(), &aPadCmp->Padstack() );
}


void PAD::Rotate( const VECTOR2I& aRotCentre, const EDA_ANGLE& aAngle )
{
    RotatePoint( m_pos, aRotCentre, aAngle );
    m_padStack.SetOrientation( m_padStack.GetOrientation() + aAngle );

    SetDirty();
}


wxString PAD::ShowPadShape( PCB_LAYER_ID aLayer ) const
{
    switch( GetShape( aLayer ) )
    {
    case PAD_SHAPE::CIRCLE:         return _( "Circle" );
    case PAD_SHAPE::OVAL:           return _( "Oval" );
    case PAD_SHAPE::RECTANGLE:      return _( "Rect" );
    case PAD_SHAPE::TRAPEZOID:      return _( "Trap" );
    case PAD_SHAPE::ROUNDRECT:      return _( "Roundrect" );
    case PAD_SHAPE::CHAMFERED_RECT: return _( "Chamferedrect" );
    case PAD_SHAPE::CUSTOM:         return _( "CustomShape" );
    default:                        return wxT( "???" );
    }
}


wxString PAD::ShowPadAttr() const
{
    switch( GetAttribute() )
    {
    case PAD_ATTRIB::PTH:    return _( "PTH" );
    case PAD_ATTRIB::SMD:    return _( "SMD" );
    case PAD_ATTRIB::CONN:   return _( "Conn" );
    case PAD_ATTRIB::NPTH:   return _( "NPTH" );
    default:                 return wxT( "???" );
    }
}


wxString PAD::GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const
{
    FOOTPRINT* parentFP = GetParentFootprint();

    // Don't report parent footprint info from footprint editor, viewer, etc.
    if( GetBoard() && GetBoard()->GetBoardUse() == BOARD_USE::FPHOLDER )
        parentFP = nullptr;

    if( GetAttribute() == PAD_ATTRIB::NPTH )
    {
        if( parentFP )
            return wxString::Format( _( "NPTH pad of %s" ), parentFP->GetReference() );
        else
            return _( "NPTH pad" );
    }
    else if( GetNumber().IsEmpty() )
    {
        if( GetAttribute() == PAD_ATTRIB::SMD || GetAttribute() == PAD_ATTRIB::CONN )
        {
            if( parentFP )
            {
                return wxString::Format( _( "Pad %s of %s on %s" ),
                                         GetNetnameMsg(),
                                         parentFP->GetReference(),
                                         layerMaskDescribe() );
            }
            else
            {
                return wxString::Format( _( "Pad on %s" ),
                                         layerMaskDescribe() );
            }
        }
        else
        {
            if( parentFP )
            {
                return wxString::Format( _( "PTH pad %s of %s" ),
                                         GetNetnameMsg(),
                                         parentFP->GetReference() );
            }
            else
            {
                return _( "PTH pad" );
            }
        }
    }
    else
    {
        if( GetAttribute() == PAD_ATTRIB::SMD || GetAttribute() == PAD_ATTRIB::CONN )
        {
            if( parentFP )
            {
                return wxString::Format( _( "Pad %s %s of %s on %s" ),
                                         GetNumber(),
                                         GetNetnameMsg(),
                                         parentFP->GetReference(),
                                         layerMaskDescribe() );
            }
            else
            {
                return wxString::Format( _( "Pad %s on %s" ),
                                         GetNumber(),
                                         layerMaskDescribe() );
            }
        }
        else
        {
            if( parentFP )
            {
                return wxString::Format( _( "PTH pad %s %s of %s" ),
                                         GetNumber(),
                                         GetNetnameMsg(),
                                         parentFP->GetReference() );
            }
            else
            {
                return wxString::Format( _( "PTH pad %s" ),
                                         GetNumber() );
            }
        }
    }
}


BITMAPS PAD::GetMenuImage() const
{
    return BITMAPS::pad;
}


EDA_ITEM* PAD::Clone() const
{
    PAD* cloned = new PAD( *this );

    // Ensure the cloned primitives of the pad stack have the right parent
    cloned->Padstack().ForEachUniqueLayer(
            [&]( PCB_LAYER_ID aLayer )
            {
                for( std::shared_ptr<PCB_SHAPE>& primitive : cloned->m_padStack.Primitives( aLayer ) )
                    primitive->SetParent( cloned );
            } );

    return cloned;
}


std::vector<int> PAD::ViewGetLayers() const
{
    std::vector<int> layers;
    layers.reserve( 64 );

    // These 2 types of pads contain a hole
    if( m_attribute == PAD_ATTRIB::PTH )
    {
        layers.push_back( LAYER_PAD_PLATEDHOLES );
        layers.push_back( LAYER_PAD_HOLEWALLS );
    }

    if( m_attribute == PAD_ATTRIB::NPTH )
        layers.push_back( LAYER_NON_PLATEDHOLES );


    if( IsLocked() || ( GetParentFootprint() && GetParentFootprint()->IsLocked() ) )
        layers.push_back( LAYER_LOCKED_ITEM_SHADOW );

    LSET cuLayers = ( m_padStack.LayerSet() & LSET::AllCuMask() );

    // Don't spend cycles rendering layers that aren't visible
    if( const BOARD* board = GetBoard() )
        cuLayers &= board->GetEnabledLayers();

    if( cuLayers.count() > 1 )
    {
        // Multi layer pad
        for( PCB_LAYER_ID layer : cuLayers.Seq() )
        {
            layers.push_back( LAYER_PAD_COPPER_START + layer );
            layers.push_back( LAYER_CLEARANCE_START + layer );
        }

        layers.push_back( LAYER_PAD_NETNAMES );
    }
    else if( IsOnLayer( F_Cu ) )
    {
        layers.push_back( LAYER_PAD_COPPER_START );
        layers.push_back( LAYER_CLEARANCE_START );

        // Is this a PTH pad that has only front copper?  If so, we need to also display the
        // net name on the PTH netname layer so that it isn't blocked by the drill hole.
        if( m_attribute == PAD_ATTRIB::PTH )
            layers.push_back( LAYER_PAD_NETNAMES );
        else
            layers.push_back( LAYER_PAD_FR_NETNAMES );
    }
    else if( IsOnLayer( B_Cu ) )
    {
        layers.push_back( LAYER_PAD_COPPER_START + B_Cu );
        layers.push_back( LAYER_CLEARANCE_START + B_Cu );

        // Is this a PTH pad that has only back copper?  If so, we need to also display the
        // net name on the PTH netname layer so that it isn't blocked by the drill hole.
        if( m_attribute == PAD_ATTRIB::PTH )
            layers.push_back( LAYER_PAD_NETNAMES );
        else
            layers.push_back( LAYER_PAD_BK_NETNAMES );
    }

    // Check non-copper layers. This list should include all the layers that the
    // footprint editor allows a pad to be placed on.
    static const PCB_LAYER_ID layers_mech[] = { F_Mask, B_Mask, F_Paste, B_Paste,
        F_Adhes, B_Adhes, F_SilkS, B_SilkS, Dwgs_User, Eco1_User, Eco2_User };

    for( PCB_LAYER_ID each_layer : layers_mech )
    {
        if( IsOnLayer( each_layer ) )
            layers.push_back( each_layer );
    }

    return layers;
}


double PAD::ViewGetLOD( int aLayer, const KIGFX::VIEW* aView ) const
{
    PCB_PAINTER&         painter = static_cast<PCB_PAINTER&>( *aView->GetPainter() );
    PCB_RENDER_SETTINGS& renderSettings = *painter.GetSettings();
    const BOARD*         board = GetBoard();

    // Meta control for hiding all pads
    if( !aView->IsLayerVisible( LAYER_PADS ) )
        return LOD_HIDE;

    // Handle Render tab switches
    //const PCB_LAYER_ID& pcbLayer = static_cast<PCB_LAYER_ID>( aLayer );

    if( !IsFlipped() && !aView->IsLayerVisible( LAYER_FOOTPRINTS_FR ) )
        return LOD_HIDE;

    if( IsFlipped() && !aView->IsLayerVisible( LAYER_FOOTPRINTS_BK ) )
        return LOD_HIDE;

    if( IsHoleLayer( aLayer ) )
    {
        LSET visiblePhysical = board->GetVisibleLayers();
        visiblePhysical &= board->GetEnabledLayers();
        visiblePhysical &= LSET::PhysicalLayersMask();

        if( !visiblePhysical.any() )
            return LOD_HIDE;
    }
    else if( IsNetnameLayer( aLayer ) )
    {
        if( renderSettings.GetHighContrast() )
        {
            // Hide netnames unless pad is flashed to a high-contrast layer
            if( !FlashLayer( renderSettings.GetPrimaryHighContrastLayer() ) )
                return LOD_HIDE;
        }
        else
        {
            LSET visible = board->GetVisibleLayers();
            visible &= board->GetEnabledLayers();

            // Hide netnames unless pad is flashed to a visible layer
            if( !FlashLayer( visible ) )
                return LOD_HIDE;
        }

        // Netnames will be shown only if zoom is appropriate
        const int minSize = std::min( GetBoundingBox().GetWidth(), GetBoundingBox().GetHeight() );

        return lodScaleForThreshold( aView, minSize, pcbIUScale.mmToIU( 0.5 ) );
    }

    // Hole walls always need a repaint when zoom level changes after the last
    // LAYER_PAD_HOLEWALLS shape rebuild
    if( aLayer == LAYER_PAD_HOLEWALLS )
    {
        if( aView->GetGAL()->GetZoomFactor() != m_lastGalZoomLevel )
        {
            aView->Update( this, KIGFX::REPAINT );
            m_lastGalZoomLevel = aView->GetGAL()->GetZoomFactor();
        }
    }

    VECTOR2L padSize = GetBoundingBox().GetSize();
    int64_t  minSide = std::min( padSize.x, padSize.y );

    if( minSide > 0 )
        return std::min( lodScaleForThreshold( aView, minSide, pcbIUScale.mmToIU( 0.2 ) ), 3.5 );

    return LOD_SHOW;
}


const BOX2I PAD::ViewBBox() const
{
    // Bounding box includes soldermask too. Remember mask and/or paste margins can be < 0
    int      solderMaskMargin = 0;
    VECTOR2I solderPasteMargin;

    Padstack().ForEachUniqueLayer(
            [&]( PCB_LAYER_ID aLayer )
            {
                solderMaskMargin = std::max( solderMaskMargin, std::max( GetSolderMaskExpansion( aLayer ), 0 ) );
                VECTOR2I layerMargin = GetSolderPasteMargin( aLayer );
                solderPasteMargin.x = std::max( solderPasteMargin.x, layerMargin.x );
                solderPasteMargin.y = std::max( solderPasteMargin.y, layerMargin.y );
            } );

    BOX2I    bbox              = GetBoundingBox();
    int      clearance         = 0;

    // If we're drawing clearance lines then get the biggest possible clearance
    if( PCBNEW_SETTINGS* cfg = dynamic_cast<PCBNEW_SETTINGS*>( Kiface().KifaceSettings() ) )
    {
        if( cfg && cfg->m_Display.m_PadClearance && GetBoard() )
            clearance = GetBoard()->GetMaxClearanceValue();
    }

    // Look for the biggest possible bounding box
    int xMargin = std::max( solderMaskMargin, solderPasteMargin.x ) + clearance;
    int yMargin = std::max( solderMaskMargin, solderPasteMargin.y ) + clearance;

    return BOX2I( VECTOR2I( bbox.GetOrigin() ) - VECTOR2I( xMargin, yMargin ),
                  VECTOR2I( bbox.GetSize() ) + VECTOR2I( 2 * xMargin, 2 * yMargin ) );
}


void PAD::ImportSettingsFrom( const PAD& aMasterPad )
{
    SetPadstack( aMasterPad.Padstack() );
    // Layer Set should be updated before calling SetAttribute()
    SetLayerSet( aMasterPad.GetLayerSet() );
    SetAttribute( aMasterPad.GetAttribute() );
    // Unfortunately, SetAttribute() can change m_layerMask.
    // Be sure we keep the original mask by calling SetLayerSet() after SetAttribute()
    SetLayerSet( aMasterPad.GetLayerSet() );
    SetProperty( aMasterPad.GetProperty() );

    // Must be after setting attribute and layerSet
    if( !CanHaveNumber() )
        SetNumber( wxEmptyString );

    // I am not sure the m_LengthPadToDie should be imported, because this is a parameter
    // really specific to a given pad (JPC).
#if 0
    SetPadToDieLength( aMasterPad.GetPadToDieLength() );
    SetPadToDieDelay( aMasterPad.GetPadToDieDelay() );
#endif

    // The pad orientation, for historical reasons is the pad rotation + parent rotation.
    EDA_ANGLE pad_rot = aMasterPad.GetOrientation();

    if( aMasterPad.GetParentFootprint() )
        pad_rot -= aMasterPad.GetParentFootprint()->GetOrientation();

    if( GetParentFootprint() )
        pad_rot += GetParentFootprint()->GetOrientation();

    SetOrientation( pad_rot );

    Padstack().ForEachUniqueLayer(
            [&]( PCB_LAYER_ID aLayer )
            {
                // Ensure that circles are circles
                if( aMasterPad.GetShape( aLayer ) == PAD_SHAPE::CIRCLE )
                    SetSize( aLayer, VECTOR2I( GetSize( aLayer ).x, GetSize( aLayer ).x ) );
            } );

    switch( aMasterPad.GetAttribute() )
    {
    case PAD_ATTRIB::SMD:
    case PAD_ATTRIB::CONN:
        // These pads do not have a hole (they are expected to be on one external copper layer)
        SetDrillSize( VECTOR2I( 0, 0 ) );
        break;

    default:
        ;
    }

    // copy also local settings:
    SetLocalClearance( aMasterPad.GetLocalClearance() );
    SetLocalSolderMaskMargin( aMasterPad.GetLocalSolderMaskMargin() );
    SetLocalSolderPasteMargin( aMasterPad.GetLocalSolderPasteMargin() );
    SetLocalSolderPasteMarginRatio( aMasterPad.GetLocalSolderPasteMarginRatio() );

    SetLocalZoneConnection( aMasterPad.GetLocalZoneConnection() );
    SetLocalThermalSpokeWidthOverride( aMasterPad.GetLocalThermalSpokeWidthOverride() );
    SetThermalSpokeAngle( aMasterPad.GetThermalSpokeAngle() );
    SetLocalThermalGapOverride( aMasterPad.GetLocalThermalGapOverride() );

    SetCustomShapeInZoneOpt( aMasterPad.GetCustomShapeInZoneOpt() );

    m_teardropParams = aMasterPad.m_teardropParams;

    SetDirty();
}


void PAD::swapData( BOARD_ITEM* aImage )
{
    assert( aImage->Type() == PCB_PAD_T );

    std::swap( *this, *static_cast<PAD*>( aImage ) );
}


bool PAD::TransformHoleToPolygon( SHAPE_POLY_SET& aBuffer, int aClearance, int aError,
                                  ERROR_LOC aErrorLoc ) const
{
    VECTOR2I drillsize = GetDrillSize();

    if( !drillsize.x || !drillsize.y )
        return false;

    std::shared_ptr<SHAPE_SEGMENT> slot = GetEffectiveHoleShape();

    TransformOvalToPolygon( aBuffer, slot->GetSeg().A, slot->GetSeg().B, slot->GetWidth() + aClearance * 2,
                            aError, aErrorLoc );

    return true;
}


void PAD::TransformShapeToPolygon( SHAPE_POLY_SET& aBuffer, PCB_LAYER_ID aLayer, int aClearance,
                                   int aMaxError, ERROR_LOC aErrorLoc, bool ignoreLineWidth ) const
{
    wxASSERT_MSG( !ignoreLineWidth, wxT( "IgnoreLineWidth has no meaning for pads." ) );
    wxASSERT_MSG( aLayer != UNDEFINED_LAYER,
                  wxT( "UNDEFINED_LAYER is no longer allowed for PAD::TransformShapeToPolygon" ) );

    // minimal segment count to approximate a circle to create the polygonal pad shape
    // This minimal value is mainly for very small pads, like SM0402.
    // Most of time pads are using the segment count given by aError value.
    const int pad_min_seg_per_circle_count = 16;
    int       dx = m_padStack.Size( aLayer ).x / 2;
    int       dy = m_padStack.Size( aLayer ).y / 2;

    VECTOR2I padShapePos = ShapePos( aLayer ); // Note: for pad having a shape offset, the pad
                                               // position is NOT the shape position

    switch( PAD_SHAPE shape = GetShape( aLayer ) )
    {
    case PAD_SHAPE::CIRCLE:
    case PAD_SHAPE::OVAL:
        // Note: dx == dy is not guaranteed for circle pads in legacy boards
        if( dx == dy || ( shape == PAD_SHAPE::CIRCLE ) )
        {
            TransformCircleToPolygon( aBuffer, padShapePos, dx + aClearance, aMaxError, aErrorLoc,
                                      pad_min_seg_per_circle_count );
        }
        else
        {
            int      half_width = std::min( dx, dy );
            VECTOR2I delta( dx - half_width, dy - half_width );

            RotatePoint( delta, GetOrientation() );

            TransformOvalToPolygon( aBuffer, padShapePos - delta, padShapePos + delta,
                                    ( half_width + aClearance ) * 2, aMaxError, aErrorLoc,
                                    pad_min_seg_per_circle_count );
        }

        break;

    case PAD_SHAPE::TRAPEZOID:
    case PAD_SHAPE::RECTANGLE:
    {
        const VECTOR2I& trapDelta = m_padStack.TrapezoidDeltaSize( aLayer );
        int  ddx = shape == PAD_SHAPE::TRAPEZOID ? trapDelta.x / 2 : 0;
        int  ddy = shape == PAD_SHAPE::TRAPEZOID ? trapDelta.y / 2 : 0;

        SHAPE_POLY_SET outline;
        TransformTrapezoidToPolygon( outline, padShapePos, m_padStack.Size( aLayer ), GetOrientation(),
                                     ddx, ddy, aClearance, aMaxError, aErrorLoc );
        aBuffer.Append( outline );
        break;
    }

    case PAD_SHAPE::CHAMFERED_RECT:
    case PAD_SHAPE::ROUNDRECT:
    {
        bool doChamfer = shape == PAD_SHAPE::CHAMFERED_RECT;

        SHAPE_POLY_SET outline;
        TransformRoundChamferedRectToPolygon( outline, padShapePos, m_padStack.Size( aLayer ),
                                              GetOrientation(), GetRoundRectCornerRadius( aLayer ),
                                              doChamfer ? GetChamferRectRatio( aLayer ) : 0,
                                              doChamfer ? GetChamferPositions( aLayer ) : 0,
                                              aClearance, aMaxError, aErrorLoc );
        aBuffer.Append( outline );
        break;
    }

    case PAD_SHAPE::CUSTOM:
    {
        SHAPE_POLY_SET outline;
        MergePrimitivesAsPolygon( aLayer, &outline, aErrorLoc );
        outline.Rotate( GetOrientation() );
        outline.Move( VECTOR2I( padShapePos ) );

        if( aClearance > 0 || aErrorLoc == ERROR_OUTSIDE )
        {
            if( aErrorLoc == ERROR_OUTSIDE )
                aClearance += aMaxError;

            outline.Inflate( aClearance, CORNER_STRATEGY::ROUND_ALL_CORNERS, aMaxError );
            outline.Fracture();
        }
        else if( aClearance < 0 )
        {
            // Negative clearances are primarily for drawing solder paste layer, so we don't
            // worry ourselves overly about which side the error is on.

            // aClearance is negative so this is actually a deflate
            outline.Inflate( aClearance, CORNER_STRATEGY::ALLOW_ACUTE_CORNERS, aMaxError );
            outline.Fracture();
        }

        aBuffer.Append( outline );
        break;
    }

    default:
        wxFAIL_MSG( wxT( "PAD::TransformShapeToPolygon no implementation for " )
                    + wxString( std::string( magic_enum::enum_name( shape ) ) ) );
        break;
    }
}


std::vector<PCB_SHAPE*> PAD::Recombine( bool aIsDryRun, int maxError )
{
    FOOTPRINT* footprint = GetParentFootprint();

    for( BOARD_ITEM* item : footprint->GraphicalItems() )
        item->ClearFlags( SKIP_STRUCT );

    auto findNext =
            [&]( PCB_LAYER_ID aLayer ) -> PCB_SHAPE*
            {
                SHAPE_POLY_SET padPoly;
                TransformShapeToPolygon( padPoly, aLayer, 0, maxError, ERROR_INSIDE );

                for( BOARD_ITEM* item : footprint->GraphicalItems() )
                {
                    PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( item );

                    if( !shape || ( shape->GetFlags() & SKIP_STRUCT ) )
                        continue;

                    if( shape->GetLayer() != aLayer )
                        continue;

                    if( shape->IsProxyItem() )    // Pad number (and net name) box
                        return shape;

                    SHAPE_POLY_SET drawPoly;
                    shape->TransformShapeToPolygon( drawPoly, aLayer, 0, maxError, ERROR_INSIDE );
                    drawPoly.BooleanIntersection( padPoly );

                    if( !drawPoly.IsEmpty() )
                        return shape;
                }

                return nullptr;
            };

    auto findMatching =
            [&]( PCB_SHAPE* aShape ) -> std::vector<PCB_SHAPE*>
            {
                std::vector<PCB_SHAPE*> matching;

                for( BOARD_ITEM* item : footprint->GraphicalItems() )
                {
                    PCB_SHAPE* other = dynamic_cast<PCB_SHAPE*>( item );

                    if( !other || ( other->GetFlags() & SKIP_STRUCT ) )
                        continue;

                    if( GetLayerSet().test( other->GetLayer() ) && aShape->Compare( other ) == 0 )
                        matching.push_back( other );
                }

                return matching;
            };

    PCB_LAYER_ID            layer;
    std::vector<PCB_SHAPE*> mergedShapes;

    if( IsOnLayer( F_Cu ) )
        layer = F_Cu;
    else if( IsOnLayer( B_Cu ) )
        layer = B_Cu;
    else
        layer = GetLayerSet().UIOrder().front();

    PAD_SHAPE origShape = GetShape( layer );

    // If there are intersecting items to combine, we need to first make sure the pad is a
    // custom-shape pad.
    if( !aIsDryRun && findNext( layer ) && origShape != PAD_SHAPE::CUSTOM )
    {
        if( origShape == PAD_SHAPE::CIRCLE || origShape == PAD_SHAPE::RECTANGLE )
        {
            // Use the existing pad as an anchor
            SetAnchorPadShape( layer, origShape );
            SetShape( layer, PAD_SHAPE::CUSTOM );
        }
        else
        {
            // Create a new circular anchor and convert existing pad to a polygon primitive
            SHAPE_POLY_SET existingOutline;
            TransformShapeToPolygon( existingOutline, layer, 0, maxError, ERROR_INSIDE );

            int minExtent = std::min( GetSize( layer ).x, GetSize( layer ).y );
            SetAnchorPadShape( layer, PAD_SHAPE::CIRCLE );
            SetSize( layer, VECTOR2I( minExtent, minExtent ) );
            SetShape( layer, PAD_SHAPE::CUSTOM );

            PCB_SHAPE* shape = new PCB_SHAPE( nullptr, SHAPE_T::POLY );
            shape->SetFilled( true );
            shape->SetStroke( STROKE_PARAMS( 0, LINE_STYLE::SOLID ) );
            shape->SetPolyShape( existingOutline );
            shape->Move( - ShapePos( layer ) );
            shape->Rotate( VECTOR2I( 0, 0 ), - GetOrientation() );
            AddPrimitive( layer, shape );
        }
    }

    while( PCB_SHAPE* fpShape = findNext( layer ) )
    {
        fpShape->SetFlags( SKIP_STRUCT );

        mergedShapes.push_back( fpShape );

        if( !aIsDryRun )
        {
            // If the editor was inside a group when the pad was exploded, the added exploded shapes
            // will be part of the group.  Remove them here before duplicating; we don't want the
            // primitives to wind up in a group.
            if( EDA_GROUP* group = fpShape->GetParentGroup(); group )
                group->RemoveItem( fpShape );

            PCB_SHAPE* primitive = static_cast<PCB_SHAPE*>( fpShape->Duplicate( IGNORE_PARENT_GROUP ) );

            primitive->SetParent( nullptr );

            // Convert any hatched fills to solid
            if( primitive->IsAnyFill() )
                primitive->SetFillMode( FILL_T::FILLED_SHAPE );

            primitive->Move( - ShapePos( layer ) );
            primitive->Rotate( VECTOR2I( 0, 0 ), - GetOrientation() );

            AddPrimitive( layer, primitive );
        }

        // See if there are other shapes that match and mark them for delete.  (KiCad won't
        // produce these, but old footprints from other vendors have them.)
        for( PCB_SHAPE* other : findMatching( fpShape ) )
        {
            other->SetFlags( SKIP_STRUCT );
            mergedShapes.push_back( other );
        }
    }

    for( BOARD_ITEM* item : footprint->GraphicalItems() )
        item->ClearFlags( SKIP_STRUCT );

    if( !aIsDryRun )
        ClearFlags( ENTERED );

    return mergedShapes;
}


void PAD::CheckPad( UNITS_PROVIDER* aUnitsProvider, bool aForPadProperties,
                    const std::function<void( int aErrorCode, const wxString& aMsg )>& aErrorHandler ) const
{
    Padstack().ForEachUniqueLayer(
            [&]( PCB_LAYER_ID aLayer )
            {
                doCheckPad( aLayer, aUnitsProvider, aForPadProperties, aErrorHandler );
            } );

    LSET padlayers_mask = GetLayerSet();
    VECTOR2I drill_size = GetDrillSize();

    if( !padlayers_mask[F_Cu] && !padlayers_mask[B_Cu] )
    {
        if( ( drill_size.x || drill_size.y ) && GetAttribute() != PAD_ATTRIB::NPTH )
        {
            aErrorHandler( DRCE_PADSTACK, _( "(plated through holes normally have a copper pad on "
                                             "at least one outer layer)" ) );
        }
    }

    if( ( GetProperty() == PAD_PROP::FIDUCIAL_GLBL || GetProperty() == PAD_PROP::FIDUCIAL_LOCAL )
            && GetAttribute() == PAD_ATTRIB::NPTH )
    {
        aErrorHandler( DRCE_PADSTACK, _( "('fiducial' property makes no sense on NPTH pads)" ) );
    }

    if( GetProperty() == PAD_PROP::TESTPOINT && GetAttribute() == PAD_ATTRIB::NPTH )
        aErrorHandler( DRCE_PADSTACK, _( "('testpoint' property makes no sense on NPTH pads)" ) );

    if( GetProperty() == PAD_PROP::HEATSINK && GetAttribute() == PAD_ATTRIB::NPTH )
        aErrorHandler( DRCE_PADSTACK, _( "('heatsink' property makes no sense on NPTH pads)" ) );

    if( GetProperty() == PAD_PROP::CASTELLATED && GetAttribute() != PAD_ATTRIB::PTH )
        aErrorHandler( DRCE_PADSTACK, _( "('castellated' property is for PTH pads)" ) );

    if( GetProperty() == PAD_PROP::BGA && GetAttribute() != PAD_ATTRIB::SMD )
        aErrorHandler( DRCE_PADSTACK, _( "('BGA' property is for SMD pads)" ) );

    if( GetProperty() == PAD_PROP::MECHANICAL && GetAttribute() != PAD_ATTRIB::PTH )
        aErrorHandler( DRCE_PADSTACK, _( "('mechanical' property is for PTH pads)" ) );

    switch( GetAttribute() )
    {
    case PAD_ATTRIB::NPTH:   // Not plated, but through hole, a hole is expected
    case PAD_ATTRIB::PTH:    // Pad through hole, a hole is also expected
        if( drill_size.x <= 0
            || ( drill_size.y <= 0 && GetDrillShape() == PAD_DRILL_SHAPE::OBLONG ) )
        {
            aErrorHandler( DRCE_PAD_TH_WITH_NO_HOLE, wxEmptyString );
        }
        break;

    case PAD_ATTRIB::CONN:      // Connector pads are smd pads, just they do not have solder paste.
        if( padlayers_mask[B_Paste] || padlayers_mask[F_Paste] )
        {
            aErrorHandler( DRCE_PADSTACK, _( "(connector pads normally have no solder paste; use a "
                                             "SMD pad instead)" ) );
        }
        KI_FALLTHROUGH;

    case PAD_ATTRIB::SMD:       // SMD and Connector pads (One external copper layer only)
    {
        if( drill_size.x > 0 || drill_size.y > 0 )
            aErrorHandler( DRCE_PADSTACK_INVALID, _( "(SMD pad has a hole)" ) );

        LSET innerlayers_mask = padlayers_mask & LSET::InternalCuMask();

        if( IsOnLayer( F_Cu ) && IsOnLayer( B_Cu ) )
        {
            aErrorHandler( DRCE_PADSTACK, _( "(SMD pad has copper on both sides of the board)" ) );
        }
        else if( IsOnLayer( F_Cu ) )
        {
            if( IsOnLayer( B_Mask ) )
            {
                aErrorHandler( DRCE_PADSTACK, _( "(SMD pad has copper and mask layers on different "
                                                 "sides of the board)" ) );
            }
            else if( IsOnLayer( B_Paste ) )
            {
                aErrorHandler( DRCE_PADSTACK, _( "(SMD pad has copper and paste layers on different "
                                                 "sides of the board)" ) );
            }
        }
        else if( IsOnLayer( B_Cu ) )
        {
            if( IsOnLayer( F_Mask ) )
            {
                aErrorHandler( DRCE_PADSTACK, _( "(SMD pad has copper and mask layers on different "
                                                 "sides of the board)" ) );
            }
            else if( IsOnLayer( F_Paste ) )
            {
                aErrorHandler( DRCE_PADSTACK, _( "(SMD pad has copper and paste layers on different "
                                                 "sides of the board)" ) );
            }
        }
        else if( innerlayers_mask.count() != 0 )
        {
            aErrorHandler( DRCE_PADSTACK, _( "(SMD pad has no outer layers)" ) );
        }

        break;
    }
    }
}


void PAD::doCheckPad( PCB_LAYER_ID aLayer, UNITS_PROVIDER* aUnitsProvider, bool aForPadProperties,
                      const std::function<void( int aErrorCode, const wxString& aMsg )>& aErrorHandler ) const
{
    wxString msg;

    VECTOR2I pad_size = GetSize( aLayer );

    if( GetShape( aLayer ) == PAD_SHAPE::CUSTOM )
        pad_size = GetBoundingBox().GetSize();
    else if( pad_size.x <= 0 || ( pad_size.y <= 0 && GetShape( aLayer ) != PAD_SHAPE::CIRCLE ) )
        aErrorHandler( DRCE_PADSTACK_INVALID, _( "(Pad must have a positive size)" ) );

    // Test hole against pad shape
    if( IsOnCopperLayer() && GetDrillSize().x > 0 )
    {
        // Ensure the drill size can be handled in next calculations.
        // Use min size = 4 IU to be able to build a polygon from a hole shape
        const int min_drill_size = 4;

        if( GetDrillSizeX() <= min_drill_size || GetDrillSizeY() <= min_drill_size )
        {
            msg.Printf( _( "(PTH pad hole size must be larger than %s)" ),
                        aUnitsProvider->StringFromValue( min_drill_size, true ) );
            aErrorHandler( DRCE_PADSTACK_INVALID, msg );
        }

        SHAPE_POLY_SET padOutline;

        TransformShapeToPolygon( padOutline, aLayer, 0, GetMaxError(), ERROR_INSIDE );

        if( GetAttribute() == PAD_ATTRIB::PTH )
        {
            // Test if there is copper area outside hole
            std::shared_ptr<SHAPE_SEGMENT> hole = GetEffectiveHoleShape();
            SHAPE_POLY_SET                 holeOutline;

            TransformOvalToPolygon( holeOutline, hole->GetSeg().A, hole->GetSeg().B, hole->GetWidth(),
                                    GetMaxError(), ERROR_OUTSIDE );

            SHAPE_POLY_SET copper = padOutline;
            copper.BooleanSubtract( holeOutline );

            if( copper.IsEmpty() )
            {
                aErrorHandler( DRCE_PADSTACK, _( "(PTH pad hole leaves no copper)" ) );
            }
            else if( aForPadProperties )
            {
                // Test if the pad hole is fully inside the copper area.  Note that we only run
                // this check for pad properties because we run the more complete annular ring
                // checker on the board (which handles multiple pads with the same name).
                holeOutline.BooleanSubtract( padOutline );

                if( !holeOutline.IsEmpty() )
                    aErrorHandler( DRCE_PADSTACK, _( "(PTH pad hole not fully inside copper)" ) );
            }
        }
        else
        {
            // Test only if the pad hole's centre is inside the copper area
            if( !padOutline.Collide( GetPosition() ) )
                aErrorHandler( DRCE_PADSTACK, _( "(pad hole not inside pad shape)" ) );
        }
    }

    if( GetLocalClearance().value_or( 0 ) < 0 )
        aErrorHandler( DRCE_PADSTACK, _( "(negative local clearance values have no effect)" ) );

    // Some pads need a negative solder mask clearance (mainly for BGA with small pads)
    // However the negative solder mask clearance must not create negative mask size
    // Therefore test for minimal acceptable negative value
    std::optional<int> solderMaskMargin = GetLocalSolderMaskMargin();

    if( solderMaskMargin.has_value() && solderMaskMargin.value() < 0 )
    {
        int absMargin = abs( solderMaskMargin.value() );

        if( GetShape( aLayer ) == PAD_SHAPE::CUSTOM )
        {
            for( const std::shared_ptr<PCB_SHAPE>& shape : GetPrimitives( aLayer ) )
            {
                BOX2I shapeBBox = shape->GetBoundingBox();

                if( absMargin > shapeBBox.GetWidth() || absMargin > shapeBBox.GetHeight() )
                {
                    aErrorHandler( DRCE_PADSTACK, _( "(negative solder mask clearance is larger "
                                                     "than some shape primitives; results may be "
                                                     "surprising)" ) );

                    break;
                }
            }
        }
        else if( absMargin > pad_size.x || absMargin > pad_size.y )
        {
            aErrorHandler( DRCE_PADSTACK, _( "(negative solder mask clearance is larger than pad; "
                                             "no solder mask will be generated)" ) );
        }
    }

    // Some pads need a positive solder paste clearance (mainly for BGA with small pads)
    // However, a positive value can create issues if the resulting shape is too big.
    // (like a solder paste creating a solder paste area on a neighbor pad or on the solder mask)
    // So we could ask for user to confirm the choice
    // For now we just check for disappearing paste
    wxSize paste_size;
    int    paste_margin = GetLocalSolderPasteMargin().value_or( 0 );
    double paste_ratio = GetLocalSolderPasteMarginRatio().value_or( 0 );

    paste_size.x = pad_size.x + paste_margin + KiROUND( pad_size.x * paste_ratio );
    paste_size.y = pad_size.y + paste_margin + KiROUND( pad_size.y * paste_ratio );

    if( paste_size.x <= 0 || paste_size.y <= 0 )
    {
        aErrorHandler( DRCE_PADSTACK, _( "(negative solder paste margin is larger than pad; "
                                         "no solder paste mask will be generated)" ) );
    }

    if( GetShape( aLayer ) == PAD_SHAPE::ROUNDRECT )
    {
        if( GetRoundRectRadiusRatio( aLayer ) < 0.0 )
            aErrorHandler( DRCE_PADSTACK_INVALID, _( "(negative corner radius is not allowed)" ) );
        else if( GetRoundRectRadiusRatio( aLayer ) > 50.0 )
            aErrorHandler( DRCE_PADSTACK, _( "(corner size will make pad circular)" ) );
    }
    else if( GetShape( aLayer ) == PAD_SHAPE::CHAMFERED_RECT )
    {
        if( GetChamferRectRatio( aLayer ) < 0.0 )
            aErrorHandler( DRCE_PADSTACK_INVALID, _( "(negative corner chamfer is not allowed)" ) );
        else if( GetChamferRectRatio( aLayer ) > 50.0 )
            aErrorHandler( DRCE_PADSTACK_INVALID, _( "(corner chamfer is too large)" ) );
    }
    else if( GetShape( aLayer ) == PAD_SHAPE::TRAPEZOID )
    {
        if(    ( GetDelta( aLayer ).x < 0 && GetDelta( aLayer ).x < -GetSize( aLayer ).y )
            || ( GetDelta( aLayer ).x > 0 && GetDelta( aLayer ).x > GetSize( aLayer ).y )
            || ( GetDelta( aLayer ).y < 0 && GetDelta( aLayer ).y < -GetSize( aLayer ).x )
            || ( GetDelta( aLayer ).y > 0 && GetDelta( aLayer ).y > GetSize( aLayer ).x ) )
        {
            aErrorHandler( DRCE_PADSTACK_INVALID, _( "(trapezoid delta is too large)" ) );
        }
    }

    if( GetShape( aLayer ) == PAD_SHAPE::CUSTOM )
    {
        SHAPE_POLY_SET mergedPolygon;
        MergePrimitivesAsPolygon( aLayer, &mergedPolygon );

        if( mergedPolygon.OutlineCount() > 1 )
            aErrorHandler( DRCE_PADSTACK_INVALID, _( "(custom pad shape must resolve to a single polygon)" ) );
    }
}


bool PAD::operator==( const BOARD_ITEM& aBoardItem ) const
{
    if( Type() != aBoardItem.Type() )
        return false;

    if( m_parent && aBoardItem.GetParent() && m_parent->m_Uuid != aBoardItem.GetParent()->m_Uuid )
        return false;

    const PAD& other = static_cast<const PAD&>( aBoardItem );

    return *this == other;
}


bool PAD::operator==( const PAD& aOther ) const
{
    if( Padstack() != aOther.Padstack() )
        return false;

    if( GetPosition() != aOther.GetPosition() )
        return false;

    if( GetAttribute() != aOther.GetAttribute() )
        return false;

    return true;
}


double PAD::Similarity( const BOARD_ITEM& aOther ) const
{
    if( aOther.Type() != Type() )
        return 0.0;

    if( m_parent->m_Uuid != aOther.GetParent()->m_Uuid )
        return 0.0;

    const PAD& other = static_cast<const PAD&>( aOther );

    double similarity = 1.0;

    if( GetPosition() != other.GetPosition() )
        similarity *= 0.9;

    if( GetAttribute() != other.GetAttribute() )
        similarity *= 0.9;

    similarity *= Padstack().Similarity( other.Padstack() );

    return similarity;
}


void PAD::AddPrimitivePoly( PCB_LAYER_ID aLayer, const SHAPE_POLY_SET& aPoly, int aThickness,
                            bool aFilled )
{
    // If aPoly has holes, convert it to a polygon with no holes.
    SHAPE_POLY_SET poly_no_hole;
    poly_no_hole.Append( aPoly );

    if( poly_no_hole.HasHoles() )
        poly_no_hole.Fracture();

    // There should never be multiple shapes, but if there are, we split them into
    // primitives so that we can edit them both.
    for( int ii = 0; ii < poly_no_hole.OutlineCount(); ++ii )
    {
        SHAPE_POLY_SET poly_outline( poly_no_hole.COutline( ii ) );
        PCB_SHAPE* item = new PCB_SHAPE();
        item->SetShape( SHAPE_T::POLY );
        item->SetFilled( aFilled );
        item->SetPolyShape( poly_outline );
        item->SetStroke( STROKE_PARAMS( aThickness, LINE_STYLE::SOLID ) );
        item->SetParent( this );
        m_padStack.AddPrimitive( item, aLayer );
    }

    SetDirty();
}


void PAD::AddPrimitivePoly( PCB_LAYER_ID aLayer, const std::vector<VECTOR2I>& aPoly, int aThickness,
                            bool aFilled )
{
    PCB_SHAPE* item = new PCB_SHAPE( nullptr, SHAPE_T::POLY );
    item->SetFilled( aFilled );
    item->SetPolyPoints( aPoly );
    item->SetStroke( STROKE_PARAMS( aThickness, LINE_STYLE::SOLID ) );
    item->SetParent( this );
    m_padStack.AddPrimitive( item, aLayer );
    SetDirty();
}


void PAD::ReplacePrimitives( PCB_LAYER_ID aLayer, const std::vector<std::shared_ptr<PCB_SHAPE>>& aPrimitivesList )
{
    // clear old list
    DeletePrimitivesList( aLayer );

    // Import to the given shape list
    if( aPrimitivesList.size() )
        AppendPrimitives( aLayer, aPrimitivesList );

    SetDirty();
}


void PAD::AppendPrimitives( PCB_LAYER_ID aLayer, const std::vector<std::shared_ptr<PCB_SHAPE>>& aPrimitivesList )
{
    // Add duplicates of aPrimitivesList to the pad primitives list:
    for( const std::shared_ptr<PCB_SHAPE>& prim : aPrimitivesList )
        AddPrimitive( aLayer, new PCB_SHAPE( *prim ) );

    SetDirty();
}


void PAD::AddPrimitive( PCB_LAYER_ID aLayer, PCB_SHAPE* aPrimitive )
{
    aPrimitive->SetParent( this );
    m_padStack.AddPrimitive( aPrimitive, aLayer );

    SetDirty();
}


void PAD::DeletePrimitivesList( PCB_LAYER_ID aLayer )
{
    if( aLayer == UNDEFINED_LAYER )
    {
        m_padStack.ForEachUniqueLayer(
                [&]( PCB_LAYER_ID l )
                {
                    m_padStack.ClearPrimitives( l );
                } );
    }
    else
    {
        m_padStack.ClearPrimitives( aLayer);
    }

    SetDirty();
}


void PAD::MergePrimitivesAsPolygon( PCB_LAYER_ID aLayer, SHAPE_POLY_SET* aMergedPolygon,
                                    ERROR_LOC aErrorLoc ) const
{
    aMergedPolygon->RemoveAllContours();

    // Add the anchor pad shape in aMergedPolygon, others in aux_polyset:
    // The anchor pad is always at 0,0
    VECTOR2I padSize = GetSize( aLayer );

    switch( GetAnchorPadShape( aLayer ) )
    {
    case PAD_SHAPE::RECTANGLE:
    {
        SHAPE_RECT rect( -padSize.x / 2, -padSize.y / 2, padSize.x, padSize.y );
        aMergedPolygon->AddOutline( rect.Outline() );
        break;
    }

    default:
    case PAD_SHAPE::CIRCLE:
        TransformCircleToPolygon( *aMergedPolygon, VECTOR2I( 0, 0 ), padSize.x / 2, GetMaxError(), aErrorLoc );
        break;
    }

    SHAPE_POLY_SET polyset;

    for( const std::shared_ptr<PCB_SHAPE>& primitive : m_padStack.Primitives( aLayer ) )
    {
        if( !primitive->IsProxyItem() )
            primitive->TransformShapeToPolygon( polyset, UNDEFINED_LAYER, 0, GetMaxError(), aErrorLoc );
    }

    polyset.Simplify();

    // Merge all polygons with the initial pad anchor shape
    if( polyset.OutlineCount() )
    {
        aMergedPolygon->BooleanAdd( polyset );
        aMergedPolygon->Fracture();
    }
}


static struct PAD_DESC
{
    PAD_DESC()
    {
        ENUM_MAP<PAD_ATTRIB>::Instance()
                .Map( PAD_ATTRIB::PTH,             _HKI( "Through-hole" ) )
                .Map( PAD_ATTRIB::SMD,             _HKI( "SMD" ) )
                .Map( PAD_ATTRIB::CONN,            _HKI( "Edge connector" ) )
                .Map( PAD_ATTRIB::NPTH,            _HKI( "NPTH, mechanical" ) );

        ENUM_MAP<PAD_SHAPE>::Instance()
                .Map( PAD_SHAPE::CIRCLE,           _HKI( "Circle" ) )
                .Map( PAD_SHAPE::RECTANGLE,        _HKI( "Rectangle" ) )
                .Map( PAD_SHAPE::OVAL,             _HKI( "Oval" ) )
                .Map( PAD_SHAPE::TRAPEZOID,        _HKI( "Trapezoid" ) )
                .Map( PAD_SHAPE::ROUNDRECT,        _HKI( "Rounded rectangle" ) )
                .Map( PAD_SHAPE::CHAMFERED_RECT,   _HKI( "Chamfered rectangle" ) )
                .Map( PAD_SHAPE::CUSTOM,           _HKI( "Custom" ) );

        ENUM_MAP<PAD_PROP>::Instance()
                .Map( PAD_PROP::NONE,              _HKI( "None" ) )
                .Map( PAD_PROP::BGA,               _HKI( "BGA pad" ) )
                .Map( PAD_PROP::FIDUCIAL_GLBL,     _HKI( "Fiducial, global to board" ) )
                .Map( PAD_PROP::FIDUCIAL_LOCAL,    _HKI( "Fiducial, local to footprint" ) )
                .Map( PAD_PROP::TESTPOINT,         _HKI( "Test point pad" ) )
                .Map( PAD_PROP::HEATSINK,          _HKI( "Heatsink pad" ) )
                .Map( PAD_PROP::CASTELLATED,       _HKI( "Castellated pad" ) )
                .Map( PAD_PROP::MECHANICAL,        _HKI( "Mechanical pad" ) );

        ENUM_MAP<PAD_DRILL_SHAPE>::Instance()
                .Map( PAD_DRILL_SHAPE::CIRCLE,     _HKI( "Round" ) )
                .Map( PAD_DRILL_SHAPE::OBLONG,     _HKI( "Oblong" ) );

        ENUM_MAP<ZONE_CONNECTION>& zcMap = ENUM_MAP<ZONE_CONNECTION>::Instance();

        if( zcMap.Choices().GetCount() == 0 )
        {
            zcMap.Undefined( ZONE_CONNECTION::INHERITED );
            zcMap.Map( ZONE_CONNECTION::INHERITED,   _HKI( "Inherited" ) )
                 .Map( ZONE_CONNECTION::NONE,        _HKI( "None" ) )
                 .Map( ZONE_CONNECTION::THERMAL,     _HKI( "Thermal reliefs" ) )
                 .Map( ZONE_CONNECTION::FULL,        _HKI( "Solid" ) )
                 .Map( ZONE_CONNECTION::THT_THERMAL, _HKI( "Thermal reliefs for PTH" ) );
        }

        ENUM_MAP<PADSTACK::UNCONNECTED_LAYER_MODE>::Instance()
                .Map( PADSTACK::UNCONNECTED_LAYER_MODE::KEEP_ALL,   _HKI( "All copper layers" ) )
                .Map( PADSTACK::UNCONNECTED_LAYER_MODE::REMOVE_ALL, _HKI( "Connected layers only" ) )
                .Map( PADSTACK::UNCONNECTED_LAYER_MODE::REMOVE_EXCEPT_START_AND_END,
                      _HKI( "Front, back and connected layers" ) )
                .Map( PADSTACK::UNCONNECTED_LAYER_MODE::START_END_ONLY,
                      _HKI( "Start and end layers only" ) );

        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( PAD );
        propMgr.InheritsAfter( TYPE_HASH( PAD ), TYPE_HASH( BOARD_CONNECTED_ITEM ) );

        propMgr.Mask( TYPE_HASH( PAD ), TYPE_HASH( BOARD_CONNECTED_ITEM ), _HKI( "Layer" ) );
        propMgr.Mask( TYPE_HASH( PAD ), TYPE_HASH( BOARD_ITEM ), _HKI( "Locked" ) );

        propMgr.AddProperty( new PROPERTY<PAD, double>( _HKI( "Orientation" ),
                    &PAD::SetOrientationDegrees, &PAD::GetOrientationDegrees,
                    PROPERTY_DISPLAY::PT_DEGREE ) );

        auto isCopperPad =
                []( INSPECTABLE* aItem ) -> bool
                {
                    if( PAD* pad = dynamic_cast<PAD*>( aItem ) )
                        return pad->GetAttribute() != PAD_ATTRIB::NPTH;

                    return false;
                };

        auto padCanHaveHole =
                []( INSPECTABLE* aItem ) -> bool
                {
                    if( PAD* pad = dynamic_cast<PAD*>( aItem ) )
                        return pad->GetAttribute() == PAD_ATTRIB::PTH || pad->GetAttribute() == PAD_ATTRIB::NPTH;

                    return false;
                };

        auto hasNormalPadstack =
                []( INSPECTABLE* aItem ) -> bool
                {
                    if( PAD* pad = dynamic_cast<PAD*>( aItem ) )
                        return pad->Padstack().Mode() == PADSTACK::MODE::NORMAL;

                    return true;
                };

        propMgr.OverrideAvailability( TYPE_HASH( PAD ), TYPE_HASH( BOARD_CONNECTED_ITEM ), _HKI( "Net" ),
                                      isCopperPad );
        propMgr.OverrideAvailability( TYPE_HASH( PAD ), TYPE_HASH( BOARD_CONNECTED_ITEM ), _HKI( "Net Class" ),
                                      isCopperPad );

        const wxString groupPad = _HKI( "Pad Properties" );

        propMgr.AddProperty( new PROPERTY_ENUM<PAD, PAD_ATTRIB>( _HKI( "Pad Type" ),
                    &PAD::SetAttribute, &PAD::GetAttribute ), groupPad );

        propMgr.AddProperty( new PROPERTY_ENUM<PAD, PAD_SHAPE>( _HKI( "Pad Shape" ),
                    &PAD::SetFrontShape, &PAD::GetFrontShape ), groupPad )
                .SetAvailableFunc( hasNormalPadstack );

        propMgr.AddProperty( new PROPERTY<PAD, wxString>( _HKI( "Pad Number" ),
                    &PAD::SetNumber, &PAD::GetNumber ), groupPad )
                .SetAvailableFunc( isCopperPad );

        propMgr.AddProperty( new PROPERTY<PAD, wxString>( _HKI( "Pin Name" ),
                    &PAD::SetPinFunction, &PAD::GetPinFunction ), groupPad )
                .SetIsHiddenFromLibraryEditors();
        propMgr.AddProperty( new PROPERTY<PAD, wxString>( _HKI( "Pin Type" ),
                    &PAD::SetPinType, &PAD::GetPinType ), groupPad )
                .SetIsHiddenFromLibraryEditors()
                .SetChoicesFunc( []( INSPECTABLE* aItem )
                                 {
                                     wxPGChoices choices;

                                     for( int ii = 0; ii < ELECTRICAL_PINTYPES_TOTAL; ii++ )
                                         choices.Add( GetCanonicalElectricalTypeName( (ELECTRICAL_PINTYPE) ii ) );

                                     return choices;
                                 } );

        propMgr.AddProperty( new PROPERTY<PAD, int>( _HKI( "Size X" ),
                    &PAD::SetSizeX, &PAD::GetSizeX, PROPERTY_DISPLAY::PT_SIZE ), groupPad )
                .SetAvailableFunc( hasNormalPadstack );
        propMgr.AddProperty( new PROPERTY<PAD, int>( _HKI( "Size Y" ),
                    &PAD::SetSizeY, &PAD::GetSizeY, PROPERTY_DISPLAY::PT_SIZE ), groupPad )
                .SetAvailableFunc( []( INSPECTABLE* aItem ) -> bool
                                   {
                                       if( PAD* pad = dynamic_cast<PAD*>( aItem ) )
                                       {
                                           // Custom padstacks can't have size modified through panel
                                           if( pad->Padstack().Mode() != PADSTACK::MODE::NORMAL )
                                               return false;

                                           // Circle pads have no usable y-size
                                           return pad->GetShape( PADSTACK::ALL_LAYERS ) != PAD_SHAPE::CIRCLE;
                                       }

                                       return true;
                                   } );

        const auto hasRoundRadius =
                []( INSPECTABLE* aItem ) -> bool
                {
                    if( PAD* pad = dynamic_cast<PAD*>( aItem ) )
                    {
                        // Custom padstacks can't have this property modified through panel
                        if( pad->Padstack().Mode() != PADSTACK::MODE::NORMAL )
                            return false;

                        return PAD_UTILS::PadHasMeaningfulRoundingRadius( *pad, F_Cu );
                    }

                    return false;
                };

        propMgr.AddProperty( new PROPERTY<PAD, double>( _HKI( "Corner Radius Ratio" ),
                    &PAD::SetFrontRoundRectRadiusRatio, &PAD::GetFrontRoundRectRadiusRatio ), groupPad )
                .SetAvailableFunc( hasRoundRadius );

        propMgr.AddProperty( new PROPERTY<PAD, int>( _HKI( "Corner Radius Size" ),
                    &PAD::SetFrontRoundRectRadiusSize, &PAD::GetFrontRoundRectRadiusSize, PROPERTY_DISPLAY::PT_SIZE ),
                    groupPad )
                .SetAvailableFunc( hasRoundRadius );

        propMgr.AddProperty( new PROPERTY_ENUM<PAD, PAD_DRILL_SHAPE>( _HKI( "Hole Shape" ),
                    &PAD::SetDrillShape, &PAD::GetDrillShape ), groupPad )
                .SetWriteableFunc( padCanHaveHole );

        propMgr.AddProperty( new PROPERTY<PAD, int>( _HKI( "Hole Size X" ),
                    &PAD::SetDrillSizeX, &PAD::GetDrillSizeX, PROPERTY_DISPLAY::PT_SIZE ), groupPad )
                .SetWriteableFunc( padCanHaveHole )
                .SetValidator( PROPERTY_VALIDATORS::PositiveIntValidator );

        propMgr.AddProperty( new PROPERTY<PAD, int>( _HKI( "Hole Size Y" ),
                    &PAD::SetDrillSizeY, &PAD::GetDrillSizeY, PROPERTY_DISPLAY::PT_SIZE ), groupPad )
                .SetWriteableFunc( padCanHaveHole )
                .SetValidator( PROPERTY_VALIDATORS::PositiveIntValidator )
                .SetAvailableFunc( []( INSPECTABLE* aItem ) -> bool
                                   {
                                       // Circle holes have no usable y-size
                                       if( PAD* pad = dynamic_cast<PAD*>( aItem ) )
                                           return pad->GetDrillShape() != PAD_DRILL_SHAPE::CIRCLE;

                                       return true;
                                   } );

        propMgr.AddProperty( new PROPERTY_ENUM<PAD, PAD_PROP>( _HKI( "Fabrication Property" ),
                    &PAD::SetProperty, &PAD::GetProperty ), groupPad );

        propMgr.AddProperty( new PROPERTY_ENUM<PAD, PADSTACK::UNCONNECTED_LAYER_MODE>( _HKI( "Copper Layers" ),
                    &PAD::SetUnconnectedLayerMode, &PAD::GetUnconnectedLayerMode ), groupPad );

        propMgr.AddProperty( new PROPERTY<PAD, int>( _HKI( "Pad To Die Length" ),
                    &PAD::SetPadToDieLength, &PAD::GetPadToDieLength, PROPERTY_DISPLAY::PT_SIZE ), groupPad )
                .SetAvailableFunc( isCopperPad );

        propMgr.AddProperty( new PROPERTY<PAD, int>( _HKI( "Pad To Die Delay" ),
                    &PAD::SetPadToDieDelay, &PAD::GetPadToDieDelay, PROPERTY_DISPLAY::PT_TIME ), groupPad )
                .SetAvailableFunc( isCopperPad );

        const wxString groupOverrides = _HKI( "Overrides" );

        propMgr.AddProperty( new PROPERTY<PAD, std::optional<int>>( _HKI( "Clearance Override" ),
                    &PAD::SetLocalClearance, &PAD::GetLocalClearance, PROPERTY_DISPLAY::PT_SIZE ), groupOverrides );

        propMgr.AddProperty( new PROPERTY<PAD, std::optional<int>>( _HKI( "Soldermask Margin Override" ),
                    &PAD::SetLocalSolderMaskMargin, &PAD::GetLocalSolderMaskMargin, PROPERTY_DISPLAY::PT_SIZE ),
                    groupOverrides );

        propMgr.AddProperty( new PROPERTY<PAD, std::optional<int>>( _HKI( "Solderpaste Margin Override" ),
                    &PAD::SetLocalSolderPasteMargin, &PAD::GetLocalSolderPasteMargin, PROPERTY_DISPLAY::PT_SIZE ),
                    groupOverrides );

        propMgr.AddProperty( new PROPERTY<PAD, std::optional<double>>( _HKI( "Solderpaste Margin Ratio Override" ),
                    &PAD::SetLocalSolderPasteMarginRatio, &PAD::GetLocalSolderPasteMarginRatio,
                    PROPERTY_DISPLAY::PT_RATIO ), groupOverrides );

        propMgr.AddProperty( new PROPERTY_ENUM<PAD, ZONE_CONNECTION>( _HKI( "Zone Connection Style" ),
                    &PAD::SetLocalZoneConnection, &PAD::GetLocalZoneConnection ), groupOverrides );

        constexpr int minZoneWidth = pcbIUScale.mmToIU( ZONE_THICKNESS_MIN_VALUE_MM );

        propMgr.AddProperty( new PROPERTY<PAD, std::optional<int>>( _HKI( "Thermal Relief Spoke Width" ),
                    &PAD::SetLocalThermalSpokeWidthOverride, &PAD::GetLocalThermalSpokeWidthOverride,
                    PROPERTY_DISPLAY::PT_SIZE ), groupOverrides )
                .SetValidator( PROPERTY_VALIDATORS::RangeIntValidator<minZoneWidth, INT_MAX> );

        propMgr.AddProperty( new PROPERTY<PAD, double>( _HKI( "Thermal Relief Spoke Angle" ),
                    &PAD::SetThermalSpokeAngleDegrees, &PAD::GetThermalSpokeAngleDegrees,
                    PROPERTY_DISPLAY::PT_DEGREE ), groupOverrides );

        propMgr.AddProperty( new PROPERTY<PAD, std::optional<int>>( _HKI( "Thermal Relief Gap" ),
                    &PAD::SetLocalThermalGapOverride, &PAD::GetLocalThermalGapOverride,
                    PROPERTY_DISPLAY::PT_SIZE ), groupOverrides )
                .SetValidator( PROPERTY_VALIDATORS::PositiveIntValidator );

        // TODO delta, drill shape offset, layer set
    }
} _PAD_DESC;

ENUM_TO_WXANY( PAD_ATTRIB );
ENUM_TO_WXANY( PAD_SHAPE );
ENUM_TO_WXANY( PAD_PROP );
ENUM_TO_WXANY( PAD_DRILL_SHAPE );
