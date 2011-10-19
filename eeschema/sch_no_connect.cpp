/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
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
 * @file sch_no_connect.cpp
 * @brief Class SCH_NO_CONNECT implementation.
 */

#include "fctsys.h"
#include "gr_basic.h"
#include "macros.h"
#include "class_drawpanel.h"
#include "common.h"
#include "trigo.h"
#include "richio.h"
#include "plot_common.h"

#include "general.h"
#include "protos.h"
#include "sch_no_connect.h"
#include "class_netlist_object.h"


SCH_NO_CONNECT::SCH_NO_CONNECT( const wxPoint& pos ) :
    SCH_ITEM( NULL, SCH_NO_CONNECT_T )
{
#define DRAWNOCONNECT_SIZE 48       /* No symbol connection range. */
    m_Pos    = pos;
    m_Size.x = m_Size.y = DRAWNOCONNECT_SIZE;
#undef DRAWNOCONNECT_SIZE

    SetLayer( LAYER_NOCONNECT );
}


SCH_NO_CONNECT::SCH_NO_CONNECT( const SCH_NO_CONNECT& aNoConnect ) :
    SCH_ITEM( aNoConnect )
{
    m_Pos = aNoConnect.m_Pos;
    m_Size = aNoConnect.m_Size;
}


EDA_ITEM* SCH_NO_CONNECT::doClone() const
{
    return new SCH_NO_CONNECT( *this );
}


void SCH_NO_CONNECT::SwapData( SCH_ITEM* aItem )
{
    wxCHECK_RET( (aItem != NULL) && (aItem->Type() == SCH_NO_CONNECT_T),
                 wxT( "Cannot swap no connect data with invalid item." ) );

    SCH_NO_CONNECT* item = (SCH_NO_CONNECT*)aItem;
    EXCHG( m_Pos, item->m_Pos );
    EXCHG( m_Size, item->m_Size );
}


EDA_RECT SCH_NO_CONNECT::GetBoundingBox() const
{
    int      delta = ( GetPenSize() + m_Size.x ) / 2;
    EDA_RECT box;

    box.SetOrigin( m_Pos );
    box.Inflate( delta );

    return box;
}


bool SCH_NO_CONNECT::Save( FILE* aFile ) const
{
    bool success = true;

    if( fprintf( aFile, "NoConn ~ %-4d %-4d\n", m_Pos.x, m_Pos.y ) == EOF )
    {
        success = false;
    }

    return success;
}


bool SCH_NO_CONNECT::Load( LINE_READER& aLine, wxString& aErrorMsg )
{
    char name[256];
    char* line = (char*) aLine;

    while( (*line != ' ' ) && *line )
        line++;

    if( sscanf( line, "%s %d %d", name, &m_Pos.x, &m_Pos.y ) != 3 )
    {
        aErrorMsg.Printf( wxT( "Eeschema file No Connect load error at line %d" ),
                          aLine.LineNumber() );
        aErrorMsg << wxT( "\n" ) << FROM_UTF8( ((char*)aLine) );
        return false;
    }

    return true;
}


int SCH_NO_CONNECT::GetPenSize() const
{
    return g_DrawDefaultLineThickness;
}


void SCH_NO_CONNECT::Draw( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aOffset,
                           int aDrawMode, int aColor )
{
    int pX, pY, color;
    int delta = m_Size.x / 2;
    int width = g_DrawDefaultLineThickness;

    pX = m_Pos.x + aOffset.x;
    pY = m_Pos.y + aOffset.y;

    if( aColor >= 0 )
        color = aColor;
    else
        color = ReturnLayerColor( LAYER_NOCONNECT );

    GRSetDrawMode( aDC, aDrawMode );

    GRLine( &aPanel->m_ClipBox, aDC, pX - delta, pY - delta, pX + delta, pY + delta, width, color );
    GRLine( &aPanel->m_ClipBox, aDC, pX + delta, pY - delta, pX - delta, pY + delta, width, color );
}


void SCH_NO_CONNECT::Mirror_X( int aXaxis_position )
{
    m_Pos.y -= aXaxis_position;
    NEGATE(  m_Pos.y );
    m_Pos.y += aXaxis_position;
}


void SCH_NO_CONNECT::Mirror_Y( int aYaxis_position )
{
    m_Pos.x -= aYaxis_position;
    NEGATE(  m_Pos.x );
    m_Pos.x += aYaxis_position;
}


void SCH_NO_CONNECT::Rotate( wxPoint rotationPoint )
{
    RotatePoint( &m_Pos, rotationPoint, 900 );
}


bool SCH_NO_CONNECT::IsSelectStateChanged( const wxRect& aRect )
{
    bool previousState = IsSelected();

    if( aRect.Contains( m_Pos ) )
        m_Flags |= SELECTED;
    else
        m_Flags &= ~SELECTED;

    return previousState != IsSelected();
}


void SCH_NO_CONNECT::GetConnectionPoints( vector< wxPoint >& aPoints ) const
{
    aPoints.push_back( m_Pos );
}


void SCH_NO_CONNECT::GetNetListItem( vector<NETLIST_OBJECT*>& aNetListItems,
                                     SCH_SHEET_PATH*          aSheetPath )
{
    NETLIST_OBJECT* item = new NETLIST_OBJECT();

    item->m_SheetList = *aSheetPath;
    item->m_SheetListInclude = *aSheetPath;
    item->m_Comp = this;
    item->m_Type = NET_NOCONNECT;
    item->m_Start = item->m_End = m_Pos;

    aNetListItems.push_back( item );
}


bool SCH_NO_CONNECT::doIsConnected( const wxPoint& aPosition ) const
{
    return m_Pos == aPosition;
}

bool SCH_NO_CONNECT::doHitTest( const wxPoint& aPoint, int aAccuracy ) const
{
    int delta = ( ( m_Size.x + g_DrawDefaultLineThickness ) / 2 ) + aAccuracy;

    wxPoint dist = aPoint - m_Pos;

    if( ( ABS( dist.x ) <= delta ) && ( ABS( dist.y ) <= delta ) )
        return true;

    return false;
}


bool SCH_NO_CONNECT::doHitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    EDA_RECT rect = aRect;

    rect.Inflate( aAccuracy );

    if( aContained )
        return rect.Contains( GetBoundingBox() );

    return rect.Intersects( GetBoundingBox() );
}


void SCH_NO_CONNECT::doPlot( PLOTTER* aPlotter )
{
    int delta = m_Size.x / 2;
    int pX, pY;

    pX = m_Pos.x;
    pY = m_Pos.y;

    aPlotter->set_current_line_width( GetPenSize() );
    aPlotter->set_color( ReturnLayerColor( GetLayer() ) );
    aPlotter->move_to( wxPoint( pX - delta, pY - delta ) );
    aPlotter->finish_to( wxPoint( pX + delta, pY + delta ) );
    aPlotter->move_to( wxPoint( pX + delta, pY - delta ) );
    aPlotter->finish_to( wxPoint( pX - delta, pY + delta ) );
}
