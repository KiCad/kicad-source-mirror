/*********************************************************************/
/*          EESchema - symbdraw.cpp                                  */
/* Create, move .. graphic shapes used to build and draw a component */
/* (lines, arcs ..                                                   */
/*********************************************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "eeschema_id.h"

#include "program.h"
#include "general.h"
#include "trigo.h"
#include "protos.h"
#include "libeditfrm.h"
#include "class_libentry.h"
#include "dialog_lib_edit_draw_item.h"


static void SymbolDisplayDraw( WinEDA_DrawPanel* panel, wxDC* DC, bool erase );
static void ComputeArc( LIB_ARC* DrawItem, wxPoint ArcCentre );
static void RedrawWhileMovingCursor( WinEDA_DrawPanel* panel,
                                     wxDC*             DC,
                                     bool              erase );


static int     StateDrawArc, ArcStartX, ArcStartY, ArcEndX, ArcEndY;
static wxPoint InitPosition, StartCursor, ItemPreviousPos;


/*
 * Show the dialog box for editing a graphical item properties
 */
void WinEDA_LibeditFrame::EditGraphicSymbol( wxDC* DC, LIB_DRAW_ITEM* DrawItem )
{
    if( DrawItem == NULL )
        return;

    LIB_COMPONENT* component = DrawItem->GetParent();

    DIALOG_LIB_EDIT_DRAW_ITEM dlg( this, DrawItem->m_typeName );

    dlg.SetWidthUnits( ReturnUnitSymbol( g_UnitMetric ) );

    wxString val = ReturnStringFromValue( g_UnitMetric, m_drawLineWidth,
                                          m_InternalUnits );
    dlg.SetWidth( val );
    dlg.SetApplyToAllUnits( !m_drawSpecificUnit );
    dlg.EnableApplyToAllUnits( component && component->GetPartCount() > 1 );
    dlg.SetApplyToAllConversions( !m_drawSpecificConvert );
    dlg.EnableApplyToAllConversions( component && component->HasConversion() );
    dlg.SetFillStyle( m_drawFillStyle );
    dlg.EnableFillStyle( DrawItem->IsFillable() );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    val = dlg.GetWidth();
    m_drawLineWidth       = ReturnValueFromString( g_UnitMetric, val,
                                                   m_InternalUnits );
    m_drawSpecificConvert = !dlg.GetApplyToAllConversions();
    m_drawSpecificUnit    = !dlg.GetApplyToAllUnits();

    if( DrawItem->IsFillable() )
        m_drawFillStyle = (FILL_T) dlg.GetFillStyle();

    // Save copy for undo is done before place.
    if( !( DrawItem->m_Flags & IS_NEW ) )
        SaveCopyInUndoList( DrawItem->GetParent() );

    if( m_drawSpecificUnit )
        DrawItem->m_Unit = GetUnit();
    else
        DrawItem->m_Unit = 0;

    if( m_drawSpecificConvert )
        DrawItem->m_Convert = GetConvert();
    else
        DrawItem->m_Convert = 0;

    if( DrawItem->IsFillable() )
        DrawItem->m_Fill = m_drawFillStyle;

    DrawItem->SetWidth( m_drawLineWidth );

    if( component )
        component->GetDrawItemList().sort();
    GetScreen()->SetModify();

    DrawItem->DisplayInfo( this );
    DrawPanel->Refresh();
}


static void AbortSymbolTraceOn( WinEDA_DrawPanel* Panel, wxDC* DC )
{
    LIB_DRAW_ITEM* item;
    WinEDA_LibeditFrame* parent = ( WinEDA_LibeditFrame* ) Panel->GetParent();

    item = parent->GetDrawItem();

    if( item == NULL )
        return;

    StateDrawArc = 0;
    Panel->ManageCurseur = NULL;
    Panel->ForceCloseManageCurseur = NULL;

    if( item->m_Flags & IS_NEW )
    {
        if( item->Type() == COMPONENT_ARC_DRAW_TYPE )
            Panel->GetParent()->RedrawActiveWindow( DC, TRUE );
        else
            item->Draw( Panel, DC, wxPoint( 0, 0 ), -1, g_XorMode, NULL,
                        DefaultTransformMatrix );

        SAFE_DELETE( item );
        parent->SetDrawItem( NULL );
    }
    else
    {
        wxPoint curpos = Panel->GetScreen()->m_Curseur;
        Panel->GetScreen()->m_Curseur = StartCursor;
        RedrawWhileMovingCursor( Panel, DC, TRUE );
        Panel->GetScreen()->m_Curseur = curpos;
        item->Draw( Panel, DC, wxPoint( 0, 0 ), -1, GR_DEFAULT_DRAWMODE, NULL,
                    DefaultTransformMatrix );
        item->m_Flags = 0;
    }
}


LIB_DRAW_ITEM* WinEDA_LibeditFrame::CreateGraphicItem( LIB_COMPONENT* LibEntry,
                                                       wxDC*          DC )
{
    DrawPanel->ManageCurseur = SymbolDisplayDraw;
    DrawPanel->ForceCloseManageCurseur = AbortSymbolTraceOn;

    switch( m_ID_current_state )
    {
    case ID_LIBEDIT_BODY_ARC_BUTT:
    {
        LIB_ARC* Arc = new LIB_ARC( LibEntry );

        m_drawItem = Arc;
        ArcStartX    = ArcEndX = GetScreen()->m_Curseur.x;
        ArcStartY    = ArcEndY = -( GetScreen()->m_Curseur.y );
        StateDrawArc = 1;
        Arc->m_Fill  = m_drawFillStyle;
        Arc->m_Width = m_drawLineWidth;
    }
    break;

    case ID_LIBEDIT_BODY_CIRCLE_BUTT:
    {
        LIB_CIRCLE* Circle = new LIB_CIRCLE( LibEntry );

        m_drawItem = Circle;
        Circle->m_Pos   = GetScreen()->m_Curseur;
        NEGATE( Circle->m_Pos.y );
        Circle->m_Fill  = m_drawFillStyle;
        Circle->m_Width = m_drawLineWidth;
    }
    break;

    case ID_LIBEDIT_BODY_RECT_BUTT:
    {
        LIB_RECTANGLE* Square = new LIB_RECTANGLE( LibEntry );

        m_drawItem = Square;
        Square->m_Pos   = GetScreen()->m_Curseur;
        NEGATE( Square->m_Pos.y );
        Square->m_End   = Square->m_Pos;
        Square->m_Fill  = m_drawFillStyle;
        Square->m_Width = m_drawLineWidth;
    }
    break;

    case ID_LIBEDIT_BODY_LINE_BUTT:
    {
        LIB_POLYLINE* polyline = new LIB_POLYLINE( LibEntry );
        m_drawItem = polyline;
        wxPoint          point = GetScreen()->m_Curseur;
        NEGATE( point.y );
        polyline->AddPoint( point );    // Start point of the current segment
        polyline->AddPoint( point );    // End point of the current segment
        polyline->m_Fill  = m_drawFillStyle;
        polyline->m_Width = m_drawLineWidth;
    }
    break;

    case COMPONENT_LINE_DRAW_TYPE:
    {
        LIB_SEGMENT* Segment = new LIB_SEGMENT( LibEntry );

        m_drawItem = Segment;
        Segment->m_Pos  = GetScreen()->m_Curseur;
        NEGATE( Segment->m_Pos.y );
        Segment->m_End   = Segment->m_Pos;
        Segment->m_Width = m_drawLineWidth;
    }
    break;

    case ID_LIBEDIT_BODY_TEXT_BUTT:
    {
        LIB_TEXT* Text = new LIB_TEXT( LibEntry );

        m_drawItem = Text;
        Text->m_Size.x  = Text->m_Size.y = m_textSize;
        Text->m_Orient  = m_textOrientation;
        Text->m_Pos   = GetScreen()->m_Curseur;
        NEGATE( Text->m_Pos.y );
        
        // Enter the graphic text info
        DrawPanel->m_IgnoreMouseEvents = true;
        EditSymbolText( NULL, Text );
        DrawPanel->MouseToCursorSchema();
        DrawPanel->m_IgnoreMouseEvents = false;
        
        if( Text->m_Text.IsEmpty() )
        {
            SAFE_DELETE( Text );
            m_drawItem = NULL;
            DrawPanel->ManageCurseur = NULL;
            DrawPanel->ForceCloseManageCurseur = NULL;
        }
        else
        {
            StartMoveDrawSymbol( DC );
            Text->Draw( DrawPanel, DC, wxPoint( 0, 0 ), -1, g_XorMode, NULL,
                        DefaultTransformMatrix );
        }
    }
    break;

    default:
        DisplayError( this, wxT( "WinEDA_LibeditFrame::CreateGraphicItem() \
error" ) );
        return NULL;
    }

    if( m_drawItem )
    {
        m_drawItem->m_Flags |= IS_NEW;
        if( m_drawSpecificUnit )
            m_drawItem->m_Unit = m_unit;
        if( m_drawSpecificConvert )
            m_drawItem->m_Convert = m_convert;
    }

    DrawPanel->MouseToCursorSchema();
    DrawPanel->m_IgnoreMouseEvents = FALSE;

    return m_drawItem;
}


/* Create new library component graphic object.
 */
void WinEDA_LibeditFrame::GraphicItemBeginDraw( wxDC* DC )
{
    if( m_drawItem == NULL )
        return;

    switch( m_drawItem->Type() )
    {
    case COMPONENT_ARC_DRAW_TYPE:
        if( StateDrawArc == 1 )
        {
            SymbolDisplayDraw( DrawPanel, DC, FALSE );
            StateDrawArc = 2;
            SymbolDisplayDraw( DrawPanel, DC, FALSE );
            break;
        }
        if( StateDrawArc > 1 )
        {
            EndDrawGraphicItem( DC );
            return;
        }
        break;

    case COMPONENT_CIRCLE_DRAW_TYPE:
    case COMPONENT_RECT_DRAW_TYPE:
    case COMPONENT_GRAPHIC_TEXT_DRAW_TYPE:
        EndDrawGraphicItem( DC );
        return;

    case COMPONENT_POLYLINE_DRAW_TYPE:
    {
        wxPoint pos = GetScreen()->m_Curseur;
        NEGATE( pos.y );
        ( (LIB_POLYLINE*) m_drawItem )->AddPoint( pos );
    }
    break;

    case COMPONENT_LINE_DRAW_TYPE:
        break;

    default:
        ;
    }
}


/*
 * Redraw the graphic shape while moving
 */
static void RedrawWhileMovingCursor( WinEDA_DrawPanel* panel,
                                     wxDC*             DC,
                                     bool              erase )
{
    LIB_DRAW_ITEM* item;

    item = ( ( WinEDA_LibeditFrame* ) panel->GetParent() )->GetDrawItem();

    if( item == NULL )
        return;

    BASE_SCREEN* Screen = panel->GetScreen();
    wxPoint      pos;

    /* Erase shape in the old position*/
    if( erase )
    {
        pos = ItemPreviousPos - StartCursor;
        item->Draw( panel, DC, pos, -1, g_XorMode, NULL,
                    DefaultTransformMatrix );
    }

    /* Redraw moved shape */
    pos = Screen->m_Curseur - StartCursor;
    item->Draw( panel, DC, pos, -1, g_XorMode, NULL, DefaultTransformMatrix );
    ItemPreviousPos = Screen->m_Curseur;
}


void WinEDA_LibeditFrame::StartMoveDrawSymbol( wxDC* DC )
{
    if( m_drawItem == NULL )
        return;

    SetCursor( wxCURSOR_HAND );

    m_drawItem->m_Flags |= IS_MOVED;
    StartCursor = GetScreen()->m_Curseur;
    InitPosition = m_drawItem->GetPosition();
    ItemPreviousPos = GetScreen()->m_Curseur;
    DrawPanel->ManageCurseur = RedrawWhileMovingCursor;
    DrawPanel->ForceCloseManageCurseur = AbortSymbolTraceOn;
    DrawPanel->ManageCurseur( DrawPanel, DC, TRUE );
}


/* Manage mouse events when creating new graphic object. */
static void SymbolDisplayDraw( WinEDA_DrawPanel* panel, wxDC* DC, bool erase )
{
    int            dx, dy;
    int            DrawMode  = g_XorMode;
    BASE_SCREEN*   Screen    = panel->GetScreen();
    wxPoint        curr_pos  = Screen->m_Curseur;
    FILL_T         fillStyle = NO_FILL;
    LIB_DRAW_ITEM* item;

    item = ( ( WinEDA_LibeditFrame* ) panel->GetParent() )->GetDrawItem();

    if( item == NULL )
        return;

    fillStyle = ( ( WinEDA_LibeditFrame* ) panel->GetParent() )->GetFillStyle();

    NEGATE( curr_pos.y );

    GRSetDrawMode( DC, DrawMode );

    if( erase )
    {
        if( StateDrawArc == 1 )
        {
            int Color = ReturnLayerColor( LAYER_DEVICE );
            GRLine( &panel->m_ClipBox, DC, ArcStartX, -ArcStartY,
                    ArcEndX, -ArcEndY, 0, Color );
        }
        else
        {
            item->Draw( panel, DC, wxPoint( 0, 0 ), -1, DrawMode, NULL,
                        DefaultTransformMatrix );
            if( item->Type() == COMPONENT_ARC_DRAW_TYPE )
            {
                int Color = ReturnLayerColor( LAYER_DEVICE );
                GRDashedLine( &panel->m_ClipBox, DC, ArcStartX, -ArcStartY,
                              ( (LIB_ARC*) item )->m_Pos.x,
                              -( (LIB_ARC*) item )->m_Pos.y,
                              0, Color );
                GRDashedLine( &panel->m_ClipBox, DC, ArcEndX, -ArcEndY,
                              ( (LIB_ARC*) item )->m_Pos.x,
                              -( (LIB_ARC*) item )->m_Pos.y,
                              0, Color );
            }
        }
    }

    switch( item->Type() )
    {
    case COMPONENT_ARC_DRAW_TYPE:
        if( StateDrawArc == 1 )
        {
            ArcEndX = curr_pos.x;
            ArcEndY = curr_pos.y;
        }

        if( StateDrawArc == 2 )
        {
            ComputeArc( (LIB_ARC*) item, Screen->m_Curseur );
        }
        item ->m_Fill = fillStyle;
        break;

    case COMPONENT_CIRCLE_DRAW_TYPE:
        dx = ( (LIB_CIRCLE*) item )->m_Pos.x - curr_pos.x;
        dy = ( (LIB_CIRCLE*) item )->m_Pos.y - curr_pos.y;
        ( (LIB_CIRCLE*) item )->m_Radius =
            (int) sqrt( ( (double) dx * dx ) + ( (double) dy * dy ) );
        item->m_Fill = fillStyle;
        break;

    case COMPONENT_RECT_DRAW_TYPE:
        ( (LIB_RECTANGLE*) item )->m_End  = curr_pos;
        item->m_Fill = fillStyle;
        break;

    case COMPONENT_POLYLINE_DRAW_TYPE:
    {
        unsigned idx = ( (LIB_POLYLINE*) item )->GetCornerCount() - 1;
        ( (LIB_POLYLINE*) item )->m_PolyPoints[idx] = curr_pos;
        item->m_Fill = fillStyle;
    }
    break;

    case COMPONENT_LINE_DRAW_TYPE:
        ( (LIB_SEGMENT*) item )->m_End = curr_pos;
        break;

    case COMPONENT_GRAPHIC_TEXT_DRAW_TYPE:
        break;

    default:
        ;
    }

    if( StateDrawArc == 1 )
    {
        int Color = ReturnLayerColor( LAYER_DEVICE );
        GRLine( &panel->m_ClipBox, DC, ArcStartX, -ArcStartY, ArcEndX,
                -ArcEndY, 0, Color );
    }
    else
    {
        item->Draw( panel, DC, wxPoint( 0, 0 ), -1, DrawMode, NULL,
                    DefaultTransformMatrix );

        if( item->Type() == COMPONENT_ARC_DRAW_TYPE )
        {
            int Color = ReturnLayerColor( LAYER_DEVICE );
            GRDashedLine( &panel->m_ClipBox, DC, ArcStartX, -ArcStartY,
                          ( (LIB_ARC*) item )->m_Pos.x,
                          -( (LIB_ARC*) item )->m_Pos.y,
                          0, Color );
            GRDashedLine( &panel->m_ClipBox, DC, ArcEndX, -ArcEndY,
                          ( (LIB_ARC*) item )->m_Pos.x,
                          -( (LIB_ARC*) item )->m_Pos.y,
                          0, Color );
        }
    }
}


/*
 * Place the new graphic object in the list of component drawing objects.
 */
void WinEDA_LibeditFrame::EndDrawGraphicItem( wxDC* DC )
{
    if( m_component == NULL || m_drawItem == NULL )
        return;

    if( m_drawItem->Type() == COMPONENT_ARC_DRAW_TYPE )
    {
        if( StateDrawArc == 1 ) /* Trace arc under way must be completed. */
        {
            DisplayError( this, wxT( "Arc in progress.." ) );
            return;
        }
        else
        {
            if( ( m_drawItem->m_Flags & IS_MOVED ) == 0 )
                SymbolDisplayDraw( DrawPanel, DC, FALSE );
        }
    }

    StateDrawArc = 0;

    if( m_drawItem->m_Flags & IS_NEW )
    {
        SaveCopyInUndoList( m_component );
        m_component->AddDrawItem( m_drawItem );

        switch( m_drawItem->Type() )
        {
        case COMPONENT_ARC_DRAW_TYPE:
            ( (LIB_ARC*) m_drawItem )->m_Fill = m_drawFillStyle;
            break;

        case COMPONENT_CIRCLE_DRAW_TYPE:
            ( (LIB_CIRCLE*) m_drawItem )->m_Fill = m_drawFillStyle;
            break;

        case COMPONENT_RECT_DRAW_TYPE:
            ( (LIB_RECTANGLE*) m_drawItem )->m_Fill = m_drawFillStyle;
            break;

        case COMPONENT_POLYLINE_DRAW_TYPE:
            ( (LIB_POLYLINE*) m_drawItem )->m_Fill = m_drawFillStyle;
            break;

        case COMPONENT_PIN_DRAW_TYPE:
        case COMPONENT_LINE_DRAW_TYPE:
        case COMPONENT_GRAPHIC_TEXT_DRAW_TYPE:
            break;

        default:
            ;
        }
    }

    if( m_ID_current_state )
        SetCursor( wxCURSOR_PENCIL );
    else
        SetCursor( wxCURSOR_ARROW );

    if( m_drawItem->m_Flags & IS_MOVED )
    {
        wxPoint pos;
        pos.x = GetScreen()->m_Curseur.x + InitPosition.x - StartCursor.x,
        pos.y = GetScreen()->m_Curseur.y - InitPosition.y - StartCursor.y;
        NEGATE( pos.y );
        m_drawItem->Move( pos );
    }

    m_component->Draw( DrawPanel, DC, wxPoint( 0, 0 ), m_unit, m_convert,
                       GR_DEFAULT_DRAWMODE );

    m_drawItem->m_Flags = 0;
    m_drawItem = NULL;

    GetScreen()->SetModify();

    DrawPanel->ManageCurseur = NULL;
    DrawPanel->ForceCloseManageCurseur = NULL;
}


/*
 * Routine for adjusting the parameters of the arc currently being drawn.
 * Calculates the center, radius, angles for the arc current
 * Passes through the points ArcStartX, ArcEndX Y and Y with the nearest center
 * of the mouse position.
 * Note: The center is obviously not on the grid
 */
static void ComputeArc( LIB_ARC* DrawItem, wxPoint ArcCentre )
{
    int dx, dy;
    int cX, cY;
    int angle;

    cX = ArcCentre.x;
    cY = ArcCentre.y;

    cY = -cY;   /* Attention to the orientation of the axis Y. */

    /* Calculating cX and cY for the arc passes through ArcStartX, ArcEndX,
     * X and Y */
    dx    = ArcEndX - ArcStartX;
    dy    = ArcEndY - ArcStartY;
    cX   -= ArcStartX;
    cY   -= ArcStartY;
    angle = (int) ( atan2( (double) dy, (double) dx ) * 1800 / M_PI );
    RotatePoint( &dx, &dy, angle );     /* The segment dx, dy is horizontal
                                         * -> Length = dx, dy = 0 */
    RotatePoint( &cX, &cY, angle );
    cX = dx / 2;           /* cX, cY is on the median segment 0.0 a dx, 0 */

    RotatePoint( &cX, &cY, -angle );
    cX += ArcStartX;
    cY += ArcStartY;
    DrawItem->m_Pos.x = cX;
    DrawItem->m_Pos.y = cY;

    dx = ArcStartX - DrawItem->m_Pos.x;
    dy = ArcStartY - DrawItem->m_Pos.y;

    DrawItem->m_Radius = (int) sqrt( ( (double) dx * dx ) +
                                     ( (double) dy * dy ) );

    DrawItem->m_t1 = (int) ( atan2( (double) dy, (double) dx ) * 1800 / M_PI );

    dx = ArcEndX - DrawItem->m_Pos.x;
    dy = ArcEndY - DrawItem->m_Pos.y;

    DrawItem->m_t2 = (int) ( atan2( (double) dy, (double) dx ) * 1800 / M_PI );

    DrawItem->m_ArcStart.x = ArcStartX;
    DrawItem->m_ArcStart.y = ArcStartY;
    DrawItem->m_ArcEnd.x   = ArcEndX;
    DrawItem->m_ArcEnd.y   = ArcEndY;

    NORMALIZE_ANGLE( DrawItem->m_t1 );
    NORMALIZE_ANGLE( DrawItem->m_t2 );  // angles = 0 .. 3600

    // Restrict angle to less than 180 to avoid PBS display mirror
    // Trace because it is assumed that the arc is less than 180 deg to find
    // orientation after rotate or mirror.
    if( (DrawItem->m_t2 - DrawItem->m_t1) > 1800 )
        DrawItem->m_t2 -= 3600;
    else if( (DrawItem->m_t2 - DrawItem->m_t1) <= -1800 )
        DrawItem->m_t2 += 3600;

    wxString msg;
    angle = DrawItem->m_t2 - DrawItem->m_t1;
    msg.Printf( _( "Arc %.1f deg" ), (float) angle / 10 );
    WinEDA_SchematicFrame* frame =
        (WinEDA_SchematicFrame*) wxGetApp().GetTopWindow();
    frame->m_LibeditFrame->PrintMsg( msg );

    while( (DrawItem->m_t2 - DrawItem->m_t1) >= 1800 )
    {
        DrawItem->m_t2--;
        DrawItem->m_t1++;
    }

    while( (DrawItem->m_t1 - DrawItem->m_t2) >= 1800 )
    {
        DrawItem->m_t2++;
        DrawItem->m_t1--;
    }

    NORMALIZE_ANGLE( DrawItem->m_t1 );
    NORMALIZE_ANGLE( DrawItem->m_t2 );
}


/*
 * Used for deleting last entered segment while creating a Polyline
 */
void WinEDA_LibeditFrame::DeleteDrawPoly( wxDC* DC )
{
    if( m_drawItem == NULL
        || m_drawItem->Type() != COMPONENT_POLYLINE_DRAW_TYPE )
        return;

    LIB_POLYLINE* Poly = (LIB_POLYLINE*) m_drawItem;

    m_drawItem->Draw( DrawPanel, DC, wxPoint( 0, 0 ), -1, g_XorMode, NULL,
                      DefaultTransformMatrix );

    // First segment is kept, only its end point is changed
    while( Poly->GetCornerCount() > 2 )
    {
        Poly->m_PolyPoints.pop_back();
        unsigned idx   = Poly->GetCornerCount() - 1;
        wxPoint  point = GetScreen()->m_Curseur;
        NEGATE( point.y );
        if( Poly->m_PolyPoints[idx] != point )
        {
            Poly->m_PolyPoints[idx] = point;
            break;
        }
    }

    m_drawItem->Draw( DrawPanel, DC, wxPoint( 0, 0 ), -1, g_XorMode, NULL,
                      DefaultTransformMatrix );
}
