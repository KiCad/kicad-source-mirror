/*************************/
/** class LIB_CIRCLE **/
/*************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "plot_common.h"
#include "trigo.h"
#include "wxstruct.h"

#include "general.h"
#include "protos.h"
#include "lib_circle.h"
#include "transform.h"


LIB_CIRCLE::LIB_CIRCLE( LIB_COMPONENT* aParent ) :
    LIB_DRAW_ITEM( LIB_CIRCLE_T, aParent )
{
    m_Radius     = 0;
    m_Fill       = NO_FILL;
    m_isFillable = true;
    m_typeName   = _( "Circle" );
}


LIB_CIRCLE::LIB_CIRCLE( const LIB_CIRCLE& aCircle ) :
    LIB_DRAW_ITEM( aCircle )
{
    m_Pos    = aCircle.m_Pos;
    m_Radius = aCircle.m_Radius;
    m_Fill   = aCircle.m_Fill;
}


bool LIB_CIRCLE::Save( FILE* aFile )
{
    if( fprintf( aFile, "C %d %d %d %d %d %d %c\n", m_Pos.x, m_Pos.y,
                 m_Radius, m_Unit, m_Convert, m_Width, fill_tab[m_Fill] ) < 0 )
        return false;

    return true;
}


bool LIB_CIRCLE::Load( char* aLine, wxString& aErrorMsg )
{
    char tmp[256];

    int  cnt = sscanf( &aLine[2], "%d %d %d %d %d %d %s", &m_Pos.x, &m_Pos.y,
                       &m_Radius, &m_Unit, &m_Convert, &m_Width, tmp );

    if( cnt < 6 )
    {
        aErrorMsg.Printf( _( "circle only had %d parameters of the required 6" ), cnt );
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
 * @return - true if a hit, else false
 */
bool LIB_CIRCLE::HitTest( const wxPoint& aPosRef )
{
    int mindist = m_Width ? m_Width / 2 : g_DrawDefaultLineThickness / 2;

    // Have a minimal tolerance for hit test
    if( mindist < MINIMUM_SELECTION_DISTANCE )
        mindist = MINIMUM_SELECTION_DISTANCE;

    return HitTest( aPosRef, mindist, DefaultTransform );
}


/**
 * Function HitTest
 * @return true if the point aPosRef is near this object
 * @param aPosRef = a wxPoint to test
 * @param aThreshold = max distance to this object (usually the half thickness of a line)
 * @param aTransform = the transform matrix
 */
bool LIB_CIRCLE::HitTest( wxPoint aPosRef, int aThreshold, const TRANSFORM& aTransform )
{
    wxPoint relpos = aPosRef - aTransform.TransformCoordinate( m_Pos );

    int dist = wxRound( sqrt( ( (double) relpos.x * relpos.x ) +
                              ( (double) relpos.y * relpos.y ) ) );

    if( abs( dist - m_Radius ) <= aThreshold )
        return true;
    return false;
}


LIB_DRAW_ITEM* LIB_CIRCLE::DoGenCopy()
{
    LIB_CIRCLE* newitem = new LIB_CIRCLE( GetParent() );

    newitem->m_Pos     = m_Pos;
    newitem->m_Radius  = m_Radius;
    newitem->m_Width   = m_Width;
    newitem->m_Unit    = m_Unit;
    newitem->m_Convert = m_Convert;
    newitem->m_Flags   = m_Flags;
    newitem->m_Fill    = m_Fill;

    return (LIB_DRAW_ITEM*) newitem;
}


int LIB_CIRCLE::DoCompare( const LIB_DRAW_ITEM& aOther ) const
{
    wxASSERT( aOther.Type() == LIB_CIRCLE_T );

    const LIB_CIRCLE* tmp = ( LIB_CIRCLE* ) &aOther;

    if( m_Pos.x != tmp->m_Pos.x )
        return m_Pos.x - tmp->m_Pos.x;

    if( m_Pos.y != tmp->m_Pos.y )
        return m_Pos.y - tmp->m_Pos.y;

    if( m_Radius != tmp->m_Radius )
        return m_Radius - tmp->m_Radius;

    return 0;
}


void LIB_CIRCLE::DoOffset( const wxPoint& aOffset )
{
    m_Pos += aOffset;
}


bool LIB_CIRCLE::DoTestInside( EDA_Rect& aRect ) const
{
    /*
     * FIXME: This fails to take into account the radius around the center
     *        point.
     */
    return aRect.Inside( m_Pos.x, -m_Pos.y );
}


void LIB_CIRCLE::DoMove( const wxPoint& aPosition )
{
    m_Pos = aPosition;
}


void LIB_CIRCLE::DoMirrorHorizontal( const wxPoint& aCenter )
{
    m_Pos.x -= aCenter.x;
    m_Pos.x *= -1;
    m_Pos.x += aCenter.x;
}


void LIB_CIRCLE::DoPlot( PLOTTER* aPlotter, const wxPoint& aOffset, bool aFill,
                         const TRANSFORM& aTransform )
{
    wxPoint pos = aTransform.TransformCoordinate( m_Pos ) + aOffset;

    if( aFill && m_Fill == FILLED_WITH_BG_BODYCOLOR )
    {
        aPlotter->set_color( ReturnLayerColor( LAYER_DEVICE_BACKGROUND ) );
        aPlotter->circle( pos, m_Radius * 2, FILLED_SHAPE, 0 );
    }

    bool already_filled = m_Fill == FILLED_WITH_BG_BODYCOLOR;
    aPlotter->set_color( ReturnLayerColor( LAYER_DEVICE ) );
    aPlotter->circle( pos, m_Radius * 2, already_filled ? NO_FILL : m_Fill, GetPenSize() );
}


/**
 * Function GetPenSize
 * @return the size of the "pen" that be used to draw or plot this item
 */
int LIB_CIRCLE::GetPenSize()
{
    return ( m_Width == 0 ) ? g_DrawDefaultLineThickness : m_Width;
}


void LIB_CIRCLE::drawGraphic( WinEDA_DrawPanel* aPanel, wxDC* aDC, const wxPoint& aOffset,
                              int aColor, int aDrawMode, void* aData, const TRANSFORM& aTransform )
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

    pos1 = aTransform.TransformCoordinate( m_Pos ) + aOffset;
    GRSetDrawMode( aDC, aDrawMode );

    FILL_T fill = aData ? NO_FILL : m_Fill;
    if( aColor >= 0 )
        fill = NO_FILL;

    if( fill == FILLED_WITH_BG_BODYCOLOR )
        GRFilledCircle( &aPanel->m_ClipBox, aDC, pos1.x, pos1.y, m_Radius, GetPenSize(),
                        (m_Flags & IS_MOVED) ? color : ReturnLayerColor( LAYER_DEVICE_BACKGROUND ),
                        ReturnLayerColor( LAYER_DEVICE_BACKGROUND ) );
    else if( fill == FILLED_SHAPE )
        GRFilledCircle( &aPanel->m_ClipBox, aDC, pos1.x, pos1.y, m_Radius, 0, color, color );
    else
        GRCircle( &aPanel->m_ClipBox, aDC, pos1.x, pos1.y, m_Radius, GetPenSize( ), color );

    /* Set to one (1) to draw bounding box around circle to validate bounding
     * box calculation. */
#if 0
    EDA_Rect bBox = GetBoundingBox();
    GRRect( &aPanel->m_ClipBox, aDC, bBox.GetOrigin().x, bBox.GetOrigin().y,
            bBox.GetEnd().x, bBox.GetEnd().y, 0, LIGHTMAGENTA );
#endif
}


EDA_Rect LIB_CIRCLE::GetBoundingBox() const
{
    EDA_Rect rect;

    rect.SetOrigin( m_Pos.x - m_Radius, ( m_Pos.y - m_Radius ) * -1 );
    rect.SetEnd( m_Pos.x + m_Radius, ( m_Pos.y + m_Radius ) * -1 );
    rect.Inflate( m_Width / 2, m_Width / 2 );

    return rect;
}


void LIB_CIRCLE::DisplayInfo( WinEDA_DrawFrame* aFrame )
{
    wxString msg;
    EDA_Rect bBox = GetBoundingBox();

    LIB_DRAW_ITEM::DisplayInfo( aFrame );

    msg = ReturnStringFromValue( g_UserUnit, m_Width, EESCHEMA_INTERNAL_UNIT, true );

    aFrame->AppendMsgPanel( _( "Line width" ), msg, BLUE );

    msg = ReturnStringFromValue( g_UserUnit, m_Radius, EESCHEMA_INTERNAL_UNIT, true );
    aFrame->AppendMsgPanel( _( "Radius" ), msg, RED );

    msg.Printf( wxT( "(%d, %d, %d, %d)" ), bBox.GetOrigin().x,
                bBox.GetOrigin().y, bBox.GetEnd().x, bBox.GetEnd().y );

    aFrame->AppendMsgPanel( _( "Bounding box" ), msg, BROWN );
}


void LIB_CIRCLE::BeginEdit( int aEditMode, const wxPoint aPosition )
{
    wxCHECK_RET( ( aEditMode & ( IS_NEW | IS_MOVED | IS_RESIZED ) ) != 0,
                 wxT( "Invalid edit mode for LIB_CIRCLE object." ) );

    if( aEditMode == IS_NEW )
    {
        m_Pos = m_initialPos = aPosition;
    }
    else if( aEditMode == IS_MOVED )
    {
        m_initialPos = m_Pos;
        m_initialCursorPos = aPosition;
        SetEraseLastDrawItem();
    }
    else if( aEditMode == IS_RESIZED )
    {
        SetEraseLastDrawItem();
    }

    m_Flags = aEditMode;
}


bool LIB_CIRCLE::ContinueEdit( const wxPoint aPosition )
{
    wxCHECK_MSG( ( m_Flags & ( IS_NEW | IS_MOVED | IS_RESIZED ) ) != 0, false,
                   wxT( "Bad call to ContinueEdit().  LIB_CIRCLE is not being edited." ) );

    return false;
}


void LIB_CIRCLE::EndEdit( const wxPoint& aPosition, bool aAbort )
{
    wxCHECK_RET( ( m_Flags & ( IS_NEW | IS_MOVED | IS_RESIZED ) ) != 0,
                   wxT( "Bad call to EndEdit().  LIB_CIRCLE is not being edited." ) );

    SetEraseLastDrawItem( false );
    m_Flags = 0;
}


void LIB_CIRCLE::calcEdit( const wxPoint& aPosition )
{
    if( m_Flags == IS_NEW || m_Flags == IS_RESIZED )
    {
        if( m_Flags == IS_NEW )
            SetEraseLastDrawItem();

        int dx = m_Pos.x - aPosition.x;
        int dy = m_Pos.y - aPosition.y;
        m_Radius = wxRound( sqrt( ( (double) dx * dx ) + ( (double) dy * dy ) ) );
    }
    else
    {
        Move( m_initialPos + aPosition - m_initialCursorPos );
    }
}
