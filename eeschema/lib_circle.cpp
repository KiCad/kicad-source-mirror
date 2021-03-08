/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2004-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <sch_draw_panel.h>
#include <plotter.h>
#include <trigo.h>
#include <base_units.h>
#include <widgets/msgpanel.h>
#include <bitmaps.h>
#include <math/util.h>      // for KiROUND
#include <eda_draw_frame.h>
#include <general.h>
#include <lib_circle.h>
#include <settings/color_settings.h>
#include <transform.h>


LIB_CIRCLE::LIB_CIRCLE( LIB_PART* aParent ) :
    LIB_ITEM( LIB_CIRCLE_T, aParent )
{
    m_Width      = 0;
    m_fill       = FILL_TYPE::NO_FILL;
    m_isFillable = true;
}


bool LIB_CIRCLE::HitTest( const wxPoint& aPosRef, int aAccuracy ) const
{
    int mindist = std::max( aAccuracy + GetPenWidth() / 2,
                            Mils2iu( MINIMUM_SELECTION_DISTANCE ) );
    int dist = KiROUND( GetLineLength( aPosRef, DefaultTransform.TransformCoordinate( m_Pos ) ) );

    if( abs( dist - GetRadius() ) <= mindist )
        return true;

    if( m_fill == FILL_TYPE::NO_FILL )
        return false;

    return dist <= GetRadius();
}


bool LIB_CIRCLE::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    if( m_flags & (STRUCT_DELETED | SKIP_STRUCT ) )
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


int LIB_CIRCLE::compare( const LIB_ITEM& aOther, LIB_ITEM::COMPARE_FLAGS aCompareFlags ) const
{
    wxASSERT( aOther.Type() == LIB_CIRCLE_T );

    int retv = LIB_ITEM::compare( aOther, aCompareFlags );

    if( retv )
        return retv;

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
                       const TRANSFORM& aTransform ) const
{
    wxPoint pos = aTransform.TransformCoordinate( m_Pos ) + aOffset;

    if( aFill && m_fill == FILL_TYPE::FILLED_WITH_BG_BODYCOLOR )
    {
        aPlotter->SetColor( aPlotter->RenderSettings()->GetLayerColor( LAYER_DEVICE_BACKGROUND ) );
        aPlotter->Circle( pos, GetRadius() * 2, FILL_TYPE::FILLED_WITH_BG_BODYCOLOR, 0 );
    }

    bool already_filled = m_fill == FILL_TYPE::FILLED_WITH_BG_BODYCOLOR;
    int  pen_size = GetPenWidth();

    if( !already_filled || pen_size > 0 )
    {
        pen_size = std::max( pen_size, aPlotter->RenderSettings()->GetMinPenWidth() );

        aPlotter->SetColor( aPlotter->RenderSettings()->GetLayerColor( LAYER_DEVICE ) );
        aPlotter->Circle( pos, GetRadius() * 2, already_filled ? FILL_TYPE::NO_FILL : m_fill,
                          pen_size );
    }
}


int LIB_CIRCLE::GetPenWidth() const
{
    // Historically 0 meant "default width" and negative numbers meant "don't stroke".
    if( m_Width < 0 && GetFillMode() != FILL_TYPE::NO_FILL )
        return 0;
    else
        return std::max( m_Width, 1 );
}


void LIB_CIRCLE::print( const RENDER_SETTINGS* aSettings, const wxPoint& aOffset,
                        void* aData, const TRANSFORM& aTransform )
{
    bool forceNoFill = static_cast<bool>( aData );
    int  penWidth = GetPenWidth();

    if( forceNoFill && m_fill != FILL_TYPE::NO_FILL && penWidth == 0 )
        return;

    wxDC*   DC       = aSettings->GetPrintDC();
    wxPoint pos1     = aTransform.TransformCoordinate( m_Pos ) + aOffset;
    COLOR4D color    = aSettings->GetLayerColor( LAYER_DEVICE );

    if( forceNoFill || m_fill == FILL_TYPE::NO_FILL )
    {
        penWidth = std::max( penWidth, aSettings->GetDefaultPenWidth() );

        GRCircle( nullptr, DC, pos1.x, pos1.y, GetRadius(), penWidth, color );
    }
    else
    {
        if( m_fill == FILL_TYPE::FILLED_WITH_BG_BODYCOLOR )
            color = aSettings->GetLayerColor( LAYER_DEVICE_BACKGROUND );

        GRFilledCircle( nullptr, DC, pos1.x, pos1.y, GetRadius(), 0, color, color );
    }
}


const EDA_RECT LIB_CIRCLE::GetBoundingBox() const
{
    EDA_RECT rect;
    int radius = GetRadius();

    rect.SetOrigin( m_Pos.x - radius, m_Pos.y - radius );
    rect.SetEnd( m_Pos.x + radius, m_Pos.y + radius );
    rect.Inflate( ( GetPenWidth() / 2 ) + 1 );

    rect.RevertYAxis();

    return rect;
}


void LIB_CIRCLE::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, MSG_PANEL_ITEMS& aList )
{
    wxString msg;
    EDA_RECT bBox = GetBoundingBox();

    LIB_ITEM::GetMsgPanelInfo( aFrame, aList );

    msg = MessageTextFromValue( aFrame->GetUserUnits(), m_Width );

    aList.push_back( MSG_PANEL_ITEM(  _( "Line Width" ), msg ) );

    msg = MessageTextFromValue( aFrame->GetUserUnits(), GetRadius() );
    aList.push_back( MSG_PANEL_ITEM( _( "Radius" ), msg ) );

    msg.Printf( wxT( "(%d, %d, %d, %d)" ),
                bBox.GetOrigin().x,
                bBox.GetOrigin().y,
                bBox.GetEnd().x,
                bBox.GetEnd().y );

    aList.push_back( MSG_PANEL_ITEM( _( "Bounding Box" ), msg ) );
}


wxString LIB_CIRCLE::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    return wxString::Format( _( "Circle, radius %s" ),
                             MessageTextFromValue( aUnits, GetRadius() ) );
}


BITMAPS LIB_CIRCLE::GetMenuImage() const
{
    return BITMAPS::add_circle;
}


void LIB_CIRCLE::BeginEdit( const wxPoint aPosition )
{
    m_Pos = aPosition;
}


void LIB_CIRCLE::CalcEdit( const wxPoint& aPosition )
{
    SetEnd( aPosition );
}
