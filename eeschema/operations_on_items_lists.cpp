/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2014 KiCad Developers, see CHANGELOG.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file operations_on_items_lists.cpp
 * @brief Functions used in block commands, or undo/redo, to move, mirror, delete, copy ...
 *        lists of schematic items.
 */

#include <fctsys.h>
#include <pgm_base.h>
#include <class_drawpanel.h>
#include <schframe.h>

#include <general.h>
#include <protos.h>
#include <sch_bus_entry.h>
#include <sch_marker.h>
#include <sch_line.h>
#include <sch_no_connect.h>
#include <sch_sheet.h>
#include <sch_component.h>
#include <sch_junction.h>


void SetSchItemParent( SCH_ITEM* Struct, SCH_SCREEN* Screen )
{
    switch( Struct->Type() )
    {
    case SCH_JUNCTION_T:
    case SCH_TEXT_T:
    case SCH_LABEL_T:
    case SCH_GLOBAL_LABEL_T:
    case SCH_HIERARCHICAL_LABEL_T:
    case SCH_COMPONENT_T:
    case SCH_LINE_T:
    case SCH_BUS_BUS_ENTRY_T:
    case SCH_BUS_WIRE_ENTRY_T:
    case SCH_SHEET_T:
    case SCH_MARKER_T:
    case SCH_NO_CONNECT_T:
        Struct->SetParent( Screen );
        break;

    case SCH_SHEET_PIN_T:
        break;

    default:
        break;
    }
}


void RotateListOfItems( PICKED_ITEMS_LIST& aItemsList, wxPoint& rotationPoint )
{
    for( unsigned ii = 0; ii < aItemsList.GetCount(); ii++ )
    {
        SCH_ITEM* item = (SCH_ITEM*) aItemsList.GetPickedItem( ii );
        item->Rotate( rotationPoint );      // Place it in its new position.
        item->ClearFlags();
    }
}


void MirrorY( PICKED_ITEMS_LIST& aItemsList, wxPoint& aMirrorPoint )
{
    for( unsigned ii = 0; ii < aItemsList.GetCount(); ii++ )
    {
        SCH_ITEM* item = (SCH_ITEM*) aItemsList.GetPickedItem( ii );
        item->MirrorY( aMirrorPoint.x );      // Place it in its new position.
        item->ClearFlags();
    }
}


void MirrorX( PICKED_ITEMS_LIST& aItemsList, wxPoint& aMirrorPoint )
{
    for( unsigned ii = 0; ii < aItemsList.GetCount(); ii++ )
    {
        SCH_ITEM* item = (SCH_ITEM*) aItemsList.GetPickedItem( ii );
        item->MirrorX( aMirrorPoint.y );      // Place it in its new position.
        item->ClearFlags();
    }
}


/**
 * Function MoveItemsInList
 *  Move a list of items to a given move vector
 * @param aItemsList = list of picked items
 * @param aMoveVector = the move vector value
 */
void MoveItemsInList( PICKED_ITEMS_LIST& aItemsList, const wxPoint aMoveVector )
{
    for( unsigned ii = 0; ii < aItemsList.GetCount(); ii++ )
    {
        SCH_ITEM* item = (SCH_ITEM*) aItemsList.GetPickedItem( ii );
        item->Move( aMoveVector );
    }
}


/**
 * Function DeleteItemsInList
 * delete schematic items in aItemsList
 * deleted items are put in undo list
 */
void DeleteItemsInList( EDA_DRAW_PANEL* panel, PICKED_ITEMS_LIST& aItemsList )
{
    SCH_SCREEN*        screen = (SCH_SCREEN*) panel->GetScreen();
    SCH_EDIT_FRAME*    frame  = (SCH_EDIT_FRAME*) panel->GetParent();
    PICKED_ITEMS_LIST  itemsList;

    for( unsigned ii = 0; ii < aItemsList.GetCount(); ii++ )
    {
        SCH_ITEM* item = (SCH_ITEM*) aItemsList.GetPickedItem( ii );
        ITEM_PICKER itemWrapper( item, UR_DELETED );

        if( item->Type() == SCH_SHEET_PIN_T )
        {
            /* this item is depending on a sheet, and is not in global list */
            wxMessageBox( wxT( "DeleteItemsInList() err: unexpected SCH_SHEET_PIN_T" ) );
        }
        else
        {
            screen->Remove( item );

            /* Unlink the structure */
            itemsList.PushItem( itemWrapper );
        }
    }

    frame->SaveCopyInUndoList( itemsList, UR_DELETED );
}


void SCH_EDIT_FRAME::DeleteItem( SCH_ITEM* aItem )
{
    wxCHECK_RET( aItem != NULL, wxT( "Cannot delete invalid item." ) );

    if( aItem == NULL )
        return;

    SCH_SCREEN* screen = GetScreen();

    if( aItem->Type() == SCH_SHEET_PIN_T )
    {
        // This iten is attached to a node, and is not accessible by the global list directly.
        SCH_SHEET* sheet = (SCH_SHEET*) aItem->GetParent();
        wxCHECK_RET( (sheet != NULL) && (sheet->Type() == SCH_SHEET_T),
                     wxT( "Sheet label has invalid parent item." ) );
        SaveCopyInUndoList( (SCH_ITEM*) sheet, UR_CHANGED );
        sheet->RemovePin( (SCH_SHEET_PIN*) aItem );
        m_canvas->RefreshDrawingRect( sheet->GetBoundingBox() );
    }
    else
    {
        screen->Remove( aItem );
        SaveCopyInUndoList( aItem, UR_DELETED );
        m_canvas->RefreshDrawingRect( aItem->GetBoundingBox() );
    }
}


/* Routine to copy a new entity of an object for each object in list and
 * reposition it.
 * Return the new created object list in aItemsList
 */
void DuplicateItemsInList( SCH_SCREEN* screen, PICKED_ITEMS_LIST& aItemsList,
                           const wxPoint aMoveVector )
{
    SCH_ITEM* newitem;

    if( aItemsList.GetCount() == 0 )
        return;

    for( unsigned ii = 0; ii < aItemsList.GetCount(); ii++ )
    {
        newitem = DuplicateStruct( (SCH_ITEM*) aItemsList.GetPickedItem( ii ) );
        aItemsList.SetPickedItem( newitem, ii );
        aItemsList.SetPickedItemStatus( UR_NEW, ii );
        {
            switch( newitem->Type() )
            {
            case SCH_JUNCTION_T:
            case SCH_LINE_T:
            case SCH_BUS_BUS_ENTRY_T:
            case SCH_BUS_WIRE_ENTRY_T:
            case SCH_TEXT_T:
            case SCH_LABEL_T:
            case SCH_GLOBAL_LABEL_T:
            case SCH_HIERARCHICAL_LABEL_T:
            case SCH_SHEET_PIN_T:
            case SCH_MARKER_T:
            case SCH_NO_CONNECT_T:
            default:
                break;

            case SCH_SHEET_T:
            {
                SCH_SHEET* sheet = (SCH_SHEET*) newitem;
                sheet->SetTimeStamp( GetNewTimeStamp() );
                break;
            }

            case SCH_COMPONENT_T:
                ( (SCH_COMPONENT*) newitem )->SetTimeStamp( GetNewTimeStamp() );
                ( (SCH_COMPONENT*) newitem )->ClearAnnotation( NULL );
                break;
            }

            SetSchItemParent( newitem, screen );
            screen->Append( newitem );
        }
    }

    MoveItemsInList( aItemsList, aMoveVector );
}


/**
 * Function DuplicateStruct
 *  Routine to create a new copy of given struct.
 *  The new object is not put in draw list (not linked)
 * @param aDrawStruct = the SCH_ITEM to duplicate
 * @param aClone (default = false)
 *     if true duplicate also some parameters that must be unique
 *     (timestamp and sheet name)
 *      aClone must be false. use true only is undo/redo duplications
 */
SCH_ITEM* DuplicateStruct( SCH_ITEM* aDrawStruct, bool aClone )
{
    wxCHECK_MSG( aDrawStruct != NULL, NULL,
                 wxT( "Cannot duplicate NULL schematic item!  Bad programmer." ) );

    SCH_ITEM* NewDrawStruct = (SCH_ITEM*) aDrawStruct->Clone();

    if( aClone )
        NewDrawStruct->SetTimeStamp( aDrawStruct->GetTimeStamp() );

    NewDrawStruct->SetImage( aDrawStruct );

    return NewDrawStruct;
}
