/************************/
/* class_body_items.cpp */
/************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "drawtxt.h"
#include "trigo.h"

#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "protos.h"


const wxChar* MsgPinElectricType[] =
{
    wxT( "input" ),
    wxT( "output" ),
    wxT( "BiDi" ),
    wxT( "3state" ),
    wxT( "passive" ),
    wxT( "unspc" ),
    wxT( "power_in" ),
    wxT( "power_out" ),
    wxT( "openCol" ),
    wxT( "openEm" ),
    wxT( "?????" )
};

static int    fill_tab[3] = { 'N', 'F', 'f' };

//#define DRAW_ARC_WITH_ANGLE		// Used to draw arcs


/* Base class (abstract) for components bodies items */
LibEDA_BaseStruct::LibEDA_BaseStruct( KICAD_T struct_type ) :
    EDA_BaseStruct( struct_type )
{
    m_Unit    = 0;   /* Unit identification (for multi part per package)
                      *  0 if the item is common to all units */
    m_Convert = 0;   /* Shape identification (for parts which have a convert
                      * shape) 0 if the item is common to all shapes */
    m_Fill    = NO_FILL;

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

LibDrawArc::LibDrawArc() : LibEDA_BaseStruct( COMPONENT_ARC_DRAW_TYPE )
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
 * @param aRefPos A wxPoint to test
 * @return bool - true if a hit, else false
 */
bool LibDrawArc::HitTest( const wxPoint& aRefPos )
{
    wxPoint relpos = aRefPos - m_Pos;
    int dist = wxRound( sqrt( ( (double) relpos.x * relpos.x ) + ( (double) relpos.y * relpos.y ) ) );
    int mindist = m_Width ? m_Width /2 : g_DrawDefaultLineThickness / 2;
    // Have a minimal tolerance for hit test
    if ( mindist < 3 )
        mindist = 3;        // = 3 mils
    if( abs( dist - m_Rayon ) > mindist )
        return false;
    
    // We are on the circle, ensure we are on the arc, between m_ArcStart and m_ArcEnd
    int astart = t1;    // arc starting point ( in 0.1 degree)
    int aend = t2;      // arc ending point ( in 0.1 degree)
    int atest = wxRound( atan2(relpos.y, relpos.x) * 1800.0 / M_PI );
    NORMALIZE_ANGLE_180(atest);
    NORMALIZE_ANGLE_180(astart);
    NORMALIZE_ANGLE_180(aend);

    if ( astart > aend )
        EXCHG(astart, aend);

    if( atest >= astart && atest <= aend )
        return true;
    
    return false;
}

LibDrawArc* LibDrawArc::GenCopy()
{
    LibDrawArc* newitem = new LibDrawArc();

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


LibDrawCircle::LibDrawCircle() : LibEDA_BaseStruct( COMPONENT_CIRCLE_DRAW_TYPE )
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
 * @param aRefPos A wxPoint to test
 * @return bool - true if a hit, else false
 */
bool LibDrawCircle::HitTest( const wxPoint& aRefPos )
{
    wxPoint relpos = aRefPos - m_Pos;
    int dist = wxRound( sqrt( ( (double) relpos.x * relpos.x ) + ( (double) relpos.y * relpos.y ) ) );
    int mindist = m_Width ? m_Width /2 : g_DrawDefaultLineThickness / 2;
    // Have a minimal tolerance for hit test
    if ( mindist < 3 )
        mindist = 3;        // = 3 mils
    if( abs( dist - m_Rayon ) > mindist )
        return false;

    return false;
}

LibDrawCircle* LibDrawCircle::GenCopy()
{
    LibDrawCircle* newitem = new LibDrawCircle();

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


/***********************/
/** class LibDrawText **/
/***********************/

LibDrawText::LibDrawText() :
    LibEDA_BaseStruct( COMPONENT_GRAPHIC_TEXT_DRAW_TYPE ), EDA_TextStruct()
{
    m_Size     = wxSize( 50, 50 );
    m_typeName = _( "Text" );
}


bool LibDrawText::Save( FILE* ExportFile ) const
{
    wxString text = m_Text;

    // Spaces are not allowed in text because it is not double quoted:
    // changed to '~'
    text.Replace( wxT( " " ), wxT( "~" ) );

    fprintf( ExportFile, "T %d %d %d %d %d %d %d %s ", m_Orient,
            m_Pos.x, m_Pos.y, m_Size.x, m_Attributs, m_Unit, m_Convert,
            CONV_TO_UTF8( text ) );
    fprintf( ExportFile, " %s %d", m_Italic ? "Italic" : "Normal", (m_Bold>0) ? 1 : 0 );

    char hjustify = 'C';
    if( m_HJustify == GR_TEXT_HJUSTIFY_LEFT )
        hjustify = 'L';
    else if( m_HJustify == GR_TEXT_HJUSTIFY_RIGHT )
        hjustify = 'R';

    char vjustify = 'C';
    if( m_VJustify == GR_TEXT_VJUSTIFY_BOTTOM )
        vjustify = 'B';
    else if( m_VJustify == GR_TEXT_VJUSTIFY_TOP )
        vjustify = 'T';

    fprintf( ExportFile, "%c %c", hjustify, vjustify );

    fprintf( ExportFile, "\n" );

    return true;
}


bool LibDrawText::Load( char* line, wxString& errorMsg )
{
    int  cnt, thickness;
    char hjustify = 'C', vjustify = 'C';
    char buf[256];
    char tmp[256];

    buf[0] = 0;
    tmp[0] = 0;         // For italic option, Not in old versions

    cnt = sscanf( &line[2], "%d %d %d %d %d %d %d %s %s %d %c %c",
                  &m_Orient, &m_Pos.x, &m_Pos.y, &m_Size.x, &m_Attributs,
                  &m_Unit, &m_Convert, buf, tmp, &thickness, &hjustify, &vjustify );

    if( cnt < 8 )
    {
        errorMsg.Printf( _( "text only had %d parameters of the required 8" ),
                         cnt );
        return false;
    }

    m_Size.y = m_Size.x;

    if( strnicmp( tmp, "Italic", 6 ) == 0 )
        m_Italic = true;
    if( thickness > 0 )
    {
        m_Bold = true;
    }

    switch( hjustify )
    {
    case 'L':
        m_HJustify = GR_TEXT_HJUSTIFY_LEFT;
        break;

    case 'C':
        m_HJustify = GR_TEXT_HJUSTIFY_CENTER;
        break;

    case 'R':
        m_HJustify = GR_TEXT_HJUSTIFY_RIGHT;
        break;
    }

    switch( vjustify )
    {
    case 'T':
        m_VJustify = GR_TEXT_VJUSTIFY_TOP;
        break;

    case 'C':
        m_VJustify = GR_TEXT_VJUSTIFY_CENTER;
        break;

    case 'B':
        m_VJustify = GR_TEXT_VJUSTIFY_BOTTOM;
        break;
    }

    /* Convert '~' to spaces. */
    m_Text = CONV_FROM_UTF8( buf );
    m_Text.Replace( wxT( "~" ), wxT( " " ) );

    return true;
}

/**
 * Function HitTest
 * tests if the given wxPoint is within the bounds of this object.
 * @param refPos A wxPoint to test
 * @return bool - true if a hit, else false
 */
bool LibDrawText::HitTest( const wxPoint& refPos )
{
    // if using TextHitTest() remember this function uses top to bottom y axis convention
    // and for lib items we are using bottom to top convention
    // so for non center Y justification we use a trick.
    GRTextVertJustifyType vJustify = m_VJustify;
    if ( m_VJustify == GR_TEXT_VJUSTIFY_TOP )
        m_VJustify = GR_TEXT_VJUSTIFY_BOTTOM;
    else if  ( m_VJustify == GR_TEXT_VJUSTIFY_BOTTOM )
        m_VJustify = GR_TEXT_VJUSTIFY_TOP;

    bool hit = TextHitTest(refPos);
    m_VJustify = vJustify;
    
    return hit;
}


LibDrawText* LibDrawText::GenCopy()
{
    LibDrawText* newitem = new LibDrawText();

    newitem->m_Pos = m_Pos;
    newitem->m_Orient    = m_Orient;
    newitem->m_Size      = m_Size;
    newitem->m_Attributs = m_Attributs;
    newitem->m_Unit      = m_Unit;
    newitem->m_Convert   = m_Convert;
    newitem->m_Flags     = m_Flags;
    newitem->m_Text      = m_Text;
    newitem->m_Width     = m_Width;
    newitem->m_Italic    = m_Italic;
    newitem->m_Bold      = m_Bold;
    newitem->m_HJustify  = m_HJustify;
    newitem->m_VJustify  = m_VJustify;
    return newitem;
}


void LibDrawText::Draw( WinEDA_DrawPanel* aPanel, wxDC* aDC,
                        const wxPoint& aOffset, int aColor, int aDrawMode,
                        void* aData, const int aTransformMatrix[2][2] )
{
    wxPoint pos1, pos2;

    int     color     = ReturnLayerColor( LAYER_DEVICE );
    int     linewidth = m_Width;

    if( linewidth == 0 )   // Use default values for pen size
    {
        if( m_Bold  )
            linewidth = GetPenSizeForBold( m_Size.x );
        else
            linewidth = g_DrawDefaultLineThickness;
    }

    // Clip pen size for small texts:
    linewidth = Clamp_Text_PenSize( linewidth, m_Size, m_Bold );

    if( aColor < 0 )       // Used normal color or selected color
    {
        if( ( m_Selected & IS_SELECTED ) )
            color = g_ItemSelectetColor;
    }
    else
        color = aColor;

    pos1 = TransformCoordinate( aTransformMatrix, m_Pos ) + aOffset;

    /* The text orientation may need to be flipped if the
     *  transformation matrix causes xy axes to be flipped. */
    int t1 = ( aTransformMatrix[0][0] != 0 ) ^ ( m_Orient != 0 );

    DrawGraphicText( aPanel, aDC, pos1, (EDA_Colors) color, m_Text,
                     t1 ? TEXT_ORIENT_HORIZ : TEXT_ORIENT_VERT,
                     m_Size, m_HJustify, m_VJustify,
                     linewidth, m_Italic, m_Bold );
}


void LibDrawText::DisplayInfo( WinEDA_DrawFrame* frame )
{
    wxString msg;

    LibEDA_BaseStruct::DisplayInfo( frame );

    msg = ReturnStringFromValue( g_UnitMetric, m_Width,
                                 EESCHEMA_INTERNAL_UNIT, true );

    frame->MsgPanel->Affiche_1_Parametre( 20, _( "Line width" ), msg, BLUE );
}


/*************************/
/** class LibDrawSquare **/
/*************************/

LibDrawSquare::LibDrawSquare() : LibEDA_BaseStruct( COMPONENT_RECT_DRAW_TYPE )
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
    LibDrawSquare* newitem = new LibDrawSquare();

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


LibDrawSegment::LibDrawSegment() : LibEDA_BaseStruct( COMPONENT_LINE_DRAW_TYPE )
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
    LibDrawSegment* newitem = new LibDrawSegment();

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


LibDrawPolyline::LibDrawPolyline() :
    LibEDA_BaseStruct( COMPONENT_POLYLINE_DRAW_TYPE )
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
    LibDrawPolyline* newitem = new LibDrawPolyline();

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
        ref   = aPosRef - start;
        end  -= start;

        if( distance( end.x, end.y, ref.x, ref.y, aThreshold ) )
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
