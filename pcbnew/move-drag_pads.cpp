

/**
 * @file move-drag_pads.cpp
 * @brief Edit footprint pads.
 */

#include <fctsys.h>
#include <gr_basic.h>
#include <common.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <trigo.h>
#include <block_commande.h>
#include <wxBasePcbFrame.h>
#include <macros.h>
#include <pcbcommon.h>

#include <class_board.h>
#include <class_module.h>

#include <pcbnew.h>
#include <drag.h>
#include <protos.h>


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
    pad->ClearFlags();
    pad->SetPosition( Pad_OldPos );
    pad->Draw( Panel, DC, GR_XOR );

    // Pad move in progress: the restore origin.
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

    pad->SetPosition( screen->GetCrossHairPosition() );
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
            Track->m_Start = pad->GetPosition();
        }

        if( g_DragSegmentList[ii].m_Pad_End )
        {
            Track->m_End = pad->GetPosition();
        }

        Track->Draw( aPanel, aDC, GR_XOR );
    }
}


/* Load list of features for default pad selection.
 */
void PCB_BASE_FRAME::Export_Pad_Settings( D_PAD* aPad )
{
    if( aPad == NULL )
        return;

    aPad->DisplayInfo( this );

    D_PAD& mp = GetDesignSettings().m_Pad_Master;

    mp.SetShape( aPad->GetShape() );
    mp.SetAttribute( aPad->GetAttribute() );
    mp.SetLayerMask( aPad->GetLayerMask() );

    mp.SetOrientation( aPad->GetOrientation() - aPad->GetParent()->GetOrientation() );

    mp.SetSize( aPad->GetSize() );
    mp.SetDelta( aPad->GetDelta() );

    mp.SetOffset( aPad->GetOffset() );
    mp.SetDrillSize( aPad->GetDrillSize() );
    mp.SetDrillShape( aPad->GetDrillShape() );
}


/* Imports the new values of dimensions of the pad edge by aPad
 * - Source: selected values of general characteristics
 * - Measurements are modified
 * - The position, names, and keys are not.
 */
void PCB_BASE_FRAME::Import_Pad_Settings( D_PAD* aPad, bool aDraw )
{
    if( aDraw )
    {
        aPad->SetFlags( DO_NOT_DRAW );
        m_canvas->RefreshDrawingRect( aPad->GetBoundingBox() );
        aPad->ClearFlags( DO_NOT_DRAW );
    }

    D_PAD& mp = GetDesignSettings().m_Pad_Master;

    aPad->SetShape( mp.GetShape() );
    aPad->SetLayerMask( mp.GetLayerMask() );
    aPad->SetAttribute( mp.GetAttribute() );
    aPad->SetOrientation( mp.GetOrientation() + aPad->GetParent()->GetOrientation() );
    aPad->SetSize( mp.GetSize() );
    aPad->SetDelta( wxSize( 0, 0 ) );
    aPad->SetOffset( mp.GetOffset() );
    aPad->SetDrillSize( mp.GetDrillSize() );
    aPad->SetDrillShape( mp.GetDrillShape() );

    switch( mp.GetShape() )
    {
    case PAD_TRAPEZOID:
        aPad->SetDelta( mp.GetDelta() );
        break;

    case PAD_CIRCLE:
        // set size.y to size.x
        aPad->SetSize( wxSize( aPad->GetSize().x, aPad->GetSize().x ) );
        break;

    default:
        ;
    }

    switch( mp.GetAttribute() )
    {
    case PAD_SMD:
    case PAD_CONN:
        aPad->SetDrillSize( wxSize( 0, 0 ) );
        aPad->SetOffset( wxPoint( 0, 0 ) );
        break;
    default:
        ;
    }

    if( aDraw )
        m_canvas->RefreshDrawingRect( aPad->GetBoundingBox() );

    aPad->GetParent()->m_LastEdit_Time = time( NULL );
}


/* Add a pad on the selected module.
 */
void PCB_BASE_FRAME::AddPad( MODULE* aModule, bool draw )
{
    // Last used pad name (pad num)
    wxString lastPadName = GetDesignSettings().m_Pad_Master.GetPadName();

    m_Pcb->m_Status_Pcb     = 0;
    aModule->m_LastEdit_Time = time( NULL );

    D_PAD* pad = new D_PAD( aModule );

    // Add the new pad to end of the module pad list.
    aModule->m_Pads.PushBack( pad );

    // Update the pad properties.
    Import_Pad_Settings( pad, false );
    pad->SetNetname( wxEmptyString );

    pad->SetPosition( GetScreen()->GetCrossHairPosition() );

    // Set the relative pad position
    // ( pad position for module orient, 0, and relative to the module position)

    wxPoint pos0 = pad->GetPosition() - aModule->GetPosition();
    RotatePoint( &pos0, -aModule->GetOrientation() );
    pad->SetPos0( pos0 );

    // Automatically increment the current pad number.
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
    pad->SetPadName( lastPadName );

    GetDesignSettings().m_Pad_Master.SetPadName(lastPadName);

    aModule->CalculateBoundingBox();
    pad->DisplayInfo( this );

    if( draw )
        m_canvas->RefreshDrawingRect( aModule->GetBoundingBox() );
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
    MODULE*  module;

    if( aPad == NULL )
        return;

    module = (MODULE*) aPad->GetParent();
    module->m_LastEdit_Time = time( NULL );

    if( aQuery )
    {
        wxString msg;
        msg.Printf( _( "Delete Pad (module %s %s) " ),
                    GetChars( module->m_Reference->m_Text ),
                    GetChars( module->m_Value->m_Text ) );

        if( !IsOK( this, msg ) )
            return;
    }

    m_Pcb->m_Status_Pcb = 0;
    aPad->DeleteStructure();
    m_canvas->RefreshDrawingRect( module->GetBoundingBox() );
    module->CalculateBoundingBox();

    OnModify();
}


// Function to initialize the "move pad" command
void PCB_BASE_FRAME::StartMovePad( D_PAD* aPad, wxDC* DC )
{
    if( aPad == NULL )
        return;

    s_CurrentSelectedPad = aPad;

    Pad_OldPos = aPad->GetPosition();

    aPad->DisplayInfo( this );
    m_canvas->SetMouseCapture( Show_Pad_Move, Abort_Move_Pad );

    // Draw the pad  (SKETCH mode)
    aPad->Draw( m_canvas, DC, GR_XOR );
    aPad->SetFlags( IS_MOVED );
    aPad->Draw( m_canvas, DC, GR_XOR );

    // Build the list of track segments to drag if the command is a drag pad
    if( g_Drag_Pistes_On )
        Build_1_Pad_SegmentsToDrag( m_canvas, DC, aPad );
    else
        EraseDragList();
}


// Routine to place a moved pad.
void PCB_BASE_FRAME::PlacePad( D_PAD* aPad, wxDC* DC )
{
    int     dX, dY;
    TRACK*  Track;

    if( aPad == NULL )
        return;

    MODULE* module = aPad->GetParent();

    ITEM_PICKER       picker( NULL, UR_CHANGED );
    PICKED_ITEMS_LIST pickList;

    // Save dragged track segments in undo list
    for( unsigned ii = 0; ii < g_DragSegmentList.size(); ii++ )
    {
        Track = g_DragSegmentList[ii].m_Segm;

        // Set the old state
        if( g_DragSegmentList[ii].m_Pad_Start )
            Track->m_Start = Pad_OldPos;

        if( g_DragSegmentList[ii].m_Pad_End )
            Track->m_End = Pad_OldPos;

        picker.SetItem( Track );
        pickList.PushItem( picker );
    }

    // Save old module and old items values
    wxPoint pad_curr_position = aPad->GetPosition();

    aPad->SetPosition( Pad_OldPos );

    if( g_DragSegmentList.size() == 0 )
        SaveCopyInUndoList( module, UR_CHANGED );
    else
    {
        picker.SetItem( module );
        pickList.PushItem( picker );
        SaveCopyInUndoList( pickList, UR_CHANGED );
    }

    aPad->SetPosition( pad_curr_position );
    aPad->Draw( m_canvas, DC, GR_XOR );

    // Redraw dragged track segments
    for( unsigned ii = 0; ii < g_DragSegmentList.size(); ii++ )
    {
        Track = g_DragSegmentList[ii].m_Segm;

        // Set the new state
        if( g_DragSegmentList[ii].m_Pad_Start )
            Track->m_Start = aPad->GetPosition();

        if( g_DragSegmentList[ii].m_Pad_End )
            Track->m_End = aPad->GetPosition();

        Track->SetState( IN_EDIT, OFF );

        if( DC )
            Track->Draw( m_canvas, DC, GR_OR );
    }

    // Compute local coordinates (i.e refer to module position and for module orient = 0)
    dX = aPad->GetPosition().x - Pad_OldPos.x;
    dY = aPad->GetPosition().y - Pad_OldPos.y;

    RotatePoint( &dX, &dY, -module->GetOrientation() );

    aPad->SetX0( dX + aPad->GetPos0().x );

    s_CurrentSelectedPad->SetY0( dY + s_CurrentSelectedPad->GetPos0().y );

    aPad->ClearFlags();

    if( DC )
        aPad->Draw( m_canvas, DC, GR_OR );

    module->CalculateBoundingBox();
    module->m_LastEdit_Time = time( NULL );

    EraseDragList();

    OnModify();
    m_canvas->SetMouseCapture( NULL, NULL );
    m_Pcb->m_Status_Pcb &= ~( LISTE_RATSNEST_ITEM_OK | CONNEXION_OK );
}


// Rotate selected pad 90 degrees.
void PCB_BASE_FRAME::RotatePad( D_PAD* aPad, wxDC* DC )
{
    if( aPad == NULL )
        return;

    MODULE* module = aPad->GetParent();

    module->m_LastEdit_Time = time( NULL );

    OnModify();

    if( DC )
        module->Draw( m_canvas, DC, GR_XOR );

    wxSize  sz = aPad->GetSize();
    EXCHG( sz.x, sz.y );
    aPad->SetSize( sz );

    sz = aPad->GetDrillSize();
    EXCHG( sz.x, sz.y );
    aPad->SetDrillSize( sz );

    wxPoint pt = aPad->GetOffset();
    EXCHG( pt.x, pt.y );
    aPad->SetOffset( pt );

    aPad->SetOffset( wxPoint( aPad->GetOffset().x, -aPad->GetOffset().y ) );

    sz = aPad->GetDelta();
    EXCHG( sz.x, sz.y );
    sz.x = -sz.x;
    aPad->SetDelta( sz );

    module->CalculateBoundingBox();
    aPad->DisplayInfo( this );

    if( DC )
        module->Draw( m_canvas, DC, GR_OR );
}
