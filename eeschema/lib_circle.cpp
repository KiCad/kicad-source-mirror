/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2004-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <gr_basic.h>
#include <macros.h>
#include <sch_draw_panel.h>
#include <plotter.h>
#include <trigo.h>
#include <base_units.h>
#include <msgpanel.h>
#include <bitmaps.h>

#include <general.h>
#include <lib_circle.h>
#include <transform.h>


LIB_CIRCLE::LIB_CIRCLE( LIB_PART* aParent ) :
    LIB_ITEM( LIB_CIRCLE_T, aParent )
{
    m_Width      = 0;
    m_Fill       = NO_FILL;
    m_isFillable = true;
}


bool LIB_CIRCLE::HitTest( const wxPoint& aPosRef, int aAccuracy ) const
{
    int mindist = std::max( aAccuracy + GetPenSize() / 2, MINIMUM_SELECTION_DISTANCE );
    int dist = KiROUND( GetLineLength( aPosRef, DefaultTransform.TransformCoordinate( m_Pos ) ) );

    if( abs( dist - GetRadius() ) <= mindist )
        return true;

    return false;
}


bool LIB_CIRCLE::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    if( m_Flags & ( STRUCT_DELETED | SKIP_STRUCT ) )
        return false;

    wxPoint  center = DefaultTransform.TransformCoordinate( GetPosition() );
    int      radius = GetRadius();
    int      lineWidth = GetWidth();
    EDA_RECT sel = aRect ;

    if ( aAccuracy )
        sel.Inflate( aAccuracy );

    if( aContained )
        return sel.Contains( GetBoundingBox() );

    // If the rectangle does not intersect the bounding box, this is a much quicker test
    if( !sel.Intersects( GetBoundingBox() ) )
        return false;
    else
        return sel.IntersectsCircleEdge( center, radius, lineWidth );
}


EDA_ITEM* LIB_CIRCLE::Clone() const
{
    return new LIB_CIRCLE( *this );
}


int LIB_CIRCLE::compare( const LIB_ITEM& aOther ) const
{
    wxASSERT( aOther.Type() == LIB_CIRCLE_T );

    const LIB_CIRCLE* tmp = ( LIB_CIRCLE* ) &aOther;

    if( m_Pos.x != tmp->m_Pos.x )
        return m_Pos.x - tmp->m_Pos.x;

    if( m_Pos.y != tmp->m_Pos.y )
        return m_Pos.y - tmp->m_Pos.y;

    if( m_EndPos.x != tmp->m_EndPos.x )
        return m_EndPos.x - tmp->m_EndPos.x;

    if( m_EndPos.y != tmp->m_EndPos.y )
        return m_EndPos.y - tmp->m_EndPos.y;

    return 0;
}


void LIB_CIRCLE::Offset( const wxPoint& aOffset )
{
    m_Pos += aOffset;
    m_EndPos += aOffset;
}


bool LIB_CIRCLE::Inside( EDA_RECT& aRect ) const
{
    wxPoint center(m_Pos.x, -m_Pos.y);
    return aRect.IntersectsCircle( center, GetRadius() );
}


void LIB_CIRCLE::MoveTo( const wxPoint& aPosition )
{
    Offset( aPosition - m_Pos );
}


void LIB_CIRCLE::MirrorHorizontal( const wxPoint& aCenter )
{
    m_Pos.x -= aCenter.x;
    m_Pos.x *= -1;
    m_Pos.x += aCenter.x;
    m_EndPos.x -= aCenter.x;
    m_EndPos.x *= -1;
    m_EndPos.x += aCenter.x;
}


void LIB_CIRCLE::MirrorVertical( const wxPoint& aCenter )
{
    m_Pos.y -= aCenter.y;
    m_Pos.y *= -1;
    m_Pos.y += aCenter.y;
    m_EndPos.y -= aCenter.y;
    m_EndPos.y *= -1;
    m_EndPos.y += aCenter.y;
}


void LIB_CIRCLE::Rotate( const wxPoint& aCenter, bool aRotateCCW )
{
    int rot_angle = aRotateCCW ? -900 : 900;

    RotatePoint( &m_Pos, aCenter, rot_angle );
    RotatePoint( &m_EndPos, aCenter, rot_angle );
}


void LIB_CIRCLE::Plot( PLOTTER* aPlotter, const wxPoint& aOffset, bool aFill,
                       const TRANSFORM& aTransform )
{
    wxPoint pos = aTransform.TransformCoordinate( m_Pos ) + aOffset;

    if( aFill && m_Fill == FILLED_WITH_BG_BODYCOLOR )
    {
        aPlotter->SetColor( GetLayerColor( LAYER_DEVICE_BACKGROUND ) );
        aPlotter->Circle( pos, GetRadius() * 2, FILLED_WITH_BG_BODYCOLOR, 0 );
    }

    bool already_filled = m_Fill == FILLED_WITH_BG_BODYCOLOR;
    auto pen_size = GetPenSize();

    if( !already_filled || pen_size > 0 )
    {
        pen_size = std::max( 0, pen_size );
        aPlotter->SetColor( GetLayerColor( LAYER_DEVICE ) );
        aPlotter->Circle( pos, GetRadius() * 2, already_filled ? NO_FILL : m_Fill, pen_size );
    }
}


int LIB_CIRCLE::GetPenSize() const
{
    if( m_Width > 0 )
        return m_Width;

    if( m_Width == 0 )
       return GetDefaultLineThickness();

    return -1;   // a value to use a minimal pen size
}


void LIB_CIRCLE::print( wxDC* aDC, const wxPoint& aOffset, void* aData,
                        const TRANSFORM& aTransform )
{
    wxPoint pos1    = aTransform.TransformCoordinate( m_Pos ) + aOffset;
    COLOR4D color   = GetLayerColor( LAYER_DEVICE );
    COLOR4D bgColor = GetLayerColor( LAYER_DEVICE_BACKGROUND );
    FILL_T  fill    = aData ? NO_FILL : m_Fill;

    if( fill == FILLED_WITH_BG_BODYCOLOR )
        GRFilledCircle( nullptr, aDC, pos1.x, pos1.y, GetRadius(), GetPenSize(), bgColor, bgColor );
    else if( fill == FILLED_SHAPE )
        GRFilledCircle( nullptr, aDC, pos1.x, pos1.y, GetRadius(), 0, color, color );
    else
        GRCircle( nullptr, aDC, pos1.x, pos1.y, GetRadius(), GetPenSize(), color );
}


const EDA_RECT LIB_CIRCLE::GetBoundingBox() const
{
    EDA_RECT rect;
    int radius = GetRadius();

    rect.SetOrigin( m_Pos.x - radius, m_Pos.y - radius );
    rect.SetEnd( m_Pos.x + radius, m_Pos.y + radius );
    rect.Inflate( ( GetPenSize()+1 ) / 2 );

    rect.RevertYAxis();

    return rect;
}


void LIB_CIRCLE::GetMsgPanelInfo( EDA_UNITS_T aUnits, MSG_PANEL_ITEMS& aList )
{
    wxString msg;
    EDA_RECT bBox = GetBoundingBox();

    LIB_ITEM::GetMsgPanelInfo( aUnits, aList );

    msg = MessageTextFromValue( aUnits, m_Width, true );

    aList.push_back( MSG_PANEL_ITEM(  _( "Line Width" ), msg, BLUE ) );

    msg = MessageTextFromValue( aUnits, GetRadius(), true );
    aList.push_back( MSG_PANEL_ITEM( _( "Radius" ), msg, RED ) );

    msg.Printf( wxT( "(%d, %d, %d, %d)" ),
                bBox.GetOrigin().x,
                bBox.GetOrigin().y,
                bBox.GetEnd().x,
                bBox.GetEnd().y );

    aList.push_back( MSG_PANEL_ITEM( _( "Bounding Box" ), msg, BROWN ) );
}


wxString LIB_CIRCLE::GetSelectMenuText( EDA_UNITS_T aUnits ) const
{
    return wxString::Format( _( "Circle center (%s, %s), radius %s" ),
                             MessageTextFromValue( aUnits, m_Pos.x ),
                             MessageTextFromValue( aUnits, m_Pos.y ),
                             MessageTextFromValue( aUnits, GetRadius() ) );
}


BITMAP_DEF LIB_CIRCLE::GetMenuImage() const
{
    return add_circle_xpm;
}


void LIB_CIRCLE::BeginEdit( const wxPoint aPosition )
{
    m_Pos = aPosition;
}


void LIB_CIRCLE::CalcEdit( const wxPoint& aPosition )
{
    SetEnd( aPosition );
}
