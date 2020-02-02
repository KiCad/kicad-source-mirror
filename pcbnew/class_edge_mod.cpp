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

#include <fctsys.h>
#include <bitmaps.h>
#include <math/util.h>      // for KiROUND
#include <settings/color_settings.h>
#include <settings/settings_manager.h>
#include <pcb_edit_frame.h>
#include <class_board.h>
#include <class_module.h>
#include <class_edge_mod.h>
#include <view/view.h>


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


// see class_edge_mod.h
void EDGE_MODULE::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    wxString msg;

    MODULE*  module = (MODULE*) m_Parent;

    if( !module )
        return;

    BOARD* board = (BOARD*) module->GetParent();

    if( !board )
        return;

    aList.emplace_back( _( "Footprint" ), module->GetReference(), DARKCYAN );

    // append the features shared with the base class
    DRAWSEGMENT::GetMsgPanelInfo( aFrame, aList );
}


wxString EDGE_MODULE::GetSelectMenuText( EDA_UNITS aUnits ) const
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
    wxPoint pt( 0, 0 );

    switch( GetShape() )
    {
    case S_ARC:
        SetAngle( -GetAngle() );
        KI_FALLTHROUGH;

    default:
    case S_SEGMENT:
    case S_CURVE:
        // If Start0 and Start are equal (ie: ModEdit), then flip both sets around the
        // centre point.
        if( m_Start == m_Start0 )
            pt = aCentre;

        if( aFlipLeftRight )
        {
            MIRROR( m_Start.x, aCentre.x );
            MIRROR( m_End.x, aCentre.x );
            MIRROR( m_BezierC1.x, aCentre.x );
            MIRROR( m_BezierC2.x, aCentre.x );
            MIRROR( m_Start0.x, pt.x );
            MIRROR( m_End0.x, pt.x );
            MIRROR( m_Bezier0_C1.x, pt.x );
            MIRROR( m_Bezier0_C2.x, pt.x );
        }
        else
        {
            MIRROR( m_Start.y, aCentre.y );
            MIRROR( m_End.y, aCentre.y );
            MIRROR( m_BezierC1.y, aCentre.y );
            MIRROR( m_BezierC2.y, aCentre.y );
            MIRROR( m_Start0.y, pt.y );
            MIRROR( m_End0.y, pt.y );
            MIRROR( m_Bezier0_C1.y, pt.y );
            MIRROR( m_Bezier0_C2.y, pt.y );
        }

        RebuildBezierToSegmentsPointsList( m_Width );
        break;

    case S_POLYGON:
        // polygon corners coordinates are relative to the footprint position, orientation 0
        m_Poly.Mirror( aFlipLeftRight, !aFlipLeftRight );
        break;
    }

    // DRAWSEGMENT items are not usually on copper layers, but it can happen in microwave apps.
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

void EDGE_MODULE::Mirror( const wxPoint& aCentre, bool aMirrorAroundXAxis )
{
    // Mirror an edge of the footprint. the layer is not modified
    // This is a footprint shape modification.
    switch( GetShape() )
    {
    case S_ARC:
        SetAngle( -GetAngle() );
        KI_FALLTHROUGH;

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
        m_Poly.Mirror( !aMirrorAroundXAxis, aMirrorAroundXAxis );
        break;
    }

    SetDrawCoord();
}

void EDGE_MODULE::Rotate( const wxPoint& aRotCentre, double aAngle )
{
    // We should rotate the relative coordinates, but to avoid duplicate code do the base class
    // rotation of draw coordinates, which is acceptable because in module editor, m_Pos0 = m_Pos
    DRAWSEGMENT::Rotate( aRotCentre, aAngle );

    // and now update the relative coordinates, which are the reference in most transforms.
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
        m_Poly.Move( VECTOR2I( aMoveVector ) );

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


static struct EDGE_MODULE_DESC
{
    EDGE_MODULE_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( EDGE_MODULE );
        propMgr.InheritsAfter( TYPE_HASH( EDGE_MODULE ), TYPE_HASH( DRAWSEGMENT ) );
    }
} _EDGE_MODULE_DESC;
