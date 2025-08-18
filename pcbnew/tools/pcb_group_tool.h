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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <tool/group_tool.h>
#include <board_commit.h>
#include <pcb_group.h>

class PCB_GROUP_TOOL : public GROUP_TOOL
{
public:
    /**
     * Invoke the picker tool to select a new member of the group.
     */
    int PickNewMember( const TOOL_EVENT& aEvent ) override;

    ///< Group selected items.
    int Group( const TOOL_EVENT& aEvent ) override;

protected:
    std::shared_ptr<COMMIT> createCommit() override;

    EDA_GROUP* getGroupFromItem( EDA_ITEM* aItem ) override
    {
        if( aItem->Type() == PCB_GROUP_T )
            return static_cast<PCB_GROUP*>( aItem );

        return nullptr;
    }
};
