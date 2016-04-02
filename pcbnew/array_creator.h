/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Created on: 11 Mar 2016, author John Beard
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file array_creator.h
 */

#ifndef PCBNEW_ARRAY_CREATOR_H_
#define PCBNEW_ARRAY_CREATOR_H_

#include <dialogs/dialog_create_array.h>

#include <class_board.h>
#include <class_module.h>
#include <class_board_item.h>

/*!
 * Class that performs array creation by producing a dialog to gather
 * parameters and then creating and laying out the items.
 *
 * This is a template class which needs to be implemented by the relevant
 * edit tooling, since the details of how the document is manipulated
 * varies between edit modes (e.g. legacy or GAL)
 */
class ARRAY_CREATOR
{
public:
    ARRAY_CREATOR(PCB_BASE_FRAME& parent):
        m_parent( parent )
    {}

    /*!
     * Open the dialog, gather parameters and create the array
     */
    void Invoke();

protected:
    virtual ~ARRAY_CREATOR() {}

    PCB_BASE_FRAME& m_parent;

private:

    /*!
     * Get the BOARD that is currently being edited.
     */
    virtual BOARD* getBoard() const = 0;

    /*!
     * If editing a footprint, returns the relevant MODULE, else NULL
     */
    virtual MODULE* getModule() const = 0;

    /*!
     * @return number of original items to put into an array (eg size of the
     * selection)
     */
    virtual int getNumberOfItemsToArray() const = 0;

    /*!
     * @return the n'th original item to be arrayed
     */
    virtual BOARD_ITEM* getNthItemToArray( int n ) const = 0;

    /*!
     * @return the rotation centre of all the items to be arrayed, when taken
     * together
     */
    virtual wxPoint getRotationCentre() const = 0;

    /*!
     * Perform any relevant action before pushing a newly created array item
     * to the BOARD
     */
    virtual void prePushAction( BOARD_ITEM* new_item )
    {}

    /*!
     * Perform any actions needed after pushing an item to the BOARD
     */
    virtual void postPushAction( BOARD_ITEM* new_item )
    {}

    /*!
     * Actions to perform after the array process is complete
     */
    virtual void finalise() = 0;
};




#endif /* PCBNEW_ARRAY_CREATOR_H_ */
