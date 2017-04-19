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
 * @file array_creator.cpp
 */

#include "array_creator.h"
#include <board_commit.h>

#include <dialogs/dialog_create_array.h>


void ARRAY_CREATOR::Invoke()
{
    const int numItems = getNumberOfItemsToArray();

    // bail out if no items
    if( numItems == 0 )
        return;

    MODULE* const module = getModule();
    const bool isModuleEditor = module != NULL;

    const bool enableArrayNumbering = isModuleEditor;
    const wxPoint rotPoint = getRotationCentre();

    DIALOG_CREATE_ARRAY dialog( &m_parent, enableArrayNumbering, rotPoint );
    int ret = dialog.ShowModal();

    DIALOG_CREATE_ARRAY::ARRAY_OPTIONS* const array_opts = dialog.GetArrayOptions();

    if( ret != wxID_OK || array_opts == NULL )
        return;

    BOARD_COMMIT commit( &m_parent );

    for ( int i = 0; i < numItems; ++i )
    {
        BOARD_ITEM* item = getNthItemToArray( i );

        if( item->Type() == PCB_PAD_T && !isModuleEditor )
        {
            // If it is not the module editor, then duplicate the parent module instead
            item = static_cast<MODULE*>( item )->GetParent();
        }

        // The first item in list is the original item. We do not modify it
        for( int ptN = 1; ptN < array_opts->GetArraySize(); ptN++ )
        {
            BOARD_ITEM* new_item;

            if( isModuleEditor )
            {
                // increment pad numbers if do any renumbering
                // (we will number again later according to the numbering scheme if set)
                new_item = module->Duplicate( item, array_opts->ShouldNumberItems() );
            }
            else
            {
                // PCB items keep the same numbering
                new_item = getBoard()->Duplicate( item );

                // @TODO: we should merge zones. This is a bit tricky, because
                // the undo command needs saving old area, if it is merged.
            }

            if( new_item )
            {
                array_opts->TransformItem( ptN, new_item, rotPoint );
                prePushAction( new_item );
                commit.Add( new_item );
                postPushAction( new_item );
            }

            // attempt to renumber items if the array parameters define
            // a complete numbering scheme to number by (as opposed to
            // implicit numbering by incrementing the items during creation
            if( new_item && array_opts->NumberingStartIsSpecified() )
            {
                // Renumber pads. Only new pad number renumbering has meaning,
                // in the footprint editor.
                if( new_item->Type() == PCB_PAD_T )
                {
                    const wxString padName = array_opts->GetItemNumber( ptN );
                    static_cast<D_PAD*>( new_item )->SetPadName( padName );
                }
            }
        }
    }

    commit.Push( _( "Create an array" ) );
    finalise();
}
