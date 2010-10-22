/***************************************************************/
/* Code for handling creation of buses, wires, and junctions. */
/***************************************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"

#include "program.h"
#include "lib_draw_item.h"
#include "lib_pin.h"
#include "general.h"
#include "protos.h"


/* Routines Locales */
static void Show_Polyline_in_Ghost( WinEDA_DrawPanel* panel,
                                    wxDC*             DC,
                                    bool              erase );
static void Segment_in_Ghost( WinEDA_DrawPanel* panel, wxDC* DC, bool erase );
static void AbortCreateNewLine( WinEDA_DrawPanel* Panel, wxDC* DC );
static bool IsTerminalPoint( SCH_SCREEN* screen, const wxPoint& pos, int layer );
static bool IsJunctionNeeded( WinEDA_SchematicFrame* frame, wxPoint& pos );
static void ComputeBreakPoint( SCH_LINE* segment, const wxPoint& new_pos );

SCH_ITEM* s_OldWiresList;
wxPoint   s_ConnexionStartPoint;


/* Extract the old wires, junctions and buses, an if CreateCopy replace them
 * by a copy.  Old ones must be put in undo list, and the new ones can be
 * modified by clean up safely.
 * If an abort command is made, old wires must be put in EEDrawList, and
 * copies must be deleted.  This is because previously stored undo commands
 * can handle pointers on wires or bus, and we do not delete wires or bus,
 * we must put they in undo list.
 *
 * Because cleanup delete and/or modify bus and wires, the more easy is to put
 * all wires in undo list and use a new copy of wires for cleanup.
 */
SCH_ITEM* SCH_SCREEN::ExtractWires( bool CreateCopy )
{
    SCH_ITEM* item, * next_item, * new_item, * List = NULL;

    for( item = EEDrawList; item != NULL; item = next_item )
    {
        next_item = item->Next();

        switch( item->Type() )
        {
        case DRAW_JUNCTION_STRUCT_TYPE:
        case DRAW_SEGMENT_STRUCT_TYPE:
            RemoveFromDrawList( item );
            item->SetNext( List );
            List = item;
            if( CreateCopy )
            {
                if( item->Type() == DRAW_JUNCTION_STRUCT_TYPE )
                    new_item = ( (SCH_JUNCTION*) item )->GenCopy();
                else
                    new_item = ( (SCH_LINE*) item )->GenCopy();
                new_item->SetNext( EEDrawList );
                EEDrawList = new_item;
            }
            break;

        default:
            break;
        }
    }

    return List;
}


/* Replace the wires in screen->EEDrawList by s_OldWiresList wires.
 */
static void RestoreOldWires( SCH_SCREEN* screen )
{
    SCH_ITEM* item;
    SCH_ITEM* next_item;

    for( item = screen->EEDrawList; item != NULL; item = next_item )
    {
        next_item = item->Next();

        switch( item->Type() )
        {
        case DRAW_JUNCTION_STRUCT_TYPE:
        case DRAW_SEGMENT_STRUCT_TYPE:
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

        s_OldWiresList->SetNext( screen->EEDrawList );
        screen->EEDrawList = s_OldWiresList;
        s_OldWiresList     = next_item;
    }
}


/* Creates a new segment ( WIRE, BUS ),
 * or terminates the current segment
 * If the end of the current segment is on an other segment, place a junction
 * if needed and terminates the command
 * If the end of the current segment is on a pin, terminates the command
 * In others cases starts a new segment
 */
void WinEDA_SchematicFrame::BeginSegment( wxDC* DC, int type )
{
    SCH_LINE* oldsegment, * newsegment, * nextsegment;
    wxPoint   cursorpos = GetScreen()->m_Curseur;

    if( GetScreen()->GetCurItem() && (GetScreen()->GetCurItem()->m_Flags == 0) )
        GetScreen()->SetCurItem( NULL );

    if( GetScreen()->GetCurItem() )
    {
        switch( GetScreen()->GetCurItem()->Type() )
        {
        case DRAW_SEGMENT_STRUCT_TYPE:
        case DRAW_POLYLINE_STRUCT_TYPE:
            break;

        default:
            return;
        }
    }

    oldsegment = newsegment = (SCH_LINE*) GetScreen()->GetCurItem();

    if( !newsegment )  /* first point : Create first wire or bus */
    {
        s_ConnexionStartPoint = cursorpos;
        s_OldWiresList = ( (SCH_SCREEN*) GetScreen() )->ExtractWires( TRUE );
        ( (SCH_SCREEN*) GetScreen() )->SchematicCleanUp( NULL );

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
        if( g_HVLines ) // We need 2 segments to go from a given start pint to
                        // an end point
        {
            nextsegment = newsegment->GenCopy();
            nextsegment->m_Flags = IS_NEW;
            newsegment->SetNext( nextsegment );
            nextsegment->SetBack( newsegment );
        }
        GetScreen()->SetCurItem( newsegment );
        DrawPanel->ManageCurseur = Segment_in_Ghost;
        DrawPanel->ForceCloseManageCurseur = AbortCreateNewLine;
        g_ItemToRepeat = NULL;
    }
    else    /* A segment is in progress: terminates the current segment and add
             * a new segment */
    {
        nextsegment = oldsegment->Next();
        if( !g_HVLines ) /* if only one segment is needed and the current is
                          * has len = 0, do not create a new one */
        {
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

        DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );

        /* Creates the new segment, or terminates the command
         * if the end point is on a pin, junction or an other wire or bus */
        if( IsTerminalPoint( GetScreen(), cursorpos, oldsegment->GetLayer() ) )
        {
            EndSegment( DC ); return;
        }

        oldsegment->SetNext( GetScreen()->EEDrawList );
        GetScreen()->EEDrawList = oldsegment;
        DrawPanel->CursorOff( DC );     // Erase schematic cursor
        RedrawOneStruct( DrawPanel, DC, oldsegment, GR_DEFAULT_DRAWMODE );
        DrawPanel->CursorOn( DC );      // Display schematic cursor

        /* Create a new segment, and chain it after the current new segment */
        if( nextsegment )
        {
            newsegment = nextsegment->GenCopy();
            nextsegment->m_Start = newsegment->m_End;
            nextsegment->SetNext( NULL );
            nextsegment->SetBack( newsegment );
            newsegment->SetNext( nextsegment );
            newsegment->SetBack( NULL );
        }
        else
        {
            newsegment = oldsegment->GenCopy();
            newsegment->m_Start = oldsegment->m_End;
        }
        newsegment->m_End   = cursorpos;
        oldsegment->m_Flags = SELECTED;
        newsegment->m_Flags = IS_NEW;
        GetScreen()->SetCurItem( newsegment );
        DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );

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
void WinEDA_SchematicFrame::EndSegment( wxDC* DC )
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
        lastsegment->SetNext( GetScreen()->EEDrawList );
        GetScreen()->EEDrawList = lastsegment;
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

    ( (SCH_SCREEN*) GetScreen() )->SchematicCleanUp( NULL );

    /* clear flags and find last segment entered, for repeat function */
    segment = (SCH_LINE*) GetScreen()->EEDrawList;
    while( segment )
    {
        if( segment->m_Flags )
        {
            if( !g_ItemToRepeat )
                g_ItemToRepeat = segment;
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

    TestDanglingEnds( GetScreen()->EEDrawList, DC );


    /* Redraw wires and junctions which can be changed by TestDanglingEnds() */
    DrawPanel->CursorOff( DC );   // Erase schematic cursor
    EDA_BaseStruct* item = GetScreen()->EEDrawList;
    while( item )
    {
        switch( item->Type() )
        {
        case DRAW_JUNCTION_STRUCT_TYPE:
        case DRAW_SEGMENT_STRUCT_TYPE:
            DrawPanel->PostDirtyRect( item->GetBoundingBox() );
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


/* Redraw the segment (g_HVLines == FALSE ) or the two segments (g_HVLines ==
 * TRUE )
 *  from the start point to the cursor, when moving the mouse
 */
static void Segment_in_Ghost( WinEDA_DrawPanel* panel, wxDC* DC, bool erase )
{
    SCH_LINE* CurrentLine = (SCH_LINE*) panel->GetScreen()->GetCurItem();
    SCH_LINE* segment;
    int color;

    if( CurrentLine == NULL )
        return;

    color = ReturnLayerColor( CurrentLine->GetLayer() ) ^ HIGHT_LIGHT_FLAG;

    if( erase )
    {
        segment = CurrentLine;
        while( segment )
        {
            if( !segment->IsNull() )  // Redraw if segment length != 0
                RedrawOneStruct( panel, DC, segment, g_XorMode, color );
            segment = segment->Next();
        }
    }

    wxPoint endpos = panel->GetScreen()->m_Curseur;
    if( g_HVLines ) /* Coerce the line to vertical or horizontal one: */
    {
        ComputeBreakPoint( CurrentLine, endpos );
    }
    else
        CurrentLine->m_End = endpos;

    segment = CurrentLine;
    while( segment )
    {
        if( !segment->IsNull() )  // Redraw if segment length != 0
            RedrawOneStruct( panel, DC, segment, g_XorMode, color );
        segment = segment->Next();
    }
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
    if( iDy != 0 )         // keep the first segment orientation (currently
                           // horizontal)
    {
        middle_position.x = segment->m_Start.x;
    }
    else if( iDx != 0 )    // keep the first segment orientation (currently
                           // vertical)
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
static void Show_Polyline_in_Ghost( WinEDA_DrawPanel* panel,
                                    wxDC*             DC,
                                    bool              erase )
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
        RedrawOneStruct( panel, DC, NewPoly, g_XorMode, color );

    NewPoly->m_PolyPoints[idx] = endpos;
    RedrawOneStruct( panel, DC, NewPoly, g_XorMode, color );
}


/*
 *  Erase the last trace or the element at the current mouse position.
 */
void WinEDA_SchematicFrame::DeleteCurrentSegment( wxDC* DC )
{
    g_ItemToRepeat = NULL;

    if( ( GetScreen()->GetCurItem() == NULL )
       || ( ( GetScreen()->GetCurItem()->m_Flags & IS_NEW ) == 0 ) )
    {
        return;
    }

    /* Cancel trace in progress */
    if( GetScreen()->GetCurItem()->Type() == DRAW_POLYLINE_STRUCT_TYPE )
    {
        Show_Polyline_in_Ghost( DrawPanel, DC, FALSE );
    }
    else
    {
        Segment_in_Ghost( DrawPanel, DC, FALSE );
    }

    EraseStruct( (SCH_ITEM*) GetScreen()->GetCurItem(),
                (SCH_SCREEN*) GetScreen() );
    DrawPanel->ManageCurseur = NULL;
    GetScreen()->SetCurItem( NULL );
}


/* Routine to create new connection struct.
 */
SCH_JUNCTION* WinEDA_SchematicFrame::CreateNewJunctionStruct(
    wxDC* DC, const wxPoint& pos, bool PutInUndoList )
{
    SCH_JUNCTION* NewJunction;

    NewJunction = new SCH_JUNCTION( pos );

    g_ItemToRepeat = NewJunction;

    DrawPanel->CursorOff( DC );     // Erase schematic cursor
    RedrawOneStruct( DrawPanel, DC, NewJunction, GR_DEFAULT_DRAWMODE );
    DrawPanel->CursorOn( DC );      // Display schematic cursor

    NewJunction->SetNext( GetScreen()->EEDrawList );
    GetScreen()->EEDrawList = NewJunction;
    OnModify( );
    if( PutInUndoList )
        SaveCopyInUndoList( NewJunction, UR_NEW );
    return NewJunction;
}


/* Routine to create new NoConnect struct. */
SCH_NO_CONNECT* WinEDA_SchematicFrame::CreateNewNoConnectStruct( wxDC* DC )
{
    SCH_NO_CONNECT* NewNoConnect;

    NewNoConnect   = new SCH_NO_CONNECT( GetScreen()->m_Curseur );
    g_ItemToRepeat = NewNoConnect;

    DrawPanel->CursorOff( DC );     // Erase schematic cursor
    RedrawOneStruct( DrawPanel, DC, NewNoConnect, GR_DEFAULT_DRAWMODE );
    DrawPanel->CursorOn( DC );      // Display schematic cursor

    NewNoConnect->SetNext( GetScreen()->EEDrawList );
    GetScreen()->EEDrawList = NewNoConnect;
    OnModify( );
    SaveCopyInUndoList( NewNoConnect, UR_NEW );
    return NewNoConnect;
}


/* Abort function for wire, bus or line creation
 */
static void AbortCreateNewLine( WinEDA_DrawPanel* Panel, wxDC* DC )
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
        g_ItemToRepeat = NULL;

    /* Clear m_Flags which is used in edit functions: */
    SCH_ITEM* item = Screen->EEDrawList;
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
void WinEDA_SchematicFrame::RepeatDrawItem( wxDC* DC )
{
    wxPoint new_pos;

    if( g_ItemToRepeat == NULL )
        return;

    switch( g_ItemToRepeat->Type() )
    {
    case DRAW_JUNCTION_STRUCT_TYPE:
        #undef STRUCT
        #define STRUCT ( (SCH_JUNCTION*) g_ItemToRepeat )
        g_ItemToRepeat = STRUCT->GenCopy();
        STRUCT->m_Pos += g_RepeatStep;
        new_pos = STRUCT->m_Pos;
        break;

    case DRAW_NOCONNECT_STRUCT_TYPE:
        #undef STRUCT
        #define STRUCT ( (SCH_NO_CONNECT*) g_ItemToRepeat )
        g_ItemToRepeat = STRUCT->GenCopy();
        STRUCT->m_Pos += g_RepeatStep;
        new_pos = STRUCT->m_Pos;
        break;

    case TYPE_SCH_TEXT:
        #undef STRUCT
        #define STRUCT ( (SCH_TEXT*) g_ItemToRepeat )
        g_ItemToRepeat = STRUCT->GenCopy();
        STRUCT->m_Pos += g_RepeatStep;
        new_pos = STRUCT->m_Pos;
        IncrementLabelMember( STRUCT->m_Text );
        break;


    case TYPE_SCH_LABEL:
        #undef STRUCT
        #define STRUCT ( (SCH_LABEL*) g_ItemToRepeat )
        g_ItemToRepeat = STRUCT->GenCopy();
        STRUCT->m_Pos += g_RepeatStep;
        new_pos = STRUCT->m_Pos;
        IncrementLabelMember( STRUCT->m_Text );
        break;


    case TYPE_SCH_HIERLABEL:
        #undef STRUCT
        #define STRUCT ( (SCH_HIERLABEL*) g_ItemToRepeat )
        g_ItemToRepeat = STRUCT->GenCopy();
        STRUCT->m_Pos += g_RepeatStep;
        new_pos = STRUCT->m_Pos;
        IncrementLabelMember( STRUCT->m_Text );
        break;

    case TYPE_SCH_GLOBALLABEL:
        #undef STRUCT
        #define STRUCT ( (SCH_GLOBALLABEL*) g_ItemToRepeat )
        g_ItemToRepeat = STRUCT->GenCopy();
        STRUCT->m_Pos += g_RepeatStep;
        new_pos = STRUCT->m_Pos;
        IncrementLabelMember( STRUCT->m_Text );
        break;

    case DRAW_SEGMENT_STRUCT_TYPE:
        #undef STRUCT
        #define STRUCT ( (SCH_LINE*) g_ItemToRepeat )
        g_ItemToRepeat   = STRUCT->GenCopy();
        STRUCT->m_Start += g_RepeatStep;
        new_pos = STRUCT->m_Start;
        STRUCT->m_End += g_RepeatStep;
        break;

    case DRAW_BUSENTRY_STRUCT_TYPE:
            #undef STRUCT
            #define STRUCT ( (SCH_BUS_ENTRY*) g_ItemToRepeat )
        g_ItemToRepeat = STRUCT->GenCopy();
        STRUCT->m_Pos += g_RepeatStep;
        new_pos = STRUCT->m_Pos;
        break;

    case TYPE_SCH_COMPONENT:     // In repeat command the new component is put
                                 // in move mode
        #undef STRUCT
        #define STRUCT ( (SCH_COMPONENT*) g_ItemToRepeat )

        // Create the duplicate component, position = mouse cursor
        g_ItemToRepeat = STRUCT->GenCopy();
        new_pos.x = GetScreen()->m_Curseur.x - STRUCT->m_Pos.x;
        new_pos.y = GetScreen()->m_Curseur.y - STRUCT->m_Pos.y;
        STRUCT->m_Pos = GetScreen()->m_Curseur;
        STRUCT->m_Flags     = IS_NEW;
        STRUCT->m_TimeStamp = GetTimeStamp();

        for( int ii = 0; ii < STRUCT->GetFieldCount(); ii++ )
        {
            STRUCT->GetField( ii )->m_Pos += new_pos;
        }

        RedrawOneStruct( DrawPanel, DC, STRUCT, g_XorMode );
        StartMovePart( STRUCT, DC );
        return;
        break;

    default:
        g_ItemToRepeat = NULL;
        DisplayError( this, wxT( "Repeat Type Error" ), 10 );
        break;
    }

    if( g_ItemToRepeat )
    {
        g_ItemToRepeat->SetNext( GetScreen()->EEDrawList );
        GetScreen()->EEDrawList = g_ItemToRepeat;
        TestDanglingEnds( GetScreen()->EEDrawList, NULL );
        RedrawOneStruct( DrawPanel, DC, g_ItemToRepeat, GR_DEFAULT_DRAWMODE );
        SaveCopyInUndoList( g_ItemToRepeat, UR_NEW );
        g_ItemToRepeat->m_Flags = 0;

//      GetScreen()->Curseur = new_pos;
//      DrawPanel->MouseTo( DrawPanel->CursorScreenPosition() );
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


/* Return TRUE if pos can be a terminal point for a wire or a bus
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
    EDA_BaseStruct* item;
    LIB_PIN*        pin;
    SCH_COMPONENT*  LibItem = NULL;
    SCH_SHEET_PIN*  pinsheet;
    wxPoint         itempos;

    switch( layer )
    {
    case LAYER_BUS:
        item = PickStruct( pos, screen, BUSITEM );
        if( item )
            return TRUE;
        pinsheet = LocateAnyPinSheet( pos, screen->EEDrawList );
        if( pinsheet && IsBusLabel( pinsheet->m_Text ) )
        {
            itempos = pinsheet->m_Pos;
            if( (itempos.x == pos.x) && (itempos.y == pos.y) )
                return TRUE;
        }
        break;

    case LAYER_NOTES:
        item = PickStruct( pos, screen, DRAWITEM );
        if( item )
            return TRUE;
        break;

    case LAYER_WIRE:
        item = PickStruct( pos, screen, RACCORDITEM | JUNCTIONITEM );
        if( item )
            return TRUE;

        pin = LocateAnyPin( screen->EEDrawList, pos, &LibItem );
        if( pin && LibItem )
        {
            // Calculate the exact position of the connection point of the pin,
            // depending on orientation of the component.
            itempos    = LibItem->GetScreenCoord( pin->m_Pos );
            itempos.x += LibItem->m_Pos.x;
            itempos.y += LibItem->m_Pos.y;
            if( ( itempos.x == pos.x ) && ( itempos.y == pos.y ) )
                return TRUE;
        }

        item = PickStruct( pos, screen, WIREITEM );
        if( item )
            return TRUE;

        item = PickStruct( pos, screen, LABELITEM );
        if( item && (item->Type() != TYPE_SCH_TEXT)
           && ( ( (SCH_GLOBALLABEL*) item )->m_Pos.x == pos.x )
           && ( ( (SCH_GLOBALLABEL*) item )->m_Pos.y == pos.y ) )
            return TRUE;

        pinsheet = LocateAnyPinSheet( pos, screen->EEDrawList );
        if( pinsheet && !IsBusLabel( pinsheet->m_Text ) )
        {
            itempos = pinsheet->m_Pos;
            if( ( itempos.x == pos.x ) && ( itempos.y == pos.y ) )
                return TRUE;
        }

        break;

    default:
        break;
    }

    return FALSE;
}


/* Return True when a wire is located at pos "pos" if
 *  - there is no junction.
 *  - The wire has no ends at pos "pos",
 *      and therefore it is considered as no connected.
 *  - One (or more) wire has one end at pos "pos"
 *  or
 *  - a pin is on location pos
 */
bool IsJunctionNeeded( WinEDA_SchematicFrame* frame, wxPoint& pos )
{
    if( PickStruct( pos, frame->GetScreen(), JUNCTIONITEM ) )
        return FALSE;

    if( PickStruct( pos, frame->GetScreen(), WIREITEM |
                    EXCLUDE_WIRE_BUS_ENDPOINTS ) )
    {
        if( PickStruct( pos, frame->GetScreen(), WIREITEM |
                        WIRE_BUS_ENDPOINTS_ONLY ) )
            return TRUE;
        if( frame->LocatePinEnd( frame->GetScreen()->EEDrawList, pos ) )
            return TRUE;
    }

    return FALSE;
}
