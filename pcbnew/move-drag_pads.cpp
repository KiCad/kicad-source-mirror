/************************/
/* Edit footprint pads. */
/************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "pcbnew.h"
#include "trigo.h"
#include "block_commande.h"

#include "drag.h"

#include "protos.h"


static D_PAD*  s_CurrentSelectedPad;
static wxPoint Pad_OldPos;


/* Cancel move pad command.
 */
static void Exit_Move_Pad( WinEDA_DrawPanel* Panel, wxDC* DC )
{
    D_PAD* pad = s_CurrentSelectedPad;

    Panel->ManageCurseur = NULL;
    Panel->ForceCloseManageCurseur = NULL;
    if( pad == NULL )
        return;

    pad->Draw( Panel, DC, GR_XOR );
    pad->m_Flags = 0;
    pad->m_Pos   = Pad_OldPos;
    pad->Draw( Panel, DC, GR_XOR );

    /* Pad move in progress: the restore origin. */
    if( g_Drag_Pistes_On )
    {
        for( unsigned ii = 0; ii < g_DragSegmentList.size(); ii++ )
        {
            TRACK* Track = g_DragSegmentList[ii].m_Segm;
            Track->Draw( Panel, DC, GR_XOR );
            Track->SetState( EDIT, OFF );
            g_DragSegmentList[ii].SetInitialValues();
            Track->Draw( Panel, DC, GR_OR );
        }
    }

    EraseDragList();
    s_CurrentSelectedPad = NULL;
    g_Drag_Pistes_On     = FALSE;
}


/* Draw in drag mode when moving a pad.
 */
static void Show_Pad_Move( WinEDA_DrawPanel* panel, wxDC* DC, bool erase )
{
    TRACK*       Track;
    BASE_SCREEN* screen = panel->GetScreen();
    D_PAD*       pad    = s_CurrentSelectedPad;

    if( erase )
        pad->Draw( panel, DC, GR_XOR );

    pad->m_Pos = screen->m_Curseur;
    pad->Draw( panel, DC, GR_XOR );

    if( !g_Drag_Pistes_On )
        return;

    for( unsigned ii = 0; ii < g_DragSegmentList.size(); ii++ )
    {
        Track = g_DragSegmentList[ii].m_Segm;
        if( erase )
            Track->Draw( panel, DC, GR_XOR );
        if( g_DragSegmentList[ii].m_Pad_Start )
        {
            Track->m_Start = pad->m_Pos;
        }
        if( g_DragSegmentList[ii].m_Pad_End )
        {
            Track->m_End = pad->m_Pos;
        }
        Track->Draw( panel, DC, GR_XOR );
    }
}


/* Load list of features for default pad selection.
 */
void WinEDA_BasePcbFrame::Export_Pad_Settings( D_PAD* pt_pad )
{
    MODULE* Module;

    if( pt_pad == NULL )
        return;

    Module = (MODULE*) pt_pad->GetParent();

    pt_pad->DisplayInfo( this );

    g_Pad_Master.m_PadShape     = pt_pad->m_PadShape;
    g_Pad_Master.m_Attribut     = pt_pad->m_Attribut;
    g_Pad_Master.m_Masque_Layer = pt_pad->m_Masque_Layer;
    g_Pad_Master.m_Orient = pt_pad->m_Orient -
                            ( (MODULE*) pt_pad->GetParent() )->m_Orient;
    g_Pad_Master.m_Size = pt_pad->m_Size;
    g_Pad_Master.m_DeltaSize = pt_pad->m_DeltaSize;
    pt_pad->ComputeRayon();

    g_Pad_Master.m_Offset     = pt_pad->m_Offset;
    g_Pad_Master.m_Drill      = pt_pad->m_Drill;
    g_Pad_Master.m_DrillShape = pt_pad->m_DrillShape;
}


/* Imports the new values of dimensions of the pad edge by pt_pad
 * - Source: selected values of general characteristics
 * - Measurements are modified
 * - The position, names, and keys are not.
 */
void WinEDA_BasePcbFrame::Import_Pad_Settings( D_PAD* aPad, bool aDraw )
{
    if( aDraw )
    {
        aPad->m_Flags |= DO_NOT_DRAW;
        DrawPanel->PostDirtyRect( aPad->GetBoundingBox() );
        aPad->m_Flags &= ~DO_NOT_DRAW;
    }

    aPad->m_PadShape     = g_Pad_Master.m_PadShape;
    aPad->m_Masque_Layer = g_Pad_Master.m_Masque_Layer;
    aPad->m_Attribut     = g_Pad_Master.m_Attribut;
    aPad->m_Orient = g_Pad_Master.m_Orient +
                     ( (MODULE*) aPad->GetParent() )->m_Orient;
    aPad->m_Size = g_Pad_Master.m_Size;
    aPad->m_DeltaSize  = wxSize( 0, 0 );
    aPad->m_Offset     = g_Pad_Master.m_Offset;
    aPad->m_Drill      = g_Pad_Master.m_Drill;
    aPad->m_DrillShape = g_Pad_Master.m_DrillShape;

    switch( g_Pad_Master.m_PadShape )
    {
    case PAD_TRAPEZOID:
        aPad->m_DeltaSize = g_Pad_Master.m_DeltaSize;
        break;

    case PAD_CIRCLE:
        aPad->m_Size.y = aPad->m_Size.x;
        break;
    }

    switch( g_Pad_Master.m_Attribut & 0x7F )
    {
    case PAD_SMD:
    case PAD_CONN:
        aPad->m_Drill    = wxSize( 0, 0 );
        aPad->m_Offset.x = 0;
        aPad->m_Offset.y = 0;
    }

    aPad->ComputeRayon();

    if( aDraw )
        DrawPanel->PostDirtyRect( aPad->GetBoundingBox() );

    ( (MODULE*) aPad->GetParent() )->m_LastEdit_Time = time( NULL );
}


/* Add a pad on the selected module.
 */
void WinEDA_BasePcbFrame::AddPad( MODULE* Module, bool draw )
{
    D_PAD* Pad;
    int    rX, rY;

    m_Pcb->m_Status_Pcb     = 0;
    Module->m_LastEdit_Time = time( NULL );

    Pad = new D_PAD( Module );

    /* Add the new pad to end of the module pad list. */
    Module->m_Pads.PushBack( Pad );

    /* Update the pad properties. */
    Import_Pad_Settings( Pad, false );
    Pad->SetNetname( wxEmptyString );

    Pad->m_Pos = GetScreen()->m_Curseur;

    rX = Pad->m_Pos.x - Module->m_Pos.x;
    rY = Pad->m_Pos.y - Module->m_Pos.y;

    RotatePoint( &rX, &rY, -Module->m_Orient );

    Pad->m_Pos0.x = rX;
    Pad->m_Pos0.y = rY;

    /* Automatically increment the current pad number and name. */
    long num    = 0;
    int  ponder = 1;

    while( g_Current_PadName.Len() && g_Current_PadName.Last() >= '0'
           && g_Current_PadName.Last() <= '9' )
    {
        num += ( g_Current_PadName.Last() - '0' ) * ponder;
        g_Current_PadName.RemoveLast();
        ponder *= 10;
    }

    num++;
    g_Current_PadName << num;
    Pad->SetPadName( g_Current_PadName );

    Module->Set_Rectangle_Encadrement();
    Pad->DisplayInfo( this );
    if( draw )
        DrawPanel->PostDirtyRect( Module->GetBoundingBox() );
}


/** Function DeletePad
 * Delete the pad aPad.
 * Refresh the modified screen area
 * Refresh modified parameters of the parent module (bounding box, last date)
 * @param aPad = the pad to delete
 * @param aQuery = true to promt for confirmation, false to delete silently
 */
void WinEDA_BasePcbFrame::DeletePad( D_PAD* aPad, bool aQuery )
{
    MODULE*  Module;
 
    if( aPad == NULL )
        return;

    Module = (MODULE*) aPad->GetParent();
    Module->m_LastEdit_Time = time( NULL );

    if( aQuery )
    {
        wxString msg;
        msg.Printf( _( "Delete Pad (module %s %s) " ),
                     GetChars( Module->m_Reference->m_Text ),
                     GetChars( Module->m_Value->m_Text ) );
        if( !IsOK( this, msg ) )
            return;
    }

    m_Pcb->m_Status_Pcb = 0;
    aPad->DeleteStructure();
    DrawPanel->PostDirtyRect( Module->GetBoundingBox() );
    Module->Set_Rectangle_Encadrement();

    OnModify();
}


/* Function to initialize the "move pad" command */
void WinEDA_BasePcbFrame::StartMovePad( D_PAD* Pad, wxDC* DC )
{
    MODULE* Module;

    if( Pad == NULL )
        return;

    Module = (MODULE*) Pad->GetParent();

    s_CurrentSelectedPad = Pad;
    Pad_OldPos = Pad->m_Pos;
    Pad->DisplayInfo( this );
    DrawPanel->ManageCurseur = Show_Pad_Move;
    DrawPanel->ForceCloseManageCurseur = Exit_Move_Pad;

    /* Draw the pad  (SKETCH mode) */
    Pad->Draw( DrawPanel, DC, GR_XOR );
    Pad->m_Flags |= IS_MOVED;
    Pad->Draw( DrawPanel, DC, GR_XOR );

    /* Build the list of track segments to drag if the command is a drag pad*/
    if( g_Drag_Pistes_On )
        Build_1_Pad_SegmentsToDrag( DrawPanel, DC, Pad );
    else
        EraseDragList();
}


/* Routine to place a moved pad. */
void WinEDA_BasePcbFrame::PlacePad( D_PAD* Pad, wxDC* DC )
{
    int     dX, dY;
    TRACK*  Track;
    MODULE* Module;

    if( Pad == NULL )
        return;

    Module = (MODULE*) Pad->GetParent();

    ITEM_PICKER       picker( NULL, UR_CHANGED );
    PICKED_ITEMS_LIST pickList;

    /* Save dragged track segments in undo list */
    for( unsigned ii = 0; ii < g_DragSegmentList.size(); ii++ )
    {
        Track = g_DragSegmentList[ii].m_Segm;

        // Set the old state
        wxPoint t_start = Track->m_Start;
        wxPoint t_end   = Track->m_End;
        if( g_DragSegmentList[ii].m_Pad_Start )
            Track->m_Start = Pad_OldPos;
        if( g_DragSegmentList[ii].m_Pad_End )
            Track->m_End = Pad_OldPos;

        picker.m_PickedItem = Track;
        pickList.PushItem( picker );
    }

    /* Save old module and old items values */
    wxPoint pad_curr_position = Pad->m_Pos;

    Pad->m_Pos = Pad_OldPos;
    if( g_DragSegmentList.size() == 0 )
        SaveCopyInUndoList( Module, UR_CHANGED );
    else
    {
        picker.m_PickedItem = Module;
        pickList.PushItem( picker );
        SaveCopyInUndoList( pickList, UR_CHANGED );
    }

    Pad->m_Pos = pad_curr_position;
    Pad->Draw( DrawPanel, DC, GR_XOR );

    /* Redraw dragged track segments */
    for( unsigned ii = 0; ii < g_DragSegmentList.size(); ii++ )
    {
        Track = g_DragSegmentList[ii].m_Segm;

        // Set the new state
        if( g_DragSegmentList[ii].m_Pad_Start )
            Track->m_Start = Pad->m_Pos;
        if( g_DragSegmentList[ii].m_Pad_End )
            Track->m_End = Pad->m_Pos;

        Track->SetState( EDIT, OFF );
        if( DC )
            Track->Draw( DrawPanel, DC, GR_OR );
    }

    /* Compute local coordinates (i.e refer to Module position and for Module
     * orient = 0) */
    dX = Pad->m_Pos.x - Pad_OldPos.x;
    dY = Pad->m_Pos.y - Pad_OldPos.y;
    RotatePoint( &dX, &dY, -Module->m_Orient );

    Pad->m_Pos0.x += dX;
    s_CurrentSelectedPad->m_Pos0.y += dY;

    Pad->m_Flags = 0;

    if( DC )
        Pad->Draw( DrawPanel, DC, GR_OR );

    Module->Set_Rectangle_Encadrement();
    Module->m_LastEdit_Time = time( NULL );

    EraseDragList();

    OnModify();
    DrawPanel->ManageCurseur = NULL;
    DrawPanel->ForceCloseManageCurseur = NULL;
    m_Pcb->m_Status_Pcb &= ~( LISTE_RATSNEST_ITEM_OK | CONNEXION_OK );
}


/* Rotate selected pad 90 degrees.
 */
void WinEDA_BasePcbFrame::RotatePad( D_PAD* Pad, wxDC* DC )
{
    MODULE* Module;

    if( Pad == NULL )
        return;

    Module = (MODULE*) Pad->GetParent();
    Module->m_LastEdit_Time = time( NULL );

    OnModify();

    if( DC )
        Module->Draw( DrawPanel, DC, GR_XOR );

    EXCHG( Pad->m_Size.x, Pad->m_Size.y );
    EXCHG( Pad->m_Drill.x, Pad->m_Drill.y );
    EXCHG( Pad->m_Offset.x, Pad->m_Offset.y );
    Pad->m_Offset.y = -Pad->m_Offset.y;

    EXCHG( Pad->m_DeltaSize.x, Pad->m_DeltaSize.y );
    Pad->m_DeltaSize.x = -Pad->m_DeltaSize.x;
    Module->Set_Rectangle_Encadrement();
    Pad->DisplayInfo( this );

    if( DC )
        Module->Draw( DrawPanel, DC, GR_OR );
}
