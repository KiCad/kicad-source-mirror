/************************/
/* Class SCH_NO_CONNECT */
/************************/

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
        aErrorMsg.Printf( wxT( "EESchema file No Connect load error at line %d" ),
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
