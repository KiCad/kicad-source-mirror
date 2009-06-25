/************************/
/* class_body_items.cpp */
/************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "drawtxt.h"
#include "trigo.h"
#include "bezier_curves.h"

#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "protos.h"


static int fill_tab[3] = { 'N', 'F', 'f' };

//#define DRAW_ARC_WITH_ANGLE		// Used to draw arcs


/* Base class (abstract) for components bodies items */
LibEDA_BaseStruct::LibEDA_BaseStruct( KICAD_T struct_type, EDA_LibComponentStruct* aParent ) :
    EDA_BaseStruct( struct_type )
{
    m_Unit    = 0;   /* Unit identification (for multi part per package)
                      *  0 if the item is common to all units */
    m_Convert = 0;   /* Shape identification (for parts which have a convert
                      * shape) 0 if the item is common to all shapes */
    m_Fill    = NO_FILL;

    m_Parent   = aParent;
    m_typeName = _( "Undefined" );
}


/**
 * Update the message panel information with the drawing information.
 *
 * This base function is used to display the information common to the
 * all library items.  Call the base class from the derived class or the
 * common information will not be updated in the message panel.
 */
void LibEDA_BaseStruct::DisplayInfo( WinEDA_DrawFrame* frame )
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


/**********************/
/** class LibDrawArc **/
/**********************/

LibDrawArc::LibDrawArc( EDA_LibComponentStruct* aParent ) :
    LibEDA_BaseStruct( COMPONENT_ARC_DRAW_TYPE, aParent )
{
    m_Rayon = 0;
    t1 = t2 = 0;
    m_Width    = 0;
    m_Fill     = NO_FILL;
    m_typeName = _( "Arc" );
}


/**
 * format:
 *  A centre_posx centre_posy rayon start_angle end_angle unit convert
 *  fill('N', 'F' ou 'f') startx starty endx endy
 */
bool LibDrawArc::Save( FILE* ExportFile ) const
{
    int x1 = t1;

    if( x1 > 1800 )
        x1 -= 3600;

    int x2 = t2;

    if( x2 > 1800 )
        x2 -= 3600;

    fprintf( ExportFile, "A %d %d %d %d %d %d %d %d %c %d %d %d %d\n",
             m_Pos.x, m_Pos.y, m_Rayon, x1, x2, m_Unit, m_Convert, m_Width,
             fill_tab[m_Fill], m_ArcStart.x, m_ArcStart.y, m_ArcEnd.x,
             m_ArcEnd.y );

    return true;
}


bool LibDrawArc::Load( char* line, wxString& errorMsg )
{
    int  startx, starty, endx, endy, cnt;
    char tmp[256];

    cnt = sscanf( &line[2], "%d %d %d %d %d %d %d %d %s %d %d %d %d",
                  &m_Pos.x, &m_Pos.y, &m_Rayon, &t1, &t2, &m_Unit, &m_Convert,
                  &m_Width, tmp, &startx, &starty, &endx, &endy );
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

    NORMALIZE_ANGLE( t1 );
    NORMALIZE_ANGLE( t2 );

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
        m_ArcStart.x = m_Rayon;
        m_ArcStart.y = 0;
        m_ArcEnd.x   = m_Rayon;
        m_ArcEnd.y   = 0;
        RotatePoint( &m_ArcStart.x, &m_ArcStart.y, -t1 );
        m_ArcStart.x += m_Pos.x;
        m_ArcStart.y += m_Pos.y;
        RotatePoint( &m_ArcEnd.x, &m_ArcEnd.y, -t2 );
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
 * @param aThreshold = max distance to this object (usually the half thickness of a line)
 * @param aTransMat = the transform matrix
 */
bool LibDrawArc::HitTest( wxPoint aRefPoint, int aThreshold, const int aTransMat[2][2] )
{
    // TODO: use aTransMat to calculmates parameters
    wxPoint relpos = aRefPoint;

    NEGATE( relpos.y );       // reverse Y axis

    relpos -= m_Pos;
    int dist = wxRound( sqrt( ( (double) relpos.x * relpos.x ) + ( (double) relpos.y * relpos.y ) ) );

    if( abs( dist - m_Rayon ) > aThreshold )
        return false;

    // We are on the circle, ensure we are only on the arc, i.e. between m_ArcStart and m_ArcEnd
    int astart = t1;    // arc starting point ( in 0.1 degree)
    int aend   = t2;    // arc ending point ( in 0.1 degree)
    int atest  = wxRound( atan2( relpos.y, relpos.x ) * 1800.0 / M_PI );
    NORMALIZE_ANGLE_180( atest );
    NORMALIZE_ANGLE_180( astart );
    NORMALIZE_ANGLE_180( aend );

    if( astart > aend )
        EXCHG( astart, aend );

    if( atest >= astart && atest <= aend )
        return true;

    return false;
}


LibDrawArc* LibDrawArc::GenCopy()
{
    LibDrawArc* newitem = new LibDrawArc( GetParent() );

    newitem->m_Pos = m_Pos;
    newitem->m_ArcStart = m_ArcStart;
    newitem->m_ArcEnd   = m_ArcEnd;
    newitem->m_Rayon    = m_Rayon;
    newitem->t1 = t1;
    newitem->t2 = t2;
    newitem->m_Width   = m_Width;
    newitem->m_Unit    = m_Unit;
    newitem->m_Convert = m_Convert;
    newitem->m_Flags   = m_Flags;
    newitem->m_Fill    = m_Fill;
    return newitem;
}


void LibDrawArc::Draw( WinEDA_DrawPanel* aPanel, wxDC* aDC,
                       const wxPoint& aOffset, int aColor,
                       int aDrawMode, void* aData,
                       const int aTransformMatrix[2][2] )
{
    wxPoint pos1, pos2, posc;

    int     color     = ReturnLayerColor( LAYER_DEVICE );
    int     linewidth = (m_Width == 0) ? g_DrawDefaultLineThickness : m_Width;

    if( aColor < 0 )       // Used normal color or selected color
    {
        if( (m_Selected & IS_SELECTED) )
            color = g_ItemSelectetColor;
    }
    else
        color = aColor;

    pos1 = TransformCoordinate( aTransformMatrix, m_ArcEnd ) + aOffset;
    pos2 = TransformCoordinate( aTransformMatrix, m_ArcStart ) + aOffset;
    posc = TransformCoordinate( aTransformMatrix, m_Pos ) + aOffset;
    int  pt1  = t1;
    int  pt2  = t2;
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
                    m_Rayon, linewidth, color,
                    ReturnLayerColor( LAYER_DEVICE_BACKGROUND ) );
    else if( fill == FILLED_SHAPE && !aData )
        GRFilledArc( &aPanel->m_ClipBox, aDC, posc.x, posc.y, pt1, pt2,
                     m_Rayon, color, color );
    else
    {
#ifdef DRAW_ARC_WITH_ANGLE

        GRArc( &aPanel->m_ClipBox, aDC, posc.x, posc.y, pt1, pt2,
               m_Rayon, linewidth, color );

#else

        GRArc1( &aPanel->m_ClipBox, aDC, pos1.x, pos1.y, pos2.x, pos2.y,
                posc.x, posc.y, linewidth, color );

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
       || ( m_Rayon == 0 ) )
    {
        wxLogDebug( wxT(
                        "Invalid arc drawing definition, center(%d, %d) \
start(%d, %d), end(%d, %d), radius %d"                                                                            ),
                    m_Pos.x, m_Pos.y, m_ArcStart.x, m_ArcStart.y, m_ArcEnd.x,
                    m_ArcEnd.y, m_Rayon );
        return rect;
    }

    endPos     = TransformCoordinate( DefaultTransformMatrix, m_ArcEnd );
    startPos   = TransformCoordinate( DefaultTransformMatrix, m_ArcStart );
    centerPos  = TransformCoordinate( DefaultTransformMatrix, m_Pos );
    angleStart = t1;
    angleEnd   = t2;

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
        maxX = centerPos.x + m_Rayon;

    /* Arc end angle wrapped passed 360. */
    if( angleStart > angleEnd )
        angleEnd += 3600;

    if( angleStart <= 900 && angleEnd >= 900 )          /* 90 deg */
        maxY = centerPos.y + m_Rayon;
    if( angleStart <= 1800 && angleEnd >= 1800 )        /* 180 deg */
        minX = centerPos.x - m_Rayon;
    if( angleStart <= 2700 && angleEnd >= 2700 )        /* 270 deg */
        minY = centerPos.y - m_Rayon;
    if( angleStart <= 3600 && angleEnd >= 3600 )        /* 0 deg   */
        maxX = centerPos.x + m_Rayon;

    rect.SetOrigin( minX, minY );
    rect.SetEnd( maxX, maxY );
    rect.Inflate( m_Width / 2, m_Width / 2 );

    return rect;
}


void LibDrawArc::DisplayInfo( WinEDA_DrawFrame* frame )
{
    wxString msg;
    EDA_Rect bBox = GetBoundingBox();

    LibEDA_BaseStruct::DisplayInfo( frame );

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

LibDrawCircle::LibDrawCircle( EDA_LibComponentStruct* aParent ) :
    LibEDA_BaseStruct( COMPONENT_CIRCLE_DRAW_TYPE, aParent )
{
    m_Rayon    = 0;
    m_Fill     = NO_FILL;
    m_typeName = _( "Circle" );
}


bool LibDrawCircle::Save( FILE* ExportFile ) const
{
    fprintf( ExportFile, "C %d %d %d %d %d %d %c\n", m_Pos.x, m_Pos.y,
             m_Rayon, m_Unit, m_Convert, m_Width, fill_tab[m_Fill] );

    return true;
}


bool LibDrawCircle::Load( char* line, wxString& errorMsg )
{
    char tmp[256];

    int  cnt = sscanf( &line[2], "%d %d %d %d %d %d %s", &m_Pos.x, &m_Pos.y,
                       &m_Rayon, &m_Unit, &m_Convert, &m_Width, tmp );

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
 * @param aThreshold = max distance to this object (usually the half thickness of a line)
 * @param aTransMat = the transform matrix
 */
bool LibDrawCircle::HitTest( wxPoint aPosRef, int aThreshold, const int aTransMat[2][2] )
{
    wxPoint relpos = aPosRef - TransformCoordinate( aTransMat, m_Pos );

    int     dist =
        wxRound( sqrt( ( (double) relpos.x * relpos.x ) + ( (double) relpos.y * relpos.y ) ) );

    if( abs( dist - m_Rayon ) <= aThreshold )
        return true;
    return false;
}


LibDrawCircle* LibDrawCircle::GenCopy()
{
    LibDrawCircle* newitem = new LibDrawCircle( GetParent() );

    newitem->m_Pos     = m_Pos;
    newitem->m_Rayon   = m_Rayon;
    newitem->m_Width   = m_Width;
    newitem->m_Unit    = m_Unit;
    newitem->m_Convert = m_Convert;
    newitem->m_Flags   = m_Flags;
    newitem->m_Fill    = m_Fill;
    return newitem;
}


void LibDrawCircle::Draw( WinEDA_DrawPanel* aPanel, wxDC* aDC,
                          const wxPoint& aOffset, int aColor, int aDrawMode,
                          void* aData, const int aTransformMatrix[2][2] )
{
    wxPoint pos1;

    int     color     = ReturnLayerColor( LAYER_DEVICE );
    int     linewidth = (m_Width == 0) ? g_DrawDefaultLineThickness : m_Width;

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
                       m_Rayon, linewidth, color,
                       ReturnLayerColor( LAYER_DEVICE_BACKGROUND ) );
    else if( fill == FILLED_SHAPE )
        GRFilledCircle( &aPanel->m_ClipBox, aDC, pos1.x, pos1.y,
                        m_Rayon, 0, color, color );
    else
        GRCircle( &aPanel->m_ClipBox, aDC, pos1.x, pos1.y,
                  m_Rayon, linewidth, color );
}


EDA_Rect LibDrawCircle::GetBoundingBox()
{
    EDA_Rect rect;

    rect.SetOrigin( m_Pos.x - m_Rayon, ( m_Pos.y - m_Rayon ) * -1 );
    rect.SetEnd( m_Pos.x + m_Rayon, ( m_Pos.y + m_Rayon ) * -1 );
    rect.Inflate( m_Width / 2, m_Width / 2 );

    return rect;
}


void LibDrawCircle::DisplayInfo( WinEDA_DrawFrame* frame )
{
    wxString msg;
    EDA_Rect bBox = GetBoundingBox();

    LibEDA_BaseStruct::DisplayInfo( frame );

    msg = ReturnStringFromValue( g_UnitMetric, m_Width,
                                 EESCHEMA_INTERNAL_UNIT, true );

    frame->MsgPanel->Affiche_1_Parametre( 20, _( "Line width" ), msg, BLUE );

    msg = ReturnStringFromValue( g_UnitMetric, m_Rayon,
                                 EESCHEMA_INTERNAL_UNIT, true );
    frame->MsgPanel->Affiche_1_Parametre( 40, _( "Radius" ), msg, RED );

    msg.Printf( wxT( "(%d, %d, %d, %d)" ), bBox.GetOrigin().x,
                bBox.GetOrigin().y, bBox.GetEnd().x, bBox.GetEnd().y );

    frame->MsgPanel->Affiche_1_Parametre( 60, _( "Bounding box" ), msg, BROWN );
}


/*************************/
/** class LibDrawSquare **/
/*************************/

LibDrawSquare::LibDrawSquare( EDA_LibComponentStruct* aParent ) :
    LibEDA_BaseStruct( COMPONENT_RECT_DRAW_TYPE, aParent )
{
    m_Width    = 0;
    m_Fill     = NO_FILL;
    m_typeName = _( "Rectangle" );
}


bool LibDrawSquare::Save( FILE* ExportFile ) const
{
    fprintf( ExportFile, "S %d %d %d %d %d %d %d %c\n", m_Pos.x, m_Pos.y,
             m_End.x, m_End.y, m_Unit, m_Convert, m_Width, fill_tab[m_Fill] );

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


LibDrawSquare* LibDrawSquare::GenCopy()
{
    LibDrawSquare* newitem = new LibDrawSquare( GetParent() );

    newitem->m_Pos     = m_Pos;
    newitem->m_End     = m_End;
    newitem->m_Width   = m_Width;
    newitem->m_Unit    = m_Unit;
    newitem->m_Convert = m_Convert;
    newitem->m_Flags   = m_Flags;
    newitem->m_Fill    = m_Fill;
    return newitem;
}


void LibDrawSquare::Draw( WinEDA_DrawPanel* aPanel, wxDC* aDC,
                          const wxPoint& aOffset, int aColor, int aDrawMode,
                          void* aData, const int aTransformMatrix[2][2] )
{
    wxPoint pos1, pos2;

    int     color     = ReturnLayerColor( LAYER_DEVICE );
    int     linewidth = (m_Width == 0) ? g_DrawDefaultLineThickness : m_Width;

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

    if( fill == FILLED_WITH_BG_BODYCOLOR && !aData )
        GRFilledRect( &aPanel->m_ClipBox, aDC, pos1.x, pos1.y, pos2.x, pos2.y,
                     linewidth, color,
                     ReturnLayerColor( LAYER_DEVICE_BACKGROUND ) );
    else if( m_Fill == FILLED_SHAPE  && !aData )
        GRFilledRect( &aPanel->m_ClipBox, aDC, pos1.x, pos1.y, pos2.x, pos2.y,
                      linewidth, color, color );
    else
        GRRect( &aPanel->m_ClipBox, aDC, pos1.x, pos1.y, pos2.x, pos2.y,
                linewidth, color );
}


void LibDrawSquare::DisplayInfo( WinEDA_DrawFrame* frame )
{
    wxString msg;

    LibEDA_BaseStruct::DisplayInfo( frame );

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
 * @param aThreshold = max distance to this object (usually the half thickness of a line)
 * @param aTransMat = the transform matrix
 */
bool LibDrawSquare::HitTest( wxPoint aRefPoint, int aThreshold, const int aTransMat[2][2] )
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
LibDrawSegment::LibDrawSegment( EDA_LibComponentStruct* aParent ) :
    LibEDA_BaseStruct( COMPONENT_LINE_DRAW_TYPE, aParent )
{
    m_Width    = 0;
    m_typeName = _( "Segment" );
}


bool LibDrawSegment::Save( FILE* ExportFile ) const
{
    fprintf( ExportFile, "L %d %d %d", m_Unit, m_Convert, m_Width );

    return true;
}


bool LibDrawSegment::Load( char* line, wxString& errorMsg )
{
    return true;
}


LibDrawSegment* LibDrawSegment::GenCopy()
{
    LibDrawSegment* newitem = new LibDrawSegment( GetParent() );

    newitem->m_Pos     = m_Pos;
    newitem->m_End     = m_End;
    newitem->m_Width   = m_Width;
    newitem->m_Unit    = m_Unit;
    newitem->m_Convert = m_Convert;
    newitem->m_Flags   = m_Flags;
    return newitem;
}


void LibDrawSegment::Draw( WinEDA_DrawPanel* aPanel, wxDC* aDC,
                           const wxPoint& aOffset, int aColor, int aDrawMode,
                           void* aData, const int aTransformMatrix[2][2] )
{
    wxPoint pos1, pos2;

    int     color     = ReturnLayerColor( LAYER_DEVICE );
    int     linewidth = (m_Width == 0) ? g_DrawDefaultLineThickness : m_Width;

    if( aColor < 0 )       // Used normal color or selected color
    {
        if( m_Selected & IS_SELECTED )
            color = g_ItemSelectetColor;
    }
    else
        color = aColor;

    pos1 = TransformCoordinate( aTransformMatrix, m_Pos ) + aOffset;
    pos2 = TransformCoordinate( aTransformMatrix, m_End ) + aOffset;

    GRLine( &aPanel->m_ClipBox, aDC, pos1.x, pos1.y, pos2.x, pos2.y,
            linewidth, color );
}


void LibDrawSegment::DisplayInfo( WinEDA_DrawFrame* frame )
{
    wxString msg;
    EDA_Rect bBox = GetBoundingBox();

    LibEDA_BaseStruct::DisplayInfo( frame );

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
 * @param aThreshold = max distance to this object (usually the half thickness of a line)
 * @param aTransMat = the transform matrix
 */
bool LibDrawSegment::HitTest( wxPoint aPosRef, int aThreshold, const int aTransMat[2][2] )
{
    wxPoint start = TransformCoordinate( aTransMat, m_Pos );
    wxPoint end   = TransformCoordinate( aTransMat, m_End );

    return TestSegmentHit( aPosRef, start, end, aThreshold );
}


/***************************/
/** class LibDrawPolyline **/
/***************************/
LibDrawPolyline::LibDrawPolyline( EDA_LibComponentStruct* aParent ) :
    LibEDA_BaseStruct( COMPONENT_POLYLINE_DRAW_TYPE, aParent )
{
    m_Fill     = NO_FILL;
    m_Width    = 0;
    m_typeName = _( "PolyLine" );
}


bool LibDrawPolyline::Save( FILE* ExportFile ) const
{
    int ccount = GetCornerCount();

    fprintf( ExportFile, "P %d %d %d %d", ccount, m_Unit, m_Convert, m_Width );

    for( unsigned i = 0; i < GetCornerCount(); i++ )
    {
        fprintf( ExportFile, "  %d %d", m_PolyPoints[i].x, m_PolyPoints[i].y );
    }

    fprintf( ExportFile, " %c\n", fill_tab[m_Fill] );

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


LibDrawPolyline* LibDrawPolyline::GenCopy()
{
    LibDrawPolyline* newitem = new LibDrawPolyline( GetParent() );

    newitem->m_PolyPoints = m_PolyPoints;   // Vector copy
    newitem->m_Width   = m_Width;
    newitem->m_Unit    = m_Unit;
    newitem->m_Convert = m_Convert;
    newitem->m_Flags   = m_Flags;
    newitem->m_Fill    = m_Fill;
    return newitem;
}


void LibDrawPolyline::AddPoint( const wxPoint& point )
{
    m_PolyPoints.push_back( point );
}


void LibDrawPolyline::Draw( WinEDA_DrawPanel* aPanel, wxDC* aDC,
                            const wxPoint& aOffset, int aColor, int aDrawMode,
                            void* aData, const int aTransformMatrix[2][2] )
{
    wxPoint         pos1;

    int             color     = ReturnLayerColor( LAYER_DEVICE );
    int             linewidth = (m_Width == 0) ? g_DrawDefaultLineThickness : m_Width;

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

    if( fill == FILLED_WITH_BG_BODYCOLOR )
        GRPoly( &aPanel->m_ClipBox, aDC, m_PolyPoints.size(),
               Buf_Poly_Drawings, 1, linewidth, color,
               ReturnLayerColor( LAYER_DEVICE_BACKGROUND ) );
    else if( fill == FILLED_SHAPE  )
        GRPoly( &aPanel->m_ClipBox, aDC, m_PolyPoints.size(),
                Buf_Poly_Drawings, 1, linewidth, color, color );
    else
        GRPoly( &aPanel->m_ClipBox, aDC, m_PolyPoints.size(),
                Buf_Poly_Drawings, 0, linewidth, color, color );
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

    LibEDA_BaseStruct::DisplayInfo( frame );

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
LibDrawBezier::LibDrawBezier( EDA_LibComponentStruct* aParent ) :
    LibEDA_BaseStruct( COMPONENT_BEZIER_DRAW_TYPE, aParent )
{
    m_Fill     = NO_FILL;
    m_Width    = 0;
    m_typeName = _( "Bezier" );
}


bool LibDrawBezier::Save( FILE* ExportFile ) const
{
    int ccount = GetCornerCount();

    fprintf( ExportFile, "B %d %d %d %d", ccount, m_Unit, m_Convert, m_Width );

    for( unsigned i = 0; i < GetCornerCount(); i++ )
    {
        fprintf( ExportFile, "  %d %d", m_BezierPoints[i].x, m_BezierPoints[i].y );
    }

    fprintf( ExportFile, " %c\n", fill_tab[m_Fill] );

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
        errorMsg.Printf( _( "Bezier only had %d parameters of the required 4" ), i );
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
            errorMsg.Printf( _( "Bezier point %d X position not defined" ),
							i );
            return false;
        }
        p = strtok( NULL, " \t\n" );
        if( sscanf( p, "%d", &pt.y ) != 1 )
        {
            errorMsg.Printf( _( "Bezier point %d Y position not defined" ),
							i );
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


LibDrawBezier* LibDrawBezier::GenCopy()
{
    LibDrawBezier* newitem = new LibDrawBezier(GetParent());

    newitem->m_BezierPoints = m_BezierPoints;   // Vector copy
    newitem->m_Width   = m_Width;
    newitem->m_Unit    = m_Unit;
    newitem->m_Convert = m_Convert;
    newitem->m_Flags   = m_Flags;
    newitem->m_Fill    = m_Fill;
    return newitem;
}

void LibDrawBezier::Draw( WinEDA_DrawPanel* aPanel, wxDC* aDC,
						   const wxPoint& aOffset, int aColor, int aDrawMode,
						   void* aData, const int aTransformMatrix[2][2] )
{
    wxPoint         pos1;
	std::vector<wxPoint> PolyPointsTraslated;

    int             color     = ReturnLayerColor( LAYER_DEVICE );
    int             linewidth = (m_Width == 0) ? g_DrawDefaultLineThickness : m_Width;

    m_PolyPoints = Bezier2Poly( m_BezierPoints[0] ,
                           m_BezierPoints[1] ,
                           m_BezierPoints[2] ,
                           m_BezierPoints[3]);

	PolyPointsTraslated.clear();
	for( unsigned int i = 0; i < m_PolyPoints.size() ; i++)
		PolyPointsTraslated.push_back( TransformCoordinate( aTransformMatrix, m_PolyPoints[i] ) + aOffset);

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

    if( fill == FILLED_WITH_BG_BODYCOLOR )
        GRPoly( &aPanel->m_ClipBox, aDC, m_PolyPoints.size(),
                &PolyPointsTraslated[0], 1, linewidth, color,
                ReturnLayerColor( LAYER_DEVICE_BACKGROUND ) );
    else if( fill == FILLED_SHAPE  )
        GRPoly( &aPanel->m_ClipBox, aDC, m_PolyPoints.size(),
                &PolyPointsTraslated[0], 1, linewidth, color, color );
    else
        GRPoly( &aPanel->m_ClipBox, aDC, m_PolyPoints.size(),
                &PolyPointsTraslated[0], 0, linewidth, color, color );

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
	if(!GetCornerCount())
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

    LibEDA_BaseStruct::DisplayInfo( frame );

    msg = ReturnStringFromValue( g_UnitMetric, m_Width,
								EESCHEMA_INTERNAL_UNIT, true );

    frame->MsgPanel->Affiche_1_Parametre( 20, _( "Line width" ), msg, BLUE );

    msg.Printf( wxT( "(%d, %d, %d, %d)" ), bBox.GetOrigin().x,
			   bBox.GetOrigin().y, bBox.GetEnd().x, bBox.GetEnd().y );

    frame->MsgPanel->Affiche_1_Parametre( 40, _( "Bounding box" ), msg, BROWN );
}
