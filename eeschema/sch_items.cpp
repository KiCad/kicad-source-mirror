/*****************************/
/* class_schematic_items.cpp */
/*****************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "macros.h"
#include "class_drawpanel.h"
#include "trigo.h"
#include "common.h"
#include "richio.h"

#include "general.h"
#include "protos.h"
#include "sch_items.h"


/**********************/
/* class SCH_JUNCTION */
/**********************/

SCH_JUNCTION::SCH_JUNCTION( const wxPoint& pos ) :
    SCH_ITEM( NULL, SCH_JUNCTION_T )
{
#define DRAWJUNCTION_DIAMETER 32   /* Diameter of junction symbol between wires */
    m_Pos    = pos;
    m_Layer  = LAYER_JUNCTION;
    m_Size.x = m_Size.y = DRAWJUNCTION_DIAMETER;
#undef DRAWJUNCTION_DIAMETER
}


SCH_JUNCTION::SCH_JUNCTION( const SCH_JUNCTION& aJunction ) :
    SCH_ITEM( aJunction )
{
    m_Pos = aJunction.m_Pos;
    m_Size = aJunction.m_Size;
}


SCH_JUNCTION* SCH_JUNCTION::GenCopy()
{
    SCH_JUNCTION* newitem = new SCH_JUNCTION( m_Pos );

    newitem->m_Size  = m_Size;
    newitem->m_Layer = m_Layer;
    newitem->m_Flags = m_Flags;

    return newitem;
}


bool SCH_JUNCTION::Save( FILE* aFile ) const
{
    bool success = true;

    if( fprintf( aFile, "Connection ~ %-4d %-4d\n", m_Pos.x, m_Pos.y ) == EOF )
    {
        success = false;
    }

    return success;
}


EDA_ITEM* SCH_JUNCTION::doClone() const
{
    return new SCH_JUNCTION( *this );
}


bool SCH_JUNCTION::Load( LINE_READER& aLine, wxString& aErrorMsg )
{
    char name[256];
    char* line = (char*) aLine;

    while( (*line != ' ' ) && *line )
        line++;

    if( sscanf( line, "%s %d %d", name, &m_Pos.x, &m_Pos.y ) != 3 )
    {
        aErrorMsg.Printf( wxT( "EESchema file connection load error at line %d, aborted" ),
                          aLine.LineNumber() );
        aErrorMsg << wxT( "\n" ) << CONV_FROM_UTF8( (char*) aLine );
        return false;
    }

    return true;
}


EDA_Rect SCH_JUNCTION::GetBoundingBox() const
{
    EDA_Rect rect;

    rect.SetOrigin( m_Pos );
    rect.Inflate( ( GetPenSize() + m_Size.x ) / 2 );

    return rect;
}


void SCH_JUNCTION::Draw( WinEDA_DrawPanel* aPanel, wxDC* aDC, const wxPoint& aOffset,
                         int aDrawMode, int aColor )
{
    int color;

    if( aColor >= 0 )
        color = aColor;
    else
        color = ReturnLayerColor( m_Layer );

    GRSetDrawMode( aDC, aDrawMode );

    GRFilledCircle( &aPanel->m_ClipBox, aDC, m_Pos.x + aOffset.x, m_Pos.y + aOffset.y,
                    ( m_Size.x / 2 ), 0, color, color );
}


void SCH_JUNCTION::Mirror_X( int aXaxis_position )
{
    m_Pos.y -= aXaxis_position;
    NEGATE( m_Pos.y );
    m_Pos.y += aXaxis_position;
}


void SCH_JUNCTION::Mirror_Y( int aYaxis_position )
{
    m_Pos.x -= aYaxis_position;
    NEGATE( m_Pos.x );
    m_Pos.x += aYaxis_position;
}


void SCH_JUNCTION::Rotate( wxPoint rotationPoint )
{
    RotatePoint( &m_Pos, rotationPoint, 900 );
}


void SCH_JUNCTION::GetEndPoints( std::vector <DANGLING_END_ITEM>& aItemList )
{
    DANGLING_END_ITEM item( JUNCTION_END, this );
    item.m_Pos = m_Pos;
    aItemList.push_back( item );
}


bool SCH_JUNCTION::IsSelectStateChanged( const wxRect& aRect )
{
    bool previousState = IsSelected();

    if( aRect.Contains( m_Pos ) )
        m_Flags |= SELECTED;
    else
        m_Flags &= ~SELECTED;

    return previousState != IsSelected();
}


void SCH_JUNCTION::GetConnectionPoints( vector< wxPoint >& aPoints ) const
{
    aPoints.push_back( m_Pos );
}


#if defined(DEBUG)
void SCH_JUNCTION::Show( int nestLevel, std::ostream& os )
{
    // XML output:
    wxString s = GetClass();

    NestedSpace( nestLevel, os ) << '<' << s.Lower().mb_str() << m_Pos << "/>\n";
}
#endif


bool SCH_JUNCTION::doHitTest( const wxPoint& aPoint, int aAccuracy, SCH_FILTER_T aFilter ) const
{
    if( !( aFilter & JUNCTION_T ) )
        return false;

    EDA_Rect rect = GetBoundingBox();

    rect.Inflate( aAccuracy );

    return rect.Contains( aPoint );
}


bool SCH_JUNCTION::doHitTest( const EDA_Rect& aRect, bool aContained, int aAccuracy ) const
{
    EDA_Rect rect = aRect;

    rect.Inflate( aAccuracy );

    if( aContained )
        return rect.Contains( GetBoundingBox() );

    return rect.Intersects( GetBoundingBox() );
}


bool SCH_JUNCTION::doIsConnected( const wxPoint& aPosition ) const
{
    return m_Pos == aPosition;
}
