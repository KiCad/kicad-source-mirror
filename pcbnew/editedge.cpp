/***********************************/
/* Edit segments and edges of PCB. */
/***********************************/

#include "fctsys.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "wxPcbStruct.h"
#include "gr_basic.h"
#include "pcbcommon.h"

#include "pcbnew.h"
#include "protos.h"

#include "class_board.h"
#include "class_drawsegment.h"


static void Abort_EditEdge( EDA_DRAW_PANEL* Panel, wxDC* DC );
static void DrawSegment( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition, bool aErase );
static void Move_Segment( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                          bool aErase );


static wxPoint s_InitialPosition;  // Initial cursor position.
static wxPoint s_LastPosition;     // Current cursor position.


/* Start move of a graphic element type DRAWSEGMENT */
void PCB_EDIT_FRAME::Start_Move_DrawItem( DRAWSEGMENT* drawitem, wxDC* DC )
{
    if( drawitem == NULL )
        return;

    drawitem->Draw( DrawPanel, DC, GR_XOR );
    drawitem->SetFlags( IS_MOVED );
    s_InitialPosition = s_LastPosition = GetScreen()->GetCrossHairPosition();
    drawitem->DisplayInfo( this );
    DrawPanel->SetMouseCapture( Move_Segment, Abort_EditEdge );
    SetCurItem( drawitem );
    DrawPanel->m_mouseCaptureCallback( DrawPanel, DC, wxDefaultPosition, false );
}


/*
 * Place graphic element of type DRAWSEGMENT.
 */
void PCB_EDIT_FRAME::Place_DrawItem( DRAWSEGMENT* drawitem, wxDC* DC )
{
    if( drawitem == NULL )
        return;

    drawitem->ClearFlags();
    SaveCopyInUndoList(drawitem, UR_MOVED, s_LastPosition - s_InitialPosition);
    drawitem->Draw( DrawPanel, DC, GR_OR );
    DrawPanel->SetMouseCapture( NULL, NULL );
    SetCurItem( NULL );
    OnModify();
}

/*
 * Redraw segment during cursor movement.
 */
static void Move_Segment( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                          bool aErase )
{
    DRAWSEGMENT* segment = (DRAWSEGMENT*) aPanel->GetScreen()->GetCurItem();

    if( segment == NULL )
        return;

    if( aErase )
        segment->Draw( aPanel, aDC, GR_XOR );

    wxPoint delta;
    delta = aPanel->GetScreen()->GetCrossHairPosition() - s_LastPosition;

    segment->SetStart( segment->GetStart() + delta );
    segment->SetEnd(   segment->GetEnd()   + delta );

    s_LastPosition = aPanel->GetScreen()->GetCrossHairPosition();

    segment->Draw( aPanel, aDC, GR_XOR );
}


void PCB_EDIT_FRAME::Delete_Segment_Edge( DRAWSEGMENT* Segment, wxDC* DC )
{
    EDA_ITEM* PtStruct;
    int       track_fill_copy = DisplayOpt.DisplayDrawItems;

    if( Segment == NULL )
        return;

    if( Segment->IsNew() )  // Trace in progress.
    {
        /* Delete current segment. */
        DisplayOpt.DisplayDrawItems = SKETCH;
        Segment->Draw( DrawPanel, DC, GR_XOR );
        PtStruct = Segment->Back();
        Segment ->DeleteStructure();

        if( PtStruct && (PtStruct->Type() == PCB_LINE_T ) )
            Segment = (DRAWSEGMENT*) PtStruct;

        DisplayOpt.DisplayDrawItems = track_fill_copy;
        SetCurItem( NULL );
    }
    else if( Segment->GetFlags() == 0 )
    {
        Segment->Draw( DrawPanel, DC, GR_XOR );
        Segment->ClearFlags();
        SaveCopyInUndoList(Segment, UR_DELETED);
        Segment->UnLink();
        SetCurItem( NULL );
        OnModify();
    }
}


void PCB_EDIT_FRAME::Delete_Drawings_All_Layer( int aLayer )
{
    if( aLayer <= LAST_COPPER_LAYER )
    {
        DisplayError( this, _( "Copper layer global delete not allowed!" ) );
        return;
    }

    wxString msg = _( "Delete Layer " ) + GetBoard()->GetLayerName( aLayer );

    if( !IsOK( this, msg ) )
        return;

    PICKED_ITEMS_LIST pickList;
    ITEM_PICKER picker(NULL, UR_DELETED);

    BOARD_ITEM*     PtNext;
    for( BOARD_ITEM* item = GetBoard()->m_Drawings;  item;  item = PtNext )
    {
        PtNext = item->Next();

        switch( item->Type() )
        {
        case PCB_LINE_T:
        case PCB_TEXT_T:
        case PCB_DIMENSION_T:
        case PCB_TARGET_T:
            if( item->GetLayer() == aLayer )
            {
                item->UnLink();
                picker.m_PickedItem = item;
                pickList.PushItem( picker );
            }

            break;

        default:
        {
            wxString msg;
            msg.Printf( wxT("Delete_Drawings_All_Layer() error: unknown type %d"),
                        item->Type() );
            wxMessageBox( msg );
            break;
        }
        }
    }

    if( pickList.GetCount() )
    {
        OnModify();
        SaveCopyInUndoList(pickList, UR_DELETED);
    }
}


static void Abort_EditEdge( EDA_DRAW_PANEL* Panel, wxDC* DC )
{
    DRAWSEGMENT* Segment = (DRAWSEGMENT*) Panel->GetScreen()->GetCurItem();

    if( Segment == NULL )
    {
        Panel->SetMouseCapture( NULL, NULL );
        return;
    }

    if( Segment->IsNew() )
    {
        Panel->m_mouseCaptureCallback( Panel, DC, wxDefaultPosition, false );
        Segment ->DeleteStructure();
        Segment = NULL;
    }
    else
    {
        wxPoint pos = Panel->GetScreen()->GetCrossHairPosition();
        Panel->GetScreen()->SetCrossHairPosition( s_InitialPosition );
        Panel->m_mouseCaptureCallback( Panel, DC, wxDefaultPosition, true );
        Panel->GetScreen()->SetCrossHairPosition( pos );
        Segment->ClearFlags();
        Segment->Draw( Panel, DC, GR_OR );
    }

    Panel->SetMouseCapture( NULL, NULL );
    ( (PCB_EDIT_FRAME*) Panel->GetParent() )->SetCurItem( NULL );
}


/* Initialize the drawing of a segment of type other than trace.
 */
DRAWSEGMENT* PCB_EDIT_FRAME::Begin_DrawSegment( DRAWSEGMENT* Segment, int shape, wxDC* DC )
{
    int          s_large;
    DRAWSEGMENT* DrawItem;

    s_large = GetBoard()->GetDesignSettings().m_DrawSegmentWidth;

    if( getActiveLayer() == EDGE_N )
    {
        s_large = GetBoard()->GetDesignSettings().m_EdgeSegmentWidth;
    }

    if( Segment == NULL )        /* Create new trace. */
    {
        SetCurItem( Segment = new DRAWSEGMENT( GetBoard() ) );
        Segment->SetFlags( IS_NEW );
        Segment->SetLayer( getActiveLayer() );
        Segment->SetWidth( s_large );
        Segment->SetShape( shape );
        Segment->SetAngle( 900 );
        Segment->SetStart( GetScreen()->GetCrossHairPosition() );
        Segment->SetEnd( GetScreen()->GetCrossHairPosition() );
        DrawPanel->SetMouseCapture( DrawSegment, Abort_EditEdge );
    }
    else    /* The ending point ccordinate Segment->m_End was updated by he function
             * DrawSegment() called on a move mouse event
             * during the segment creation
             */
    {
        if( Segment->GetStart() != Segment->GetEnd() )
        {
            if( Segment->GetShape() == S_SEGMENT )
            {
                SaveCopyInUndoList( Segment, UR_NEW );
                GetBoard()->Add( Segment );

                OnModify();
                Segment->ClearFlags();

                Segment->Draw( DrawPanel, DC, GR_OR );

                DrawItem = Segment;

                SetCurItem( Segment = new DRAWSEGMENT( GetBoard() ) );

                Segment->SetFlags( IS_NEW );
                Segment->SetLayer( DrawItem->GetLayer() );
                Segment->SetWidth( s_large );
                Segment->SetShape( DrawItem->GetShape() );
                Segment->SetType( DrawItem->GetType() );
                Segment->SetAngle( DrawItem->GetAngle() );
                Segment->SetStart( DrawItem->GetEnd() );
                Segment->SetEnd( DrawItem->GetEnd() );
                DrawSegment( DrawPanel, DC, wxDefaultPosition, false );
            }
            else
            {
                End_Edge( Segment, DC );
                Segment = NULL;
            }
        }
    }

    return Segment;
}


void PCB_EDIT_FRAME::End_Edge( DRAWSEGMENT* Segment, wxDC* DC )
{
    if( Segment == NULL )
        return;

    Segment->Draw( DrawPanel, DC, GR_OR );

    // Delete if segment length is zero.
    if( Segment->GetStart() == Segment->GetEnd() )
    {
        Segment->DeleteStructure();
    }
    else
    {
        Segment->ClearFlags();
        GetBoard()->Add( Segment );
        OnModify();
        SaveCopyInUndoList( Segment, UR_NEW );
    }

    DrawPanel->SetMouseCapture( NULL, NULL );
    SetCurItem( NULL );
}


/* Redraw segment during cursor movement
 */
static void DrawSegment( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition, bool aErase )
{
    DRAWSEGMENT* Segment = (DRAWSEGMENT*) aPanel->GetScreen()->GetCurItem();
    int          t_fill = DisplayOpt.DisplayDrawItems;

    if( Segment == NULL )
        return;

    DisplayOpt.DisplayDrawItems = SKETCH;

    if( aErase )
        Segment->Draw( aPanel, aDC, GR_XOR );

    if( Segments_45_Only && Segment->GetShape() == S_SEGMENT )
    {
        wxPoint pt;

        CalculateSegmentEndPoint( aPanel->GetScreen()->GetCrossHairPosition(),
                                  Segment->GetStart().x, Segment->GetStart().y,
                                  &pt.x, &pt.y );
        Segment->SetEnd( pt );
    }
    else    // here the angle is arbitrary
    {
        Segment->SetEnd( aPanel->GetScreen()->GetCrossHairPosition() );
    }

    Segment->Draw( aPanel, aDC, GR_XOR );
    DisplayOpt.DisplayDrawItems = t_fill;
}
