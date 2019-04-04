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


class wxPoint;
class wxDC;
class SCH_ITEM;
class SCH_SCREEN;
class PICKED_ITEMS_LIST;

void SetSchItemParent( SCH_ITEM* Struct, SCH_SCREEN* Screen );
void RotateListOfItems( PICKED_ITEMS_LIST& aItemsList, const wxPoint& rotationPoint );
void MirrorY( PICKED_ITEMS_LIST& aItemsList, const wxPoint& aMirrorPoint );
void MirrorX( PICKED_ITEMS_LIST& aItemsList, const wxPoint& aMirrorPoint );

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


#endif /* LIST_OPERATIONS_H */
