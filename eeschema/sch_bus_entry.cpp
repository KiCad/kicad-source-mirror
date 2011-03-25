/***********************/
/* class SCH_BUS_ENTRY */
/***********************/

#include "fctsys.h"
#include "gr_basic.h"
#include "macros.h"
#include "class_drawpanel.h"
#include "trigo.h"
#include "common.h"
#include "richio.h"

#include "general.h"
#include "protos.h"
#include "sch_bus_entry.h"


SCH_BUS_ENTRY::SCH_BUS_ENTRY( const wxPoint& pos, int shape, int id ) :
    SCH_ITEM( NULL, SCH_BUS_ENTRY_T )
{
    m_Pos    = pos;
    m_Size.x = 100;
    m_Size.y = 100;
    m_Layer  = LAYER_WIRE;
    m_Width  = 0;

    if( id == BUS_TO_BUS )
    {
        m_Layer = LAYER_BUS;
    }

    if( shape == '/' )
        m_Size.y = -100;
}


SCH_BUS_ENTRY::SCH_BUS_ENTRY( const SCH_BUS_ENTRY& aBusEntry ) :
    SCH_ITEM( aBusEntry )
{
    m_Pos = aBusEntry.m_Pos;
    m_Size = aBusEntry.m_Size;
    m_Width = aBusEntry.m_Width;
}


EDA_ITEM* SCH_BUS_ENTRY::doClone() const
{
    return new SCH_BUS_ENTRY( *this );
}


wxPoint SCH_BUS_ENTRY::m_End() const
{
    return wxPoint( m_Pos.x + m_Size.x, m_Pos.y + m_Size.y );
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
                 m_Pos.x, m_Pos.y, m_End().x, m_End().y ) == EOF )
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
        aErrorMsg.Printf( wxT( "EESchema file bus entry load error at line %d" ),
                          aLine.LineNumber() );
        aErrorMsg << wxT( "\n" ) << FROM_UTF8( (char*) aLine );
        return false;
    }

    m_Layer = LAYER_WIRE;

    if( Name1[0] == 'B' )
        m_Layer = LAYER_BUS;

    if( !aLine.ReadLine() || sscanf( (char*) aLine, "%d %d %d %d ", &m_Pos.x, &m_Pos.y,
                                      &m_Size.x, &m_Size.y ) != 4 )
    {
        aErrorMsg.Printf( wxT( "EESchema file bus entry load error at line %d" ),
                          aLine.LineNumber() );
        aErrorMsg << wxT( "\n" ) << FROM_UTF8( (char*) aLine );
        return false;
    }

    m_Size.x -= m_Pos.x;
    m_Size.y -= m_Pos.y;

    return true;
}


EDA_Rect SCH_BUS_ENTRY::GetBoundingBox() const
{
    EDA_Rect box;

    box.SetOrigin( m_Pos );
    box.SetEnd( m_End() );

    box.Normalize();
    int width = ( m_Width == 0 ) ? g_DrawDefaultLineThickness : m_Width;
    box.Inflate( width / 2 );

    return box;
}


int SCH_BUS_ENTRY::GetPenSize() const
{
    int pensize = ( m_Width == 0 ) ? g_DrawDefaultLineThickness : m_Width;

    if( m_Layer == LAYER_BUS && m_Width == 0 )
    {
        pensize = wxRound( g_DrawDefaultLineThickness * BUS_WIDTH_EXPAND );
        pensize = MAX( pensize, 3 );
    }

    return pensize;
}


void SCH_BUS_ENTRY::Draw( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aOffset,
                          int aDrawMode, int aColor )
{
    int color;

    if( aColor >= 0 )
        color = aColor;
    else
        color = ReturnLayerColor( m_Layer );

    GRSetDrawMode( aDC, aDrawMode );

    GRLine( &aPanel->m_ClipBox, aDC, m_Pos.x + aOffset.x, m_Pos.y + aOffset.y,
            m_End().x + aOffset.x, m_End().y + aOffset.y, GetPenSize(), color );
}


void SCH_BUS_ENTRY::Mirror_X( int aXaxis_position )
{
    m_Pos.y -= aXaxis_position;
    NEGATE(  m_Pos.y );
    m_Pos.y += aXaxis_position;
    NEGATE(  m_Size.y );
}


void SCH_BUS_ENTRY::Mirror_Y( int aYaxis_position )
{
    m_Pos.x -= aYaxis_position;
    NEGATE(  m_Pos.x );
    m_Pos.x += aYaxis_position;
    NEGATE(  m_Size.x );
}


void SCH_BUS_ENTRY::Rotate( wxPoint rotationPoint )
{
    RotatePoint( &m_Pos, rotationPoint, 900 );
    RotatePoint( &m_Size.x, &m_Size.y, 900 );
}


void SCH_BUS_ENTRY::GetEndPoints( std::vector< DANGLING_END_ITEM >& aItemList )
{
    DANGLING_END_ITEM item( ENTRY_END, this );
    item.m_Pos = m_Pos;

    DANGLING_END_ITEM item1( ENTRY_END, this );
    item1.m_Pos = m_End();
    aItemList.push_back( item );
    aItemList.push_back( item1 );
}


bool SCH_BUS_ENTRY::IsSelectStateChanged( const wxRect& aRect )
{
    bool previousState = IsSelected();

    // If either end of the bus entry is inside the selection rectangle, the entire
    // bus entry is selected.  Bus entries have a fixed length and angle.
    if( aRect.Contains( m_Pos ) || aRect.Contains( m_End() ) )
        m_Flags |= SELECTED;
    else
        m_Flags &= ~SELECTED;

    return previousState != IsSelected();
}


void SCH_BUS_ENTRY::GetConnectionPoints( vector< wxPoint >& aPoints ) const
{
    aPoints.push_back( m_Pos );
    aPoints.push_back( m_End() );
}


wxString SCH_BUS_ENTRY::GetSelectMenuText() const
{
    if( m_Layer == LAYER_WIRE )
        return wxString( _( "Bus to Wire Entry" ) );

    return wxString( _( "Bus to Bus Entry" ) );
}


bool SCH_BUS_ENTRY::doHitTest( const wxPoint& aPoint, int aAccuracy ) const
{
    return TestSegmentHit( aPoint, m_Pos, m_End(), aAccuracy );
}


bool SCH_BUS_ENTRY::doHitTest( const EDA_Rect& aRect, bool aContained, int aAccuracy ) const
{
    EDA_Rect rect = aRect;

    rect.Inflate( aAccuracy );

    if( aContained )
        return rect.Contains( GetBoundingBox() );

    return rect.Intersects( GetBoundingBox() );
}
