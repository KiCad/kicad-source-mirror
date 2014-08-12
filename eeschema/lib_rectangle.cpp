/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2004-2012 KiCad Developers, see change_log.txt for contributors.
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
 * @file lib_rectangle.cpp
 */

#include <fctsys.h>
#include <gr_basic.h>
#include <macros.h>
#include <class_drawpanel.h>
#include <plot_common.h>
#include <trigo.h>
#include <wxstruct.h>
#include <richio.h>
#include <base_units.h>
#include <msgpanel.h>

#include <general.h>
#include <lib_rectangle.h>
#include <transform.h>


LIB_RECTANGLE::LIB_RECTANGLE( LIB_COMPONENT* aParent ) :
    LIB_ITEM( LIB_RECTANGLE_T, aParent )
{
    m_Width                = 0;
    m_Fill                 = NO_FILL;
    m_isFillable           = true;
    m_typeName             = _( "Rectangle" );
    m_isHeightLocked       = false;
    m_isWidthLocked        = false;
    m_isStartPointSelected = false;
}


bool LIB_RECTANGLE::Save( OUTPUTFORMATTER& aFormatter )
{
    aFormatter.Print( 0, "S %d %d %d %d %d %d %d %c\n", m_Pos.x, m_Pos.y,
                      m_End.x, m_End.y, m_Unit, m_Convert, m_Width, fill_tab[m_Fill] );

    return true;
}


bool LIB_RECTANGLE::Load( LINE_READER& aLineReader, wxString& aErrorMsg )
{
    int  cnt;
    char tmp[256] = "";
    char* line = (char*)aLineReader;

    cnt = sscanf( line + 2, "%d %d %d %d %d %d %d %255s", &m_Pos.x, &m_Pos.y,
                  &m_End.x, &m_End.y, &m_Unit, &m_Convert, &m_Width, tmp );

    if( cnt < 7 )
    {
        aErrorMsg.Printf( _( "Rectangle only had %d parameters of the required 7" ), cnt );
        return false;
    }

    if( tmp[0] == 'F' )
        m_Fill = FILLED_SHAPE;

    if( tmp[0] == 'f' )
        m_Fill = FILLED_WITH_BG_BODYCOLOR;

    return true;
}


EDA_ITEM* LIB_RECTANGLE::Clone() const
{
    return new LIB_RECTANGLE( *this );
}


int LIB_RECTANGLE::compare( const LIB_ITEM& aOther ) const
{
    wxASSERT( aOther.Type() == LIB_RECTANGLE_T );

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


void LIB_RECTANGLE::SetOffset( const wxPoint& aOffset )
{
    m_Pos += aOffset;
    m_End += aOffset;
}


bool LIB_RECTANGLE::Inside( EDA_RECT& aRect ) const
{
    return aRect.Contains( m_Pos.x, -m_Pos.y ) || aRect.Contains( m_End.x, -m_End.y );
}


void LIB_RECTANGLE::Move( const wxPoint& aPosition )
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
                          const TRANSFORM& aTransform )
{
    wxASSERT( aPlotter != NULL );

    wxPoint pos = aTransform.TransformCoordinate( m_Pos ) + aOffset;
    wxPoint end = aTransform.TransformCoordinate( m_End ) + aOffset;

    if( aFill && m_Fill == FILLED_WITH_BG_BODYCOLOR )
    {
        aPlotter->SetColor( GetLayerColor( LAYER_DEVICE_BACKGROUND ) );
        aPlotter->Rect( pos, end, FILLED_WITH_BG_BODYCOLOR, 0 );
    }

    bool already_filled = m_Fill == FILLED_WITH_BG_BODYCOLOR;
    aPlotter->SetColor( GetLayerColor( LAYER_DEVICE ) );
    aPlotter->Rect( pos, end, already_filled ? NO_FILL : m_Fill, GetPenSize() );
}


int LIB_RECTANGLE::GetPenSize() const
{
    return ( m_Width == 0 ) ? GetDefaultLineThickness() : m_Width;
}


void LIB_RECTANGLE::drawGraphic( EDA_DRAW_PANEL* aPanel, wxDC* aDC,
                                 const wxPoint& aOffset, EDA_COLOR_T aColor, GR_DRAWMODE aDrawMode,
                                 void* aData, const TRANSFORM& aTransform )
{
    wxPoint pos1, pos2;

    EDA_COLOR_T color = GetLayerColor( LAYER_DEVICE );

    if( aColor < 0 )       // Used normal color or selected color
    {
        if( IsSelected() )
            color = GetItemSelectedColor();
    }
    else
    {
        color = aColor;
    }

    pos1 = aTransform.TransformCoordinate( m_Pos ) + aOffset;
    pos2 = aTransform.TransformCoordinate( m_End ) + aOffset;

    FILL_T fill = aData ? NO_FILL : m_Fill;

    if( aColor >= 0 )
        fill = NO_FILL;

    GRSetDrawMode( aDC, aDrawMode );

    EDA_RECT* const clipbox  = aPanel? aPanel->GetClipBox() : NULL;
    if( fill == FILLED_WITH_BG_BODYCOLOR && !aData )
        GRFilledRect( clipbox, aDC, pos1.x, pos1.y, pos2.x, pos2.y, GetPenSize( ),
                      (m_Flags & IS_MOVED) ? color : GetLayerColor( LAYER_DEVICE_BACKGROUND ),
                      GetLayerColor( LAYER_DEVICE_BACKGROUND ) );
    else if( m_Fill == FILLED_SHAPE  && !aData )
        GRFilledRect( clipbox, aDC, pos1.x, pos1.y, pos2.x, pos2.y,
                      GetPenSize(), color, color );
    else
        GRRect( clipbox, aDC, pos1.x, pos1.y, pos2.x, pos2.y, GetPenSize(), color );

    /* Set to one (1) to draw bounding box around rectangle to validate
     * bounding box calculation. */
#if 0
    EDA_RECT bBox = GetBoundingBox();
    bBox.Inflate( m_Thickness + 1, m_Thickness + 1 );
    GRRect( clipbox, aDC, bBox.GetOrigin().x, bBox.GetOrigin().y,
            bBox.GetEnd().x, bBox.GetEnd().y, 0, LIGHTMAGENTA );
#endif
}


void LIB_RECTANGLE::GetMsgPanelInfo( MSG_PANEL_ITEMS& aList )
{
    wxString msg;

    LIB_ITEM::GetMsgPanelInfo( aList );

    msg = StringFromValue( g_UserUnit, m_Width, true );

    aList.push_back( MSG_PANEL_ITEM( _( "Line width" ), msg, BLUE ) );
}


const EDA_RECT LIB_RECTANGLE::GetBoundingBox() const
{
    EDA_RECT rect;

    rect.SetOrigin( m_Pos.x, m_Pos.y * -1 );
    rect.SetEnd( m_End.x, m_End.y * -1 );
    rect.Inflate( (GetPenSize() / 2) + 1 );
    return rect;
}


bool LIB_RECTANGLE::HitTest( const wxPoint& aPosition ) const
{
    int mindist = ( GetPenSize() / 2 ) + 1;

    // Have a minimal tolerance for hit test
    if( mindist < MINIMUM_SELECTION_DISTANCE )
        mindist = MINIMUM_SELECTION_DISTANCE;

    return HitTest( aPosition, mindist, DefaultTransform );
}


bool LIB_RECTANGLE::HitTest( const wxPoint &aPosition, int aThreshold, const TRANSFORM& aTransform ) const
{
    if( aThreshold < 0 )
        aThreshold = GetPenSize() / 2;

    wxPoint actualStart = aTransform.TransformCoordinate( m_Pos );
    wxPoint actualEnd   = aTransform.TransformCoordinate( m_End );

    // locate lower segment
    wxPoint start, end;

    start = actualStart;
    end.x = actualEnd.x;
    end.y = actualStart.y;

    if( TestSegmentHit( aPosition, start, end, aThreshold ) )
        return true;

    // locate right segment
    start.x = actualEnd.x;
    end.y   = actualEnd.y;

    if( TestSegmentHit( aPosition, start, end, aThreshold ) )
        return true;

    // locate upper segment
    start.y = actualEnd.y;
    end.x   = actualStart.x;

    if( TestSegmentHit( aPosition, start, end, aThreshold ) )
        return true;

    // locate left segment
    start = actualStart;
    end.x = actualStart.x;
    end.y = actualEnd.y;

    if( TestSegmentHit( aPosition, start, end, aThreshold ) )
        return true;

    return false;
}


wxString LIB_RECTANGLE::GetSelectMenuText() const
{
    return wxString::Format( _( "Rectangle from (%s, %s) to (%s, %s)" ),
                             GetChars( CoordinateToString( m_Pos.x ) ),
                             GetChars( CoordinateToString( m_Pos.y ) ),
                             GetChars( CoordinateToString( m_End.x ) ),
                             GetChars( CoordinateToString( m_End.y ) ) );
}


void LIB_RECTANGLE::BeginEdit( STATUS_FLAGS aEditMode, const wxPoint aPosition )
{
    wxCHECK_RET( ( aEditMode & ( IS_NEW | IS_MOVED | IS_RESIZED ) ) != 0,
                 wxT( "Invalid edit mode for LIB_RECTANGLE object." ) );

    if( aEditMode == IS_NEW )
    {
        m_Pos = m_End = aPosition;
    }
    else if( aEditMode == IS_RESIZED )
    {
        m_isStartPointSelected = abs( m_Pos.x - aPosition.x ) < MINIMUM_SELECTION_DISTANCE
            || abs( m_Pos.y - aPosition.y ) < MINIMUM_SELECTION_DISTANCE;

        if( m_isStartPointSelected )
        {
            m_isWidthLocked = abs( m_Pos.x - aPosition.x ) >= MINIMUM_SELECTION_DISTANCE;
            m_isHeightLocked = abs( m_Pos.y - aPosition.y ) >= MINIMUM_SELECTION_DISTANCE;
        }
        else
        {
            m_isWidthLocked = abs( m_End.x - aPosition.x ) >= MINIMUM_SELECTION_DISTANCE;
            m_isHeightLocked = abs( m_End.y - aPosition.y ) >= MINIMUM_SELECTION_DISTANCE;
        }

        SetEraseLastDrawItem();
    }
    else if( aEditMode == IS_MOVED )
    {
        m_initialPos = m_Pos;
        m_initialCursorPos = aPosition;
        SetEraseLastDrawItem();
    }

    m_Flags = aEditMode;
}


bool LIB_RECTANGLE::ContinueEdit( const wxPoint aPosition )
{
    wxCHECK_MSG( ( m_Flags & ( IS_NEW | IS_MOVED | IS_RESIZED ) ) != 0, false,
                   wxT( "Bad call to ContinueEdit().  LIB_RECTANGLE is not being edited." ) );

    return false;
}


void LIB_RECTANGLE::EndEdit( const wxPoint& aPosition, bool aAbort )
{
    wxCHECK_RET( ( m_Flags & ( IS_NEW | IS_MOVED | IS_RESIZED ) ) != 0,
                   wxT( "Bad call to EndEdit().  LIB_RECTANGLE is not being edited." ) );

    m_Flags = 0;
    m_isHeightLocked = false;
    m_isWidthLocked  = false;
    SetEraseLastDrawItem( false );
}


void LIB_RECTANGLE::calcEdit( const wxPoint& aPosition )
{
    if( m_Flags == IS_NEW )
    {
        m_End = aPosition;
        SetEraseLastDrawItem();
    }
    else if( m_Flags == IS_RESIZED )
    {
        if( m_isHeightLocked )
        {
            if( m_isStartPointSelected )
                m_Pos.x = aPosition.x;
            else
                m_End.x = aPosition.x;
        }
        else if( m_isWidthLocked )
        {
            if( m_isStartPointSelected )
                m_Pos.y = aPosition.y;
            else
                m_End.y = aPosition.y;
        }
        else
        {
            if( m_isStartPointSelected )
                m_Pos = aPosition;
            else
                m_End = aPosition;
        }
    }
    else if( m_Flags == IS_MOVED )
    {
        Move( m_initialPos + aPosition - m_initialCursorPos );
    }
}
