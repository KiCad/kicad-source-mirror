/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanoadoo.fr
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

/**
 * @file sch_no_connect.cpp
 * @brief Class SCH_NO_CONNECT implementation.
 */

#include <sch_draw_panel.h>
#include <plotters/plotter.h>
#include <bitmaps.h>
#include <schematic.h>
#include <geometry/geometry_utils.h>
#include <sch_no_connect.h>
#include <settings/color_settings.h>
#include <default_values.h>    // For some default values
#include <core/mirror.h>
#include <trigo.h>
#include <gr_basic.h>


SCH_NO_CONNECT::SCH_NO_CONNECT( const VECTOR2I& pos ) :
    SCH_ITEM( nullptr, SCH_NO_CONNECT_T )
{
    m_pos    = pos;
    m_size   = schIUScale.MilsToIU( DEFAULT_NOCONNECT_SIZE );   // Default no-connect symbol size.

    SetLayer( LAYER_NOCONNECT );
}


EDA_ITEM* SCH_NO_CONNECT::Clone() const
{
    return new SCH_NO_CONNECT( *this );
}


void SCH_NO_CONNECT::swapData( SCH_ITEM* aItem )
{
    wxCHECK_RET( ( aItem != nullptr ) && ( aItem->Type() == SCH_NO_CONNECT_T ),
                 wxT( "Cannot swap no connect data with invalid item." ) );

    SCH_NO_CONNECT* item = (SCH_NO_CONNECT*)aItem;
    std::swap( m_pos, item->m_pos );
    std::swap( m_size, item->m_size );
}


const BOX2I SCH_NO_CONNECT::GetBoundingBox() const
{
    int   delta = ( GetPenWidth() + GetSize() ) / 2;
    BOX2I bbox( m_pos );

    bbox.Inflate( delta );

    return bbox;
}


std::vector<int> SCH_NO_CONNECT::ViewGetLayers() const
{
    return { LAYER_NOCONNECT, LAYER_SELECTION_SHADOWS };
}


void SCH_NO_CONNECT::GetEndPoints( std::vector< DANGLING_END_ITEM >& aItemList )
{
    DANGLING_END_ITEM item( NO_CONNECT_END, this, m_pos );
    aItemList.push_back( item );
}


int SCH_NO_CONNECT::GetPenWidth() const
{
    if( !Schematic() )
        return 1;

    return std::max( Schematic()->Settings().m_DefaultLineWidth, 1 );
}


void SCH_NO_CONNECT::MirrorVertically( int aCenter )
{
    MIRROR( m_pos.y, aCenter );
}


void SCH_NO_CONNECT::MirrorHorizontally( int aCenter )
{
    MIRROR( m_pos.x, aCenter );
}


void SCH_NO_CONNECT::Rotate( const VECTOR2I& aCenter, bool aRotateCCW )
{
    RotatePoint( m_pos, aCenter, aRotateCCW ? ANGLE_90 : ANGLE_270 );
}


bool SCH_NO_CONNECT::HasConnectivityChanges( const SCH_ITEM* aItem,
                                             const SCH_SHEET_PATH* aInstance ) const
{
    // Do not compare to ourself.
    if( aItem == this )
        return false;

    const SCH_NO_CONNECT* noConnect = dynamic_cast<const SCH_NO_CONNECT*>( aItem );

    // Don't compare against a different SCH_ITEM.
    wxCHECK( noConnect, false );

    return GetPosition() != noConnect->GetPosition();
}


std::vector<VECTOR2I> SCH_NO_CONNECT::GetConnectionPoints() const
{
    return { m_pos };
}


bool SCH_NO_CONNECT::doIsConnected( const VECTOR2I& aPosition ) const
{
    return m_pos == aPosition;
}


bool SCH_NO_CONNECT::HitTest( const VECTOR2I& aPosition, int aAccuracy ) const
{
    int delta = ( GetPenWidth() + GetSize() ) / 2 + aAccuracy;

    VECTOR2I dist = aPosition - m_pos;

    if( ( std::abs( dist.x ) <= delta ) && ( std::abs( dist.y ) <= delta ) )
        return true;

    return false;
}


bool SCH_NO_CONNECT::HitTest( const BOX2I& aRect, bool aContained, int aAccuracy ) const
{
    BOX2I rect = aRect;

    rect.Inflate( aAccuracy );

    if( aContained )
        return rect.Contains( GetBoundingBox() );

    return rect.Intersects( GetBoundingBox() );
}


bool SCH_NO_CONNECT::HitTest( const SHAPE_LINE_CHAIN& aPoly, bool aContained ) const
{
    return KIGEOM::BoxHitTest( aPoly, GetBoundingBox(), aContained );
}


void SCH_NO_CONNECT::Plot( PLOTTER* aPlotter, bool aBackground, const SCH_PLOT_OPTS& aPlotOpts,
                           int aUnit, int aBodyStyle, const VECTOR2I& aOffset, bool aDimmed )
{
    if( aBackground )
        return;

    int delta = GetSize() / 2;
    int pX = m_pos.x;
    int pY = m_pos.y;
    int penWidth = GetEffectivePenWidth( getRenderSettings( aPlotter ) );

    COLOR4D color = aPlotter->RenderSettings()->GetLayerColor( LAYER_NOCONNECT );

    if( color.m_text.has_value() && Schematic() )
        color = COLOR4D( ResolveText( color.m_text.value(), &Schematic()->CurrentSheet() ) );

    aPlotter->SetCurrentLineWidth( penWidth );
    aPlotter->SetColor( color );
    aPlotter->MoveTo( VECTOR2I( pX - delta, pY - delta ) );
    aPlotter->FinishTo( VECTOR2I( pX + delta, pY + delta ) );
    aPlotter->MoveTo( VECTOR2I( pX + delta, pY - delta ) );
    aPlotter->FinishTo( VECTOR2I( pX - delta, pY + delta ) );
}


BITMAPS SCH_NO_CONNECT::GetMenuImage() const
{
    return BITMAPS::noconn;
}


bool SCH_NO_CONNECT::operator==( const SCH_ITEM& aOther ) const
{
    if( aOther.Type() != Type() )
        return false;

    const SCH_NO_CONNECT* other = static_cast<const SCH_NO_CONNECT*>( &aOther );

    if( m_pos != other->m_pos )
        return false;

    return true;
}


double SCH_NO_CONNECT::Similarity( const SCH_ITEM& aOther ) const
{
    if( m_Uuid == aOther.m_Uuid )
        return 1.0;

    if( aOther.Type() != Type() )
        return 0.0;

    const SCH_NO_CONNECT* other = static_cast<const SCH_NO_CONNECT*>( &aOther );

    if( m_pos != other->m_pos )
        return 0.0;

    return 1.0;
}
