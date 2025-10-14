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
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html,
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#pragma once

#include <vector>

#include <math/box2.h>
#include <tools/sch_tool_base.h>

class CONDITIONAL_MENU;
class SCH_ITEM;
class SCH_COMMIT;


class SCH_ALIGN_TOOL : public SCH_TOOL_BASE<SCH_EDIT_FRAME>
{
public:
    SCH_ALIGN_TOOL();
    ~SCH_ALIGN_TOOL() override;

    bool Init() override;

    int AlignTop( const TOOL_EVENT& aEvent );
    int AlignBottom( const TOOL_EVENT& aEvent );
    int AlignLeft( const TOOL_EVENT& aEvent );
    int AlignRight( const TOOL_EVENT& aEvent );
    int AlignCenterX( const TOOL_EVENT& aEvent );
    int AlignCenterY( const TOOL_EVENT& aEvent );

private:
    using ITEM_BOX = std::pair<SCH_ITEM*, BOX2I>;

    template< typename T >
    int selectTarget( const std::vector<ITEM_BOX>& aItems, const std::vector<ITEM_BOX>& aLocked,
                      T aGetValue );

    template< typename T >
    size_t GetSelections( std::vector<ITEM_BOX>& aItemsToAlign, std::vector<ITEM_BOX>& aLockedItems,
                          T aCompare );

    void moveItem( SCH_ITEM* aItem, const VECTOR2I& aDelta, SCH_COMMIT& aCommit );
    VECTOR2I adjustDeltaForGrid( SCH_ITEM* aItem, const VECTOR2I& aDelta );
    void setTransitions() override;

private:
    CONDITIONAL_MENU* m_alignMenu;
};

