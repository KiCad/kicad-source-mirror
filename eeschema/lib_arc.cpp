/*******************/
/** class LIB_ARC **/
/*******************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "plot_common.h"
#include "trigo.h"

#include "general.h"
#include "protos.h"
#include "lib_arc.h"


LIB_ARC::LIB_ARC( LIB_COMPONENT* aParent ) : LIB_DRAW_ITEM( COMPONENT_ARC_DRAW_TYPE, aParent )
{
    m_Radius     = 0;
    m_t1         = 0;
    m_t2         = 0;
    m_Width      = 0;
    m_Fill       = NO_FILL;
    m_isFillable = true;
    m_typeName   = _( "Arc" );
}


LIB_ARC::LIB_ARC( const LIB_ARC& aArc ) : LIB_DRAW_ITEM( aArc )
{
    m_Radius   = aArc.m_Radius;
    m_t1       = aArc.m_t1;
    m_t2       = aArc.m_t2;
    m_Width    = aArc.m_Width;
    m_Fill     = aArc.m_Fill;
    m_Pos      = aArc.m_Pos;
    m_ArcStart = aArc.m_ArcStart;
    m_ArcEnd   = aArc.m_ArcEnd;
}


/**
 * format:
 *  A centre_posx centre_posy rayon start_angle end_angle unit convert
 *  fill('N', 'F' ou 'f') startx starty endx endy
 */
bool LIB_ARC::Save( FILE* aFile )
{
    int x1 = m_t1;

    if( x1 > 1800 )
        x1 -= 3600;

    int x2 = m_t2;

    if( x2 > 1800 )
        x2 -= 3600;

    if( fprintf( aFile, "A %d %d %d %d %d %d %d %d %c %d %d %d %d\n",
                 m_Pos.x, m_Pos.y, m_Radius, x1, x2, m_Unit, m_Convert, m_Width,
                 fill_tab[m_Fill], m_ArcStart.x, m_ArcStart.y, m_ArcEnd.x,
                 m_ArcEnd.y ) < 0 )
        return false;

    return true;
}


bool LIB_ARC::Load( char* aLine, wxString& aErrorMsg )
{
    int  startx, starty, endx, endy, cnt;
    char tmp[256];

    cnt = sscanf( &aLine[2], "%d %d %d %d %d %d %d %d %s %d %d %d %d",
                  &m_Pos.x, &m_Pos.y, &m_Radius, &m_t1, &m_t2, &m_Unit,
                  &m_Convert, &m_Width, tmp, &startx, &starty, &endx, &endy );
    if( cnt < 8 )
    {
        aErrorMsg.Printf( _( "arc only had %d parameters of the required 8" ), cnt );
        return false;
    }

    if( tmp[0] == 'F' )
        m_Fill = FILLED_SHAPE;
    if( tmp[0] == 'f' )
        m_Fill = FILLED_WITH_BG_BODYCOLOR;

    NORMALIZE_ANGLE( m_t1 );
    NORMALIZE_ANGLE( m_t2 );

    // Actual Coordinates of arc ends are read from file
    if( cnt >= 13 )
    {
        m_ArcStart.x = startx;
        m_ArcStart.y = starty;
        m_ArcEnd.x   = endx;
        m_ArcEnd.y   = endy;
    }
    else
    {
        // Actual Coordinates of arc ends are not read from file
        // (old library), calculate them
        m_ArcStart.x = m_Radius;
        m_ArcStart.y = 0;
        m_ArcEnd.x   = m_Radius;
        m_ArcEnd.y   = 0;
        RotatePoint( &m_ArcStart.x, &m_ArcStart.y, -m_t1 );
        m_ArcStart.x += m_Pos.x;
        m_ArcStart.y += m_Pos.y;
        RotatePoint( &m_ArcEnd.x, &m_ArcEnd.y, -m_t2 );
        m_ArcEnd.x += m_Pos.x;
        m_ArcEnd.y += m_Pos.y;
    }

    return true;
}


/**
 * Function HitTest
 * tests if the given wxPoint is within the bounds of this object.
 * @param aRefPoint A wxPoint to test in eeschema space
 * @return bool - true if a hit, else false
 */
bool LIB_ARC::HitTest( const wxPoint& aRefPoint )
{
    int mindist = m_Width ? m_Width / 2 : g_DrawDefaultLineThickness / 2;

    // Have a minimal tolerance for hit test
    if( mindist < MINIMUM_SELECTION_DISTANCE )
        mindist = MINIMUM_SELECTION_DISTANCE;

    return HitTest( aRefPoint, mindist, DefaultTransformMatrix );
}

/** Function HitTest
 * @return true if the point aPosRef is near this object
 * @param aRefPoint = a wxPoint to test
 * @param aThreshold = max distance to this object (usually the half thickness
 *                     of a line)
 * @param aTransMat = the transform matrix
 */
bool LIB_ARC::HitTest( wxPoint aReferencePoint, int aThreshold,
                       const int aTransformationMatrix[2][2] )
{

    // TODO: use aTransMat to calculmates parameters
    wxPoint relativePosition = aReferencePoint;

    NEGATE( relativePosition.y );       // reverse Y axis

    int distance = wxRound( EuclideanNorm( TwoPointVector(m_Pos, relativePosition) ) );

    if( abs( distance - m_Radius ) > aThreshold )
        return false;

    // We are on the circle, ensure we are only on the arc, i.e. between
    //  m_ArcStart and m_ArcEnd

    wxPoint startEndVector = TwoPointVector( m_ArcStart, m_ArcEnd);
    wxPoint startRelativePositionVector = TwoPointVector( m_ArcStart, relativePosition );

    wxPoint centerStartVector = TwoPointVector( m_Pos, m_ArcStart);
    wxPoint centerEndVector = TwoPointVector( m_Pos, m_ArcEnd);
    wxPoint centerRelativePositionVector = TwoPointVector( m_Pos, relativePosition );

    // Compute the cross product to check if the point is in the sector
    int crossProductStart = CrossProduct( centerStartVector, centerRelativePositionVector );
    int crossProductEnd = CrossProduct( centerEndVector, centerRelativePositionVector );

    // The cross products need to be exchanged, depending on which side the center point
    // relative to the start point to end point vector lies
    if( CrossProduct( startEndVector, startRelativePositionVector ) < 0 )
    {
    	EXCHG(crossProductStart, crossProductEnd);
    }

    // When the cross products have a different sign, the point lies in sector
    // also check, if the reference is near start or end point
    return 	HitTestPoints( m_ArcStart, relativePosition, MINIMUM_SELECTION_DISTANCE ) ||
    		HitTestPoints( m_ArcEnd, relativePosition, MINIMUM_SELECTION_DISTANCE ) ||
    		( crossProductStart <= 0 && crossProductEnd >= 0 );
}


LIB_DRAW_ITEM* LIB_ARC::DoGenCopy()
{
    LIB_ARC* newitem = new LIB_ARC( GetParent() );

    newitem->m_Pos      = m_Pos;
    newitem->m_ArcStart = m_ArcStart;
    newitem->m_ArcEnd   = m_ArcEnd;
    newitem->m_Radius   = m_Radius;
    newitem->m_t1       = m_t1;
    newitem->m_t2       = m_t2;
    newitem->m_Width    = m_Width;
    newitem->m_Unit     = m_Unit;
    newitem->m_Convert  = m_Convert;
    newitem->m_Flags    = m_Flags;
    newitem->m_Fill     = m_Fill;

    return (LIB_DRAW_ITEM*) newitem;
}


int LIB_ARC::DoCompare( const LIB_DRAW_ITEM& aOther ) const
{
    wxASSERT( aOther.Type() == COMPONENT_ARC_DRAW_TYPE );

    const LIB_ARC* tmp = ( LIB_ARC* ) &aOther;

    if( m_Pos.x != tmp->m_Pos.x )
        return m_Pos.x - tmp->m_Pos.x;

    if( m_Pos.y != tmp->m_Pos.y )
        return m_Pos.y - tmp->m_Pos.y;

    if( m_t1 != tmp->m_t1 )
        return m_t1 - tmp->m_t1;

    if( m_t2 != tmp->m_t2 )
        return m_t2 - tmp->m_t2;

    return 0;
}


void LIB_ARC::DoOffset( const wxPoint& aOffset )
{
    m_Pos += aOffset;
    m_ArcStart += aOffset;
    m_ArcEnd += aOffset;
}


bool LIB_ARC::DoTestInside( EDA_Rect& aRect )
{
    return aRect.Inside( m_ArcStart.x, -m_ArcStart.y )
        || aRect.Inside( m_ArcEnd.x, -m_ArcEnd.y );
}


void LIB_ARC::DoMove( const wxPoint& aPosition )
{
    wxPoint offset = aPosition - m_Pos;
    m_Pos = aPosition;
    m_ArcStart += offset;
    m_ArcEnd   += offset;
}


void LIB_ARC::DoMirrorHorizontal( const wxPoint& aCenter )
{
    m_Pos.x -= aCenter.x;
    m_Pos.x *= -1;
    m_Pos.x += aCenter.x;
    m_ArcStart.x -= aCenter.x;
    m_ArcStart.x *= -1;
    m_ArcStart.x += aCenter.x;
    m_ArcEnd.x -= aCenter.x;
    m_ArcEnd.x *= -1;
    m_ArcEnd.x += aCenter.x;
    EXCHG( m_ArcStart, m_ArcEnd );
}


void LIB_ARC::DoPlot( PLOTTER* aPlotter, const wxPoint& aOffset, bool aFill,
                      const int aTransform[2][2] )
{
    wxASSERT( aPlotter != NULL );

    int t1 = m_t1;
    int t2 = m_t2;
    wxPoint pos = TransformCoordinate( aTransform, m_Pos ) + aOffset;

    MapAngles( &t1, &t2, aTransform );

    if( aFill && m_Fill == FILLED_WITH_BG_BODYCOLOR )
    {
        aPlotter->set_color( ReturnLayerColor( LAYER_DEVICE_BACKGROUND ) );
        aPlotter->arc( pos, -t2, -t1, m_Radius, FILLED_SHAPE, 0 );
    }

    aPlotter->set_color( ReturnLayerColor( LAYER_DEVICE ) );
    aPlotter->arc( pos, -t2, -t1, m_Radius, m_Fill, GetPenSize() );
}


/** Function GetPenSize
 * @return the size of the "pen" that be used to draw or plot this item
 */
int LIB_ARC::GetPenSize()
{
    return ( m_Width == 0 ) ? g_DrawDefaultLineThickness : m_Width;
}


void LIB_ARC::Draw( WinEDA_DrawPanel* aPanel, wxDC* aDC,
                    const wxPoint& aOffset, int aColor, int aDrawMode,
                    void* aData, const int aTransformMatrix[2][2] )
{
    wxPoint pos1, pos2, posc;

    int     color = ReturnLayerColor( LAYER_DEVICE );

    if( aColor < 0 )       // Used normal color or selected color
    {
        if( ( m_Selected & IS_SELECTED ) )
            color = g_ItemSelectetColor;
    }
    else
        color = aColor;

    pos1 = TransformCoordinate( aTransformMatrix, m_ArcEnd ) + aOffset;
    pos2 = TransformCoordinate( aTransformMatrix, m_ArcStart ) + aOffset;
    posc = TransformCoordinate( aTransformMatrix, m_Pos ) + aOffset;
    int  pt1  = m_t1;
    int  pt2  = m_t2;
    bool swap = MapAngles( &pt1, &pt2, aTransformMatrix );
    if( swap )
    {
        EXCHG( pos1.x, pos2.x );
        EXCHG( pos1.y, pos2.y );
    }

    GRSetDrawMode( aDC, aDrawMode );

    FILL_T fill = aData ? NO_FILL : m_Fill;
    if( aColor >= 0 )
        fill = NO_FILL;

    if( fill == FILLED_WITH_BG_BODYCOLOR )
        GRFilledArc( &aPanel->m_ClipBox, aDC, posc.x, posc.y, pt1, pt2,
                     m_Radius, GetPenSize( ),
                     (m_Flags & IS_MOVED) ? color : ReturnLayerColor( LAYER_DEVICE_BACKGROUND ),
                     ReturnLayerColor( LAYER_DEVICE_BACKGROUND ) );
    else if( fill == FILLED_SHAPE && !aData )
        GRFilledArc( &aPanel->m_ClipBox, aDC, posc.x, posc.y, pt1, pt2,
                     m_Radius, color, color );
    else
    {
#ifdef DRAW_ARC_WITH_ANGLE

        GRArc( &aPanel->m_ClipBox, aDC, posc.x, posc.y, pt1, pt2,
               m_Radius, GetPenSize( ), color );
#else

        GRArc1( &aPanel->m_ClipBox, aDC, pos1.x, pos1.y, pos2.x, pos2.y,
                posc.x, posc.y, GetPenSize( ), color );
#endif
    }

    /* Set to one (1) to draw bounding box around arc to validate bounding box
     * calculation. */
#if 0
    EDA_Rect bBox = GetBoundingBox();
    GRRect( &aPanel->m_ClipBox, aDC, bBox.GetOrigin().x, bBox.GetOrigin().y,
            bBox.GetEnd().x, bBox.GetEnd().y, 0, LIGHTMAGENTA );
#endif
}


EDA_Rect LIB_ARC::GetBoundingBox()
{
    int      minX, minY, maxX, maxY, angleStart, angleEnd;
    EDA_Rect rect;
    wxPoint  nullPoint, startPos, endPos, centerPos;
    wxPoint  normStart = m_ArcStart - m_Pos;
    wxPoint  normEnd   = m_ArcEnd - m_Pos;

    if( ( normStart == nullPoint ) || ( normEnd == nullPoint ) || ( m_Radius == 0 ) )
    {
        wxLogDebug( wxT("Invalid arc drawing definition, center(%d, %d) \
start(%d, %d), end(%d, %d), radius %d" ),
                    m_Pos.x, m_Pos.y, m_ArcStart.x, m_ArcStart.y, m_ArcEnd.x,
                    m_ArcEnd.y, m_Radius );
        return rect;
    }

    endPos     = TransformCoordinate( DefaultTransformMatrix, m_ArcEnd );
    startPos   = TransformCoordinate( DefaultTransformMatrix, m_ArcStart );
    centerPos  = TransformCoordinate( DefaultTransformMatrix, m_Pos );
    angleStart = m_t1;
    angleEnd   = m_t2;

    if( MapAngles( &angleStart, &angleEnd, DefaultTransformMatrix ) )
    {
        EXCHG( endPos.x, startPos.x );
        EXCHG( endPos.y, startPos.y );
    }

    /* Start with the start and end point of the arc. */
    minX = MIN( startPos.x, endPos.x );
    minY = MIN( startPos.y, endPos.y );
    maxX = MAX( startPos.x, endPos.x );
    maxY = MAX( startPos.y, endPos.y );

    /* Zero degrees is a special case. */
    if( angleStart == 0 )
        maxX = centerPos.x + m_Radius;

    /* Arc end angle wrapped passed 360. */
    if( angleStart > angleEnd )
        angleEnd += 3600;

    if( angleStart <= 900 && angleEnd >= 900 )          /* 90 deg */
        maxY = centerPos.y + m_Radius;
    if( angleStart <= 1800 && angleEnd >= 1800 )        /* 180 deg */
        minX = centerPos.x - m_Radius;
    if( angleStart <= 2700 && angleEnd >= 2700 )        /* 270 deg */
        minY = centerPos.y - m_Radius;
    if( angleStart <= 3600 && angleEnd >= 3600 )        /* 0 deg   */
        maxX = centerPos.x + m_Radius;

    rect.SetOrigin( minX, minY );
    rect.SetEnd( maxX, maxY );
    rect.Inflate( m_Width / 2, m_Width / 2 );

    return rect;
}


void LIB_ARC::DisplayInfo( WinEDA_DrawFrame* aFrame )
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
