/************************************/
/*           delete.cpp             */
/************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_sch_screen.h"
#include "wxEeschemaStruct.h"

#include "general.h"
#include "protos.h"
#include "sch_marker.h"
#include "sch_junction.h"
#include "sch_line.h"
#include "sch_sheet.h"
#include "sch_text.h"


/*
 * Delete a connection, i.e wires or bus connected
 * stop on a node (more than 2 wires (bus) connected)
 */
void SCH_EDIT_FRAME::DeleteConnection( bool DeleteFullConnection )
{
    SCH_ITEM* item;
    EDA_ITEM* tmp;
    PICKED_ITEMS_LIST pickList;
    SCH_SCREEN* screen = GetScreen();
    wxPoint pos = screen->GetCrossHairPosition();

    // Clear flags member for all items.
    screen->ClearDrawingState();
    screen->BreakSegmentsOnJunctions();

    // Save the list entry point of this screen
    SCH_ITEM* savedItems = screen->GetDrawItems();
    item = screen->GetDrawItems();

    while( item && ( item = screen->GetItem( pos, 0, JUNCTION_T | WIRE_T | BUS_T ) ) != NULL )
    {
        item->SetFlags( SELECTEDNODE | STRUCT_DELETED );

        /* Put this structure in the picked list: */
        ITEM_PICKER picker( item, UR_DELETED );
        pickList.PushItem( picker );

        item = item->Next();
        screen->SetDrawItems( item );
    }

    screen->SetDrawItems( savedItems ); // Restore the list entry point.

    /* Mark all wires, junctions, .. connected to one of the item to delete
     */
    if( DeleteFullConnection )
    {
        SCH_LINE* segment;

        for( item = screen->GetDrawItems(); item != NULL; item = item->Next() )
        {
            if( !(item->GetFlags() & SELECTEDNODE) )
                continue;

            if( item->Type() != SCH_LINE_T )
                continue;

            screen->MarkConnections( (SCH_LINE*) item );
        }

        // Search all removable wires (i.e wire with one new dangling end )
        for( item = screen->GetDrawItems(); item != NULL; item = item->Next() )
        {
            bool noconnect = false;

            if( item->GetFlags() & STRUCT_DELETED )
                continue;                                   // Already seen

            if( !(item->GetFlags() & CANDIDATE) )
                continue;                                   // Already seen

            if( item->Type() != SCH_LINE_T )
                continue;

            item->SetFlags( SKIP_STRUCT );

            segment = (SCH_LINE*) item;

            /* Test the SEGM->m_Start point: if this point was connected to
             * an STRUCT_DELETED wire, and now is not connected, the wire can
             * be deleted */
            SCH_LINE* testSegment = NULL;

            for( tmp = screen->GetDrawItems(); tmp != NULL; tmp = tmp->Next() )
            {
                if( ( tmp->GetFlags() & STRUCT_DELETED ) == 0 )
                    continue;

                if( tmp->Type() != SCH_LINE_T )
                    continue;

                testSegment = (SCH_LINE*) tmp;

                if( testSegment->IsEndPoint( segment->m_Start ) )
                    break;
            }

            if( testSegment && !screen->CountConnectedItems( segment->m_Start, true ) )
                noconnect = true;

            /* Test the SEGM->m_End point: if this point was connected to
             * an STRUCT_DELETED wire, and now is not connected, the wire
             * can be deleted */
            for( tmp = screen->GetDrawItems(); tmp != NULL; tmp = tmp->Next() )
            {
                if( ( tmp->GetFlags() & STRUCT_DELETED ) == 0 )
                    continue;

                if( tmp->Type() != SCH_LINE_T )
                    continue;

                if( testSegment->IsEndPoint( segment->m_End ) )
                    break;
            }

            if( tmp && !screen->CountConnectedItems( segment->m_End, true ) )
                noconnect = true;

            item->ClearFlags( SKIP_STRUCT );

            if( noconnect )
            {
                item->SetFlags( STRUCT_DELETED );
                /* Put this structure in the picked list: */
                ITEM_PICKER picker( item, UR_DELETED );
                pickList.PushItem( picker );

                item = screen->GetDrawItems();
            }
        }

        // Delete redundant junctions (junctions which connect < 3 end wires
        // and no pin are removed)
        for( item = screen->GetDrawItems(); item != NULL; item = item->Next() )
        {
            if( item->GetFlags() & STRUCT_DELETED )
                continue;

            if( !(item->GetFlags() & CANDIDATE) )
                continue;

            if( item->Type() != SCH_JUNCTION_T )
                continue;

            SCH_JUNCTION* junction = (SCH_JUNCTION*) item;

            if( screen->CountConnectedItems( junction->m_Pos, false ) <= 2 )
            {
                item->SetFlags( STRUCT_DELETED );

                /* Put this structure in the picked list: */
                ITEM_PICKER picker( item, UR_DELETED );
                pickList.PushItem( picker );
            }
        }

        for( item = screen->GetDrawItems(); item != NULL;  item = item->Next() )
        {
            if( item->GetFlags() & STRUCT_DELETED )
                continue;

            if( item->Type() != SCH_LABEL_T )
                continue;

            tmp = screen->GetItem( ( (SCH_TEXT*) item )->m_Pos, 0, WIRE_T | BUS_T );

            if( tmp && tmp->GetFlags() & STRUCT_DELETED )
            {
                item->SetFlags( STRUCT_DELETED );

                /* Put this structure in the picked list: */
                ITEM_PICKER picker( item, UR_DELETED );
                pickList.PushItem( picker );
            }
        }
    }

    screen->ClearDrawingState();

    if( pickList.GetCount() )
    {
        DeleteItemsInList( DrawPanel, pickList );
        OnModify();
    }
}


bool SCH_EDIT_FRAME::DeleteItemAtCrossHair( wxDC* DC )
{
    SCH_ITEM* item;
    SCH_SCREEN* screen = GetScreen();
    bool item_deleted  = false;

    item = screen->GetItem( screen->GetCrossHairPosition(), 0, MARKER_T );

    if( item == NULL )
        item = screen->GetItem( screen->GetCrossHairPosition(), 0, JUNCTION_T );

    if( item == NULL )
        item = screen->GetItem( screen->GetCrossHairPosition(), 0, NO_CONNECT_T );

    if( item == NULL )
        item = screen->GetItem( screen->GetCrossHairPosition(), 0, BUS_ENTRY_T );

    if( item == NULL )
        item = screen->GetItem( screen->GetCrossHairPosition(), 0, WIRE_T | BUS_T );

    if( item == NULL )
        item = screen->GetItem( screen->GetCrossHairPosition(), 0, DRAW_ITEM_T );

    if( item == NULL )
        item = screen->GetItem( screen->GetCrossHairPosition(), 0, TEXT_T | LABEL_T );

    if( item == NULL )
        item = screen->GetItem( screen->GetCrossHairPosition(), 0, COMPONENT_T );

    if( item == NULL )
        item = screen->GetItem( screen->GetCrossHairPosition(), 0, SHEET_T );

    if( item )
    {
        SetRepeatItem( NULL );
        DeleteItem( item );
        screen->TestDanglingEnds( DrawPanel, DC );
        OnModify();
        item_deleted = true;
    }

    return item_deleted;
}
