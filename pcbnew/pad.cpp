/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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

#include <base_units.h>
#include <bitmaps.h>
#include <core/mirror.h>
#include <math/util.h>      // for KiROUND
#include <eda_draw_frame.h>
#include <geometry/shape_circle.h>
#include <geometry/shape_segment.h>
#include <geometry/shape_simple.h>
#include <geometry/shape_rect.h>
#include <geometry/shape_compound.h>
#include <geometry/shape_null.h>
#include <string_utils.h>
#include <i18n_utility.h>
#include <view/view.h>
#include <board.h>
#include <board_connected_item.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_shape.h>
#include <connectivity/connectivity_data.h>
#include <eda_units.h>
#include <convert_basic_shapes_to_polygon.h>
#include <widgets/msgpanel.h>
#include <pcb_painter.h>
#include <properties/property_validators.h>
#include <wx/log.h>

#include <memory>
#include <macros.h>
#include <drc/drc_item.h>
#include "kiface_base.h"
#include "pcbnew_settings.h"

using KIGFX::PCB_PAINTER;
using KIGFX::PCB_RENDER_SETTINGS;


PAD::PAD( FOOTPRINT* parent ) :
    BOARD_CONNECTED_ITEM( parent, PCB_PAD_T )
{
    m_size.x = m_size.y = EDA_UNIT_UTILS::Mils2IU( pcbIUScale, 60 ); // Default pad size 60 mils.
    m_drill.x = m_drill.y = EDA_UNIT_UTILS::Mils2IU( pcbIUScale, 30 );       // Default drill size 30 mils.
    m_orient              = ANGLE_0;
    m_lengthPadToDie      = 0;

    if( m_parent && m_parent->Type() == PCB_FOOTPRINT_T )
        m_pos = GetParent()->GetPosition();

    SetShape( PAD_SHAPE::CIRCLE );               // Default pad shape is PAD_CIRCLE.
    SetAnchorPadShape( PAD_SHAPE::CIRCLE );      // Default shape for custom shaped pads
                                                 // is PAD_CIRCLE.
    SetDrillShape( PAD_DRILL_SHAPE_CIRCLE );     // Default pad drill shape is a circle.
    m_attribute           = PAD_ATTRIB::PTH;     // Default pad type is plated through hole
    SetProperty( PAD_PROP::NONE );               // no special fabrication property
    m_localClearance      = 0;
    m_localSolderMaskMargin  = 0;
    m_localSolderPasteMargin = 0;
    m_localSolderPasteMarginRatio = 0.0;

    // Parameters for round rect only:
    m_roundedCornerScale = 0.25;                 // from IPC-7351C standard

    // Parameters for chamfered rect only:
    m_chamferScale = 0.2;                        // Size of chamfer: ratio of smallest of X,Y size
    m_chamferPositions  = RECT_NO_CHAMFER;       // No chamfered corner

    m_zoneConnection    = ZONE_CONNECTION::INHERITED; // Use parent setting by default
    m_thermalSpokeWidth = 0;                          // Use parent setting by default
    m_thermalSpokeAngle = ANGLE_45;                   // Default for circular pads
    m_thermalGap        = 0;                          // Use parent setting by default

    m_customShapeClearanceArea = CUST_PAD_SHAPE_IN_ZONE_OUTLINE;

    // Set layers mask to default for a standard thru hole pad.
    m_layerMask = PTHMask();

    SetSubRatsnest( 0 );                       // used in ratsnest calculations

    SetDirty();
    m_effectiveBoundingRadius = 0;
    m_removeUnconnectedLayer = false;
    m_keepTopBottomLayer = true;

    m_zoneLayerOverrides.fill( ZLO_NONE );
}


PAD::PAD( const PAD& aOther ) :
    BOARD_CONNECTED_ITEM( aOther.GetParent(), PCB_PAD_T )
{
    PAD::operator=( aOther );

    const_cast<KIID&>( m_Uuid ) = aOther.m_Uuid;
}


PAD& PAD::operator=( const PAD &aOther )
{
    BOARD_CONNECTED_ITEM::operator=( aOther );

    ImportSettingsFrom( aOther );
    SetPadToDieLength( aOther.GetPadToDieLength() );
    SetPosition( aOther.GetPosition() );
    SetNumber( aOther.GetNumber() );
    SetPinType( aOther.GetPinType() );
    SetPinFunction( aOther.GetPinFunction() );
    SetSubRatsnest( aOther.GetSubRatsnest() );
    m_effectiveBoundingRadius = aOther.m_effectiveBoundingRadius;
    m_removeUnconnectedLayer = aOther.m_removeUnconnectedLayer;
    m_keepTopBottomLayer = aOther.m_keepTopBottomLayer;

    return *this;
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
    static LSET saved = LSET::AllCuMask() | LSET( 2, F_Mask, B_Mask );
    return saved;
}


LSET PAD::SMDMask()
{
    static LSET saved( 3, F_Cu, F_Paste, F_Mask );
    return saved;
}


LSET PAD::ConnSMDMask()
{
    static LSET saved( 2, F_Cu, F_Mask );
    return saved;
}


LSET PAD::UnplatedHoleMask()
{
    static LSET saved = LSET( 4, F_Cu, B_Cu, F_Mask, B_Mask );
    return saved;
}


LSET PAD::ApertureMask()
{
    static LSET saved( 1, F_Paste );
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


bool PAD::FlashLayer( LSET aLayers ) const
{
    for( PCB_LAYER_ID layer : aLayers.Seq() )
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

    if( !IsOnLayer( static_cast<PCB_LAYER_ID>( aLayer ) ) )
        return false;

    if( GetAttribute() == PAD_ATTRIB::NPTH && IsCopperLayer( aLayer ) )
    {
        if( GetShape() == PAD_SHAPE::CIRCLE && GetDrillShape() == PAD_DRILL_SHAPE_CIRCLE )
        {
            if( GetOffset() == VECTOR2I( 0, 0 ) && GetDrillSize().x >= GetSize().x )
                return false;
        }
        else if( GetShape() == PAD_SHAPE::OVAL && GetDrillShape() == PAD_DRILL_SHAPE_OBLONG )
        {
            if( GetOffset() == VECTOR2I( 0, 0 )
                    && GetDrillSize().x >= GetSize().x && GetDrillSize().y >= GetSize().y )
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
        /// Heat sink pads always get copper
        if( GetProperty() == PAD_PROP::HEATSINK )
            return true;

        if( !m_removeUnconnectedLayer )
            return true;

        // Plated through hole pads need copper on the top/bottom layers for proper soldering
        // Unless the user has removed them in the pad dialog
        if( m_keepTopBottomLayer && ( aLayer == F_Cu || aLayer == B_Cu ) )
            return true;

        if( const BOARD* board = GetBoard() )
        {
            // Must be static to keep from raising its ugly head in performance profiles
            static std::initializer_list<KICAD_T> types = { PCB_TRACE_T, PCB_ARC_T, PCB_VIA_T,
                                                            PCB_PAD_T };

            if( m_zoneLayerOverrides[ aLayer ] == ZLO_FORCE_FLASHED )
                return true;
            else if( aOnlyCheckIfPermitted )
                return true;
            else
                return board->GetConnectivity()->IsConnectedOnLayer( this, aLayer, types );
        }
    }

    return true;
}


void PAD::SetDrillSizeX( const int aX )
{
    m_drill.x = aX;

    if( GetDrillShape() == PAD_DRILL_SHAPE_CIRCLE )
        SetDrillSizeY( aX );

    SetDirty();
}


void PAD::SetDrillShape( PAD_DRILL_SHAPE_T aShape )
{
    m_drillShape = aShape;

    if( aShape == PAD_DRILL_SHAPE_CIRCLE )
        SetDrillSizeY( GetDrillSizeX() );

    m_shapesDirty = true;
}


int PAD::GetRoundRectCornerRadius() const
{
    return KiROUND( std::min( m_size.x, m_size.y ) * m_roundedCornerScale );
}


void PAD::SetRoundRectCornerRadius( double aRadius )
{
    int min_r = std::min( m_size.x, m_size.y );

    if( min_r > 0 )
        SetRoundRectRadiusRatio( aRadius / min_r );
}


void PAD::SetRoundRectRadiusRatio( double aRadiusScale )
{
    m_roundedCornerScale = alg::clamp( 0.0, aRadiusScale, 0.5 );

    SetDirty();
}


void PAD::SetChamferRectRatio( double aChamferScale )
{
    m_chamferScale = alg::clamp( 0.0, aChamferScale, 0.5 );

    SetDirty();
}


const std::shared_ptr<SHAPE_POLY_SET>& PAD::GetEffectivePolygon( ERROR_LOC aErrorLoc ) const
{
    if( m_polyDirty[ aErrorLoc ] )
        BuildEffectivePolygon( aErrorLoc );

    return m_effectivePolygon[ aErrorLoc ];
}


std::shared_ptr<SHAPE> PAD::GetEffectiveShape( PCB_LAYER_ID aLayer, FLASHING flashPTHPads ) const
{
    if( aLayer == Edge_Cuts )
    {
        if( GetAttribute() == PAD_ATTRIB::PTH || GetAttribute() == PAD_ATTRIB::NPTH )
            return GetEffectiveHoleShape();
        else
            return std::make_shared<SHAPE_NULL>();
    }

    if( GetAttribute() == PAD_ATTRIB::PTH )
    {
        bool flash;

        if( flashPTHPads == FLASHING::NEVER_FLASHED )
            flash = false;
        else if( flashPTHPads == FLASHING::ALWAYS_FLASHED )
            flash = true;
        else
            flash = FlashLayer( aLayer );

        if( !flash )
        {
            if( GetAttribute() == PAD_ATTRIB::PTH )
                return GetEffectiveHoleShape();
            else
                return std::make_shared<SHAPE_NULL>();
        }
    }

    if( m_shapesDirty )
        BuildEffectiveShapes( aLayer );

    return m_effectiveShape;
}


std::shared_ptr<SHAPE_SEGMENT> PAD::GetEffectiveHoleShape() const
{
    if( m_shapesDirty )
        BuildEffectiveShapes( UNDEFINED_LAYER );

    return m_effectiveHoleShape;
}


int PAD::GetBoundingRadius() const
{
    if( m_polyDirty[ ERROR_OUTSIDE ] )
        BuildEffectivePolygon( ERROR_OUTSIDE );

    return m_effectiveBoundingRadius;
}


void PAD::BuildEffectiveShapes( PCB_LAYER_ID aLayer ) const
{
    std::lock_guard<std::mutex> RAII_lock( m_shapesBuildingLock );

    // If we had to wait for the lock then we were probably waiting for someone else to
    // finish rebuilding the shapes.  So check to see if they're clean now.
    if( !m_shapesDirty )
        return;

    const BOARD* board = GetBoard();
    int          maxError = board ? board->GetDesignSettings().m_MaxError : ARC_HIGH_DEF;

    m_effectiveShape = std::make_shared<SHAPE_COMPOUND>();
    m_effectiveHoleShape = nullptr;

    auto add = [this]( SHAPE* aShape )
               {
                   m_effectiveShape->AddShape( aShape );
               };

    VECTOR2I  shapePos = ShapePos(); // Fetch only once; rotation involves trig
    PAD_SHAPE effectiveShape = GetShape();

    if( GetShape() == PAD_SHAPE::CUSTOM )
        effectiveShape = GetAnchorPadShape();

    switch( effectiveShape )
    {
    case PAD_SHAPE::CIRCLE:
        add( new SHAPE_CIRCLE( shapePos, m_size.x / 2 ) );
        break;

    case PAD_SHAPE::OVAL:
        if( m_size.x == m_size.y ) // the oval pad is in fact a circle
        {
            add( new SHAPE_CIRCLE( shapePos, m_size.x / 2 ) );
        }
        else
        {
            VECTOR2I half_size = m_size / 2;
            int     half_width = std::min( half_size.x, half_size.y );
            VECTOR2I half_len( half_size.x - half_width, half_size.y - half_width );
            RotatePoint( half_len, m_orient );
            add( new SHAPE_SEGMENT( shapePos - half_len, shapePos + half_len, half_width * 2 ) );
        }

        break;

    case PAD_SHAPE::RECTANGLE:
    case PAD_SHAPE::TRAPEZOID:
    case PAD_SHAPE::ROUNDRECT:
    {
        int      r = ( effectiveShape == PAD_SHAPE::ROUNDRECT ) ? GetRoundRectCornerRadius() : 0;
        VECTOR2I half_size( m_size.x / 2, m_size.y / 2 );
        VECTOR2I trap_delta( 0, 0 );

        if( r )
        {
            half_size -= VECTOR2I( r, r );

            // Avoid degenerated shapes (0 length segments) that always create issues
            // For roundrect pad very near a circle, use only a circle
            const int min_len = pcbIUScale.mmToIU( 0.0001);

            if( half_size.x < min_len && half_size.y < min_len )
            {
                add( new SHAPE_CIRCLE( shapePos, r ) );
                break;
            }
        }
        else if( effectiveShape == PAD_SHAPE::TRAPEZOID )
        {
            trap_delta = m_deltaSize / 2;
        }

        SHAPE_LINE_CHAIN corners;

        corners.Append( -half_size.x - trap_delta.y,  half_size.y + trap_delta.x );
        corners.Append(  half_size.x + trap_delta.y,  half_size.y - trap_delta.x );
        corners.Append(  half_size.x - trap_delta.y, -half_size.y + trap_delta.x );
        corners.Append( -half_size.x + trap_delta.y, -half_size.y - trap_delta.x );

        corners.Rotate( m_orient );
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

        TransformRoundChamferedRectToPolygon( outline, shapePos, GetSize(), m_orient,
                                              GetRoundRectCornerRadius(), GetChamferRectRatio(),
                                              GetChamferPositions(), 0, maxError, ERROR_INSIDE );

        add( new SHAPE_SIMPLE( outline.COutline( 0 ) ) );
    }
        break;

    default:
        wxFAIL_MSG( wxT( "PAD::buildEffectiveShapes: Unsupported pad shape: " )
                    + PAD_SHAPE_T_asString( effectiveShape ) );
        break;
    }

    if( GetShape() == PAD_SHAPE::CUSTOM )
    {
        for( const std::shared_ptr<PCB_SHAPE>& primitive : m_editPrimitives )
        {
            if( !primitive->IsProxyItem() )
            {
                for( SHAPE* shape : primitive->MakeEffectiveShapes() )
                {
                    shape->Rotate( m_orient );
                    shape->Move( shapePos );
                    add( shape );
                }
            }
        }
    }

    m_effectiveBoundingBox = m_effectiveShape->BBox();

    // Hole shape
    VECTOR2I half_size = m_drill / 2;
    int      half_width = std::min( half_size.x, half_size.y );
    VECTOR2I half_len( half_size.x - half_width, half_size.y - half_width );

    RotatePoint( half_len, m_orient );

    m_effectiveHoleShape = std::make_shared<SHAPE_SEGMENT>( m_pos - half_len, m_pos + half_len,
                                                            half_width * 2 );
    m_effectiveBoundingBox.Merge( m_effectiveHoleShape->BBox() );

    // All done
    m_shapesDirty = false;
}


void PAD::BuildEffectivePolygon( ERROR_LOC aErrorLoc ) const
{
    std::lock_guard<std::mutex> RAII_lock( m_polyBuildingLock );

    // If we had to wait for the lock then we were probably waiting for someone else to
    // finish rebuilding the shapes.  So check to see if they're clean now.
    if( !m_polyDirty[ aErrorLoc ] )
        return;

    const BOARD* board = GetBoard();
    int          maxError = board ? board->GetDesignSettings().m_MaxError : ARC_HIGH_DEF;

    // Polygon
    std::shared_ptr<SHAPE_POLY_SET>& effectivePolygon = m_effectivePolygon[ aErrorLoc ];

    effectivePolygon = std::make_shared<SHAPE_POLY_SET>();
    TransformShapeToPolygon( *effectivePolygon, UNDEFINED_LAYER, 0, maxError, aErrorLoc );

    // Bounding radius
    //
    // PADSTACKS TODO: these will both need to cycle through all layers to get the largest
    // values....
    if( aErrorLoc == ERROR_OUTSIDE )
    {
        m_effectiveBoundingRadius = 0;

        for( int cnt = 0; cnt < effectivePolygon->OutlineCount(); ++cnt )
        {
            const SHAPE_LINE_CHAIN& poly = effectivePolygon->COutline( cnt );

            for( int ii = 0; ii < poly.PointCount(); ++ii )
            {
                int dist = KiROUND( ( poly.CPoint( ii ) - m_pos ).EuclideanNorm() );
                m_effectiveBoundingRadius = std::max( m_effectiveBoundingRadius, dist );
            }
        }
    }

    // All done
    m_polyDirty[ aErrorLoc ] = false;
}


const BOX2I PAD::GetBoundingBox() const
{
    if( m_shapesDirty )
        BuildEffectiveShapes( UNDEFINED_LAYER );

    return m_effectiveBoundingBox;
}


void PAD::SetAttribute( PAD_ATTRIB aAttribute )
{
    if( m_attribute != aAttribute )
    {
        m_attribute = aAttribute;

        switch( aAttribute )
        {
        case PAD_ATTRIB::PTH:
            // Plump up to all copper layers
            m_layerMask |= LSET::AllCuMask();
            break;

        case PAD_ATTRIB::SMD:
        case PAD_ATTRIB::CONN:
        {
            // Trim down to no more than one copper layer
            LSET copperLayers = m_layerMask & LSET::AllCuMask();

            if( copperLayers.count() > 1 )
            {
                m_layerMask &= ~LSET::AllCuMask();

                if( copperLayers.test( B_Cu ) )
                    m_layerMask.set( B_Cu );
                else
                    m_layerMask.set( copperLayers.Seq().front() );
            }

            // No hole
            m_drill = VECTOR2I( 0, 0 );
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


void PAD::SetProperty( PAD_PROP aProperty )
{
    m_property = aProperty;

    SetDirty();
}


void PAD::SetOrientation( const EDA_ANGLE& aAngle )
{
    m_orient = aAngle;
    m_orient.Normalize();

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


void PAD::Flip( const VECTOR2I& aCentre, bool aFlipLeftRight )
{
    if( aFlipLeftRight )
    {
        MIRROR( m_pos.x, aCentre.x );
        MIRROR( m_offset.x, 0 );
        MIRROR( m_deltaSize.x, 0 );
    }
    else
    {
        MIRROR( m_pos.y, aCentre.y );
        MIRROR( m_offset.y, 0 );
        MIRROR( m_deltaSize.y, 0 );
    }

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

    if( aFlipLeftRight )
    {
        mirrorBitFlags( m_chamferPositions, RECT_CHAMFER_TOP_LEFT, RECT_CHAMFER_TOP_RIGHT );
        mirrorBitFlags( m_chamferPositions, RECT_CHAMFER_BOTTOM_LEFT, RECT_CHAMFER_BOTTOM_RIGHT );
    }
    else
    {
        mirrorBitFlags( m_chamferPositions, RECT_CHAMFER_TOP_LEFT, RECT_CHAMFER_BOTTOM_LEFT );
        mirrorBitFlags( m_chamferPositions, RECT_CHAMFER_TOP_RIGHT, RECT_CHAMFER_BOTTOM_RIGHT );
    }

    // flip pads layers
    // PADS items are currently on all copper layers, or
    // currently, only on Front or Back layers.
    // So the copper layers count is not taken in account
    SetLayerSet( FlipLayerMask( m_layerMask ) );

    // Flip the basic shapes, in custom pads
    FlipPrimitives( aFlipLeftRight );

    SetDirty();
}


void PAD::FlipPrimitives( bool aFlipLeftRight )
{
    for( std::shared_ptr<PCB_SHAPE>& primitive : m_editPrimitives )
        primitive->Flip( VECTOR2I( 0, 0 ), aFlipLeftRight );

    SetDirty();
}


VECTOR2I PAD::ShapePos() const
{
    if( m_offset.x == 0 && m_offset.y == 0 )
        return m_pos;

    VECTOR2I loc_offset = m_offset;

    RotatePoint( loc_offset, m_orient );

    VECTOR2I shape_pos = m_pos + loc_offset;

    return shape_pos;
}


bool PAD::IsOnCopperLayer() const
{
    if( GetAttribute() == PAD_ATTRIB::NPTH )
    {
        // NPTH pads have no plated hole cylinder.  If their annular ring size is 0 or
        // negative, then they have no annular ring either.

        switch( GetShape() )
        {
        case PAD_SHAPE::CIRCLE:
            if( m_offset == VECTOR2I( 0, 0 ) && m_size.x <= m_drill.x )
                return false;

            break;

        case PAD_SHAPE::OVAL:
            if( m_offset == VECTOR2I( 0, 0 ) && m_size.x <= m_drill.x && m_size.y <= m_drill.y )
                return false;

            break;

        default:
            // We could subtract the hole polygon from the shape polygon for these, but it
            // would be expensive and we're probably well out of the common use cases....
            break;
        }
    }

    return ( GetLayerSet() & LSET::AllCuMask() ).any();
}


int PAD::GetLocalClearanceOverrides( wxString* aSource ) const
{
    // A pad can have specific clearance that overrides its NETCLASS clearance value
    if( GetLocalClearance() )
        return GetLocalClearance( aSource );

    // A footprint can have a specific clearance value
    if( GetParentFootprint() && GetParentFootprint()->GetLocalClearance() )
        return GetParentFootprint()->GetLocalClearance( aSource );

    return 0;
}


int PAD::GetLocalClearance( wxString* aSource ) const
{
    if( aSource )
        *aSource = _( "pad" );

    return m_localClearance;
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


int PAD::GetSolderMaskExpansion() const
{
    // Pads defined only on mask layers (and perhaps on other tech layers) use the shape
    // defined by the pad settings only.  ALL other pads, even those that don't actually have
    // any copper (such as NPTH pads with holes the same size as the pad) get mask expansion.
    if( ( m_layerMask & LSET::AllCuMask() ).none() )
        return 0;

    int margin = m_localSolderMaskMargin;

    if( FOOTPRINT* parentFootprint = GetParentFootprint() )
    {
        if( margin == 0 )
        {
            if( parentFootprint->GetLocalSolderMaskMargin() )
                margin = parentFootprint->GetLocalSolderMaskMargin();
        }

        if( margin == 0 )
        {
            const BOARD* brd = GetBoard();

            if( brd )
                margin = brd->GetDesignSettings().m_SolderMaskExpansion;
        }
    }

    // ensure mask have a size always >= 0
    if( margin < 0 )
    {
        int minsize = -std::min( m_size.x, m_size.y ) / 2;

        if( margin < minsize )
            margin = minsize;
    }

    return margin;
}


VECTOR2I PAD::GetSolderPasteMargin() const
{
    // Pads defined only on mask layers (and perhaps on other tech layers) use the shape
    // defined by the pad settings only.  ALL other pads, even those that don't actually have
    // any copper (such as NPTH pads with holes the same size as the pad) get paste expansion.
    if( ( m_layerMask & LSET::AllCuMask() ).none() )
        return VECTOR2I( 0, 0 );

    int     margin = m_localSolderPasteMargin;
    double  mratio = m_localSolderPasteMarginRatio;

    if( FOOTPRINT* parentFootprint = GetParentFootprint() )
    {
        if( margin == 0 )
            margin = parentFootprint->GetLocalSolderPasteMargin();

        auto brd = GetBoard();

        if( margin == 0 && brd )
            margin = brd->GetDesignSettings().m_SolderPasteMargin;

        if( mratio == 0.0 )
            mratio = parentFootprint->GetLocalSolderPasteMarginRatio();

        if( mratio == 0.0 && brd )
        {
            mratio = brd->GetDesignSettings().m_SolderPasteMarginRatio;
        }
    }

    VECTOR2I pad_margin;
    pad_margin.x = margin + KiROUND( m_size.x * mratio );
    pad_margin.y = margin + KiROUND( m_size.y * mratio );

    // ensure mask have a size always >= 0
    if( m_padShape != PAD_SHAPE::CUSTOM )
    {
        if( pad_margin.x < -m_size.x / 2 )
            pad_margin.x = -m_size.x / 2;

        if( pad_margin.y < -m_size.y / 2 )
            pad_margin.y = -m_size.y / 2;
    }

    return pad_margin;
}


ZONE_CONNECTION PAD::GetLocalZoneConnectionOverride( wxString* aSource ) const
{
    if( m_zoneConnection != ZONE_CONNECTION::INHERITED && aSource )
        *aSource = _( "pad" );

    return m_zoneConnection;
}


int PAD::GetLocalSpokeWidthOverride( wxString* aSource ) const
{
    if( m_thermalSpokeWidth > 0 && aSource )
        *aSource = _( "pad" );

    return m_thermalSpokeWidth;
}


int PAD::GetLocalThermalGapOverride( wxString* aSource ) const
{
    if( m_thermalGap > 0 && aSource )
        *aSource = _( "pad" );

    return m_thermalGap;
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
                            UnescapeString( GetEffectiveNetClass()->GetName() ) );

        if( IsLocked() )
            aList.emplace_back( _( "Status" ), _( "Locked" ) );
    }

    if( GetAttribute() == PAD_ATTRIB::SMD || GetAttribute() == PAD_ATTRIB::CONN )
        aList.emplace_back( _( "Layer" ), layerMaskDescribe() );

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
    }

    aList.emplace_back( ShowPadShape(), props );

    if( ( GetShape() == PAD_SHAPE::CIRCLE || GetShape() == PAD_SHAPE::OVAL )
            && m_size.x == m_size.y )
    {
        aList.emplace_back( _( "Diameter" ), aFrame->MessageTextFromValue( m_size.x ) );
    }
    else
    {
        aList.emplace_back( _( "Width" ), aFrame->MessageTextFromValue( m_size.x ) );
        aList.emplace_back( _( "Height" ), aFrame->MessageTextFromValue( m_size.y ) );
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

    if( m_drill.x > 0 || m_drill.y > 0 )
    {
        if( GetDrillShape() == PAD_DRILL_SHAPE_CIRCLE )
        {
            aList.emplace_back( _( "Hole" ),
                                wxString::Format( wxT( "%s" ),
                                                  aFrame->MessageTextFromValue( m_drill.x ) ) );
        }
        else
        {
            aList.emplace_back( _( "Hole X / Y" ),
                                wxString::Format( wxT( "%s / %s" ),
                                                  aFrame->MessageTextFromValue( m_drill.x ),
                                                  aFrame->MessageTextFromValue( m_drill.y ) ) );
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

    return GetEffectivePolygon( ERROR_INSIDE )->Contains( aPosition, -1, aAccuracy );
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

        const std::shared_ptr<SHAPE_POLY_SET>& poly = GetEffectivePolygon( ERROR_INSIDE );

        int count = poly->TotalVertices();

        for( int ii = 0; ii < count; ii++ )
        {
            VECTOR2I vertex = poly->CVertex( ii );
            VECTOR2I vertexNext = poly->CVertex( ( ii + 1 ) % count );

            // Test if the point is within aRect
            if( arect.Contains( vertex ) )
                return true;

            // Test if this edge intersects aRect
            if( arect.Intersects( vertex, vertexNext ) )
                return true;
        }

        return false;
    }
}


int PAD::Compare( const PAD* aPadRef, const PAD* aPadCmp )
{
    int diff;

    if( ( diff = static_cast<int>( aPadRef->GetShape() ) -
          static_cast<int>( aPadCmp->GetShape() ) ) != 0 )
        return diff;

    if( ( diff = static_cast<int>( aPadRef->m_attribute ) -
          static_cast<int>( aPadCmp->m_attribute ) ) != 0 )
        return diff;

    if( ( diff = aPadRef->m_drillShape - aPadCmp->m_drillShape ) != 0 )
        return diff;

    if( ( diff = aPadRef->m_drill.x - aPadCmp->m_drill.x ) != 0 )
        return diff;

    if( ( diff = aPadRef->m_drill.y - aPadCmp->m_drill.y ) != 0 )
        return diff;

    if( ( diff = aPadRef->m_size.x - aPadCmp->m_size.x ) != 0 )
        return diff;

    if( ( diff = aPadRef->m_size.y - aPadCmp->m_size.y ) != 0 )
        return diff;

    if( ( diff = aPadRef->m_offset.x - aPadCmp->m_offset.x ) != 0 )
        return diff;

    if( ( diff = aPadRef->m_offset.y - aPadCmp->m_offset.y ) != 0 )
        return diff;

    if( ( diff = aPadRef->m_deltaSize.x - aPadCmp->m_deltaSize.x ) != 0 )
        return diff;

    if( ( diff = aPadRef->m_deltaSize.y - aPadCmp->m_deltaSize.y ) != 0 )
        return diff;

    if( ( diff = aPadRef->m_roundedCornerScale - aPadCmp->m_roundedCornerScale ) != 0 )
        return diff;

    if( ( diff = aPadRef->m_chamferPositions - aPadCmp->m_chamferPositions ) != 0 )
        return diff;

    if( ( diff = aPadRef->m_chamferScale - aPadCmp->m_chamferScale ) != 0 )
        return diff;

    if( ( diff = static_cast<int>( aPadRef->m_editPrimitives.size() ) -
          static_cast<int>( aPadCmp->m_editPrimitives.size() ) ) != 0 )
        return diff;

    // @todo: Compare custom pad primitives for pads that have the same number of primitives
    //        here.  Currently there is no compare function for PCB_SHAPE objects.

    // Dick: specctra_export needs this
    // Lorenzo: gencad also needs it to implement padstacks!

#if __cplusplus >= 201103L
    long long d = aPadRef->m_layerMask.to_ullong() - aPadCmp->m_layerMask.to_ullong();

    if( d < 0 )
        return -1;
    else if( d > 0 )
        return 1;

    return 0;
#else
    // these strings are not typically constructed, since we don't get here often.
    std::string s1 = aPadRef->m_layerMask.to_string();
    std::string s2 = aPadCmp->m_layerMask.to_string();
    return s1.compare( s2 );
#endif
}


void PAD::Rotate( const VECTOR2I& aRotCentre, const EDA_ANGLE& aAngle )
{
    RotatePoint( m_pos, aRotCentre, aAngle );

    m_orient += aAngle;
    m_orient.Normalize();

    SetDirty();
}


wxString PAD::ShowPadShape() const
{
    switch( GetShape() )
    {
    case PAD_SHAPE::CIRCLE:         return _( "Circle" );
    case PAD_SHAPE::OVAL:           return _( "Oval" );
    case PAD_SHAPE::RECTANGLE:           return _( "Rect" );
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


wxString PAD::GetItemDescription( UNITS_PROVIDER* aUnitsProvider ) const
{
    if( GetNumber().IsEmpty() )
    {
        if( GetAttribute() == PAD_ATTRIB::SMD || GetAttribute() == PAD_ATTRIB::CONN )
        {
            return wxString::Format( _( "Pad %s of %s on %s" ),
                                     GetNetnameMsg(),
                                     GetParentFootprint()->GetReference(),
                                     layerMaskDescribe() );
        }
        else if( GetAttribute() == PAD_ATTRIB::NPTH )
        {
            return wxString::Format( _( "NPTH pad of %s" ), GetParentFootprint()->GetReference() );
        }
        else
        {
            return wxString::Format( _( "PTH pad %s of %s" ),
                                     GetNetnameMsg(),
                                     GetParentFootprint()->GetReference() );
        }
    }
    else
    {
        if( GetAttribute() == PAD_ATTRIB::SMD || GetAttribute() == PAD_ATTRIB::CONN )
        {
            return wxString::Format( _( "Pad %s %s of %s on %s" ),
                                     GetNumber(),
                                     GetNetnameMsg(),
                                     GetParentFootprint()->GetReference(),
                                     layerMaskDescribe() );
        }
        else if( GetAttribute() == PAD_ATTRIB::NPTH )
        {
            return wxString::Format( _( "NPTH of %s" ), GetParentFootprint()->GetReference() );
        }
        else
        {
            return wxString::Format( _( "PTH pad %s %s of %s" ),
                                     GetNumber(),
                                     GetNetnameMsg(),
                                     GetParentFootprint()->GetReference() );
        }
    }
}


BITMAPS PAD::GetMenuImage() const
{
    return BITMAPS::pad;
}


EDA_ITEM* PAD::Clone() const
{
    return new PAD( *this );
}


void PAD::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount = 0;

    // These 2 types of pads contain a hole
    if( m_attribute == PAD_ATTRIB::PTH )
    {
        aLayers[aCount++] = LAYER_PAD_PLATEDHOLES;
        aLayers[aCount++] = LAYER_PAD_HOLEWALLS;
    }

    if( m_attribute == PAD_ATTRIB::NPTH )
        aLayers[aCount++] = LAYER_NON_PLATEDHOLES;

    if( IsOnLayer( F_Cu ) && IsOnLayer( B_Cu ) )
    {
        // Multi layer pad
        aLayers[aCount++] = LAYER_PADS_TH;
        aLayers[aCount++] = LAYER_PAD_NETNAMES;
    }
    else if( IsOnLayer( F_Cu ) )
    {
        aLayers[aCount++] = LAYER_PADS_SMD_FR;

        // Is this a PTH pad that has only front copper?  If so, we need to also display the
        // net name on the PTH netname layer so that it isn't blocked by the drill hole.
        if( m_attribute == PAD_ATTRIB::PTH )
            aLayers[aCount++] = LAYER_PAD_NETNAMES;
        else
            aLayers[aCount++] = LAYER_PAD_FR_NETNAMES;
    }
    else if( IsOnLayer( B_Cu ) )
    {
        aLayers[aCount++] = LAYER_PADS_SMD_BK;

        // Is this a PTH pad that has only back copper?  If so, we need to also display the
        // net name on the PTH netname layer so that it isn't blocked by the drill hole.
        if( m_attribute == PAD_ATTRIB::PTH )
            aLayers[aCount++] = LAYER_PAD_NETNAMES;
        else
            aLayers[aCount++] = LAYER_PAD_BK_NETNAMES;
    }
    else
    {
        // Internal layers only.  (Not yet supported in GUI, but is being used by Python
        // footprint generators and will be needed anyway once pad stacks are supported.)
        for ( int internal = In1_Cu; internal < In30_Cu; ++internal )
        {
            if( IsOnLayer( (PCB_LAYER_ID) internal ) )
                aLayers[aCount++] = internal;
        }
    }

    // Check non-copper layers. This list should include all the layers that the
    // footprint editor allows a pad to be placed on.
    static const PCB_LAYER_ID layers_mech[] = { F_Mask, B_Mask, F_Paste, B_Paste,
        F_Adhes, B_Adhes, F_SilkS, B_SilkS, Dwgs_User, Eco1_User, Eco2_User };

    for( PCB_LAYER_ID each_layer : layers_mech )
    {
        if( IsOnLayer( each_layer ) )
            aLayers[aCount++] = each_layer;
    }
}


double PAD::ViewGetLOD( int aLayer, KIGFX::VIEW* aView ) const
{
    constexpr double HIDE = std::numeric_limits<double>::max();

    PCB_PAINTER*         painter = static_cast<PCB_PAINTER*>( aView->GetPainter() );
    PCB_RENDER_SETTINGS* renderSettings = painter->GetSettings();
    const BOARD*         board = GetBoard();

    // Meta control for hiding all pads
    if( !aView->IsLayerVisible( LAYER_PADS ) )
        return HIDE;

    // Handle Render tab switches
    if( ( GetAttribute() == PAD_ATTRIB::PTH || GetAttribute() == PAD_ATTRIB::NPTH )
         && !aView->IsLayerVisible( LAYER_PADS_TH ) )
    {
        return HIDE;
    }

    if( !IsFlipped() && !aView->IsLayerVisible( LAYER_FOOTPRINTS_FR ) )
        return HIDE;

    if( IsFlipped() && !aView->IsLayerVisible( LAYER_FOOTPRINTS_BK ) )
        return HIDE;

    if( IsFrontLayer( (PCB_LAYER_ID) aLayer ) && !aView->IsLayerVisible( LAYER_PADS_SMD_FR ) )
        return HIDE;

    if( IsBackLayer( (PCB_LAYER_ID) aLayer ) && !aView->IsLayerVisible( LAYER_PADS_SMD_BK ) )
        return HIDE;

    LSET visible = board->GetVisibleLayers() & board->GetEnabledLayers();

    if( IsHoleLayer( aLayer ) )
    {
        if( !( visible & LSET::PhysicalLayersMask() ).any() )
            return HIDE;
    }
    else if( IsNetnameLayer( aLayer ) )
    {
        if( renderSettings->GetHighContrast() )
        {
            // Hide netnames unless pad is flashed to a high-contrast layer
            if( !FlashLayer( renderSettings->GetPrimaryHighContrastLayer() ) )
                return HIDE;
        }
        else
        {
            // Hide netnames unless pad is flashed to a visible layer
            if( !FlashLayer( visible ) )
                return HIDE;
        }

        // Netnames will be shown only if zoom is appropriate
        int divisor = std::min( GetBoundingBox().GetWidth(), GetBoundingBox().GetHeight() );

        // Pad sizes can be zero briefly when someone is typing a number like "0.5" in the pad
        // properties dialog
        if( divisor == 0 )
            return HIDE;

        return ( double ) pcbIUScale.mmToIU( 5 ) / divisor;
    }

    // Passed all tests; show.
    return 0.0;
}


const BOX2I PAD::ViewBBox() const
{
    // Bounding box includes soldermask too. Remember mask and/or paste margins can be < 0
    int      solderMaskMargin  = std::max( GetSolderMaskExpansion(), 0 );
    VECTOR2I solderPasteMargin = VECTOR2D( GetSolderPasteMargin() );
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
    SetShape( aMasterPad.GetShape() );
    SetLayerSet( aMasterPad.GetLayerSet() );
    SetAttribute( aMasterPad.GetAttribute() );
    SetProperty( aMasterPad.GetProperty() );

    // Must be after setting attribute and layerSet
    if( !CanHaveNumber() )
        SetNumber( wxEmptyString );

    // I am not sure the m_LengthPadToDie should be imported, because this is a parameter
    // really specific to a given pad (JPC).
#if 0
    SetPadToDieLength( aMasterPad.GetPadToDieLength() );
#endif

    // The pad orientation, for historical reasons is the pad rotation + parent rotation.
    EDA_ANGLE pad_rot = aMasterPad.GetOrientation();

    if( aMasterPad.GetParentFootprint() )
        pad_rot -= aMasterPad.GetParentFootprint()->GetOrientation();

    if( GetParentFootprint() )
        pad_rot += GetParentFootprint()->GetOrientation();

    SetOrientation( pad_rot );

    SetRemoveUnconnected( aMasterPad.GetRemoveUnconnected() );
    SetKeepTopBottom( aMasterPad.GetKeepTopBottom() );

    SetSize( aMasterPad.GetSize() );
    SetDelta( VECTOR2I( 0, 0 ) );
    SetOffset( aMasterPad.GetOffset() );
    SetDrillSize( aMasterPad.GetDrillSize() );
    SetDrillShape( aMasterPad.GetDrillShape() );
    SetRoundRectRadiusRatio( aMasterPad.GetRoundRectRadiusRatio() );
    SetChamferRectRatio( aMasterPad.GetChamferRectRatio() );
    SetChamferPositions( aMasterPad.GetChamferPositions() );

    switch( aMasterPad.GetShape() )
    {
    case PAD_SHAPE::TRAPEZOID:
        SetDelta( aMasterPad.GetDelta() );
        break;

    case PAD_SHAPE::CIRCLE:
        // ensure size.y == size.x
        SetSize( VECTOR2I( GetSize().x, GetSize().x ) );
        break;

    default:
        ;
    }

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

    SetZoneConnection( aMasterPad.GetZoneConnection() );
    SetThermalSpokeWidth( aMasterPad.GetThermalSpokeWidth() );
    SetThermalSpokeAngle( aMasterPad.GetThermalSpokeAngle() );
    SetThermalGap( aMasterPad.GetThermalGap() );

    SetCustomShapeInZoneOpt( aMasterPad.GetCustomShapeInZoneOpt() );

    m_teardropParams = aMasterPad.m_teardropParams;

    // Add or remove custom pad shapes:
    ReplacePrimitives( aMasterPad.GetPrimitives() );
    SetAnchorPadShape( aMasterPad.GetAnchorPadShape() );

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

    TransformOvalToPolygon( aBuffer, slot->GetSeg().A, slot->GetSeg().B,
                            slot->GetWidth() + aClearance * 2, aError, aErrorLoc );

    return true;
}


void PAD::TransformShapeToPolygon( SHAPE_POLY_SET& aBuffer, PCB_LAYER_ID aLayer, int aClearance,
                                   int aMaxError, ERROR_LOC aErrorLoc, bool ignoreLineWidth ) const
{
    wxASSERT_MSG( !ignoreLineWidth, wxT( "IgnoreLineWidth has no meaning for pads." ) );

    // minimal segment count to approximate a circle to create the polygonal pad shape
    // This minimal value is mainly for very small pads, like SM0402.
    // Most of time pads are using the segment count given by aError value.
    const int pad_min_seg_per_circle_count = 16;
    int       dx = m_size.x / 2;
    int       dy = m_size.y / 2;

    VECTOR2I padShapePos = ShapePos();    // Note: for pad having a shape offset, the pad
                                          //   position is NOT the shape position

    switch( GetShape() )
    {
    case PAD_SHAPE::CIRCLE:
    case PAD_SHAPE::OVAL:
        // Note: dx == dy is not guaranteed for circle pads in legacy boards
        if( dx == dy || ( GetShape() == PAD_SHAPE::CIRCLE ) )
        {
            TransformCircleToPolygon( aBuffer, padShapePos, dx + aClearance, aMaxError, aErrorLoc,
                                      pad_min_seg_per_circle_count );
        }
        else
        {
            int      half_width = std::min( dx, dy );
            VECTOR2I delta( dx - half_width, dy - half_width );

            RotatePoint( delta, m_orient );

            TransformOvalToPolygon( aBuffer, padShapePos - delta, padShapePos + delta,
                                    ( half_width + aClearance ) * 2, aMaxError, aErrorLoc,
                                    pad_min_seg_per_circle_count );
        }

        break;

    case PAD_SHAPE::TRAPEZOID:
    case PAD_SHAPE::RECTANGLE:
    {
        int  ddx = GetShape() == PAD_SHAPE::TRAPEZOID ? m_deltaSize.x / 2 : 0;
        int  ddy = GetShape() == PAD_SHAPE::TRAPEZOID ? m_deltaSize.y / 2 : 0;

        SHAPE_POLY_SET outline;
        TransformTrapezoidToPolygon( outline, padShapePos, m_size, m_orient, ddx, ddy, aClearance,
                                     aMaxError, aErrorLoc );
        aBuffer.Append( outline );
        break;
    }

    case PAD_SHAPE::CHAMFERED_RECT:
    case PAD_SHAPE::ROUNDRECT:
    {
        bool doChamfer = GetShape() == PAD_SHAPE::CHAMFERED_RECT;

        SHAPE_POLY_SET outline;
        TransformRoundChamferedRectToPolygon( outline, padShapePos, m_size, m_orient,
                                              GetRoundRectCornerRadius(),
                                              doChamfer ? GetChamferRectRatio() : 0,
                                              doChamfer ? GetChamferPositions() : 0,
                                              aClearance, aMaxError, aErrorLoc );
        aBuffer.Append( outline );
        break;
    }

    case PAD_SHAPE::CUSTOM:
    {
        SHAPE_POLY_SET outline;
        MergePrimitivesAsPolygon( &outline, aErrorLoc );
        outline.Rotate( m_orient );
        outline.Move( VECTOR2I( padShapePos ) );

        if( aClearance > 0 || aErrorLoc == ERROR_OUTSIDE )
        {
            if( aErrorLoc == ERROR_OUTSIDE )
                aClearance += aMaxError;

            outline.Inflate( aClearance, CORNER_STRATEGY::ROUND_ALL_CORNERS, aMaxError );
            outline.Fracture( SHAPE_POLY_SET::PM_FAST );
        }
        else if( aClearance < 0 )
        {
            // Negative clearances are primarily for drawing solder paste layer, so we don't
            // worry ourselves overly about which side the error is on.

            // aClearance is negative so this is actually a deflate
            outline.Inflate( aClearance, CORNER_STRATEGY::ALLOW_ACUTE_CORNERS, aMaxError );
            outline.Fracture( SHAPE_POLY_SET::PM_FAST );
        }

        aBuffer.Append( outline );
        break;
    }

    default:
        wxFAIL_MSG( wxT( "PAD::TransformShapeToPolygon no implementation for " )
                    + PAD_SHAPE_T_asString( GetShape() ) );
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
                    drawPoly.BooleanIntersection( padPoly, SHAPE_POLY_SET::PM_FAST );

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

                    if( GetLayerSet().test( other->GetLayer() )
                            && aShape->Compare( other ) == 0 )
                    {
                        matching.push_back( other );
                    }
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
        layer = *GetLayerSet().UIOrder();

    // If there are intersecting items to combine, we need to first make sure the pad is a
    // custom-shape pad.
    if( !aIsDryRun && findNext( layer ) && GetShape() != PAD_SHAPE::CUSTOM )
    {
        if( GetShape() == PAD_SHAPE::CIRCLE || GetShape() == PAD_SHAPE::RECTANGLE )
        {
            // Use the existing pad as an anchor
            SetAnchorPadShape( GetShape() );
            SetShape( PAD_SHAPE::CUSTOM );
        }
        else
        {
            // Create a new circular anchor and convert existing pad to a polygon primitive
            SHAPE_POLY_SET existingOutline;
            TransformShapeToPolygon( existingOutline, layer, 0, maxError, ERROR_INSIDE );

            int minExtent = std::min( GetSize().x, GetSize().y );
            SetAnchorPadShape( PAD_SHAPE::CIRCLE );
            SetSize( VECTOR2I( minExtent, minExtent ) );
            SetShape( PAD_SHAPE::CUSTOM );

            PCB_SHAPE* shape = new PCB_SHAPE( nullptr, SHAPE_T::POLY );
            shape->SetFilled( true );
            shape->SetStroke( STROKE_PARAMS( 0, LINE_STYLE::SOLID ) );
            shape->SetPolyShape( existingOutline );
            shape->Move( - ShapePos() );
            shape->Rotate( VECTOR2I( 0, 0 ), - GetOrientation() );
            AddPrimitive( shape );
        }
    }

    while( PCB_SHAPE* fpShape = findNext( layer ) )
    {
        fpShape->SetFlags( SKIP_STRUCT );

        mergedShapes.push_back( fpShape );

        if( !aIsDryRun )
        {
            PCB_SHAPE* primitive = static_cast<PCB_SHAPE*>( fpShape->Duplicate() );

            primitive->SetParent( nullptr );
            primitive->Move( - ShapePos() );
            primitive->Rotate( VECTOR2I( 0, 0 ), - GetOrientation() );

            AddPrimitive( primitive );
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


void PAD::CheckPad( UNITS_PROVIDER* aUnitsProvider,
                    const std::function<void( int aErrorCode,
                                              const wxString& aMsg )>& aErrorHandler ) const
{
    VECTOR2I pad_size = GetSize();
    VECTOR2I drill_size = GetDrillSize();
    wxString msg;

    if( GetShape() == PAD_SHAPE::CUSTOM )
        pad_size = GetBoundingBox().GetSize();
    else if( pad_size.x <= 0 || ( pad_size.y <= 0 && GetShape() != PAD_SHAPE::CIRCLE ) )
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

        int            maxError = GetBoard()->GetDesignSettings().m_MaxError;
        SHAPE_POLY_SET padOutline;

        TransformShapeToPolygon( padOutline, UNDEFINED_LAYER, 0, maxError, ERROR_INSIDE );

        if( GetAttribute() == PAD_ATTRIB::PTH )
        {
            // Test if there is copper area outside hole
            std::shared_ptr<SHAPE_SEGMENT> hole = GetEffectiveHoleShape();
            SHAPE_POLY_SET                 holeOutline;

            TransformOvalToPolygon( holeOutline, hole->GetSeg().A, hole->GetSeg().B,
                                    hole->GetWidth(), ARC_HIGH_DEF, ERROR_OUTSIDE );

            SHAPE_POLY_SET copper = padOutline;
            copper.BooleanSubtract( holeOutline, SHAPE_POLY_SET::POLYGON_MODE::PM_FAST );

            if( copper.IsEmpty() )
            {
                aErrorHandler( DRCE_PADSTACK, _( "(PTH pad hole leaves no copper)" ) );
            }
            else
            {
                // Test if the pad hole is fully inside the copper area
                holeOutline.BooleanSubtract( padOutline, SHAPE_POLY_SET::POLYGON_MODE::PM_FAST );

                if( !holeOutline.IsEmpty() )
                    aErrorHandler( DRCE_PADSTACK, _( "(PTH pad hole non fully inside copper)" ) );
            }
        }
        else
        {
            // Test only if the pad hole's centre is inside the copper area
            if( !padOutline.Collide( GetPosition() ) )
                aErrorHandler( DRCE_PADSTACK, _( "(pad hole not inside pad shape)" ) );
        }
    }

    if( GetLocalClearance() < 0 )
        aErrorHandler( DRCE_PADSTACK, _( "(negative local clearance values have no effect)" ) );

    // Some pads need a negative solder mask clearance (mainly for BGA with small pads)
    // However the negative solder mask clearance must not create negative mask size
    // Therefore test for minimal acceptable negative value
    std::optional<int> solderMaskMargin = GetLocalSolderMaskMargin();

    if( solderMaskMargin.has_value() && solderMaskMargin.value() < 0 )
    {
        int absMargin = abs( solderMaskMargin.value() );

        if( GetShape() == PAD_SHAPE::CUSTOM )
        {
            for( const std::shared_ptr<PCB_SHAPE>& shape : GetPrimitives() )
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
    int    paste_margin = GetLocalSolderPasteMargin();
    double paste_ratio = GetLocalSolderPasteMarginRatio();

    paste_size.x = pad_size.x + paste_margin + KiROUND( pad_size.x * paste_ratio );
    paste_size.y = pad_size.y + paste_margin + KiROUND( pad_size.y * paste_ratio );

    if( paste_size.x <= 0 || paste_size.y <= 0 )
    {
        aErrorHandler( DRCE_PADSTACK, _( "(negative solder paste margin is larger than pad; "
                                         "no solder paste mask will be generated)" ) );
    }

    LSET padlayers_mask = GetLayerSet();

    if( padlayers_mask == 0 )
        aErrorHandler( DRCE_PADSTACK_INVALID, _( "(Pad has no layer)" ) );

    if( GetAttribute() == PAD_ATTRIB::PTH && !IsOnCopperLayer() )
        aErrorHandler( DRCE_PADSTACK, _( "(PTH pad has no copper layers)" ) );

    if( !padlayers_mask[F_Cu] && !padlayers_mask[B_Cu] )
    {
        if( ( drill_size.x || drill_size.y ) && GetAttribute() != PAD_ATTRIB::NPTH )
        {
            aErrorHandler( DRCE_PADSTACK, _( "(plated through holes normally have a copper pad on "
                                             "at least one layer)" ) );
        }
    }

    switch( GetAttribute() )
    {
    case PAD_ATTRIB::NPTH:   // Not plated, but through hole, a hole is expected
    case PAD_ATTRIB::PTH:    // Pad through hole, a hole is also expected
        if( drill_size.x <= 0
            || ( drill_size.y <= 0 && GetDrillShape() == PAD_DRILL_SHAPE_T::PAD_DRILL_SHAPE_OBLONG ) )
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

    if( ( GetProperty() == PAD_PROP::FIDUCIAL_GLBL || GetProperty() == PAD_PROP::FIDUCIAL_LOCAL )
            && GetAttribute() == PAD_ATTRIB::NPTH )
    {
        aErrorHandler( DRCE_PADSTACK, _( "('fiducial' property makes no sense on NPTH pads)" ) );
    }

    if( GetProperty() == PAD_PROP::TESTPOINT && GetAttribute() == PAD_ATTRIB::NPTH )
        aErrorHandler( DRCE_PADSTACK, _( "('testpoint' property makes no sense on NPTH pads)" ) );

    if( GetProperty() == PAD_PROP::HEATSINK && GetAttribute() == PAD_ATTRIB::NPTH )
        aErrorHandler( DRCE_PADSTACK, _( "('heatsink' property makes no sense of NPTH pads)" ) );

    if( GetProperty() == PAD_PROP::CASTELLATED && GetAttribute() != PAD_ATTRIB::PTH )
        aErrorHandler( DRCE_PADSTACK, _( "('castellated' property is for PTH pads)" ) );

    if( GetProperty() == PAD_PROP::BGA && GetAttribute() != PAD_ATTRIB::SMD )
        aErrorHandler( DRCE_PADSTACK, _( "('BGA' property is for SMD pads)" ) );

    if( GetShape() == PAD_SHAPE::ROUNDRECT )
    {
        if( GetRoundRectRadiusRatio() < 0.0 )
            aErrorHandler( DRCE_PADSTACK_INVALID, _( "(negative corner radius is not allowed)" ) );
        else if( GetRoundRectRadiusRatio() > 50.0 )
            aErrorHandler( DRCE_PADSTACK, _( "(corner size will make pad circular)" ) );
    }
    else if( GetShape() == PAD_SHAPE::CHAMFERED_RECT )
    {
        if( GetChamferRectRatio() < 0.0 )
            aErrorHandler( DRCE_PADSTACK_INVALID, _( "(negative corner chamfer is not allowed)" ) );
        else if( GetChamferRectRatio() > 50.0 )
            aErrorHandler( DRCE_PADSTACK_INVALID, _( "(corner chamfer is too large)" ) );
    }
    else if( GetShape() == PAD_SHAPE::TRAPEZOID )
    {
        if(    ( GetDelta().x < 0 && GetDelta().x < -GetSize().y )
            || ( GetDelta().x > 0 && GetDelta().x > GetSize().y )
            || ( GetDelta().y < 0 && GetDelta().y < -GetSize().x )
            || ( GetDelta().y > 0 && GetDelta().y > GetSize().x ) )
        {
            aErrorHandler( DRCE_PADSTACK_INVALID, _( "(trapazoid delta is too large)" ) );
        }
    }

    // PADSTACKS TODO: this will need to check each layer in the pad...
    if( GetShape() == PAD_SHAPE::CUSTOM )
    {
        SHAPE_POLY_SET mergedPolygon;
        MergePrimitivesAsPolygon( &mergedPolygon );

        if( mergedPolygon.OutlineCount() > 1 )
            aErrorHandler( DRCE_PADSTACK_INVALID, _( "(custom pad shape must resolve to a single polygon)" ) );
    }
}


bool PAD::operator==( const BOARD_ITEM& aOther ) const
{
    if( Type() != aOther.Type() )
        return false;

    if( m_parent && aOther.GetParent() && m_parent->m_Uuid != aOther.GetParent()->m_Uuid )
        return false;

    const PAD& other = static_cast<const PAD&>( aOther );

    if( GetShape() != other.GetShape() )
        return false;

    if( GetPosition() != other.GetPosition() )
        return false;

    if( GetAttribute() != other.GetAttribute() )
        return false;

    if( GetSize() != other.GetSize() )
        return false;

    if( GetOffset() != other.GetOffset() )
        return false;

    if( GetDrillSize() != other.GetDrillSize() )
        return false;

    if( GetDrillShape() != other.GetDrillShape() )
        return false;

    if( GetRoundRectRadiusRatio() != other.GetRoundRectRadiusRatio() )
        return false;

    if( GetChamferRectRatio() != other.GetChamferRectRatio() )
        return false;

    if( GetChamferPositions() != other.GetChamferPositions() )
        return false;

    if( GetOrientation() != other.GetOrientation() )
        return false;

    if( GetZoneConnection() != other.GetZoneConnection() )
        return false;

    if( GetThermalSpokeWidth() != other.GetThermalSpokeWidth() )
        return false;

    if( GetThermalSpokeAngle() != other.GetThermalSpokeAngle() )
        return false;

    if( GetThermalGap() != other.GetThermalGap() )
        return false;

    if( GetCustomShapeInZoneOpt() != other.GetCustomShapeInZoneOpt() )
        return false;

    if( GetPrimitives().size() != other.GetPrimitives().size() )
        return false;

    for( size_t ii = 0; ii < GetPrimitives().size(); ii++ )
    {
        if( GetPrimitives()[ii] != other.GetPrimitives()[ii] )
            return false;
    }

    if( GetAnchorPadShape() != other.GetAnchorPadShape() )
        return false;

    if( GetLocalClearance() != other.GetLocalClearance() )
        return false;

    if( GetLocalSolderMaskMargin() != other.GetLocalSolderMaskMargin() )
        return false;

    if( GetLocalSolderPasteMargin() != other.GetLocalSolderPasteMargin() )
        return false;

    if( GetLocalSolderPasteMarginRatio() != other.GetLocalSolderPasteMarginRatio() )
        return false;

    if( GetLocalSpokeWidthOverride() != other.GetLocalSpokeWidthOverride() )
        return false;

    if( GetLayerSet() != other.GetLayerSet() )
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

    if( GetShape() != other.GetShape() )
        similarity *= 0.9;

    if( GetPosition() != other.GetPosition() )
        similarity *= 0.9;

    if( GetAttribute() != other.GetAttribute() )
        similarity *= 0.9;

    if( GetSize() != other.GetSize() )
        similarity *= 0.9;

    if( GetOffset() != other.GetOffset() )
        similarity *= 0.9;

    if( GetDrillSize() != other.GetDrillSize() )
        similarity *= 0.9;

    if( GetDrillShape() != other.GetDrillShape() )
        similarity *= 0.9;

    if( GetRoundRectRadiusRatio() != other.GetRoundRectRadiusRatio() )
        similarity *= 0.9;

    if( GetChamferRectRatio() != other.GetChamferRectRatio() )
        similarity *= 0.9;

    if( GetChamferPositions() != other.GetChamferPositions() )
        similarity *= 0.9;

    if( GetOrientation() != other.GetOrientation() )
        similarity *= 0.9;

    if( GetZoneConnection() != other.GetZoneConnection() )
        similarity *= 0.9;

    if( GetThermalSpokeWidth() != other.GetThermalSpokeWidth() )
        similarity *= 0.9;

    if( GetThermalSpokeAngle() != other.GetThermalSpokeAngle() )
        similarity *= 0.9;

    if( GetThermalGap() != other.GetThermalGap() )
        similarity *= 0.9;

    if( GetCustomShapeInZoneOpt() != other.GetCustomShapeInZoneOpt() )
        similarity *= 0.9;

    if( GetPrimitives().size() != other.GetPrimitives().size() )
        similarity *= 0.9;

    if( GetAnchorPadShape() != other.GetAnchorPadShape() )
        similarity *= 0.9;

    if( GetLocalClearance() != other.GetLocalClearance() )
        similarity *= 0.9;

    if( GetLocalSolderMaskMargin() != other.GetLocalSolderMaskMargin() )
        similarity *= 0.9;

    if( GetLocalSolderPasteMargin() != other.GetLocalSolderPasteMargin() )
        similarity *= 0.9;

    if( GetLocalSolderPasteMarginRatio() != other.GetLocalSolderPasteMarginRatio() )
        similarity *= 0.9;

    if( GetLocalSpokeWidthOverride() != other.GetLocalSpokeWidthOverride() )
        similarity *= 0.9;

    if( GetLayerSet() != other.GetLayerSet() )
        similarity *= 0.9;

    return similarity;
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
                .Map( PAD_PROP::CASTELLATED,       _HKI( "Castellated pad" ) );

        ENUM_MAP<PAD_DRILL_SHAPE_T>::Instance()
                .Map( PAD_DRILL_SHAPE_CIRCLE,     _HKI( "Round" ) )
                .Map( PAD_DRILL_SHAPE_OBLONG,     _HKI( "Oblong" ) );

        ENUM_MAP<ZONE_CONNECTION>& zcMap = ENUM_MAP<ZONE_CONNECTION>::Instance();

        if( zcMap.Choices().GetCount() == 0 )
        {
            zcMap.Undefined( ZONE_CONNECTION::INHERITED );
            zcMap.Map( ZONE_CONNECTION::INHERITED, _HKI( "Inherited" ) )
                 .Map( ZONE_CONNECTION::NONE, _HKI( "None" ) )
                 .Map( ZONE_CONNECTION::THERMAL, _HKI( "Thermal reliefs" ) )
                 .Map( ZONE_CONNECTION::FULL, _HKI( "Solid" ) )
                 .Map( ZONE_CONNECTION::THT_THERMAL, _HKI( "Thermal reliefs for PTH" ) );
        }

        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( PAD );
        propMgr.InheritsAfter( TYPE_HASH( PAD ), TYPE_HASH( BOARD_CONNECTED_ITEM ) );

        propMgr.Mask( TYPE_HASH( PAD ), TYPE_HASH( BOARD_CONNECTED_ITEM ), _HKI( "Layer" ) );

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
                    {
                        return pad->GetAttribute() == PAD_ATTRIB::PTH
                               || pad->GetAttribute() == PAD_ATTRIB::NPTH;
                    }

                    return false;
                };

        propMgr.OverrideAvailability( TYPE_HASH( PAD ), TYPE_HASH( BOARD_CONNECTED_ITEM ),
                                      _HKI( "Net" ), isCopperPad );
        propMgr.OverrideAvailability( TYPE_HASH( PAD ), TYPE_HASH( BOARD_CONNECTED_ITEM ),
                                      _HKI( "Net Class" ), isCopperPad );

        const wxString groupPad = _HKI( "Pad Properties" );

        auto padType = new PROPERTY_ENUM<PAD, PAD_ATTRIB>( _HKI( "Pad Type" ),
                    &PAD::SetAttribute, &PAD::GetAttribute );
        propMgr.AddProperty( padType, groupPad );

        auto shape = new PROPERTY_ENUM<PAD, PAD_SHAPE>( _HKI( "Pad Shape" ),
                    &PAD::SetShape, &PAD::GetShape );
        propMgr.AddProperty( shape, groupPad );

        auto padNumber = new PROPERTY<PAD, wxString>( _HKI( "Pad Number" ),
                                                      &PAD::SetNumber, &PAD::GetNumber );
        padNumber->SetAvailableFunc( isCopperPad );
        propMgr.AddProperty( padNumber, groupPad );

        propMgr.AddProperty( new PROPERTY<PAD, wxString>( _HKI( "Pin Name" ),
                             NO_SETTER( PAD, wxString ), &PAD::GetPinFunction ), groupPad )
                .SetIsHiddenFromLibraryEditors();
        propMgr.AddProperty( new PROPERTY<PAD, wxString>( _HKI( "Pin Type" ),
                             NO_SETTER( PAD, wxString ), &PAD::GetPinType ), groupPad )
                .SetIsHiddenFromLibraryEditors();

        propMgr.AddProperty( new PROPERTY<PAD, int>( _HKI( "Size X" ),
                    &PAD::SetSizeX, &PAD::GetSizeX,
                    PROPERTY_DISPLAY::PT_SIZE ), groupPad );
        propMgr.AddProperty( new PROPERTY<PAD, int>( _HKI( "Size Y" ),
                    &PAD::SetSizeY, &PAD::GetSizeY,
                    PROPERTY_DISPLAY::PT_SIZE ), groupPad )
                .SetAvailableFunc(
                        [=]( INSPECTABLE* aItem ) -> bool
                        {
                            // Circle pads have no usable y-size
                            if( PAD* pad = dynamic_cast<PAD*>( aItem ) )
                                return pad->GetShape() != PAD_SHAPE::CIRCLE;

                            return true;
                        } );

        auto roundRadiusRatio = new PROPERTY<PAD, double>( _HKI( "Corner Radius Ratio" ),
                    &PAD::SetRoundRectRadiusRatio, &PAD::GetRoundRectRadiusRatio );
        roundRadiusRatio->SetAvailableFunc(
                    [=]( INSPECTABLE* aItem ) -> bool
                    {
                        if( PAD* pad = dynamic_cast<PAD*>( aItem ) )
                            return pad->GetShape() == PAD_SHAPE::ROUNDRECT;

                        return false;
                    } );
        propMgr.AddProperty( roundRadiusRatio, groupPad );

        propMgr.AddProperty( new PROPERTY_ENUM<PAD, PAD_DRILL_SHAPE_T>( _HKI( "Hole Shape" ),
                    &PAD::SetDrillShape, &PAD::GetDrillShape ), groupPad )
                .SetWriteableFunc( padCanHaveHole );

        propMgr.AddProperty( new PROPERTY<PAD, int>( _HKI( "Hole Size X" ),
                    &PAD::SetDrillSizeX, &PAD::GetDrillSizeX,
                    PROPERTY_DISPLAY::PT_SIZE ), groupPad )
                .SetWriteableFunc( padCanHaveHole )
                .SetValidator( PROPERTY_VALIDATORS::PositiveIntValidator );

        propMgr.AddProperty( new PROPERTY<PAD, int>( _HKI( "Hole Size Y" ),
                    &PAD::SetDrillSizeY, &PAD::GetDrillSizeY,
                    PROPERTY_DISPLAY::PT_SIZE ), groupPad )
                .SetWriteableFunc( padCanHaveHole )
                .SetValidator( PROPERTY_VALIDATORS::PositiveIntValidator )
                .SetAvailableFunc(
                        [=]( INSPECTABLE* aItem ) -> bool
                        {
                            // Circle holes have no usable y-size
                            if( PAD* pad = dynamic_cast<PAD*>( aItem ) )
                                return pad->GetDrillShape() != PAD_DRILL_SHAPE_CIRCLE;

                            return true;
                        } );

        propMgr.AddProperty( new PROPERTY_ENUM<PAD, PAD_PROP>( _HKI( "Fabrication Property" ),
                    &PAD::SetProperty, &PAD::GetProperty ), groupPad );

        auto padToDie = new PROPERTY<PAD, int>( _HKI( "Pad To Die Length" ),
                                                &PAD::SetPadToDieLength, &PAD::GetPadToDieLength,
                                                PROPERTY_DISPLAY::PT_SIZE );
        padToDie->SetAvailableFunc( isCopperPad );
        propMgr.AddProperty( padToDie, groupPad );

        const wxString groupOverrides = _HKI( "Overrides" );

        propMgr.AddProperty( new PROPERTY<PAD, int>( _HKI( "Clearance Override" ),
                    &PAD::SetLocalClearance, &PAD::GetLocalClearance,
                    PROPERTY_DISPLAY::PT_SIZE ), groupOverrides );

        propMgr.AddProperty( new PROPERTY<PAD, int>( _HKI( "Soldermask Margin Override" ),
                    &PAD::SetLocalSolderMaskMargin, &PAD::GetLocalSolderMaskMargin,
                    PROPERTY_DISPLAY::PT_SIZE ), groupOverrides );

        propMgr.AddProperty( new PROPERTY<PAD, int>( _HKI( "Solderpaste Margin Override" ),
                    &PAD::SetLocalSolderPasteMargin, &PAD::GetLocalSolderPasteMargin,
                    PROPERTY_DISPLAY::PT_SIZE ), groupOverrides );

        propMgr.AddProperty( new PROPERTY<PAD, double>( _HKI( "Solderpaste Margin Ratio Override" ),
                    &PAD::SetLocalSolderPasteMarginRatio, &PAD::GetLocalSolderPasteMarginRatio ),
                    groupOverrides );

        propMgr.AddProperty( new PROPERTY_ENUM<PAD, ZONE_CONNECTION>(
                    _HKI( "Zone Connection Style" ),
                    &PAD::SetZoneConnection, &PAD::GetZoneConnection ), groupOverrides );

        constexpr int minZoneWidth = pcbIUScale.mmToIU( ZONE_THICKNESS_MIN_VALUE_MM );

        propMgr.AddProperty( new PROPERTY<PAD, int>( _HKI( "Thermal Relief Spoke Width" ),
                    &PAD::SetThermalSpokeWidth, &PAD::GetThermalSpokeWidth,
                    PROPERTY_DISPLAY::PT_SIZE ), groupOverrides )
                .SetValidator( PROPERTY_VALIDATORS::RangeIntValidator<minZoneWidth, INT_MAX> );

        propMgr.AddProperty( new PROPERTY<PAD, double>( _HKI( "Thermal Relief Spoke Angle" ),
                    &PAD::SetThermalSpokeAngleDegrees, &PAD::GetThermalSpokeAngleDegrees,
                    PROPERTY_DISPLAY::PT_DEGREE ), groupOverrides );

        propMgr.AddProperty( new PROPERTY<PAD, int>( _HKI( "Thermal Relief Gap" ),
                    &PAD::SetThermalGap, &PAD::GetThermalGap,
                    PROPERTY_DISPLAY::PT_SIZE ), groupOverrides )
                .SetValidator( PROPERTY_VALIDATORS::PositiveIntValidator );

        // TODO delta, drill shape offset, layer set
    }
} _PAD_DESC;

ENUM_TO_WXANY( PAD_ATTRIB );
ENUM_TO_WXANY( PAD_SHAPE );
ENUM_TO_WXANY( PAD_PROP );
ENUM_TO_WXANY( PAD_DRILL_SHAPE_T );
