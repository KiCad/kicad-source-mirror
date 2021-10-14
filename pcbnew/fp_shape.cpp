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

#include <core/wx_stl_compat.h>
#include <bitmaps.h>
#include <core/mirror.h>
#include <macros.h>
#include <math/util.h>      // for KiROUND
#include <settings/color_settings.h>
#include <settings/settings_manager.h>
#include <pcb_edit_frame.h>
#include <footprint.h>
#include <fp_shape.h>
#include <view/view.h>


FP_SHAPE::FP_SHAPE( FOOTPRINT* parent, SHAPE_T aShape ) :
        PCB_SHAPE( parent, PCB_FP_SHAPE_T, aShape )
{
    m_layer = F_SilkS;
}


FP_SHAPE::~FP_SHAPE()
{
}


void FP_SHAPE::SetLocalCoord()
{
    FOOTPRINT* fp = static_cast<FOOTPRINT*>( m_parent );

    if( fp == NULL )
    {
        m_start0     = m_start;
        m_end0       = m_end;
        m_arcCenter0 = m_arcCenter;
        m_bezierC1_0 = m_bezierC1;
        m_bezierC2_0 = m_bezierC2;
        return;
    }

    m_start0     = m_start     - fp->GetPosition();
    m_end0       = m_end       - fp->GetPosition();
    m_arcCenter0 = m_arcCenter - fp->GetPosition();
    m_bezierC1_0 = m_bezierC1  - fp->GetPosition();
    m_bezierC2_0 = m_bezierC2  - fp->GetPosition();

    double angle = fp->GetOrientation();
    RotatePoint( &m_start0.x,     &m_start0.y,     -angle );
    RotatePoint( &m_end0.x,       &m_end0.y,       -angle );
    RotatePoint( &m_arcCenter0.x, &m_arcCenter0.y, -angle );
    RotatePoint( &m_bezierC1_0.x, &m_bezierC1_0.y, -angle );
    RotatePoint( &m_bezierC2_0.x, &m_bezierC2_0.y, -angle );
}


void FP_SHAPE::SetDrawCoord()
{
    FOOTPRINT* fp = static_cast<FOOTPRINT*>( m_parent );

    m_start     = m_start0;
    m_end       = m_end0;
    m_arcCenter = m_arcCenter0;
    m_bezierC1  = m_bezierC1_0;
    m_bezierC2  = m_bezierC2_0;

    if( fp )
    {
        RotatePoint( &m_start.x, &m_start.y, fp->GetOrientation() );
        RotatePoint( &m_end.x, &m_end.y, fp->GetOrientation() );
        RotatePoint( &m_arcCenter.x, &m_arcCenter.y, fp->GetOrientation() );
        RotatePoint( &m_bezierC1.x, &m_bezierC1.y, fp->GetOrientation() );
        RotatePoint( &m_bezierC2.x, &m_bezierC2.y, fp->GetOrientation() );

        m_start     += fp->GetPosition();
        m_end       += fp->GetPosition();
        m_arcCenter += fp->GetPosition();
        m_bezierC1  += fp->GetPosition();
        m_bezierC2  += fp->GetPosition();
    }

    RebuildBezierToSegmentsPointsList( m_width );
}


void FP_SHAPE::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    FOOTPRINT* fp = static_cast<FOOTPRINT*>( m_parent );

    aList.emplace_back( _( "Footprint" ), fp ? fp->GetReference() : _( "<invalid>" ) );

    // append the features shared with the base class
    PCB_SHAPE::GetMsgPanelInfo( aFrame, aList );
}


wxString FP_SHAPE::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    return wxString::Format( _( "%s on %s" ),
                             ShowShape(),
                             GetLayerName() );
}


BITMAPS FP_SHAPE::GetMenuImage() const
{
    return BITMAPS::show_mod_edge;
}


EDA_ITEM* FP_SHAPE::Clone() const
{
    return new FP_SHAPE( *this );
}


wxPoint FP_SHAPE::GetCenter0() const
{
    switch( m_shape )
    {
    case SHAPE_T::ARC:
        return m_arcCenter0;

    case SHAPE_T::CIRCLE:
        return m_start0;

    default:
        UNIMPLEMENTED_FOR( SHAPE_T_asString() );
        return wxPoint();
    }
}


void FP_SHAPE::SetCenter0( const wxPoint& aCenter )
{
    switch( m_shape )
    {
    case SHAPE_T::ARC:
        m_arcCenter0 = aCenter;
        break;

    case SHAPE_T::CIRCLE:
        m_start0 = aCenter;
        break;

    default:
        UNIMPLEMENTED_FOR( SHAPE_T_asString() );
    }
}


wxPoint FP_SHAPE::GetArcMid0() const
{
    wxPoint mid0 = m_start0;
    RotatePoint( &mid0, m_arcCenter0, -GetArcAngle() / 2.0 );
    return mid0;
}


void FP_SHAPE::SetArcAngleAndEnd0( double aAngle, bool aCheckNegativeAngle )
{
    m_end0 = m_start0;
    RotatePoint( &m_end0, m_arcCenter0, -NormalizeAngle360Max( aAngle ) );

    if( aCheckNegativeAngle && aAngle < 0 )
        std::swap( m_start0, m_end0 );
}


void FP_SHAPE::SetArcGeometry0( const wxPoint& aStart0, const wxPoint& aMid0, const wxPoint& aEnd0 )
{
    m_start0 = aStart0;
    m_end0 = aEnd0;
    m_arcCenter0 = CalcArcCenter( aStart0, aMid0, aEnd0 );
}


void FP_SHAPE::Flip( const wxPoint& aCentre, bool aFlipLeftRight )
{
    wxPoint pt( 0, 0 );

    switch( GetShape() )
    {
    case SHAPE_T::ARC:
    case SHAPE_T::SEGMENT:
    case SHAPE_T::RECT:
    case SHAPE_T::CIRCLE:
    case SHAPE_T::BEZIER:
        // If Start0 and Start are equal (ie: Footprint Editor), then flip both sets around the
        // centre point.
        if( m_start == m_start0 )
            pt = aCentre;

        if( aFlipLeftRight )
        {
            MIRROR( m_start.x, aCentre.x );
            MIRROR( m_end.x, aCentre.x );
            MIRROR( m_arcCenter.x, aCentre.x );
            MIRROR( m_bezierC1.x, aCentre.x );
            MIRROR( m_bezierC2.x, aCentre.x );
            MIRROR( m_start0.x, pt.x );
            MIRROR( m_end0.x, pt.x );
            MIRROR( m_arcCenter0.x, pt.x );
            MIRROR( m_bezierC1_0.x, pt.x );
            MIRROR( m_bezierC2_0.x, pt.x );
        }
        else
        {
            MIRROR( m_start.y, aCentre.y );
            MIRROR( m_end.y, aCentre.y );
            MIRROR( m_arcCenter.y, aCentre.y );
            MIRROR( m_bezierC1.y, aCentre.y );
            MIRROR( m_bezierC2.y, aCentre.y );
            MIRROR( m_start0.y, pt.y );
            MIRROR( m_end0.y, pt.y );
            MIRROR( m_arcCenter0.y, pt.y );
            MIRROR( m_bezierC1_0.y, pt.y );
            MIRROR( m_bezierC2_0.y, pt.y );
        }

        if( GetShape() == SHAPE_T::BEZIER )
            RebuildBezierToSegmentsPointsList( m_width );

        break;

    case SHAPE_T::POLY:
        // polygon corners coordinates are relative to the footprint position, orientation 0
        m_poly.Mirror( aFlipLeftRight, !aFlipLeftRight );
        break;

    default:
        UNIMPLEMENTED_FOR( SHAPE_T_asString() );
    }

    SetLayer( FlipLayer( GetLayer(), GetBoard()->GetCopperLayerCount() ) );
}

bool FP_SHAPE::IsParentFlipped() const
{
    if( GetParent() &&  GetParent()->GetLayer() == B_Cu )
        return true;
    return false;
}

void FP_SHAPE::Mirror( const wxPoint& aCentre, bool aMirrorAroundXAxis )
{
    // Mirror an edge of the footprint. the layer is not modified
    // This is a footprint shape modification.

    switch( GetShape() )
    {
    case SHAPE_T::ARC:
    case SHAPE_T::SEGMENT:
    case SHAPE_T::RECT:
    case SHAPE_T::CIRCLE:
    case SHAPE_T::BEZIER:
        if( aMirrorAroundXAxis )
        {
            MIRROR( m_start0.y, aCentre.y );
            MIRROR( m_end0.y, aCentre.y );
            MIRROR( m_arcCenter0.y, aCentre.y );
            MIRROR( m_bezierC1_0.y, aCentre.y );
            MIRROR( m_bezierC2_0.y, aCentre.y );
        }
        else
        {
            MIRROR( m_start0.x, aCentre.x );
            MIRROR( m_end0.x, aCentre.x );
            MIRROR( m_arcCenter0.x, aCentre.x );
            MIRROR( m_bezierC1_0.x, aCentre.x );
            MIRROR( m_bezierC2_0.x, aCentre.x );
        }

        if( GetShape() == SHAPE_T::BEZIER )
            RebuildBezierToSegmentsPointsList( m_width );

        break;

    case SHAPE_T::POLY:
        // polygon corners coordinates are always relative to the
        // footprint position, orientation 0
        m_poly.Mirror( !aMirrorAroundXAxis, aMirrorAroundXAxis );
        break;

    default:
        UNIMPLEMENTED_FOR( SHAPE_T_asString() );
    }

    SetDrawCoord();
}

void FP_SHAPE::Rotate( const wxPoint& aRotCentre, double aAngle )
{
    // We should rotate the relative coordinates, but to avoid duplicate code do the base class
    // rotation of draw coordinates, which is acceptable because in the footprint editor
    // m_Pos0 = m_Pos
    PCB_SHAPE::Rotate( aRotCentre, aAngle );

    // and now update the relative coordinates, which are the reference in most transforms.
    SetLocalCoord();
}


void FP_SHAPE::Move( const wxPoint& aMoveVector )
{
    // Move an edge of the footprint.
    // This is a footprint shape modification.

    switch( GetShape() )
    {
    case SHAPE_T::ARC:
    case SHAPE_T::SEGMENT:
    case SHAPE_T::RECT:
    case SHAPE_T::CIRCLE:
    case SHAPE_T::BEZIER:
        m_start0     += aMoveVector;
        m_end0       += aMoveVector;
        m_arcCenter0 += aMoveVector;
        m_bezierC1_0 += aMoveVector;
        m_bezierC2_0 += aMoveVector;
        break;

    case SHAPE_T::POLY:
        // polygon corners coordinates are always relative to the
        // footprint position, orientation 0
        m_poly.Move( VECTOR2I( aMoveVector ) );

        break;

    default:
        UNIMPLEMENTED_FOR( SHAPE_T_asString() );
    }

    SetDrawCoord();
}


double FP_SHAPE::ViewGetLOD( int aLayer, KIGFX::VIEW* aView ) const
{
    constexpr double HIDE = std::numeric_limits<double>::max();

    if( !aView )
        return 0;

    // Handle Render tab switches
    if( !IsParentFlipped() && !aView->IsLayerVisible( LAYER_MOD_FR ) )
        return HIDE;

    if( IsParentFlipped() && !aView->IsLayerVisible( LAYER_MOD_BK ) )
        return HIDE;

    // Other layers are shown without any conditions
    return 0.0;
}


static struct FP_SHAPE_DESC
{
    FP_SHAPE_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( FP_SHAPE );
        propMgr.AddTypeCast( new TYPE_CAST<FP_SHAPE, BOARD_ITEM> );
        propMgr.AddTypeCast( new TYPE_CAST<FP_SHAPE, EDA_SHAPE> );
        propMgr.AddTypeCast( new TYPE_CAST<FP_SHAPE, PCB_SHAPE> );
        propMgr.InheritsAfter( TYPE_HASH( FP_SHAPE ), TYPE_HASH( BOARD_ITEM ) );
        propMgr.InheritsAfter( TYPE_HASH( FP_SHAPE ), TYPE_HASH( EDA_SHAPE ) );
        propMgr.InheritsAfter( TYPE_HASH( FP_SHAPE ), TYPE_HASH( PCB_SHAPE ) );

        propMgr.AddProperty( new PROPERTY<FP_SHAPE, wxString>( _HKI( "Parent" ),
                    NO_SETTER( FP_SHAPE, wxString ), &FP_SHAPE::GetParentAsString ) );
    }
} _FP_SHAPE_DESC;
