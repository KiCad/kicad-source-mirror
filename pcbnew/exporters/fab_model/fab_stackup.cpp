/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software: you can redistribute it and/or
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <exporters/fab_model/fab_stackup.h>

#include <board.h>
#include <board_design_settings.h>


FAB_STACKUP::FAB_STACKUP( const BOARD& aBoard ) :
        m_stackup( aBoard.GetDesignSettings().GetStackupDescriptor() )
{
    m_stackup.SynchronizeWithBoard( &aBoard.GetDesignSettings() );
}


const BOARD_STACKUP_ITEM* FAB_STACKUP::ItemForLayer( PCB_LAYER_ID aLayer ) const
{
    if( aLayer == UNDEFINED_LAYER )
        return nullptr;

    for( const BOARD_STACKUP_ITEM* item : m_stackup.GetList() )
    {
        if( item->GetBrdLayerId() == aLayer )
            return item;
    }

    return nullptr;
}
