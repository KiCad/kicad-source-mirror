/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanoadoo.fr
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
 * @file sch_no_connect.cpp
 * @brief Class SCH_NO_CONNECT implementation.
 */

#include <sch_draw_panel.h>
#include <plotter.h>
#include <bitmaps.h>
#include <schematic.h>
#include <sch_no_connect.h>
#include <settings/color_settings.h>
#include <default_values.h>    // For some default values
#include <core/mirror.h>
#include <trigo.h>


SCH_NO_CONNECT::SCH_NO_CONNECT( const wxPoint& pos ) :
    SCH_ITEM( NULL, SCH_NO_CONNECT_T )
{
    m_pos    = pos;
    m_size   = Mils2iu( DEFAULT_NOCONNECT_SIZE );      ///< No-connect symbol size.

    SetLayer( LAYER_NOCONNECT );
}


EDA_ITEM* SCH_NO_CONNECT::Clone() const
{
    return new SCH_NO_CONNECT( *this );
}


void SCH_NO_CONNECT::SwapData( SCH_ITEM* aItem )
{
    wxCHECK_RET( (aItem != NULL) && (aItem->Type() == SCH_NO_CONNECT_T),
                 wxT( "Cannot swap no connect data with invalid item." ) );

    SCH_NO_CONNECT* item = (SCH_NO_CONNECT*)aItem;
    std::swap( m_pos, item->m_pos );
    std::swap( m_size, item->m_size );
}


const EDA_RECT SCH_NO_CONNECT::GetBoundingBox() const
{
    int      delta = ( GetPenWidth() + GetSize() ) / 2;
    EDA_RECT box;

    box.SetOrigin( m_pos );
    box.Inflate( delta );

    return box;
}


void SCH_NO_CONNECT::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount = 2;
    aLayers[0] = LAYER_NOCONNECT;
    aLayers[1] = LAYER_SELECTION_SHADOWS;
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


void SCH_NO_CONNECT::Print( const RENDER_SETTINGS* aSettings, const wxPoint& aOffset )
{
    wxDC*   DC = aSettings->GetPrintDC();
    int     half = GetSize() / 2;
    int     penWidth = std::max( GetPenWidth(), aSettings->GetDefaultPenWidth() );
    int     pX = m_pos.x + aOffset.x;
    int     pY = m_pos.y + aOffset.y;
    COLOR4D color = aSettings->GetLayerColor( LAYER_NOCONNECT );

    GRLine( nullptr, DC, pX - half, pY - half, pX + half, pY + half, penWidth, color );
    GRLine( nullptr, DC, pX + half, pY - half, pX - half, pY + half, penWidth, color );
}


void SCH_NO_CONNECT::MirrorVertically( int aCenter )
{
    MIRROR( m_pos.y, aCenter );
}


void SCH_NO_CONNECT::MirrorHorizontally( int aCenter )
{
    MIRROR( m_pos.x, aCenter );
}


void SCH_NO_CONNECT::Rotate( wxPoint aPosition )
{
    RotatePoint( &m_pos, aPosition, 900 );
}


std::vector<wxPoint> SCH_NO_CONNECT::GetConnectionPoints() const
{
    return { m_pos };
}


bool SCH_NO_CONNECT::doIsConnected( const wxPoint& aPosition ) const
{
    return m_pos == aPosition;
}

bool SCH_NO_CONNECT::HitTest( const wxPoint& aPosition, int aAccuracy ) const
{
    int delta = ( GetPenWidth() + GetSize() ) / 2 + aAccuracy;

    wxPoint dist = aPosition - m_pos;

    if( ( std::abs( dist.x ) <= delta ) && ( std::abs( dist.y ) <= delta ) )
        return true;

    return false;
}


bool SCH_NO_CONNECT::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    EDA_RECT rect = aRect;

    rect.Inflate( aAccuracy );

    if( aContained )
        return rect.Contains( GetBoundingBox() );

    return rect.Intersects( GetBoundingBox() );
}


void SCH_NO_CONNECT::Plot( PLOTTER* aPlotter )
{
    int delta = GetSize() / 2;
    int pX = m_pos.x;
    int pY = m_pos.y;
    int penWidth = std::max( GetPenWidth(), aPlotter->RenderSettings()->GetMinPenWidth() );

    aPlotter->SetCurrentLineWidth( penWidth );
    aPlotter->SetColor( aPlotter->RenderSettings()->GetLayerColor( LAYER_NOCONNECT ) );
    aPlotter->MoveTo( wxPoint( pX - delta, pY - delta ) );
    aPlotter->FinishTo( wxPoint( pX + delta, pY + delta ) );
    aPlotter->MoveTo( wxPoint( pX + delta, pY - delta ) );
    aPlotter->FinishTo( wxPoint( pX - delta, pY + delta ) );
}


BITMAP_DEF SCH_NO_CONNECT::GetMenuImage() const
{
    return noconn_xpm;
}
