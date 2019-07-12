/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file class_edge_mod.cpp
 * @brief EDGE_MODULE class definition.
 */

#include <fctsys.h>
#include <gr_basic.h>
#include <trigo.h>
#include <pcb_screen.h>
#include <confirm.h>
#include <kicad_string.h>
#include <richio.h>
#include <macros.h>
#include <math_for_graphics.h>
#include <pcb_base_frame.h>
#include <msgpanel.h>
#include <base_units.h>
#include <bitmaps.h>

#include <pcb_edit_frame.h>
#include <class_board.h>
#include <class_module.h>
#include <class_edge_mod.h>

#include <view/view.h>

#include <stdio.h>

EDGE_MODULE::EDGE_MODULE( MODULE* parent, STROKE_T aShape ) :
    DRAWSEGMENT( parent, PCB_MODULE_EDGE_T )
{
    m_Shape = aShape;
    m_Angle = 0;
    m_Layer = F_SilkS;
}


EDGE_MODULE::~EDGE_MODULE()
{
}


void EDGE_MODULE::SetLocalCoord()
{
    MODULE* module = (MODULE*) m_Parent;

    if( module == NULL )
    {
        m_Start0 = m_Start;
        m_End0 = m_End;
        m_Bezier0_C1 = m_BezierC1;
        m_Bezier0_C2 = m_BezierC2;
        return;
    }

    m_Start0 = m_Start - module->GetPosition();
    m_End0 = m_End - module->GetPosition();
    m_Bezier0_C1 = m_BezierC1 - module->GetPosition();
    m_Bezier0_C2 = m_BezierC2 - module->GetPosition();
    double angle = module->GetOrientation();
    RotatePoint( &m_Start0.x, &m_Start0.y, -angle );
    RotatePoint( &m_End0.x, &m_End0.y, -angle );
    RotatePoint( &m_Bezier0_C1.x, &m_Bezier0_C1.y, -angle );
    RotatePoint( &m_Bezier0_C2.x, &m_Bezier0_C2.y, -angle );
}


void EDGE_MODULE::SetDrawCoord()
{
    MODULE* module = (MODULE*) m_Parent;

    m_Start = m_Start0;
    m_End   = m_End0;
    m_BezierC1 = m_Bezier0_C1;
    m_BezierC2 = m_Bezier0_C2;

    if( module )
    {
        RotatePoint( &m_Start.x, &m_Start.y, module->GetOrientation() );
        RotatePoint( &m_End.x, &m_End.y, module->GetOrientation() );
        RotatePoint( &m_BezierC1.x, &m_BezierC1.y, module->GetOrientation() );
        RotatePoint( &m_BezierC2.x, &m_BezierC2.y, module->GetOrientation() );

        m_Start += module->GetPosition();
        m_End   += module->GetPosition();
        m_BezierC1   += module->GetPosition();
        m_BezierC2   += module->GetPosition();
    }

    RebuildBezierToSegmentsPointsList( m_Width );
}


void EDGE_MODULE::Print( PCB_BASE_FRAME* aFrame, wxDC* DC, const wxPoint& offset )
{
    int     ux0, uy0, dx, dy, radius, StAngle, EndAngle;
    MODULE* module = (MODULE*) m_Parent;
    BOARD*  brd = GetBoard( );

    if( !module || !brd->IsLayerVisible( m_Layer ) )
        return;

    auto color = aFrame->Settings().Colors().GetLayerColor( m_Layer );
    auto displ_opts = (PCB_DISPLAY_OPTIONS*)( aFrame->GetDisplayOptions() );

    ux0 = m_Start.x - offset.x;
    uy0 = m_Start.y - offset.y;

    dx = m_End.x - offset.x;
    dy = m_End.y - offset.y;

    bool filled = displ_opts ? displ_opts->m_DisplayModEdgeFill : FILLED;

    if( IsCopperLayer( m_Layer ) )
        filled = displ_opts ? displ_opts->m_DisplayPcbTrackFill : FILLED;

    switch( m_Shape )
    {
    case S_SEGMENT:
        if( filled )
            GRLine( nullptr, DC, ux0, uy0, dx, dy, m_Width, color );
        else
            // SKETCH Mode
            GRCSegm( nullptr, DC, ux0, uy0, dx, dy, m_Width, color );

        break;

    case S_CIRCLE:
        radius = KiROUND( Distance( ux0, uy0, dx, dy ) );

        if( filled )
        {
            GRCircle( nullptr, DC, ux0, uy0, radius, m_Width, color );
        }
        else        // SKETCH Mode
        {
            GRCircle( nullptr, DC, ux0, uy0, radius + (m_Width / 2), color );
            GRCircle( nullptr, DC, ux0, uy0, radius - (m_Width / 2), color );
        }

        break;

    case S_ARC:
        radius   = KiROUND( Distance( ux0, uy0, dx, dy ) );
        StAngle  = ArcTangente( dy - uy0, dx - ux0 );
        EndAngle = StAngle + m_Angle;

        if( StAngle > EndAngle )
            std::swap( StAngle, EndAngle );

        if( filled )
        {
            GRArc( nullptr, DC, ux0, uy0, StAngle, EndAngle, radius, m_Width, color );
        }
        else        // SKETCH Mode
        {
            GRArc( nullptr, DC, ux0, uy0, StAngle, EndAngle, radius + (m_Width / 2), color );
            GRArc( nullptr, DC, ux0, uy0, StAngle, EndAngle, radius - (m_Width / 2), color );
        }
        break;

    case S_POLYGON:
        if( m_Poly.IsEmpty() )
            break;

        {
        // We must compute absolute coordinates from m_PolyPoints
        // which are relative to module position, orientation 0
        std::vector<wxPoint> points;

        for( auto iter = m_Poly.CIterate(); iter; iter++ )
        {
            points.push_back( wxPoint( iter->x,iter->y ) );
        }

        for( unsigned ii = 0; ii < points.size(); ii++ )
        {
            wxPoint& pt = points[ii];

            RotatePoint( &pt.x, &pt.y, module->GetOrientation() );
            pt += module->GetPosition() - offset;
        }

        GRPoly( nullptr, DC, points.size(), &points[0], true, m_Width, color, color );
        }
        break;

    case S_CURVE:
        {
            RebuildBezierToSegmentsPointsList( m_Width );

            wxPoint& startp = m_BezierPoints[0];

            for( unsigned int i = 1; i < m_BezierPoints.size(); i++ )
            {
                wxPoint& endp = m_BezierPoints[i];

                if( filled )
                    GRFilledSegment( nullptr, DC, startp-offset, endp-offset, m_Width, color );
                else
                    GRCSegm( nullptr, DC, startp-offset, endp-offset, m_Width, color );

                startp = m_BezierPoints[i];
            }
        }
        break;

    default:
        break;
    }
}


// see class_edge_mod.h
void EDGE_MODULE::GetMsgPanelInfo( EDA_UNITS_T aUnits, std::vector< MSG_PANEL_ITEM >& aList )
{
    wxString msg;

    MODULE*  module = (MODULE*) m_Parent;

    if( !module )
        return;

    BOARD* board = (BOARD*) module->GetParent();

    if( !board )
        return;

    aList.push_back( MSG_PANEL_ITEM( _( "Footprint" ), module->GetReference(), DARKCYAN ) );

    // append the features shared with the base class
    DRAWSEGMENT::GetMsgPanelInfo( aUnits, aList );
}



wxString EDGE_MODULE::GetSelectMenuText( EDA_UNITS_T aUnits ) const
{
    return wxString::Format( _( "Graphic %s of %s on %s" ),
                             ShowShape( m_Shape  ),
                             ((MODULE*) GetParent())->GetReference(),
                             GetLayerName() );
}


BITMAP_DEF EDGE_MODULE::GetMenuImage() const
{
    return show_mod_edge_xpm;
}


EDA_ITEM* EDGE_MODULE::Clone() const
{
    return new EDGE_MODULE( *this );
}


void EDGE_MODULE::Flip( const wxPoint& aCentre, bool aFlipLeftRight )
{
    wxPoint pt;

    switch( GetShape() )
    {
    case S_ARC:
        SetAngle( -GetAngle() );
        //Fall through
    default:
    case S_SEGMENT:
    case S_CURVE:
        if( aFlipLeftRight )
        {
            MIRROR( m_Start.x, aCentre.x );
            MIRROR( m_End.x, aCentre.x );
            MIRROR( m_BezierC1.x, aCentre.x );
            MIRROR( m_BezierC2.x, aCentre.x );
            MIRROR( m_Start0.x, 0 );
            MIRROR( m_End0.x, 0 );
            MIRROR( m_Bezier0_C1.x, 0 );
            MIRROR( m_Bezier0_C2.x, 0 );
        }
        else
        {
            MIRROR( m_Start.y, aCentre.y );
            MIRROR( m_End.y, aCentre.y );
            MIRROR( m_BezierC1.y, aCentre.y );
            MIRROR( m_BezierC2.y, aCentre.y );
            MIRROR( m_Start0.y, 0 );
            MIRROR( m_End0.y, 0 );
            MIRROR( m_Bezier0_C1.y, 0 );
            MIRROR( m_Bezier0_C2.y, 0 );
        }

        RebuildBezierToSegmentsPointsList( m_Width );
        break;

    case S_POLYGON:
        // polygon corners coordinates are always relative to the
        // footprint position, orientation 0
        for( auto iter = m_Poly.Iterate(); iter; iter++ )
        {
            if( aFlipLeftRight )
                MIRROR( iter->x, 0 );
            else
                MIRROR( iter->y, 0 );
        }
	break;
    }

    // DRAWSEGMENT items are not usually on copper layers, but
    // it can happen in microwave apps.
    // However, currently, only on Front or Back layers.
    // So the copper layers count is not taken in account
    SetLayer( FlipLayer( GetLayer() ) );
}

bool EDGE_MODULE::IsParentFlipped() const
{
    if( GetParent() &&  GetParent()->GetLayer() == B_Cu )
        return true;
    return false;
}

void EDGE_MODULE::Mirror( wxPoint aCentre, bool aMirrorAroundXAxis )
{
    // Mirror an edge of the footprint. the layer is not modified
    // This is a footprint shape modification.
    switch( GetShape() )
    {
    case S_ARC:
        SetAngle( -GetAngle() );
        //Fall through
    default:
    case S_CURVE:
    case S_SEGMENT:
        if( aMirrorAroundXAxis )
        {
            MIRROR( m_Start0.y, aCentre.y );
            MIRROR( m_End0.y, aCentre.y );
            MIRROR( m_Bezier0_C1.y, aCentre.y );
            MIRROR( m_Bezier0_C2.y, aCentre.y );
        }
        else
        {
            MIRROR( m_Start0.x, aCentre.x );
            MIRROR( m_End0.x, aCentre.x );
            MIRROR( m_Bezier0_C1.x, aCentre.x );
            MIRROR( m_Bezier0_C2.x, aCentre.x );
        }

        for( unsigned ii = 0; ii < m_BezierPoints.size(); ii++ )
        {
            if( aMirrorAroundXAxis )
                MIRROR( m_BezierPoints[ii].y, aCentre.y );
            else
                MIRROR( m_BezierPoints[ii].x, aCentre.x );
        }

        break;

    case S_POLYGON:
        // polygon corners coordinates are always relative to the
        // footprint position, orientation 0
        for( auto iter = m_Poly.Iterate(); iter; iter++ )
        {
            if( aMirrorAroundXAxis )
                MIRROR( iter->y, aCentre.y );
            else
                MIRROR( iter->x, aCentre.x );
        }
    }

    SetDrawCoord();
}

void EDGE_MODULE::Rotate( const wxPoint& aRotCentre, double aAngle )
{
    // We should rotate the relative coordinates, but to avoid duplicate code,
    // do the base class rotation of draw coordinates, which is acceptable
    // because in module editor, m_Pos0 = m_Pos
    DRAWSEGMENT::Rotate( aRotCentre, aAngle );

    // and now update the relative coordinates, which are
    // the reference in most transforms.
    SetLocalCoord();
}


void EDGE_MODULE::Move( const wxPoint& aMoveVector )
{
    // Move an edge of the footprint.
    // This is a footprint shape modification.
    m_Start0 += aMoveVector;
    m_End0   += aMoveVector;
    m_Bezier0_C1   += aMoveVector;
    m_Bezier0_C2   += aMoveVector;

    switch( GetShape() )
    {
    default:
        break;

    case S_POLYGON:
        // polygon corners coordinates are always relative to the
        // footprint position, orientation 0
        for( auto iter = m_Poly.Iterate(); iter; iter++ )
            *iter += VECTOR2I( aMoveVector );

        break;
    }

    SetDrawCoord();
}

unsigned int EDGE_MODULE::ViewGetLOD( int aLayer, KIGFX::VIEW* aView ) const
{
    const int HIDE = std::numeric_limits<unsigned int>::max();

    if( !aView )
        return 0;

    // Handle Render tab switches
    if( !IsParentFlipped() && !aView->IsLayerVisible( LAYER_MOD_FR ) )
        return HIDE;

    if( IsParentFlipped() && !aView->IsLayerVisible( LAYER_MOD_BK ) )
        return HIDE;

    // Other layers are shown without any conditions
    return 0;
}
