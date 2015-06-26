/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jean-pierre.charras@gipsa-lab.inpg.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file class_drawsegment.cpp
 * @brief Class and functions to handle a graphic segments.
 */

#include <fctsys.h>
#include <macros.h>
#include <wxstruct.h>
#include <gr_basic.h>
#include <bezier_curves.h>
#include <class_drawpanel.h>
#include <class_pcb_screen.h>
#include <colors_selection.h>
#include <trigo.h>
#include <msgpanel.h>

#include <pcbnew.h>

#include <class_board.h>
#include <class_module.h>
#include <class_drawsegment.h>
#include <base_units.h>


DRAWSEGMENT::DRAWSEGMENT( BOARD_ITEM* aParent, KICAD_T idtype ) :
    BOARD_ITEM( aParent, idtype )
{
    m_Type  = 0;
    m_Angle = 0;
    m_Flags = 0;
    m_Shape = S_SEGMENT;
    m_Width = Millimeter2iu( 0.15 );    // Gives a decent width
}


DRAWSEGMENT::~DRAWSEGMENT()
{
}


const DRAWSEGMENT& DRAWSEGMENT::operator = ( const DRAWSEGMENT& rhs )
{
    // skip the linked list stuff, and parent

    m_Type         = rhs.m_Type;
    m_Layer        = rhs.m_Layer;
    m_Width        = rhs.m_Width;
    m_Start        = rhs.m_Start;
    m_End          = rhs.m_End;
    m_Shape        = rhs.m_Shape;
    m_Angle        = rhs.m_Angle;
    m_TimeStamp    = rhs.m_TimeStamp;
    m_BezierC1     = rhs.m_BezierC1;
    m_BezierC2     = rhs.m_BezierC1;
    m_BezierPoints = rhs.m_BezierPoints;

    return *this;
}


void DRAWSEGMENT::Copy( DRAWSEGMENT* source )
{
    if( source == NULL )    // who would do this?
        return;

    *this = *source;    // operator = ()
}

void DRAWSEGMENT::Rotate( const wxPoint& aRotCentre, double aAngle )
{
    switch( m_Shape )
    {
    case S_ARC:
    case S_SEGMENT:
    case S_CIRCLE:
        // these can all be done by just rotating the start and end points
        RotatePoint( &m_Start, aRotCentre, aAngle);
        RotatePoint( &m_End, aRotCentre, aAngle);
        break;

    case S_POLYGON:
        for( unsigned ii = 0; ii < m_PolyPoints.size(); ii++ )
        {
            RotatePoint( &m_PolyPoints[ii], aRotCentre, aAngle);
        }
        break;

    case S_CURVE:
        RotatePoint( &m_Start, aRotCentre, aAngle);
        RotatePoint( &m_End, aRotCentre, aAngle);

        for( unsigned int ii = 0; ii < m_BezierPoints.size(); ii++ )
        {
            RotatePoint( &m_BezierPoints[ii], aRotCentre, aAngle);
        }
        break;

    case S_RECT:
    default:
        // un-handled edge transform
        wxASSERT_MSG( false, wxT( "DRAWSEGMENT::Rotate not implemented for "
                + ShowShape( m_Shape ) ) );
        break;
    }
};

void DRAWSEGMENT::Flip( const wxPoint& aCentre )
{
    m_Start.y  = aCentre.y - (m_Start.y - aCentre.y);
    m_End.y  = aCentre.y - (m_End.y - aCentre.y);

    if( m_Shape == S_ARC )
        m_Angle = -m_Angle;

    SetLayer( FlipLayer( GetLayer() ) );
}

const wxPoint DRAWSEGMENT::GetCenter() const
{
    wxPoint c;

    switch( m_Shape )
    {
    case S_ARC:
    case S_CIRCLE:
        c = m_Start;
        break;

    case S_SEGMENT:
        // Midpoint of the line
        c = ( GetStart() + GetEnd() ) / 2;
        break;

    case S_POLYGON:
    case S_RECT:
    case S_CURVE:
        c = GetBoundingBox().Centre();
        break;

    default:
        wxASSERT_MSG( false, "DRAWSEGMENT::GetCentre not implemented for shape"
                + ShowShape( GetShape() ) );
        break;
    }

    return c;
}

const wxPoint DRAWSEGMENT::GetArcEnd() const
{
    wxPoint endPoint;         // start of arc

    switch( m_Shape )
    {
    case S_ARC:
        // rotate the starting point of the arc, given by m_End, through the
        // angle m_Angle to get the ending point of the arc.
        // m_Start is the arc centre
        endPoint  = m_End;         // m_End = start point of arc
        RotatePoint( &endPoint, m_Start, -m_Angle );
        break;

    default:
        ;
    }

    return endPoint;   // after rotation, the end of the arc.
}

double DRAWSEGMENT::GetArcAngleStart() const
{
    // due to the Y axis orient atan2 needs - y value
    double angleStart = ArcTangente( GetArcStart().y - GetCenter().y,
                                     GetArcStart().x - GetCenter().x );

    // Normalize it to 0 ... 360 deg, to avoid discontinuity for angles near 180 deg
    // because 180 deg and -180 are very near angles when ampping betewwen -180 ... 180 deg.
    // and this is not easy to handle in calculations
    NORMALIZE_ANGLE_POS( angleStart );

    return angleStart;
}


void DRAWSEGMENT::SetAngle( double aAngle )
{
    NORMALIZE_ANGLE_360( aAngle );

    m_Angle = aAngle;
}


MODULE* DRAWSEGMENT::GetParentModule() const
{
    if( m_Parent->Type() != PCB_MODULE_T )
        return NULL;

    return (MODULE*) m_Parent;
}


void DRAWSEGMENT::Draw( EDA_DRAW_PANEL* panel, wxDC* DC, GR_DRAWMODE draw_mode,
                        const wxPoint& aOffset )
{
    int ux0, uy0, dx, dy;
    int l_trace;
    int radius;

    LAYER_ID    curr_layer = ( (PCB_SCREEN*) panel->GetScreen() )->m_Active_Layer;
    EDA_COLOR_T color;

    BOARD * brd =  GetBoard( );

    if( brd->IsLayerVisible( GetLayer() ) == false )
        return;

    color = brd->GetLayerColor( GetLayer() );

    DISPLAY_OPTIONS* displ_opts = (DISPLAY_OPTIONS*)panel->GetDisplayOptions();

    if( ( draw_mode & GR_ALLOW_HIGHCONTRAST ) &&  displ_opts && displ_opts->m_ContrastModeDisplay )
    {
        if( !IsOnLayer( curr_layer ) && !IsOnLayer( Edge_Cuts ) )
            ColorTurnToDarkDarkGray( &color );
    }

    GRSetDrawMode( DC, draw_mode );
    l_trace = m_Width >> 1;         // half trace width

    // Line start point or Circle and Arc center
    ux0 = m_Start.x + aOffset.x;
    uy0 = m_Start.y + aOffset.y;

    // Line end point or circle and arc start point
    dx = m_End.x + aOffset.x;
    dy = m_End.y + aOffset.y;

    bool filled = displ_opts ? displ_opts->m_DisplayDrawItemsFill : FILLED;

    if( m_Flags & FORCE_SKETCH )
        filled = SKETCH;

    switch( m_Shape )
    {
    case S_CIRCLE:
        radius = KiROUND( Distance( ux0, uy0, dx, dy ) );

        if( filled )
        {
            GRCircle( panel->GetClipBox(), DC, ux0, uy0, radius, m_Width, color );
        }
        else
        {
            GRCircle( panel->GetClipBox(), DC, ux0, uy0, radius - l_trace, color );
            GRCircle( panel->GetClipBox(), DC, ux0, uy0, radius + l_trace, color );
        }

        break;

    case S_ARC:
        double StAngle, EndAngle;
        radius   = KiROUND( Distance( ux0, uy0, dx, dy ) );
        StAngle  = ArcTangente( dy - uy0, dx - ux0 );
        EndAngle = StAngle + m_Angle;

        if( !panel->GetPrintMirrored() )
        {
            if( StAngle > EndAngle )
                std::swap( StAngle, EndAngle );
        }
        else    // Mirrored mode: arc orientation is reversed
        {
            if( StAngle < EndAngle )
                std::swap( StAngle, EndAngle );
        }

        if( filled )
        {
            GRArc( panel->GetClipBox(), DC, ux0, uy0, StAngle, EndAngle,
                   radius, m_Width, color );
        }
        else
        {
            GRArc( panel->GetClipBox(), DC, ux0, uy0, StAngle, EndAngle,
                   radius - l_trace, color );
            GRArc( panel->GetClipBox(), DC, ux0, uy0, StAngle, EndAngle,
                   radius + l_trace, color );
        }

        break;

    case S_CURVE:
        m_BezierPoints = Bezier2Poly( m_Start, m_BezierC1, m_BezierC2, m_End );

        for( unsigned int i=1; i < m_BezierPoints.size(); i++ )
        {
            if( filled )
            {
                GRFillCSegm( panel->GetClipBox(), DC,
                             m_BezierPoints[i].x, m_BezierPoints[i].y,
                             m_BezierPoints[i-1].x, m_BezierPoints[i-1].y,
                             m_Width, color );
            }
            else
            {
                GRCSegm( panel->GetClipBox(), DC,
                         m_BezierPoints[i].x, m_BezierPoints[i].y,
                         m_BezierPoints[i-1].x, m_BezierPoints[i-1].y,
                         m_Width, color );
            }
        }

        break;

    default:
        if( filled )
        {
            GRFillCSegm( panel->GetClipBox(), DC, ux0, uy0, dx, dy, m_Width, color );
        }
        else
        {
            GRCSegm( panel->GetClipBox(), DC, ux0, uy0, dx, dy, m_Width, color );
        }

        break;
    }
}


// see pcbstruct.h
void DRAWSEGMENT::GetMsgPanelInfo( std::vector< MSG_PANEL_ITEM >& aList )
{
    wxString msg;
    wxString coords;

    wxASSERT( m_Parent );

    msg = _( "Drawing" );

    aList.push_back( MSG_PANEL_ITEM( _( "Type" ), msg, DARKCYAN ) );

    wxString    shape = _( "Shape" );

    switch( m_Shape )
    {
    case S_CIRCLE:
        aList.push_back( MSG_PANEL_ITEM( shape, _( "Circle" ), RED ) );
        break;

    case S_ARC:
        aList.push_back( MSG_PANEL_ITEM( shape, _( "Arc" ), RED ) );
        msg.Printf( wxT( "%.1f" ), m_Angle / 10.0 );
        aList.push_back( MSG_PANEL_ITEM( _( "Angle" ), msg, RED ) );
        break;

    case S_CURVE:
        aList.push_back( MSG_PANEL_ITEM( shape, _( "Curve" ), RED ) );
        break;

    default:
    {
        aList.push_back( MSG_PANEL_ITEM( shape, _( "Segment" ), RED ) );

        msg = ::CoordinateToString( GetLineLength( m_Start, m_End ) );
        aList.push_back( MSG_PANEL_ITEM( _( "Length" ), msg, DARKGREEN ) );

        // angle counter-clockwise from 3'o-clock
        const double deg = RAD2DEG( atan2( m_Start.y - m_End.y,
                                           m_End.x - m_Start.x ) );
        msg.Printf( wxT( "%.1f" ), deg );
        aList.push_back( MSG_PANEL_ITEM( _( "Angle" ), msg, DARKGREEN ) );
    }
    }

    wxString start;
    start << GetStart();

    wxString end;
    end << GetEnd();

    aList.push_back( MSG_PANEL_ITEM( start, end, DARKGREEN ) );
    aList.push_back( MSG_PANEL_ITEM( _( "Layer" ), GetLayerName(), DARKBROWN ) );
    msg = ::CoordinateToString( m_Width );
    aList.push_back( MSG_PANEL_ITEM( _( "Width" ), msg, DARKCYAN ) );
}


const EDA_RECT DRAWSEGMENT::GetBoundingBox() const
{
    EDA_RECT bbox;

    bbox.SetOrigin( m_Start );

    switch( m_Shape )
    {
    case S_SEGMENT:
        bbox.SetEnd( m_End );
        break;

    case S_CIRCLE:
        bbox.Inflate( GetRadius() );
        break;

    case S_ARC:
    {
        bbox.Merge( m_End );
        wxPoint end = m_End;
        RotatePoint( &end, m_Start, -m_Angle );
        bbox.Merge( end );

        // Determine the starting quarter
        // 0 right-bottom
        // 1 left-bottom
        // 2 left-top
        // 3 right-top
        unsigned int quarter = 0;       // assume right-bottom

        if( m_End.y < m_Start.y )       // change to left-top
            quarter |= 3;

        if( m_End.x < m_Start.x )       // for left side, the LSB is 2nd bit negated
            quarter ^= 1;

        int radius = GetRadius();
        int angle = (int) GetArcAngleStart() % 900 + m_Angle;
        bool directionCW = ( m_Angle > 0 );      // Is the direction of arc clockwise?

        if( !directionCW )
        {
            angle = 900 - angle;
            quarter = ( quarter + 3 ) % 4;       // -1 modulo arithmetic
        }

        while( angle > 900 )
        {
            switch( quarter )
            {
            case 0:
                bbox.Merge( wxPoint( m_Start.x, m_Start.y + radius ) );     // down
                break;

            case 1:
                bbox.Merge( wxPoint( m_Start.x - radius, m_Start.y ) );     // left
                break;

            case 2:
                bbox.Merge( wxPoint( m_Start.x, m_Start.y - radius ) );     // up
                break;

            case 3:
                bbox.Merge( wxPoint( m_Start.x + radius, m_Start.y ) );     // right
                break;
            }

            if( directionCW )
                ++quarter;
            else
                quarter += 3;       // -1 modulo arithmetic

            quarter %= 4;
            angle -= 900;
        }
    }
        break;

    case S_POLYGON:
    {
        wxPoint p_end;
        MODULE* module = GetParentModule();

        for( unsigned ii = 0; ii < m_PolyPoints.size(); ii++ )
        {
            wxPoint pt = m_PolyPoints[ii];

            if( module ) // Transform, if we belong to a module
            {
                RotatePoint( &pt, module->GetOrientation() );
                pt += module->GetPosition();
            }

            if( ii == 0 )
                p_end = pt;

            bbox.SetX( std::min( bbox.GetX(), pt.x ) );
            bbox.SetY( std::min( bbox.GetY(), pt.y ) );
            p_end.x   = std::max( p_end.x, pt.x );
            p_end.y   = std::max( p_end.y, pt.y );
        }

        bbox.SetEnd( p_end );
    }
        break;

    default:
        ;
    }

    bbox.Inflate( ((m_Width+1) / 2) + 1 );
    bbox.Normalize();

    return bbox;
}


bool DRAWSEGMENT::HitTest( const wxPoint& aPosition ) const
{
    switch( m_Shape )
    {
    case S_CIRCLE:
    case S_ARC:
    {
        wxPoint relPos = aPosition - GetCenter();
        int radius = GetRadius();
        int dist   = KiROUND( EuclideanNorm( relPos ) );

        if( abs( radius - dist ) <= ( m_Width / 2 ) )
        {
            if( m_Shape == S_CIRCLE )
                return true;

            // For arcs, the test point angle must be >= arc angle start
            // and <= arc angle end
            // However angle values > 360 deg are not easy to handle
            // so we calculate the relative angle between arc start point and teast point
            // this relative arc should be < arc angle if arc angle > 0 (CW arc)
            // and > arc angle if arc angle < 0 (CCW arc)
            double arc_angle_start = GetArcAngleStart();    // Always 0.0 ... 360 deg, in 0.1 deg

            double arc_hittest = ArcTangente( relPos.y, relPos.x );

            // Calculate relative angle between the starting point of the arc, and the test point
            arc_hittest -= arc_angle_start;

            // Normalise arc_hittest between 0 ... 360 deg
            NORMALIZE_ANGLE_POS( arc_hittest );

            // Check angle: inside the arc angle when it is > 0
            // and outside the not drawn arc when it is < 0
            if( GetAngle() >= 0.0 )
            {
                if( arc_hittest <= GetAngle() )
                    return true;
            }
            else
            {
                if( arc_hittest >= (3600.0 + GetAngle()) )
                    return true;
            }
        }
    }
        break;

    case S_CURVE:
        for( unsigned int i= 1; i < m_BezierPoints.size(); i++)
        {
            if( TestSegmentHit( aPosition, m_BezierPoints[i-1], m_BezierPoints[i-1], m_Width / 2 ) )
                return true;
        }
        break;

    case S_SEGMENT:
        if( TestSegmentHit( aPosition, m_Start, m_End, m_Width / 2 ) )
            return true;
        break;

    case S_POLYGON:     // not yet handled
        break;

    default:
        wxASSERT_MSG( 0, wxString::Format( "unknown DRAWSEGMENT shape: %d", m_Shape ) );
        break;
    }

    return false;
}


bool DRAWSEGMENT::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    wxPoint p1, p2;
    int radius;
    float theta;
    EDA_RECT arect = aRect;
    arect.Inflate( aAccuracy );

    switch( m_Shape )
    {
    case S_CIRCLE:
        // Test if area intersects or contains the circle:
        if( aContained )
            return arect.Contains( GetBoundingBox() );
        else
            return arect.Intersects( GetBoundingBox() );
        break;

    case S_ARC:
        radius = hypot( (double)( GetEnd().x - GetStart().x ),
                        (double)( GetEnd().y - GetStart().y ) );
        theta  = std::atan2( GetEnd().y - GetStart().y , GetEnd().x - GetStart().x );

        //Approximate the arc with two lines. This should be accurate enough for selection.
        p1.x   = radius * std::cos( theta + M_PI/4 ) + GetStart().x;
        p1.y   = radius * std::sin( theta + M_PI/4 ) + GetStart().y;
        p2.x   = radius * std::cos( theta + M_PI/2 ) + GetStart().x;
        p2.y   = radius * std::sin( theta + M_PI/2 ) + GetStart().y;

        if( aContained )
            return arect.Contains( GetEnd() ) && aRect.Contains( p1 ) && aRect.Contains( p2 );
        else
            return arect.Intersects( GetEnd(), p1 ) || aRect.Intersects( p1, p2 );

        break;

    case S_SEGMENT:
        if( aContained )
            return arect.Contains( GetStart() ) && aRect.Contains( GetEnd() );
        else
            return arect.Intersects( GetStart(), GetEnd() );

        break;

    case S_CURVE:
    case S_POLYGON:     // not yet handled
        break;

    default:
        wxASSERT_MSG( 0, wxString::Format( "unknown DRAWSEGMENT shape: %d", m_Shape ) );
        break;
    }

    return false;
}


wxString DRAWSEGMENT::GetSelectMenuText() const
{
    wxString text;
    wxString temp = ::LengthDoubleToString( GetLength() );

    text.Printf( _( "Pcb Graphic: %s, length %s on %s" ),
                 GetChars( ShowShape( (STROKE_T) m_Shape ) ),
                 GetChars( temp ), GetChars( GetLayerName() ) );

    return text;
}


EDA_ITEM* DRAWSEGMENT::Clone() const
{
    return new DRAWSEGMENT( *this );
}
