/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2004-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <eda_draw_frame.h>
#include <general.h>
#include <lib_rectangle.h>
#include <settings/color_settings.h>
#include <transform.h>


LIB_RECTANGLE::LIB_RECTANGLE( LIB_SYMBOL* aParent ) : LIB_ITEM( LIB_RECTANGLE_T, aParent )
{
    m_Width      = 0;
    m_fill       = FILL_TYPE::NO_FILL;
    m_isFillable = true;
}


EDA_ITEM* LIB_RECTANGLE::Clone() const
{
    return new LIB_RECTANGLE( *this );
}


int LIB_RECTANGLE::compare( const LIB_ITEM& aOther, LIB_ITEM::COMPARE_FLAGS aCompareFlags ) const
{
    wxASSERT( aOther.Type() == LIB_RECTANGLE_T );

    int retv = LIB_ITEM::compare( aOther );

    if( retv )
        return retv;

    const LIB_RECTANGLE* tmp = ( LIB_RECTANGLE* ) &aOther;

    if( m_Pos.x != tmp->m_Pos.x )
        return m_Pos.x - tmp->m_Pos.x;

    if( m_Pos.y != tmp->m_Pos.y )
        return m_Pos.y - tmp->m_Pos.y;

    if( m_End.x != tmp->m_End.x )
        return m_End.x - tmp->m_End.x;

    if( m_End.y != tmp->m_End.y )
        return m_End.y - tmp->m_End.y;

    return 0;
}


void LIB_RECTANGLE::Offset( const wxPoint& aOffset )
{
    m_Pos += aOffset;
    m_End += aOffset;
}


void LIB_RECTANGLE::MoveTo( const wxPoint& aPosition )
{
    wxPoint size = m_End - m_Pos;
    m_Pos = aPosition;
    m_End = aPosition + size;
}


void LIB_RECTANGLE::MirrorHorizontal( const wxPoint& aCenter )
{
    m_Pos.x -= aCenter.x;
    m_Pos.x *= -1;
    m_Pos.x += aCenter.x;
    m_End.x -= aCenter.x;
    m_End.x *= -1;
    m_End.x += aCenter.x;
}


void LIB_RECTANGLE::MirrorVertical( const wxPoint& aCenter )
{
    m_Pos.y -= aCenter.y;
    m_Pos.y *= -1;
    m_Pos.y += aCenter.y;
    m_End.y -= aCenter.y;
    m_End.y *= -1;
    m_End.y += aCenter.y;
}


void LIB_RECTANGLE::Rotate( const wxPoint& aCenter, bool aRotateCCW )
{
    int rot_angle = aRotateCCW ? -900 : 900;
    RotatePoint( &m_Pos, aCenter, rot_angle );
    RotatePoint( &m_End, aCenter, rot_angle );
}


void LIB_RECTANGLE::Plot( PLOTTER* aPlotter, const wxPoint& aOffset, bool aFill,
                          const TRANSFORM& aTransform ) const
{
    wxASSERT( aPlotter != nullptr );

    wxPoint pos = aTransform.TransformCoordinate( m_Pos ) + aOffset;
    wxPoint end = aTransform.TransformCoordinate( m_End ) + aOffset;

    if( aFill && m_fill == FILL_TYPE::FILLED_WITH_BG_BODYCOLOR )
    {
        aPlotter->SetColor( aPlotter->RenderSettings()->GetLayerColor( LAYER_DEVICE_BACKGROUND ) );
        aPlotter->Rect( pos, end, FILL_TYPE::FILLED_WITH_BG_BODYCOLOR, 0 );
    }

    bool already_filled = m_fill == FILL_TYPE::FILLED_WITH_BG_BODYCOLOR;
    int  pen_size = GetEffectivePenWidth( aPlotter->RenderSettings() );

    if( !already_filled || pen_size > 0 )
    {
        aPlotter->SetColor( aPlotter->RenderSettings()->GetLayerColor( LAYER_DEVICE ) );
        aPlotter->Rect( pos, end, already_filled ? FILL_TYPE::NO_FILL : m_fill, pen_size );
    }
}


int LIB_RECTANGLE::GetPenWidth() const
{
    return m_Width;
}


void LIB_RECTANGLE::print( const RENDER_SETTINGS* aSettings, const wxPoint& aOffset,
                           void* aData, const TRANSFORM& aTransform )
{
    bool forceNoFill = static_cast<bool>( aData );
    int  penWidth = GetEffectivePenWidth( aSettings );

    if( forceNoFill && m_fill != FILL_TYPE::NO_FILL && penWidth == 0 )
        return;

    wxDC*   DC     = aSettings->GetPrintDC();
    COLOR4D color  = aSettings->GetLayerColor( LAYER_DEVICE );
    wxPoint pt1    = aTransform.TransformCoordinate( m_Pos ) + aOffset;
    wxPoint pt2    = aTransform.TransformCoordinate( m_End ) + aOffset;

    if( forceNoFill || m_fill == FILL_TYPE::NO_FILL )
    {
        GRRect( nullptr, DC, pt1.x, pt1.y, pt2.x, pt2.y, penWidth, color );
    }
    else
    {
        if( m_fill == FILL_TYPE::FILLED_WITH_BG_BODYCOLOR )
            color = aSettings->GetLayerColor( LAYER_DEVICE_BACKGROUND );

        GRFilledRect( nullptr, DC, pt1.x, pt1.y, pt2.x, pt2.y, penWidth, color, color );
    }
}


void LIB_RECTANGLE::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, MSG_PANEL_ITEMS& aList )
{
    LIB_ITEM::GetMsgPanelInfo( aFrame, aList );

    wxString msg = MessageTextFromValue( aFrame->GetUserUnits(), m_Width );

    aList.push_back( MSG_PANEL_ITEM( _( "Line Width" ), msg ) );
}


const EDA_RECT LIB_RECTANGLE::GetBoundingBox() const
{
    EDA_RECT rect;

    rect.SetOrigin( m_Pos );
    rect.SetEnd( m_End );
    rect.Inflate( ( GetPenWidth() / 2 ) + 1 );

    rect.RevertYAxis();

    return rect;
}


bool LIB_RECTANGLE::HitTest( const wxPoint& aPosition, int aAccuracy ) const
{
    int     mindist = std::max( aAccuracy + GetPenWidth() / 2,
                                Mils2iu( MINIMUM_SELECTION_DISTANCE ) );
    wxPoint actualStart = DefaultTransform.TransformCoordinate( m_Pos );
    wxPoint actualEnd   = DefaultTransform.TransformCoordinate( m_End );

    // locate lower segment
    wxPoint start, end;

    start = actualStart;
    end.x = actualEnd.x;
    end.y = actualStart.y;

    if( TestSegmentHit( aPosition, start, end, mindist ) )
        return true;

    // locate right segment
    start.x = actualEnd.x;
    end.y   = actualEnd.y;

    if( TestSegmentHit( aPosition, start, end, mindist ) )
        return true;

    // locate upper segment
    start.y = actualEnd.y;
    end.x   = actualStart.x;

    if( TestSegmentHit( aPosition, start, end, mindist ) )
        return true;

    // locate left segment
    start = actualStart;
    end.x = actualStart.x;
    end.y = actualEnd.y;

    if( TestSegmentHit( aPosition, start, end, mindist ) )
        return true;

    if( m_fill == FILL_TYPE::NO_FILL )
        return false;

    return GetBoundingBox().Contains( aPosition );
}


bool LIB_RECTANGLE::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    if( m_flags & (STRUCT_DELETED | SKIP_STRUCT ) )
        return false;

    EDA_RECT sel = aRect;

    if ( aAccuracy )
        sel.Inflate( aAccuracy );

    if( aContained )
        return sel.Contains( GetBoundingBox() );

    return sel.Intersects( GetBoundingBox() );
}


wxString LIB_RECTANGLE::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    return wxString::Format( _( "Rectangle, width %s height %s" ),
                             MessageTextFromValue( aUnits, std::abs( m_Pos.x - m_End.x ) ),
                             MessageTextFromValue( aUnits, std::abs( m_Pos.y - m_End.y ) ) );
}


BITMAPS LIB_RECTANGLE::GetMenuImage() const
{
    return BITMAPS::add_rectangle;
}


void LIB_RECTANGLE::BeginEdit( const wxPoint& aPosition )
{
    m_Pos = m_End = aPosition;
}


void LIB_RECTANGLE::CalcEdit( const wxPoint& aPosition )
{
    m_End = aPosition;
}
