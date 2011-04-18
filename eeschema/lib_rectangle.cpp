/*************************/
/** class LIB_RECTANGLE **/
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
#include "lib_rectangle.h"
#include "transform.h"


LIB_RECTANGLE::LIB_RECTANGLE( LIB_COMPONENT* aParent ) :
    LIB_DRAW_ITEM( LIB_RECTANGLE_T, aParent )
{
    m_Width                = 0;
    m_Fill                 = NO_FILL;
    m_isFillable           = true;
    m_typeName             = _( "Rectangle" );
    m_isHeightLocked       = false;
    m_isWidthLocked        = false;
    m_isStartPointSelected = false;
}


LIB_RECTANGLE::LIB_RECTANGLE( const LIB_RECTANGLE& aRect ) :
    LIB_DRAW_ITEM( aRect )
{
    m_Pos   = aRect.m_Pos;
    m_End   = aRect.m_End;
    m_Width = aRect.m_Width;
    m_Fill  = aRect.m_Fill;
}


bool LIB_RECTANGLE::Save( FILE* aFile )
{
    if( fprintf( aFile, "S %d %d %d %d %d %d %d %c\n", m_Pos.x, m_Pos.y,
                 m_End.x, m_End.y, m_Unit, m_Convert, m_Width, fill_tab[m_Fill] ) < 0 )
        return false;

    return true;
}


bool LIB_RECTANGLE::Load( char* aLine, wxString& aErrorMsg )
{
    int  cnt;
    char tmp[256];

    cnt = sscanf( &aLine[2], "%d %d %d %d %d %d %d %s", &m_Pos.x, &m_Pos.y,
                  &m_End.x, &m_End.y, &m_Unit, &m_Convert, &m_Width, tmp );

    if( cnt < 7 )
    {
        aErrorMsg.Printf( _( "rectangle only had %d parameters of the required 7" ), cnt );
        return false;
    }

    if( tmp[0] == 'F' )
        m_Fill = FILLED_SHAPE;
    if( tmp[0] == 'f' )
        m_Fill = FILLED_WITH_BG_BODYCOLOR;

    return true;
}


LIB_DRAW_ITEM* LIB_RECTANGLE::DoGenCopy()
{
    LIB_RECTANGLE* newitem = new LIB_RECTANGLE( GetParent() );

    newitem->m_Pos     = m_Pos;
    newitem->m_End     = m_End;
    newitem->m_Width   = m_Width;
    newitem->m_Unit    = m_Unit;
    newitem->m_Convert = m_Convert;
    newitem->m_Flags   = m_Flags;
    newitem->m_Fill    = m_Fill;

    return (LIB_DRAW_ITEM*) newitem;
}


int LIB_RECTANGLE::DoCompare( const LIB_DRAW_ITEM& aOther ) const
{
    wxASSERT( aOther.Type() == LIB_RECTANGLE_T );

    const LIB_RECTANGLE* tmp = ( LIB_RECTANGLE* ) &aOther;

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


void LIB_RECTANGLE::DoOffset( const wxPoint& aOffset )
{
    m_Pos += aOffset;
    m_End += aOffset;
}


bool LIB_RECTANGLE::DoTestInside( EDA_RECT& aRect ) const
{
    return aRect.Contains( m_Pos.x, -m_Pos.y ) || aRect.Contains( m_End.x, -m_End.y );
}


void LIB_RECTANGLE::DoMove( const wxPoint& aPosition )
{
    wxPoint size = m_End - m_Pos;
    m_Pos = aPosition;
    m_End = aPosition + size;
}


void LIB_RECTANGLE::DoMirrorHorizontal( const wxPoint& aCenter )
{
    m_Pos.x -= aCenter.x;
    m_Pos.x *= -1;
    m_Pos.x += aCenter.x;
    m_End.x -= aCenter.x;
    m_End.x *= -1;
    m_End.x += aCenter.x;
}


void LIB_RECTANGLE::DoPlot( PLOTTER* aPlotter, const wxPoint& aOffset, bool aFill,
                            const TRANSFORM& aTransform )
{
    wxASSERT( aPlotter != NULL );

    wxPoint pos = aTransform.TransformCoordinate( m_Pos ) + aOffset;
    wxPoint end = aTransform.TransformCoordinate( m_End ) + aOffset;

    if( aFill && m_Fill == FILLED_WITH_BG_BODYCOLOR )
    {
        aPlotter->set_color( ReturnLayerColor( LAYER_DEVICE_BACKGROUND ) );
        aPlotter->rect( pos, end, FILLED_WITH_BG_BODYCOLOR, 0 );
    }

    bool already_filled = m_Fill == FILLED_WITH_BG_BODYCOLOR;
    aPlotter->set_color( ReturnLayerColor( LAYER_DEVICE ) );
    aPlotter->rect( pos, end, already_filled ? NO_FILL : m_Fill, GetPenSize() );
}


/**
 * Function GetPenSize
 * @return the size of the "pen" that be used to draw or plot this item
 */
int LIB_RECTANGLE::GetPenSize() const
{
    return ( m_Width == 0 ) ? g_DrawDefaultLineThickness : m_Width;
}

void LIB_RECTANGLE::drawGraphic( EDA_DRAW_PANEL* aPanel, wxDC* aDC,
                                 const wxPoint& aOffset, int aColor, int aDrawMode,
                                 void* aData, const TRANSFORM& aTransform )
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

    pos1 = aTransform.TransformCoordinate( m_Pos ) + aOffset;
    pos2 = aTransform.TransformCoordinate( m_End ) + aOffset;

    FILL_T fill = aData ? NO_FILL : m_Fill;
    if( aColor >= 0 )
        fill = NO_FILL;

    GRSetDrawMode( aDC, aDrawMode );

    if( fill == FILLED_WITH_BG_BODYCOLOR && !aData )
        GRFilledRect( &aPanel->m_ClipBox, aDC, pos1.x, pos1.y, pos2.x, pos2.y, GetPenSize( ),
                      (m_Flags & IS_MOVED) ? color : ReturnLayerColor( LAYER_DEVICE_BACKGROUND ),
                      ReturnLayerColor( LAYER_DEVICE_BACKGROUND ) );
    else if( m_Fill == FILLED_SHAPE  && !aData )
        GRFilledRect( &aPanel->m_ClipBox, aDC, pos1.x, pos1.y, pos2.x, pos2.y,
                      GetPenSize(), color, color );
    else
        GRRect( &aPanel->m_ClipBox, aDC, pos1.x, pos1.y, pos2.x, pos2.y, GetPenSize(), color );

    /* Set to one (1) to draw bounding box around rectangle to validate
     * bounding box calculation. */
#if 0
    EDA_RECT bBox = GetBoundingBox();
    bBox.Inflate( m_Thickness + 1, m_Thickness + 1 );
    GRRect( &aPanel->m_ClipBox, aDC, bBox.GetOrigin().x, bBox.GetOrigin().y,
            bBox.GetEnd().x, bBox.GetEnd().y, 0, LIGHTMAGENTA );
#endif
}


void LIB_RECTANGLE::DisplayInfo( EDA_DRAW_FRAME* aFrame )
{
    wxString msg;

    LIB_DRAW_ITEM::DisplayInfo( aFrame );

    msg = ReturnStringFromValue( g_UserUnit, m_Width, EESCHEMA_INTERNAL_UNIT, true );

    aFrame->AppendMsgPanel( _( "Line width" ), msg, BLUE );
}


EDA_RECT LIB_RECTANGLE::GetBoundingBox() const
{
    EDA_RECT rect;

    rect.SetOrigin( m_Pos.x, m_Pos.y * -1 );
    rect.SetEnd( m_End.x, m_End.y * -1 );
    rect.Inflate( m_Width / 2, m_Width / 2 );
    return rect;
}


bool LIB_RECTANGLE::HitTest( const wxPoint& aPosition )
{
    int mindist = ( m_Width ? m_Width / 2 : g_DrawDefaultLineThickness / 2 ) + 1;

    // Have a minimal tolerance for hit test
    if( mindist < MINIMUM_SELECTION_DISTANCE )
        mindist = MINIMUM_SELECTION_DISTANCE;

    return HitTest( aPosition, mindist, DefaultTransform );
}


bool LIB_RECTANGLE::HitTest( wxPoint aPosition, int aThreshold, const TRANSFORM& aTransform )
{
    wxPoint actualStart = aTransform.TransformCoordinate( m_Pos );
    wxPoint actualEnd   = aTransform.TransformCoordinate( m_End );

    // locate lower segment
    wxPoint start, end;

    start = actualStart;
    end.x = actualEnd.x;
    end.y = actualStart.y;
    if( TestSegmentHit( aPosition, start, end, aThreshold ) )
        return true;

    // locate right segment
    start.x = actualEnd.x;
    end.y   = actualEnd.y;
    if( TestSegmentHit( aPosition, start, end, aThreshold ) )
        return true;

    // locate upper segment
    start.y = actualEnd.y;
    end.x   = actualStart.x;
    if( TestSegmentHit( aPosition, start, end, aThreshold ) )
        return true;

    // locate left segment
    start = actualStart;
    end.x = actualStart.x;
    end.y = actualEnd.y;
    if( TestSegmentHit( aPosition, start, end, aThreshold ) )
        return true;

    return false;
}


void LIB_RECTANGLE::BeginEdit( int aEditMode, const wxPoint aPosition )
{
    wxCHECK_RET( ( aEditMode & ( IS_NEW | IS_MOVED | IS_RESIZED ) ) != 0,
                 wxT( "Invalid edit mode for LIB_RECTANGLE object." ) );

    if( aEditMode == IS_NEW )
    {
        m_Pos = m_End = aPosition;
    }
    else if( aEditMode == IS_RESIZED )
    {
        m_isStartPointSelected = abs( m_Pos.x - aPosition.x ) < MINIMUM_SELECTION_DISTANCE
            || abs( m_Pos.y - aPosition.y ) < MINIMUM_SELECTION_DISTANCE;

        if( m_isStartPointSelected )
        {
            m_isWidthLocked = abs( m_Pos.x - aPosition.x ) >= MINIMUM_SELECTION_DISTANCE;
            m_isHeightLocked = abs( m_Pos.y - aPosition.y ) >= MINIMUM_SELECTION_DISTANCE;
        }
        else
        {
            m_isWidthLocked = abs( m_End.x - aPosition.x ) >= MINIMUM_SELECTION_DISTANCE;
            m_isHeightLocked = abs( m_End.y - aPosition.y ) >= MINIMUM_SELECTION_DISTANCE;
        }

        SetEraseLastDrawItem();
    }
    else if( aEditMode == IS_MOVED )
    {
        m_initialPos = m_Pos;
        m_initialCursorPos = aPosition;
        SetEraseLastDrawItem();
    }

    m_Flags = aEditMode;
}


bool LIB_RECTANGLE::ContinueEdit( const wxPoint aPosition )
{
    wxCHECK_MSG( ( m_Flags & ( IS_NEW | IS_MOVED | IS_RESIZED ) ) != 0, false,
                   wxT( "Bad call to ContinueEdit().  LIB_RECTANGLE is not being edited." ) );

    return false;
}


void LIB_RECTANGLE::EndEdit( const wxPoint& aPosition, bool aAbort )
{
    wxCHECK_RET( ( m_Flags & ( IS_NEW | IS_MOVED | IS_RESIZED ) ) != 0,
                   wxT( "Bad call to EndEdit().  LIB_RECTANGLE is not being edited." ) );

    m_Flags = 0;
    m_isHeightLocked = false;
    m_isWidthLocked  = false;
    SetEraseLastDrawItem( false );
}


void LIB_RECTANGLE::calcEdit( const wxPoint& aPosition )
{
    if( m_Flags == IS_NEW )
    {
        m_End = aPosition;
        SetEraseLastDrawItem();
    }
    else if( m_Flags == IS_RESIZED )
    {
        if( m_isHeightLocked )
        {
            if( m_isStartPointSelected )
                m_Pos.x = aPosition.x;
            else
                m_End.x = aPosition.x;
        }
        else if( m_isWidthLocked )
        {
            if( m_isStartPointSelected )
                m_Pos.y = aPosition.y;
            else
                m_End.y = aPosition.y;
        }
        else
        {
            if( m_isStartPointSelected )
                m_Pos = aPosition;
            else
                m_End = aPosition;
        }
    }
    else if( m_Flags == IS_MOVED )
    {
        Move( m_initialPos + aPosition - m_initialCursorPos );
    }
}
