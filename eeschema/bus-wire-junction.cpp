/*********************************************************/
/* Modules de creations de Traits, Wires, Bus, Junctions */
/*********************************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "protos.h"


/* Routines Locales */
static void Show_Polyline_in_Ghost( WinEDA_DrawPanel* panel, wxDC* DC, bool erase );
static void Segment_in_Ghost( WinEDA_DrawPanel* panel, wxDC* DC, bool erase );
static void AbortCreateNewLine( WinEDA_DrawPanel* Panel, wxDC* DC );
static bool IsTerminalPoint( SCH_SCREEN* screen, const wxPoint& pos, int layer );
static bool IsJunctionNeeded( WinEDA_SchematicFrame* frame, wxPoint& pos );
static void ComputeBreakPoint( EDA_DrawLineStruct* segment, const wxPoint& new_pos );

SCH_ITEM* s_OldWiresList;
wxPoint         s_ConnexionStartPoint;

/*********************************************************/
SCH_ITEM* SCH_SCREEN::ExtractWires( bool CreateCopy )
/*********************************************************/

/* Extract the old wires, junctions and busses, an if CreateCopy replace them by a copy.
 *  Old ones must be put in undo list, and the new ones can be modified by clean up
 *  safely.
 *  If an abord command is made, old wires must be put in EEDrawList, and copies must be deleted
 *  This is because previously stored undo commands can handle pointers on wires or bus,
 *  and we do not delete wires or bus, we must put they in undo list.
 *
 *  Because cleanup delete and/or modify bus and wires, the more easy is to put all wires in undo list
 *  and use a new copy of wires for cleanup
 */
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
                    new_item = ( (DrawJunctionStruct*) item )->GenCopy();
                else
                    new_item = ( (EDA_DrawLineStruct*) item )->GenCopy();
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


/*************************************************/
static void RestoreOldWires( SCH_SCREEN* screen )
/*************************************************/

/* Replace the wires in screen->EEDrawList by s_OldWiresList wires.
 */
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
        screen->EEDrawList    = s_OldWiresList;
        s_OldWiresList = next_item;
    }
}


/*************************************************************/
void WinEDA_SchematicFrame::BeginSegment( wxDC* DC, int type )
/*************************************************************/

/* Create a new segment ( WIRE, BUS ).
 */
{
    EDA_DrawLineStruct* oldsegment, * newsegment, * nextsegment;
    wxPoint             cursorpos = GetScreen()->m_Curseur;

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

    oldsegment = newsegment =
                     (EDA_DrawLineStruct*) GetScreen()->GetCurItem();

    if( !newsegment )  /* first point : Create first wire ou bus */
    {
        s_ConnexionStartPoint = cursorpos;
        s_OldWiresList = ((SCH_SCREEN*)GetScreen())->ExtractWires( TRUE );
        ((SCH_SCREEN*)GetScreen())->SchematicCleanUp( NULL );

        switch( type )
        {
        default:
            newsegment = new EDA_DrawLineStruct( cursorpos, LAYER_NOTES );
            break;

        case LAYER_WIRE:
            newsegment = new EDA_DrawLineStruct( cursorpos, LAYER_WIRE );

            /* A junction will be created later, when w'll know the
             *  segment end position, and if the junction is really needed */
            break;

        case LAYER_BUS:
            newsegment = new EDA_DrawLineStruct( cursorpos, LAYER_BUS );
            break;
        }

        newsegment->m_Flags = IS_NEW;
        if( g_HVLines ) // We need 2 segments to go from a given start pint to an end point
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
    else    /* Trace en cours: Placement d'un point supplementaire */
    {
        nextsegment = oldsegment->Next();
        if( !g_HVLines )
        { /* if only one segment is needed and the current is has len = 0, do not create a new one*/
            if( oldsegment->IsNull() )
                return;
        }
        else
        {
            /* if we want 2 segment and the last two have len = 0, do not create a new one*/
            if( oldsegment->IsNull() && nextsegment && nextsegment->IsNull() )
                return;
        }

        DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );

        /* Creation du segment suivant ou fin de trac� si point sur pin, jonction ...*/
        if( IsTerminalPoint( (SCH_SCREEN*)GetScreen(), cursorpos, oldsegment->GetLayer() ) )
        {
            EndSegment( DC ); return;
        }

        /* Placement en liste generale */
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
        if( oldsegment->m_Start == s_ConnexionStartPoint )
        {   /* This is the first segment: Now we know the start segment position.
             *  Create a junction if needed. Note: a junction can be needed
             *  later, if the new segment is merged (after a cleanup) with an older one
             *  (tested when the connection will be finished)*/
            if( IsJunctionNeeded( this, s_ConnexionStartPoint ) )
                CreateNewJunctionStruct( DC, s_ConnexionStartPoint );
        }
    }
}


/***********************************************/
void WinEDA_SchematicFrame::EndSegment( wxDC* DC )
/***********************************************/

/* Called to terminate a bus, wire, or line creation
 */
{
    EDA_DrawLineStruct* firstsegment = (EDA_DrawLineStruct*) GetScreen()->GetCurItem();
    EDA_DrawLineStruct* lastsegment  = firstsegment;
    EDA_DrawLineStruct* segment;

    if( firstsegment == NULL )
        return;
    if( (firstsegment->m_Flags & IS_NEW) == 0 )
        return;

    /* Delete Null segments and Put line it in Drawlist */
    lastsegment = firstsegment;
    while( lastsegment )
    {
        EDA_DrawLineStruct* nextsegment = lastsegment->Next();
        if( lastsegment->IsNull() )
        {
            EDA_DrawLineStruct* previous_segment = lastsegment->Back();
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
        segment = segment->Next();
        lastsegment->SetNext( GetScreen()->EEDrawList );
        GetScreen()->EEDrawList = lastsegment;
    }

    /* Fin de trace */
    DrawPanel->ManageCurseur = NULL;
    DrawPanel->ForceCloseManageCurseur = NULL;
    GetScreen()->SetCurItem( NULL );

    wxPoint end_point, alt_end_point;

    /* A junction can be needed to connect the last segment
     *  usually to m_End coordinate.
     *  But if the last segment is removed by a cleanup, because od redundancy,
     *  a junction can be needed to connect the previous segment m_End coordinate
     *  with is also the lastsegment->m_Start coordinate */
    if( lastsegment )
    {
        end_point     = lastsegment->m_End;
        alt_end_point = lastsegment->m_Start;
    }

    ((SCH_SCREEN*)GetScreen())->SchematicCleanUp( NULL );

    /* clear flags and find last segment entered, for repeat function */
    segment = (EDA_DrawLineStruct*) GetScreen()->EEDrawList;
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

    /* Automatic place of a junction on the start point if necessary because the
     *  Cleanup can suppress intermediate points by merging wire segments*/
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
            DrawPanel->PostDirtyRect(item->GetBoundingBox());
            break;

        default:
            break;
        }

        item = item->Next();
    }


    DrawPanel->CursorOn( DC );    // Display schematic cursor

    SaveCopyInUndoList( s_OldWiresList, IS_WIRE_IMAGE );
    s_OldWiresList = NULL;

    GetScreen()->SetModify();
}


/****************************************************************************/
static void Segment_in_Ghost( WinEDA_DrawPanel* panel, wxDC* DC, bool erase )
/****************************************************************************/

/*  Redraw the segment (g_HVLines == FALSE ) or the two segments (g_HVLines == TRUE )
 *  from the start point to the cursor, when moving the mouse
 */
{
    EDA_DrawLineStruct* CurrentLine =
        (EDA_DrawLineStruct*) panel->GetScreen()->GetCurItem();
    EDA_DrawLineStruct* segment;
    int color;

    if( CurrentLine == NULL )
        return;

    color = ReturnLayerColor( CurrentLine->GetLayer() ) ^ HIGHT_LIGHT_FLAG;

    if( erase )
    {
        segment = CurrentLine;
        while( segment )
        {
            if( !segment->IsNull() )  // Redraw if segment lengtht != 0
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
        if( !segment->IsNull() )  // Redraw if segment lengtht != 0
            RedrawOneStruct( panel, DC, segment, g_XorMode, color );
        segment = segment->Next();
    }
}


/**************************************************************************************/
static void ComputeBreakPoint( EDA_DrawLineStruct* segment, const wxPoint& new_pos )
/**************************************************************************************/

/* compute the middle coordinate for 2 segments, from the start point to new_pos
 *  with the 2 segments kept H or V only
 */
{
    EDA_DrawLineStruct* nextsegment     = segment->Next();
    wxPoint             middle_position = new_pos;

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


/*****************************************************************************/
static void Show_Polyline_in_Ghost( WinEDA_DrawPanel* panel, wxDC* DC, bool erase )
/*****************************************************************************/

/*  Dessin du du Polyline Fantome lors des deplacements du curseur
 */
{
    DrawPolylineStruct* NewPoly =
        (DrawPolylineStruct*) panel->GetScreen()->GetCurItem();
    int color;
    wxPoint             endpos;

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


/**********************************************************/
void WinEDA_SchematicFrame::DeleteCurrentSegment( wxDC* DC )
/**********************************************************/

/*
 *  Routine effacant le dernier trait trace, ou l'element pointe par la souris
 */
{
    g_ItemToRepeat = NULL;

    if( (GetScreen()->GetCurItem() == NULL)
       || ( (GetScreen()->GetCurItem()->m_Flags & IS_NEW) == 0 ) )
    {
        return;
    }

    /* Trace en cours: annulation */
    if( GetScreen()->GetCurItem()->Type() == DRAW_POLYLINE_STRUCT_TYPE )
    {
        Show_Polyline_in_Ghost( DrawPanel, DC, FALSE ); /* Effacement du trace en cours */
    }
    else
    {
        Segment_in_Ghost( DrawPanel, DC, FALSE ); /* Effacement du trace en cours */
    }

    EraseStruct( (SCH_ITEM*) GetScreen()->GetCurItem(), (SCH_SCREEN*)GetScreen() );
    DrawPanel->ManageCurseur = NULL;
    GetScreen()->SetCurItem( NULL );
}


/***************************************************************************/
DrawJunctionStruct* WinEDA_SchematicFrame::CreateNewJunctionStruct(
    wxDC* DC, const wxPoint& pos, bool PutInUndoList )
/***************************************************************************/

/* Routine to create new connection struct.
 */
{
    DrawJunctionStruct* NewJunction;

    NewJunction = new DrawJunctionStruct( pos );

    g_ItemToRepeat = NewJunction;

    DrawPanel->CursorOff( DC );     // Erase schematic cursor
    RedrawOneStruct( DrawPanel, DC, NewJunction, GR_DEFAULT_DRAWMODE );
    DrawPanel->CursorOn( DC );      // Display schematic cursor

    NewJunction->SetNext( GetScreen()->EEDrawList );
    GetScreen()->EEDrawList = NewJunction;
    GetScreen()->SetModify();
    if( PutInUndoList )
        SaveCopyInUndoList( NewJunction, IS_NEW );
    return NewJunction;
}


/*******************************************************************************/
DrawNoConnectStruct* WinEDA_SchematicFrame::CreateNewNoConnectStruct( wxDC* DC )
/*******************************************************************************/

/*Routine to create new NoConnect struct. ( Symbole de Non Connexion)
 */
{
    DrawNoConnectStruct* NewNoConnect;

    NewNoConnect   = new DrawNoConnectStruct( GetScreen()->m_Curseur );
    g_ItemToRepeat = NewNoConnect;

    DrawPanel->CursorOff( DC );     // Erase schematic cursor
    RedrawOneStruct( DrawPanel, DC, NewNoConnect, GR_DEFAULT_DRAWMODE );
    DrawPanel->CursorOn( DC );      // Display schematic cursor

    NewNoConnect->SetNext( GetScreen()->EEDrawList );
    GetScreen()->EEDrawList = NewNoConnect;
    GetScreen()->SetModify();
    SaveCopyInUndoList( NewNoConnect, IS_NEW );
    return NewNoConnect;
}


/*****************************************************************/
static void AbortCreateNewLine( WinEDA_DrawPanel* Panel, wxDC* DC )
/*****************************************************************/

/* Abort function for wire, bus or line creation
 */
{
    SCH_SCREEN* Screen = (SCH_SCREEN*) Panel->GetScreen();

    if( Screen->GetCurItem() )  /* trace en cours */
    {
        Panel->ManageCurseur( Panel, DC, FALSE );
        Panel->ManageCurseur = NULL;
        Panel->ForceCloseManageCurseur = NULL;
        EraseStruct( (SCH_ITEM*) Screen->GetCurItem(), (SCH_SCREEN*) Screen );
        Screen->SetCurItem( NULL );
        RestoreOldWires( Screen );
    }
    else
        g_ItemToRepeat = NULL;  // Fin de commande generale

    /* Clear m_Flags wich is used in edit functions: */
    SCH_ITEM* item = Screen->EEDrawList;
    while( item )
    {
        item->m_Flags = 0;
        item = item->Next();
    }
}


/***************************************************/
void WinEDA_SchematicFrame::RepeatDrawItem( wxDC* DC )
/***************************************************/

/* Routine de recopie du dernier element dessine
 *  Les elements duplicables sont
 *      fils, bus, traits, textes, labels
 *      Les labels termines par un nombre seront incrementes
 */
{
    wxPoint new_pos;

    if( g_ItemToRepeat == NULL )
        return;

    switch( g_ItemToRepeat->Type() )
    {
    case DRAW_JUNCTION_STRUCT_TYPE:
        #undef STRUCT
        #define STRUCT ( (DrawJunctionStruct*) g_ItemToRepeat )
        g_ItemToRepeat = STRUCT->GenCopy();
        STRUCT->m_Pos += g_RepeatStep;
        new_pos = STRUCT->m_Pos;
        break;

    case DRAW_NOCONNECT_STRUCT_TYPE:
        #undef STRUCT
        #define STRUCT ( (DrawNoConnectStruct*) g_ItemToRepeat )
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
        /*** Increment du numero de label ***/
        IncrementLabelMember( STRUCT->m_Text );
        break;


    case TYPE_SCH_LABEL:
        #undef STRUCT
        #define STRUCT ( (SCH_LABEL*) g_ItemToRepeat )
        g_ItemToRepeat = STRUCT->GenCopy();
        STRUCT->m_Pos += g_RepeatStep;
        new_pos = STRUCT->m_Pos;
        /*** Increment du numero de label ***/
        IncrementLabelMember( STRUCT->m_Text );
        break;


    case TYPE_SCH_HIERLABEL:
        #undef STRUCT
        #define STRUCT ( (SCH_HIERLABEL*) g_ItemToRepeat )
        g_ItemToRepeat = STRUCT->GenCopy();
        STRUCT->m_Pos += g_RepeatStep;
        new_pos = STRUCT->m_Pos;
        /*** Increment du numero de label ***/
        IncrementLabelMember( STRUCT->m_Text );
        break;

    case TYPE_SCH_GLOBALLABEL:
        #undef STRUCT
        #define STRUCT ( (SCH_GLOBALLABEL*) g_ItemToRepeat )
        g_ItemToRepeat = STRUCT->GenCopy();
        STRUCT->m_Pos += g_RepeatStep;
        new_pos = STRUCT->m_Pos;
        /*** Increment du numero de label ***/
        IncrementLabelMember( STRUCT->m_Text );
        break;

    case DRAW_SEGMENT_STRUCT_TYPE:
        #undef STRUCT
        #define STRUCT ( (EDA_DrawLineStruct*) g_ItemToRepeat )
        g_ItemToRepeat   = STRUCT->GenCopy();
        STRUCT->m_Start += g_RepeatStep;
        new_pos = STRUCT->m_Start;
        STRUCT->m_End += g_RepeatStep;
        break;

    case DRAW_BUSENTRY_STRUCT_TYPE:
            #undef STRUCT
            #define STRUCT ( (DrawBusEntryStruct*) g_ItemToRepeat )
        g_ItemToRepeat = STRUCT->GenCopy();
        STRUCT->m_Pos += g_RepeatStep;
        new_pos = STRUCT->m_Pos;
        break;

    case TYPE_SCH_COMPONENT:     // In repeat command the new component is put in move mode
        #undef STRUCT
        #define STRUCT ( (SCH_COMPONENT*) g_ItemToRepeat )

        // Create the duplicate component, position = mouse cursor
        g_ItemToRepeat = STRUCT->GenCopy();
        new_pos.x           = GetScreen()->m_Curseur.x - STRUCT->m_Pos.x;
        new_pos.y           = GetScreen()->m_Curseur.y - STRUCT->m_Pos.y;
        STRUCT->m_Pos       = GetScreen()->m_Curseur;
        STRUCT->m_Flags     = IS_NEW;
        STRUCT->m_TimeStamp = GetTimeStamp();

        for( int ii = 0; ii < STRUCT->GetFieldCount(); ii++ )
        {
            STRUCT->GetField(ii)->m_Pos += new_pos;
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
        SaveCopyInUndoList( g_ItemToRepeat, IS_NEW );
        g_ItemToRepeat->m_Flags = 0;

//		GetScreen()->Curseur = new_pos;
//		GRMouseWarp(DrawPanel, DrawPanel->CursorScreenPosition() );
    }
}


/******************************************/
void IncrementLabelMember( wxString& name )
/******************************************/

/* Routine incrementant les labels, c'est a dire pour les textes finissant
 *  par un nombre, ajoutant <RepeatDeltaLabel> a ce nombre
 */
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


/***************************************************************************/
static bool IsTerminalPoint( SCH_SCREEN* screen, const wxPoint& pos, int layer )
/***************************************************************************/

/* Returne TRUE si pos est un point possible pour terminer automatiquement un
 *  segment, c'est a dire pour
 *  - type WIRE, si il y a
 *      - une jonction
 *      - ou une pin
 *      - ou une extr�mit� unique de fil
 *
 *  - type BUS, si il y a
 *      - ou une extr�mit� unique de BUS
 */
{
    EDA_BaseStruct*         item;
    LibDrawPin*             pin;
    SCH_COMPONENT* LibItem = NULL;
    Hierarchical_PIN_Sheet_Struct*   pinsheet;
    wxPoint itempos;

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
            // calcul de la position exacte du point de connexion de la pin,
            // selon orientation du composant:
            itempos    = LibItem->GetScreenCoord( pin->m_Pos );
            itempos.x += LibItem->m_Pos.x;
            itempos.y += LibItem->m_Pos.y;
            if( (itempos.x == pos.x) && (itempos.y == pos.y) )
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
            if( (itempos.x == pos.x) && (itempos.y == pos.y) )
                return TRUE;
        }

        break;

    default:
        break;
    }

    return FALSE;
}


/****************************************************************/
bool IsJunctionNeeded( WinEDA_SchematicFrame* frame, wxPoint& pos )
/****************************************************************/

/* Return True when a wire is located at pos "pos" if
 *  - there is no junction.
 *  - The wire has no ends at pos "pos",
 *      and therefore it is considered as no connected.
 *  - One (or more) wire has one end at pos "pos"
 *  or
 *  - a pin is on location pos
 */
{
    if( PickStruct( pos, frame->GetScreen(), JUNCTIONITEM ) )
        return FALSE;

    if( PickStruct( pos, frame->GetScreen(), WIREITEM | EXCLUDE_WIRE_BUS_ENDPOINTS ) )
    {
        if( PickStruct( pos, frame->GetScreen(), WIREITEM | WIRE_BUS_ENDPOINTS_ONLY ) )
            return TRUE;
        if( frame->LocatePinEnd( frame->GetScreen()->EEDrawList, pos ) )
            return TRUE;
    }

    return FALSE;
}
