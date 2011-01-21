/**********************/
/** class LIB_BEZIER **/
/**********************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "macros.h"
#include "class_drawpanel.h"
#include "plot_common.h"
#include "trigo.h"
#include "wxstruct.h"
#include "bezier_curves.h"

#include "general.h"
#include "protos.h"
#include "lib_bezier.h"
#include "transform.h"


LIB_BEZIER::LIB_BEZIER( LIB_COMPONENT* aParent ) :
    LIB_DRAW_ITEM( LIB_BEZIER_T, aParent )
{
    m_Fill       = NO_FILL;
    m_Width      = 0;
    m_isFillable = true;
    m_typeName   = _( "Bezier" );
}


LIB_BEZIER::LIB_BEZIER( const LIB_BEZIER& aBezier ) : LIB_DRAW_ITEM( aBezier )
{
    m_PolyPoints   = aBezier.m_PolyPoints;
    m_BezierPoints = aBezier.m_BezierPoints;   // Vector copy
    m_Width        = aBezier.m_Width;
    m_Fill         = aBezier.m_Fill;
}


bool LIB_BEZIER::Save( FILE* aFile )
{
    int ccount = GetCornerCount();

    if( fprintf( aFile, "B %d %d %d %d", ccount, m_Unit, m_Convert, m_Width ) < 0 )
        return false;

    for( unsigned i = 0; i < GetCornerCount(); i++ )
    {
        if( fprintf( aFile, "  %d %d", m_BezierPoints[i].x, m_BezierPoints[i].y ) < 0 )
            return false;
    }

    if( fprintf( aFile, " %c\n", fill_tab[m_Fill] ) < 0 )
        return false;

    return true;
}


bool LIB_BEZIER::Load( char* aLine, wxString& aErrorMsg )
{
    char*   p;
    int     i, ccount = 0;
    wxPoint pt;

    i = sscanf( &aLine[2], "%d %d %d %d", &ccount, &m_Unit, &m_Convert, &m_Width );

    if( i !=4 )
    {
        aErrorMsg.Printf( _( "Bezier only had %d parameters of the required 4" ), i );
        return false;
    }
    if( ccount <= 0 )
    {
        aErrorMsg.Printf( _( "Bezier count parameter %d is invalid" ), ccount );
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
        if( sscanf( p, "%d", &pt.x ) != 1 )
        {
            aErrorMsg.Printf( _( "Bezier point %d X position not defined" ), i );
            return false;
        }
        p = strtok( NULL, " \t\n" );
        if( sscanf( p, "%d", &pt.y ) != 1 )
        {
            aErrorMsg.Printf( _( "Bezier point %d Y position not defined" ), i );
            return false;
        }
        m_BezierPoints.push_back( pt );
    }

    m_Fill = NO_FILL;

    if( ( p = strtok( NULL, " \t\n" ) ) != NULL )
    {
        if( p[0] == 'F' )
            m_Fill = FILLED_SHAPE;
        if( p[0] == 'f' )
            m_Fill = FILLED_WITH_BG_BODYCOLOR;
    }

    return true;
}


LIB_DRAW_ITEM* LIB_BEZIER::DoGenCopy()
{
    LIB_BEZIER* newitem = new LIB_BEZIER(GetParent());

    newitem->m_BezierPoints = m_BezierPoints;   // Vector copy
    newitem->m_Width   = m_Width;
    newitem->m_Unit    = m_Unit;
    newitem->m_Convert = m_Convert;
    newitem->m_Flags   = m_Flags;
    newitem->m_Fill    = m_Fill;
    return (LIB_DRAW_ITEM*) newitem;
}


int LIB_BEZIER::DoCompare( const LIB_DRAW_ITEM& aOther ) const
{
    wxASSERT( aOther.Type() == LIB_BEZIER_T );

    const LIB_BEZIER* tmp = ( LIB_BEZIER* ) &aOther;

    if( m_BezierPoints.size() != tmp->m_BezierPoints.size() )
        return m_BezierPoints.size() - tmp->m_BezierPoints.size();

    for( size_t i = 0; i < m_BezierPoints.size(); i++ )
    {
        if( m_BezierPoints[i].x != tmp->m_BezierPoints[i].x )
            return m_BezierPoints[i].x - tmp->m_BezierPoints[i].x;
        if( m_BezierPoints[i].y != tmp->m_BezierPoints[i].y )
            return m_BezierPoints[i].y - tmp->m_BezierPoints[i].y;
    }

    return 0;
}


void LIB_BEZIER::DoOffset( const wxPoint& aOffset )
{
    size_t i;

    for( i = 0; i < m_BezierPoints.size(); i++ )
        m_BezierPoints[i] += aOffset;

    for( i = 0; i < m_PolyPoints.size(); i++ )
        m_PolyPoints[i] += aOffset;
}


bool LIB_BEZIER::DoTestInside( EDA_Rect& aRect ) const
{
    for( size_t i = 0; i < m_PolyPoints.size(); i++ )
    {
        if( aRect.Contains( m_PolyPoints[i].x, -m_PolyPoints[i].y ) )
            return true;
    }

    return false;
}


void LIB_BEZIER::DoMove( const wxPoint& aPosition )
{
    DoOffset( aPosition - m_PolyPoints[0] );
}


void LIB_BEZIER::DoMirrorHorizontal( const wxPoint& aCenter )
{
    size_t i, imax = m_PolyPoints.size();

    for( i = 0; i < imax; i++ )
    {
        m_PolyPoints[i].x -= aCenter.x;
        m_PolyPoints[i].x *= -1;
        m_PolyPoints[i].x += aCenter.x;
    }

    imax = m_BezierPoints.size();
    for( i = 0; i < imax; i++ )
    {
        m_BezierPoints[i].x -= aCenter.x;
        m_BezierPoints[i].x *= -1;
        m_BezierPoints[i].x += aCenter.x;
    }
}


void LIB_BEZIER::DoPlot( PLOTTER* aPlotter, const wxPoint& aOffset, bool aFill,
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
        pos = aTransform.TransformCoordinate( pos ) + aOffset;
        Poly[i * 2]     = pos.x;
        Poly[i * 2 + 1] = pos.y;
    }

    if( aFill && m_Fill == FILLED_WITH_BG_BODYCOLOR )
    {
        aPlotter->set_color( ReturnLayerColor( LAYER_DEVICE_BACKGROUND ) );
        aPlotter->poly( i, Poly, FILLED_WITH_BG_BODYCOLOR, 0 );
    }

    bool already_filled = m_Fill == FILLED_WITH_BG_BODYCOLOR;
    aPlotter->set_color( ReturnLayerColor( LAYER_DEVICE ) );
    aPlotter->poly( i, Poly, already_filled ? NO_FILL : m_Fill, GetPenSize() );
    MyFree( Poly );
}


/**
 * Function GetPenSize
 * @return the size of the "pen" that be used to draw or plot this item
 */
int LIB_BEZIER::GetPenSize()
{
    return ( m_Width == 0 ) ? g_DrawDefaultLineThickness : m_Width;
}

void LIB_BEZIER::drawGraphic( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aOffset,
                              int aColor, int aDrawMode, void* aData, const TRANSFORM& aTransform )
{
    wxPoint              pos1;
    std::vector<wxPoint> PolyPointsTraslated;

    int                  color = ReturnLayerColor( LAYER_DEVICE );

    m_PolyPoints = Bezier2Poly( m_BezierPoints[0],
                                m_BezierPoints[1],
                                m_BezierPoints[2],
                                m_BezierPoints[3] );

    PolyPointsTraslated.clear();

    for( unsigned int i = 0; i < m_PolyPoints.size() ; i++ )
        PolyPointsTraslated.push_back( aTransform.TransformCoordinate( m_PolyPoints[i] ) +
                                       aOffset );

    if( aColor < 0 )                // Used normal color or selected color
    {
        if( m_Selected & IS_SELECTED )
            color = g_ItemSelectetColor;
    }
    else
        color = aColor;

    FILL_T fill = aData ? NO_FILL : m_Fill;
    if( aColor >= 0 )
        fill = NO_FILL;

    GRSetDrawMode( aDC, aDrawMode );

    if( fill == FILLED_WITH_BG_BODYCOLOR )
        GRPoly( &aPanel->m_ClipBox, aDC, m_PolyPoints.size(),
                &PolyPointsTraslated[0], 1, GetPenSize(),
                (m_Flags & IS_MOVED) ? color : ReturnLayerColor( LAYER_DEVICE_BACKGROUND ),
                ReturnLayerColor( LAYER_DEVICE_BACKGROUND ) );
    else if( fill == FILLED_SHAPE  )
        GRPoly( &aPanel->m_ClipBox, aDC, m_PolyPoints.size(),
                &PolyPointsTraslated[0], 1, GetPenSize(), color, color );
    else
        GRPoly( &aPanel->m_ClipBox, aDC, m_PolyPoints.size(),
                &PolyPointsTraslated[0], 0, GetPenSize(), color, color );

    /* Set to one (1) to draw bounding box around bezier curve to validate
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
bool LIB_BEZIER::HitTest( const wxPoint& aRefPos )
{
    int mindist = m_Width ? m_Width / 2 : g_DrawDefaultLineThickness / 2;
    // Have a minimal tolerance for hit test
    if ( mindist < MINIMUM_SELECTION_DISTANCE )
        mindist = MINIMUM_SELECTION_DISTANCE;
    return HitTest( aRefPos, mindist, DefaultTransform );
}

/**
 * Function HitTest
 * @return if the point aPosRef is near a segment
 * @param aPosRef = a wxPoint to test
 * @param aThreshold = max distance to a segment
 * @param aTransform = the transform matrix
 */
bool LIB_BEZIER::HitTest( wxPoint aPosRef, int aThreshold, const TRANSFORM& aTransform )
{
    wxPoint ref, start, end;

    for( unsigned ii = 1; ii < GetCornerCount(); ii++ )
    {
        start = aTransform.TransformCoordinate( m_PolyPoints[ii - 1] );
        end   = aTransform.TransformCoordinate( m_PolyPoints[ii] );

        if ( TestSegmentHit( aPosRef, start, end, aThreshold ) )
            return true;
    }

    return false;
}


/**
 * Function GetBoundingBox
 * @return the boundary box for this, in library coordinates
 */
EDA_Rect LIB_BEZIER::GetBoundingBox() const
{
    EDA_Rect rect;
    int      xmin, xmax, ymin, ymax;

    if( !GetCornerCount() )
        return rect;

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


void LIB_BEZIER::DisplayInfo( EDA_DRAW_FRAME* aFrame )
{
    wxString msg;
    EDA_Rect bBox = GetBoundingBox();

    LIB_DRAW_ITEM::DisplayInfo( aFrame );

    msg = ReturnStringFromValue( g_UserUnit, m_Width, EESCHEMA_INTERNAL_UNIT, true );

    aFrame->AppendMsgPanel( _( "Line width" ), msg, BLUE );

    msg.Printf( wxT( "(%d, %d, %d, %d)" ), bBox.GetOrigin().x,
                bBox.GetOrigin().y, bBox.GetEnd().x, bBox.GetEnd().y );

    aFrame->AppendMsgPanel( _( "Bounding box" ), msg, BROWN );
}
