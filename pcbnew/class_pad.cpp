/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
  * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file class_pad.cpp
 * D_PAD class implementation.
 */

#include <fctsys.h>
#include <trigo.h>
#include <macros.h>
#include <msgpanel.h>
#include <base_units.h>
#include <bitmaps.h>
#include <math/util.h>      // for KiROUND
#include <eda_draw_frame.h>
#include <geometry/shape_circle.h>
#include <geometry/shape_segment.h>
#include <geometry/shape_simple.h>
#include <geometry/shape_rect.h>
#include <pcbnew.h>
#include <view/view.h>
#include <class_board.h>
#include <class_module.h>
#include <class_drawsegment.h>
#include <geometry/polygon_test_point_inside.h>
#include <convert_to_biu.h>
#include <convert_basic_shapes_to_polygon.h>

#include <memory>

D_PAD::D_PAD( MODULE* parent ) :
    BOARD_CONNECTED_ITEM( parent, PCB_PAD_T )
{
    m_Size.x = m_Size.y   = Mils2iu( 60 );  // Default pad size 60 mils.
    m_Drill.x = m_Drill.y = Mils2iu( 30 );  // Default drill size 30 mils.
    m_Orient              = 0;              // Pad rotation in 1/10 degrees.
    m_LengthPadToDie      = 0;

    if( m_Parent  &&  m_Parent->Type() == PCB_MODULE_T )
    {
        m_Pos = GetParent()->GetPosition();
    }

    SetShape( PAD_SHAPE_CIRCLE );                   // Default pad shape is PAD_CIRCLE.
    SetAnchorPadShape( PAD_SHAPE_CIRCLE );          // Default shape for custom shaped pads
                                                    // is PAD_CIRCLE.
    SetDrillShape( PAD_DRILL_SHAPE_CIRCLE );        // Default pad drill shape is a circle.
    m_Attribute           = PAD_ATTRIB_STANDARD;    // Default pad type is NORMAL (thru hole)
    SetProperty( PAD_PROP_NONE );                   // no special fabrication property
    m_LocalClearance      = 0;
    m_LocalSolderMaskMargin  = 0;
    m_LocalSolderPasteMargin = 0;
    m_LocalSolderPasteMarginRatio = 0.0;
    // Parameters for round rect only:
    m_padRoundRectRadiusScale = 0.25;               // from  IPC-7351C standard
    // Parameters for chamfered rect only:
    m_padChamferRectScale = 0.2;                   // Size of chamfer: ratio of smallest of X,Y size
    m_chamferPositions  = RECT_NO_CHAMFER;          // No chamfered corner

    m_ZoneConnection    = ZONE_CONNECTION::INHERITED; // Use parent setting by default
    m_ThermalWidth      = 0;                        // Use parent setting by default
    m_ThermalGap        = 0;                        // Use parent setting by default

    m_customShapeClearanceArea = CUST_PAD_SHAPE_IN_ZONE_OUTLINE;

    // Set layers mask to default for a standard thru hole pad.
    m_layerMask           = StandardMask();

    SetSubRatsnest( 0 );                       // used in ratsnest calculations

    m_shapesDirty = true;
    m_effectiveBoundingRadius = 0;
}


LSET D_PAD::StandardMask()
{
    static LSET saved = LSET::AllCuMask() | LSET( 2, B_Mask, F_Mask );
    return saved;
}


LSET D_PAD::SMDMask()
{
    static LSET saved( 3, F_Cu, F_Paste, F_Mask );
    return saved;
}


LSET D_PAD::ConnSMDMask()
{
    static LSET saved( 2, F_Cu, F_Mask );
    return saved;
}


LSET D_PAD::UnplatedHoleMask()
{
    static LSET saved = LSET::AllCuMask() | LSET( 2, B_Mask, F_Mask );
    return saved;
}


LSET D_PAD::ApertureMask()
{
    static LSET saved( 1, F_Paste );
    return saved;
}


bool D_PAD::IsFlipped() const
{
    if( GetParent() &&  GetParent()->GetLayer() == B_Cu )
        return true;
    return false;
}


int D_PAD::calcBoundingRadius() const
{
    int radius = 0;
    SHAPE_POLY_SET polygons;
    TransformShapeWithClearanceToPolygon( polygons, 0 );

    for( int cnt = 0; cnt < polygons.OutlineCount(); ++cnt )
    {
        const SHAPE_LINE_CHAIN& poly = polygons.COutline( cnt );

        for( int ii = 0; ii < poly.PointCount(); ++ii )
        {
            int dist = KiROUND( ( poly.CPoint( ii ) - m_Pos ).EuclideanNorm() );
            radius = std::max( radius, dist );
        }
    }

    return radius + 1;
}


int D_PAD::GetRoundRectCornerRadius() const
{
    return KiROUND( std::min( m_Size.x, m_Size.y ) * m_padRoundRectRadiusScale );
}


void D_PAD::SetRoundRectCornerRadius( double aRadius )
{
    int min_r = std::min( m_Size.x, m_Size.y );

    if( min_r > 0 )
        SetRoundRectRadiusRatio( aRadius / min_r );
}


void D_PAD::SetRoundRectRadiusRatio( double aRadiusScale )
{
    m_padRoundRectRadiusScale = std::max( 0.0, std::min( aRadiusScale, 0.5 ) );

    m_shapesDirty = true;
}


void D_PAD::SetChamferRectRatio( double aChamferScale )
{
    m_padChamferRectScale = std::max( 0.0, std::min( aChamferScale, 0.5 ) );

    m_shapesDirty = true;
}


const std::vector<std::shared_ptr<SHAPE>>& D_PAD::GetEffectiveShapes() const
{
    if( m_shapesDirty )
        BuildEffectiveShapes();

    return m_effectiveShapes;
}


const std::shared_ptr<SHAPE_SEGMENT>& D_PAD::GetEffectiveHoleShape() const
{
    if( m_shapesDirty )
        BuildEffectiveShapes();

    return m_effectiveHoleShape;
}


int D_PAD::GetBoundingRadius() const
{
    if( m_shapesDirty )
        BuildEffectiveShapes();

    return m_effectiveBoundingRadius;
}


void D_PAD::BuildEffectiveShapes() const
{
    m_effectiveShapes.clear();
    m_effectiveHoleShape = nullptr;

    auto add = [this]( SHAPE* aShape )
               {
                   m_effectiveShapes.emplace_back( aShape );
               };

    wxPoint shapePos = ShapePos();  // Fetch only once; rotation involves trig
    PAD_SHAPE_T effectiveShape = GetShape();

    if( GetShape() == PAD_SHAPE_CUSTOM )
        effectiveShape = GetAnchorPadShape();

    switch( effectiveShape )
    {
    case PAD_SHAPE_CIRCLE:
        add( new SHAPE_CIRCLE( shapePos, m_Size.x / 2 ) );
        break;

    case PAD_SHAPE_OVAL:
    {
        wxSize  half_size = m_Size / 2;
        int     half_width = std::min( half_size.x, half_size.y );
        wxPoint half_len( half_size.x - half_width, half_size.y - half_width );

        RotatePoint( &half_len, m_Orient );

        add( new SHAPE_SEGMENT( shapePos - half_len, shapePos + half_len, half_width * 2 ) );
    }
        break;

    case PAD_SHAPE_RECT:
        if( m_Orient == 0 || m_Orient == 1800 )
        {
            add( new SHAPE_RECT( shapePos - m_Size / 2, m_Size.x, m_Size.y ) );
            break;
        }
        else if( m_Orient == 900 || m_Orient == -900 )
        {
            wxSize rot_size( m_Size.y, m_Size.x );
            add( new SHAPE_RECT( shapePos - rot_size / 2, rot_size.x, rot_size.y ) );
            break;
        }

        // Not at a cartesian angle; fall through to general case
        KI_FALLTHROUGH;

    case PAD_SHAPE_TRAPEZOID:
    case PAD_SHAPE_ROUNDRECT:
    {
        int     r = GetRoundRectCornerRadius();
        wxPoint half_size( m_Size.x / 2, m_Size.y / 2 );
        wxSize  trap_delta( 0, 0 );

        if( effectiveShape == PAD_SHAPE_ROUNDRECT )
            half_size -= wxPoint( r, r );
        else if( effectiveShape == PAD_SHAPE_TRAPEZOID )
            trap_delta = m_DeltaSize / 2;

        SHAPE_LINE_CHAIN corners;

        corners.Append( -half_size.x + trap_delta.y, -half_size.y - trap_delta.x );
        corners.Append(  half_size.x - trap_delta.y, -half_size.y + trap_delta.x );
        corners.Append(  half_size.x + trap_delta.y,  half_size.y - trap_delta.x );
        corners.Append( -half_size.x - trap_delta.y,  half_size.y + trap_delta.x );

        corners.Rotate( -DECIDEG2RAD( m_Orient ) );
        corners.Move( shapePos );

        add( new SHAPE_SIMPLE( corners ) );

        if( effectiveShape == PAD_SHAPE_ROUNDRECT )
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
        auto board = GetBoard();
        int maxError = ARC_HIGH_DEF;

        if( board )
            maxError = board->GetDesignSettings().m_MaxError;

        TransformRoundChamferedRectToPolygon( outline, shapePos, GetSize(), m_Orient,
                                              GetRoundRectCornerRadius(), GetChamferRectRatio(),
                                              GetChamferPositions(), maxError );

        add( new SHAPE_SIMPLE( outline.COutline( 0 ) ) );
    }
        break;

    default:
        wxFAIL_MSG( "D_PAD::buildEffectiveShapes: Unsupported pad shape: "
                    + PAD_SHAPE_T_asString( effectiveShape ) );
        break;
    }

    if( GetShape() == PAD_SHAPE_CUSTOM )
    {
        for( const std::shared_ptr<DRAWSEGMENT>& primitive : m_editPrimitives )
        {
            for( SHAPE* shape : primitive->MakeEffectiveShapes() )
            {
                shape->Rotate( -DECIDEG2RAD( m_Orient ) );
                shape->Move( shapePos );
                add( shape );
            }
        }
    }

    // Bounding box and radius
    //
    m_effectiveBoundingRadius = calcBoundingRadius();

    // reset the bbox to uninitialized state to prepare for merging
    m_effectiveBoundingBox = EDA_RECT();

    for( const std::shared_ptr<SHAPE>& shape : m_effectiveShapes )
    {
        BOX2I r = shape->BBox();
        m_effectiveBoundingBox.Merge( EDA_RECT( (wxPoint) r.GetOrigin(),
                                      wxSize( r.GetWidth(), r.GetHeight() ) ) );
    }
    // Hole shape
    //
    wxSize  half_size = m_Drill / 2;
    int     half_width = std::min( half_size.x, half_size.y );
    wxPoint half_len( half_size.x - half_width, half_size.y - half_width );

    RotatePoint( &half_len, m_Orient );

    m_effectiveHoleShape = std::make_shared<SHAPE_SEGMENT>( m_Pos - half_len, m_Pos + half_len,
                                                            half_width * 2 );

    // All done
    //
    m_shapesDirty = false;
}


const EDA_RECT D_PAD::GetBoundingBox() const
{
    if( m_shapesDirty )
        BuildEffectiveShapes();

    return m_effectiveBoundingBox;
}


void D_PAD::SetDrawCoord()
{
    MODULE* module = (MODULE*) m_Parent;

    m_Pos = m_Pos0;

    if( module == NULL )
        return;

    double angle = module->GetOrientation();

    RotatePoint( &m_Pos.x, &m_Pos.y, angle );
    m_Pos += module->GetPosition();
}


void D_PAD::SetLocalCoord()
{
    MODULE* module = (MODULE*) m_Parent;

    if( module == NULL )
    {
        m_Pos0 = m_Pos;
        return;
    }

    m_Pos0 = m_Pos - module->GetPosition();
    RotatePoint( &m_Pos0.x, &m_Pos0.y, -module->GetOrientation() );
}


void D_PAD::SetAttribute( PAD_ATTR_T aAttribute )
{
    m_Attribute = aAttribute;

    if( aAttribute == PAD_ATTRIB_SMD )
        m_Drill = wxSize( 0, 0 );

    m_shapesDirty = true;
}


void D_PAD::SetProperty( PAD_PROP_T aProperty )
{
    m_Property = aProperty;

    m_shapesDirty = true;
}


void D_PAD::SetOrientation( double aAngle )
{
    NORMALIZE_ANGLE_POS( aAngle );
    m_Orient = aAngle;

    m_shapesDirty = true;
}


void D_PAD::Flip( const wxPoint& aCentre, bool aFlipLeftRight )
{
    if( aFlipLeftRight )
    {
        MIRROR( m_Pos.x, aCentre.x );
        MIRROR( m_Pos0.x, 0 );
        MIRROR( m_Offset.x, 0 );
        MIRROR( m_DeltaSize.x, 0 );
    }
    else
    {
        MIRROR( m_Pos.y, aCentre.y );
        MIRROR( m_Pos0.y, 0 );
        MIRROR( m_Offset.y, 0 );
        MIRROR( m_DeltaSize.y, 0 );
    }

    SetOrientation( -GetOrientation() );

    // flip pads layers
    // PADS items are currently on all copper layers, or
    // currently, only on Front or Back layers.
    // So the copper layers count is not taken in account
    SetLayerSet( FlipLayerMask( m_layerMask ) );

    // Flip the basic shapes, in custom pads
    FlipPrimitives();

    m_shapesDirty = true;
}


// Flip the basic shapes, in custom pads
void D_PAD::FlipPrimitives()
{
    for( std::shared_ptr<DRAWSEGMENT>& primitive : m_editPrimitives )
        primitive->Flip( wxPoint( 0, 0 ), false );

    m_shapesDirty = true;
}


void D_PAD::MirrorXPrimitives( int aX )
{
    for( std::shared_ptr<DRAWSEGMENT>& primitive : m_editPrimitives )
        primitive->Flip( wxPoint( aX, 0 ), true );

    m_shapesDirty = true;
}


// Returns the position of the pad.
wxPoint D_PAD::ShapePos() const
{
    if( m_Offset.x == 0 && m_Offset.y == 0 )
        return m_Pos;

    wxPoint loc_offset = m_Offset;

    RotatePoint( &loc_offset, m_Orient );

    wxPoint shape_pos = m_Pos + loc_offset;

    return shape_pos;
}


int D_PAD::GetLocalClearanceOverrides( wxString* aSource ) const
{
    // A pad can have specific clearance that overrides its NETCLASS clearance value
    if( GetLocalClearance() )
        return GetLocalClearance( aSource );

    // A footprint can have a specific clearance value
    if( GetParent() && GetParent()->GetLocalClearance() )
        return GetParent()->GetLocalClearance( aSource );

    return 0;
}


int D_PAD::GetLocalClearance( wxString* aSource ) const
{
    if( aSource )
        *aSource = wxString::Format( _( "pad %s" ), GetName() );

    return m_LocalClearance;
}


// Mask margins handling:

int D_PAD::GetSolderMaskMargin() const
{
    // The pad inherits the margin only to calculate a default shape,
    // therefore only if it is also a copper layer
    // Pads defined only on mask layers (and perhaps on other tech layers) use the shape
    // defined by the pad settings only
    bool isOnCopperLayer = ( m_layerMask & LSET::AllCuMask() ).any();

    if( !isOnCopperLayer )
        return 0;

    int     margin = m_LocalSolderMaskMargin;

    MODULE* module = GetParent();

    if( module )
    {
        if( margin == 0 )
        {
            if( module->GetLocalSolderMaskMargin() )
                margin = module->GetLocalSolderMaskMargin();
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
        int minsize = -std::min( m_Size.x, m_Size.y ) / 2;

        if( margin < minsize )
            margin = minsize;
    }

    return margin;
}


wxSize D_PAD::GetSolderPasteMargin() const
{
    // The pad inherits the margin only to calculate a default shape,
    // therefore only if it is also a copper layer.
    // Pads defined only on mask layers (and perhaps on other tech layers) use the shape
    // defined by the pad settings only
    bool isOnCopperLayer = ( m_layerMask & LSET::AllCuMask() ).any();

    if( !isOnCopperLayer )
        return wxSize( 0, 0 );

    int     margin = m_LocalSolderPasteMargin;
    double  mratio = m_LocalSolderPasteMarginRatio;

    MODULE* module = GetParent();

    if( module )
    {
        if( margin == 0 )
            margin = module->GetLocalSolderPasteMargin();

        auto brd = GetBoard();

        if( margin == 0 && brd )
        {
                margin = brd->GetDesignSettings().m_SolderPasteMargin;
        }

        if( mratio == 0.0 )
            mratio = module->GetLocalSolderPasteMarginRatio();

        if( mratio == 0.0 && brd )
        {
            mratio = brd->GetDesignSettings().m_SolderPasteMarginRatio;
        }
    }

    wxSize pad_margin;
    pad_margin.x = margin + KiROUND( m_Size.x * mratio );
    pad_margin.y = margin + KiROUND( m_Size.y * mratio );

    // ensure mask have a size always >= 0
    if( pad_margin.x < -m_Size.x / 2 )
        pad_margin.x = -m_Size.x / 2;

    if( pad_margin.y < -m_Size.y / 2 )
        pad_margin.y = -m_Size.y / 2;

    return pad_margin;
}


ZONE_CONNECTION D_PAD::GetEffectiveZoneConnection() const
{
    MODULE* module = GetParent();

    if( m_ZoneConnection == ZONE_CONNECTION::INHERITED && module )
        return module->GetZoneConnection();
    else
        return m_ZoneConnection;
}


int D_PAD::GetThermalWidth() const
{
    MODULE* module = GetParent();

    if( m_ThermalWidth == 0 && module )
        return module->GetThermalWidth();
    else
        return m_ThermalWidth;
}


int D_PAD::GetThermalGap() const
{
    MODULE* module = GetParent();

    if( m_ThermalGap == 0 && module )
        return module->GetThermalGap();
    else
        return m_ThermalGap;
}


void D_PAD::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    EDA_UNITS              units = aFrame->GetUserUnits();
    wxString               msg, msg2;
    BOARD*                 board = GetBoard();
    BOARD_DESIGN_SETTINGS& bds = board->GetDesignSettings();
    MODULE*                module = (MODULE*) m_Parent;

    if( module )
        aList.emplace_back( _( "Footprint" ), module->GetReference(), DARKCYAN );

    aList.emplace_back( _( "Pad" ), m_name, BROWN );

    if( !GetPinFunction().IsEmpty() )
        aList.emplace_back( _( "Pin Name" ), GetPinFunction(), BROWN );

    aList.emplace_back( _( "Net" ), UnescapeString( GetNetname() ), DARKCYAN );

    // Display the netclass name (a pad having a netcode = 0 (no net) use the
    // default netclass for clearance):
    if( m_netinfo->GetNet() <= 0 )
        msg = bds.GetDefault()->GetName();
    else
        msg = GetNetClassName();

    aList.emplace_back( _( "NetClass" ), msg, CYAN );

    aList.emplace_back( _( "Layer" ), LayerMaskDescribe( board, m_layerMask ), DARKGREEN );

    // Show the pad shape, attribute and property
    wxString props = ShowPadAttr();

    if( GetProperty() != PAD_PROP_NONE )
        props += ',';

    switch( GetProperty() )
    {
    case PAD_PROP_NONE: break;
    case PAD_PROP_BGA:              props += _("BGA" ); break;
    case PAD_PROP_FIDUCIAL_GLBL:    props += _("Fiducial global" ); break;
    case PAD_PROP_FIDUCIAL_LOCAL:   props += _("Fiducial local" ); break;
    case PAD_PROP_TESTPOINT:        props += _("Test point" ); break;
    case PAD_PROP_HEATSINK:         props += _("Heat sink" ); break;
    case PAD_PROP_CASTELLATED:      props += _("Castellated" ); break;
    }

    aList.emplace_back( ShowPadShape(), props, DARKGREEN );

    if( (GetShape() == PAD_SHAPE_CIRCLE || GetShape() == PAD_SHAPE_OVAL )
        && m_Size.x == m_Size.y )
    {
        msg = MessageTextFromValue( units, m_Size.x, true );
        aList.emplace_back( _( "Diameter" ), msg, RED );
    }
    else
    {
        msg = MessageTextFromValue( units, m_Size.x, true );
        aList.emplace_back( _( "Width" ), msg, RED );

        msg = MessageTextFromValue( units, m_Size.y, true );
        aList.emplace_back( _( "Height" ), msg, RED );
    }

    if( GetPadToDieLength() )
    {
        msg = MessageTextFromValue(units, GetPadToDieLength(), true );
        aList.emplace_back( _( "Length in Package" ), msg, CYAN );
    }

    msg = MessageTextFromValue( units, m_Drill.x, true );

    if( GetDrillShape() == PAD_DRILL_SHAPE_CIRCLE )
    {
        aList.emplace_back( _( "Drill" ), msg, RED );
    }
    else
    {
        msg = MessageTextFromValue( units, m_Drill.x, true )
               + wxT( "/" )
               + MessageTextFromValue( units, m_Drill.y, true );
        aList.emplace_back( _( "Drill X / Y" ), msg, RED );
    }

    wxString source;
    int      clearance = GetClearance( nullptr, &source );

    msg.Printf( _( "Min Clearance: %s" ), MessageTextFromValue( units, clearance, true ) );
    msg2.Printf( _( "(from %s)" ), source );
    aList.emplace_back( msg, msg2, BLACK );
}


void D_PAD::GetOblongGeometry( const wxSize& aDrillOrPadSize,
                               wxPoint* aStartPoint, wxPoint* aEndPoint, int* aWidth ) const
{
    // calculates the start point, end point and width
    // of an equivalent segment which have the same position and width as the pad or hole
    int delta_cx, delta_cy;

    wxSize  halfsize = aDrillOrPadSize / 2;
    wxPoint offset;

    if( aDrillOrPadSize.x > aDrillOrPadSize.y )  // horizontal
    {
        delta_cx = halfsize.x - halfsize.y;
        delta_cy = 0;
        *aWidth   = aDrillOrPadSize.y;
    }
    else                                        // vertical
    {
        delta_cx = 0;
        delta_cy = halfsize.y - halfsize.x;
        *aWidth   = aDrillOrPadSize.x;
    }

    RotatePoint( &delta_cx, &delta_cy, m_Orient );

    aStartPoint->x = delta_cx + offset.x;
    aStartPoint->y = delta_cy + offset.y;

    aEndPoint->x = - delta_cx + offset.x;
    aEndPoint->y = - delta_cy + offset.y;
}


bool D_PAD::HitTest( const wxPoint& aPosition, int aAccuracy ) const
{
    VECTOR2I delta = aPosition - GetPosition();
    int      boundingRadius = GetBoundingRadius() + aAccuracy;

    if( delta.SquaredEuclideanNorm() > SEG::Square( boundingRadius ) )
        return false;

    SHAPE_POLY_SET polySet;
    TransformShapeWithClearanceToPolygon( polySet, aAccuracy );

    return polySet.Contains( aPosition );
}


bool D_PAD::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
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

    SHAPE_POLY_SET padPoly;
    TransformShapeWithClearanceToPolygon( padPoly, aAccuracy );

    selRect.BooleanIntersection( padPoly, SHAPE_POLY_SET::PM_FAST );

    double padArea = padPoly.Outline( 0 ).Area();
    double intersection = selRect.Outline( 0 ).Area();

    if( intersection > ( padArea * 0.99 ) )
        return true;
    else
        return !aContained && intersection > 0;
}


int D_PAD::Compare( const D_PAD* padref, const D_PAD* padcmp )
{
    int diff;

    if( ( diff = padref->GetShape() - padcmp->GetShape() ) != 0 )
        return diff;

    if( ( diff = padref->GetDrillShape() - padcmp->GetDrillShape() ) != 0)
        return diff;

    if( ( diff = padref->m_Drill.x - padcmp->m_Drill.x ) != 0 )
        return diff;

    if( ( diff = padref->m_Drill.y - padcmp->m_Drill.y ) != 0 )
        return diff;

    if( ( diff = padref->m_Size.x - padcmp->m_Size.x ) != 0 )
        return diff;

    if( ( diff = padref->m_Size.y - padcmp->m_Size.y ) != 0 )
        return diff;

    if( ( diff = padref->m_Offset.x - padcmp->m_Offset.x ) != 0 )
        return diff;

    if( ( diff = padref->m_Offset.y - padcmp->m_Offset.y ) != 0 )
        return diff;

    if( ( diff = padref->m_DeltaSize.x - padcmp->m_DeltaSize.x ) != 0 )
        return diff;

    if( ( diff = padref->m_DeltaSize.y - padcmp->m_DeltaSize.y ) != 0 )
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


void D_PAD::Rotate( const wxPoint& aRotCentre, double aAngle )
{
    RotatePoint( &m_Pos, aRotCentre, aAngle );

    m_Orient = NormalizeAngle360Min( m_Orient + aAngle );

    SetLocalCoord();

    m_shapesDirty = true;
}


wxString D_PAD::ShowPadShape() const
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


wxString D_PAD::ShowPadAttr() const
{
    switch( GetAttribute() )
    {
    case PAD_ATTRIB_STANDARD:        return _( "Std" );
    case PAD_ATTRIB_SMD:             return _( "SMD" );
    case PAD_ATTRIB_CONN:            return _( "Conn" );
    case PAD_ATTRIB_HOLE_NOT_PLATED: return _( "Not Plated" );
    default:                         return wxT( "???" );
    }
}


wxString D_PAD::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    if( GetName().IsEmpty() )
    {
        return wxString::Format( _( "Pad of %s on %s" ),
                                 GetParent()->GetReference(),
                                 LayerMaskDescribe( GetBoard(), m_layerMask ) );
    }
    else
    {
        return wxString::Format( _( "Pad %s of %s on %s" ),
                                 GetName(),
                                 GetParent()->GetReference(),
                                 LayerMaskDescribe( GetBoard(), m_layerMask ) );
    }
}


BITMAP_DEF D_PAD::GetMenuImage() const
{
    return pad_xpm;
}


EDA_ITEM* D_PAD::Clone() const
{
    return new D_PAD( *this );
}


bool D_PAD::PadShouldBeNPTH() const
{
    return( m_Attribute == PAD_ATTRIB_STANDARD
            && m_Drill.x >= m_Size.x && m_Drill.y >= m_Size.y );
}


void D_PAD::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount = 0;

    // These 2 types of pads contain a hole
    if( m_Attribute == PAD_ATTRIB_STANDARD )
        aLayers[aCount++] = LAYER_PADS_PLATEDHOLES;

    if( m_Attribute == PAD_ATTRIB_HOLE_NOT_PLATED )
        aLayers[aCount++] = LAYER_NON_PLATEDHOLES;

    if( IsOnLayer( F_Cu ) && IsOnLayer( B_Cu ) )
    {
        // Multi layer pad
        aLayers[aCount++] = LAYER_PADS_TH;
        aLayers[aCount++] = LAYER_PADS_NETNAMES;
    }
    else if( IsOnLayer( F_Cu ) )
    {
        aLayers[aCount++] = LAYER_PAD_FR;

        // Is this a PTH pad that has only front copper?  If so, we need to also display the
        // net name on the PTH netname layer so that it isn't blocked by the drill hole.
        if( m_Attribute == PAD_ATTRIB_STANDARD )
            aLayers[aCount++] = LAYER_PADS_NETNAMES;
        else
            aLayers[aCount++] = LAYER_PAD_FR_NETNAMES;
    }
    else if( IsOnLayer( B_Cu ) )
    {
        aLayers[aCount++] = LAYER_PAD_BK;

        // Is this a PTH pad that has only back copper?  If so, we need to also display the
        // net name on the PTH netname layer so that it isn't blocked by the drill hole.
        if( m_Attribute == PAD_ATTRIB_STANDARD )
            aLayers[aCount++] = LAYER_PADS_NETNAMES;
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


unsigned int D_PAD::ViewGetLOD( int aLayer, KIGFX::VIEW* aView ) const
{
    if( aView->GetPrintMode() > 0 )  // In printing mode the pad is always drawable
        return 0;

    const int HIDE = std::numeric_limits<unsigned int>::max();
    BOARD* board = GetBoard();

    // Handle Render tab switches
    if( ( GetAttribute() == PAD_ATTRIB_STANDARD || GetAttribute() == PAD_ATTRIB_HOLE_NOT_PLATED )
         && !aView->IsLayerVisible( LAYER_PADS_TH ) )
        return HIDE;

    if( !IsFlipped() && !aView->IsLayerVisible( LAYER_MOD_FR ) )
        return HIDE;

    if( IsFlipped() && !aView->IsLayerVisible( LAYER_MOD_BK ) )
        return HIDE;

    if( IsFrontLayer( ( PCB_LAYER_ID )aLayer ) && !aView->IsLayerVisible( LAYER_PAD_FR ) )
        return HIDE;

    if( IsBackLayer( ( PCB_LAYER_ID )aLayer ) && !aView->IsLayerVisible( LAYER_PAD_BK ) )
        return HIDE;

    // Only draw the pad if at least one of the layers it crosses is being displayed
    if( board && !( board->GetVisibleLayers() & GetLayerSet() ).any() )
        return HIDE;

    // Netnames will be shown only if zoom is appropriate
    if( IsNetnameLayer( aLayer ) )
    {
        int divisor = std::min( GetBoundingBox().GetWidth(), GetBoundingBox().GetHeight() );

        // Pad sizes can be zero briefly when someone is typing a number like "0.5"
        // in the pad properties dialog
        if( divisor == 0 )
            return HIDE;

        return ( Millimeter2iu( 5 ) / divisor );
    }

    // Other layers are shown without any conditions
    return 0;
}


const BOX2I D_PAD::ViewBBox() const
{
    // Bounding box includes soldermask too
    int solderMaskMargin       = GetSolderMaskMargin();
    VECTOR2I solderPasteMargin = VECTOR2D( GetSolderPasteMargin() );
    EDA_RECT bbox              = GetBoundingBox();

    // Look for the biggest possible bounding box
    int xMargin = std::max( solderMaskMargin, solderPasteMargin.x );
    int yMargin = std::max( solderMaskMargin, solderPasteMargin.y );

    return BOX2I( VECTOR2I( bbox.GetOrigin() ) - VECTOR2I( xMargin, yMargin ),
                  VECTOR2I( bbox.GetSize() ) + VECTOR2I( 2 * xMargin, 2 * yMargin ) );
}


void D_PAD::ImportSettingsFrom( const D_PAD& aMasterPad )
{
    SetShape( aMasterPad.GetShape() );
    SetLayerSet( aMasterPad.GetLayerSet() );
    SetAttribute( aMasterPad.GetAttribute() );
    SetProperty( aMasterPad.GetProperty() );

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
    SetThermalWidth( aMasterPad.GetThermalWidth() );
    SetThermalGap( aMasterPad.GetThermalGap() );

    // Add or remove custom pad shapes:
    SetPrimitives( aMasterPad.GetPrimitives() );
    SetAnchorPadShape( aMasterPad.GetAnchorPadShape() );

    m_shapesDirty = true;
}


void D_PAD::SwapData( BOARD_ITEM* aImage )
{
    assert( aImage->Type() == PCB_PAD_T );

    std::swap( *((MODULE*) this), *((MODULE*) aImage) );
}
