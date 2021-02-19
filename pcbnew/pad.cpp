/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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
#include <kicad_string.h>
#include <i18n_utility.h>
#include <view/view.h>
#include <board.h>
#include <footprint.h>
#include <pcb_shape.h>
#include <connectivity/connectivity_data.h>
#include <convert_to_biu.h>
#include <convert_basic_shapes_to_polygon.h>
#include <widgets/msgpanel.h>
#include <pcb_painter.h>

#include <memory>

using KIGFX::PCB_PAINTER;
using KIGFX::PCB_RENDER_SETTINGS;

PAD::PAD( FOOTPRINT* parent ) :
    BOARD_CONNECTED_ITEM( parent, PCB_PAD_T )
{
    m_size.x = m_size.y   = Mils2iu( 60 );  // Default pad size 60 mils.
    m_drill.x = m_drill.y = Mils2iu( 30 );  // Default drill size 30 mils.
    m_orient              = 0;              // Pad rotation in 1/10 degrees.
    m_lengthPadToDie      = 0;

    if( m_parent && m_parent->Type() == PCB_FOOTPRINT_T )
    {
        m_pos = GetParent()->GetPosition();
    }

    SetShape( PAD_SHAPE_CIRCLE );                   // Default pad shape is PAD_CIRCLE.
    SetAnchorPadShape( PAD_SHAPE_CIRCLE );          // Default shape for custom shaped pads
                                                    // is PAD_CIRCLE.
    SetDrillShape( PAD_DRILL_SHAPE_CIRCLE );        // Default pad drill shape is a circle.
    m_attribute           = PAD_ATTRIB_PTH;         // Default pad type is plated through hole
    SetProperty( PAD_PROP_NONE );                   // no special fabrication property
    m_localClearance      = 0;
    m_localSolderMaskMargin  = 0;
    m_localSolderPasteMargin = 0;
    m_localSolderPasteMarginRatio = 0.0;
    // Parameters for round rect only:
    m_roundedCornerScale = 0.25;                    // from  IPC-7351C standard
    // Parameters for chamfered rect only:
    m_chamferScale = 0.2;                           // Size of chamfer: ratio of smallest of X,Y size
    m_chamferPositions  = RECT_NO_CHAMFER;          // No chamfered corner

    m_zoneConnection    = ZONE_CONNECTION::INHERITED; // Use parent setting by default
    m_thermalWidth      = 0;                        // Use parent setting by default
    m_thermalGap        = 0;                        // Use parent setting by default

    m_customShapeClearanceArea = CUST_PAD_SHAPE_IN_ZONE_OUTLINE;

    // Set layers mask to default for a standard thru hole pad.
    m_layerMask           = PTHMask();

    SetSubRatsnest( 0 );                       // used in ratsnest calculations

    SetDirty();
    m_effectiveBoundingRadius = 0;
    m_removeUnconnectedLayer = false;
    m_keepTopBottomLayer = true;
}


PAD::PAD( const PAD& aOther ) :
    BOARD_CONNECTED_ITEM( aOther.GetParent(), PCB_PAD_T )
{
    BOARD_CONNECTED_ITEM::operator=( aOther );

    ImportSettingsFrom( aOther );
    SetPadToDieLength( aOther.GetPadToDieLength() );
    SetPosition( aOther.GetPosition() );
    SetPos0( aOther.GetPos0() );
    SetName( aOther.GetName() );
    SetPinFunction( aOther.GetPinFunction() );
    SetSubRatsnest( aOther.GetSubRatsnest() );
    m_effectiveBoundingRadius = aOther.m_effectiveBoundingRadius;
    m_removeUnconnectedLayer = aOther.m_removeUnconnectedLayer;
    m_keepTopBottomLayer = aOther.m_keepTopBottomLayer;
    const_cast<KIID&>( m_Uuid ) = aOther.m_Uuid;
}


PAD& PAD::operator=( const PAD &aOther )
{
    BOARD_CONNECTED_ITEM::operator=( aOther );

    ImportSettingsFrom( aOther );
    SetPadToDieLength( aOther.GetPadToDieLength() );
    SetPosition( aOther.GetPosition() );
    SetPos0( aOther.GetPos0() );
    SetName( aOther.GetName() );
    SetPinFunction( aOther.GetPinFunction() );
    SetSubRatsnest( aOther.GetSubRatsnest() );
    m_effectiveBoundingRadius = aOther.m_effectiveBoundingRadius;
    m_removeUnconnectedLayer = aOther.m_removeUnconnectedLayer;
    m_keepTopBottomLayer = aOther.m_keepTopBottomLayer;

    return *this;
}


bool PAD::IsLocked() const
{
    if( GetParent() && static_cast<FOOTPRINT*>( GetParent() )->IsLocked() )
        return true;

    return BOARD_ITEM::IsLocked();
};


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
    if( GetParent() &&  GetParent()->GetLayer() == B_Cu )
        return true;
    return false;
}


bool PAD::FlashLayer( LSET aLayers ) const
{
    for( auto layer : aLayers.Seq() )
    {
        if( FlashLayer( layer ) )
            return true;
    }

    return false;
}


bool PAD::FlashLayer( int aLayer ) const
{
    std::vector<KICAD_T> types
    { PCB_TRACE_T, PCB_ARC_T, PCB_VIA_T, PCB_PAD_T, PCB_ZONE_T, PCB_FP_ZONE_T };

    // Return the "normal" shape if the caller doesn't specify a particular layer
    if( aLayer == UNDEFINED_LAYER )
        return true;

    BOARD* board = GetBoard();

    if( !board )
        return false;

    /// We don't remove the copper from non-PTH pads
    if( GetAttribute() != PAD_ATTRIB_PTH )
        return IsOnLayer( static_cast<PCB_LAYER_ID>( aLayer ) );

    /// Heatsink pads always get copper
    if( GetProperty() == PAD_PROP_HEATSINK )
        return IsOnLayer( static_cast<PCB_LAYER_ID>( aLayer ) );

    if( !m_removeUnconnectedLayer )
        return IsOnLayer( static_cast<PCB_LAYER_ID>( aLayer ) );

    /// Plated through hole pads need copper on the top/bottom layers for proper soldering
    /// Unless the user has removed them in the pad dialog
    if( m_keepTopBottomLayer && ( aLayer == F_Cu || aLayer == B_Cu ) )
        return IsOnLayer( static_cast<PCB_LAYER_ID>( aLayer ) );

    return board->GetConnectivity()->IsConnectedOnLayer( this, static_cast<int>( aLayer ),
            types );
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
    m_roundedCornerScale = std::max( 0.0, std::min( aRadiusScale, 0.5 ) );

    SetDirty();
}


void PAD::SetChamferRectRatio( double aChamferScale )
{
    m_chamferScale = std::max( 0.0, std::min( aChamferScale, 0.5 ) );

    SetDirty();
}


const std::shared_ptr<SHAPE_POLY_SET>& PAD::GetEffectivePolygon() const
{
    if( m_polyDirty )
        BuildEffectivePolygon();

    return m_effectivePolygon;
}


std::shared_ptr<SHAPE> PAD::GetEffectiveShape( PCB_LAYER_ID aLayer ) const
{
    if( m_shapesDirty )
        BuildEffectiveShapes( aLayer );

    return m_effectiveShape;
}


const SHAPE_SEGMENT* PAD::GetEffectiveHoleShape() const
{
    if( m_shapesDirty )
        BuildEffectiveShapes( UNDEFINED_LAYER );

    return m_effectiveHoleShape.get();
}


int PAD::GetBoundingRadius() const
{
    if( m_polyDirty )
        BuildEffectivePolygon();

    return m_effectiveBoundingRadius;
}


void PAD::BuildEffectiveShapes( PCB_LAYER_ID aLayer ) const
{
    std::lock_guard<std::mutex> RAII_lock( m_shapesBuildingLock );

    // If we had to wait for the lock then we were probably waiting for someone else to
    // finish rebuilding the shapes.  So check to see if they're clean now.
    if( !m_shapesDirty )
        return;

    BOARD* board = GetBoard();
    int    maxError = board ? board->GetDesignSettings().m_MaxError : ARC_HIGH_DEF;

    m_effectiveShape = std::make_shared<SHAPE_COMPOUND>();
    m_effectiveHoleShape = nullptr;

    auto add = [this]( SHAPE* aShape )
               {
                   m_effectiveShape->AddShape( aShape );
               };

    wxPoint shapePos = ShapePos();  // Fetch only once; rotation involves trig
    PAD_SHAPE_T effectiveShape = GetShape();

    if( GetShape() == PAD_SHAPE_CUSTOM )
        effectiveShape = GetAnchorPadShape();

    switch( effectiveShape )
    {
    case PAD_SHAPE_CIRCLE:
        add( new SHAPE_CIRCLE( shapePos, m_size.x / 2 ) );
        break;

    case PAD_SHAPE_OVAL:
        if( m_size.x == m_size.y ) // the oval pad is in fact a circle
            add( new SHAPE_CIRCLE( shapePos, m_size.x / 2 ) );
        else
        {
            wxSize  half_size = m_size / 2;
            int     half_width = std::min( half_size.x, half_size.y );
            wxPoint half_len( half_size.x - half_width, half_size.y - half_width );
            RotatePoint( &half_len, m_orient );
            add( new SHAPE_SEGMENT( shapePos - half_len, shapePos + half_len, half_width * 2 ) );
        }
        break;

    case PAD_SHAPE_RECT:
    case PAD_SHAPE_TRAPEZOID:
    case PAD_SHAPE_ROUNDRECT:
    {
        int     r = ( effectiveShape == PAD_SHAPE_ROUNDRECT ) ? GetRoundRectCornerRadius() : 0;
        wxPoint half_size( m_size.x / 2, m_size.y / 2 );
        wxSize  trap_delta( 0, 0 );

        if( r )
        {
            half_size -= wxPoint( r, r );

            // Avoid degenerated shapes (0 length segments) that always create issues
            // For roundrect pad very near a circle, use only a circle
            const int min_len = Millimeter2iu( 0.0001);

            if( half_size.x < min_len && half_size.y < min_len )
            {
                add( new SHAPE_CIRCLE( shapePos, r ) );
                break;
            }
        }
        else if( effectiveShape == PAD_SHAPE_TRAPEZOID )
        {
            trap_delta = m_deltaSize / 2;
        }

        SHAPE_LINE_CHAIN corners;

        corners.Append( -half_size.x - trap_delta.y,  half_size.y + trap_delta.x );
        corners.Append(  half_size.x + trap_delta.y,  half_size.y - trap_delta.x );
        corners.Append(  half_size.x - trap_delta.y, -half_size.y + trap_delta.x );
        corners.Append( -half_size.x + trap_delta.y, -half_size.y - trap_delta.x );

        corners.Rotate( -DECIDEG2RAD( m_orient ) );
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

    case PAD_SHAPE_CHAMFERED_RECT:
    {
        SHAPE_POLY_SET outline;

        TransformRoundChamferedRectToPolygon( outline, shapePos, GetSize(), m_orient,
                                              GetRoundRectCornerRadius(), GetChamferRectRatio(),
                                              GetChamferPositions(), maxError, ERROR_INSIDE );

        add( new SHAPE_SIMPLE( outline.COutline( 0 ) ) );
    }
        break;

    default:
        wxFAIL_MSG( "PAD::buildEffectiveShapes: Unsupported pad shape: "
                    + PAD_SHAPE_T_asString( effectiveShape ) );
        break;
    }

    if( GetShape() == PAD_SHAPE_CUSTOM )
    {
        for( const std::shared_ptr<PCB_SHAPE>& primitive : m_editPrimitives )
        {
            for( SHAPE* shape : primitive->MakeEffectiveShapes() )
            {
                shape->Rotate( -DECIDEG2RAD( m_orient ) );
                shape->Move( shapePos );
                add( shape );
            }
        }
    }

    BOX2I bbox = m_effectiveShape->BBox();
    m_effectiveBoundingBox = EDA_RECT( (wxPoint) bbox.GetPosition(),
                                       wxSize( bbox.GetWidth(), bbox.GetHeight() ) );

    // Hole shape
    //
    wxSize  half_size = m_drill / 2;
    int     half_width = std::min( half_size.x, half_size.y );
    wxPoint half_len( half_size.x - half_width, half_size.y - half_width );

    RotatePoint( &half_len, m_orient );

    m_effectiveHoleShape = std::make_shared<SHAPE_SEGMENT>( m_pos - half_len, m_pos + half_len,
                                                            half_width * 2 );

    // All done
    //
    m_shapesDirty = false;
}


void PAD::BuildEffectivePolygon() const
{
    std::lock_guard<std::mutex> RAII_lock( m_polyBuildingLock );

    // If we had to wait for the lock then we were probably waiting for someone else to
    // finish rebuilding the shapes.  So check to see if they're clean now.
    if( !m_polyDirty )
        return;

    BOARD* board = GetBoard();
    int    maxError = board ? board->GetDesignSettings().m_MaxError : ARC_HIGH_DEF;

    // Polygon
    //
    m_effectivePolygon = std::make_shared<SHAPE_POLY_SET>();
    TransformShapeWithClearanceToPolygon( *m_effectivePolygon, UNDEFINED_LAYER, 0, maxError,
                                          ERROR_INSIDE );

    // Bounding radius
    //
    // PADSTACKS TODO: these will both need to cycle through all layers to get the largest
    // values....
    //
    m_effectiveBoundingRadius = 0;

    for( int cnt = 0; cnt < m_effectivePolygon->OutlineCount(); ++cnt )
    {
        const SHAPE_LINE_CHAIN& poly = m_effectivePolygon->COutline( cnt );

        for( int ii = 0; ii < poly.PointCount(); ++ii )
        {
            int dist = KiROUND( ( poly.CPoint( ii ) - m_pos ).EuclideanNorm() );
            m_effectiveBoundingRadius = std::max( m_effectiveBoundingRadius, dist );
        }
    }

    // All done
    //
    m_polyDirty = false;
}


const EDA_RECT PAD::GetBoundingBox() const
{
    if( m_shapesDirty )
        BuildEffectiveShapes( UNDEFINED_LAYER );

    return m_effectiveBoundingBox;
}


void PAD::SetDrawCoord()
{
    FOOTPRINT* parentFootprint = static_cast<FOOTPRINT*>( m_parent );

    m_pos = m_pos0;

    if( parentFootprint == NULL )
        return;

    double angle = parentFootprint->GetOrientation();

    RotatePoint( &m_pos.x, &m_pos.y, angle );
    m_pos += parentFootprint->GetPosition();

    SetDirty();
}


void PAD::SetLocalCoord()
{
    FOOTPRINT* parentFootprint = static_cast<FOOTPRINT*>( m_parent );

    if( parentFootprint == NULL )
    {
        m_pos0 = m_pos;
        return;
    }

    m_pos0 = m_pos - parentFootprint->GetPosition();
    RotatePoint( &m_pos0.x, &m_pos0.y, -parentFootprint->GetOrientation() );
}


void PAD::SetAttribute( PAD_ATTR_T aAttribute )
{
    m_attribute = aAttribute;

    if( aAttribute == PAD_ATTRIB_SMD )
        m_drill = wxSize( 0, 0 );

    SetDirty();
}


void PAD::SetProperty( PAD_PROP_T aProperty )
{
    m_property = aProperty;

    SetDirty();
}


void PAD::SetOrientation( double aAngle )
{
    NORMALIZE_ANGLE_POS( aAngle );
    m_orient = aAngle;

    SetDirty();
}


void PAD::Flip( const wxPoint& aCentre, bool aFlipLeftRight )
{
    if( aFlipLeftRight )
    {
        MIRROR( m_pos.x, aCentre.x );
        MIRROR( m_pos0.x, 0 );
        MIRROR( m_offset.x, 0 );
        MIRROR( m_deltaSize.x, 0 );
    }
    else
    {
        MIRROR( m_pos.y, aCentre.y );
        MIRROR( m_pos0.y, 0 );
        MIRROR( m_offset.y, 0 );
        MIRROR( m_deltaSize.y, 0 );
    }

    SetOrientation( -GetOrientation() );

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


// Flip (mirror) the basic shapes (primitives), in custom pads
void PAD::FlipPrimitives( bool aFlipLeftRight )
{
    for( std::shared_ptr<PCB_SHAPE>& primitive : m_editPrimitives )
        primitive->Flip( wxPoint( 0, 0 ), aFlipLeftRight );

    SetDirty();
}


// Returns the position of the pad.
wxPoint PAD::ShapePos() const
{
    if( m_offset.x == 0 && m_offset.y == 0 )
        return m_pos;

    wxPoint loc_offset = m_offset;

    RotatePoint( &loc_offset, m_orient );

    wxPoint shape_pos = m_pos + loc_offset;

    return shape_pos;
}


int PAD::GetLocalClearanceOverrides( wxString* aSource ) const
{
    // A pad can have specific clearance that overrides its NETCLASS clearance value
    if( GetLocalClearance() )
        return GetLocalClearance( aSource );

    // A footprint can have a specific clearance value
    if( GetParent() && GetParent()->GetLocalClearance() )
        return GetParent()->GetLocalClearance( aSource );

    return 0;
}


int PAD::GetLocalClearance( wxString* aSource ) const
{
    if( aSource )
        *aSource = wxString::Format( _( "pad %s" ), GetName() );

    return m_localClearance;
}


// Mask margins handling:

int PAD::GetSolderMaskMargin() const
{
    // The pad inherits the margin only to calculate a default shape,
    // therefore only if it is also a copper layer
    // Pads defined only on mask layers (and perhaps on other tech layers) use the shape
    // defined by the pad settings only
    bool isOnCopperLayer = ( m_layerMask & LSET::AllCuMask() ).any();

    if( !isOnCopperLayer )
        return 0;

    int     margin = m_localSolderMaskMargin;

    FOOTPRINT* parentFootprint = GetParent();

    if( parentFootprint )
    {
        if( margin == 0 )
        {
            if( parentFootprint->GetLocalSolderMaskMargin() )
                margin = parentFootprint->GetLocalSolderMaskMargin();
        }

        if( margin == 0 )
        {
            BOARD* brd = GetBoard();

            if( brd )
                margin = brd->GetDesignSettings().m_SolderMaskMargin;
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


wxSize PAD::GetSolderPasteMargin() const
{
    // The pad inherits the margin only to calculate a default shape,
    // therefore only if it is also a copper layer.
    // Pads defined only on mask layers (and perhaps on other tech layers) use the shape
    // defined by the pad settings only
    bool isOnCopperLayer = ( m_layerMask & LSET::AllCuMask() ).any();

    if( !isOnCopperLayer )
        return wxSize( 0, 0 );

    int     margin = m_localSolderPasteMargin;
    double  mratio = m_localSolderPasteMarginRatio;

    FOOTPRINT* parentFootprint = GetParent();

    if( parentFootprint )
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

    wxSize pad_margin;
    pad_margin.x = margin + KiROUND( m_size.x * mratio );
    pad_margin.y = margin + KiROUND( m_size.y * mratio );

    // ensure mask have a size always >= 0
    if( pad_margin.x < -m_size.x / 2 )
        pad_margin.x = -m_size.x / 2;

    if( pad_margin.y < -m_size.y / 2 )
        pad_margin.y = -m_size.y / 2;

    return pad_margin;
}


ZONE_CONNECTION PAD::GetEffectiveZoneConnection( wxString* aSource ) const
{
    FOOTPRINT* parentFootprint = GetParent();

    if( m_zoneConnection == ZONE_CONNECTION::INHERITED && parentFootprint )
    {
        if( aSource )
            *aSource = _( "parent footprint" );

        return parentFootprint->GetZoneConnection();
    }
    else
    {
        if( aSource )
            *aSource = _( "pad" );

        return m_zoneConnection;
    }
}


int PAD::GetEffectiveThermalSpokeWidth( wxString* aSource ) const
{
    FOOTPRINT* parentFootprint = GetParent();

    if( m_thermalWidth == 0 && parentFootprint )
    {
        if( aSource )
            *aSource = _( "parent footprint" );

        return parentFootprint->GetThermalWidth();
    }

    if( aSource )
        *aSource = _( "pad" );

    return m_thermalWidth;
}


int PAD::GetEffectiveThermalGap( wxString* aSource ) const
{
    FOOTPRINT* parentFootprint = GetParent();

    if( m_thermalGap == 0 && parentFootprint )
    {
        if( aSource )
            *aSource = _( "parent footprint" );

        return parentFootprint->GetThermalGap();
    }

    if( aSource )
        *aSource = _( "pad" );

    return m_thermalGap;
}


void PAD::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    EDA_UNITS  units = aFrame->GetUserUnits();
    wxString   msg;
    FOOTPRINT* parentFootprint = static_cast<FOOTPRINT*>( m_parent );

    if( parentFootprint )
        aList.emplace_back( _( "Footprint" ), parentFootprint->GetReference() );

    aList.emplace_back( _( "Pad" ), m_name );

    if( !GetPinFunction().IsEmpty() )
        aList.emplace_back( _( "Pin Name" ), GetPinFunction() );

    if( !GetPinType().IsEmpty() )
        aList.emplace_back( _( "Pin Type" ), GetPinType() );

    aList.emplace_back( _( "Net" ), UnescapeString( GetNetname() ) );

    aList.emplace_back( _( "NetClass" ), UnescapeString( GetNetClass()->GetName() ) );

    if( GetAttribute() == PAD_ATTRIB_SMD || GetAttribute() == PAD_ATTRIB_CONN )
        aList.emplace_back( _( "Layer" ), layerMaskDescribe() );

    // Show the pad shape, attribute and property
    wxString props = ShowPadAttr();

    if( GetProperty() != PAD_PROP_NONE )
        props += ',';

    switch( GetProperty() )
    {
    case PAD_PROP_NONE:                                           break;
    case PAD_PROP_BGA:            props += _("BGA" );             break;
    case PAD_PROP_FIDUCIAL_GLBL:  props += _("Fiducial global" ); break;
    case PAD_PROP_FIDUCIAL_LOCAL: props += _("Fiducial local" );  break;
    case PAD_PROP_TESTPOINT:      props += _("Test point" );      break;
    case PAD_PROP_HEATSINK:       props += _("Heat sink" );       break;
    case PAD_PROP_CASTELLATED:    props += _("Castellated" );     break;
    }

    aList.emplace_back( ShowPadShape(), props );

    if( ( GetShape() == PAD_SHAPE_CIRCLE || GetShape() == PAD_SHAPE_OVAL ) && m_size.x == m_size.y )
    {
        aList.emplace_back( _( "Diameter" ), MessageTextFromValue( units, m_size.x ) );
    }
    else
    {
        aList.emplace_back( _( "Width" ), MessageTextFromValue( units, m_size.x ) );
        aList.emplace_back( _( "Height" ), MessageTextFromValue( units, m_size.y ) );
    }

    double fp_orient_degrees = parentFootprint ? parentFootprint->GetOrientationDegrees() : 0;
    double pad_orient_degrees = GetOrientationDegrees() - fp_orient_degrees;
    pad_orient_degrees = NormalizeAngleDegrees( pad_orient_degrees, -180.0, +180.0 );

    if( fp_orient_degrees != 0.0 )
        msg.Printf( wxT( "%g(+ %g)" ), pad_orient_degrees, fp_orient_degrees );
    else
        msg.Printf( wxT( "%g" ), GetOrientationDegrees() );

    aList.emplace_back( _( "Rotation" ), msg );

    if( GetPadToDieLength() )
    {
        msg = MessageTextFromValue(units, GetPadToDieLength() );
        aList.emplace_back( _( "Length in Package" ), msg );
    }

    if( m_drill.x > 0 || m_drill.y > 0 )
    {
        if( GetDrillShape() == PAD_DRILL_SHAPE_CIRCLE )
        {
            aList.emplace_back( _( "Hole" ),
                                wxString::Format( "%s",
                                                  MessageTextFromValue( units, m_drill.x ) ) );
        }
        else
        {
            aList.emplace_back( _( "Hole X / Y" ),
                                wxString::Format( "%s / %s",
                                                  MessageTextFromValue( units, m_drill.x ),
                                                  MessageTextFromValue( units, m_drill.y ) ) );
        }
    }

    wxString source;
    int      clearance = GetOwnClearance( GetLayer(), &source );

    aList.emplace_back( wxString::Format( _( "Min Clearance: %s" ),
                                          MessageTextFromValue( units, clearance ) ),
                        wxString::Format( _( "(from %s)" ), source ) );
}


bool PAD::HitTest( const wxPoint& aPosition, int aAccuracy ) const
{
    VECTOR2I delta = aPosition - GetPosition();
    int      boundingRadius = GetBoundingRadius() + aAccuracy;

    if( delta.SquaredEuclideanNorm() > SEG::Square( boundingRadius ) )
        return false;

    return GetEffectivePolygon()->Contains( aPosition, -1, aAccuracy );
}


bool PAD::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    auto getArea = []( const SHAPE_POLY_SET& aPoly ) -> double
                   {
                       return aPoly.OutlineCount() ? aPoly.COutline( 0 ).Area() : 0;
                   };

    EDA_RECT arect = aRect;
    arect.Normalize();
    arect.Inflate( aAccuracy );

    EDA_RECT bbox = GetBoundingBox();

    if( !arect.Intersects( bbox ) )
        return false;

    // This covers total containment for all test cases
    if( arect.Contains( bbox ) )
        return true;

    SHAPE_POLY_SET selRect;
    selRect.NewOutline();
    selRect.Append( arect.GetOrigin() );
    selRect.Append( VECTOR2I( arect.GetRight(), arect.GetTop() ) );
    selRect.Append( VECTOR2I( arect.GetRight(), arect.GetBottom() ) );
    selRect.Append( VECTOR2I( arect.GetLeft(), arect.GetBottom() ) );

    selRect.BooleanIntersection( *GetEffectivePolygon(), SHAPE_POLY_SET::PM_FAST );

    double padArea = getArea( *GetEffectivePolygon() );
    double intersection = getArea( selRect );

    if( intersection > ( padArea * 0.99 ) )
        return true;
    else
        return !aContained && intersection > 0;
}


int PAD::Compare( const PAD* padref, const PAD* padcmp )
{
    int diff;

    if( ( diff = padref->GetShape() - padcmp->GetShape() ) != 0 )
        return diff;

    if( ( diff = padref->GetDrillShape() - padcmp->GetDrillShape() ) != 0)
        return diff;

    if( ( diff = padref->m_drill.x - padcmp->m_drill.x ) != 0 )
        return diff;

    if( ( diff = padref->m_drill.y - padcmp->m_drill.y ) != 0 )
        return diff;

    if( ( diff = padref->m_size.x - padcmp->m_size.x ) != 0 )
        return diff;

    if( ( diff = padref->m_size.y - padcmp->m_size.y ) != 0 )
        return diff;

    if( ( diff = padref->m_offset.x - padcmp->m_offset.x ) != 0 )
        return diff;

    if( ( diff = padref->m_offset.y - padcmp->m_offset.y ) != 0 )
        return diff;

    if( ( diff = padref->m_deltaSize.x - padcmp->m_deltaSize.x ) != 0 )
        return diff;

    if( ( diff = padref->m_deltaSize.y - padcmp->m_deltaSize.y ) != 0 )
        return diff;

// TODO: test custom shapes

    // Dick: specctra_export needs this
    // Lorenzo: gencad also needs it to implement padstacks!

#if __cplusplus >= 201103L
    long long d = padref->m_layerMask.to_ullong() - padcmp->m_layerMask.to_ullong();
    if( d < 0 )
        return -1;
    else if( d > 0 )
        return 1;

    return 0;
#else
    // these strings are not typically constructed, since we don't get here often.
    std::string s1 = padref->m_layerMask.to_string();
    std::string s2 = padcmp->m_layerMask.to_string();
    return s1.compare( s2 );
#endif
}


void PAD::Rotate( const wxPoint& aRotCentre, double aAngle )
{
    RotatePoint( &m_pos, aRotCentre, aAngle );

    m_orient = NormalizeAngle360Min( m_orient + aAngle );

    SetLocalCoord();

    SetDirty();
}


wxString PAD::ShowPadShape() const
{
    switch( GetShape() )
    {
    case PAD_SHAPE_CIRCLE:         return _( "Circle" );
    case PAD_SHAPE_OVAL:           return _( "Oval" );
    case PAD_SHAPE_RECT:           return _( "Rect" );
    case PAD_SHAPE_TRAPEZOID:      return _( "Trap" );
    case PAD_SHAPE_ROUNDRECT:      return _( "Roundrect" );
    case PAD_SHAPE_CHAMFERED_RECT: return _( "Chamferedrect" );
    case PAD_SHAPE_CUSTOM:         return _( "CustomShape" );
    default:                       return wxT( "???" );
    }
}


wxString PAD::ShowPadAttr() const
{
    switch( GetAttribute() )
    {
    case PAD_ATTRIB_PTH:    return _( "PTH" );
    case PAD_ATTRIB_SMD:    return _( "SMD" );
    case PAD_ATTRIB_CONN:   return _( "Conn" );
    case PAD_ATTRIB_NPTH:   return _( "NPTH" );
    default:                return wxT( "???" );
    }
}


wxString PAD::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    if( GetName().IsEmpty() )
    {
        if( GetAttribute() == PAD_ATTRIB_SMD || GetAttribute() == PAD_ATTRIB_CONN )
        {
            return wxString::Format( _( "Pad of %s on %s" ),
                                     GetParent()->GetReference(),
                                     layerMaskDescribe() );
        }
        else
        {
            return wxString::Format( _( "Through hole pad of %s" ),
                                     GetParent()->GetReference() );
        }
    }
    else
    {
        if( GetAttribute() == PAD_ATTRIB_SMD || GetAttribute() == PAD_ATTRIB_CONN )
        {
            return wxString::Format( _( "Pad %s of %s on %s" ),
                                     GetName(),
                                     GetParent()->GetReference(),
                                     layerMaskDescribe() );
        }
        else
        {
            return wxString::Format( _( "Through hole pad %s of %s" ),
                                     GetName(),
                                     GetParent()->GetReference() );
        }
    }
}


BITMAP_DEF PAD::GetMenuImage() const
{
    return pad_xpm;
}


EDA_ITEM* PAD::Clone() const
{
    return new PAD( *this );
}


void PAD::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount = 0;

    // These 2 types of pads contain a hole
    if( m_attribute == PAD_ATTRIB_PTH )
    {
        aLayers[aCount++] = LAYER_PAD_PLATEDHOLES;
        aLayers[aCount++] = LAYER_PAD_HOLEWALLS;
    }

    if( m_attribute == PAD_ATTRIB_NPTH )
        aLayers[aCount++] = LAYER_NON_PLATEDHOLES;

    if( IsOnLayer( F_Cu ) && IsOnLayer( B_Cu ) )
    {
        // Multi layer pad
        aLayers[aCount++] = LAYER_PADS_TH;
        aLayers[aCount++] = LAYER_PAD_NETNAMES;
    }
    else if( IsOnLayer( F_Cu ) )
    {
        aLayers[aCount++] = LAYER_PAD_FR;

        // Is this a PTH pad that has only front copper?  If so, we need to also display the
        // net name on the PTH netname layer so that it isn't blocked by the drill hole.
        if( m_attribute == PAD_ATTRIB_PTH )
            aLayers[aCount++] = LAYER_PAD_NETNAMES;
        else
            aLayers[aCount++] = LAYER_PAD_FR_NETNAMES;
    }
    else if( IsOnLayer( B_Cu ) )
    {
        aLayers[aCount++] = LAYER_PAD_BK;

        // Is this a PTH pad that has only back copper?  If so, we need to also display the
        // net name on the PTH netname layer so that it isn't blocked by the drill hole.
        if( m_attribute == PAD_ATTRIB_PTH )
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

#ifdef __WXDEBUG__
    if( aCount == 0 )    // Should not occur
    {
        wxString msg;
        msg.Printf( wxT( "footprint %s, pad %s: could not find valid layer for pad" ),
                GetParent() ? GetParent()->GetReference() : "<null>",
                GetName().IsEmpty() ? "(unnamed)" : GetName() );
        wxLogWarning( msg );
    }
#endif
}


double PAD::ViewGetLOD( int aLayer, KIGFX::VIEW* aView ) const
{
    constexpr double HIDE = std::numeric_limits<double>::max();

    PCB_PAINTER*         painter = static_cast<PCB_PAINTER*>( aView->GetPainter() );
    PCB_RENDER_SETTINGS* renderSettings = painter->GetSettings();
    BOARD*               board = GetBoard();
    LSET                 visible = LSET::AllLayersMask();

    // Meta control for hiding all pads
    if( !aView->IsLayerVisible( LAYER_PADS ) )
        return HIDE;

    // Handle board visibility (unless printing)
    if( board && !aView->GetPrintMode() )
        visible = board->GetVisibleLayers() & board->GetEnabledLayers();

    // Handle Render tab switches
    if( ( GetAttribute() == PAD_ATTRIB_PTH || GetAttribute() == PAD_ATTRIB_NPTH )
         && !aView->IsLayerVisible( LAYER_PADS_TH ) )
    {
        return HIDE;
    }

    if( !IsFlipped() && !aView->IsLayerVisible( LAYER_MOD_FR ) )
        return HIDE;

    if( IsFlipped() && !aView->IsLayerVisible( LAYER_MOD_BK ) )
        return HIDE;

    if( IsFrontLayer( (PCB_LAYER_ID) aLayer ) && !aView->IsLayerVisible( LAYER_PAD_FR ) )
        return HIDE;

    if( IsBackLayer( (PCB_LAYER_ID) aLayer ) && !aView->IsLayerVisible( LAYER_PAD_BK ) )
        return HIDE;

    if( aLayer == LAYER_PADS_TH )
    {
        if( !FlashLayer( visible ) )
            return HIDE;
    }
    else if( IsHoleLayer( aLayer ) )
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

        // Pad sizes can be zero briefly when someone is typing a number like "0.5"
        // in the pad properties dialog
        if( divisor == 0 )
            return HIDE;

        return ( double ) Millimeter2iu( 5 ) / divisor;
    }

    if( aLayer == LAYER_PADS_TH
            && GetShape() != PAD_SHAPE_CUSTOM
            && GetSizeX() <= GetDrillSizeX()
            && GetSizeY() <= GetDrillSizeY() )
    {
        // Don't tweak the drawing code with a degenerate pad
        return HIDE;
    }

    // Passed all tests; show.
    return 0.0;
}


const BOX2I PAD::ViewBBox() const
{
    // Bounding box includes soldermask too. Remember mask and/or paste
    // margins can be < 0
    int solderMaskMargin       = std::max( GetSolderMaskMargin(), 0 );
    VECTOR2I solderPasteMargin = VECTOR2D( GetSolderPasteMargin() );
    EDA_RECT bbox              = GetBoundingBox();

    // Look for the biggest possible bounding box
    int xMargin = std::max( solderMaskMargin, solderPasteMargin.x );
    int yMargin = std::max( solderMaskMargin, solderPasteMargin.y );

    return BOX2I( VECTOR2I( bbox.GetOrigin() ) - VECTOR2I( xMargin, yMargin ),
                  VECTOR2I( bbox.GetSize() ) + VECTOR2I( 2 * xMargin, 2 * yMargin ) );
}


FOOTPRINT* PAD::GetParent() const
{
    return dynamic_cast<FOOTPRINT*>( m_parent );
}


void PAD::ImportSettingsFrom( const PAD& aMasterPad )
{
    SetShape( aMasterPad.GetShape() );
    SetLayerSet( aMasterPad.GetLayerSet() );
    SetAttribute( aMasterPad.GetAttribute() );
    SetProperty( aMasterPad.GetProperty() );

    // I am not sure the m_LengthPadToDie must be imported, because this is
    // a parameter really specific to a given pad (JPC).
    // So this is currently non imported
    #if 0
    SetPadToDieLength( aMasterPad.GetPadToDieLength() );
    #endif

    // The pad orientation, for historical reasons is the
    // pad rotation + parent rotation.
    // So we have to manage this parent rotation
    double pad_rot = aMasterPad.GetOrientation();

    if( aMasterPad.GetParent() )
        pad_rot -= aMasterPad.GetParent()->GetOrientation();

    if( GetParent() )
        pad_rot += GetParent()->GetOrientation();

    SetOrientation( pad_rot );

    SetSize( aMasterPad.GetSize() );
    SetDelta( wxSize( 0, 0 ) );
    SetOffset( aMasterPad.GetOffset() );
    SetDrillSize( aMasterPad.GetDrillSize() );
    SetDrillShape( aMasterPad.GetDrillShape() );
    SetRoundRectRadiusRatio( aMasterPad.GetRoundRectRadiusRatio() );
    SetChamferRectRatio( aMasterPad.GetChamferRectRatio() );
    SetChamferPositions( aMasterPad.GetChamferPositions() );

    switch( aMasterPad.GetShape() )
    {
    case PAD_SHAPE_TRAPEZOID:
        SetDelta( aMasterPad.GetDelta() );
        break;

    case PAD_SHAPE_CIRCLE:
        // ensure size.y == size.x
        SetSize( wxSize( GetSize().x, GetSize().x ) );
        break;

    default:
        ;
    }

    switch( aMasterPad.GetAttribute() )
    {
    case PAD_ATTRIB_SMD:
    case PAD_ATTRIB_CONN:
        // These pads do not have hole (they are expected to be only on one
        // external copper layer)
        SetDrillSize( wxSize( 0, 0 ) );
        break;

    default:
        ;
    }

    // copy also local settings:
    SetLocalClearance( aMasterPad.GetLocalClearance() );
    SetLocalSolderMaskMargin( aMasterPad.GetLocalSolderMaskMargin() );
    SetLocalSolderPasteMargin( aMasterPad.GetLocalSolderPasteMargin() );
    SetLocalSolderPasteMarginRatio( aMasterPad.GetLocalSolderPasteMarginRatio() );

    SetZoneConnection( aMasterPad.GetEffectiveZoneConnection() );
    SetThermalSpokeWidth( aMasterPad.GetThermalSpokeWidth() );
    SetThermalGap( aMasterPad.GetThermalGap() );

    SetCustomShapeInZoneOpt( aMasterPad.GetCustomShapeInZoneOpt() );

    // Add or remove custom pad shapes:
    ReplacePrimitives( aMasterPad.GetPrimitives() );
    SetAnchorPadShape( aMasterPad.GetAnchorPadShape() );

    SetDirty();
}


void PAD::SwapData( BOARD_ITEM* aImage )
{
    assert( aImage->Type() == PCB_PAD_T );

    std::swap( *((FOOTPRINT*) this), *((FOOTPRINT*) aImage) );
}


static struct PAD_DESC
{
    PAD_DESC()
    {
        ENUM_MAP<PAD_ATTR_T>::Instance()
                .Map( PAD_ATTRIB_PTH,             _HKI( "Through-hole" ) )
                .Map( PAD_ATTRIB_SMD,             _HKI( "SMD" ) )
                .Map( PAD_ATTRIB_CONN,            _HKI( "Edge connector" ) )
                .Map( PAD_ATTRIB_NPTH,            _HKI( "NPTH, mechanical" ) );

        ENUM_MAP<PAD_SHAPE_T>::Instance()
                .Map( PAD_SHAPE_CIRCLE,           _HKI( "Circle" ) )
                .Map( PAD_SHAPE_RECT,             _HKI( "Rectangle" ) )
                .Map( PAD_SHAPE_OVAL,             _HKI( "Oval" ) )
                .Map( PAD_SHAPE_TRAPEZOID,        _HKI( "Trapezoid" ) )
                .Map( PAD_SHAPE_ROUNDRECT,        _HKI( "Rounded rectangle" ) )
                .Map( PAD_SHAPE_CHAMFERED_RECT,   _HKI( "Chamfered rectangle" ) )
                .Map( PAD_SHAPE_CUSTOM,           _HKI( "Custom" ) );

        ENUM_MAP<PAD_PROP_T>::Instance()
                .Map( PAD_PROP_NONE,              _HKI( "None" ) )
                .Map( PAD_PROP_BGA,               _HKI( "BGA pad" ) )
                .Map( PAD_PROP_FIDUCIAL_GLBL,     _HKI( "Fiducial, global to board" ) )
                .Map( PAD_PROP_FIDUCIAL_LOCAL,    _HKI( "Fiducial, local to footprint" ) )
                .Map( PAD_PROP_TESTPOINT,         _HKI( "Test point pad" ) )
                .Map( PAD_PROP_HEATSINK,          _HKI( "Heatsink pad" ) )
                .Map( PAD_PROP_CASTELLATED,       _HKI( "Castellated pad" ) );

        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( PAD );
        propMgr.InheritsAfter( TYPE_HASH( PAD ), TYPE_HASH( BOARD_CONNECTED_ITEM ) );

        auto padType = new PROPERTY_ENUM<PAD, PAD_ATTR_T>( _HKI( "Pad Type" ),
                    &PAD::SetAttribute, &PAD::GetAttribute );
        propMgr.AddProperty( padType );

        auto shape = new PROPERTY_ENUM<PAD, PAD_SHAPE_T>( _HKI( "Shape" ),
                    &PAD::SetShape, &PAD::GetShape );
        propMgr.AddProperty( shape );

        propMgr.AddProperty( new PROPERTY<PAD, wxString>( _HKI( "Pad Number" ),
                    &PAD::SetName, &PAD::GetName ) );
        propMgr.AddProperty( new PROPERTY<PAD, wxString>( _HKI( "Pin Name" ),
                    &PAD::SetPinFunction, &PAD::GetPinFunction ) );
        propMgr.AddProperty( new PROPERTY<PAD, wxString>( _HKI( "Pin Type" ),
                    &PAD::SetPinType, &PAD::GetPinType ) );
        propMgr.AddProperty( new PROPERTY<PAD, double>( _HKI( "Orientation" ),
                    &PAD::SetOrientationDegrees, &PAD::GetOrientationDegrees,
                    PROPERTY_DISPLAY::DEGREE ) );
        propMgr.AddProperty( new PROPERTY<PAD, int>( _HKI( "Size X" ),
                    &PAD::SetSizeX, &PAD::GetSizeX,
                    PROPERTY_DISPLAY::DISTANCE ) );
        propMgr.AddProperty( new PROPERTY<PAD, int>( _HKI( "Size Y" ),
                    &PAD::SetSizeY, &PAD::GetSizeY,
                    PROPERTY_DISPLAY::DISTANCE ) );
        propMgr.AddProperty( new PROPERTY<PAD, int>( _HKI( "Hole Size X" ),
                    &PAD::SetDrillSizeX, &PAD::GetDrillSizeX,
                    PROPERTY_DISPLAY::DISTANCE ) );
        propMgr.AddProperty( new PROPERTY<PAD, int>( _HKI( "Hole Size Y" ),
                    &PAD::SetDrillSizeY, &PAD::GetDrillSizeY,
                    PROPERTY_DISPLAY::DISTANCE ) );
        propMgr.AddProperty( new PROPERTY<PAD, int>( _HKI( "Pad To Die Length" ),
                    &PAD::SetPadToDieLength, &PAD::GetPadToDieLength,
                    PROPERTY_DISPLAY::DISTANCE ) );
        propMgr.AddProperty( new PROPERTY<PAD, int>( _HKI( "Local Soldermask Margin" ),
                    &PAD::SetLocalSolderMaskMargin, &PAD::GetLocalSolderMaskMargin,
                    PROPERTY_DISPLAY::DISTANCE ) );
        propMgr.AddProperty( new PROPERTY<PAD, int>( _HKI( "Local Solderpaste Margin" ),
                    &PAD::SetLocalSolderPasteMargin, &PAD::GetLocalSolderPasteMargin,
                    PROPERTY_DISPLAY::DISTANCE ) );
        propMgr.AddProperty( new PROPERTY<PAD, double>( _HKI( "Local Solderpaste Margin Ratio" ),
                    &PAD::SetLocalSolderPasteMarginRatio, &PAD::GetLocalSolderPasteMarginRatio ) );
        propMgr.AddProperty( new PROPERTY<PAD, int>( _HKI( "Thermal Relief Spoke Width" ),
                    &PAD::SetThermalSpokeWidth, &PAD::GetThermalSpokeWidth,
                    PROPERTY_DISPLAY::DISTANCE ) );
        propMgr.AddProperty( new PROPERTY<PAD, int>( _HKI( "Thermal Relief" ),
                    &PAD::SetThermalGap, &PAD::GetThermalGap,
                    PROPERTY_DISPLAY::DISTANCE ) );
        propMgr.AddProperty( new PROPERTY_ENUM<PAD, PAD_PROP_T>( _HKI( "Fabrication Property" ),
                    &PAD::SetProperty, &PAD::GetProperty ) );

        auto roundRadiusRatio = new PROPERTY<PAD, double>( _HKI( "Round Radius Ratio" ),
                    &PAD::SetRoundRectRadiusRatio, &PAD::GetRoundRectRadiusRatio );
        roundRadiusRatio->SetAvailableFunc(
                    [=]( INSPECTABLE* aItem ) -> bool
                    {
                        return aItem->Get( shape ) == PAD_SHAPE_ROUNDRECT;
                    } );
        propMgr.AddProperty( roundRadiusRatio );

        propMgr.AddProperty( new PROPERTY<PAD, int>( _HKI( "Local Clearance" ),
                    &PAD::SetLocalClearance, &PAD::GetLocalClearance,
                    PROPERTY_DISPLAY::DISTANCE ) );
        propMgr.AddProperty( new PROPERTY<PAD, wxString>( _HKI( "Parent" ),
                    NO_SETTER( PAD, wxString ), &PAD::GetParentAsString ) );

        // TODO delta, dirill shape offset, layerset, zone connection
    }
} _PAD_DESC;

ENUM_TO_WXANY( PAD_ATTR_T );
ENUM_TO_WXANY( PAD_SHAPE_T );
ENUM_TO_WXANY( PAD_PROP_T );
