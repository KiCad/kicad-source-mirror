/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2004-2011 KiCad Developers, see change_log.txt for contributors.
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
 * @file sch_bus_entry.cpp
 *
 */

#include <fctsys.h>
#include <gr_basic.h>
#include <macros.h>
#include <class_drawpanel.h>
#include <trigo.h>
#include <common.h>
#include <richio.h>
#include <plot_common.h>

#include <eeschema_config.h>
#include <general.h>
#include <protos.h>
#include <sch_bus_entry.h>


SCH_BUS_ENTRY::SCH_BUS_ENTRY( const wxPoint& pos, int shape, int id ) :
    SCH_ITEM( NULL, SCH_BUS_ENTRY_T )
{
    m_pos    = pos;
    m_size.x = 100;
    m_size.y = 100;
    m_Layer  = LAYER_WIRE;
    m_width  = 0;

    if( id == BUS_TO_BUS )
    {
        m_Layer = LAYER_BUS;
    }

    if( shape == '/' )
        m_size.y = -100;
}


EDA_ITEM* SCH_BUS_ENTRY::Clone() const
{
    return new SCH_BUS_ENTRY( *this );
}


wxPoint SCH_BUS_ENTRY::m_End() const
{
    return wxPoint( m_pos.x + m_size.x, m_pos.y + m_size.y );
}


void SCH_BUS_ENTRY::SwapData( SCH_ITEM* aItem )
{
    wxCHECK_RET( (aItem != NULL) && (aItem->Type() == SCH_BUS_ENTRY_T),
                 wxT( "Cannot swap bus entry data with invalid item." ) );

    SCH_BUS_ENTRY* item = (SCH_BUS_ENTRY*)aItem;
    EXCHG( m_pos, item->m_pos );
    EXCHG( m_size, item->m_size );
    EXCHG( m_width, item->m_width );
}


bool SCH_BUS_ENTRY::Save( FILE* aFile ) const
{
    bool        success = true;

    const char* layer = "Wire";
    const char* width = "Line";

    if( GetLayer() == LAYER_BUS )
    {
        layer = "Bus"; width = "Bus";
    }

    if( fprintf( aFile, "Entry %s %s\n", layer, width ) == EOF )
    {
        success = false;
    }
    if( fprintf( aFile, "\t%-4d %-4d %-4d %-4d\n",
                 m_pos.x, m_pos.y, m_End().x, m_End().y ) == EOF )
    {
        success = false;
    }

    return success;
}


bool SCH_BUS_ENTRY::Load( LINE_READER& aLine, wxString& aErrorMsg )
{
    char Name1[256];
    char Name2[256];
    char* line = (char*) aLine;

    while( (*line != ' ' ) && *line )
        line++;

    if( sscanf( line, "%s %s", Name1, Name2 ) != 2  )
    {
        aErrorMsg.Printf( wxT( "Eeschema file bus entry load error at line %d" ),
                          aLine.LineNumber() );
        aErrorMsg << wxT( "\n" ) << FROM_UTF8( (char*) aLine );
        return false;
    }

    m_Layer = LAYER_WIRE;

    if( Name1[0] == 'B' )
        m_Layer = LAYER_BUS;

    if( !aLine.ReadLine() || sscanf( (char*) aLine, "%d %d %d %d ", &m_pos.x, &m_pos.y,
                                      &m_size.x, &m_size.y ) != 4 )
    {
        aErrorMsg.Printf( wxT( "Eeschema file bus entry load error at line %d" ),
                          aLine.LineNumber() );
        aErrorMsg << wxT( "\n" ) << FROM_UTF8( (char*) aLine );
        return false;
    }

    m_size.x -= m_pos.x;
    m_size.y -= m_pos.y;

    return true;
}


EDA_RECT SCH_BUS_ENTRY::GetBoundingBox() const
{
    EDA_RECT box;

    box.SetOrigin( m_pos );
    box.SetEnd( m_End() );

    box.Normalize();
    int width = ( m_width == 0 ) ? GetDefaultLineThickness() : m_width;
    box.Inflate( width / 2 );

    return box;
}


int SCH_BUS_ENTRY::GetPenSize() const
{
    int pensize = ( m_width == 0 ) ? GetDefaultLineThickness() : m_width;

    if( m_Layer == LAYER_BUS )
    {
        pensize = ( m_width == 0 ) ? GetDefaultBusThickness() : m_width;
    }

    return pensize;
}


void SCH_BUS_ENTRY::Draw( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aOffset,
                          GR_DRAWMODE aDrawMode, EDA_COLOR_T aColor )
{
    EDA_COLOR_T color;

    if( aColor >= 0 )
        color = aColor;
    else
        color = ReturnLayerColor( m_Layer );

    GRSetDrawMode( aDC, aDrawMode );

    GRLine( aPanel->GetClipBox(), aDC, m_pos.x + aOffset.x, m_pos.y + aOffset.y,
            m_End().x + aOffset.x, m_End().y + aOffset.y, GetPenSize(), color );
}


void SCH_BUS_ENTRY::MirrorX( int aXaxis_position )
{
    m_pos.y -= aXaxis_position;
    NEGATE(  m_pos.y );
    m_pos.y += aXaxis_position;
    NEGATE(  m_size.y );
}


void SCH_BUS_ENTRY::MirrorY( int aYaxis_position )
{
    m_pos.x -= aYaxis_position;
    NEGATE(  m_pos.x );
    m_pos.x += aYaxis_position;
    NEGATE(  m_size.x );
}


void SCH_BUS_ENTRY::Rotate( wxPoint aPosition )
{
    RotatePoint( &m_pos, aPosition, 900 );
    RotatePoint( &m_size.x, &m_size.y, 900 );
}


void SCH_BUS_ENTRY::GetEndPoints( std::vector< DANGLING_END_ITEM >& aItemList )
{
    DANGLING_END_ITEM item( ENTRY_END, this, m_pos );
    aItemList.push_back( item );

    DANGLING_END_ITEM item1( ENTRY_END, this, m_End() );
    aItemList.push_back( item1 );
}


bool SCH_BUS_ENTRY::IsSelectStateChanged( const wxRect& aRect )
{
    bool previousState = IsSelected();

    // If either end of the bus entry is inside the selection rectangle, the entire
    // bus entry is selected.  Bus entries have a fixed length and angle.
    if( aRect.Contains( m_pos ) || aRect.Contains( m_End() ) )
        m_Flags |= SELECTED;
    else
        m_Flags &= ~SELECTED;

    return previousState != IsSelected();
}


void SCH_BUS_ENTRY::GetConnectionPoints( vector< wxPoint >& aPoints ) const
{
    aPoints.push_back( m_pos );
    aPoints.push_back( m_End() );
}


wxString SCH_BUS_ENTRY::GetSelectMenuText() const
{
    if( m_Layer == LAYER_WIRE )
        return wxString( _( "Bus to Wire Entry" ) );

    return wxString( _( "Bus to Bus Entry" ) );
}


bool SCH_BUS_ENTRY::HitTest( const wxPoint& aPosition, int aAccuracy ) const
{
    return TestSegmentHit( aPosition, m_pos, m_End(), aAccuracy );
}


bool SCH_BUS_ENTRY::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    EDA_RECT rect = aRect;

    rect.Inflate( aAccuracy );

    if( aContained )
        return rect.Contains( GetBoundingBox() );

    return rect.Intersects( GetBoundingBox() );
}


void SCH_BUS_ENTRY::Plot( PLOTTER* aPlotter )
{
    aPlotter->SetCurrentLineWidth( GetPenSize() );
    aPlotter->SetColor( ReturnLayerColor( GetLayer() ) );
    aPlotter->MoveTo( m_pos );
    aPlotter->FinishTo( m_End() );
}

/* SetBusEntryShape:
 * Set the shape of the bus entry.
 * aShape = ascii code '/' or '\'
 */
void SCH_BUS_ENTRY::SetBusEntryShape( int aShape )
{
    switch( aShape )
    {
    case '\\':
        if( m_size.y < 0 )
            m_size.y = -m_size.y;
        break;

    case '/':
        if( m_size.y > 0 )
            m_size.y = -m_size.y;
        break;
    }
}


/* GetBusEntryShape:
 * return the shape of the bus entry, as an ascii code '/' or '\'
 */
int SCH_BUS_ENTRY::GetBusEntryShape() const
{
    int shape = '\\';

    if( GetSize().y < 0 )
        shape = '/';

    return shape;
}
