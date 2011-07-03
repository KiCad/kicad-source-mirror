/***************************************************************/
/* Code for handling creation of buses, wires, and junctions. */
/***************************************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "wxEeschemaStruct.h"
#include "class_sch_screen.h"

#include "lib_draw_item.h"
#include "lib_pin.h"
#include "general.h"
#include "protos.h"
#include "sch_bus_entry.h"
#include "sch_junction.h"
#include "sch_line.h"
#include "sch_no_connect.h"
#include "sch_polyline.h"
#include "sch_text.h"
#include "sch_component.h"
#include "sch_sheet.h"


static void AbortCreateNewLine( EDA_DRAW_PANEL* Panel, wxDC* DC );
static void ComputeBreakPoint( SCH_LINE* segment, const wxPoint& new_pos );

SCH_ITEM* s_OldWiresList;
wxPoint   s_ConnexionStartPoint;


/**
 * Mouse capture callback for drawing line segments.
 */
static void DrawSegment( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                         bool aErase )
{
    SCH_LINE* CurrentLine = (SCH_LINE*) aPanel->GetScreen()->GetCurItem();
    SCH_LINE* segment;
    int color;

    if( CurrentLine == NULL )
        return;

    color = ReturnLayerColor( CurrentLine->GetLayer() ) ^ HIGHLIGHT_FLAG;

    if( aErase )
    {
        segment = CurrentLine;

        while( segment )
        {
            if( !segment->IsNull() )  // Redraw if segment length != 0
                segment->Draw( aPanel, aDC, wxPoint( 0, 0 ), g_XorMode, color );

            segment = segment->Next();
        }
    }

    wxPoint endpos = aPanel->GetScreen()->GetCrossHairPosition();

    if( g_HVLines ) /* Coerce the line to vertical or horizontal one: */
        ComputeBreakPoint( CurrentLine, endpos );
    else
        CurrentLine->m_End = endpos;

    segment = CurrentLine;

    while( segment )
    {
        if( !segment->IsNull() )  // Redraw if segment length != 0
            segment->Draw( aPanel, aDC, wxPoint( 0, 0 ), g_XorMode, color );

        segment = segment->Next();
    }
}


/* Creates a new segment ( WIRE, BUS ),
 * or terminates the current segment
 * If the end of the current segment is on an other segment, place a junction
 * if needed and terminates the command
 * If the end of the current segment is on a pin, terminates the command
 * In others cases starts a new segment
 */
void SCH_EDIT_FRAME::BeginSegment( wxDC* DC, int type )
{
    SCH_LINE* oldsegment, * newsegment, * nextsegment;
    wxPoint   cursorpos = GetScreen()->GetCrossHairPosition();

    if( GetScreen()->GetCurItem() && (GetScreen()->GetCurItem()->GetFlags() == 0) )
        GetScreen()->SetCurItem( NULL );

    if( GetScreen()->GetCurItem() )
    {
        switch( GetScreen()->GetCurItem()->Type() )
        {
        case SCH_LINE_T:
        case SCH_POLYLINE_T:
            break;

        default:
            return;
        }
    }

    oldsegment = newsegment = (SCH_LINE*) GetScreen()->GetCurItem();

    if( !newsegment )  /* first point : Create first wire or bus */
    {
        s_ConnexionStartPoint = cursorpos;
        s_OldWiresList = GetScreen()->ExtractWires( true );
        GetScreen()->SchematicCleanUp( DrawPanel );

        switch( type )
        {
        default:
            newsegment = new SCH_LINE( cursorpos, LAYER_NOTES );
            break;

        case LAYER_WIRE:
            newsegment = new SCH_LINE( cursorpos, LAYER_WIRE );

            /* A junction will be created later, when we'll know the
             * segment end position, and if the junction is really needed */
            break;

        case LAYER_BUS:
            newsegment = new SCH_LINE( cursorpos, LAYER_BUS );
            break;
        }

        newsegment->SetFlags( IS_NEW );

        if( g_HVLines ) // We need 2 segments to go from a given start pin to an end point
        {
            nextsegment = new SCH_LINE( *newsegment );
            nextsegment->SetFlags( IS_NEW );
            newsegment->SetNext( nextsegment );
            nextsegment->SetBack( newsegment );
        }

        GetScreen()->SetCurItem( newsegment );
        DrawPanel->SetMouseCapture( DrawSegment, AbortCreateNewLine );
        m_itemToRepeat = NULL;
    }
    else    // A segment is in progress: terminates the current segment and add a new segment.
    {
        nextsegment = oldsegment->Next();

        if( !g_HVLines )
        {
            // if only one segment is needed and it has length = 0, do not create a new one.
            if( oldsegment->IsNull() )
                return;
        }
        else
        {
            /* if we want 2 segment and the last two have len = 0, do not
             * create a new one */
            if( oldsegment->IsNull() && nextsegment && nextsegment->IsNull() )
                return;
        }

        DrawPanel->m_mouseCaptureCallback( DrawPanel, DC, wxDefaultPosition, false );

        /* Creates the new segment, or terminates the command
         * if the end point is on a pin, junction or an other wire or bus */
        if( GetScreen()->IsTerminalPoint( cursorpos, oldsegment->GetLayer() ) )
        {
            EndSegment( DC );
            return;
        }

        oldsegment->SetNext( GetScreen()->GetDrawItems() );
        GetScreen()->SetDrawItems( oldsegment );
        DrawPanel->CrossHairOff( DC );     // Erase schematic cursor
        oldsegment->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );
        DrawPanel->CrossHairOn( DC );      // Display schematic cursor

        /* Create a new segment, and chain it after the current new segment */
        if( nextsegment )
        {
            newsegment = new SCH_LINE( *nextsegment );
            nextsegment->m_Start = newsegment->m_End;
            nextsegment->SetNext( NULL );
            nextsegment->SetBack( newsegment );
            newsegment->SetNext( nextsegment );
            newsegment->SetBack( NULL );
        }
        else
        {
            newsegment = new SCH_LINE( *oldsegment );
            newsegment->m_Start = oldsegment->m_End;
        }

        newsegment->m_End   = cursorpos;
        oldsegment->ClearFlags( IS_NEW );
        oldsegment->SetFlags( SELECTED );
        newsegment->SetFlags( IS_NEW );
        GetScreen()->SetCurItem( newsegment );
        DrawPanel->m_mouseCaptureCallback( DrawPanel, DC, wxDefaultPosition, false );

        /* This is the first segment: Now we know the start segment position.
         * Create a junction if needed. Note: a junction can be needed later,
         * if the new segment is merged (after a cleanup) with an older one
         * (tested when the connection will be finished)*/
        if( oldsegment->m_Start == s_ConnexionStartPoint )
        {
            if( GetScreen()->IsJunctionNeeded( s_ConnexionStartPoint ) )
                AddJunction( DC, s_ConnexionStartPoint );
        }
    }
}


/* Called to terminate a bus, wire, or line creation
 */
void SCH_EDIT_FRAME::EndSegment( wxDC* DC )
{
    SCH_LINE* firstsegment = (SCH_LINE*) GetScreen()->GetCurItem();
    SCH_LINE* lastsegment = firstsegment;
    SCH_LINE* segment;

    if( firstsegment == NULL )
        return;

    if( !firstsegment->IsNew() )
        return;

    /* Delete Null segments and Put line it in Drawlist */
    lastsegment = firstsegment;

    while( lastsegment )
    {
        SCH_LINE* nextsegment = lastsegment->Next();

        if( lastsegment->IsNull() )
        {
            SCH_LINE* previous_segment = lastsegment->Back();

            if( firstsegment == lastsegment )
                firstsegment = nextsegment;

            if( nextsegment )
                nextsegment->SetBack( NULL );

            if( previous_segment )
                previous_segment->SetNext( nextsegment );

            delete lastsegment;
        }

        lastsegment = nextsegment;
    }

    /* put the segment list to the main linked list */
    segment = lastsegment = firstsegment;

    while( segment )
    {
        lastsegment = segment;
        segment     = segment->Next();
        lastsegment->SetNext( GetScreen()->GetDrawItems() );
        GetScreen()->SetDrawItems( lastsegment );
    }

    DrawPanel->SetMouseCapture( NULL, NULL );
    GetScreen()->SetCurItem( NULL );

    wxPoint end_point, alt_end_point;

    /* A junction can be needed to connect the last segment
     *  usually to m_End coordinate.
     *  But if the last segment is removed by a cleanup, because of redundancy,
     * a junction can be needed to connect the previous segment m_End
     * coordinate with is also the lastsegment->m_Start coordinate */
    if( lastsegment )
    {
        end_point     = lastsegment->m_End;
        alt_end_point = lastsegment->m_Start;
    }

    GetScreen()->SchematicCleanUp( DrawPanel );

    /* clear flags and find last segment entered, for repeat function */
    segment = (SCH_LINE*) GetScreen()->GetDrawItems();

    while( segment )
    {
        if( segment->GetFlags() )
        {
            if( !m_itemToRepeat )
                m_itemToRepeat = segment;
        }

        segment->ClearFlags();
        segment = segment->Next();
    }

    // Automatic place of a junction on the end point, if needed
    if( lastsegment )
    {
        if( GetScreen()->IsJunctionNeeded( end_point ) )
            AddJunction( DC, end_point );
        else if( GetScreen()->IsJunctionNeeded( alt_end_point ) )
            AddJunction( DC, alt_end_point );
    }

    /* Automatic place of a junction on the start point if necessary because
     * the cleanup can suppress intermediate points by merging wire segments */
    if( GetScreen()->IsJunctionNeeded( s_ConnexionStartPoint ) )
        AddJunction( DC, s_ConnexionStartPoint );

    GetScreen()->TestDanglingEnds( DrawPanel, DC );

    /* Redraw wires and junctions which can be changed by TestDanglingEnds() */
    DrawPanel->CrossHairOff( DC );   // Erase schematic cursor
    EDA_ITEM* item = GetScreen()->GetDrawItems();

    while( item )
    {
        switch( item->Type() )
        {
        case SCH_JUNCTION_T:
        case SCH_LINE_T:
            DrawPanel->RefreshDrawingRect( item->GetBoundingBox() );
            break;

        default:
            break;
        }

        item = item->Next();
    }

    DrawPanel->CrossHairOn( DC );    // Display schematic cursor

    SaveCopyInUndoList( s_OldWiresList, UR_WIRE_IMAGE );
    s_OldWiresList = NULL;

    OnModify( );
}


/* compute the middle coordinate for 2 segments, from the start point to
 * new_pos
 *  with the 2 segments kept H or V only
 */
static void ComputeBreakPoint( SCH_LINE* segment, const wxPoint& new_pos )
{
    SCH_LINE* nextsegment     = segment->Next();
    wxPoint   middle_position = new_pos;

    if( nextsegment == NULL )
        return;
#if 0
    if( ABS( middle_position.x - segment->m_Start.x ) <
        ABS( middle_position.y - segment->m_Start.y ) )
        middle_position.x = segment->m_Start.x;
    else
        middle_position.y = segment->m_Start.y;
#else
    int iDx = segment->m_End.x - segment->m_Start.x;
    int iDy = segment->m_End.y - segment->m_Start.y;

    if( iDy != 0 )         // keep the first segment orientation (currently horizontal)
    {
        middle_position.x = segment->m_Start.x;
    }
    else if( iDx != 0 )    // keep the first segment orientation (currently vertical)
    {
        middle_position.y = segment->m_Start.y;
    }
    else
    {
        if( ABS( middle_position.x - segment->m_Start.x ) <
            ABS( middle_position.y - segment->m_Start.y ) )
            middle_position.x = segment->m_Start.x;
        else
            middle_position.y = segment->m_Start.y;
    }
#endif

    segment->m_End = middle_position;

    nextsegment->m_Start = middle_position;
    nextsegment->m_End   = new_pos;
}


/*
 *  Erase the last trace or the element at the current mouse position.
 */
void SCH_EDIT_FRAME::DeleteCurrentSegment( wxDC* DC )
{
    SCH_SCREEN* screen = GetScreen();

    m_itemToRepeat = NULL;

    if( ( screen->GetCurItem() == NULL ) || !screen->GetCurItem()->IsNew() )
        return;

    /* Cancel trace in progress */
    if( screen->GetCurItem()->Type() == SCH_POLYLINE_T )
    {
        SCH_POLYLINE* polyLine = (SCH_POLYLINE*) screen->GetCurItem();
        wxPoint       endpos;

        endpos = screen->GetCrossHairPosition();

        int idx = polyLine->GetCornerCount() - 1;

        if( g_HVLines )
        {
            /* Coerce the line to vertical or horizontal one: */
            if( ABS( endpos.x - polyLine->m_PolyPoints[idx].x ) <
                ABS( endpos.y - polyLine->m_PolyPoints[idx].y ) )
                endpos.x = polyLine->m_PolyPoints[idx].x;
            else
                endpos.y = polyLine->m_PolyPoints[idx].y;
        }

        polyLine->m_PolyPoints[idx] = endpos;
        polyLine->Draw( DrawPanel, DC, wxPoint( 0, 0 ), g_XorMode );
    }
    else
    {
        DrawSegment( DrawPanel, DC, wxDefaultPosition, false );
    }

    screen->RemoveFromDrawList( screen->GetCurItem() );
    DrawPanel->m_mouseCaptureCallback = NULL;
    screen->SetCurItem( NULL );
}


/* Routine to create new connection struct.
 */
SCH_JUNCTION* SCH_EDIT_FRAME::AddJunction( wxDC* aDC, const wxPoint& aPosition,
                                           bool aPutInUndoList )
{
    SCH_JUNCTION* junction = new SCH_JUNCTION( aPosition );

    m_itemToRepeat = junction;

    DrawPanel->CrossHairOff( aDC );     // Erase schematic cursor
    junction->Draw( DrawPanel, aDC, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );
    DrawPanel->CrossHairOn( aDC );      // Display schematic cursor

    junction->SetNext( GetScreen()->GetDrawItems() );
    GetScreen()->SetDrawItems( junction );
    OnModify();

    if( aPutInUndoList )
        SaveCopyInUndoList( junction, UR_NEW );

    return junction;
}


SCH_NO_CONNECT* SCH_EDIT_FRAME::AddNoConnect( wxDC* aDC, const wxPoint& aPosition )
{
    SCH_NO_CONNECT* NewNoConnect;

    NewNoConnect   = new SCH_NO_CONNECT( aPosition );
    m_itemToRepeat = NewNoConnect;

    DrawPanel->CrossHairOff( aDC );     // Erase schematic cursor
    NewNoConnect->Draw( DrawPanel, aDC, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );
    DrawPanel->CrossHairOn( aDC );      // Display schematic cursor

    NewNoConnect->SetNext( GetScreen()->GetDrawItems() );
    GetScreen()->SetDrawItems( NewNoConnect );
    OnModify();
    SaveCopyInUndoList( NewNoConnect, UR_NEW );
    return NewNoConnect;
}


/* Abort function for wire, bus or line creation
 */
static void AbortCreateNewLine( EDA_DRAW_PANEL* Panel, wxDC* DC )
{
    SCH_SCREEN* screen = (SCH_SCREEN*) Panel->GetScreen();

    if( screen->GetCurItem() )
    {
        screen->RemoveFromDrawList( (SCH_ITEM*) screen->GetCurItem() );
        screen->SetCurItem( NULL );
        screen->ReplaceWires( s_OldWiresList );
        Panel->Refresh();
    }
    else
    {
        SCH_EDIT_FRAME* parent = ( SCH_EDIT_FRAME* ) Panel->GetParent();
        parent->SetRepeatItem( NULL );
    }

    // Clear flags used in edit functions.
    screen->ClearDrawingState();
}


/* Repeat the last item placement.
 * Bus lines, text, labels
 * Labels that end with a number will be incremented.
 */
void SCH_EDIT_FRAME::RepeatDrawItem( wxDC* DC )
{
    if( m_itemToRepeat == NULL )
        return;

    m_itemToRepeat = m_itemToRepeat->Clone();

    if( m_itemToRepeat->Type() == SCH_COMPONENT_T ) // If repeat component then put in move mode
    {
        wxPoint pos = GetScreen()->GetCrossHairPosition() -
            ( (SCH_COMPONENT*) m_itemToRepeat )->m_Pos;
        m_itemToRepeat->SetFlags( IS_NEW );
        ( (SCH_COMPONENT*) m_itemToRepeat )->m_TimeStamp = GetTimeStamp();
        m_itemToRepeat->Move( pos );
        m_itemToRepeat->Draw( DrawPanel, DC, wxPoint( 0, 0 ), g_XorMode );
        StartMovePart( (SCH_COMPONENT*) m_itemToRepeat, DC );
        return;
    }

    m_itemToRepeat->Move( wxPoint( g_RepeatStep.GetWidth(), g_RepeatStep.GetHeight() ) );

    if( m_itemToRepeat->CanIncrementLabel() )
        ( (SCH_TEXT*) m_itemToRepeat )->IncrementLabel();

    if( m_itemToRepeat )
    {
        m_itemToRepeat->SetNext( GetScreen()->GetDrawItems() );
        GetScreen()->SetDrawItems( m_itemToRepeat );
        GetScreen()->TestDanglingEnds();
        m_itemToRepeat->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );
        SaveCopyInUndoList( m_itemToRepeat, UR_NEW );
        m_itemToRepeat->ClearFlags();
    }
}

