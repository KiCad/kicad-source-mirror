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


/* Routines Locales */
static void Show_Polyline_in_Ghost( EDA_DRAW_PANEL* panel, wxDC* DC, bool erase );
static void AbortCreateNewLine( EDA_DRAW_PANEL* Panel, wxDC* DC );
static bool IsTerminalPoint( SCH_SCREEN* screen, const wxPoint& pos, int layer );
static bool IsJunctionNeeded( SCH_EDIT_FRAME* frame, wxPoint& pos );
static void ComputeBreakPoint( SCH_LINE* segment, const wxPoint& new_pos );

SCH_ITEM* s_OldWiresList;
wxPoint   s_ConnexionStartPoint;


/* Replace the wires in screen->GetDrawItems() by s_OldWiresList wires.
 */
static void RestoreOldWires( SCH_SCREEN* screen )
{
    SCH_ITEM* item;
    SCH_ITEM* next_item;

    for( item = screen->GetDrawItems(); item != NULL; item = next_item )
    {
        next_item = item->Next();

        switch( item->Type() )
        {
        case SCH_JUNCTION_T:
        case SCH_LINE_T:
            screen->RemoveFromDrawList( item );
            delete item;
            break;

        default:
            break;
        }
    }

    while( s_OldWiresList )
    {
        next_item = s_OldWiresList->Next();

        s_OldWiresList->SetNext( screen->GetDrawItems() );
        screen->SetDrawItems( s_OldWiresList );
        s_OldWiresList     = next_item;
    }
}


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

    wxPoint endpos = aPanel->GetScreen()->m_Curseur;

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
    wxPoint   cursorpos = GetScreen()->m_Curseur;

    if( GetScreen()->GetCurItem() && (GetScreen()->GetCurItem()->m_Flags == 0) )
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

        newsegment->m_Flags = IS_NEW;

        if( g_HVLines ) // We need 2 segments to go from a given start pin to an end point
        {
            nextsegment = new SCH_LINE( *newsegment );
            nextsegment->m_Flags = IS_NEW;
            newsegment->SetNext( nextsegment );
            nextsegment->SetBack( newsegment );
        }

        GetScreen()->SetCurItem( newsegment );
        DrawPanel->ManageCurseur = DrawSegment;
        DrawPanel->ForceCloseManageCurseur = AbortCreateNewLine;
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

        DrawPanel->ManageCurseur( DrawPanel, DC, wxDefaultPosition, false );

        /* Creates the new segment, or terminates the command
         * if the end point is on a pin, junction or an other wire or bus */
        if( IsTerminalPoint( GetScreen(), cursorpos, oldsegment->GetLayer() ) )
        {
            EndSegment( DC );
            return;
        }

        oldsegment->SetNext( GetScreen()->GetDrawItems() );
        GetScreen()->SetDrawItems( oldsegment );
        DrawPanel->CursorOff( DC );     // Erase schematic cursor
        oldsegment->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );
        DrawPanel->CursorOn( DC );      // Display schematic cursor

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
        oldsegment->m_Flags = SELECTED;
        newsegment->m_Flags = IS_NEW;
        GetScreen()->SetCurItem( newsegment );
        DrawPanel->ManageCurseur( DrawPanel, DC, wxDefaultPosition, false );

        /* This is the first segment: Now we know the start segment position.
         * Create a junction if needed. Note: a junction can be needed later,
         * if the new segment is merged (after a cleanup) with an older one
         * (tested when the connection will be finished)*/
        if( oldsegment->m_Start == s_ConnexionStartPoint )
        {
            if( IsJunctionNeeded( this, s_ConnexionStartPoint ) )
                CreateNewJunctionStruct( DC, s_ConnexionStartPoint );
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

    if( ( firstsegment->m_Flags & IS_NEW ) == 0 )
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

    DrawPanel->ManageCurseur = NULL;
    DrawPanel->ForceCloseManageCurseur = NULL;
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
        if( segment->m_Flags )
        {
            if( !m_itemToRepeat )
                m_itemToRepeat = segment;
        }

        segment->m_Flags = 0;
        segment = segment->Next();
    }

    // Automatic place of a junction on the end point, if needed
    if( lastsegment )
    {
        if( IsJunctionNeeded( this, end_point ) )
            CreateNewJunctionStruct( DC, end_point );
        else if( IsJunctionNeeded( this, alt_end_point ) )
            CreateNewJunctionStruct( DC, alt_end_point );
    }

    /* Automatic place of a junction on the start point if necessary because
     * the cleanup can suppress intermediate points by merging wire segments */
    if( IsJunctionNeeded( this, s_ConnexionStartPoint ) )
        CreateNewJunctionStruct( DC, s_ConnexionStartPoint );

    GetScreen()->TestDanglingEnds( DrawPanel, DC );

    /* Redraw wires and junctions which can be changed by TestDanglingEnds() */
    DrawPanel->CursorOff( DC );   // Erase schematic cursor
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

    DrawPanel->CursorOn( DC );    // Display schematic cursor

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


/*  Drawing Polyline phantom at the displacement of the cursor
 */
static void Show_Polyline_in_Ghost( EDA_DRAW_PANEL* panel, wxDC* DC, bool erase )
{
    SCH_POLYLINE* NewPoly = (SCH_POLYLINE*) panel->GetScreen()->GetCurItem();
    int           color;
    wxPoint       endpos;

    endpos = panel->GetScreen()->m_Curseur;
    color  = ReturnLayerColor( NewPoly->GetLayer() );

    GRSetDrawMode( DC, g_XorMode );

    int idx = NewPoly->GetCornerCount() - 1;

    if( g_HVLines )
    {
        /* Coerce the line to vertical or horizontal one: */
        if( ABS( endpos.x - NewPoly->m_PolyPoints[idx].x ) <
           ABS( endpos.y - NewPoly->m_PolyPoints[idx].y ) )
            endpos.x = NewPoly->m_PolyPoints[idx].x;
        else
            endpos.y = NewPoly->m_PolyPoints[idx].y;
    }

    if( erase )
        NewPoly->Draw( panel, DC, wxPoint( 0, 0 ), g_XorMode, color );

    NewPoly->m_PolyPoints[idx] = endpos;
    NewPoly->Draw( panel, DC, wxPoint( 0, 0 ), g_XorMode, color );
}


/*
 *  Erase the last trace or the element at the current mouse position.
 */
void SCH_EDIT_FRAME::DeleteCurrentSegment( wxDC* DC )
{
    m_itemToRepeat = NULL;

    if( ( GetScreen()->GetCurItem() == NULL )
       || ( ( GetScreen()->GetCurItem()->m_Flags & IS_NEW ) == 0 ) )
    {
        return;
    }

    /* Cancel trace in progress */
    if( GetScreen()->GetCurItem()->Type() == SCH_POLYLINE_T )
    {
        Show_Polyline_in_Ghost( DrawPanel, DC, false );
    }
    else
    {
        DrawSegment( DrawPanel, DC, wxDefaultPosition, false );
    }

    EraseStruct( (SCH_ITEM*) GetScreen()->GetCurItem(), GetScreen() );
    DrawPanel->ManageCurseur = NULL;
    GetScreen()->SetCurItem( NULL );
}


/* Routine to create new connection struct.
 */
SCH_JUNCTION* SCH_EDIT_FRAME::CreateNewJunctionStruct( wxDC*          DC,
                                                       const wxPoint& pos,
                                                       bool           PutInUndoList )
{
    SCH_JUNCTION* NewJunction;

    NewJunction = new SCH_JUNCTION( pos );

    m_itemToRepeat = NewJunction;

    DrawPanel->CursorOff( DC );     // Erase schematic cursor
    NewJunction->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );
    DrawPanel->CursorOn( DC );      // Display schematic cursor

    NewJunction->SetNext( GetScreen()->GetDrawItems() );
    GetScreen()->SetDrawItems( NewJunction );
    OnModify();

    if( PutInUndoList )
        SaveCopyInUndoList( NewJunction, UR_NEW );

    return NewJunction;
}


SCH_NO_CONNECT* SCH_EDIT_FRAME::AddNoConnect( wxDC* aDC, const wxPoint& aPosition )
{
    SCH_NO_CONNECT* NewNoConnect;

    NewNoConnect   = new SCH_NO_CONNECT( aPosition );
    m_itemToRepeat = NewNoConnect;

    DrawPanel->CursorOff( aDC );     // Erase schematic cursor
    NewNoConnect->Draw( DrawPanel, aDC, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );
    DrawPanel->CursorOn( aDC );      // Display schematic cursor

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
    SCH_SCREEN* Screen = (SCH_SCREEN*) Panel->GetScreen();

    if( Screen->GetCurItem() )
    {
        Panel->ManageCurseur = NULL;
        Panel->ForceCloseManageCurseur = NULL;
        EraseStruct( (SCH_ITEM*) Screen->GetCurItem(), (SCH_SCREEN*) Screen );
        Screen->SetCurItem( NULL );
        RestoreOldWires( Screen );
        Panel->Refresh();
    }
    else
    {
        SCH_EDIT_FRAME* parent = ( SCH_EDIT_FRAME* ) Panel->GetParent();

        parent->SetRepeatItem( NULL );
    }

    /* Clear m_Flags which is used in edit functions: */
    SCH_ITEM* item = Screen->GetDrawItems();

    while( item )
    {
        item->m_Flags = 0;
        item = item->Next();
    }
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
        wxPoint pos = GetScreen()->m_Curseur - ( (SCH_COMPONENT*) m_itemToRepeat )->m_Pos;
        m_itemToRepeat->m_Flags = IS_NEW;
        ( (SCH_COMPONENT*) m_itemToRepeat )->m_TimeStamp = GetTimeStamp();
        m_itemToRepeat->Move( pos );
        m_itemToRepeat->Draw( DrawPanel, DC, wxPoint( 0, 0 ), g_XorMode );
        StartMovePart( (SCH_COMPONENT*) m_itemToRepeat, DC );
        return;
    }

    m_itemToRepeat->Move( wxPoint( g_RepeatStep.GetWidth(), g_RepeatStep.GetHeight() ) );

    if( m_itemToRepeat->Type() == SCH_TEXT_T
        || m_itemToRepeat->Type() == SCH_LABEL_T
        || m_itemToRepeat->Type() == SCH_HIERARCHICAL_LABEL_T
        || m_itemToRepeat->Type() == SCH_GLOBAL_LABEL_T )
    {
        ( (SCH_TEXT*) m_itemToRepeat )->IncrementLabel();
    }

    if( m_itemToRepeat )
    {
        m_itemToRepeat->SetNext( GetScreen()->GetDrawItems() );
        GetScreen()->SetDrawItems( m_itemToRepeat );
        GetScreen()->TestDanglingEnds();
        m_itemToRepeat->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );
        SaveCopyInUndoList( m_itemToRepeat, UR_NEW );
        m_itemToRepeat->m_Flags = 0;
    }
}


/* Routine incrementing labels, ie for the text ending with a number, adding
 * that a number <RepeatDeltaLabel>
 */
void IncrementLabelMember( wxString& name )
{
    int  ii, nn;
    long number = 0;

    ii = name.Len() - 1; nn = 0;

    if( !isdigit( name.GetChar( ii ) ) )
        return;

    while( (ii >= 0) && isdigit( name.GetChar( ii ) ) )
    {
        ii--; nn++;
    }

    ii++;   /* digits are starting at ii position */
    wxString litt_number = name.Right( nn );

    if( litt_number.ToLong( &number ) )
    {
        number += g_RepeatDeltaLabel;
        name.Remove( ii ); name << number;
    }
}


/* Return true if pos can be a terminal point for a wire or a bus
 * i.e. :
 *  for a WIRE, if at pos is found:
 *      - a junction
 *      - or a pin
 *      - or an other wire
 *
 *  - for a BUS, if at pos is found:
 *      - a BUS
 */
static bool IsTerminalPoint( SCH_SCREEN* screen, const wxPoint& pos, int layer )
{
    EDA_ITEM*      item;
    LIB_PIN*       pin;
    SCH_COMPONENT* LibItem = NULL;
    SCH_SHEET_PIN* pinsheet;
    wxPoint        itempos;

    switch( layer )
    {
    case LAYER_BUS:
        item = PickStruct( pos, screen, BUS_T );

        if( item )
            return true;

        pinsheet = screen->GetSheetLabel( pos );

        if( pinsheet && IsBusLabel( pinsheet->m_Text ) )
        {
            itempos = pinsheet->m_Pos;

            if( (itempos.x == pos.x) && (itempos.y == pos.y) )
                return true;
        }
        break;

    case LAYER_NOTES:
        item = PickStruct( pos, screen, DRAW_ITEM_T );
        if( item )
            return true;
        break;

    case LAYER_WIRE:
        item = PickStruct( pos, screen, BUS_ENTRY_T | JUNCTION_T );

        if( item )
            return true;

        pin = screen->GetPin( pos, &LibItem );

        if( pin && LibItem )
        {
            // Calculate the exact position of the connection point of the pin,
            // depending on orientation of the component.
            itempos    = LibItem->GetScreenCoord( pin->GetPosition() );
            itempos.x += LibItem->m_Pos.x;
            itempos.y += LibItem->m_Pos.y;

            if( ( itempos.x == pos.x ) && ( itempos.y == pos.y ) )
                return true;
        }

        item = PickStruct( pos, screen, WIRE_T );

        if( item )
            return true;

        item = PickStruct( pos, screen, LABEL_T );
        if( item && (item->Type() != SCH_TEXT_T)
           && ( ( (SCH_GLOBALLABEL*) item )->m_Pos.x == pos.x )
           && ( ( (SCH_GLOBALLABEL*) item )->m_Pos.y == pos.y ) )
            return true;

        pinsheet = screen->GetSheetLabel( pos );

        if( pinsheet && !IsBusLabel( pinsheet->m_Text ) )
        {
            itempos = pinsheet->m_Pos;

            if( ( itempos.x == pos.x ) && ( itempos.y == pos.y ) )
                return true;
        }

        break;

    default:
        break;
    }

    return false;
}


/* Return True when a wire is located at pos "pos" if
 *  - there is no junction.
 *  - The wire has no ends at pos "pos",
 *      and therefore it is considered as no connected.
 *  - One (or more) wire has one end at pos "pos"
 *  or
 *  - a pin is on location pos
 */
bool IsJunctionNeeded( SCH_EDIT_FRAME* frame, wxPoint& pos )
{
    if( PickStruct( pos, frame->GetScreen(), JUNCTION_T ) )
        return false;

    if( PickStruct( pos, frame->GetScreen(), WIRE_T | EXCLUDE_ENDPOINTS_T ) )
    {
        if( PickStruct( pos, frame->GetScreen(), WIRE_T | ENDPOINTS_ONLY_T ) )
            return true;

        if( frame->GetScreen()->GetPin( pos, NULL, true ) )
            return true;
    }

    return false;
}
