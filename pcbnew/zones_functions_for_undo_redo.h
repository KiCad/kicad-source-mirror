/**
 * @file zones_functions_for_undo_redo.h
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras <jean-pierre.charras@gipsa-lab.inpg.fr>
 * Copyright (C) 2007 KiCad Developers, see change_log.txt for contributors.
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


/* These functions are relative to undo redo function, when zones are involved.
 *
 * When a zone outline is modified (or created) this zone, or others zones on the same layer
 * and with the same netcode can change or can be deleted due to the fact overlapping zones are
 * merged.  Also, when a zone outline is modified by adding a cutout area, this zone can be
 * converted to more than one area, if the outline is break to 2 or more outlines and therefore
 * new zones are created
 *
 * Due to the complexity of potential changes, and the fact there are only few zones in a board,
 * and a zone has only few segments outlines, the more easy way to undo redo changes is to make
 * a copy of all zones that can be changed and see after zone editing or creation what zones that
 * are really modified, and ones they are modified (changes, deletion or addition)
 */

#ifndef ZONES_FUNCTIONS_TO_UNDO_REDO_H
#define ZONES_FUNCTIONS_TO_UNDO_REDO_H


/**
 * Function SaveCopyOfZones
 * creates a copy of zones having a given netcode on a given layer,
 * and fill a pick list with pickers to handle these copies
 * @param aPickList = the pick list
 * @param aPcb = the Board
 */
void SaveCopyOfZones(PICKED_ITEMS_LIST & aPickList, BOARD* aPcb );


/**
 * Function UpdateCopyOfZonesList
 * check a pick list to remove zones identical to their copies
 * and set the type of operation in picker (DELETED, CHANGED)
 * @param aPickList = the main pick list
 * @param aAuxiliaryList = the list of deleted or added (new created) items after calculations
 * @param aPcb = the Board
 */
void UpdateCopyOfZonesList( PICKED_ITEMS_LIST& aPickList, PICKED_ITEMS_LIST& aAuxiliaryList,
                            BOARD* aPcb );

#endif      // ZONES_FUNCTIONS_TO_UNDO_REDO_H
