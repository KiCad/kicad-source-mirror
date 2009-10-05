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


/* Routines locales */
static void SymbolDisplayDraw( WinEDA_DrawPanel* panel, wxDC* DC, bool erase );
static void ComputeArc( LibDrawArc* DrawItem, wxPoint ArcCentre );
static void RedrawWhileMovingCursor( WinEDA_DrawPanel* panel,
                                     wxDC*             DC,
                                     bool              erase );

/* Variables locales */
static int     StateDrawArc, ArcStartX, ArcStartY, ArcEndX, ArcEndY;
static wxPoint InitPosition, StartCursor, ItemPreviousPos;
static FILL_T  FlSymbol_Fill = NO_FILL;


/************************************/
/* class WinEDA_PartPropertiesFrame */
/************************************/
#include "dialog_cmp_graphic_properties.cpp"


/************************************************************/
void WinEDA_bodygraphics_PropertiesFrame::bodygraphics_PropertiesAccept( wxCommandEvent& event )
/************************************************************/

/* Update the current draw item
 */
{
    LIB_DRAW_ITEM* item = m_Parent->GetDrawItem();

    if( item == NULL )
        return;

    g_FlDrawSpecificConvert = m_CommonConvert->GetValue() ? FALSE : TRUE;
    g_FlDrawSpecificUnit    = m_CommonUnit->GetValue() ? FALSE : TRUE;

    if( m_Filled )
        FlSymbol_Fill = (FILL_T) m_Filled->GetSelection();

    g_LibSymbolDefaultLineWidth = m_GraphicShapeWidthCtrl->GetValue();

    if( !(item->m_Flags & IS_NEW) )  // if IS_NEW, copy for undo is done before place
        m_Parent->SaveCopyInUndoList( item->GetParent() );
    wxClientDC dc( m_Parent->DrawPanel );

    m_Parent->DrawPanel->PrepareGraphicContext( &dc );

    item->Draw( m_Parent->DrawPanel, &dc, wxPoint( 0, 0 ), -1, g_XorMode,
                NULL, DefaultTransformMatrix );

    if( g_FlDrawSpecificUnit )
        item->m_Unit = m_Parent->GetUnit();
    else
        item->m_Unit = 0;

    if( g_FlDrawSpecificConvert )
        item->m_Convert = m_Parent->GetConvert();
    else
        item->m_Convert = 0;

    if( m_Filled  )
    {
        switch( item->Type() )
        {
        case COMPONENT_ARC_DRAW_TYPE:
            ( (LibDrawArc*) item )->m_Fill  = FlSymbol_Fill;
            ( (LibDrawArc*) item )->m_Width =
                m_GraphicShapeWidthCtrl->GetValue();
            break;

        case COMPONENT_CIRCLE_DRAW_TYPE:
            ( (LibDrawCircle*) item )->m_Fill  = FlSymbol_Fill;
            ( (LibDrawCircle*) item )->m_Width =
                m_GraphicShapeWidthCtrl->GetValue();
            break;

        case COMPONENT_RECT_DRAW_TYPE:
            ( (LibDrawSquare*) item )->m_Fill  = FlSymbol_Fill;
            ( (LibDrawSquare*) item )->m_Width =
                m_GraphicShapeWidthCtrl->GetValue();
            break;

        case COMPONENT_POLYLINE_DRAW_TYPE:
            ( (LibDrawPolyline*) item )->m_Fill =
                FlSymbol_Fill;
            ( (LibDrawPolyline*) item )->m_Width =
                m_GraphicShapeWidthCtrl->GetValue();
            break;

        default:
            break;
        }

        item->GetParent()->GetDrawItemList().sort();

        m_Parent->GetScreen()->SetModify();

        item->Draw( m_Parent->DrawPanel, &dc, wxPoint( 0, 0 ), -1, g_XorMode,
                    NULL, DefaultTransformMatrix );
    }

    Close();

    item->DisplayInfo( m_Parent );
    m_Parent->DrawPanel->Refresh();
}


/*
 * Show the dialog box for editing a graphical item properties
 */
void WinEDA_LibeditFrame::EditGraphicSymbol( wxDC* DC, LIB_DRAW_ITEM* DrawItem )
{
    if( DrawItem == NULL )
        return;

    WinEDA_bodygraphics_PropertiesFrame dlg( this );

    dlg.ShowModal();
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
            Panel->m_Parent->RedrawActiveWindow( DC, TRUE );
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
        LibDrawArc* Arc = new LibDrawArc( LibEntry );

        m_drawItem = Arc;
        ArcStartX    = ArcEndX = GetScreen()->m_Curseur.x;
        ArcStartY    = ArcEndY = -( GetScreen()->m_Curseur.y );
        StateDrawArc = 1;
        Arc->m_Fill  = FlSymbol_Fill;
        Arc->m_Width = g_LibSymbolDefaultLineWidth;
    }
    break;

    case ID_LIBEDIT_BODY_CIRCLE_BUTT:
    {
        LibDrawCircle* Circle = new LibDrawCircle( LibEntry );

        m_drawItem = Circle;
        Circle->m_Pos   = GetScreen()->m_Curseur;
        NEGATE( Circle->m_Pos.y );
        Circle->m_Fill  = FlSymbol_Fill;
        Circle->m_Width = g_LibSymbolDefaultLineWidth;
    }
    break;

    case ID_LIBEDIT_BODY_RECT_BUTT:
    {
        LibDrawSquare* Square = new LibDrawSquare( LibEntry );

        m_drawItem = Square;
        Square->m_Pos   = GetScreen()->m_Curseur;
        NEGATE( Square->m_Pos.y );
        Square->m_End   = Square->m_Pos;
        Square->m_Fill  = FlSymbol_Fill;
        Square->m_Width = g_LibSymbolDefaultLineWidth;
    }
    break;

    case ID_LIBEDIT_BODY_LINE_BUTT:
    {
        LibDrawPolyline* polyline = new LibDrawPolyline( LibEntry );
        m_drawItem = polyline;
        wxPoint          point = GetScreen()->m_Curseur;
        NEGATE( point.y );
        polyline->AddPoint( point );    // Start point of the current segment
        polyline->AddPoint( point );    // End point of the current segment
        polyline->m_Fill  = FlSymbol_Fill;
        polyline->m_Width = g_LibSymbolDefaultLineWidth;
    }
    break;

    case COMPONENT_LINE_DRAW_TYPE:
    {
        LibDrawSegment* Segment = new LibDrawSegment( LibEntry );

        m_drawItem = Segment;
        Segment->m_Pos  = GetScreen()->m_Curseur;
        NEGATE( Segment->m_Pos.y );
        Segment->m_End   = Segment->m_Pos;
        Segment->m_Width = g_LibSymbolDefaultLineWidth;
    }
    break;

    case ID_LIBEDIT_BODY_TEXT_BUTT:
    {
        LibDrawText* Text = new LibDrawText( LibEntry );

        m_drawItem = Text;
        Text->m_Size.x  = Text->m_Size.y = g_LastTextSize;
        Text->m_Orient  = g_LastTextOrient;
        Text->m_Pos   = GetScreen()->m_Curseur;
        NEGATE( Text->m_Pos.y );
        EditSymbolText( NULL, Text );
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
        if( g_FlDrawSpecificUnit )
            m_drawItem->m_Unit = m_unit;
        if( g_FlDrawSpecificConvert )
            m_drawItem->m_Convert = m_convert;
    }

    DrawPanel->MouseToCursorSchema();
    DrawPanel->m_IgnoreMouseEvents = FALSE;

    return m_drawItem;
}


/*
 * Routine de creation d'un nouvel element type LibraryDrawStruct
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
        ( (LibDrawPolyline*) m_drawItem )->AddPoint( pos );
    }
    break;

    case COMPONENT_LINE_DRAW_TYPE:
        break;

    default:
        ;
    }
}


/*
 * Redraw the graphoc shape while moving
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

    /* Erase shape in the old positon*/
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


/****************************************************************/
/* Routine de Gestion des evenements souris lors de la creation */
/* d'un nouvel element type LibraryDrawStruct                   */
/****************************************************************/
static void SymbolDisplayDraw( WinEDA_DrawPanel* panel, wxDC* DC, bool erase )
{
    int          DrawMode = g_XorMode;
    int          dx, dy;
    BASE_SCREEN* Screen   = panel->GetScreen();
    wxPoint      curr_pos = Screen->m_Curseur;

    LIB_DRAW_ITEM* item;

    item = ( ( WinEDA_LibeditFrame* ) panel->GetParent() )->GetDrawItem();

    if( item == NULL )
        return;

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
                              ( (LibDrawArc*) item )->m_Pos.x,
                              -( (LibDrawArc*) item )->m_Pos.y,
                              0, Color );
                GRDashedLine( &panel->m_ClipBox, DC, ArcEndX, -ArcEndY,
                              ( (LibDrawArc*) item )->m_Pos.x,
                              -( (LibDrawArc*) item )->m_Pos.y,
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
            ComputeArc( (LibDrawArc*) item, Screen->m_Curseur );
        }
        ( (LibDrawArc*) item )->m_Fill = FlSymbol_Fill;
        break;

    case COMPONENT_CIRCLE_DRAW_TYPE:
        dx = ( (LibDrawCircle*) item )->m_Pos.x - curr_pos.x;
        dy = ( (LibDrawCircle*) item )->m_Pos.y - curr_pos.y;
        ( (LibDrawCircle*) item )->m_Radius =
            (int) sqrt( ( (double) dx * dx ) + ( (double) dy * dy ) );
        ( (LibDrawCircle*) item )->m_Fill = FlSymbol_Fill;
        break;

    case COMPONENT_RECT_DRAW_TYPE:
        ( (LibDrawSquare*) item )->m_End  = curr_pos;
        ( (LibDrawSquare*) item )->m_Fill = FlSymbol_Fill;
        break;

    case COMPONENT_POLYLINE_DRAW_TYPE:
    {
        unsigned idx = ( (LibDrawPolyline*) item )->GetCornerCount() - 1;
        ( (LibDrawPolyline*) item )->m_PolyPoints[idx] = curr_pos;
        ( (LibDrawPolyline*) item )->m_Fill = FlSymbol_Fill;
    }
    break;

    case COMPONENT_LINE_DRAW_TYPE:
        ( (LibDrawSegment*) item )->m_End = curr_pos;
        break;

    case COMPONENT_GRAPHIC_TEXT_DRAW_TYPE:      /* Traite par des routines specifiques */
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
                          ( (LibDrawArc*) item )->m_Pos.x,
                          -( (LibDrawArc*) item )->m_Pos.y,
                          0, Color );
            GRDashedLine( &panel->m_ClipBox, DC, ArcEndX, -ArcEndY,
                          ( (LibDrawArc*) item )->m_Pos.x,
                          -( (LibDrawArc*) item )->m_Pos.y,
                          0, Color );
        }
    }
}


/*
 * Place la structure courante en liste des structures du composant
 * courant, si elle existe et redessine toujours celle ci
 *  Parametres: (tous globaux)
 *      m_drawItem
 *      m_component
 */
void WinEDA_LibeditFrame::EndDrawGraphicItem( wxDC* DC )
{
    if( m_component == NULL || m_drawItem == NULL )
        return;

    if( m_drawItem->Type() == COMPONENT_ARC_DRAW_TYPE )
    {
        if( StateDrawArc == 1 ) /* Trace d'arc en cours: doit etre termine */
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
            ( (LibDrawArc*) m_drawItem )->m_Fill = FlSymbol_Fill;
            break;

        case COMPONENT_CIRCLE_DRAW_TYPE:
            ( (LibDrawCircle*) m_drawItem )->m_Fill = FlSymbol_Fill;
            break;

        case COMPONENT_RECT_DRAW_TYPE:
            ( (LibDrawSquare*) m_drawItem )->m_Fill = FlSymbol_Fill;
            break;

        case COMPONENT_POLYLINE_DRAW_TYPE:
            ( (LibDrawPolyline*) m_drawItem )->m_Fill = FlSymbol_Fill;
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


/***************************************************************/
static void ComputeArc( LibDrawArc* DrawItem, wxPoint ArcCentre )
/***************************************************************/

/* routine d'ajustage des parametres de l'arc en cours de trace
 *  calcule le centre, rayon, angles pour que l'arc en cours
 *  passe par les points ArcStartX,Y et ArcEndX,Y avec le centre le plus proche
 *  de la pos souris
 *  Remarque: le centre n'est evidemment pas sur la grille
 */
{
    int dx, dy;
    int cX, cY;     /* Coord centre de l'arc */
    int angle;

    cX = ArcCentre.x;
    cY = ArcCentre.y;

    cY = -cY;   /* Attention a l'orientation de l'axe Y */

    /* calcul de cX et cY pour que l'arc passe par ArcStartX,Y et ArcEndX,Y */
    dx    = ArcEndX - ArcStartX;
    dy    = ArcEndY - ArcStartY;
    cX   -= ArcStartX;
    cY   -= ArcStartY;
    angle = (int) ( atan2( (double) dy, (double) dx ) * 1800 / M_PI );
    RotatePoint( &dx, &dy, angle );     /* Le segment dx, dy est horizontal */
                                        /* -> dx = longueur, dy = 0 */
    RotatePoint( &cX, &cY, angle );
    cX = dx / 2;                        /* cX, cY est sur la mediane du segment 0,0 a dx,0 */

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

    // limitation val abs a < 1800 (1/2 cercle) pour eviter Pbs d'affichage en miroir
    // car en trace on suppose que l'arc fait moins de 180 deg pour trouver
    // son orientation apres rot, miroir...
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

    LibDrawPolyline* Poly = (LibDrawPolyline*) m_drawItem;

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
