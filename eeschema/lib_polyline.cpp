/************************/
/** class LIB_POLYLINE **/
/************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "macros.h"
#include "class_drawpanel.h"
#include "plot_common.h"
#include "trigo.h"
#include "wxstruct.h"

#include "general.h"
#include "protos.h"
#include "lib_polyline.h"
#include "transform.h"

#include <boost/foreach.hpp>


LIB_POLYLINE::LIB_POLYLINE( LIB_COMPONENT* aParent ) :
    LIB_DRAW_ITEM( COMPONENT_POLYLINE_DRAW_TYPE, aParent )
{
    m_Fill       = NO_FILL;
    m_Width      = 0;
    m_isFillable = true;
    m_typeName   = _( "PolyLine" );
}


LIB_POLYLINE::LIB_POLYLINE( const LIB_POLYLINE& polyline ) :
    LIB_DRAW_ITEM( polyline )
{
    m_PolyPoints = polyline.m_PolyPoints;   // Vector copy
    m_Width      = polyline.m_Width;
    m_Fill       = polyline.m_Fill;
}


bool LIB_POLYLINE::Save( FILE* aFile )
{
    int ccount = GetCornerCount();

    if( fprintf( aFile, "P %d %d %d %d", ccount, m_Unit, m_Convert, m_Width ) < 0 )
        return false;

    for( unsigned i = 0; i < GetCornerCount(); i++ )
    {
        if( fprintf( aFile, "  %d %d", m_PolyPoints[i].x, m_PolyPoints[i].y ) < 0 )
            return false;
    }

    if( fprintf( aFile, " %c\n", fill_tab[m_Fill] ) < 0 )
        return false;

    return true;
}


bool LIB_POLYLINE::Load( char* aLine, wxString& aErrorMsg )
{
    char*   p;
    int     i, ccount = 0;
    wxPoint pt;

    i = sscanf( &aLine[2], "%d %d %d %d", &ccount, &m_Unit, &m_Convert,
                &m_Width );

    m_Fill = NO_FILL;

    if( i < 4 )
    {
        aErrorMsg.Printf( _( "polyline only had %d parameters of the required 4" ), i );
        return false;
    }
    if( ccount <= 0 )
    {
        aErrorMsg.Printf( _( "polyline count parameter %d is invalid" ), ccount );
        return false;
    }

    p = strtok( &aLine[2], " \t\n" );
    p = strtok( NULL, " \t\n" );
    p = strtok( NULL, " \t\n" );
    p = strtok( NULL, " \t\n" );

    for( i = 0; i < ccount; i++ )
    {
        wxPoint point;
        p = strtok( NULL, " \t\n" );
        if( p == NULL || sscanf( p, "%d", &pt.x ) != 1 )
        {
            aErrorMsg.Printf( _( "polyline point %d X position not defined" ), i );
            return false;
        }
        p = strtok( NULL, " \t\n" );
        if( p == NULL || sscanf( p, "%d", &pt.y ) != 1 )
        {
            aErrorMsg.Printf( _( "polyline point %d Y position not defined" ), i );
            return false;
        }
        AddPoint( pt );
    }

    if( ( p = strtok( NULL, " \t\n" ) ) != NULL )
    {
        if( p[0] == 'F' )
            m_Fill = FILLED_SHAPE;
        if( p[0] == 'f' )
            m_Fill = FILLED_WITH_BG_BODYCOLOR;
    }

    return true;
}


LIB_DRAW_ITEM* LIB_POLYLINE::DoGenCopy()
{
    LIB_POLYLINE* newitem = new LIB_POLYLINE( GetParent() );

    newitem->m_PolyPoints = m_PolyPoints;   // Vector copy
    newitem->m_Width      = m_Width;
    newitem->m_Unit       = m_Unit;
    newitem->m_Convert    = m_Convert;
    newitem->m_Flags      = m_Flags;
    newitem->m_Fill       = m_Fill;

    return (LIB_DRAW_ITEM*) newitem;
}


int LIB_POLYLINE::DoCompare( const LIB_DRAW_ITEM& aOther ) const
{
    wxASSERT( aOther.Type() == COMPONENT_POLYLINE_DRAW_TYPE );

    const LIB_POLYLINE* tmp = ( LIB_POLYLINE* ) &aOther;

    if( m_PolyPoints.size() != tmp->m_PolyPoints.size() )
        return m_PolyPoints.size() - tmp->m_PolyPoints.size();

    for( size_t i = 0; i < m_PolyPoints.size(); i++ )
    {
        if( m_PolyPoints[i].x != tmp->m_PolyPoints[i].x )
            return m_PolyPoints[i].x - tmp->m_PolyPoints[i].x;
        if( m_PolyPoints[i].y != tmp->m_PolyPoints[i].y )
            return m_PolyPoints[i].y - tmp->m_PolyPoints[i].y;
    }

    return 0;
}


void LIB_POLYLINE::DoOffset( const wxPoint& aOffset )
{
    for( size_t i = 0; i < m_PolyPoints.size(); i++ )
        m_PolyPoints[i] += aOffset;
}


bool LIB_POLYLINE::DoTestInside( EDA_Rect& aRect ) const
{
    for( size_t i = 0; i < m_PolyPoints.size(); i++ )
    {
        if( aRect.Inside( m_PolyPoints[i].x, -m_PolyPoints[i].y ) )
            return true;
    }

    return false;
}


void LIB_POLYLINE::DoMove( const wxPoint& aPosition )
{
    DoOffset( aPosition - m_PolyPoints[0] );
}


void LIB_POLYLINE::DoMirrorHorizontal( const wxPoint& aCenter )
{
    size_t i, imax = m_PolyPoints.size();

    for( i = 0; i < imax; i++ )
    {
        m_PolyPoints[i].x -= aCenter.x;
        m_PolyPoints[i].x *= -1;
        m_PolyPoints[i].x += aCenter.x;
    }
}


void LIB_POLYLINE::DoPlot( PLOTTER* aPlotter, const wxPoint& aOffset, bool aFill,
                           const TRANSFORM& aTransform )
{
    wxASSERT( aPlotter != NULL );

    size_t i;

    int* Poly = (int*) MyMalloc( sizeof(int) * 2 * GetCornerCount() );

    if( Poly == NULL )
        return;

    for( i = 0; i < m_PolyPoints.size(); i++ )
    {
        wxPoint pos = m_PolyPoints[i];
        pos =  aTransform.TransformCoordinate(pos ) + aOffset;
        Poly[i * 2]     = pos.x;
        Poly[i * 2 + 1] = pos.y;

    }

    if( aFill && m_Fill == FILLED_WITH_BG_BODYCOLOR )
    {
        aPlotter->set_color( ReturnLayerColor( LAYER_DEVICE_BACKGROUND ) );
        aPlotter->poly( i, Poly, FILLED_WITH_BG_BODYCOLOR, 0 );
        aFill = false;  // body is now filled, do not fill it later.
    }

    bool already_filled = m_Fill == FILLED_WITH_BG_BODYCOLOR;
    aPlotter->set_color( ReturnLayerColor( LAYER_DEVICE ) );
    aPlotter->poly( i, Poly, already_filled ? NO_FILL : m_Fill, GetPenSize() );
    MyFree( Poly );
}


void LIB_POLYLINE::AddPoint( const wxPoint& point )
{
    m_PolyPoints.push_back( point );
}


/**
 * Function GetPenSize
 * @return the size of the "pen" that be used to draw or plot this item
 */
int LIB_POLYLINE::GetPenSize()
{
    return ( m_Width == 0 ) ? g_DrawDefaultLineThickness : m_Width;
}


void LIB_POLYLINE::drawGraphic( WinEDA_DrawPanel* aPanel, wxDC* aDC, const wxPoint& aOffset,
                                int aColor, int aDrawMode, void* aData,
                                const TRANSFORM& aTransform )
{
    wxPoint         pos1;
    int             color = ReturnLayerColor( LAYER_DEVICE );

    // Buffer used to store current corners coordinates for drawings
    static wxPoint* Buf_Poly_Drawings = NULL;
    static unsigned Buf_Poly_Size     = 0;

    if( aColor < 0 )                // Used normal color or selected color
    {
        if( m_Selected & IS_SELECTED )
            color = g_ItemSelectetColor;
    }
    else
        color = aColor;

    // Set the size of the buffer of coordinates
    if( Buf_Poly_Drawings == NULL )
    {
        Buf_Poly_Size     = m_PolyPoints.size();
        Buf_Poly_Drawings = (wxPoint*) MyMalloc( sizeof(wxPoint) * Buf_Poly_Size );
    }
    else if( Buf_Poly_Size < m_PolyPoints.size() )
    {
        Buf_Poly_Size     = m_PolyPoints.size();
        Buf_Poly_Drawings = (wxPoint*) realloc( Buf_Poly_Drawings,
                                                sizeof(wxPoint) * Buf_Poly_Size );
    }

    // This should probably throw an exception instead of displaying a warning message.
    if( Buf_Poly_Drawings == NULL )
    {
        wxLogWarning( wxT( "Cannot allocate memory to draw polylines." ) );
        return;
    }

    for( unsigned ii = 0; ii < m_PolyPoints.size(); ii++ )
    {
        Buf_Poly_Drawings[ii] = aTransform.TransformCoordinate( m_PolyPoints[ii] ) + aOffset;
    }

    FILL_T fill = aData ? NO_FILL : m_Fill;
    if( aColor >= 0 )
        fill = NO_FILL;

    GRSetDrawMode( aDC, aDrawMode );

    if( fill == FILLED_WITH_BG_BODYCOLOR )
        GRPoly( &aPanel->m_ClipBox, aDC, m_PolyPoints.size(),
                Buf_Poly_Drawings, 1, GetPenSize( ),
                (m_Flags & IS_MOVED) ? color : ReturnLayerColor( LAYER_DEVICE_BACKGROUND ),
                ReturnLayerColor( LAYER_DEVICE_BACKGROUND ) );
    else if( fill == FILLED_SHAPE  )
        GRPoly( &aPanel->m_ClipBox, aDC, m_PolyPoints.size(),
                Buf_Poly_Drawings, 1, GetPenSize( ), color, color );
    else
        GRPoly( &aPanel->m_ClipBox, aDC, m_PolyPoints.size(),
                Buf_Poly_Drawings, 0, GetPenSize( ), color, color );

    /* Set to one (1) to draw bounding box around polyline to validate
     * bounding box calculation. */
#if 0
    EDA_Rect bBox = GetBoundingBox();
    bBox.Inflate( m_Thickness + 1, m_Thickness + 1 );
    GRRect( &aPanel->m_ClipBox, aDC, bBox.GetOrigin().x, bBox.GetOrigin().y,
            bBox.GetEnd().x, bBox.GetEnd().y, 0, LIGHTMAGENTA );
#endif
}


/**
 * Function HitTest
 * tests if the given wxPoint is within the bounds of this object.
 * @param aRefPos A wxPoint to test
 * @return true if a hit, else false
 */
bool LIB_POLYLINE::HitTest( const wxPoint& aRefPos )
{
    int mindist = m_Width ? m_Width / 2 : g_DrawDefaultLineThickness / 2;

    // Have a minimal tolerance for hit test
    if( mindist < MINIMUM_SELECTION_DISTANCE )
        mindist = MINIMUM_SELECTION_DISTANCE;
    return HitTest( aRefPos, mindist, DefaultTransform );
}


/**
 * Function HitTest
 * @return true if the point aPosRef is near a segment
 * @param aPosRef = a wxPoint to test
 * @param aThreshold = max distance to a segment
 * @param aTransMat = the transform matrix
 */
bool LIB_POLYLINE::HitTest( wxPoint aPosRef, int aThreshold, const TRANSFORM& aTransform )
{
    wxPoint ref, start, end;

    for( unsigned ii = 1; ii < GetCornerCount(); ii++ )
    {
        start = aTransform.TransformCoordinate( m_PolyPoints[ii - 1] );
        end   = aTransform.TransformCoordinate( m_PolyPoints[ii] );

        if( TestSegmentHit( aPosRef, start, end, aThreshold ) )
            return true;
    }

    return false;
}


/**
 * Function GetBoundingBox
 * @return the boundary box for this, in library coordinates
 */
EDA_Rect LIB_POLYLINE::GetBoundingBox()
{
    EDA_Rect rect;
    int      xmin, xmax, ymin, ymax;

    xmin = xmax = m_PolyPoints[0].x;
    ymin = ymax = m_PolyPoints[0].y;

    for( unsigned ii = 1; ii < GetCornerCount(); ii++ )
    {
        xmin = MIN( xmin, m_PolyPoints[ii].x );
        xmax = MAX( xmax, m_PolyPoints[ii].x );
        ymin = MIN( ymin, m_PolyPoints[ii].y );
        ymax = MAX( ymax, m_PolyPoints[ii].y );
    }

    rect.SetOrigin( xmin, ymin * -1 );
    rect.SetEnd( xmax, ymax * -1 );
    rect.Inflate( m_Width / 2, m_Width / 2 );

    return rect;
}


void LIB_POLYLINE::DeleteSegment( const wxPoint aPosition )
{
    // First segment is kept, only its end point is changed
    while( GetCornerCount() > 2 )
    {
        m_PolyPoints.pop_back();

        if( m_PolyPoints[ GetCornerCount() - 1 ] != aPosition )
        {
            m_PolyPoints[ GetCornerCount() - 1 ] = aPosition;
            break;
        }
    }
}


void LIB_POLYLINE::DisplayInfo( WinEDA_DrawFrame* aFrame )
{
    wxString msg;
    EDA_Rect bBox = GetBoundingBox();

    LIB_DRAW_ITEM::DisplayInfo( aFrame );

    msg = ReturnStringFromValue( g_UserUnit, m_Width, EESCHEMA_INTERNAL_UNIT, true );

    aFrame->AppendMsgPanel(_( "Line width" ), msg, BLUE );

    msg.Printf( wxT( "(%d, %d, %d, %d)" ), bBox.GetOrigin().x,
                bBox.GetOrigin().y, bBox.GetEnd().x, bBox.GetEnd().y );

    aFrame->AppendMsgPanel( _( "Bounding box" ), msg, BROWN );
}


void LIB_POLYLINE::BeginEdit( int aEditMode, const wxPoint aPosition )
{
    wxCHECK_RET( ( aEditMode & ( IS_NEW | IS_MOVED | IS_RESIZED ) ) != 0,
                 wxT( "Invalid edit mode for LIB_POLYLINE object." ) );

    if( aEditMode == IS_NEW )
    {
        m_PolyPoints.push_back( aPosition );  // Start point of first segment.
        m_PolyPoints.push_back( aPosition );  // End point of first segment.
    }
    else if( aEditMode == IS_RESIZED )
    {
        // Drag one edge point of the polyline
        // Find the nearest edge point to be dragged
        wxPoint startPoint = m_PolyPoints[0];

        // Begin with the first list point as nearest point
        int index = 0;
        m_ModifyIndex = 0;
        m_initialPos = startPoint;

        // First distance is the current minimum distance
        int distanceMin = (aPosition - startPoint).x * (aPosition - startPoint).x
                          + (aPosition - startPoint).y * (aPosition - startPoint).y;

        // Find the right index of the point to be dragged
        BOOST_FOREACH( wxPoint point, m_PolyPoints )
        {
            int distancePoint = (aPosition - point).x * (aPosition - point).x +
                                (aPosition - point).y * (aPosition - point).y;

            if( distancePoint < distanceMin )
            {
                // Save point.
                m_initialPos = point;
                m_ModifyIndex = index;
                distanceMin = distancePoint;
                break;
            }

            index++;
        }

        SetEraseLastDrawItem();
    }
    else if( aEditMode == IS_MOVED )
    {
        m_initialCursorPos = aPosition;
        m_initialPos = m_PolyPoints[0];
        SetEraseLastDrawItem();
    }

    m_Flags = aEditMode;
}


bool LIB_POLYLINE::ContinueEdit( const wxPoint aPosition )
{
    wxCHECK_MSG( ( m_Flags & ( IS_NEW | IS_MOVED | IS_RESIZED ) ) != 0, false,
                   wxT( "Bad call to ContinueEdit().  LIB_POLYLINE is not being edited." ) );

    if( m_Flags == IS_NEW )
    {
        m_PolyPoints.push_back( aPosition );
        return true;
    }

    return false;
}


void LIB_POLYLINE::EndEdit( const wxPoint& aPosition, bool aAbort )
{
    wxCHECK_RET( ( m_Flags & ( IS_NEW | IS_MOVED | IS_RESIZED ) ) != 0,
                   wxT( "Bad call to EndEdit().  LIB_POLYLINE is not being edited." ) );

    m_Flags = 0;
    SetEraseLastDrawItem( false );
}


void LIB_POLYLINE::calcEdit( const wxPoint& aPosition )
{
    if( m_Flags == IS_NEW )
    {
        m_PolyPoints[ GetCornerCount() - 1 ] = aPosition;
        SetEraseLastDrawItem();
    }
    else if( m_Flags == IS_RESIZED )
    {
        m_PolyPoints[ m_ModifyIndex ] = aPosition;
    }
    else if( m_Flags == IS_MOVED )
    {
        Move( m_initialPos + aPosition - m_initialCursorPos );
    }
}
