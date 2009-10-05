/************************/
/* class_body_items.cpp */
/************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "plot_common.h"
#include "drawtxt.h"
#include "trigo.h"
#include "bezier_curves.h"

#include "program.h"
#include "general.h"
#include "protos.h"
#include "classes_body_items.h"


static int fill_tab[3] = { 'N', 'F', 'f' };

//#define DRAW_ARC_WITH_ANGLE       // Used to draw arcs


/* Base class (abstract) for components bodies items */
LIB_DRAW_ITEM::LIB_DRAW_ITEM( KICAD_T struct_type, LIB_COMPONENT* aParent ) :
    EDA_BaseStruct( struct_type )
{
    m_Unit     = 0;  /* Unit identification (for multi part per package)
                      *  0 if the item is common to all units */
    m_Convert  = 0;  /* Shape identification (for parts which have a convert
                      * shape) 0 if the item is common to all shapes */
    m_Fill     = NO_FILL;
    m_Parent   = (EDA_BaseStruct*) aParent;
    m_typeName = _( "Undefined" );
}


LIB_DRAW_ITEM::LIB_DRAW_ITEM( const LIB_DRAW_ITEM& item ) :
    EDA_BaseStruct( item )
{
    m_Unit = item.m_Unit;
    m_Convert = item.m_Convert;
    m_Fill = item.m_Fill;
    m_Parent = item.m_Parent;
    m_typeName = item.m_typeName;
}


/**
 * Update the message panel information with the drawing information.
 *
 * This base function is used to display the information common to the
 * all library items.  Call the base class from the derived class or the
 * common information will not be updated in the message panel.
 */
void LIB_DRAW_ITEM::DisplayInfo( WinEDA_DrawFrame* frame )
{
    wxString msg;

    frame->MsgPanel->EraseMsgBox();

    frame->MsgPanel->Affiche_1_Parametre( 1, _( "Type" ), m_typeName, CYAN );

    /* Affichage de l'appartenance */
    if( m_Unit == 0 )
        msg = _( "All" );
    else
        msg.Printf( wxT( "%d" ), m_Unit );
    Affiche_1_Parametre( frame, 8, _( "Unit" ), msg, BROWN );

    if( m_Convert == 0 )
        msg = _( "All" );
    else if( m_Convert == 1 )
        msg = _( "no" );
    else if( m_Convert == 2 )
        msg = _( "yes" );
    else
        msg = wxT( "?" );
    Affiche_1_Parametre( frame, 14, _( "Convert" ), msg, BROWN );
}


bool LIB_DRAW_ITEM::operator==( const LIB_DRAW_ITEM& other ) const
{
    return ( ( Type() == other.Type() )
             && ( m_Unit == other.m_Unit )
             && ( m_Convert == other.m_Convert )
             && DoCompare( other ) == 0 );
}


bool LIB_DRAW_ITEM::operator<( const LIB_DRAW_ITEM& other ) const
{
    int result = m_Convert - other.m_Convert;

    if( result != 0 )
        return result < 0;

    result = m_Unit - other.m_Unit;

    if( result != 0 )
        return result < 0;

    result = Type() - other.Type();

    if( result != 0 )
        return result < 0;

    return ( DoCompare( other ) < 0 );
}


/**********************/
/** class LibDrawArc **/
/**********************/

LibDrawArc::LibDrawArc( LIB_COMPONENT* aParent ) :
    LIB_DRAW_ITEM( COMPONENT_ARC_DRAW_TYPE, aParent )
{
    m_Radius   = 0;
    m_t1       = 0;
    m_t2       = 0;
    m_Width    = 0;
    m_Fill     = NO_FILL;
    m_typeName = _( "Arc" );
}


LibDrawArc::LibDrawArc( const LibDrawArc& arc ) : LIB_DRAW_ITEM( arc )
{
    m_Radius   = arc.m_Radius;
    m_t1       = arc.m_t1;
    m_t2       = arc.m_t2;
    m_Width    = arc.m_Width;
    m_Fill     = arc.m_Fill;
    m_Pos      = arc.m_Pos;
    m_ArcStart = arc.m_ArcStart;
    m_ArcEnd   = arc.m_ArcEnd;
}


/**
 * format:
 *  A centre_posx centre_posy rayon start_angle end_angle unit convert
 *  fill('N', 'F' ou 'f') startx starty endx endy
 */
bool LibDrawArc::Save( FILE* ExportFile ) const
{
    int x1 = m_t1;

    if( x1 > 1800 )
        x1 -= 3600;

    int x2 = m_t2;

    if( x2 > 1800 )
        x2 -= 3600;

    if( fprintf( ExportFile, "A %d %d %d %d %d %d %d %d %c %d %d %d %d\n",
                 m_Pos.x, m_Pos.y, m_Radius, x1, x2, m_Unit, m_Convert, m_Width,
                 fill_tab[m_Fill], m_ArcStart.x, m_ArcStart.y, m_ArcEnd.x,
                 m_ArcEnd.y ) < 0 )
        return false;

    return true;
}


bool LibDrawArc::Load( char* line, wxString& errorMsg )
{
    int  startx, starty, endx, endy, cnt;
    char tmp[256];

    cnt = sscanf( &line[2], "%d %d %d %d %d %d %d %d %s %d %d %d %d",
                  &m_Pos.x, &m_Pos.y, &m_Radius, &m_t1, &m_t2, &m_Unit,
                  &m_Convert, &m_Width, tmp, &startx, &starty, &endx, &endy );
    if( cnt < 8 )
    {
        errorMsg.Printf( _( "arc only had %d parameters of the required 8" ),
                         cnt );
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
bool LibDrawArc::HitTest( const wxPoint& aRefPoint )
{
    int mindist = m_Width ? m_Width / 2 : g_DrawDefaultLineThickness / 2;

    // Have a minimal tolerance for hit test
    if( mindist < 3 )
        mindist = 3;        // = 3 mils

    return HitTest( aRefPoint, mindist, DefaultTransformMatrix );
}


/** Function HitTest
 * @return true if the point aPosRef is near this object
 * @param aRefPoint = a wxPoint to test
 * @param aThreshold = max distance to this object (usually the half thickness
 *                     of a line)
 * @param aTransMat = the transform matrix
 */
bool LibDrawArc::HitTest( wxPoint aRefPoint, int aThreshold,
                          const int aTransMat[2][2] )
{
    // TODO: use aTransMat to calculmates parameters
    wxPoint relpos = aRefPoint;

    NEGATE( relpos.y );       // reverse Y axis

    relpos -= m_Pos;
    int dist = wxRound( sqrt( ( (double) relpos.x * (double) relpos.x ) +
                              ( (double) relpos.y * (double) relpos.y ) ) );

    if( abs( dist - m_Radius ) > aThreshold )
        return false;

    // We are on the circle, ensure we are only on the arc, i.e. between
    //  m_ArcStart and m_ArcEnd
    int astart = m_t1;    // arc starting point ( in 0.1 degree)
    int aend   = m_t2;    // arc ending point ( in 0.1 degree)
    int atest  = wxRound( atan2( (double) relpos.y,
                                 (double) relpos.x ) * 1800.0 / M_PI );
    NORMALIZE_ANGLE_180( atest );
    NORMALIZE_ANGLE_180( astart );
    NORMALIZE_ANGLE_180( aend );

    if( astart > aend )
        EXCHG( astart, aend );

    if( atest >= astart && atest <= aend )
        return true;

    return false;
}


LIB_DRAW_ITEM* LibDrawArc::DoGenCopy()
{
    LibDrawArc* newitem = new LibDrawArc( GetParent() );

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


int LibDrawArc::DoCompare( const LIB_DRAW_ITEM& other ) const
{
    wxASSERT( other.Type() == COMPONENT_ARC_DRAW_TYPE );

    const LibDrawArc* tmp = ( LibDrawArc* ) &other;

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


void LibDrawArc::DoOffset( const wxPoint& offset )
{
    m_Pos += offset;
    m_ArcStart += offset;
    m_ArcEnd += offset;
}


bool LibDrawArc::DoTestInside( EDA_Rect& rect )
{
    return rect.Inside( m_ArcStart.x, -m_ArcStart.y )
        || rect.Inside( m_ArcEnd.x, -m_ArcEnd.y );
}


void LibDrawArc::DoMove( const wxPoint& newPosition )
{
    wxPoint offset = newPosition - m_Pos;
    m_Pos = newPosition;
    m_ArcStart += offset;
    m_ArcEnd   += offset;
}


void LibDrawArc::DoMirrorHorizontal( const wxPoint& center )
{
    m_Pos.x -= center.x;
    m_Pos.x *= -1;
    m_Pos.x += center.x;
    m_ArcStart.x -= center.x;
    m_ArcStart.x *= -1;
    m_ArcStart.x += center.x;
    m_ArcEnd.x -= center.x;
    m_ArcEnd.x *= -1;
    m_ArcEnd.x += center.x;
    EXCHG( m_ArcStart, m_ArcEnd );
}


void LibDrawArc::DoPlot( PLOTTER* plotter, const wxPoint& offset, bool fill,
                         const int transform[2][2] )
{
    wxASSERT( plotter != NULL );

    int t1 = m_t1;
    int t2 = m_t2;
    wxPoint pos = TransformCoordinate( transform, m_Pos ) + offset;

    MapAngles( &t1, &t2, transform );

    if( fill && m_Fill == FILLED_WITH_BG_BODYCOLOR )
    {
        plotter->set_color( ReturnLayerColor( LAYER_DEVICE_BACKGROUND ) );
        plotter->arc( pos, -t2, -t1, m_Radius, FILLED_SHAPE, 0 );
    }

    plotter->set_color( ReturnLayerColor( LAYER_DEVICE ) );
    plotter->arc( pos, -t2, -t1, m_Radius, m_Fill, GetPenSize() );
}


/** Function GetPenSize
 * @return the size of the "pen" that be used to draw or plot this item
 */
int LibDrawArc::GetPenSize()
{
    return ( m_Width == 0 ) ? g_DrawDefaultLineThickness : m_Width;
}


void LibDrawArc::Draw( WinEDA_DrawPanel* aPanel, wxDC* aDC,
                       const wxPoint& aOffset, int aColor,
                       int aDrawMode, void* aData,
                       const int aTransformMatrix[2][2] )
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
                     m_Radius, GetPenSize( ), color,
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


EDA_Rect LibDrawArc::GetBoundingBox()
{
    int      minX, minY, maxX, maxY, angleStart, angleEnd;
    EDA_Rect rect;
    wxPoint  nullPoint, startPos, endPos, centerPos;
    wxPoint  normStart = m_ArcStart - m_Pos;
    wxPoint  normEnd   = m_ArcEnd - m_Pos;

    if( ( normStart == nullPoint ) || ( normEnd == nullPoint )
       || ( m_Radius == 0 ) )
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


void LibDrawArc::DisplayInfo( WinEDA_DrawFrame* frame )
{
    wxString msg;
    EDA_Rect bBox = GetBoundingBox();

    LIB_DRAW_ITEM::DisplayInfo( frame );

    msg = ReturnStringFromValue( g_UnitMetric, m_Width,
                                 EESCHEMA_INTERNAL_UNIT, true );

    frame->MsgPanel->Affiche_1_Parametre( 20, _( "Line width" ), msg, BLUE );

    msg.Printf( wxT( "(%d, %d, %d, %d)" ), bBox.GetOrigin().x,
                bBox.GetOrigin().y, bBox.GetEnd().x, bBox.GetEnd().y );

    frame->MsgPanel->Affiche_1_Parametre( 40, _( "Bounding box" ), msg, BROWN );
}


/*************************/
/** class LibDrawCircle **/
/*************************/

LibDrawCircle::LibDrawCircle( LIB_COMPONENT* aParent ) :
    LIB_DRAW_ITEM( COMPONENT_CIRCLE_DRAW_TYPE, aParent )
{
    m_Radius   = 0;
    m_Fill     = NO_FILL;
    m_typeName = _( "Circle" );
}


LibDrawCircle::LibDrawCircle( const LibDrawCircle& circle ) :
    LIB_DRAW_ITEM( circle )
{
    m_Pos    = circle.m_Pos;
    m_Radius = circle.m_Radius;
    m_Fill   = circle.m_Fill;
}


bool LibDrawCircle::Save( FILE* ExportFile ) const
{
    if( fprintf( ExportFile, "C %d %d %d %d %d %d %c\n", m_Pos.x, m_Pos.y,
                 m_Radius, m_Unit, m_Convert, m_Width, fill_tab[m_Fill] ) < 0 )
        return false;

    return true;
}


bool LibDrawCircle::Load( char* line, wxString& errorMsg )
{
    char tmp[256];

    int  cnt = sscanf( &line[2], "%d %d %d %d %d %d %s", &m_Pos.x, &m_Pos.y,
                       &m_Radius, &m_Unit, &m_Convert, &m_Width, tmp );

    if( cnt < 6 )
    {
        errorMsg.Printf( _( "circle only had %d parameters of the required 6" ),
                         cnt );
        return false;
    }

    if( tmp[0] == 'F' )
        m_Fill = FILLED_SHAPE;
    if( tmp[0] == 'f' )
        m_Fill = FILLED_WITH_BG_BODYCOLOR;

    return true;
}


/**
 * Function HitTest
 * tests if the given wxPoint is within the bounds of this object.
 * @param aRefPos A wxPoint to test in eeschema space
 * @return bool - true if a hit, else false
 */
bool LibDrawCircle::HitTest( const wxPoint& aPosRef )
{
    int mindist = m_Width ? m_Width / 2 : g_DrawDefaultLineThickness / 2;

    // Have a minimal tolerance for hit test
    if( mindist < 3 )
        mindist = 3;        // = 3 mils

    return HitTest( aPosRef, mindist, DefaultTransformMatrix );
}


/** Function HitTest
 * @return true if the point aPosRef is near this object
 * @param aPosRef = a wxPoint to test
 * @param aThreshold = max distance to this object (usually the half
 *                     thickness of a line)
 * @param aTransMat = the transform matrix
 */
bool LibDrawCircle::HitTest( wxPoint aPosRef, int aThreshold,
                             const int aTransMat[2][2] )
{
    wxPoint relpos = aPosRef - TransformCoordinate( aTransMat, m_Pos );

    int     dist =
        wxRound( sqrt( ( (double) relpos.x * relpos.x ) +
                       ( (double) relpos.y * relpos.y ) ) );

    if( abs( dist - m_Radius ) <= aThreshold )
        return true;
    return false;
}


LIB_DRAW_ITEM* LibDrawCircle::DoGenCopy()
{
    LibDrawCircle* newitem = new LibDrawCircle( GetParent() );

    newitem->m_Pos     = m_Pos;
    newitem->m_Radius  = m_Radius;
    newitem->m_Width   = m_Width;
    newitem->m_Unit    = m_Unit;
    newitem->m_Convert = m_Convert;
    newitem->m_Flags   = m_Flags;
    newitem->m_Fill    = m_Fill;

    return (LIB_DRAW_ITEM*) newitem;
}


int LibDrawCircle::DoCompare( const LIB_DRAW_ITEM& other ) const
{
    wxASSERT( other.Type() == COMPONENT_CIRCLE_DRAW_TYPE );

    const LibDrawCircle* tmp = ( LibDrawCircle* ) &other;

    if( m_Pos.x != tmp->m_Pos.x )
        return m_Pos.x - tmp->m_Pos.x;

    if( m_Pos.y != tmp->m_Pos.y )
        return m_Pos.y - tmp->m_Pos.y;

    if( m_Radius != tmp->m_Radius )
        return m_Radius - tmp->m_Radius;

    return 0;
}


void LibDrawCircle::DoOffset( const wxPoint& offset )
{
    m_Pos += offset;
}


bool LibDrawCircle::DoTestInside( EDA_Rect& rect )
{
    /*
     * FIXME: This fails to take into acount the radius around the center
     *        point.
     */
    return rect.Inside( m_Pos.x, -m_Pos.y );
}


void LibDrawCircle::DoMove( const wxPoint& newPosition )
{
    m_Pos = newPosition;
}


void LibDrawCircle::DoMirrorHorizontal( const wxPoint& center )
{
    m_Pos.x -= center.x;
    m_Pos.x *= -1;
    m_Pos.x += center.x;
}


void LibDrawCircle::DoPlot( PLOTTER* plotter, const wxPoint& offset, bool fill,
                            const int transform[2][2] )
{
    wxPoint pos = TransformCoordinate( transform, m_Pos ) + offset;

    if( fill && m_Fill == FILLED_WITH_BG_BODYCOLOR )
    {
        plotter->set_color( ReturnLayerColor( LAYER_DEVICE_BACKGROUND ) );
        plotter->circle( pos, m_Radius * 2, FILLED_SHAPE, 0 );
    }

    plotter->set_color( ReturnLayerColor( LAYER_DEVICE ) );
    plotter->circle( pos, m_Radius * 2, m_Fill, GetPenSize() );
}


/** Function GetPenSize
 * @return the size of the "pen" that be used to draw or plot this item
 */
int LibDrawCircle::GetPenSize()
{
    return ( m_Width == 0 ) ? g_DrawDefaultLineThickness : m_Width;
}


void LibDrawCircle::Draw( WinEDA_DrawPanel* aPanel, wxDC* aDC,
                          const wxPoint& aOffset, int aColor, int aDrawMode,
                          void* aData, const int aTransformMatrix[2][2] )
{
    wxPoint pos1;

    int     color = ReturnLayerColor( LAYER_DEVICE );

    if( aColor < 0 )       // Used normal color or selected color
    {
        if( ( m_Selected & IS_SELECTED ) )
            color = g_ItemSelectetColor;
    }
    else
        color = aColor;

    pos1 = TransformCoordinate( aTransformMatrix, m_Pos ) + aOffset;
    GRSetDrawMode( aDC, aDrawMode );

    FILL_T fill = aData ? NO_FILL : m_Fill;
    if( aColor >= 0 )
        fill = NO_FILL;

    if( fill == FILLED_WITH_BG_BODYCOLOR )
        GRFilledCircle( &aPanel->m_ClipBox, aDC, pos1.x, pos1.y,
                        m_Radius, GetPenSize( ), color,
                        ReturnLayerColor( LAYER_DEVICE_BACKGROUND ) );
    else if( fill == FILLED_SHAPE )
        GRFilledCircle( &aPanel->m_ClipBox, aDC, pos1.x, pos1.y,
                        m_Radius, 0, color, color );
    else
        GRCircle( &aPanel->m_ClipBox, aDC, pos1.x, pos1.y,
                  m_Radius, GetPenSize( ), color );

    /* Set to one (1) to draw bounding box around circle to validate bounding
     * box calculation. */
#if 0
    EDA_Rect bBox = GetBoundingBox();
    GRRect( &aPanel->m_ClipBox, aDC, bBox.GetOrigin().x, bBox.GetOrigin().y,
            bBox.GetEnd().x, bBox.GetEnd().y, 0, LIGHTMAGENTA );
#endif
}


EDA_Rect LibDrawCircle::GetBoundingBox()
{
    EDA_Rect rect;

    rect.SetOrigin( m_Pos.x - m_Radius, ( m_Pos.y - m_Radius ) * -1 );
    rect.SetEnd( m_Pos.x + m_Radius, ( m_Pos.y + m_Radius ) * -1 );
    rect.Inflate( m_Width / 2, m_Width / 2 );

    return rect;
}


void LibDrawCircle::DisplayInfo( WinEDA_DrawFrame* frame )
{
    wxString msg;
    EDA_Rect bBox = GetBoundingBox();

    LIB_DRAW_ITEM::DisplayInfo( frame );

    msg = ReturnStringFromValue( g_UnitMetric, m_Width,
                                 EESCHEMA_INTERNAL_UNIT, true );

    frame->MsgPanel->Affiche_1_Parametre( 20, _( "Line width" ), msg, BLUE );

    msg = ReturnStringFromValue( g_UnitMetric, m_Radius,
                                 EESCHEMA_INTERNAL_UNIT, true );
    frame->MsgPanel->Affiche_1_Parametre( 40, _( "Radius" ), msg, RED );

    msg.Printf( wxT( "(%d, %d, %d, %d)" ), bBox.GetOrigin().x,
                bBox.GetOrigin().y, bBox.GetEnd().x, bBox.GetEnd().y );

    frame->MsgPanel->Affiche_1_Parametre( 60, _( "Bounding box" ), msg, BROWN );
}


/*************************/
/** class LibDrawSquare **/
/*************************/

LibDrawSquare::LibDrawSquare( LIB_COMPONENT* aParent ) :
    LIB_DRAW_ITEM( COMPONENT_RECT_DRAW_TYPE, aParent )
{
    m_Width    = 0;
    m_Fill     = NO_FILL;
    m_typeName = _( "Rectangle" );
}


LibDrawSquare::LibDrawSquare( const LibDrawSquare& rect ) :
    LIB_DRAW_ITEM( rect )
{
    m_Pos   = rect.m_Pos;
    m_End   = rect.m_End;
    m_Width = rect.m_Width;
    m_Fill  = rect.m_Fill;
}


bool LibDrawSquare::Save( FILE* ExportFile ) const
{
    if( fprintf( ExportFile, "S %d %d %d %d %d %d %d %c\n", m_Pos.x, m_Pos.y,
                 m_End.x, m_End.y, m_Unit, m_Convert, m_Width,
                 fill_tab[m_Fill] ) < 0 )
        return false;

    return true;
}


bool LibDrawSquare::Load( char* line, wxString& errorMsg )
{
    int  cnt;
    char tmp[256];

    cnt = sscanf( &line[2], "%d %d %d %d %d %d %d %s", &m_Pos.x, &m_Pos.y,
                  &m_End.x, &m_End.y, &m_Unit, &m_Convert, &m_Width, tmp );

    if( cnt < 7 )
    {
        errorMsg.Printf( _( "rectangle only had %d parameters of the required 7" ),
                         cnt );
        return false;
    }

    if( tmp[0] == 'F' )
        m_Fill = FILLED_SHAPE;
    if( tmp[0] == 'f' )
        m_Fill = FILLED_WITH_BG_BODYCOLOR;

    return true;
}


LIB_DRAW_ITEM* LibDrawSquare::DoGenCopy()
{
    LibDrawSquare* newitem = new LibDrawSquare( GetParent() );

    newitem->m_Pos     = m_Pos;
    newitem->m_End     = m_End;
    newitem->m_Width   = m_Width;
    newitem->m_Unit    = m_Unit;
    newitem->m_Convert = m_Convert;
    newitem->m_Flags   = m_Flags;
    newitem->m_Fill    = m_Fill;

    return (LIB_DRAW_ITEM*) newitem;
}


int LibDrawSquare::DoCompare( const LIB_DRAW_ITEM& other ) const
{
    wxASSERT( other.Type() == COMPONENT_RECT_DRAW_TYPE );

    const LibDrawSquare* tmp = ( LibDrawSquare* ) &other;

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


void LibDrawSquare::DoOffset( const wxPoint& offset )
{
    m_Pos += offset;
    m_End += offset;
}


bool LibDrawSquare::DoTestInside( EDA_Rect& rect )
{
    return rect.Inside( m_Pos.x, -m_Pos.y ) || rect.Inside( m_End.x, -m_End.y );
}


void LibDrawSquare::DoMove( const wxPoint& newPosition )
{
    wxPoint size = m_End - m_Pos;
    m_Pos = newPosition;
    m_End = newPosition + size;
}


void LibDrawSquare::DoMirrorHorizontal( const wxPoint& center )
{
    m_Pos.x -= center.x;
    m_Pos.x *= -1;
    m_Pos.x += center.x;
    m_End.x -= center.x;
    m_End.x *= -1;
    m_End.x += center.x;
}


void LibDrawSquare::DoPlot( PLOTTER* plotter, const wxPoint& offset, bool fill,
                            const int transform[2][2] )
{
    wxASSERT( plotter != NULL );

    wxPoint pos = TransformCoordinate( transform, m_Pos ) + offset;
    wxPoint end = TransformCoordinate( transform, m_End ) + offset;

    if( fill && m_Fill == FILLED_WITH_BG_BODYCOLOR )
    {
        plotter->set_color( ReturnLayerColor( LAYER_DEVICE_BACKGROUND ) );
        plotter->rect( pos, end, FILLED_WITH_BG_BODYCOLOR, 0 );
    }

    plotter->set_color( ReturnLayerColor( LAYER_DEVICE ) );
    plotter->rect( pos, end, m_Fill, GetPenSize() );
}


/** Function GetPenSize
 * @return the size of the "pen" that be used to draw or plot this item
 */
int LibDrawSquare::GetPenSize()
{
    return ( m_Width == 0 ) ? g_DrawDefaultLineThickness : m_Width;
}

void LibDrawSquare::Draw( WinEDA_DrawPanel* aPanel, wxDC* aDC,
                          const wxPoint& aOffset, int aColor, int aDrawMode,
                          void* aData, const int aTransformMatrix[2][2] )
{
    wxPoint pos1, pos2;

    int     color = ReturnLayerColor( LAYER_DEVICE );

    if( aColor < 0 )       // Used normal color or selected color
    {
        if( m_Selected & IS_SELECTED )
            color = g_ItemSelectetColor;
    }
    else
        color = aColor;

    pos1 = TransformCoordinate( aTransformMatrix, m_Pos ) + aOffset;
    pos2 = TransformCoordinate( aTransformMatrix, m_End ) + aOffset;

    FILL_T fill = aData ? NO_FILL : m_Fill;
    if( aColor >= 0 )
        fill = NO_FILL;

    GRSetDrawMode( aDC, aDrawMode );

    if( fill == FILLED_WITH_BG_BODYCOLOR && !aData )
        GRFilledRect( &aPanel->m_ClipBox, aDC, pos1.x, pos1.y, pos2.x, pos2.y,
                      GetPenSize( ), color,
                      ReturnLayerColor( LAYER_DEVICE_BACKGROUND ) );
    else if( m_Fill == FILLED_SHAPE  && !aData )
        GRFilledRect( &aPanel->m_ClipBox, aDC, pos1.x, pos1.y, pos2.x, pos2.y,
                      GetPenSize( ), color, color );
    else
        GRRect( &aPanel->m_ClipBox, aDC, pos1.x, pos1.y, pos2.x, pos2.y,
                GetPenSize( ), color );

    /* Set to one (1) to draw bounding box around rectangle to validate
     * bounding box calculation. */
#if 0
    EDA_Rect bBox = GetBoundingBox();
    bBox.Inflate( m_Width + 1, m_Width + 1 );
    GRRect( &aPanel->m_ClipBox, aDC, bBox.GetOrigin().x, bBox.GetOrigin().y,
            bBox.GetEnd().x, bBox.GetEnd().y, 0, LIGHTMAGENTA );
#endif
}


void LibDrawSquare::DisplayInfo( WinEDA_DrawFrame* frame )
{
    wxString msg;

    LIB_DRAW_ITEM::DisplayInfo( frame );

    msg = ReturnStringFromValue( g_UnitMetric, m_Width,
                                 EESCHEMA_INTERNAL_UNIT, true );

    frame->MsgPanel->Affiche_1_Parametre( 20, _( "Line width" ), msg, BLUE );
}


EDA_Rect LibDrawSquare::GetBoundingBox()
{
    EDA_Rect rect;

    rect.SetOrigin( m_Pos.x, m_Pos.y * -1 );
    rect.SetEnd( m_End.x, m_End.y * -1 );
    rect.Inflate( m_Width / 2, m_Width / 2 );
    return rect;
}


/**
 * Function HitTest
 * tests if the given wxPoint is within the bounds of this object.
 * @param aRefPoint A wxPoint to test in eeschema space
 * @return bool - true if a hit, else false
 */
bool LibDrawSquare::HitTest( const wxPoint& aRefPoint )
{
    int mindist = (m_Width ? m_Width / 2 : g_DrawDefaultLineThickness / 2) + 1;

    // Have a minimal tolerance for hit test
    if( mindist < 3 )
        mindist = 3;        // = 3 mils

    return HitTest( aRefPoint, mindist, DefaultTransformMatrix );
}


/** Function HitTest
 * @return true if the point aPosRef is near this object
 * @param aRefPoint = a wxPoint to test
 * @param aThreshold = max distance to this object (usually the half thickness
 *                     of a line)
 * @param aTransMat = the transform matrix
 */
bool LibDrawSquare::HitTest( wxPoint aRefPoint, int aThreshold,
                             const int aTransMat[2][2] )
{
    wxPoint actualStart = TransformCoordinate( aTransMat, m_Pos );
    wxPoint actualEnd   = TransformCoordinate( aTransMat, m_End );

    // locate lower segment
    wxPoint start, end;

    start = actualStart;
    end.x = actualEnd.x;
    end.y = actualStart.y;
    if( TestSegmentHit( aRefPoint, start, end, aThreshold ) )
        return true;

    // locate right segment
    start.x = actualEnd.x;
    end.y   = actualEnd.y;
    if( TestSegmentHit( aRefPoint, start, end, aThreshold ) )
        return true;

    // locate upper segment
    start.y = actualEnd.y;
    end.x   = actualStart.x;
    if( TestSegmentHit( aRefPoint, start, end, aThreshold ) )
        return true;

    // locate left segment
    start.x = actualStart.x;
    end.x   = actualStart.y;
    if( TestSegmentHit( aRefPoint, start, end, aThreshold ) )
        return true;

    return false;
}


/**************************/
/** class LibDrawSegment **/
/**************************/
LibDrawSegment::LibDrawSegment( LIB_COMPONENT* aParent ) :
    LIB_DRAW_ITEM( COMPONENT_LINE_DRAW_TYPE, aParent )
{
    m_Width    = 0;
    m_typeName = _( "Segment" );
}


LibDrawSegment::LibDrawSegment( const LibDrawSegment& segment ) :
    LIB_DRAW_ITEM( segment )
{
    m_Pos   = segment.m_Pos;
    m_End   = segment.m_End;
    m_Width = segment.m_Width;
}


bool LibDrawSegment::Save( FILE* ExportFile ) const
{
    if( fprintf( ExportFile, "L %d %d %d", m_Unit, m_Convert, m_Width ) )
        return false;

    return true;
}


bool LibDrawSegment::Load( char* line, wxString& errorMsg )
{
    return true;
}


LIB_DRAW_ITEM* LibDrawSegment::DoGenCopy()
{
    LibDrawSegment* newitem = new LibDrawSegment( GetParent() );

    newitem->m_Pos     = m_Pos;
    newitem->m_End     = m_End;
    newitem->m_Width   = m_Width;
    newitem->m_Unit    = m_Unit;
    newitem->m_Convert = m_Convert;
    newitem->m_Flags   = m_Flags;

    return (LIB_DRAW_ITEM*) newitem;
}


int LibDrawSegment::DoCompare( const LIB_DRAW_ITEM& other ) const
{
    wxASSERT( other.Type() == COMPONENT_LINE_DRAW_TYPE );

    const LibDrawSegment* tmp = ( LibDrawSegment* ) &other;

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


void LibDrawSegment::DoOffset( const wxPoint& offset )
{
    m_Pos += offset;
    m_End += offset;
}


bool LibDrawSegment::DoTestInside( EDA_Rect& rect )
{
    return rect.Inside( m_Pos.x, -m_Pos.y ) || rect.Inside( m_End.x, -m_End.y );
}


void LibDrawSegment::DoMove( const wxPoint& newPosition )
{
    wxPoint offset = newPosition - m_Pos;
    m_Pos += offset;
    m_End += offset;
}


void LibDrawSegment::DoMirrorHorizontal( const wxPoint& center )
{
    m_Pos.x -= center.x;
    m_Pos.x *= -1;
    m_Pos.x += center.x;
    m_End.x -= center.x;
    m_End.x *= -1;
    m_End.x += center.x;
}


void LibDrawSegment::DoPlot( PLOTTER* plotter, const wxPoint& offset, bool fill,
                             const int transform[2][2] )
{
    wxASSERT( plotter != NULL );

    int points[4];
    wxPoint pos = TransformCoordinate( transform, m_Pos ) + offset;
    wxPoint end = TransformCoordinate( transform, m_End ) + offset;
    points[0] = pos.x;
    points[1] = pos.y;
    points[2] = end.x;
    points[3] = end.y;
    plotter->set_color( ReturnLayerColor( LAYER_DEVICE ) );
    plotter->poly( 2, points, m_Fill, GetPenSize() );
}


/** Function GetPenSize
 * @return the size of the "pen" that be used to draw or plot this item
 */
int LibDrawSegment::GetPenSize()
{
    return ( m_Width == 0 ) ? g_DrawDefaultLineThickness : m_Width;
}

void LibDrawSegment::Draw( WinEDA_DrawPanel* aPanel, wxDC* aDC,
                           const wxPoint& aOffset, int aColor, int aDrawMode,
                           void* aData, const int aTransformMatrix[2][2] )
{
    wxPoint pos1, pos2;

    int     color = ReturnLayerColor( LAYER_DEVICE );

    if( aColor < 0 )       // Used normal color or selected color
    {
        if( m_Selected & IS_SELECTED )
            color = g_ItemSelectetColor;
    }
    else
        color = aColor;

    pos1 = TransformCoordinate( aTransformMatrix, m_Pos ) + aOffset;
    pos2 = TransformCoordinate( aTransformMatrix, m_End ) + aOffset;

    GRSetDrawMode( aDC, aDrawMode );

    GRLine( &aPanel->m_ClipBox, aDC, pos1.x, pos1.y, pos2.x, pos2.y,
            GetPenSize( ), color );

    /* Set to one (1) to draw bounding box around line segment to validate
     * bounding box calculation. */
#if 0
    EDA_Rect bBox = GetBoundingBox();
    bBox.Inflate( m_Width + 2, m_Width + 2 );
    GRRect( &aPanel->m_ClipBox, aDC, bBox.GetOrigin().x, bBox.GetOrigin().y,
            bBox.GetEnd().x, bBox.GetEnd().y, 0, LIGHTMAGENTA );
#endif
}


void LibDrawSegment::DisplayInfo( WinEDA_DrawFrame* frame )
{
    wxString msg;
    EDA_Rect bBox = GetBoundingBox();

    LIB_DRAW_ITEM::DisplayInfo( frame );

    msg = ReturnStringFromValue( g_UnitMetric, m_Width,
                                 EESCHEMA_INTERNAL_UNIT, true );

    frame->MsgPanel->Affiche_1_Parametre( 20, _( "Line width" ), msg, BLUE );

    msg.Printf( wxT( "(%d, %d, %d, %d)" ), bBox.GetOrigin().x,
                bBox.GetOrigin().y, bBox.GetEnd().x, bBox.GetEnd().y );

    frame->MsgPanel->Affiche_1_Parametre( 60, _( "Bounding box" ), msg, BROWN );
}


/**
 * Function HitTest
 * tests if the given wxPoint is within the bounds of this object.
 * @param aRefPos A wxPoint to test
 * @return bool - true if a hit, else false
 */
bool LibDrawSegment::HitTest( const wxPoint& aPosRef )
{
    int mindist = m_Width ? m_Width / 2 : g_DrawDefaultLineThickness / 2;

    // Have a minimal tolerance for hit test
    if( mindist < 3 )
        mindist = 3;        // = 3 mils

    return HitTest( aPosRef, mindist, DefaultTransformMatrix );
}


/** Function HitTest
 * @return true if the point aPosRef is near this object
 * @param aPosRef = a wxPoint to test
 * @param aThreshold = max distance to this object (usually the half
 *                     thickness of a line)
 * @param aTransMat = the transform matrix
 */
bool LibDrawSegment::HitTest( wxPoint aPosRef, int aThreshold,
                              const int aTransMat[2][2] )
{
    wxPoint start = TransformCoordinate( aTransMat, m_Pos );
    wxPoint end   = TransformCoordinate( aTransMat, m_End );

    return TestSegmentHit( aPosRef, start, end, aThreshold );
}


/***************************/
/** class LibDrawPolyline **/
/***************************/
LibDrawPolyline::LibDrawPolyline( LIB_COMPONENT* aParent ) :
    LIB_DRAW_ITEM( COMPONENT_POLYLINE_DRAW_TYPE, aParent )
{
    m_Fill     = NO_FILL;
    m_Width    = 0;
    m_typeName = _( "PolyLine" );
}


LibDrawPolyline::LibDrawPolyline( const LibDrawPolyline& polyline ) :
    LIB_DRAW_ITEM( polyline )
{
    m_PolyPoints = polyline.m_PolyPoints;   // Vector copy
    m_Width      = polyline.m_Width;
    m_Fill       = polyline.m_Fill;
}


bool LibDrawPolyline::Save( FILE* ExportFile ) const
{
    int ccount = GetCornerCount();

    if( fprintf( ExportFile, "P %d %d %d %d",
                 ccount, m_Unit, m_Convert, m_Width ) < 0 )
        return false;

    for( unsigned i = 0; i < GetCornerCount(); i++ )
    {
        if( fprintf( ExportFile, "  %d %d",
                     m_PolyPoints[i].x, m_PolyPoints[i].y ) < 0 )
            return false;
    }

    if( fprintf( ExportFile, " %c\n", fill_tab[m_Fill] ) < 0 )
        return false;

    return true;
}


bool LibDrawPolyline::Load( char* line, wxString& errorMsg )
{
    char*   p;
    int     i, ccount = 0;
    wxPoint pt;

    i = sscanf( &line[2], "%d %d %d %d", &ccount, &m_Unit, &m_Convert,
                &m_Width );

    if( i < 4 )
    {
        errorMsg.Printf( _( "polyline only had %d parameters of the required 4" ), i );
        return false;
    }
    if( ccount <= 0 )
    {
        errorMsg.Printf( _( "polyline count parameter %d is invalid" ),
                         ccount );
        return false;
    }

    p = strtok( &line[2], " \t\n" );
    p = strtok( NULL, " \t\n" );
    p = strtok( NULL, " \t\n" );
    p = strtok( NULL, " \t\n" );

    for( i = 0; i < ccount; i++ )
    {
        wxPoint point;
        p = strtok( NULL, " \t\n" );
        if( sscanf( p, "%d", &pt.x ) != 1 )
        {
            errorMsg.Printf( _( "polyline point %d X position not defined" ),
                             i );
            return false;
        }
        p = strtok( NULL, " \t\n" );
        if( sscanf( p, "%d", &pt.y ) != 1 )
        {
            errorMsg.Printf( _( "polyline point %d Y position not defined" ),
                             i );
            return false;
        }
        AddPoint( pt );
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


LIB_DRAW_ITEM* LibDrawPolyline::DoGenCopy()
{
    LibDrawPolyline* newitem = new LibDrawPolyline( GetParent() );

    newitem->m_PolyPoints = m_PolyPoints;   // Vector copy
    newitem->m_Width      = m_Width;
    newitem->m_Unit       = m_Unit;
    newitem->m_Convert    = m_Convert;
    newitem->m_Flags      = m_Flags;
    newitem->m_Fill       = m_Fill;

    return (LIB_DRAW_ITEM*) newitem;
}


int LibDrawPolyline::DoCompare( const LIB_DRAW_ITEM& other ) const
{
    wxASSERT( other.Type() == COMPONENT_POLYLINE_DRAW_TYPE );

    const LibDrawPolyline* tmp = ( LibDrawPolyline* ) &other;

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


void LibDrawPolyline::DoOffset( const wxPoint& offset )
{
    for( size_t i = 0; i < m_PolyPoints.size(); i++ )
        m_PolyPoints[i] += offset;
}


bool LibDrawPolyline::DoTestInside( EDA_Rect& rect )
{
    for( size_t i = 0; i < m_PolyPoints.size(); i++ )
    {
        if( rect.Inside( m_PolyPoints[i].x, -m_PolyPoints[i].y ) )
            return true;
    }

    return false;
}


void LibDrawPolyline::DoMove( const wxPoint& newPosition )
{
    DoOffset( newPosition - m_PolyPoints[0] );
}


void LibDrawPolyline::DoMirrorHorizontal( const wxPoint& center )
{
    size_t i, imax = m_PolyPoints.size();

    for( i = 0; i < imax; i++ )
    {
        m_PolyPoints[i].x -= center.x;
        m_PolyPoints[i].x *= -1;
        m_PolyPoints[i].x += center.x;
    }
}


void LibDrawPolyline::DoPlot( PLOTTER* plotter, const wxPoint& offset, bool fill,
                              const int transform[2][2] )
{
    wxASSERT( plotter != NULL );

    size_t i;

    int* Poly = (int*) MyMalloc( sizeof(int) * 2 * GetCornerCount() );

    for( i = 0; i < m_PolyPoints.size(); i++ )
    {
        wxPoint pos = m_PolyPoints[i];
        pos = TransformCoordinate( transform, pos ) + offset;
        Poly[i * 2]     = pos.x;
        Poly[i * 2 + 1] = pos.y;
    }

    if( fill && m_Fill == FILLED_WITH_BG_BODYCOLOR )
    {
        plotter->set_color( ReturnLayerColor( LAYER_DEVICE_BACKGROUND ) );
        plotter->poly( i, Poly, FILLED_WITH_BG_BODYCOLOR, 0 );
    }

    plotter->set_color( ReturnLayerColor( LAYER_DEVICE ) );
    plotter->poly( i, Poly, m_Fill, GetPenSize() );
    MyFree( Poly );
}


void LibDrawPolyline::AddPoint( const wxPoint& point )
{
    m_PolyPoints.push_back( point );
}


/** Function GetPenSize
 * @return the size of the "pen" that be used to draw or plot this item
 */
int LibDrawPolyline::GetPenSize()
{
    return ( m_Width == 0 ) ? g_DrawDefaultLineThickness : m_Width;
}

void LibDrawPolyline::Draw( WinEDA_DrawPanel* aPanel, wxDC* aDC,
                            const wxPoint& aOffset, int aColor, int aDrawMode,
                            void* aData, const int aTransformMatrix[2][2] )
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

    // Set the size of the buffer od coordinates
    if( Buf_Poly_Drawings == NULL )
    {
        Buf_Poly_Size     = m_PolyPoints.size();
        Buf_Poly_Drawings =
            (wxPoint*) MyMalloc( sizeof(wxPoint) * Buf_Poly_Size );
    }
    else if( Buf_Poly_Size < m_PolyPoints.size() )
    {
        Buf_Poly_Size     = m_PolyPoints.size();
        Buf_Poly_Drawings =
            (wxPoint*) realloc( Buf_Poly_Drawings,
                                sizeof(wxPoint) * Buf_Poly_Size );
    }

    for( unsigned ii = 0; ii < m_PolyPoints.size(); ii++ )
    {
        Buf_Poly_Drawings[ii] =
            TransformCoordinate( aTransformMatrix, m_PolyPoints[ii] ) + aOffset;
    }

    FILL_T fill = aData ? NO_FILL : m_Fill;
    if( aColor >= 0 )
        fill = NO_FILL;

    GRSetDrawMode( aDC, aDrawMode );

    if( fill == FILLED_WITH_BG_BODYCOLOR )
        GRPoly( &aPanel->m_ClipBox, aDC, m_PolyPoints.size(),
                Buf_Poly_Drawings, 1, GetPenSize( ), color,
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
    bBox.Inflate( m_Width + 1, m_Width + 1 );
    GRRect( &aPanel->m_ClipBox, aDC, bBox.GetOrigin().x, bBox.GetOrigin().y,
            bBox.GetEnd().x, bBox.GetEnd().y, 0, LIGHTMAGENTA );
#endif
}


/**
 * Function HitTest
 * tests if the given wxPoint is within the bounds of this object.
 * @param aRefPos A wxPoint to test
 * @return bool - true if a hit, else false
 */
bool LibDrawPolyline::HitTest( const wxPoint& aRefPos )
{
    int mindist = m_Width ? m_Width / 2 : g_DrawDefaultLineThickness / 2;

    // Have a minimal tolerance for hit test
    if( mindist < 3 )
        mindist = 3;        // = 3 mils
    return HitTest( aRefPos, mindist, DefaultTransformMatrix );
}


/** Function HitTest
 * @return true if the point aPosRef is near a segment
 * @param aPosRef = a wxPoint to test
 * @param aThreshold = max distance to a segment
 * @param aTransMat = the transform matrix
 */
bool LibDrawPolyline::HitTest( wxPoint aPosRef, int aThreshold,
                               const int aTransMat[2][2] )
{
    wxPoint ref, start, end;

    for( unsigned ii = 1; ii < GetCornerCount(); ii++ )
    {
        start = TransformCoordinate( aTransMat, m_PolyPoints[ii - 1] );
        end   = TransformCoordinate( aTransMat, m_PolyPoints[ii] );

        if( TestSegmentHit( aPosRef, start, end, aThreshold ) )
            return true;
    }

    return false;
}


/** Function GetBoundingBox
 * @return the boundary box for this, in library coordinates
 */
EDA_Rect LibDrawPolyline::GetBoundingBox()
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


void LibDrawPolyline::DisplayInfo( WinEDA_DrawFrame* frame )
{
    wxString msg;
    EDA_Rect bBox = GetBoundingBox();

    LIB_DRAW_ITEM::DisplayInfo( frame );

    msg = ReturnStringFromValue( g_UnitMetric, m_Width,
                                 EESCHEMA_INTERNAL_UNIT, true );

    frame->MsgPanel->Affiche_1_Parametre( 20, _( "Line width" ), msg, BLUE );

    msg.Printf( wxT( "(%d, %d, %d, %d)" ), bBox.GetOrigin().x,
                bBox.GetOrigin().y, bBox.GetEnd().x, bBox.GetEnd().y );

    frame->MsgPanel->Affiche_1_Parametre( 40, _( "Bounding box" ), msg, BROWN );
}

/***************************/
/** class LibDrawBezier **/
/***************************/
LibDrawBezier::LibDrawBezier( LIB_COMPONENT* aParent ) :
    LIB_DRAW_ITEM( COMPONENT_BEZIER_DRAW_TYPE, aParent )
{
    m_Fill     = NO_FILL;
    m_Width    = 0;
    m_typeName = _( "Bezier" );
}


LibDrawBezier::LibDrawBezier( const LibDrawBezier& bezier ) :
    LIB_DRAW_ITEM( bezier )
{
    m_PolyPoints   = bezier.m_PolyPoints;
    m_BezierPoints = bezier.m_BezierPoints;   // Vector copy
    m_Width        = bezier.m_Width;
    m_Fill         = bezier.m_Fill;
}


bool LibDrawBezier::Save( FILE* ExportFile ) const
{
    int ccount = GetCornerCount();

    if( fprintf( ExportFile, "B %d %d %d %d",
                 ccount, m_Unit, m_Convert, m_Width ) < 0 )
        return false;

    for( unsigned i = 0; i < GetCornerCount(); i++ )
    {
        if( fprintf( ExportFile, "  %d %d", m_BezierPoints[i].x,
                     m_BezierPoints[i].y ) < 0 )
            return false;
    }

    if( fprintf( ExportFile, " %c\n", fill_tab[m_Fill] ) < 0 )
        return false;

    return true;
}


bool LibDrawBezier::Load( char* line, wxString& errorMsg )
{
    char*   p;
    int     i, ccount = 0;
    wxPoint pt;

    i = sscanf( &line[2], "%d %d %d %d", &ccount, &m_Unit, &m_Convert,
               &m_Width );

    if( i !=4 )
    {
        errorMsg.Printf( _( "Bezier only had %d parameters of the required 4" ),
                         i );
        return false;
    }
    if( ccount <= 0 )
    {
        errorMsg.Printf( _( "Bezier count parameter %d is invalid" ),
                         ccount );
        return false;
    }

    p = strtok( &line[2], " \t\n" );
    p = strtok( NULL, " \t\n" );
    p = strtok( NULL, " \t\n" );
    p = strtok( NULL, " \t\n" );

    for( i = 0; i < ccount; i++ )
    {
        wxPoint point;
        p = strtok( NULL, " \t\n" );
        if( sscanf( p, "%d", &pt.x ) != 1 )
        {
            errorMsg.Printf( _( "Bezier point %d X position not defined" ), i );
            return false;
        }
        p = strtok( NULL, " \t\n" );
        if( sscanf( p, "%d", &pt.y ) != 1 )
        {
            errorMsg.Printf( _( "Bezier point %d Y position not defined" ), i );
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


LIB_DRAW_ITEM* LibDrawBezier::DoGenCopy()
{
    LibDrawBezier* newitem = new LibDrawBezier(GetParent());

    newitem->m_BezierPoints = m_BezierPoints;   // Vector copy
    newitem->m_Width   = m_Width;
    newitem->m_Unit    = m_Unit;
    newitem->m_Convert = m_Convert;
    newitem->m_Flags   = m_Flags;
    newitem->m_Fill    = m_Fill;
    return (LIB_DRAW_ITEM*) newitem;
}


int LibDrawBezier::DoCompare( const LIB_DRAW_ITEM& other ) const
{
    wxASSERT( other.Type() == COMPONENT_BEZIER_DRAW_TYPE );

    const LibDrawBezier* tmp = ( LibDrawBezier* ) &other;

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


void LibDrawBezier::DoOffset( const wxPoint& offset )
{
    size_t i;

    for( i = 0; i < m_BezierPoints.size(); i++ )
        m_BezierPoints[i] += offset;

    for( i = 0; i < m_PolyPoints.size(); i++ )
        m_PolyPoints[i] += offset;
}


bool LibDrawBezier::DoTestInside( EDA_Rect& rect )
{
    for( size_t i = 0; i < m_PolyPoints.size(); i++ )
    {
        if( rect.Inside( m_PolyPoints[i].x, -m_PolyPoints[i].y ) )
            return true;
    }

    return false;
}


void LibDrawBezier::DoMove( const wxPoint& newPosition )
{
    DoOffset( newPosition - m_PolyPoints[0] );
}


void LibDrawBezier::DoMirrorHorizontal( const wxPoint& center )
{
    size_t i, imax = m_PolyPoints.size();

    for( i = 0; i < imax; i++ )
    {
        m_PolyPoints[i].x -= center.x;
        m_PolyPoints[i].x *= -1;
        m_PolyPoints[i].x += center.x;
    }

    imax = m_BezierPoints.size();
    for( i = 0; i < imax; i++ )
    {
        m_BezierPoints[i].x -= center.x;
        m_BezierPoints[i].x *= -1;
        m_BezierPoints[i].x += center.x;
    }
}


void LibDrawBezier::DoPlot( PLOTTER* plotter, const wxPoint& offset, bool fill,
                            const int transform[2][2] )
{
    wxASSERT( plotter != NULL );

    size_t i;

    int* Poly = (int*) MyMalloc( sizeof(int) * 2 * GetCornerCount() );

    for( i = 0; i < m_PolyPoints.size(); i++ )
    {
        wxPoint pos = m_PolyPoints[i];
        pos = TransformCoordinate( transform, pos ) + offset;
        Poly[i * 2]     = pos.x;
        Poly[i * 2 + 1] = pos.y;
    }

    if( fill && m_Fill == FILLED_WITH_BG_BODYCOLOR )
    {
        plotter->set_color( ReturnLayerColor( LAYER_DEVICE_BACKGROUND ) );
        plotter->poly( i, Poly, FILLED_WITH_BG_BODYCOLOR, 0 );
    }

    plotter->set_color( ReturnLayerColor( LAYER_DEVICE ) );
    plotter->poly( i, Poly, m_Fill, GetPenSize() );
    MyFree( Poly );
}


/** Function GetPenSize
 * @return the size of the "pen" that be used to draw or plot this item
 */
int LibDrawBezier::GetPenSize()
{
    return ( m_Width == 0 ) ? g_DrawDefaultLineThickness : m_Width;
}

void LibDrawBezier::Draw( WinEDA_DrawPanel* aPanel, wxDC* aDC,
                           const wxPoint& aOffset, int aColor, int aDrawMode,
                           void* aData, const int aTransformMatrix[2][2] )
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
        PolyPointsTraslated.push_back(
            TransformCoordinate( aTransformMatrix, m_PolyPoints[i] ) + aOffset );

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
                &PolyPointsTraslated[0], 1, GetPenSize(), color,
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
    bBox.Inflate( m_Width + 1, m_Width + 1 );
    GRRect( &aPanel->m_ClipBox, aDC, bBox.GetOrigin().x, bBox.GetOrigin().y,
            bBox.GetEnd().x, bBox.GetEnd().y, 0, LIGHTMAGENTA );
#endif
}


/**
 * Function HitTest
 * tests if the given wxPoint is within the bounds of this object.
 * @param aRefPos A wxPoint to test
 * @return bool - true if a hit, else false
 */
bool LibDrawBezier::HitTest( const wxPoint& aRefPos )
{
    int mindist = m_Width ? m_Width /2 : g_DrawDefaultLineThickness / 2;
    // Have a minimal tolerance for hit test
    if ( mindist < 3 )
        mindist = 3;        // = 3 mils
    return HitTest( aRefPos, mindist, DefaultTransformMatrix );
}

/** Function HitTest
 * @return true if the point aPosRef is near a segment
 * @param aPosRef = a wxPoint to test
 * @param aThreshold = max distance to a segment
 * @param aTransMat = the transform matrix
 */
bool LibDrawBezier::HitTest( wxPoint aPosRef, int aThreshold,
                             const int aTransMat[2][2] )
{
    wxPoint ref, start, end;

    for( unsigned ii = 1; ii < GetCornerCount(); ii++ )
    {
        start = TransformCoordinate( aTransMat, m_PolyPoints[ii - 1] );
        end   = TransformCoordinate( aTransMat, m_PolyPoints[ii] );

        if ( TestSegmentHit( aPosRef, start, end, aThreshold ) )
            return true;
    }

    return false;
}


/** Function GetBoundingBox
 * @return the boundary box for this, in library coordinates
 */
EDA_Rect LibDrawBezier::GetBoundingBox()
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


void LibDrawBezier::DisplayInfo( WinEDA_DrawFrame* frame )
{
    wxString msg;
    EDA_Rect bBox = GetBoundingBox();

    LIB_DRAW_ITEM::DisplayInfo( frame );

    msg = ReturnStringFromValue( g_UnitMetric, m_Width,
                                 EESCHEMA_INTERNAL_UNIT, true );

    frame->MsgPanel->Affiche_1_Parametre( 20, _( "Line width" ), msg, BLUE );

    msg.Printf( wxT( "(%d, %d, %d, %d)" ), bBox.GetOrigin().x,
                bBox.GetOrigin().y, bBox.GetEnd().x, bBox.GetEnd().y );

    frame->MsgPanel->Affiche_1_Parametre( 40, _( "Bounding box" ), msg, BROWN );
}
