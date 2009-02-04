/*********************************************************************/
/*			EESchema - symbdraw.cpp									 */
/* Create, move .. graphic shapes used to build and draw a component */
/* (lines, arcs ..													 */
/*********************************************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "trigo.h"
#include "protos.h"
#include "id.h"

/* Routines locales */
static void SymbolDisplayDraw( WinEDA_DrawPanel* panel, wxDC* DC, bool erase );
static void ComputeArc( LibDrawArc* DrawItem, wxPoint ArcCentre );
static void RedrawWhileMovingCursor( WinEDA_DrawPanel* panel,
                                     wxDC*             DC,
                                     bool              erase );
static void MoveLibDrawItemAt( LibEDA_BaseStruct* DrawItem, wxPoint newpos );

/* Variables locales */
static int     StateDrawArc, ArcStartX, ArcStartY, ArcEndX, ArcEndY;
static wxPoint InitPosition, StartCursor, ItemPreviousPos;
static FILL_T  FlSymbol_Fill = NO_FILL;


/************************************/
/* class WinEDA_PartPropertiesFrame */
/************************************/
#include "dialog_cmp_graphic_properties.cpp"


/************************************************************/
void WinEDA_bodygraphics_PropertiesFrame::
bodygraphics_PropertiesAccept( wxCommandEvent& event )
/************************************************************/

/* Update the current draw item
 */
{
    g_FlDrawSpecificConvert = m_CommonConvert->GetValue() ? FALSE : TRUE;
    g_FlDrawSpecificUnit    = m_CommonUnit->GetValue() ? FALSE : TRUE;

    if( m_Filled )
        FlSymbol_Fill = (FILL_T) m_Filled->GetSelection();

    g_LibSymbolDefaultLineWidth = m_GraphicShapeWidthCtrl->GetValue();

    if( CurrentDrawItem )
    {
        if( !(CurrentDrawItem->m_Flags & IS_NEW) )  // if IS_NEW, copy for undo is done before place
            m_Parent->SaveCopyInUndoList( CurrentLibEntry );
        wxClientDC dc( m_Parent->DrawPanel );

        m_Parent->DrawPanel->PrepareGraphicContext( &dc );

        DrawLibraryDrawStruct( m_Parent->DrawPanel,
                               &dc,
                               CurrentLibEntry,
                               wxPoint( 0, 0 ),
                               CurrentDrawItem,
                               g_XorMode );

        if( g_FlDrawSpecificUnit )
            CurrentDrawItem->m_Unit = CurrentUnit;
        else
            CurrentDrawItem->m_Unit = 0;
        if( g_FlDrawSpecificConvert )
            CurrentDrawItem->m_Convert = CurrentConvert;
        else
            CurrentDrawItem->m_Convert = 0;
        if( m_Filled  )
        {
            switch( CurrentDrawItem->Type() )
            {
            case COMPONENT_ARC_DRAW_TYPE:
                ( (LibDrawArc*) CurrentDrawItem )->m_Fill  = FlSymbol_Fill;
                ( (LibDrawArc*) CurrentDrawItem )->m_Width =
                    m_GraphicShapeWidthCtrl->GetValue();
                break;

            case COMPONENT_CIRCLE_DRAW_TYPE:
                ( (LibDrawCircle*) CurrentDrawItem )->m_Fill  = FlSymbol_Fill;
                ( (LibDrawCircle*) CurrentDrawItem )->m_Width =
                    m_GraphicShapeWidthCtrl->GetValue();
                break;

            case COMPONENT_RECT_DRAW_TYPE:
                ( (LibDrawSquare*) CurrentDrawItem )->m_Fill  = FlSymbol_Fill;
                ( (LibDrawSquare*) CurrentDrawItem )->m_Width =
                    m_GraphicShapeWidthCtrl->GetValue();
                break;

            case  COMPONENT_POLYLINE_DRAW_TYPE:
                ( (LibDrawPolyline*) CurrentDrawItem )->m_Fill =
                    FlSymbol_Fill;
                ( (LibDrawPolyline*) CurrentDrawItem )->m_Width =
                    m_GraphicShapeWidthCtrl->
                    GetValue();
                break;

            default:
                break;
            }
        }
        CurrentLibEntry->SortDrawItems();

        m_Parent->GetScreen()->SetModify();

        DrawLibraryDrawStruct( m_Parent->DrawPanel,
                               &dc,
                               CurrentLibEntry,
                               wxPoint( 0, 0 ),
                               CurrentDrawItem,
                               g_XorMode );
    }

    Close();

    if( CurrentDrawItem )
        CurrentDrawItem->Display_Infos_DrawEntry( m_Parent );
    m_Parent->DrawPanel->Refresh();
}


/**********************************************************/
void WinEDA_LibeditFrame::EditGraphicSymbol( wxDC*              DC,
                                             LibEDA_BaseStruct* DrawItem )
/**********************************************************/

/* Install the dialog box for editing a graphical item properties
 */
{
    if( DrawItem == NULL )
        return;

    WinEDA_bodygraphics_PropertiesFrame* frame =
        new WinEDA_bodygraphics_PropertiesFrame( this );

    frame->ShowModal(); frame->Destroy();
}


/****************************************************************/
static void AbortSymbolTraceOn( WinEDA_DrawPanel* Panel, wxDC* DC )
/****************************************************************/
{
    StateDrawArc = 0;
    Panel->ManageCurseur = NULL;
    Panel->ForceCloseManageCurseur = NULL;
    if( CurrentDrawItem == NULL )
        return;

    if( CurrentDrawItem->m_Flags & IS_NEW )
    {
        if( CurrentDrawItem->Type() == COMPONENT_ARC_DRAW_TYPE )
        {
            Panel->m_Parent->RedrawActiveWindow( DC, TRUE );
        }
        else
            DrawLibraryDrawStruct( Panel, DC, CurrentLibEntry, wxPoint( 0, 0 ),
                                   CurrentDrawItem, g_XorMode );
        SAFE_DELETE( CurrentDrawItem );
    }
    else
    {
        wxPoint             curpos;
        curpos = Panel->GetScreen()->m_Curseur;
        Panel->GetScreen()->m_Curseur = StartCursor;
        RedrawWhileMovingCursor( Panel, DC, TRUE );
        Panel->GetScreen()->m_Curseur = curpos;
        DrawLibraryDrawStruct( Panel, DC, CurrentLibEntry, wxPoint( 0, 0 ),
                               CurrentDrawItem, GR_DEFAULT_DRAWMODE );

        CurrentDrawItem->m_Flags = 0;
    }
}


/*********************************************************************/
LibEDA_BaseStruct* WinEDA_LibeditFrame::CreateGraphicItem( wxDC* DC )
/*********************************************************************/

/* Routine de creation d'un nouvel element type LibraryDrawStruct
 *  POLYLINE
 *  ARC
 *  PAD_CIRCLE
 *  PAD_RECTANGLE
 */
{
    int  DrawType;

    DrawPanel->m_IgnoreMouseEvents = TRUE;

    // Creation du nouvel element
    switch( m_ID_current_state )
    {
    case ID_LIBEDIT_BODY_LINE_BUTT:
        DrawType = COMPONENT_POLYLINE_DRAW_TYPE;
        break;

    case ID_LIBEDIT_BODY_ARC_BUTT:
        DrawType = COMPONENT_ARC_DRAW_TYPE;
        break;

    case ID_LIBEDIT_BODY_CIRCLE_BUTT:
        DrawType = COMPONENT_CIRCLE_DRAW_TYPE;
        break;

    case ID_LIBEDIT_BODY_RECT_BUTT:
        DrawType = COMPONENT_RECT_DRAW_TYPE;
        break;

    case ID_LIBEDIT_BODY_TEXT_BUTT:
        DrawType = COMPONENT_GRAPHIC_TEXT_DRAW_TYPE;
        break;

    default:
        DisplayError( this, wxT( "SymbolBeginDrawItem Internal err: Id error" ) );
        return NULL;
    }

    DrawPanel->ManageCurseur = SymbolDisplayDraw;
    DrawPanel->ForceCloseManageCurseur = AbortSymbolTraceOn;

    switch( DrawType )
    {
    case COMPONENT_ARC_DRAW_TYPE:
    {
        LibDrawArc* Arc = new LibDrawArc();

        CurrentDrawItem = Arc;
        ArcStartX    = ArcEndX = GetScreen()->m_Curseur.x;
        ArcStartY    = ArcEndY = -( GetScreen()->m_Curseur.y );
        StateDrawArc = 1;
        Arc->m_Fill  = FlSymbol_Fill;
        Arc->m_Width = g_LibSymbolDefaultLineWidth;
    }
    break;

    case COMPONENT_CIRCLE_DRAW_TYPE:
    {
        LibDrawCircle* Circle = new LibDrawCircle();

        CurrentDrawItem = Circle;
        Circle->m_Pos.x = GetScreen()->m_Curseur.x;
        Circle->m_Pos.y = -( GetScreen()->m_Curseur.y );
        Circle->m_Fill  = FlSymbol_Fill;
        Circle->m_Width = g_LibSymbolDefaultLineWidth;
    }
    break;

    case COMPONENT_RECT_DRAW_TYPE:
    {
        LibDrawSquare* Square = new LibDrawSquare();

        CurrentDrawItem = Square;
        Square->m_Pos.x = GetScreen()->m_Curseur.x;
        Square->m_Pos.y = -( GetScreen()->m_Curseur.y );
        Square->m_End   = Square->m_Pos;
        Square->m_Fill  = FlSymbol_Fill;
        Square->m_Width = g_LibSymbolDefaultLineWidth;
    }
    break;

    case COMPONENT_POLYLINE_DRAW_TYPE:
    {
        LibDrawPolyline* polyline = new LibDrawPolyline();
        CurrentDrawItem = polyline;
        wxPoint point = GetScreen()->m_Curseur;
        NEGATE( point.y );
        polyline->AddPoint( point );    // Start point of the current segment
        polyline->AddPoint( point );    // End point of the current segment
        polyline->m_Fill  = FlSymbol_Fill;
        polyline->m_Width = g_LibSymbolDefaultLineWidth;
    }
    break;

    case COMPONENT_LINE_DRAW_TYPE:
    {
        LibDrawSegment* Segment = new LibDrawSegment();

        CurrentDrawItem  = Segment;
        Segment->m_Pos.x = GetScreen()->m_Curseur.x;
        Segment->m_Pos.y = -( GetScreen()->m_Curseur.y );
        Segment->m_End   = Segment->m_Pos;
        Segment->m_Width = g_LibSymbolDefaultLineWidth;
    }
    break;

    case COMPONENT_GRAPHIC_TEXT_DRAW_TYPE:
    {
        LibDrawText* Text = new LibDrawText();

        CurrentDrawItem = Text;
        Text->m_Size.x  = Text->m_Size.y = g_LastTextSize;
        Text->m_Orient  = g_LastTextOrient;
        Text->m_Pos.x   = GetScreen()->m_Curseur.x;
        Text->m_Pos.y   = -( GetScreen()->m_Curseur.y );
        EditSymbolText( NULL, Text );
        if( Text->m_Text.IsEmpty() )
        {
            SAFE_DELETE( Text );
            CurrentDrawItem = NULL;
            DrawPanel->ManageCurseur = NULL;
            DrawPanel->ForceCloseManageCurseur = NULL;
        }
        else
        {
            StartMoveDrawSymbol( DC );
            DrawLibraryDrawStruct( DrawPanel, DC, CurrentLibEntry,
                                   wxPoint( 0, 0 ),
                                   Text, g_XorMode );
        }
    }
    break;
    }

    if( CurrentDrawItem )
    {
        CurrentDrawItem->m_Flags |= IS_NEW;
        if( g_FlDrawSpecificUnit )
            CurrentDrawItem->m_Unit = CurrentUnit;
        if( g_FlDrawSpecificConvert )
            CurrentDrawItem->m_Convert = CurrentConvert;
    }

    DrawPanel->MouseToCursorSchema();
    DrawPanel->m_IgnoreMouseEvents = FALSE;

    return CurrentDrawItem;
}


/********************************************************/
void WinEDA_LibeditFrame::GraphicItemBeginDraw( wxDC* DC )
/********************************************************/

/* Routine de creation d'un nouvel element type LibraryDrawStruct
 */
{
    if( CurrentDrawItem == NULL )
        return;

    switch( CurrentDrawItem->Type() )
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
        NEGATE(pos.y);
        ( (LibDrawPolyline*) CurrentDrawItem )->AddPoint( pos );
    }
    break;

    case COMPONENT_LINE_DRAW_TYPE:
        break;

    default:
        ;
    }
}


/**************************************************************************/
static void RedrawWhileMovingCursor( WinEDA_DrawPanel* panel,
                                     wxDC*             DC,
                                     bool              erase )
/**************************************************************************/

/* Redraw the graphoc shape while moving
 */
{
    BASE_SCREEN* Screen = panel->GetScreen();
    wxPoint      pos;

    /* Erase shape in the old positon*/
    if( erase )
    {
        pos = ItemPreviousPos - StartCursor,
        DrawLibraryDrawStruct( panel, DC, CurrentLibEntry, pos,
                               CurrentDrawItem, g_XorMode );
    }

    /* Redraw moved shape */
    pos = Screen->m_Curseur - StartCursor,
    DrawLibraryDrawStruct( panel, DC, CurrentLibEntry, pos,
                           CurrentDrawItem, g_XorMode );
    ItemPreviousPos = Screen->m_Curseur;
}


/*****************************************************************/
void MoveLibDrawItemAt( LibEDA_BaseStruct* DrawItem, wxPoint newpos )
/*****************************************************************/
{
    NEGATE(newpos.y);
    wxPoint size;

    switch( DrawItem->Type() )
    {
    case COMPONENT_ARC_DRAW_TYPE:
    {
        wxPoint offset = newpos - ( (LibDrawArc*) CurrentDrawItem )->m_Pos;
        ( (LibDrawArc*) CurrentDrawItem )->m_Pos = newpos;
        ( (LibDrawArc*) CurrentDrawItem )->m_ArcStart += offset;
        ( (LibDrawArc*) CurrentDrawItem )->m_ArcEnd   += offset;
        break;
    }

    case COMPONENT_CIRCLE_DRAW_TYPE:
        ( (LibDrawCircle*) CurrentDrawItem )->m_Pos = newpos;
        break;

    case COMPONENT_RECT_DRAW_TYPE:
        size = ( (LibDrawSquare*) CurrentDrawItem )->m_End - ( (LibDrawSquare*) CurrentDrawItem )->m_Pos;
        ( (LibDrawSquare*) CurrentDrawItem )->m_Pos = newpos;
        ( (LibDrawSquare*) CurrentDrawItem )->m_End = newpos + size;
        break;

    case COMPONENT_POLYLINE_DRAW_TYPE:
    {
        int  ii, imax = ( (LibDrawPolyline*) CurrentDrawItem )->GetCornerCount();
        wxPoint offset = newpos - ( (LibDrawPolyline*) CurrentDrawItem )->m_PolyPoints[0];
        for( ii = 0; ii < imax; ii += 2 )
            ( (LibDrawPolyline*) CurrentDrawItem )->m_PolyPoints[ii] += offset;
    }
    break;

    case COMPONENT_LINE_DRAW_TYPE:
        break;

    case COMPONENT_GRAPHIC_TEXT_DRAW_TYPE:
        ( (LibDrawText*) CurrentDrawItem )->m_Pos = newpos;
        break;

    default:
        ;
    }
}


/************************************************************/
void WinEDA_LibeditFrame::StartMoveDrawSymbol( wxDC* DC )
/************************************************************/
{
    if( CurrentDrawItem == NULL )
        return;

    SetCursor( wxCURSOR_HAND );

    CurrentDrawItem->m_Flags |= IS_MOVED;
    StartCursor = GetScreen()->m_Curseur;

    switch( CurrentDrawItem->Type() )
    {
    case COMPONENT_ARC_DRAW_TYPE:
        InitPosition = ( (LibDrawArc*) CurrentDrawItem )->m_Pos;
        break;

    case COMPONENT_CIRCLE_DRAW_TYPE:
        InitPosition = ( (LibDrawCircle*) CurrentDrawItem )->m_Pos;
        break;

    case COMPONENT_RECT_DRAW_TYPE:
        InitPosition = ( (LibDrawSquare*) CurrentDrawItem )->m_Pos;
        break;

    case COMPONENT_POLYLINE_DRAW_TYPE:
        InitPosition = ( (LibDrawPolyline*) CurrentDrawItem )->m_PolyPoints[0];
        break;

    case COMPONENT_LINE_DRAW_TYPE:
        break;

    case COMPONENT_GRAPHIC_TEXT_DRAW_TYPE:
        InitPosition = ( (LibDrawText*) CurrentDrawItem )->m_Pos;
        break;

    default:
        ;
    }

    ItemPreviousPos = GetScreen()->m_Curseur;
    DrawPanel->ManageCurseur = RedrawWhileMovingCursor;
    DrawPanel->ForceCloseManageCurseur = AbortSymbolTraceOn;
    DrawPanel->ManageCurseur( DrawPanel, DC, TRUE );
}


/****************************************************************/
/* Routine de Gestion des evenements souris lors de la creation */
/* d'un nouvel element type LibraryDrawStruct					*/
/****************************************************************/
static void SymbolDisplayDraw( WinEDA_DrawPanel* panel, wxDC* DC, bool erase )
{
    int          DrawMode = g_XorMode;
    int          dx, dy;
    BASE_SCREEN* Screen = panel->GetScreen();
    wxPoint curr_pos = Screen->m_Curseur;

    NEGATE(curr_pos.y);

    GRSetDrawMode( DC, DrawMode );

    if( erase )
    {
        if( StateDrawArc == 1 )
        {
            int Color = ReturnLayerColor( LAYER_DEVICE );
            GRLine( &panel->m_ClipBox,
                    DC,
                    ArcStartX,
                    -ArcStartY,
                    ArcEndX,
                    -ArcEndY,
                    0,
                    Color );
        }
        else
        {
            DrawLibraryDrawStruct( panel, DC, CurrentLibEntry, wxPoint( 0, 0 ),
                                   CurrentDrawItem, DrawMode );
            if( CurrentDrawItem->Type() == COMPONENT_ARC_DRAW_TYPE )
            {
                int Color = ReturnLayerColor( LAYER_DEVICE );
                GRDashedLine( &panel->m_ClipBox, DC, ArcStartX, -ArcStartY,
                              ( (LibDrawArc*) CurrentDrawItem )->m_Pos.x,
                              -( (LibDrawArc*) CurrentDrawItem )->m_Pos.y,
                              0, Color );
                GRDashedLine( &panel->m_ClipBox, DC, ArcEndX, -ArcEndY,
                              ( (LibDrawArc*) CurrentDrawItem )->m_Pos.x,
                              -( (LibDrawArc*) CurrentDrawItem )->m_Pos.y,
                              0, Color );
            }
        }
    }

    switch( CurrentDrawItem->Type() )
    {
    case COMPONENT_ARC_DRAW_TYPE:
        if( StateDrawArc == 1 )
        {
            ArcEndX = curr_pos.x; ArcEndY = curr_pos.y;
        }

        if( StateDrawArc == 2 )
        {
            ComputeArc( (LibDrawArc*) CurrentDrawItem, Screen->m_Curseur );
        }
        ( (LibDrawArc*) CurrentDrawItem )->m_Fill = FlSymbol_Fill;
        break;

    case COMPONENT_CIRCLE_DRAW_TYPE:
        dx = ( (LibDrawCircle*) CurrentDrawItem )->m_Pos.x - curr_pos.x;
        dy = ( (LibDrawCircle*) CurrentDrawItem )->m_Pos.y - curr_pos.y;
        ( (LibDrawCircle*) CurrentDrawItem )->m_Rayon =
            (int) sqrt( ( (double) dx * dx ) + ( (double) dy * dy ) );
        ( (LibDrawCircle*) CurrentDrawItem )->m_Fill = FlSymbol_Fill;
        break;

    case COMPONENT_RECT_DRAW_TYPE:
        ( (LibDrawSquare*) CurrentDrawItem )->m_End = curr_pos;
        ( (LibDrawSquare*) CurrentDrawItem )->m_Fill  = FlSymbol_Fill;
        break;

    case COMPONENT_POLYLINE_DRAW_TYPE:
    {
        unsigned idx = ( (LibDrawPolyline*) CurrentDrawItem )->GetCornerCount() - 1;
        ( (LibDrawPolyline*) CurrentDrawItem )->m_PolyPoints[idx] = curr_pos;
        ( (LibDrawPolyline*) CurrentDrawItem )->m_Fill = FlSymbol_Fill;
    }
        break;

    case COMPONENT_LINE_DRAW_TYPE:
        ( (LibDrawSegment*) CurrentDrawItem )->m_End = curr_pos;
        break;

    case COMPONENT_GRAPHIC_TEXT_DRAW_TYPE:      /* Traite par des routines specifiques */
        break;

    default:
        ;
    }

    if( StateDrawArc == 1 )
    {
        int Color = ReturnLayerColor( LAYER_DEVICE );
        GRLine( &panel->m_ClipBox,
                DC,
                ArcStartX,
                -ArcStartY,
                ArcEndX,
                -ArcEndY,
                0,
                Color );
    }
    else
    {
        DrawLibraryDrawStruct( panel, DC, CurrentLibEntry, wxPoint( 0, 0 ),
                               CurrentDrawItem, DrawMode );
        if( CurrentDrawItem->Type() == COMPONENT_ARC_DRAW_TYPE )
        {
            int Color = ReturnLayerColor( LAYER_DEVICE );
            GRDashedLine( &panel->m_ClipBox, DC, ArcStartX, -ArcStartY,
                          ( (LibDrawArc*) CurrentDrawItem )->m_Pos.x,
                          -( (LibDrawArc*) CurrentDrawItem )->m_Pos.y,
                          0, Color );
            GRDashedLine( &panel->m_ClipBox, DC, ArcEndX, -ArcEndY,
                          ( (LibDrawArc*) CurrentDrawItem )->m_Pos.x,
                          -( (LibDrawArc*) CurrentDrawItem )->m_Pos.y,
                          0, Color );
        }
    }
}


/******************************************************/
void WinEDA_LibeditFrame::EndDrawGraphicItem( wxDC* DC )
/******************************************************/

/* Place la structure courante en liste des structures du composant
 *  courant, si elle existe et redessine toujours celle ci
 *  Parametres: (tous globaux)
 *      CurrentDrawItem
 *      CurrentLibEntry
 */
{
    if( CurrentLibEntry == NULL )
        return;
    if( CurrentDrawItem == NULL )
        return;

    if( CurrentDrawItem->Type() == COMPONENT_ARC_DRAW_TYPE )
    {
        if( StateDrawArc == 1 ) /* Trace d'arc en cours: doit etre termine */
        {
            DisplayError( this, wxT( "Arc in progress.." ), 10 ); return;
        }
        else
        {
            if( (CurrentDrawItem->m_Flags & IS_MOVED) == 0 )
                SymbolDisplayDraw( DrawPanel, DC, FALSE );
        }
    }

    StateDrawArc = 0;

    if( CurrentDrawItem->m_Flags & IS_NEW )
    {
        SaveCopyInUndoList( CurrentLibEntry );
        CurrentDrawItem->SetNext( CurrentLibEntry->m_Drawings );
        CurrentLibEntry->m_Drawings = CurrentDrawItem;

        switch( CurrentDrawItem->Type() )
        {
        case COMPONENT_ARC_DRAW_TYPE:
            ( (LibDrawArc*) CurrentDrawItem )->m_Fill = FlSymbol_Fill;
            break;

        case COMPONENT_CIRCLE_DRAW_TYPE:
            ( (LibDrawCircle*) CurrentDrawItem )->m_Fill = FlSymbol_Fill;
            break;

        case COMPONENT_RECT_DRAW_TYPE:
            ( (LibDrawSquare*) CurrentDrawItem )->m_Fill = FlSymbol_Fill;
            break;

        case COMPONENT_POLYLINE_DRAW_TYPE:
            ( (LibDrawPolyline*) CurrentDrawItem )->m_Fill = FlSymbol_Fill;
            break;

        case COMPONENT_PIN_DRAW_TYPE:
        case COMPONENT_LINE_DRAW_TYPE:
        case COMPONENT_GRAPHIC_TEXT_DRAW_TYPE:
            break;

        default:
            ;
        }

        CurrentLibEntry->SortDrawItems();
    }

    if( m_ID_current_state )
        SetCursor( wxCURSOR_PENCIL );
    else
        SetCursor( wxCURSOR_ARROW );

    if( (CurrentDrawItem->m_Flags & IS_MOVED) )
    {
        wxPoint pos;
        pos.x = GetScreen()->m_Curseur.x + InitPosition.x - StartCursor.x,
        pos.y = GetScreen()->m_Curseur.y - InitPosition.y - StartCursor.y;
        MoveLibDrawItemAt( CurrentDrawItem, pos );
    }

    DrawLibEntry( DrawPanel, DC, CurrentLibEntry, wxPoint( 0, 0 ), CurrentUnit,
                  CurrentConvert, GR_DEFAULT_DRAWMODE );

    CurrentDrawItem->m_Flags = 0;
    CurrentDrawItem = NULL;

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

    cX = ArcCentre.x; cY = ArcCentre.y;

    cY = -cY;   /* Attention a l'orientation de l'axe Y */

    /* calcul de cX et cY pour que l'arc passe par ArcStartX,Y et ArcEndX,Y */
    dx    = ArcEndX - ArcStartX; dy = ArcEndY - ArcStartY;
    cX   -= ArcStartX; cY -= ArcStartY;
    angle = (int) ( atan2( (double) dy, (double) dx ) * 1800 / M_PI );
    RotatePoint( &dx, &dy, angle );     /* Le segment dx, dy est horizontal */
                                        /* -> dx = longueur, dy = 0 */
    RotatePoint( &cX, &cY, angle );
    cX = dx / 2;                        /* cX, cY est sur la mediane du segment 0,0 a dx,0 */

    RotatePoint( &cX, &cY, -angle );
    cX += ArcStartX; cY += ArcStartY;

    DrawItem->m_Pos.x = cX; DrawItem->m_Pos.y = cY;

    dx = ArcStartX - DrawItem->m_Pos.x;
    dy = ArcStartY - DrawItem->m_Pos.y;

    DrawItem->m_Rayon = (int) sqrt( ( (double) dx * dx ) + ( (double) dy * dy ) );

    DrawItem->t1 = (int) ( atan2( (double) dy, (double) dx ) * 1800 / M_PI );

    dx = ArcEndX - DrawItem->m_Pos.x;
    dy = ArcEndY - DrawItem->m_Pos.y;

    DrawItem->t2 = (int) ( atan2( (double) dy, (double) dx ) * 1800 / M_PI );

    DrawItem->m_ArcStart.x = ArcStartX;
    DrawItem->m_ArcStart.y = ArcStartY;
    DrawItem->m_ArcEnd.x   = ArcEndX;
    DrawItem->m_ArcEnd.y   = ArcEndY;

    NORMALIZE_ANGLE( DrawItem->t1 );
    NORMALIZE_ANGLE( DrawItem->t2 );  // angles = 0 .. 3600

    // limitation val abs a < 1800 (1/2 cercle) pour eviter Pbs d'affichage en miroir
    // car en trace on suppose que l'arc fait moins de 180 deg pour trouver
    // son orientation apres rot, miroir...
    if( (DrawItem->t2 - DrawItem->t1) > 1800 )
        DrawItem->t2 -= 3600;
    else if( (DrawItem->t2 - DrawItem->t1) <= -1800 )
        DrawItem->t2 += 3600;

    wxString msg;
    angle = DrawItem->t2 - DrawItem->t1;
    msg.Printf( _( "Arc %.1f deg" ), (float) angle / 10 );
    WinEDA_SchematicFrame* frame =
        (WinEDA_SchematicFrame*) wxGetApp().GetTopWindow();
    frame->m_LibeditFrame->PrintMsg( msg );

    while( (DrawItem->t2 - DrawItem->t1) >= 1800 )
    {
        DrawItem->t2--;
        DrawItem->t1++;
    }

    while( (DrawItem->t1 - DrawItem->t2) >= 1800 )
    {
        DrawItem->t2++;
        DrawItem->t1--;
    }

    NORMALIZE_ANGLE( DrawItem->t1 );
    NORMALIZE_ANGLE( DrawItem->t2 );
}


/***************************************************/
void WinEDA_LibeditFrame::DeleteDrawPoly( wxDC* DC )
/**************************************************/

/* Used for deleting last entered segment while creating a Polyline
 */
{
    if( CurrentDrawItem == NULL )
        return;
    if( CurrentDrawItem->Type() != COMPONENT_POLYLINE_DRAW_TYPE )
        return;

    LibDrawPolyline* Poly = (LibDrawPolyline*) CurrentDrawItem;

    DrawLibraryDrawStruct( DrawPanel, DC, CurrentLibEntry, wxPoint( 0, 0 ),
                           CurrentDrawItem, g_XorMode );

    while ( Poly->GetCornerCount() > 2 )    // First segment is kept, only its end point is changed
    {
        Poly->m_PolyPoints.pop_back();
        unsigned idx =  Poly->GetCornerCount() - 1;
        wxPoint point = GetScreen()->m_Curseur;
        NEGATE( point.y );
        if( Poly->m_PolyPoints[idx] != point )
        {
            Poly->m_PolyPoints[idx] = point;
            break;
        }
    }

    DrawLibraryDrawStruct( DrawPanel, DC, CurrentLibEntry, wxPoint( 0, 0 ),
                           CurrentDrawItem, g_XorMode );
}
