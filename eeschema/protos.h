/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Jean-Pierre Charras, jp.charras at wanadoo.fr
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


#ifndef __PROTOS_H__
#define __PROTOS_H__

#include <colors.h>

class EDA_DRAW_PANEL;
class EDA_DRAW_FRAME;
class PICKED_ITEMS_LIST;
class PART_LIB;
class SCH_ITEM;

//void DisplayCmpDoc( wxString& Name );
wxString DataBaseGetName( EDA_DRAW_FRAME* frame, wxString& Keys, wxString& BufName );

// operations_on_item_lists.cpp
void DeleteItemsInList( EDA_DRAW_PANEL* panel, PICKED_ITEMS_LIST& aItemsList );

/**
 * Function DuplicateStruct
 * creates a new copy of given struct.
 * @param aDrawStruct = the SCH_ITEM to duplicate
 * @param aClone (defualt = true)
 *     if true duplicate also some parameters that must be unique
 *     (timestamp and sheet name)
 *      aClone must be false. use true only is undo/redo duplications
 */
SCH_ITEM* DuplicateStruct( SCH_ITEM* DrawStruct, bool aClone = false );


/****************/
/* EEREDRAW.CPP */
/****************/
void DrawDanglingSymbol( EDA_DRAW_PANEL* panel, wxDC* DC,
                         const wxPoint& pos, EDA_COLOR_T Color );


/***************/
/* SELPART.CPP */
/***************/

/**
 * Function DisplayComponentsNamesInLib
 * Select component from list of components in this library
 *
 * If == NULL Library, selection of library REQUESTED
 * If only in research library
 *
 * Returns
 *   1 if selected component
 *   0 if canceled order
 */
int DisplayComponentsNamesInLib( EDA_DRAW_FRAME* frame,
                                 PART_LIB*    Library,
                                 wxString&       Buffer,
                                 wxString&       OldName );

/**
 * Function SelectLibraryFromList
 * displays a list of current loaded libraries, and allows the user to select
 * a library
 * This list is sorted, with the library cache always at end of the list
 */
PART_LIB* SelectLibraryFromList( EDA_DRAW_FRAME* frame );

/**
 * Get the name component from a library to load.
 *
 * If no library specified, there will be demand for selection of a library.
 * Returns
 *   1 if selected component
 *   0 if canceled order
 * Place the name of the selected component list in BufName
 */
int GetNameOfPartToLoad( EDA_DRAW_FRAME* frame, PART_LIB* Lib, wxString& BufName );

#endif  /* __PROTOS_H__ */
