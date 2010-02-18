/************************************/
/*           delete.cpp             */
/************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"

#include "program.h"
#include "general.h"
#include "protos.h"
#include "class_marker_sch.h"

// Imported function:
void DeleteItemsInList( WinEDA_DrawPanel*  panel,
                        PICKED_ITEMS_LIST& aItemsList );


/*
 * Count number of items connected to point pos :
 *     pins, end wire or bus, and junctions if TstJunction == TRUE
 * Return this count
 *
 * Used by WinEDA_SchematicFrame::DeleteConnection()
 */
static int CountConnectedItems( WinEDA_SchematicFrame* frame,
                                SCH_ITEM* ListStruct, wxPoint pos,
                                bool TstJunction )
{
    SCH_ITEM* Struct;
    int             count = 0;

    if( frame->LocatePinEnd( ListStruct, pos ) )
        count++;

    for( Struct = ListStruct; Struct != NULL; Struct = Struct->Next() )
    {
        if( Struct->m_Flags & STRUCT_DELETED )
            continue;
        if( Struct->m_Flags & SKIP_STRUCT )
            continue;


        if( TstJunction && ( Struct->Type() == DRAW_JUNCTION_STRUCT_TYPE ) )
        {
            #define JUNCTION ( (SCH_JUNCTION*) Struct )
            if( JUNCTION->m_Pos == pos )
                count++;
            #undef JUNCTION
        }

        if( Struct->Type() != DRAW_SEGMENT_STRUCT_TYPE )
            continue;

        #define SEGM ( (SCH_LINE*) Struct )
        if( SEGM->IsOneEndPointAt( pos ) )
            count++;
        #undef SEGM
    }

    return count;
}



/*
 * Mark to "CANDIDATE" all wires or junction connected to "segment" in list
 * "ListStruct"
 * Search wire stop at an any  pin
 *
 * Used by WinEDA_SchematicFrame::DeleteConnection()
 */
static bool MarkConnected( WinEDA_SchematicFrame* frame, SCH_ITEM* ListStruct,
                           SCH_LINE* segment )
{
    EDA_BaseStruct* Struct;

    for( Struct = ListStruct; Struct != NULL; Struct = Struct->Next() )
    {
        if( Struct->m_Flags )
            continue;
        if( Struct->Type() == DRAW_JUNCTION_STRUCT_TYPE )
        {
        #define JUNCTION ( (SCH_JUNCTION*) Struct )
            if( segment->IsOneEndPointAt( JUNCTION->m_Pos ) )
                Struct->m_Flags |= CANDIDATE;
            continue;
        #undef JUNCTION
        }

        if( Struct->Type() != DRAW_SEGMENT_STRUCT_TYPE )
            continue;

        #define SEGM ( (SCH_LINE*) Struct )
        if( segment->IsOneEndPointAt( SEGM->m_Start ) )
        {
            if( !frame->LocatePinEnd( ListStruct, SEGM->m_Start ) )
            {
                Struct->m_Flags |= CANDIDATE;
                MarkConnected( frame, ListStruct, SEGM );
            }
        }
        if( segment->IsOneEndPointAt( SEGM->m_End ) )
        {
            if( !frame->LocatePinEnd( ListStruct, SEGM->m_End ) )
            {
                Struct->m_Flags |= CANDIDATE;
                MarkConnected( frame, ListStruct, SEGM );
            }
        }
        #undef SEGM
    }

    return TRUE;
}


/*
 * Delete a connection, i.e wires or bus connected
 * stop on a node (more than 2 wires (bus) connected)
 */
void WinEDA_SchematicFrame::DeleteConnection( bool DeleteFullConnection )
{
    wxPoint refpos = GetScreen()->m_Curseur;
    SCH_ITEM* DelStruct;
    PICKED_ITEMS_LIST pickList;

    /* Clear .m_Flags member for all items */
    for( DelStruct = GetScreen()->EEDrawList; DelStruct != NULL;
         DelStruct = DelStruct->Next() )
        DelStruct->m_Flags = 0;

    BreakSegmentOnJunction( (SCH_SCREEN*) GetScreen() );

    /* Locate all the wires, bus or junction under the mouse cursor, and put
     * them in a list of items to delete
     */
    ITEM_PICKER picker(NULL, UR_DELETED);
    SCH_SCREEN* screen = (SCH_SCREEN*) GetScreen();
    // Save the list entry point of this screen
    SCH_ITEM* savedEEDrawList = screen->EEDrawList;
    DelStruct = GetScreen()->EEDrawList;
    while( DelStruct
           && ( DelStruct = PickStruct( screen->m_Curseur, screen,
                                        JUNCTIONITEM | WIREITEM | BUSITEM ) ) != NULL )
    {
        DelStruct->m_Flags = SELECTEDNODE | STRUCT_DELETED;

        /* Put this structure in the picked list: */
        picker.m_PickedItem = DelStruct;
        picker.m_PickedItemType = DelStruct->Type();
        pickList.PushItem(picker);

        DelStruct  = DelStruct->Next();
        screen->EEDrawList = DelStruct;
    }

    GetScreen()->EEDrawList = savedEEDrawList;

    /* Mark all wires, junctions, .. connected to one of the item to delete
     */
    if( DeleteFullConnection )
    {
        for( DelStruct = GetScreen()->EEDrawList; DelStruct != NULL;
             DelStruct = DelStruct->Next() )
        {
            if( !(DelStruct->m_Flags & SELECTEDNODE) )
                continue;

            #define SEGM ( (SCH_LINE*) DelStruct )
            if( DelStruct->Type() != DRAW_SEGMENT_STRUCT_TYPE )
                continue;

            MarkConnected( this, GetScreen()->EEDrawList, SEGM );
            #undef SEGM
        }

        // Search all removable wires (i.e wire with one new dangling end )
        for( DelStruct = GetScreen()->EEDrawList; DelStruct != NULL;
             DelStruct = DelStruct->Next() )
        {
            bool noconnect = FALSE;

            if( DelStruct->m_Flags & STRUCT_DELETED )
                continue;                                   // Already seen

            if( !(DelStruct->m_Flags & CANDIDATE) )
                continue;                                   // Already seen

            if( DelStruct->Type() != DRAW_SEGMENT_STRUCT_TYPE )
                continue;

            DelStruct->m_Flags |= SKIP_STRUCT;
            #define SEGM ( (SCH_LINE*) DelStruct )

            /* Test the SEGM->m_Start point: if this point was connected to
             * an STRUCT_DELETED wire, and now is not connected, the wire can
             * be deleted */
            EDA_BaseStruct* removed_struct;
            for( removed_struct = GetScreen()->EEDrawList;
                 removed_struct != NULL;
                 removed_struct = removed_struct->Next() )
            {
                if( ( removed_struct->m_Flags & STRUCT_DELETED ) == 0 )
                    continue;

                if( removed_struct->Type() != DRAW_SEGMENT_STRUCT_TYPE )
                    continue;

                #define WIRE ( (SCH_LINE*) removed_struct )
                if( WIRE->IsOneEndPointAt( SEGM->m_Start ) )
                    break;
            }

            if( WIRE && !CountConnectedItems( this, GetScreen()->EEDrawList,
                                              SEGM->m_Start, TRUE ) )
                noconnect = TRUE;

            /* Test the SEGM->m_End point: if this point was connected to
             * an STRUCT_DELETED wire, and now is not connected, the wire
             * can be deleted */
            for( removed_struct = GetScreen()->EEDrawList;
                 removed_struct != NULL;
                 removed_struct = removed_struct->Next() )
            {
                if( ( removed_struct->m_Flags & STRUCT_DELETED ) == 0 )
                    continue;
                if( removed_struct->Type() != DRAW_SEGMENT_STRUCT_TYPE )
                    continue;
                if( WIRE->IsOneEndPointAt( SEGM->m_End ) )
                    break;
            }

            if( removed_struct &&
               !CountConnectedItems( this, GetScreen()->EEDrawList,
                                     SEGM->m_End, TRUE ) )
                noconnect = TRUE;

            DelStruct->m_Flags &= ~SKIP_STRUCT;

            if( noconnect )
            {
                DelStruct->m_Flags |= STRUCT_DELETED;
                /* Put this structure in the picked list: */
                picker.m_PickedItem = DelStruct;
                picker.m_PickedItemType = DelStruct->Type();
                pickList.PushItem(picker);

                DelStruct  = GetScreen()->EEDrawList;
            }
            #undef SEGM
        }

        // Delete redundant junctions (junctions which connect < 3 end wires
        // and no pin are removed)
        for( DelStruct = GetScreen()->EEDrawList; DelStruct != NULL;
             DelStruct = DelStruct->Next() )
        {
            int count;
            if( DelStruct->m_Flags & STRUCT_DELETED )
                continue;

            if( !(DelStruct->m_Flags & CANDIDATE) )
                continue;

            if( DelStruct->Type() == DRAW_JUNCTION_STRUCT_TYPE )
            {
                #define JUNCTION ( (SCH_JUNCTION*) DelStruct )
                count = CountConnectedItems( this, GetScreen()->EEDrawList,
                                             JUNCTION->m_Pos, FALSE );
                if( count <= 2 )
                {
                    DelStruct->m_Flags |= STRUCT_DELETED;

                    /* Put this structure in the picked list: */
                    picker.m_PickedItem = DelStruct;
                    picker.m_PickedItemType = DelStruct->Type();
                    pickList.PushItem(picker);
                }
                #undef JUNCTION
            }
        }

        // Delete labels attached to wires
        wxPoint pos = GetScreen()->m_Curseur;
        for( DelStruct = GetScreen()->EEDrawList; DelStruct != NULL;
             DelStruct = DelStruct->Next() )
        {
            if( DelStruct->m_Flags & STRUCT_DELETED )
                continue;

            if( DelStruct->Type() != TYPE_SCH_LABEL )
                continue;

            GetScreen()->m_Curseur = ( (SCH_TEXT*) DelStruct )->m_Pos;
            EDA_BaseStruct* TstStruct =
                PickStruct( GetScreen()->m_Curseur, GetScreen(),
                            WIREITEM | BUSITEM );

            if( TstStruct && TstStruct->m_Flags & STRUCT_DELETED )
            {
                DelStruct->m_Flags |= STRUCT_DELETED;

                /* Put this structure in the picked list: */
                picker.m_PickedItem = DelStruct;
                picker.m_PickedItemType = DelStruct->Type();
                pickList.PushItem(picker);
            }
        }

        GetScreen()->m_Curseur = pos;
    }

    for( DelStruct = GetScreen()->EEDrawList; DelStruct != NULL;
         DelStruct = DelStruct->Next() )
        DelStruct->m_Flags = 0;

    if( pickList.GetCount() )
    {
        DeleteItemsInList( DrawPanel, pickList );
        OnModify( );
    }
}


/*
 * Locate and delete the item found under the mouse cursor
 * If more than one item found: the priority order is:
 *  1 : MARKER
 *  2 : JUNCTION
 *  2 : NOCONNECT
 *  3 : WIRE or BUS
 *  4 : DRAWITEM
 *  5 : TEXT
 *  6 : COMPOSANT
 *  7 : SHEET
 *
 * return TRUE if an item was deleted
 */
bool LocateAndDeleteItem( WinEDA_SchematicFrame* frame, wxDC* DC )
{
    SCH_ITEM* DelStruct;
    SCH_SCREEN* screen = (SCH_SCREEN*) ( frame->GetScreen() );
    bool item_deleted  = FALSE;

    DelStruct = PickStruct( screen->m_Curseur, screen, MARKERITEM );
    if( DelStruct == NULL )
        DelStruct = PickStruct( screen->m_Curseur, screen, JUNCTIONITEM );
    if( DelStruct == NULL )
        DelStruct = PickStruct( screen->m_Curseur, screen, NOCONNECTITEM );
    if( DelStruct == NULL )
        DelStruct = PickStruct( screen->m_Curseur, screen, RACCORDITEM );
    if( DelStruct == NULL )
        DelStruct = PickStruct( screen->m_Curseur, screen, WIREITEM | BUSITEM );
    if( DelStruct == NULL )
        DelStruct = PickStruct( screen->m_Curseur, screen, DRAWITEM );
    if( DelStruct == NULL )
        DelStruct = PickStruct( screen->m_Curseur, screen,
                                TEXTITEM | LABELITEM );
    if( DelStruct == NULL )
        DelStruct = PickStruct( screen->m_Curseur, screen, LIBITEM );
    if( DelStruct == NULL )
        DelStruct = PickStruct( screen->m_Curseur, screen, SHEETITEM );

    if( DelStruct )
    {
        g_ItemToRepeat = NULL;
        DeleteStruct( frame->DrawPanel, DC, DelStruct );
        frame->TestDanglingEnds( frame->GetScreen()->EEDrawList, DC );
        frame->OnModify( );
        item_deleted = TRUE;
    }

    return item_deleted;
}


/*
 * Remove definition of a structure in a linked list
 * Elements of Drawing
 *   DrawStruct * = pointer to the structure
 *   Screen = pointer on the screen of belonging
 *
 * Note:
 * DRAW_SHEET_STRUCT_TYPE structures for the screen and structures
 * Corresponding keys are not.
 * They must be treated separately
 */
void EraseStruct( SCH_ITEM* DrawStruct, SCH_SCREEN* Screen )
{
    EDA_BaseStruct* DrawList;
    SCH_SHEET_PIN* SheetLabel, * NextLabel;

    if( DrawStruct == NULL )
        return;

    if( Screen == NULL )
        return;

    Screen->SetModify();

    if( DrawStruct->Type() == DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE )
    {
        //this structure is attached to a sheet , which we must find.
        DrawList = Screen->EEDrawList;
        for( ; DrawList != NULL; DrawList = DrawList->Next() )
        {
            if( DrawList->Type() != DRAW_SHEET_STRUCT_TYPE )
                continue;

            /* See if our item is in this Sheet */
            SheetLabel = ( (SCH_SHEET*) DrawList )->m_Label;
            if( SheetLabel == NULL )
                continue;

            if( SheetLabel == (SCH_SHEET_PIN*) DrawStruct )
            {
                ( (SCH_SHEET*) DrawList )->m_Label =
                    (SCH_SHEET_PIN*) SheetLabel->Next();

                SAFE_DELETE( DrawStruct );
                return;
            }
            else
            {
                while( SheetLabel->Next() )
                {
                    NextLabel = (SCH_SHEET_PIN*) SheetLabel->Next();

                    if( NextLabel == (SCH_SHEET_PIN*) DrawStruct )
                    {
                        SheetLabel->SetNext( (EDA_BaseStruct*) NextLabel->Next() );
                        SAFE_DELETE( DrawStruct );
                        return;

                    }
                    SheetLabel = NextLabel;
                }
            }
        }

        return;
    }
    else
    {
        if( DrawStruct == Screen->EEDrawList )
        {
            Screen->EEDrawList = DrawStruct->Next();
            SAFE_DELETE( DrawStruct );
        }
        else
        {
            DrawList = Screen->EEDrawList;
            while( DrawList && DrawList->Next() )
            {
                if( DrawList->Next() == DrawStruct )
                {
                    DrawList->SetNext( DrawStruct->Next() );
                    SAFE_DELETE( DrawStruct );
                    return;
                }
                DrawList = DrawList->Next();
            }
        }
    }
}


void DeleteAllMarkers( int type )
{
    SCH_SCREEN* screen;
    SCH_ITEM * DrawStruct, * NextStruct;
    SCH_MARKER* Marker;

    EDA_ScreenList ScreenList;

    for( screen = ScreenList.GetFirst(); screen != NULL;
         screen = ScreenList.GetNext() )
    {
        for( DrawStruct = screen->EEDrawList; DrawStruct != NULL;
             DrawStruct = NextStruct )
        {
            NextStruct = DrawStruct->Next();
            if( DrawStruct->Type() != TYPE_SCH_MARKER )
                continue;

            Marker = (SCH_MARKER*) DrawStruct;
            if( Marker->GetMarkerType() != type )
                continue;

            /* Remove marker */
            EraseStruct( DrawStruct, screen );
        }
    }
}
