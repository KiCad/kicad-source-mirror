/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <board_tables/generated_table_refresh.h>

#include <board.h>
#include <pcb_generated_table.h>


GENERATED_TABLE_REFRESH::GENERATED_TABLE_REFRESH( BOARD& aBoard ) :
        m_board( aBoard )
{
}


void GENERATED_TABLE_REFRESH::Commit()
{
    for( auto& [type, pending] : m_pending )
        pending->Commit( m_board );
}


int RefreshGeneratedTables( BOARD& aBoard )
{
    // Pending state is committed only once every table has rebuilt, so a table that throws
    // cannot leave half a profile behind
    GENERATED_TABLE_REFRESH refresh( aBoard );
    int                     rebuilt = 0;

    for( BOARD_ITEM* item : aBoard.Drawings() )
    {
        if( !IsGeneratedTableType( item->Type() ) )
            continue;

        PCB_GENERATED_TABLE* table = static_cast<PCB_GENERATED_TABLE*>( item );

        if( !table->IsStale( aBoard ) )
            continue;

        table->RebuildCells( aBoard, &refresh );
        rebuilt++;
    }

    if( rebuilt )
        refresh.Commit();

    return rebuilt;
}
