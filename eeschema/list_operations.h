/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#ifndef LIST_OPERATIONS_H
#define LIST_OPERATIONS_H

namespace KIGFX {
class COLOR4D;
}

class wxPoint;
class wxDC;
class EDA_DRAW_PANEL;
class SCH_ITEM;
class SCH_SCREEN;
class PICKED_ITEMS_LIST;

void SetSchItemParent( SCH_ITEM* Struct, SCH_SCREEN* Screen );
void RotateListOfItems( PICKED_ITEMS_LIST& aItemsList, const wxPoint& rotationPoint );
void MirrorY( PICKED_ITEMS_LIST& aItemsList, const wxPoint& aMirrorPoint );
void MirrorX( PICKED_ITEMS_LIST& aItemsList, const wxPoint& aMirrorPoint );

/**
 * Function MoveItemsInList
 *  Move a list of items to a given move vector
 * @param aItemsList = list of picked items
 * @param aMoveVector = the move vector value
 */
void MoveItemsInList( PICKED_ITEMS_LIST& aItemsList, const wxPoint& aMoveVector );

/**
 * Function DeleteItemsInList
 * delete schematic items in aItemsList
 * deleted items are put in undo list
 */
void DeleteItemsInList( EDA_DRAW_PANEL* panel, PICKED_ITEMS_LIST& aItemsList );

/**
 * Routine to copy a new entity of an object for each object in list and
 * reposition it.
 * Return the new created object list in aItemsList
 */
void DuplicateItemsInList( SCH_SCREEN* screen, PICKED_ITEMS_LIST& aItemsList,
                           const wxPoint& aMoveVector );

/**
 * Routine to create a new copy of given struct.
 * The new object is not put in draw list (not linked)
 *
 * @param aDrawStruct = the SCH_ITEM to duplicate
 * @param aClone (default = false)
 *     if true duplicate also some parameters that must be unique
 *     (timestamp and sheet name)
 *      aClone must be false. use true only is undo/redo duplications
 */
SCH_ITEM* DuplicateStruct( SCH_ITEM* aDrawStruct, bool aClone = false );

void DrawDanglingSymbol( EDA_DRAW_PANEL* panel, wxDC* DC,
                         const wxPoint& pos, const KIGFX::COLOR4D& Color );

#endif /* LIST_OPERATIONS_H */
