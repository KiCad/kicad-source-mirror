/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
  * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <PolyLine.h>
#include <common.h>
#include <confirm.h>
#include <kicad_string.h>
#include <trigo.h>
#include <richio.h>
#include <wxstruct.h>
#include <macros.h>
#include <msgpanel.h>
#include <base_units.h>

#include <pcbnew.h>
#include <pcbnew_id.h>                      // ID_TRACK_BUTT

#include <class_board.h>
#include <class_module.h>
#include <polygon_test_point_inside.h>
#include <convert_from_iu.h>


int D_PAD::m_PadSketchModePenSize = 0;      // Pen size used to draw pads in sketch mode


D_PAD::D_PAD( MODULE* parent ) :
    BOARD_CONNECTED_ITEM( parent, PCB_PAD_T )
{
    m_NumPadName          = 0;
    m_Size.x = m_Size.y   = DMils2iu( 600 ); // Default pad size 60 mils.
    m_Drill.x = m_Drill.y = DMils2iu( 300 ); // Default drill size 30 mils.
    m_Orient              = 0;               // Pad rotation in 1/10 degrees.
    m_LengthPadToDie      = 0;

    if( m_Parent  &&  m_Parent->Type() == PCB_MODULE_T )
    {
        m_Pos = GetParent()->GetPosition();
    }

    SetShape( PAD_CIRCLE );                 // Default pad shape is PAD_CIRCLE.
    SetDrillShape( PAD_DRILL_CIRCLE );      // Default pad drill shape is a circle.
    m_Attribute           = PAD_STANDARD;   // Default pad type is NORMAL (thru hole)
    m_LocalClearance      = 0;
    m_LocalSolderMaskMargin  = 0;
    m_LocalSolderPasteMargin = 0;
    m_LocalSolderPasteMarginRatio = 0.0;
    m_ZoneConnection      = UNDEFINED_CONNECTION; // Use parent setting by default
    m_ThermalWidth        = 0;                // Use parent setting by default
    m_ThermalGap          = 0;                // Use parent setting by default

    // Set layers mask to default for a standard thru hole pad.
    m_layerMask           = StandardMask();

    SetSubRatsnest( 0 );                       // used in ratsnest calculations

    m_boundingRadius      = -1;
}


LSET D_PAD::StandardMask()
{
    static LSET saved = LSET::AllCuMask() | LSET( 3, F_SilkS, B_Mask, F_Mask );
    return saved;
}


LSET D_PAD::ConnMask()
{
    // was: #define PAD_CONN_DEFAULT_LAYERS LAYER_FRONT | SOLDERPASTE_LAYER_FRONT | SOLDERMASK_LAYER_FRONT
    static LSET saved( 3, F_Cu, F_Paste, F_Mask );
    return saved;
}


LSET D_PAD::SMDMask()
{
    // was: #define PAD_SMD_DEFAULT_LAYERS LAYER_FRONT | SOLDERMASK_LAYER_FRONT
    static LSET saved( 2, F_Cu, F_Mask );
    return saved;
}


LSET D_PAD::UnplatedHoleMask()
{
    // was #define PAD_HOLE_NOT_PLATED_DEFAULT_LAYERS ALL_CU_LAYERS |
    // SILKSCREEN_LAYER_FRONT | SOLDERMASK_LAYER_BACK | SOLDERMASK_LAYER_FRONT
    static LSET saved = LSET::AllCuMask() | LSET( 3, F_SilkS, B_Mask, F_Mask );
    return saved;
}


int D_PAD::boundingRadius() const
{
    int x, y;
    int radius;

    switch( GetShape() )
    {
    case PAD_CIRCLE:
        radius = m_Size.x / 2;
        break;

    case PAD_OVAL:
        radius = std::max( m_Size.x, m_Size.y ) / 2;
        break;

    case PAD_RECT:
        radius = 1 + KiROUND( EuclideanNorm( m_Size ) / 2 );
        break;

    case PAD_TRAPEZOID:
        x = m_Size.x + std::abs( m_DeltaSize.y );   // Remember: m_DeltaSize.y is the m_Size.x change
        y = m_Size.y + std::abs( m_DeltaSize.x );   // Remember: m_DeltaSize.x is the m_Size.y change
        radius = 1 + KiROUND( hypot( x, y ) / 2 );
        break;

    default:
        radius = 0;
    }

    return radius;
}


const EDA_RECT D_PAD::GetBoundingBox() const
{
    EDA_RECT area;
    wxPoint quadrant1, quadrant2, quadrant3, quadrant4;
    int x, y, dx, dy;

    switch( GetShape() )
    {
    case PAD_CIRCLE:
        area.SetOrigin( m_Pos );
        area.Inflate( m_Size.x / 2 );
        break;

    case PAD_OVAL:
        //Use the maximal two most distant points and track their rotation
        // (utilise symmetry to avoid four points)
        quadrant1.x =  m_Size.x/2;
        quadrant1.y =  0;
        quadrant2.x =  0;
        quadrant2.y =  m_Size.y/2;

        RotatePoint( &quadrant1, m_Orient );
        RotatePoint( &quadrant2, m_Orient );
        dx = std::max( std::abs( quadrant1.x ) , std::abs( quadrant2.x )  );
        dy = std::max( std::abs( quadrant1.y ) , std::abs( quadrant2.y )  );
        area.SetOrigin( m_Pos.x-dx, m_Pos.y-dy );
        area.SetSize( 2*dx, 2*dy );
        break;
        break;

    case PAD_RECT:
        //Use two corners and track their rotation
        // (utilise symmetry to avoid four points)
        quadrant1.x =  m_Size.x/2;
        quadrant1.y =  m_Size.y/2;
        quadrant2.x = -m_Size.x/2;
        quadrant2.y =  m_Size.y/2;

        RotatePoint( &quadrant1, m_Orient );
        RotatePoint( &quadrant2, m_Orient );
        dx = std::max( std::abs( quadrant1.x ) , std::abs( quadrant2.x )  );
        dy = std::max( std::abs( quadrant1.y ) , std::abs( quadrant2.y )  );
        area.SetOrigin( m_Pos.x-dx, m_Pos.y-dy );
        area.SetSize( 2*dx, 2*dy );
        break;

    case PAD_TRAPEZOID:
        //Use the four corners and track their rotation
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
        area.SetOrigin( m_Pos.x+x, m_Pos.y+y );
        area.SetSize( dx-x, dy-y );
        break;

    default:
        break;
    }

    return area;
}


void D_PAD::SetAttribute( PAD_ATTR_T aAttribute )
{
    m_Attribute = aAttribute;

    if( aAttribute == PAD_SMD )
        m_Drill = wxSize( 0, 0 );
}


void D_PAD::SetOrientation( double aAngle )
{
    NORMALIZE_ANGLE_POS( aAngle );
    m_Orient = aAngle;
}


void D_PAD::Flip( const wxPoint& aCentre )
{
    int y = GetPosition().y - aCentre.y;

    y = -y;         // invert about x axis.

    y += aCentre.y;

    SetY( y );

    NEGATE( m_Pos0.y );
    NEGATE( m_Offset.y );
    NEGATE( m_DeltaSize.y );

    SetOrientation( -GetOrientation() );

    // flip pads layers
    SetLayerSet( FlipLayerMask( m_layerMask ) );

    // m_boundingRadius = -1;  the shape has not been changed
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
const wxPoint D_PAD::ShapePos() const
{
    if( m_Offset.x == 0 && m_Offset.y == 0 )
        return m_Pos;

    wxPoint shape_pos;
    int     dX, dY;

    dX = m_Offset.x;
    dY = m_Offset.y;

    RotatePoint( &dX, &dY, m_Orient );

    shape_pos.x = m_Pos.x + dX;
    shape_pos.y = m_Pos.y + dY;

    return shape_pos;
}


const wxString D_PAD::GetPadName() const
{
#if 0   // m_Padname is not ASCII and not UTF8, it is LATIN1 basically, whatever
        // 8 bit font is supported in KiCad plotting and drawing.

    // Return pad name as wxString, assume it starts as a non-terminated
    // utf8 character sequence

    char    temp[sizeof(m_Padname)+1];      // a place to terminate with '\0'

    strncpy( temp, m_Padname, sizeof(m_Padname) );

    temp[sizeof(m_Padname)] = 0;

    return FROM_UTF8( temp );
#else

    wxString name;

    StringPadName( name );
    return name;
#endif
}


void D_PAD::StringPadName( wxString& text ) const
{
#if 0   // m_Padname is not ASCII and not UTF8, it is LATIN1 basically, whatever
        // 8 bit font is supported in KiCad plotting and drawing.

    // Return pad name as wxString, assume it starts as a non-terminated
    // utf8 character sequence

    char    temp[sizeof(m_Padname)+1];      // a place to terminate with '\0'

    strncpy( temp, m_Padname, sizeof(m_Padname) );

    temp[sizeof(m_Padname)] = 0;

    text = FROM_UTF8( temp );

#else

    text.Empty();

    for( int ii = 0;  ii < PADNAMEZ && m_Padname[ii];  ii++ )
    {
        // m_Padname is 8 bit KiCad font junk, do not sign extend
        text.Append( (unsigned char) m_Padname[ii] );
    }
#endif
}


// Change pad name
void D_PAD::SetPadName( const wxString& name )
{
    int ii, len;

    len = name.Length();

    if( len > PADNAMEZ )
        len = PADNAMEZ;

    // m_Padname[] is not UTF8, it is an 8 bit character that matches the KiCad font,
    // so only copy the lower 8 bits of each character.

    for( ii = 0; ii < len; ii++ )
        m_Padname[ii] = (char) name.GetChar( ii );

    for( ii = len; ii < PADNAMEZ; ii++ )
        m_Padname[ii] = '\0';
}


void D_PAD::Copy( D_PAD* source )
{
    if( source == NULL )
        return;

    m_Pos = source->m_Pos;
    m_layerMask = source->m_layerMask;

    m_NumPadName = source->m_NumPadName;
    m_netinfo = source->m_netinfo;
    m_Drill = source->m_Drill;
    m_drillShape = source->m_drillShape;
    m_Offset     = source->m_Offset;
    m_Size = source->m_Size;
    m_DeltaSize = source->m_DeltaSize;
    m_Pos0     = source->m_Pos0;
    m_boundingRadius    = source->m_boundingRadius;
    m_padShape = source->m_padShape;
    m_Attribute = source->m_Attribute;
    m_Orient   = source->m_Orient;
    m_LengthPadToDie = source->m_LengthPadToDie;
    m_LocalClearance = source->m_LocalClearance;
    m_LocalSolderMaskMargin  = source->m_LocalSolderMaskMargin;
    m_LocalSolderPasteMargin = source->m_LocalSolderPasteMargin;
    m_LocalSolderPasteMarginRatio = source->m_LocalSolderPasteMarginRatio;
    m_ZoneConnection = source->m_ZoneConnection;
    m_ThermalWidth = source->m_ThermalWidth;
    m_ThermalGap = source->m_ThermalGap;

    SetSubRatsnest( 0 );
    SetSubNet( 0 );
}


void D_PAD::CopyNetlistSettings( D_PAD* aPad )
{
    // Don't do anything foolish like trying to copy to yourself.
    wxCHECK_RET( aPad != NULL && aPad != this, wxT( "Cannot copy to NULL or yourself." ) );

    aPad->SetNetCode( GetNetCode() );

    aPad->SetLocalClearance( m_LocalClearance );
    aPad->SetLocalSolderMaskMargin( m_LocalSolderMaskMargin );
    aPad->SetLocalSolderPasteMargin( m_LocalSolderPasteMargin );
    aPad->SetLocalSolderPasteMarginRatio( m_LocalSolderPasteMarginRatio );
    aPad->SetZoneConnection( m_ZoneConnection );
    aPad->SetThermalWidth( m_ThermalWidth );
    aPad->SetThermalGap( m_ThermalGap );
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
    int     margin = m_LocalSolderPasteMargin;
    double  mratio = m_LocalSolderPasteMarginRatio;
    MODULE* module = GetParent();

    if( module )
    {
        if( margin == 0 )
            margin = module->GetLocalSolderPasteMargin();

        BOARD * brd = GetBoard();

        if( margin == 0 )
            margin = brd->GetDesignSettings().m_SolderPasteMargin;

        if( mratio == 0.0 )
            mratio = module->GetLocalSolderPasteMarginRatio();

        if( mratio == 0.0 )
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
    MODULE* module = (MODULE*) GetParent();

    if( m_ZoneConnection == UNDEFINED_CONNECTION && module )
        return module->GetZoneConnection();
    else
        return m_ZoneConnection;
}


int D_PAD::GetThermalWidth() const
{
    MODULE* module = (MODULE*) GetParent();

    if( m_ThermalWidth == 0 && module )
        return module->GetThermalWidth();
    else
        return m_ThermalWidth;
}


int D_PAD::GetThermalGap() const
{
    MODULE* module = (MODULE*) GetParent();

    if( m_ThermalGap == 0 && module )
        return module->GetThermalGap();
    else
        return m_ThermalGap;
}


void D_PAD::GetMsgPanelInfo( std::vector< MSG_PANEL_ITEM>& aList )
{
    MODULE*     module;
    wxString    Line;
    BOARD*      board;

    module = (MODULE*) m_Parent;

    if( module )
    {
        wxString msg = module->GetReference();
        aList.push_back( MSG_PANEL_ITEM( _( "Module" ), msg, DARKCYAN ) );
        StringPadName( Line );
        aList.push_back( MSG_PANEL_ITEM( _( "Pad" ), Line, BROWN ) );
    }

    aList.push_back( MSG_PANEL_ITEM( _( "Net" ), GetNetname(), DARKCYAN ) );

    /* For test and debug only: display m_physical_connexion and
     * m_logical_connexion */
#if 1   // Used only to debug connectivity calculations
    Line.Printf( wxT( "%d-%d-%d " ), GetSubRatsnest(), GetSubNet(), GetZoneSubNet() );
    aList.push_back( MSG_PANEL_ITEM( wxT( "L-P-Z" ), Line, DARKGREEN ) );
#endif

    board = GetBoard();

    aList.push_back( MSG_PANEL_ITEM( _( "Layer" ),
                     LayerMaskDescribe( board, m_layerMask ), DARKGREEN ) );

    aList.push_back( MSG_PANEL_ITEM( ShowPadShape(), ShowPadAttr(), DARKGREEN ) );

    Line = ::CoordinateToString( m_Size.x );
    aList.push_back( MSG_PANEL_ITEM( _( "H Size" ), Line, RED ) );

    Line = ::CoordinateToString( m_Size.y );
    aList.push_back( MSG_PANEL_ITEM( _( "V Size" ), Line, RED ) );

    Line = ::CoordinateToString( (unsigned) m_Drill.x );

    if( GetDrillShape() == PAD_DRILL_CIRCLE )
    {
        aList.push_back( MSG_PANEL_ITEM( _( "Drill" ), Line, RED ) );
    }
    else
    {
        Line = ::CoordinateToString( (unsigned) m_Drill.x );
        wxString msg;
        msg = ::CoordinateToString( (unsigned) m_Drill.y );
        Line += wxT( "/" ) + msg;
        aList.push_back( MSG_PANEL_ITEM( _( "Drill X / Y" ), Line, RED ) );
    }

    double module_orient = module ? module->GetOrientation() : 0;

    if( module_orient )
        Line.Printf( wxT( "%3.1f(+%3.1f)" ),
                     ( m_Orient - module_orient ) / 10.0,
                     module_orient / 10.0 );
    else
        Line.Printf( wxT( "%3.1f" ), m_Orient / 10.0 );

    aList.push_back( MSG_PANEL_ITEM( _( "Orient" ), Line, LIGHTBLUE ) );

    Line = ::CoordinateToString( m_Pos.x );
    aList.push_back( MSG_PANEL_ITEM( _( "X Pos" ), Line, LIGHTBLUE ) );

    Line = ::CoordinateToString( m_Pos.y );
    aList.push_back( MSG_PANEL_ITEM( _( "Y pos" ), Line, LIGHTBLUE ) );

    if( GetPadToDieLength() )
    {
        Line = ::CoordinateToString( GetPadToDieLength() );
        aList.push_back( MSG_PANEL_ITEM( _( "Length in package" ), Line, CYAN ) );
    }
}


void D_PAD::GetOblongDrillGeometry( wxPoint& aStartPoint,
                                    wxPoint& aEndPoint, int& aWidth ) const
{
    // calculates the start point, end point and width
    // of an equivalent segment which have the same position and width as the hole
    int delta_cx, delta_cy;

    wxSize halfsize = GetDrillSize();;
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

bool D_PAD::HitTest( const wxPoint& aPosition ) const
{
    int     dx, dy;

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
    case PAD_CIRCLE:
        if( KiROUND( EuclideanNorm( delta ) ) <= dx )
            return true;

        break;

    case PAD_TRAPEZOID:
    {
        wxPoint poly[4];
        BuildPadPolygon( poly, wxSize(0,0), 0 );
        RotatePoint( &delta, -m_Orient );
        return TestPointInsidePolygon( poly, 4, delta );
    }

    case PAD_OVAL:
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

    case PAD_RECT:
        RotatePoint( &delta, -m_Orient );

        if( (abs( delta.x ) <= dx ) && (abs( delta.y ) <= dy) )
            return true;

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


wxString D_PAD::ShowPadShape() const
{
    switch( GetShape() )
    {
    case PAD_CIRCLE:
        return _( "Circle" );

    case PAD_OVAL:
        return _( "Oval" );

    case PAD_RECT:
        return _( "Rect" );

    case PAD_TRAPEZOID:
        return _( "Trap" );

    default:
        return wxT( "???" );
    }
}


wxString D_PAD::ShowPadAttr() const
{
    switch( GetAttribute() )
    {
    case PAD_STANDARD:
        return _( "Std" );

    case PAD_SMD:
        return _( "SMD" );

    case PAD_CONN:
        return _( "Conn" );

    case PAD_HOLE_NOT_PLATED:
        return _( "Not Plated" );

    default:
        return wxT( "???" );
    }
}


wxString D_PAD::GetSelectMenuText() const
{
    wxString text;
    wxString padlayers( LayerMaskDescribe( GetBoard(), m_layerMask ) );
    wxString padname( GetPadName() );

    if( padname.IsEmpty() )
    {
    text.Printf( _( "Pad on %s of %s" ),
                 GetChars( padlayers ),
                 GetChars(( (MODULE*) GetParent() )->GetReference() ) );
    }
    else
    {
        text.Printf( _( "Pad %s on %s of %s" ),
                     GetChars(GetPadName() ), GetChars( padlayers ),
                     GetChars(( (MODULE*) GetParent() )->GetReference() ) );
    }

    return text;
}


EDA_ITEM* D_PAD::Clone() const
{
    return new D_PAD( *this );
}


void D_PAD::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount = 0;

    // These types of pads contain a hole
    if( m_Attribute == PAD_STANDARD || m_Attribute == PAD_HOLE_NOT_PLATED )
        aLayers[aCount++] = ITEM_GAL_LAYER( PADS_HOLES_VISIBLE );

    if( IsOnLayer( F_Cu ) && IsOnLayer( B_Cu ) )
    {
        // Multi layer pad
        aLayers[aCount++] = ITEM_GAL_LAYER( PADS_VISIBLE );
        aLayers[aCount++] = NETNAMES_GAL_LAYER( PADS_NETNAMES_VISIBLE );
    }
    else if( IsOnLayer( F_Cu ) )
    {
        aLayers[aCount++] = ITEM_GAL_LAYER( PAD_FR_VISIBLE );
        aLayers[aCount++] = NETNAMES_GAL_LAYER( PAD_FR_NETNAMES_VISIBLE );
    }
    else if( IsOnLayer( B_Cu ) )
    {
        aLayers[aCount++] = ITEM_GAL_LAYER( PAD_BK_VISIBLE );
        aLayers[aCount++] = NETNAMES_GAL_LAYER( PAD_BK_NETNAMES_VISIBLE );
    }

    if( IsOnLayer( F_Mask ) )
        aLayers[aCount++] = F_Mask;

    if( IsOnLayer( B_Mask ) )
        aLayers[aCount++] = B_Mask;

    if( IsOnLayer( F_Paste ) )
        aLayers[aCount++] = F_Paste;

    if( IsOnLayer( B_Paste ) )
        aLayers[aCount++] = B_Paste;

    if( IsOnLayer( B_Adhes ) )
        aLayers[aCount++] = B_Adhes;

    if( IsOnLayer( F_Adhes ) )
        aLayers[aCount++] = F_Adhes;

#ifdef __WXDEBUG__
    if( aCount == 0 )    // Should not occur
    {
        wxLogWarning( wxT("D_PAD::ViewGetLayers():PAD has no layer") );
    }
#endif
}


unsigned int D_PAD::ViewGetLOD( int aLayer ) const
{
    // Netnames and soldermasks will be shown only if zoom is appropriate
    if( IsNetnameLayer( aLayer ) )
    {
        return ( 100000000 / std::max( m_Size.x, m_Size.y ) );
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
