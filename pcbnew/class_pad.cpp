/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
  * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <geometry/geometry_utils.h>
#include <pcbnew.h>
#include <view/view.h>

#include <class_board.h>
#include <class_module.h>
#include <polygon_test_point_inside.h>
#include <convert_to_biu.h>
#include <convert_basic_shapes_to_polygon.h>


/**
 * Helper function
 * Return a string (to be shown to the user) describing a layer mask.
 * Useful for showing where is a pad.
 * The BOARD is needed because layer names are (somewhat) customizable
 */
static wxString LayerMaskDescribe( const BOARD* aBoard, LSET aMask );

int D_PAD::m_PadSketchModePenSize = 0;      // Pen size used to draw pads in sketch mode

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
    m_LocalClearance      = 0;
    m_LocalSolderMaskMargin  = 0;
    m_LocalSolderPasteMargin = 0;
    m_LocalSolderPasteMarginRatio = 0.0;
    // Parameters for round rect only:
    m_padRoundRectRadiusScale = 0.25;               // from  IPC-7351C standard
    // Parameters for chamfered rect only:
    m_padChamferRectScale = 0.2;                   // Size of chamfer: ratio of smallest of X,Y size
    m_chamferPositions  = RECT_NO_CHAMFER;          // No chamfered corner

    m_ZoneConnection    = PAD_ZONE_CONN_INHERITED;  // Use parent setting by default
    m_ThermalWidth      = 0;                        // Use parent setting by default
    m_ThermalGap        = 0;                        // Use parent setting by default

    m_customShapeClearanceArea = CUST_PAD_SHAPE_IN_ZONE_OUTLINE;

    // Set layers mask to default for a standard thru hole pad.
    m_layerMask           = StandardMask();

    SetSubRatsnest( 0 );                       // used in ratsnest calculations

    m_boundingRadius      = -1;
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
    static LSET saved = LSET( 1, F_Paste );
    return saved;
}


bool D_PAD::IsFlipped() const
{
    if( GetParent() &&  GetParent()->GetLayer() == B_Cu )
        return true;
    return false;
}

int D_PAD::boundingRadius() const
{
    int x, y;
    int radius;

    switch( GetShape() )
    {
    case PAD_SHAPE_CIRCLE:
        radius = m_Size.x / 2;
        break;

    case PAD_SHAPE_OVAL:
        radius = std::max( m_Size.x, m_Size.y ) / 2;
        break;

    case PAD_SHAPE_RECT:
        radius = 1 + KiROUND( EuclideanNorm( m_Size ) / 2 );
        break;

    case PAD_SHAPE_TRAPEZOID:
        x = m_Size.x + std::abs( m_DeltaSize.y );   // Remember: m_DeltaSize.y is the m_Size.x change
        y = m_Size.y + std::abs( m_DeltaSize.x );   // Remember: m_DeltaSize.x is the m_Size.y change
        radius = 1 + KiROUND( hypot( x, y ) / 2 );
        break;

    case PAD_SHAPE_ROUNDRECT:
        radius = GetRoundRectCornerRadius();
        x = m_Size.x >> 1;
        y = m_Size.y >> 1;
        radius += 1 + KiROUND( EuclideanNorm( wxSize( x - radius, y - radius )));
        break;

    case PAD_SHAPE_CHAMFERED_RECT:
        radius = GetRoundRectCornerRadius();
        x = m_Size.x >> 1;
        y = m_Size.y >> 1;
        radius += 1 + KiROUND( EuclideanNorm( wxSize( x - radius, y - radius )));
        // TODO: modify radius if the chamfer is smaller than corner radius
        break;

    case PAD_SHAPE_CUSTOM:
        radius = 0;

        for( int cnt = 0; cnt < m_customShapeAsPolygon.OutlineCount(); ++cnt )
        {
            const SHAPE_LINE_CHAIN& poly = m_customShapeAsPolygon.COutline( cnt );
            for( int ii = 0; ii < poly.PointCount(); ++ii )
            {
                int dist = KiROUND( poly.CPoint( ii ).EuclideanNorm() );
                radius = std::max( radius, dist );
            }
        }

        radius += 1;
        break;

    default:
        radius = 0;
    }

    return radius;
}


int D_PAD::GetRoundRectCornerRadius( const wxSize& aSize ) const
{
    // radius of rounded corners, usually 25% of shorter pad edge for now
    int r = aSize.x > aSize.y ? aSize.y : aSize.x;
    r = int( r * m_padRoundRectRadiusScale );

    return r;
}


void D_PAD::SetRoundRectCornerRadius( double aRadius )
{
    int min_r = std::min( m_Size.x, m_Size.y );

    if( min_r > 0 )
        SetRoundRectRadiusRatio( aRadius / min_r );
}


const EDA_RECT D_PAD::GetBoundingBox() const
{
    EDA_RECT area;
    wxPoint quadrant1, quadrant2, quadrant3, quadrant4;
    int x, y, r, dx, dy;

    wxPoint center = ShapePos();
    wxPoint endPoint;

    EDA_RECT endRect;

    switch( GetShape() )
    {
    case PAD_SHAPE_CIRCLE:
        area.SetOrigin( center );
        area.Inflate( m_Size.x / 2 );
        break;

    case PAD_SHAPE_OVAL:
        /* To get the BoundingBox of an oval pad:
         * a) If the pad is ROUND, see method for PAD_SHAPE_CIRCLE above
         * OTHERWISE:
         * b) Construct EDA_RECT for portion between circular ends
         * c) Rotate that EDA_RECT
         * d) Add the circular ends to the EDA_RECT
         */

        // Test if the shape is circular
        if( m_Size.x == m_Size.y )
        {
            area.SetOrigin( center );
            area.Inflate( m_Size.x / 2 );
            break;
        }

        if( m_Size.x > m_Size.y )
        {
            // Pad is horizontal
            dx = ( m_Size.x - m_Size.y ) / 2;
            dy = m_Size.y / 2;

            // Location of end-points
            x = dx;
            y = 0;
            r = dy;
        }
        else
        {
            // Pad is vertical
            dx = m_Size.x / 2;
            dy = ( m_Size.y - m_Size.x ) / 2;

            x = 0;
            y = dy;
            r = dx;
        }

        // Construct the center rectangle and rotate
        area.SetOrigin( center );
        area.Inflate( dx, dy );
        area = area.GetBoundingBoxRotated( center, m_Orient );

        endPoint = wxPoint( x, y );
        RotatePoint( &endPoint, m_Orient );

        // Add points at each quadrant of circular regions
        endRect.SetOrigin( center + endPoint );
        endRect.Inflate( r );

        area.Merge( endRect );

        endRect.SetSize( 0, 0 );
        endRect.SetOrigin( center - endPoint );
        endRect.Inflate( r );

        area.Merge( endRect );

        break;

    case PAD_SHAPE_RECT:
    case PAD_SHAPE_ROUNDRECT:
    case PAD_SHAPE_CHAMFERED_RECT:
        // Use two opposite corners and track their rotation
        // (use symmetry for other points)
        quadrant1.x =  m_Size.x/2;
        quadrant1.y =  m_Size.y/2;
        quadrant2.x = -m_Size.x/2;
        quadrant2.y =  m_Size.y/2;

        RotatePoint( &quadrant1, m_Orient );
        RotatePoint( &quadrant2, m_Orient );
        dx = std::max( std::abs( quadrant1.x ) , std::abs( quadrant2.x )  );
        dy = std::max( std::abs( quadrant1.y ) , std::abs( quadrant2.y )  );

        // Set the bbox
        area.SetOrigin( ShapePos() );
        area.Inflate( dx, dy );
        break;

    case PAD_SHAPE_TRAPEZOID:
        // Use the four corners and track their rotation
        // (Trapezoids will not be symmetric)

        quadrant1.x =  (m_Size.x + m_DeltaSize.y)/2;
        quadrant1.y =  (m_Size.y - m_DeltaSize.x)/2;

        quadrant2.x = -(m_Size.x + m_DeltaSize.y)/2;
        quadrant2.y =  (m_Size.y + m_DeltaSize.x)/2;

        quadrant3.x = -(m_Size.x - m_DeltaSize.y)/2;
        quadrant3.y = -(m_Size.y + m_DeltaSize.x)/2;

        quadrant4.x =  (m_Size.x - m_DeltaSize.y)/2;
        quadrant4.y = -(m_Size.y - m_DeltaSize.x)/2;

        RotatePoint( &quadrant1, m_Orient );
        RotatePoint( &quadrant2, m_Orient );
        RotatePoint( &quadrant3, m_Orient );
        RotatePoint( &quadrant4, m_Orient );

        x  = std::min( quadrant1.x, std::min( quadrant2.x, std::min( quadrant3.x, quadrant4.x) ) );
        y  = std::min( quadrant1.y, std::min( quadrant2.y, std::min( quadrant3.y, quadrant4.y) ) );
        dx = std::max( quadrant1.x, std::max( quadrant2.x, std::max( quadrant3.x, quadrant4.x) ) );
        dy = std::max( quadrant1.y, std::max( quadrant2.y, std::max( quadrant3.y, quadrant4.y) ) );

        area.SetOrigin( ShapePos().x + x, ShapePos().y + y );
        area.SetSize( dx-x, dy-y );
        break;

    case PAD_SHAPE_CUSTOM:
        {
        SHAPE_POLY_SET polySet( m_customShapeAsPolygon );
        // Move shape to actual position
        CustomShapeAsPolygonToBoardPosition( &polySet, GetPosition(), GetOrientation() );
        quadrant1 = m_Pos;
        quadrant2 = m_Pos;

        for( int cnt = 0; cnt < polySet.OutlineCount(); ++cnt )
        {
            const SHAPE_LINE_CHAIN& poly = polySet.COutline( cnt );

            for( int ii = 0; ii < poly.PointCount(); ++ii )
            {
                quadrant1.x = std::min( quadrant1.x, poly.CPoint( ii ).x );
                quadrant1.y = std::min( quadrant1.y, poly.CPoint( ii ).y );
                quadrant2.x = std::max( quadrant2.x, poly.CPoint( ii ).x );
                quadrant2.y = std::max( quadrant2.y, poly.CPoint( ii ).y );
            }
        }

        area.SetOrigin( quadrant1 );
        area.SetEnd( quadrant2 );
        }
        break;

    default:
        break;
    }

    return area;
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
}


void D_PAD::SetOrientation( double aAngle )
{
    NORMALIZE_ANGLE_POS( aAngle );
    m_Orient = aAngle;
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

    // m_boundingRadius = -1;  the shape has not been changed
}


// Flip the basic shapes, in custom pads
void D_PAD::FlipPrimitives()
{
    // Flip custom shapes
    for( unsigned ii = 0; ii < m_basicShapes.size(); ++ii )
    {
        PAD_CS_PRIMITIVE& primitive = m_basicShapes[ii];

        MIRROR( primitive.m_Start.y, 0 );
        MIRROR( primitive.m_End.y, 0 );
        primitive.m_ArcAngle = -primitive.m_ArcAngle;

        switch( primitive.m_Shape )
        {
        case S_POLYGON:         // polygon
            for( unsigned jj = 0; jj < primitive.m_Poly.size(); jj++ )
                MIRROR( primitive.m_Poly[jj].y, 0 );
            break;

        default:
            break;
        }
    }

    // Flip local coordinates in merged Polygon
    for( int cnt = 0; cnt < m_customShapeAsPolygon.OutlineCount(); ++cnt )
    {
        SHAPE_LINE_CHAIN& poly = m_customShapeAsPolygon.Outline( cnt );

        for( int ii = 0; ii < poly.PointCount(); ++ii )
            MIRROR( poly.Point( ii ).y, 0 );
    }
}


void D_PAD::MirrorXPrimitives( int aX )
{
    // Mirror custom shapes
    for( unsigned ii = 0; ii < m_basicShapes.size(); ++ii )
    {
        PAD_CS_PRIMITIVE& primitive = m_basicShapes[ii];

        MIRROR( primitive.m_Start.x, aX );
        MIRROR( primitive.m_End.x, aX );
        primitive.m_ArcAngle = -primitive.m_ArcAngle;

        switch( primitive.m_Shape )
        {
        case S_POLYGON:         // polygon
            for( unsigned jj = 0; jj < primitive.m_Poly.size(); jj++ )
                MIRROR( primitive.m_Poly[jj].x, 0 );
            break;

        default:
            break;
        }
    }

    // Mirror the local coordinates in merged Polygon
    for( int cnt = 0; cnt < m_customShapeAsPolygon.OutlineCount(); ++cnt )
    {
        SHAPE_LINE_CHAIN& poly = m_customShapeAsPolygon.Outline( cnt );

        for( int ii = 0; ii < poly.PointCount(); ++ii )
            MIRROR( poly.Point( ii ).x, 0 );
    }
}


void D_PAD::AppendConfigs( PARAM_CFG_ARRAY* aResult )
{
    // Parameters stored in config are only significant parameters
    // for a template.
    // So not all parameters are stored, just few.
    aResult->push_back( new PARAM_CFG_INT_WITH_SCALE( wxT( "PadDrill" ),
                            &m_Drill.x,
                            Millimeter2iu( 0.6 ),
                            Millimeter2iu( 0.1 ), Millimeter2iu( 10.0 ),
                            NULL, MM_PER_IU ) );

    aResult->push_back( new PARAM_CFG_INT_WITH_SCALE( wxT( "PadDrillOvalY" ),
                            &m_Drill.y,
                            Millimeter2iu( 0.6 ),
                            Millimeter2iu( 0.1 ), Millimeter2iu( 10.0 ),
                            NULL, MM_PER_IU ) );

    aResult->push_back( new PARAM_CFG_INT_WITH_SCALE( wxT( "PadSizeH" ),
                            &m_Size.x,
                            Millimeter2iu( 1.4 ),
                            Millimeter2iu( 0.1 ), Millimeter2iu( 20.0 ),
                            NULL, MM_PER_IU ) );

    aResult->push_back( new PARAM_CFG_INT_WITH_SCALE( wxT( "PadSizeV" ),
                            &m_Size.y,
                            Millimeter2iu( 1.4 ),
                            Millimeter2iu( 0.1 ), Millimeter2iu( 20.0 ),
                            NULL, MM_PER_IU ) );
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


bool D_PAD::IncrementPadName( bool aSkipUnconnectable, bool aFillSequenceGaps )
{
    bool skip = aSkipUnconnectable && ( GetAttribute() == PAD_ATTRIB_HOLE_NOT_PLATED );

    if( !skip )
        SetName( GetParent()->GetNextPadName( aFillSequenceGaps ) );

    return !skip;
}


int D_PAD::GetClearance( BOARD_CONNECTED_ITEM* aItem ) const
{
    // A pad can have specific clearance parameters that
    // overrides its NETCLASS clearance value
    int clearance = m_LocalClearance;

    if( clearance == 0 )
    {
        // If local clearance is 0, use the parent footprint clearance value
        if( GetParent() && GetParent()->GetLocalClearance() )
            clearance = GetParent()->GetLocalClearance();
    }

    if( clearance == 0 )   // If the parent footprint clearance value = 0, use NETCLASS value
        return BOARD_CONNECTED_ITEM::GetClearance( aItem );

    // We have a specific clearance.
    // if aItem, return the biggest clearance
    if( aItem )
    {
        int hisClearance = aItem->GetClearance();
        return std::max( hisClearance, clearance );
    }

    // Return the specific clearance.
    return clearance;
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
            {
                margin = brd->GetDesignSettings().m_SolderMaskMargin;
            }
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


ZoneConnection D_PAD::GetZoneConnection() const
{
    MODULE* module = GetParent();

    if( m_ZoneConnection == PAD_ZONE_CONN_INHERITED && module )
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


void D_PAD::GetMsgPanelInfo( EDA_UNITS_T aUnits, std::vector< MSG_PANEL_ITEM>& aList )
{
    MODULE*     module;
    wxString    msg;
    BOARD*      board;

    module = (MODULE*) m_Parent;

    if( module )
    {
        aList.push_back( MSG_PANEL_ITEM( _( "Footprint" ), module->GetReference(), DARKCYAN ) );
        aList.push_back( MSG_PANEL_ITEM( _( "Pad" ), m_name, BROWN ) );
    }

    aList.push_back( MSG_PANEL_ITEM( _( "Net" ), UnescapeString( GetNetname() ), DARKCYAN ) );

    board = GetBoard();

    aList.push_back( MSG_PANEL_ITEM( _( "Layer" ),
                     LayerMaskDescribe( board, m_layerMask ), DARKGREEN ) );

    aList.push_back( MSG_PANEL_ITEM( ShowPadShape(), ShowPadAttr(), DARKGREEN ) );

    msg = MessageTextFromValue( aUnits, m_Size.x, true );
    aList.push_back( MSG_PANEL_ITEM( _( "Width" ), msg, RED ) );

    msg = MessageTextFromValue( aUnits, m_Size.y, true );
    aList.push_back( MSG_PANEL_ITEM( _( "Height" ), msg, RED ) );

    msg = MessageTextFromValue( aUnits, m_Drill.x, true );

    if( GetDrillShape() == PAD_DRILL_SHAPE_CIRCLE )
    {
        aList.push_back( MSG_PANEL_ITEM( _( "Drill" ), msg, RED ) );
    }
    else
    {
        msg = MessageTextFromValue( aUnits, m_Drill.x, true )
               + wxT( "/" )
               + MessageTextFromValue( aUnits, m_Drill.y, true );
        aList.push_back( MSG_PANEL_ITEM( _( "Drill X / Y" ), msg, RED ) );
    }

    double module_orient_degrees = module ? module->GetOrientationDegrees() : 0;

    if( module_orient_degrees != 0.0 )
        msg.Printf( wxT( "%3.1f(+%3.1f)" ),
                    GetOrientationDegrees() - module_orient_degrees,
                    module_orient_degrees );
    else
        msg.Printf( wxT( "%3.1f" ), GetOrientationDegrees() );

    aList.push_back( MSG_PANEL_ITEM( _( "Angle" ), msg, LIGHTBLUE ) );

    msg = MessageTextFromValue( aUnits, m_Pos.x )
           + wxT( ", " )
           + MessageTextFromValue( aUnits, m_Pos.y );
    aList.push_back( MSG_PANEL_ITEM( _( "Position" ), msg, LIGHTBLUE ) );

    if( GetPadToDieLength() )
    {
        msg = MessageTextFromValue( aUnits, GetPadToDieLength(), true );
        aList.push_back( MSG_PANEL_ITEM( _( "Length in package" ), msg, CYAN ) );
    }
}


void D_PAD::GetOblongDrillGeometry( wxPoint& aStartPoint,
                                    wxPoint& aEndPoint, int& aWidth ) const
{
    // calculates the start point, end point and width
    // of an equivalent segment which have the same position and width as the hole
    int delta_cx, delta_cy;

    wxSize halfsize = GetDrillSize();
    halfsize.x /= 2;
    halfsize.y /= 2;

    if( m_Drill.x > m_Drill.y )  // horizontal
    {
        delta_cx = halfsize.x - halfsize.y;
        delta_cy = 0;
        aWidth   = m_Drill.y;
    }
    else                         // vertical
    {
        delta_cx = 0;
        delta_cy = halfsize.y - halfsize.x;
        aWidth   = m_Drill.x;
    }

    RotatePoint( &delta_cx, &delta_cy, m_Orient );

    aStartPoint.x = delta_cx;
    aStartPoint.y = delta_cy;

    aEndPoint.x = - delta_cx;
    aEndPoint.y = - delta_cy;
}


bool D_PAD::HitTest( const wxPoint& aPosition, int aAccuracy ) const
{
    int dx, dy;

    wxPoint shape_pos = ShapePos();

    wxPoint delta = aPosition - shape_pos;

    // first test: a test point must be inside a minimum sized bounding circle.
    int radius = GetBoundingRadius();

    if( ( abs( delta.x ) > radius ) || ( abs( delta.y ) > radius ) )
        return false;

    dx = m_Size.x >> 1; // dx also is the radius for rounded pads
    dy = m_Size.y >> 1;

    switch( GetShape() )
    {
    case PAD_SHAPE_CIRCLE:
        if( KiROUND( EuclideanNorm( delta ) ) <= dx )
            return true;

        break;

    case PAD_SHAPE_TRAPEZOID:
    {
        wxPoint poly[4];
        BuildPadPolygon( poly, wxSize(0,0), 0 );
        RotatePoint( &delta, -m_Orient );

        return TestPointInsidePolygon( poly, 4, delta );
    }

    case PAD_SHAPE_OVAL:
    {
        RotatePoint( &delta, -m_Orient );
        // An oval pad has the same shape as a segment with rounded ends
        // After rotation, the test point is relative to an horizontal pad
        int dist;
        wxPoint offset;
        if( dy > dx )   // shape is a vertical oval
        {
            offset.y = dy - dx;
            dist = dx;
        }
        else    //if( dy <= dx ) shape is an horizontal oval
        {
            offset.x = dy - dx;
            dist = dy;
        }
        return TestSegmentHit( delta, - offset, offset, dist );
    }
        break;

    case PAD_SHAPE_RECT:
        RotatePoint( &delta, -m_Orient );

        if( (abs( delta.x ) <= dx ) && (abs( delta.y ) <= dy) )
            return true;

        break;

    case PAD_SHAPE_CHAMFERED_RECT:
    case PAD_SHAPE_ROUNDRECT:
        {
        // Check for hit in polygon
        SHAPE_POLY_SET outline;
        bool doChamfer = GetShape() == PAD_SHAPE_CHAMFERED_RECT;
        auto board = GetBoard();
        int maxError = ARC_HIGH_DEF;

        if( board )
            maxError = board->GetDesignSettings().m_MaxError;

        TransformRoundChamferedRectToPolygon( outline, wxPoint(0,0), GetSize(), m_Orient,
                                              GetRoundRectCornerRadius(),
                                              doChamfer ? GetChamferRectRatio() : 0.0,
                                              doChamfer ? GetChamferPositions() : 0,
                                              maxError );

        const SHAPE_LINE_CHAIN &poly = outline.COutline( 0 );
        return TestPointInsidePolygon( (const wxPoint*)&poly.CPoint(0), poly.PointCount(), delta );
        }
        break;

    case PAD_SHAPE_CUSTOM:
        // Check for hit in polygon
        RotatePoint( &delta, -m_Orient );

        if( m_customShapeAsPolygon.OutlineCount() )
        {
            const SHAPE_LINE_CHAIN& poly = m_customShapeAsPolygon.COutline( 0 );
            return TestPointInsidePolygon( (const wxPoint*)&poly.CPoint(0), poly.PointCount(), delta );
        }
        break;
    }

    return false;
}


bool D_PAD::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    EDA_RECT arect = aRect;
    arect.Normalize();
    arect.Inflate( aAccuracy );

    wxPoint shapePos = ShapePos();

    EDA_RECT shapeRect;

    int r;

    EDA_RECT bb = GetBoundingBox();

    wxPoint endCenter;
    int radius;

    if( !arect.Intersects( bb ) )
        return false;

    // This covers total containment for all test cases
    if( arect.Contains( bb ) )
        return true;

    switch( GetShape() )
    {
    case PAD_SHAPE_CIRCLE:
        return arect.IntersectsCircle( GetPosition(), GetBoundingRadius() );

    case PAD_SHAPE_RECT:
    case PAD_SHAPE_CHAMFERED_RECT:    // TODO use a finer shape analysis
        shapeRect.SetOrigin( shapePos );
        shapeRect.Inflate( m_Size.x / 2, m_Size.y / 2 );
        return arect.Intersects( shapeRect, m_Orient );

    case PAD_SHAPE_OVAL:
        // Circlular test if dimensions are equal
        if( m_Size.x == m_Size.y )
            return arect.IntersectsCircle( shapePos, GetBoundingRadius() );

        shapeRect.SetOrigin( shapePos );

        // Horizontal dimension is greater
        if( m_Size.x > m_Size.y )
        {
            radius = m_Size.y / 2;

            shapeRect.Inflate( m_Size.x / 2 - radius, radius );

            endCenter = wxPoint( m_Size.x / 2 - radius, 0 );
            RotatePoint( &endCenter, m_Orient );

            // Test circular ends
            if( arect.IntersectsCircle( shapePos + endCenter, radius ) ||
                arect.IntersectsCircle( shapePos - endCenter, radius ) )
            {
                return true;
            }
        }
        else
        {
            radius = m_Size.x / 2;

            shapeRect.Inflate( radius, m_Size.y / 2 - radius );

            endCenter = wxPoint( 0, m_Size.y / 2 - radius );
            RotatePoint( &endCenter, m_Orient );

            // Test circular ends
            if( arect.IntersectsCircle( shapePos + endCenter, radius ) ||
                arect.IntersectsCircle( shapePos - endCenter, radius ) )
            {
                return true;
            }
        }

        // Test rectangular portion between rounded ends
        if( arect.Intersects( shapeRect, m_Orient ) )
        {
            return true;
        }

        break;

    case PAD_SHAPE_TRAPEZOID:
        /* Trapezoid intersection tests:
         * A) Any points of rect inside trapezoid
         * B) Any points of trapezoid inside rect
         * C) Any sides of trapezoid cross rect
         */
        {

        wxPoint poly[4];
        BuildPadPolygon( poly, wxSize( 0, 0 ), 0 );

        wxPoint corners[4];

        corners[0] = wxPoint( arect.GetLeft(),  arect.GetTop() );
        corners[1] = wxPoint( arect.GetRight(), arect.GetTop() );
        corners[2] = wxPoint( arect.GetRight(), arect.GetBottom() );
        corners[3] = wxPoint( arect.GetLeft(),  arect.GetBottom() );

        for( int i=0; i<4; i++ )
        {
            RotatePoint( &poly[i], m_Orient );
            poly[i] += shapePos;
        }

        for( int ii=0; ii<4; ii++ )
        {
            if( TestPointInsidePolygon( poly, 4, corners[ii] ) )
            {
                return true;
            }

            if( arect.Contains( poly[ii] ) )
            {
                return true;
            }

            if( arect.Intersects( poly[ii], poly[(ii+1) % 4] ) )
            {
                return true;
            }
        }

        return false;
        }

    case PAD_SHAPE_ROUNDRECT:
        /* RoundRect intersection can be broken up into simple tests:
         * a) Test intersection of horizontal rect
         * b) Test intersection of vertical rect
         * c) Test intersection of each corner
         */
        r = GetRoundRectCornerRadius();

        /* Test A - intersection of horizontal rect */
        shapeRect.SetSize( 0, 0 );
        shapeRect.SetOrigin( shapePos );
        shapeRect.Inflate( m_Size.x / 2, m_Size.y / 2 - r );

        // Short-circuit test for zero width or height
        if( shapeRect.GetWidth() > 0 && shapeRect.GetHeight() > 0 &&
            arect.Intersects( shapeRect, m_Orient ) )
        {
            return true;
        }

        /* Test B - intersection of vertical rect */
        shapeRect.SetSize( 0, 0 );
        shapeRect.SetOrigin( shapePos );
        shapeRect.Inflate( m_Size.x / 2 - r, m_Size.y / 2 );

        // Short-circuit test for zero width or height
        if( shapeRect.GetWidth() > 0 && shapeRect.GetHeight() > 0 &&
            arect.Intersects( shapeRect, m_Orient ) )
        {
            return true;
        }

        /* Test C - intersection of each corner */

        endCenter = wxPoint( m_Size.x / 2 - r, m_Size.y / 2 - r );
        RotatePoint( &endCenter, m_Orient );

        if( arect.IntersectsCircle( shapePos + endCenter, r ) ||
            arect.IntersectsCircle( shapePos - endCenter, r ) )
        {
            return true;
        }

        endCenter = wxPoint( m_Size.x / 2 - r, -m_Size.y / 2 + r );
        RotatePoint( &endCenter, m_Orient );

        if( arect.IntersectsCircle( shapePos + endCenter, r ) ||
            arect.IntersectsCircle( shapePos - endCenter, r ) )
        {
            return true;
        }

        break;

    default:
        break;
    }

    return false;
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
}


wxString D_PAD::ShowPadShape() const
{
    switch( GetShape() )
    {
    case PAD_SHAPE_CIRCLE:
        return _( "Circle" );

    case PAD_SHAPE_OVAL:
        return _( "Oval" );

    case PAD_SHAPE_RECT:
        return _( "Rect" );

    case PAD_SHAPE_TRAPEZOID:
        return _( "Trap" );

    case PAD_SHAPE_ROUNDRECT:
        return _( "Roundrect" );

    case PAD_SHAPE_CHAMFERED_RECT:
        return _( "Chamferedrect" );

    case PAD_SHAPE_CUSTOM:
        return _( "CustomShape" );

    default:
        return wxT( "???" );
    }
}


wxString D_PAD::ShowPadAttr() const
{
    switch( GetAttribute() )
    {
    case PAD_ATTRIB_STANDARD:
        return _( "Std" );

    case PAD_ATTRIB_SMD:
        return _( "SMD" );

    case PAD_ATTRIB_CONN:
        return _( "Conn" );

    case PAD_ATTRIB_HOLE_NOT_PLATED:
        return _( "Not Plated" );

    default:
        return wxT( "???" );
    }
}


wxString D_PAD::GetSelectMenuText( EDA_UNITS_T aUnits ) const
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
        int divisor = std::max( m_Size.x, m_Size.y );

        // Pad sizes can be zero briefly when someone is typing a number like "0.5"
        // in the pad properties dialog
        if( divisor == 0 )
            return HIDE;

        return ( Millimeter2iu( 10 ) / divisor );
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


wxString LayerMaskDescribe( const BOARD *aBoard, LSET aMask )
{
    // Try to be smart and useful.  Check all copper first.
    if( aMask[F_Cu] && aMask[B_Cu] )
        return _( "All copper layers" );

    // Check for copper.
    auto layer = aBoard->GetEnabledLayers().AllCuMask() & aMask;

    for( int i = 0; i < 2; i++ )
    {
        for( int bit = PCBNEW_LAYER_ID_START; bit < PCB_LAYER_ID_COUNT; ++bit )
        {
            if( layer[ bit ] )
            {
                wxString layerInfo = aBoard->GetLayerName( static_cast<PCB_LAYER_ID>( bit ) );

                if( aMask.count() > 1 )
                    layerInfo << _( " and others" );

                return layerInfo;
            }
        }

        // No copper; check for technicals.
        layer = aBoard->GetEnabledLayers().AllTechMask() & aMask;
    }

    // No copper, no technicals: no layer
    return _( "no layers" );
}


void D_PAD::ImportSettingsFrom( const D_PAD& aMasterPad )
{
    SetShape( aMasterPad.GetShape() );
    SetLayerSet( aMasterPad.GetLayerSet() );
    SetAttribute( aMasterPad.GetAttribute() );

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

    SetZoneConnection( aMasterPad.GetZoneConnection() );
    SetThermalWidth( aMasterPad.GetThermalWidth() );
    SetThermalGap( aMasterPad.GetThermalGap() );

    // Add or remove custom pad shapes:
    SetPrimitives( aMasterPad.GetPrimitives() );
    SetAnchorPadShape( aMasterPad.GetAnchorPadShape() );
    MergePrimitivesAsPolygon();
}

void D_PAD::SwapData( BOARD_ITEM* aImage )
{
    assert( aImage->Type() == PCB_PAD_T );

    std::swap( *((MODULE*) this), *((MODULE*) aImage) );
}
