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

#include <boost/foreach.hpp>


/* used to calculate the pen size from default value
 * the actual pen size is default value * BUS_WIDTH_EXPAND
 */
#if defined(KICAD_GOST)
#define BUS_WIDTH_EXPAND 3.6
#else
#define BUS_WIDTH_EXPAND 1.4
#endif

/***********************/
/* class SCH_BUS_ENTRY */
/***********************/

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


wxPoint SCH_BUS_ENTRY::m_End() const
{
    return wxPoint( m_Pos.x + m_Size.x, m_Pos.y + m_Size.y );
}


SCH_BUS_ENTRY* SCH_BUS_ENTRY::GenCopy()
{
    SCH_BUS_ENTRY* newitem = new SCH_BUS_ENTRY( m_Pos, 0, 0 );

    newitem->m_Layer = m_Layer;
    newitem->m_Width = m_Width;
    newitem->m_Size  = m_Size;
    newitem->m_Flags = m_Flags;

    return newitem;
}


/**
 * Function Save
 * writes the data structures for this object out to a FILE in "*.sch" format.
 * @param aFile The FILE to write to.
 * @return bool - true if success writing else false.
 */
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
        aErrorMsg << wxT( "\n" ) << CONV_FROM_UTF8( (char*) aLine );
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
        aErrorMsg << wxT( "\n" ) << CONV_FROM_UTF8( (char*) aLine );
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


/**
 * Function GetPenSize
 * @return the size of the "pen" that be used to draw or plot this item
 */
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


void SCH_BUS_ENTRY::Draw( WinEDA_DrawPanel* panel, wxDC* DC,
                          const wxPoint& offset, int DrawMode, int Color )
{
    int color;

    if( Color >= 0 )
        color = Color;
    else
        color = ReturnLayerColor( m_Layer );
    GRSetDrawMode( DC, DrawMode );

    GRLine( &panel->m_ClipBox, DC, m_Pos.x + offset.x, m_Pos.y + offset.y,
            m_End().x + offset.x, m_End().y + offset.y, GetPenSize(), color );
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


bool SCH_BUS_ENTRY::DoHitTest( const wxPoint& aPoint, int aAccuracy, SCH_FILTER_T aFilter ) const
{
    if( !( aFilter & BUS_ENTRY_T ) )
        return false;

    return TestSegmentHit( aPoint, m_Pos, m_End(), aAccuracy );
}


bool SCH_BUS_ENTRY::DoHitTest( const EDA_Rect& aRect, bool aContained, int aAccuracy ) const
{
    EDA_Rect rect = aRect;

    rect.Inflate( aAccuracy );

    if( aContained )
        return rect.Inside( GetBoundingBox() );

    return rect.Intersects( GetBoundingBox() );
}


/**********************/
/* class SCH_JUNCTION */
/**********************/

SCH_JUNCTION::SCH_JUNCTION( const wxPoint& pos ) : SCH_ITEM( NULL, SCH_JUNCTION_T )
{
#define DRAWJUNCTION_DIAMETER 32   /* Diameter of junction symbol between wires */
    m_Pos    = pos;
    m_Layer  = LAYER_JUNCTION;
    m_Size.x = m_Size.y = DRAWJUNCTION_DIAMETER;
#undef DRAWJUNCTION_DIAMETER
}


SCH_JUNCTION* SCH_JUNCTION::GenCopy()
{
    SCH_JUNCTION* newitem = new SCH_JUNCTION( m_Pos );

    newitem->m_Size  = m_Size;
    newitem->m_Layer = m_Layer;
    newitem->m_Flags = m_Flags;

    return newitem;
}


/**
 * Function Save
 * writes the data structures for this object out to a FILE in "*.sch" format.
 * @param aFile The FILE to write to.
 * @return bool - true if success writing else false.
 */
bool SCH_JUNCTION::Save( FILE* aFile ) const
{
    bool success = true;

    if( fprintf( aFile, "Connection ~ %-4d %-4d\n", m_Pos.x, m_Pos.y ) == EOF )
    {
        success = false;
    }

    return success;
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


/*****************************************************************************
* Routine to redraw connection struct.                                       *
*****************************************************************************/
void SCH_JUNCTION::Draw( WinEDA_DrawPanel* panel, wxDC* DC,
                         const wxPoint& offset, int DrawMode, int Color )
{
    int color;

    if( Color >= 0 )
        color = Color;
    else
        color = ReturnLayerColor( m_Layer );
    GRSetDrawMode( DC, DrawMode );

    GRFilledCircle( &panel->m_ClipBox, DC, m_Pos.x + offset.x,
                    m_Pos.y + offset.y, (m_Size.x / 2), 0, color,
                    color );
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


bool SCH_JUNCTION::DoHitTest( const wxPoint& aPoint, int aAccuracy, SCH_FILTER_T aFilter ) const
{
    if( !( aFilter & JUNCTION_T ) )
        return false;

    EDA_Rect rect = GetBoundingBox();

    rect.Inflate( aAccuracy );

    return rect.Inside( aPoint );
}


bool SCH_JUNCTION::DoHitTest( const EDA_Rect& aRect, bool aContained, int aAccuracy ) const
{
    EDA_Rect rect = aRect;

    rect.Inflate( aAccuracy );

    if( aContained )
        return rect.Inside( GetBoundingBox() );

    return rect.Intersects( GetBoundingBox() );
}


bool SCH_JUNCTION::DoIsConnected( const wxPoint& aPosition ) const
{
    return m_Pos == aPosition;
}


/************************/
/* class SCH_NO_CONNECT */
/************************/

SCH_NO_CONNECT::SCH_NO_CONNECT( const wxPoint& pos ) : SCH_ITEM( NULL, SCH_NO_CONNECT_T )
{
#define DRAWNOCONNECT_SIZE 48       /* No symbol connection range. */
    m_Pos    = pos;
    m_Size.x = m_Size.y = DRAWNOCONNECT_SIZE;
#undef DRAWNOCONNECT_SIZE
}


SCH_NO_CONNECT* SCH_NO_CONNECT::GenCopy()
{
    SCH_NO_CONNECT* newitem = new SCH_NO_CONNECT( m_Pos );

    newitem->m_Size  = m_Size;
    newitem->m_Flags = m_Flags;

    return newitem;
}


EDA_Rect SCH_NO_CONNECT::GetBoundingBox() const
{
    int      delta = ( GetPenSize() + m_Size.x ) / 2;
    EDA_Rect box;

    box.SetOrigin( m_Pos );
    box.Inflate( delta );

    return box;
}


/**
 * Function Save
 * writes the data structures for this object out to a FILE in "*.sch" format.
 * @param aFile The FILE to write to.
 * @return bool - true if success writing else false.
 */
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
        aErrorMsg << wxT( "\n" ) << CONV_FROM_UTF8( ((char*)aLine) );
        return false;
    }

    return true;
}


/**
 * Function GetPenSize
 * @return the size of the "pen" that be used to draw or plot this item
 */
int SCH_NO_CONNECT::GetPenSize() const
{
    return g_DrawDefaultLineThickness;
}


void SCH_NO_CONNECT::Draw( WinEDA_DrawPanel* panel, wxDC* DC,
                           const wxPoint& offset, int DrawMode, int Color )
{
    int pX, pY, color;
    int delta = m_Size.x / 2;
    int width = g_DrawDefaultLineThickness;

    pX = m_Pos.x + offset.x;
    pY = m_Pos.y + offset.y;

    if( Color >= 0 )
        color = Color;
    else
        color = ReturnLayerColor( LAYER_NOCONNECT );
    GRSetDrawMode( DC, DrawMode );

    GRLine( &panel->m_ClipBox, DC, pX - delta, pY - delta, pX + delta,
            pY + delta, width, color );
    GRLine( &panel->m_ClipBox, DC, pX + delta, pY - delta, pX - delta,
            pY + delta, width, color );
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


bool SCH_NO_CONNECT::DoHitTest( const wxPoint& aPoint, int aAccuracy, SCH_FILTER_T aFilter ) const
{
    if( !( aFilter & NO_CONNECT_T ) )
        return false;

    int delta = ( ( m_Size.x + g_DrawDefaultLineThickness ) / 2 ) + aAccuracy;

    wxPoint dist = aPoint - m_Pos;

    if( ( ABS( dist.x ) <= delta ) && ( ABS( dist.y ) <= delta ) )
        return true;

    return false;
}


bool SCH_NO_CONNECT::DoHitTest( const EDA_Rect& aRect, bool aContained, int aAccuracy ) const
{
    EDA_Rect rect = aRect;

    rect.Inflate( aAccuracy );

    if( aContained )
        return rect.Inside( GetBoundingBox() );

    return rect.Intersects( GetBoundingBox() );
}


/******************/
/* Class SCH_LINE */
/******************/

SCH_LINE::SCH_LINE( const wxPoint& pos, int layer ) :
    SCH_ITEM( NULL, SCH_LINE_T )
{
    m_Start = pos;
    m_End   = pos;
    m_Width = 0;        // Default thickness used
    m_StartIsDangling = m_EndIsDangling = FALSE;

    switch( layer )
    {
    default:
        m_Layer = LAYER_NOTES;
        break;

    case LAYER_WIRE:
        m_Layer = LAYER_WIRE;
        break;

    case LAYER_BUS:
        m_Layer = LAYER_BUS;
        break;
    }
}


SCH_LINE* SCH_LINE::GenCopy()
{
    SCH_LINE* newitem = new SCH_LINE( m_Start, m_Layer );

    newitem->m_End = m_End;

    return newitem;
}


void SCH_LINE::Move( const wxPoint& aOffset )
{
    if( (m_Flags & STARTPOINT) == 0 && aOffset != wxPoint( 0, 0 ) )
    {
        m_Start += aOffset;
        SetModified();
    }

    if( (m_Flags & ENDPOINT) == 0 && aOffset != wxPoint( 0, 0 ) )
    {
        m_End += aOffset;
        SetModified();
    }
}


#if defined(DEBUG)

/**
 * Function Show
 * is used to output the object tree, currently for debugging only.
 * @param nestLevel An aid to prettier tree indenting, and is the level
 *          of nesting of this object within the overall tree.
 * @param os The ostream& to output to.
 */
void SCH_LINE::Show( int nestLevel, std::ostream& os ) const
{
    NestedSpace( nestLevel, os ) << '<' << GetClass().Lower().mb_str()
                                 << " layer=\"" << m_Layer << '"'
                                 << " width=\"" << m_Width << '"'
                                 << " startIsDangling=\"" << m_StartIsDangling
                                 << '"' << " endIsDangling=\""
                                 << m_EndIsDangling << '"' << ">"
                                 << " <start" << m_Start << "/>"
                                 << " <end" << m_End << "/>" << "</"
                                 << GetClass().Lower().mb_str() << ">\n";
}


#endif


EDA_Rect SCH_LINE::GetBoundingBox() const
{
    int      width = 25;

    int      xmin = MIN( m_Start.x, m_End.x ) - width;
    int      ymin = MIN( m_Start.y, m_End.y ) - width;

    int      xmax = MAX( m_Start.x, m_End.x ) + width;
    int      ymax = MAX( m_Start.y, m_End.y ) + width;

    // return a rectangle which is [pos,dim) in nature.  therefore the +1
    EDA_Rect ret( wxPoint( xmin, ymin ), wxSize( xmax - xmin + 1, ymax - ymin + 1 ) );

    return ret;
}


/**
 * Function Save
 * writes the data structures for this object out to a FILE in "*.sch" format.
 * @param aFile The FILE to write to.
 * @return bool - true if success writing else false.
 */
bool SCH_LINE::Save( FILE* aFile ) const
{
    bool        success = true;

    const char* layer = "Notes";
    const char* width = "Line";

    if( GetLayer() == LAYER_WIRE )
        layer = "Wire";

    if( GetLayer() == LAYER_BUS )
        layer = "Bus";

    if( fprintf( aFile, "Wire %s %s\n", layer, width ) == EOF )
    {
        success = false;
    }

    if( fprintf( aFile, "\t%-4d %-4d %-4d %-4d\n", m_Start.x, m_Start.y,
                 m_End.x, m_End.y ) == EOF )
    {
        success = false;
    }

    return success;
}


bool SCH_LINE::Load( LINE_READER& aLine, wxString& aErrorMsg )
{
    char  Name1[256];
    char  Name2[256];
    char* line = (char*) aLine;

    while( (*line != ' ' ) && *line )
        line++;

    if( sscanf( line, "%s %s", Name1, Name2 ) != 2  )
    {
        aErrorMsg.Printf( wxT( "EESchema file segment error at line %d, aborted" ),
                          aLine.LineNumber() );
        aErrorMsg << wxT( "\n" ) << CONV_FROM_UTF8( (char*) aLine );
        return false;
    }

    m_Layer = LAYER_NOTES;

    if( Name1[0] == 'W' )
        m_Layer = LAYER_WIRE;

    if( Name1[0] == 'B' )
        m_Layer = LAYER_BUS;

    if( !aLine.ReadLine() || sscanf( (char*) aLine, "%d %d %d %d ",
                                      &m_Start.x, &m_Start.y, &m_End.x, &m_End.y ) != 4 )
    {
        aErrorMsg.Printf( wxT( "EESchema file Segment struct error at line %d, aborted" ),
                          aLine.LineNumber() );
        aErrorMsg << wxT( "\n" ) << CONV_FROM_UTF8( (char*) aLine );
        return false;
    }

    return true;
}


/**
 * Function GetPenSize
 * @return the size of the "pen" that be used to draw or plot this item
 */
int SCH_LINE::GetPenSize() const
{
    int pensize = ( m_Width == 0 ) ? g_DrawDefaultLineThickness : m_Width;

    if( m_Layer == LAYER_BUS && m_Width == 0 )
    {
        pensize = wxRound( g_DrawDefaultLineThickness * BUS_WIDTH_EXPAND );
        pensize = MAX( pensize, 3 );
    }

    return pensize;
}


void SCH_LINE::Draw( WinEDA_DrawPanel* panel, wxDC* DC, const wxPoint& offset,
                     int DrawMode, int Color )
{
    int color;
    int width = GetPenSize();

    if( Color >= 0 )
        color = Color;
    else
        color = ReturnLayerColor( m_Layer );

    GRSetDrawMode( DC, DrawMode );

    if( m_Layer == LAYER_NOTES )
        GRDashedLine( &panel->m_ClipBox, DC, m_Start.x + offset.x,
                      m_Start.y + offset.y, m_End.x + offset.x,
                      m_End.y + offset.y, width, color );
    else
        GRLine( &panel->m_ClipBox, DC, m_Start.x + offset.x,
                m_Start.y + offset.y, m_End.x + offset.x, m_End.y + offset.y,
                width, color );

    if( m_StartIsDangling )
        DrawDanglingSymbol( panel, DC, m_Start + offset, color );

    if( m_EndIsDangling )
        DrawDanglingSymbol( panel, DC, m_End + offset, color );
}


void SCH_LINE::Mirror_X( int aXaxis_position )
{
    m_Start.y -= aXaxis_position;
    NEGATE(  m_Start.y );
    m_Start.y += aXaxis_position;
    m_End.y   -= aXaxis_position;
    NEGATE(  m_End.y );
    m_End.y += aXaxis_position;
}


void SCH_LINE::Mirror_Y( int aYaxis_position )
{
    m_Start.x -= aYaxis_position;
    NEGATE(  m_Start.x );
    m_Start.x += aYaxis_position;
    m_End.x   -= aYaxis_position;
    NEGATE(  m_End.x );
    m_End.x += aYaxis_position;
}


void SCH_LINE::Rotate( wxPoint rotationPoint )
{
    RotatePoint( &m_Start, rotationPoint, 900 );
    RotatePoint( &m_End, rotationPoint, 900 );
}


bool SCH_LINE::MergeOverlap( SCH_LINE* aLine )
{
    wxCHECK_MSG( aLine != NULL && aLine->Type() == SCH_LINE_T, false,
                 wxT( "Cannot test line segment for overlap." ) );

    if( this == aLine || GetLayer() != aLine->GetLayer() )
        return false;

    // Search for a common end, and modify coordinates to ensure RefSegm->m_End
    // == TstSegm->m_Start
    if( m_Start == aLine->m_Start )
    {
        if( m_End == aLine->m_End )
            return true;

        EXCHG( m_Start, m_End );
    }
    else if( m_Start == aLine->m_End )
    {
        EXCHG( m_Start, m_End );
        EXCHG( aLine->m_Start, aLine->m_End );
    }
    else if( m_End == aLine->m_End )
    {
        EXCHG( aLine->m_Start, aLine->m_End );
    }
    else if( m_End != aLine->m_Start )
    {
        // No common end point, segments cannot be merged.
        return false;
    }

    /* Test alignment: */
    if( m_Start.y == m_End.y )       // Horizontal segment
    {
        if( aLine->m_Start.y == aLine->m_End.y )
        {
            m_End = aLine->m_End;
            return true;
        }
    }
    else if( m_Start.x == m_End.x )  // Vertical segment
    {
        if( aLine->m_Start.x == aLine->m_End.x )
        {
            m_End = aLine->m_End;
            return true;
        }
    }
    else
    {
        if( atan2( (double) ( m_Start.x - m_End.x ), (double) ( m_Start.y - m_End.y ) )
            == atan2( (double) ( aLine->m_Start.x - aLine->m_End.x ),
                      (double) ( aLine->m_Start.y - aLine->m_End.y ) ) )
        {
            m_End = aLine->m_End;
            return true;
        }
    }

    return false;
}


void SCH_LINE::GetEndPoints( std::vector <DANGLING_END_ITEM>& aItemList )
{
    if( GetLayer() == LAYER_NOTES )
        return;

    if( ( GetLayer() == LAYER_BUS ) || ( GetLayer() == LAYER_WIRE ) )
    {
        DANGLING_END_ITEM item( (GetLayer() == LAYER_BUS) ? BUS_START_END : WIRE_START_END, this );
        item.m_Pos = m_Start;
        DANGLING_END_ITEM item1( (GetLayer() == LAYER_BUS) ? BUS_END_END : WIRE_END_END, this );
        item1.m_Pos = m_End;

        aItemList.push_back( item );
        aItemList.push_back( item1 );
    }
}


bool SCH_LINE::IsDanglingStateChanged( std::vector< DANGLING_END_ITEM >& aItemList )
{
    bool previousStartState = m_StartIsDangling;
    bool previousEndState = m_EndIsDangling;

    m_StartIsDangling = m_EndIsDangling = true;

    if( GetLayer() == LAYER_WIRE )
    {
        BOOST_FOREACH( DANGLING_END_ITEM item, aItemList )
        {
            if( item.m_Item == this )
                continue;

            if( m_Start == item.m_Pos )
                m_StartIsDangling = false;

            if( m_End == item.m_Pos )
                m_EndIsDangling = false;

            if( (m_StartIsDangling == false) && (m_EndIsDangling == false) )
                break;
        }
    }
    else if( GetLayer() == LAYER_BUS || GetLayer() == LAYER_NOTES )
    {
        // Lines on the notes layer and the bus layer cannot be tested for dangling ends.
        previousStartState = previousEndState = m_StartIsDangling = m_EndIsDangling = false;
    }

    return ( previousStartState != m_StartIsDangling ) || ( previousEndState != m_EndIsDangling );
}


bool SCH_LINE::IsSelectStateChanged( const wxRect& aRect )
{
    bool previousState = IsSelected();

    if( aRect.Contains( m_Start ) )
        m_Flags |= STARTPOINT | SELECTED;
    else
        m_Flags &= ~( STARTPOINT | SELECTED );

    if( aRect.Contains( m_End ) )
        m_Flags |= ENDPOINT | SELECTED;
    else
        m_Flags &= ~( ENDPOINT | SELECTED );

    return previousState != IsSelected();
}


void SCH_LINE::GetConnectionPoints( vector< wxPoint >& aPoints ) const
{
    aPoints.push_back( m_Start );
    aPoints.push_back( m_End );
}


bool SCH_LINE::DoHitTest( const wxPoint& aPoint, int aAccuracy, SCH_FILTER_T aFilter ) const
{
    if( !( aFilter & ( DRAW_ITEM_T | WIRE_T | BUS_T ) ) )
        return false;

    if( ( ( aFilter & DRAW_ITEM_T ) && ( m_Layer == LAYER_NOTES ) )
        || ( ( aFilter & WIRE_T ) && ( m_Layer == LAYER_WIRE ) )
        || ( ( aFilter & BUS_T ) && ( m_Layer == LAYER_BUS ) ) )
    {
        if( aFilter & EXCLUDE_WIRE_BUS_ENDPOINTS && IsEndPoint( aPoint )
            || aFilter & WIRE_BUS_ENDPOINTS_ONLY && !IsEndPoint( aPoint )
            || TestSegmentHit( aPoint, m_Start, m_End, aAccuracy ) )
            return true;
    }

    return false;
}


bool SCH_LINE::DoHitTest( const EDA_Rect& aRect, bool aContained, int aAccuracy ) const
{
    EDA_Rect rect = aRect;

    rect.Inflate( aAccuracy );

    if( aContained )
        return rect.Inside( GetBoundingBox() );

    return rect.Intersects( GetBoundingBox() );
}


bool SCH_LINE::DoIsConnected( const wxPoint& aPosition ) const
{
    if( m_Layer != LAYER_WIRE && m_Layer != LAYER_BUS )
        return false;

    return IsEndPoint( aPosition );
}


/**********************/
/* Class SCH_POLYLINE */
/**********************/

SCH_POLYLINE::SCH_POLYLINE( int layer ) : SCH_ITEM( NULL, SCH_POLYLINE_T )
{
    m_Width = 0;

    switch( layer )
    {
    default:
        m_Layer = LAYER_NOTES;
        break;

    case LAYER_WIRE:
    case LAYER_NOTES:
    case LAYER_BUS:
        m_Layer = layer;
        break;
    }
}


SCH_POLYLINE::~SCH_POLYLINE()
{
}


SCH_POLYLINE* SCH_POLYLINE::GenCopy()
{
    SCH_POLYLINE* newitem = new SCH_POLYLINE( m_Layer );

    newitem->m_PolyPoints = m_PolyPoints;   // std::vector copy
    return newitem;
}


/**
 * Function Save
 * writes the data structures for this object out to a FILE in "*.sch" format.
 * @param aFile The FILE to write to.
 * @return bool - true if success writing else false.
 */
bool SCH_POLYLINE::Save( FILE* aFile ) const
{
    bool        success = true;

    const char* layer = "Notes";
    const char* width = "Line";

    if( GetLayer() == LAYER_WIRE )
        layer = "Wire";
    if( GetLayer() == LAYER_BUS )
        layer = "Bus";
    if( fprintf( aFile, "Poly %s %s %d\n", width, layer, GetCornerCount() ) == EOF )
    {
        return false;
    }
    for( unsigned ii = 0; ii < GetCornerCount(); ii++ )
    {
        if( fprintf( aFile, "\t%-4d %-4d\n", m_PolyPoints[ii ].x, m_PolyPoints[ii].y ) == EOF )
        {
            success = false;
            break;
        }
    }

    return success;
}


bool SCH_POLYLINE::Load( LINE_READER& aLine, wxString& aErrorMsg )
{
    char Name1[256];
    char Name2[256];
    wxPoint pt;
    int ii;
    char* line = (char*) aLine;

    while( (*line != ' ' ) && *line )
        line++;

    if( sscanf( line, "%s %s %d", Name1, Name2, &ii ) != 3 )
    {
        aErrorMsg.Printf( wxT( "EESchema file polyline struct error at line %d, aborted" ),
                          aLine.LineNumber() );
        aErrorMsg << wxT( "\n" ) << CONV_FROM_UTF8( (char*) aLine );
        return false;
    }

    m_Layer = LAYER_NOTES;
    if( Name2[0] == 'W' )
        m_Layer = LAYER_WIRE;
    if( Name2[0] == 'B' )
        m_Layer = LAYER_BUS;

    for( unsigned jj = 0; jj < (unsigned)ii; jj++ )
    {
        wxPoint point;

        if( !aLine.ReadLine() || sscanf( ((char*) aLine), "%d %d", &pt.x, &pt.y ) != 2 )
        {
            aErrorMsg.Printf( wxT( "EESchema file polyline struct error at line %d, aborted" ),
                              aLine.LineNumber() );
            aErrorMsg << wxT( "\n" ) << CONV_FROM_UTF8( (char*) aLine );
            return false;
        }

        AddPoint( pt );
    }

    return true;
}


/**
 * Function GetPenSize
 * @return the size of the "pen" that be used to draw or plot this item
 */
int SCH_POLYLINE::GetPenSize() const
{
    int pensize = ( m_Width == 0 ) ? g_DrawDefaultLineThickness : m_Width;

    return pensize;
}


void SCH_POLYLINE::Draw( WinEDA_DrawPanel* aPanel, wxDC* aDC, const wxPoint& aOffset,
                         int aDrawMode, int aColor )
{
    int color;
    int width = GetPenSize();

    if( aColor >= 0 )
        color = aColor;
    else
        color = ReturnLayerColor( m_Layer );

    GRSetDrawMode( aDC, aDrawMode );

    if( m_Layer == LAYER_BUS )
    {
        width *= 3;
    }

    GRMoveTo( m_PolyPoints[0].x, m_PolyPoints[0].y );

    if( m_Layer == LAYER_NOTES )
    {
        for( unsigned i = 1; i < GetCornerCount(); i++ )
            GRDashedLineTo( &aPanel->m_ClipBox, aDC, m_PolyPoints[i].x + aOffset.x,
                            m_PolyPoints[i].y + aOffset.y, width, color );
    }
    else
    {
        for( unsigned i = 1; i < GetCornerCount(); i++ )
            GRLineTo( &aPanel->m_ClipBox, aDC, m_PolyPoints[i].x + aOffset.x,
                      m_PolyPoints[i].y + aOffset.y, width, color );
    }
}


void SCH_POLYLINE::Mirror_X( int aXaxis_position )
{
    for( unsigned ii = 0; ii < GetCornerCount(); ii++ )
    {
        m_PolyPoints[ii].y -= aXaxis_position;
        NEGATE(  m_PolyPoints[ii].y );
        m_PolyPoints[ii].y = aXaxis_position;
    }
}


void SCH_POLYLINE::Mirror_Y( int aYaxis_position )
{
    for( unsigned ii = 0; ii < GetCornerCount(); ii++ )
    {
        m_PolyPoints[ii].x -= aYaxis_position;
        NEGATE(  m_PolyPoints[ii].x );
        m_PolyPoints[ii].x = aYaxis_position;
    }
}


void SCH_POLYLINE::Rotate( wxPoint rotationPoint )
{
    for( unsigned ii = 0; ii < GetCornerCount(); ii++ )
    {
        RotatePoint( &m_PolyPoints[ii], rotationPoint, 900 );
    }
}


bool SCH_POLYLINE::DoHitTest( const wxPoint& aPoint, int aAccuracy, SCH_FILTER_T aFilter ) const
{
    if( !( aFilter & ( DRAW_ITEM_T | WIRE_T | BUS_T ) ) )
        return false;

    for( size_t i = 0;  i < m_PolyPoints.size() - 1;  i++ )
    {
        if( TestSegmentHit( aPoint, m_PolyPoints[i], m_PolyPoints[i + 1], aAccuracy ) )
            return true;
    }

    return false;
}


bool SCH_POLYLINE::DoHitTest( const EDA_Rect& aRect, bool aContained, int aAccuracy ) const
{
    EDA_Rect rect = aRect;

    rect.Inflate( aAccuracy );

    if( aContained )
        return rect.Inside( GetBoundingBox() );

    return rect.Intersects( GetBoundingBox() );
}
