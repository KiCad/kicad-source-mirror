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
#include "sch_items.h"
#include "sch_sheet.h"
#include "sch_text.h"


// Imported function:
void DeleteItemsInList( WinEDA_DrawPanel* panel, PICKED_ITEMS_LIST& aItemsList );


/*
 * Count number of items connected to point pos :
 *     pins, end wire or bus, and junctions if TstJunction == TRUE
 * Return this count
 *
 * Used by SCH_EDIT_FRAME::DeleteConnection()
 */
static int CountConnectedItems( SCH_EDIT_FRAME* frame, SCH_ITEM* ListStruct, wxPoint pos,
                                bool TstJunction )
{
    SCH_ITEM* Struct;
    int       count = 0;

    if( frame->LocatePinEnd( ListStruct, pos ) )
        count++;

    for( Struct = ListStruct; Struct != NULL; Struct = Struct->Next() )
    {
        if( Struct->m_Flags & STRUCT_DELETED )
            continue;
        if( Struct->m_Flags & SKIP_STRUCT )
            continue;


        if( TstJunction && ( Struct->Type() == SCH_JUNCTION_T ) )
        {
            #define JUNCTION ( (SCH_JUNCTION*) Struct )
            if( JUNCTION->m_Pos == pos )
                count++;
            #undef JUNCTION
        }

        if( Struct->Type() != SCH_LINE_T )
            continue;

        #define SEGM ( (SCH_LINE*) Struct )
        if( SEGM->IsEndPoint( pos ) )
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
 * Used by SCH_EDIT_FRAME::DeleteConnection()
 */
static bool MarkConnected( SCH_EDIT_FRAME* frame, SCH_ITEM* ListStruct, SCH_LINE* segment )
{
    EDA_ITEM* Struct;

    for( Struct = ListStruct; Struct != NULL; Struct = Struct->Next() )
    {
        if( Struct->m_Flags )
            continue;
        if( Struct->Type() == SCH_JUNCTION_T )
        {
        #define JUNCTION ( (SCH_JUNCTION*) Struct )
            if( segment->IsEndPoint( JUNCTION->m_Pos ) )
                Struct->m_Flags |= CANDIDATE;
            continue;
        #undef JUNCTION
        }

        if( Struct->Type() != SCH_LINE_T )
            continue;

        #define SEGM ( (SCH_LINE*) Struct )
        if( segment->IsEndPoint( SEGM->m_Start ) )
        {
            if( !frame->LocatePinEnd( ListStruct, SEGM->m_Start ) )
            {
                Struct->m_Flags |= CANDIDATE;
                MarkConnected( frame, ListStruct, SEGM );
            }
        }
        if( segment->IsEndPoint( SEGM->m_End ) )
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
void SCH_EDIT_FRAME::DeleteConnection( bool DeleteFullConnection )
{
    wxPoint refpos = GetScreen()->m_Curseur;
    SCH_ITEM* DelStruct;
    PICKED_ITEMS_LIST pickList;

    /* Clear .m_Flags member for all items */
    for( DelStruct = GetScreen()->GetDrawItems(); DelStruct != NULL; DelStruct = DelStruct->Next() )
        DelStruct->m_Flags = 0;

    BreakSegmentOnJunction( GetScreen() );

    /* Locate all the wires, bus or junction under the mouse cursor, and put
     * them in a list of items to delete
     */
    ITEM_PICKER picker(NULL, UR_DELETED);
    SCH_SCREEN* screen = GetScreen();
    // Save the list entry point of this screen
    SCH_ITEM* savedItems = screen->GetDrawItems();
    DelStruct = GetScreen()->GetDrawItems();

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
        screen->SetDrawItems( DelStruct );
    }

    GetScreen()->SetDrawItems( savedItems );

    /* Mark all wires, junctions, .. connected to one of the item to delete
     */
    if( DeleteFullConnection )
    {
        for( DelStruct = GetScreen()->GetDrawItems(); DelStruct != NULL;
             DelStruct = DelStruct->Next() )
        {
            if( !(DelStruct->m_Flags & SELECTEDNODE) )
                continue;

            #define SEGM ( (SCH_LINE*) DelStruct )

            if( DelStruct->Type() != SCH_LINE_T )
                continue;

            MarkConnected( this, GetScreen()->GetDrawItems(), SEGM );
            #undef SEGM
        }

        // Search all removable wires (i.e wire with one new dangling end )
        for( DelStruct = GetScreen()->GetDrawItems(); DelStruct != NULL;
             DelStruct = DelStruct->Next() )
        {
            bool noconnect = FALSE;

            if( DelStruct->m_Flags & STRUCT_DELETED )
                continue;                                   // Already seen

            if( !(DelStruct->m_Flags & CANDIDATE) )
                continue;                                   // Already seen

            if( DelStruct->Type() != SCH_LINE_T )
                continue;

            DelStruct->m_Flags |= SKIP_STRUCT;
            #define SEGM ( (SCH_LINE*) DelStruct )

            /* Test the SEGM->m_Start point: if this point was connected to
             * an STRUCT_DELETED wire, and now is not connected, the wire can
             * be deleted */
            EDA_ITEM* removed_struct;
            for( removed_struct = GetScreen()->GetDrawItems();
                 removed_struct != NULL;
                 removed_struct = removed_struct->Next() )
            {
                if( ( removed_struct->m_Flags & STRUCT_DELETED ) == 0 )
                    continue;

                if( removed_struct->Type() != SCH_LINE_T )
                    continue;

                #define WIRE ( (SCH_LINE*) removed_struct )
                if( WIRE->IsEndPoint( SEGM->m_Start ) )
                    break;
            }

            if( WIRE && !CountConnectedItems( this, GetScreen()->GetDrawItems(),
                                              SEGM->m_Start, TRUE ) )
                noconnect = TRUE;

            /* Test the SEGM->m_End point: if this point was connected to
             * an STRUCT_DELETED wire, and now is not connected, the wire
             * can be deleted */
            for( removed_struct = GetScreen()->GetDrawItems();
                 removed_struct != NULL;
                 removed_struct = removed_struct->Next() )
            {
                if( ( removed_struct->m_Flags & STRUCT_DELETED ) == 0 )
                    continue;
                if( removed_struct->Type() != SCH_LINE_T )
                    continue;
                if( WIRE->IsEndPoint( SEGM->m_End ) )
                    break;
            }

            if( removed_struct &&
               !CountConnectedItems( this, GetScreen()->GetDrawItems(), SEGM->m_End, TRUE ) )
                noconnect = TRUE;

            DelStruct->m_Flags &= ~SKIP_STRUCT;

            if( noconnect )
            {
                DelStruct->m_Flags |= STRUCT_DELETED;
                /* Put this structure in the picked list: */
                picker.m_PickedItem = DelStruct;
                picker.m_PickedItemType = DelStruct->Type();
                pickList.PushItem(picker);

                DelStruct  = GetScreen()->GetDrawItems();
            }
            #undef SEGM
        }

        // Delete redundant junctions (junctions which connect < 3 end wires
        // and no pin are removed)
        for( DelStruct = GetScreen()->GetDrawItems(); DelStruct != NULL;
             DelStruct = DelStruct->Next() )
        {
            int count;
            if( DelStruct->m_Flags & STRUCT_DELETED )
                continue;

            if( !(DelStruct->m_Flags & CANDIDATE) )
                continue;

            if( DelStruct->Type() == SCH_JUNCTION_T )
            {
                #define JUNCTION ( (SCH_JUNCTION*) DelStruct )
                count = CountConnectedItems( this, GetScreen()->GetDrawItems(),
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
        for( DelStruct = GetScreen()->GetDrawItems(); DelStruct != NULL;
             DelStruct = DelStruct->Next() )
        {
            if( DelStruct->m_Flags & STRUCT_DELETED )
                continue;

            if( DelStruct->Type() != SCH_LABEL_T )
                continue;

            GetScreen()->m_Curseur = ( (SCH_TEXT*) DelStruct )->m_Pos;
            EDA_ITEM* TstStruct = PickStruct( GetScreen()->m_Curseur, GetScreen(),
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

    for( DelStruct = GetScreen()->GetDrawItems(); DelStruct != NULL;
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
bool LocateAndDeleteItem( SCH_EDIT_FRAME* frame, wxDC* DC )
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
        frame->TestDanglingEnds( frame->GetScreen()->GetDrawItems(), DC );
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
 * SCH_SHEET_T structures for the screen and structures
 * Corresponding keys are not.
 * They must be treated separately
 */
void EraseStruct( SCH_ITEM* DrawStruct, SCH_SCREEN* Screen )
{
    EDA_ITEM* DrawList;

    if( DrawStruct == NULL )
        return;

    if( Screen == NULL )
        return;

    Screen->SetModify();

    if( DrawStruct->Type() == SCH_SHEET_LABEL_T )
    {
        // This structure is attached to a sheet, get the parent sheet object.
        SCH_SHEET_PIN* sheetLabel = (SCH_SHEET_PIN*) DrawStruct;
        SCH_SHEET* sheet = sheetLabel->GetParent();
        wxASSERT_MSG( sheet != NULL,
                      wxT( "Sheet label parent not properly set, bad programmer!" ) );
        sheet->RemoveLabel( sheetLabel );
        return;
    }
    else
    {
        if( DrawStruct == Screen->GetDrawItems() )
        {
            Screen->SetDrawItems( DrawStruct->Next() );
            SAFE_DELETE( DrawStruct );
        }
        else
        {
            DrawList = Screen->GetDrawItems();

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
    SCH_SCREENS ScreenList;

    for( screen = ScreenList.GetFirst(); screen != NULL; screen = ScreenList.GetNext() )
    {
        for( DrawStruct = screen->GetDrawItems(); DrawStruct != NULL; DrawStruct = NextStruct )
        {
            NextStruct = DrawStruct->Next();

            if( DrawStruct->Type() != SCH_MARKER_T )
                continue;

            Marker = (SCH_MARKER*) DrawStruct;

            if( Marker->GetMarkerType() != type )
                continue;

            /* Remove marker */
            EraseStruct( DrawStruct, screen );
        }
    }
}
