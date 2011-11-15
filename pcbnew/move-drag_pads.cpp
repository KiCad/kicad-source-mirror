/**
 * @file move-drag_pads.cpp
 * @brief Edit footprint pads.
 */

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "trigo.h"
#include "block_commande.h"
#include "wxBasePcbFrame.h"
#include "macros.h"
#include "pcbcommon.h"

#include "class_board.h"
#include "class_module.h"

#include "pcbnew.h"
#include "drag.h"
#include "protos.h"


static D_PAD*  s_CurrentSelectedPad;
static wxPoint Pad_OldPos;


/* Cancel move pad command.
 */
static void Abort_Move_Pad( EDA_DRAW_PANEL* Panel, wxDC* DC )
{
    D_PAD* pad = s_CurrentSelectedPad;

    Panel->SetMouseCapture( NULL, NULL );

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
            Track->SetState( IN_EDIT, OFF );
            g_DragSegmentList[ii].SetInitialValues();
            Track->Draw( Panel, DC, GR_OR );
        }
    }

    EraseDragList();
    s_CurrentSelectedPad = NULL;
    g_Drag_Pistes_On     = false;
}


/* Draw in drag mode when moving a pad.
 */
static void Show_Pad_Move( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                           bool aErase )
{
    TRACK*       Track;
    BASE_SCREEN* screen = aPanel->GetScreen();
    D_PAD*       pad    = s_CurrentSelectedPad;

    if( pad == NULL )       // Should not occur
        return;

    if( aErase )
        pad->Draw( aPanel, aDC, GR_XOR );

    pad->m_Pos = screen->GetCrossHairPosition();
    pad->Draw( aPanel, aDC, GR_XOR );

    if( !g_Drag_Pistes_On )
        return;

    for( unsigned ii = 0; ii < g_DragSegmentList.size(); ii++ )
    {
        Track = g_DragSegmentList[ii].m_Segm;

        if( aErase )
            Track->Draw( aPanel, aDC, GR_XOR );

        if( g_DragSegmentList[ii].m_Pad_Start )
        {
            Track->m_Start = pad->m_Pos;
        }

        if( g_DragSegmentList[ii].m_Pad_End )
        {
            Track->m_End = pad->m_Pos;
        }

        Track->Draw( aPanel, aDC, GR_XOR );
    }
}


/* Load list of features for default pad selection.
 */
void PCB_BASE_FRAME::Export_Pad_Settings( D_PAD* pt_pad )
{
    if( pt_pad == NULL )
        return;

    pt_pad->DisplayInfo( this );

    g_Pad_Master.m_PadShape     = pt_pad->m_PadShape;
    g_Pad_Master.m_Attribut     = pt_pad->m_Attribut;
    g_Pad_Master.m_layerMask = pt_pad->m_layerMask;
    g_Pad_Master.m_Orient = pt_pad->m_Orient -
                            ( (MODULE*) pt_pad->GetParent() )->m_Orient;
    g_Pad_Master.m_Size = pt_pad->m_Size;
    g_Pad_Master.m_DeltaSize = pt_pad->m_DeltaSize;
    pt_pad->ComputeShapeMaxRadius();

    g_Pad_Master.m_Offset     = pt_pad->m_Offset;
    g_Pad_Master.m_Drill      = pt_pad->m_Drill;
    g_Pad_Master.m_DrillShape = pt_pad->m_DrillShape;
}


/* Imports the new values of dimensions of the pad edge by pt_pad
 * - Source: selected values of general characteristics
 * - Measurements are modified
 * - The position, names, and keys are not.
 */
void PCB_BASE_FRAME::Import_Pad_Settings( D_PAD* aPad, bool aDraw )
{
    if( aDraw )
    {
        aPad->m_Flags |= DO_NOT_DRAW;
        DrawPanel->RefreshDrawingRect( aPad->GetBoundingBox() );
        aPad->m_Flags &= ~DO_NOT_DRAW;
    }

    aPad->m_PadShape     = g_Pad_Master.m_PadShape;
    aPad->m_layerMask = g_Pad_Master.m_layerMask;
    aPad->m_Attribut     = g_Pad_Master.m_Attribut;
    aPad->m_Orient = g_Pad_Master.m_Orient +
                     ( (MODULE*) aPad->GetParent() )->m_Orient;
    aPad->m_Size = g_Pad_Master.m_Size;
    aPad->m_DeltaSize  = VECTOR_PCB::fromXY( ZERO_LENGTH, ZERO_LENGTH );//wxSize( 0, 0 );
    aPad->m_Offset     = g_Pad_Master.m_Offset;
    aPad->m_Drill      = g_Pad_Master.m_Drill;
    aPad->m_DrillShape = g_Pad_Master.m_DrillShape;

    switch( g_Pad_Master.m_PadShape )
    {
    case PAD_TRAPEZOID:
        aPad->m_DeltaSize = g_Pad_Master.m_DeltaSize;
        break;

    case PAD_CIRCLE:
        aPad->m_Size.y() = aPad->m_Size.x();
        break;
    }

    switch( g_Pad_Master.m_Attribut & 0x7F )
    {
    case PAD_SMD:
    case PAD_CONN:
        aPad->m_Drill    = VECTOR_PCB::fromXY( ZERO_LENGTH, ZERO_LENGTH ); //wxSize( 0, 0 );
        aPad->m_Offset   = VECTOR_PCB::fromXY( ZERO_LENGTH, ZERO_LENGTH );
    }

    aPad->ComputeShapeMaxRadius();

    if( aDraw )
        DrawPanel->RefreshDrawingRect( aPad->GetBoundingBox() );

    ( (MODULE*) aPad->GetParent() )->m_LastEdit_Time = time( NULL );
}


/* Add a pad on the selected module.
 */
void PCB_BASE_FRAME::AddPad( MODULE* Module, bool draw )
{
    wxString lastPadName;   // Last used pad name (pad num)
    lastPadName = g_Pad_Master.ReturnStringPadName();

    m_Pcb->m_Status_Pcb     = 0;
    Module->m_LastEdit_Time = time( NULL );

    D_PAD* Pad = new D_PAD( Module );

    /* Add the new pad to end of the module pad list. */
    Module->m_Pads.PushBack( Pad );

    /* Update the pad properties. */
    Import_Pad_Settings( Pad, false );
    Pad->SetNetname( wxEmptyString );

    Pad->m_Pos = GetScreen()->GetCrossHairPosition();

    // Set the relative pad position
    // ( pad position for module orient, 0, and relative to the module position)
    Pad->m_Pos0 = FROM_LEGACY_LU_VEC( Pad->m_Pos - Module->m_Pos );
    wxPoint p = TO_LEGACY_LU_WXP( Pad->m_Pos0 );
    RotatePoint( &p, -Module->m_Orient );
    Pad->m_Pos0 = FROM_LEGACY_LU_VEC( p );

    /* Automatically increment the current pad number. */
    long num    = 0;
    int  ponder = 1;

    while( lastPadName.Len() && lastPadName.Last() >= '0' && lastPadName.Last() <= '9' )
    {
        num += ( lastPadName.Last() - '0' ) * ponder;
        lastPadName.RemoveLast();
        ponder *= 10;
    }

    num++;  // Use next number for the new pad
    lastPadName << num;
    Pad->SetPadName( lastPadName );
    g_Pad_Master.SetPadName(lastPadName);

    Module->CalculateBoundingBox();
    Pad->DisplayInfo( this );

    if( draw )
        DrawPanel->RefreshDrawingRect( Module->GetBoundingBox() );
}


/**
 * Function DeletePad
 * Delete the pad aPad.
 * Refresh the modified screen area
 * Refresh modified parameters of the parent module (bounding box, last date)
 * @param aPad = the pad to delete
 * @param aQuery = true to promt for confirmation, false to delete silently
 */
void PCB_BASE_FRAME::DeletePad( D_PAD* aPad, bool aQuery )
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
    DrawPanel->RefreshDrawingRect( Module->GetBoundingBox() );
    Module->CalculateBoundingBox();

    OnModify();
}


/* Function to initialize the "move pad" command */
void PCB_BASE_FRAME::StartMovePad( D_PAD* Pad, wxDC* DC )
{
    if( Pad == NULL )
        return;

    s_CurrentSelectedPad = Pad;
    Pad_OldPos = Pad->m_Pos;
    Pad->DisplayInfo( this );
    DrawPanel->SetMouseCapture( Show_Pad_Move, Abort_Move_Pad );

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
void PCB_BASE_FRAME::PlacePad( D_PAD* Pad, wxDC* DC )
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

        Track->SetState( IN_EDIT, OFF );

        if( DC )
            Track->Draw( DrawPanel, DC, GR_OR );
    }

    /* Compute local coordinates (i.e refer to Module position and for Module orient = 0) */
    dX = Pad->m_Pos.x - Pad_OldPos.x;
    dY = Pad->m_Pos.y - Pad_OldPos.y;
    RotatePoint( &dX, &dY, -Module->m_Orient );

    Pad->m_Pos0.x() += FROM_LEGACY_LU( dX );
    s_CurrentSelectedPad->m_Pos0.y() += FROM_LEGACY_LU( dY ); /// @BUG???

    Pad->m_Flags = 0;

    if( DC )
        Pad->Draw( DrawPanel, DC, GR_OR );

    Module->CalculateBoundingBox();
    Module->m_LastEdit_Time = time( NULL );

    EraseDragList();

    OnModify();
    DrawPanel->SetMouseCapture( NULL, NULL );
    m_Pcb->m_Status_Pcb &= ~( LISTE_RATSNEST_ITEM_OK | CONNEXION_OK );
}


/* Rotate selected pad 90 degrees.
 */
void PCB_BASE_FRAME::RotatePad( D_PAD* Pad, wxDC* DC )
{
    MODULE* Module;

    if( Pad == NULL )
        return;

    Module = (MODULE*) Pad->GetParent();
    Module->m_LastEdit_Time = time( NULL );

    OnModify();

    if( DC )
        Module->Draw( DrawPanel, DC, GR_XOR );

    EXCHG( Pad->m_Size.x(), Pad->m_Size.y() );
    EXCHG( Pad->m_Drill.x(), Pad->m_Drill.y() );
    EXCHG( Pad->m_Offset.x(), Pad->m_Offset.y() );
    Pad->m_Offset.y() = -Pad->m_Offset.y();

    EXCHG( Pad->m_DeltaSize.x(), Pad->m_DeltaSize.y() );
    Pad->m_DeltaSize.x() = -Pad->m_DeltaSize.x();
    Module->CalculateBoundingBox();
    Pad->DisplayInfo( this );

    if( DC )
        Module->Draw( DrawPanel, DC, GR_OR );
}
