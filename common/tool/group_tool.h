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

#pragma once

#include <tool/tool_interactive.h>

class COMMIT;
class DIALOG_GROUP_PROPERTIES;
class EDA_DRAW_FRAME;
class EDA_GROUP;
class SELECTION_TOOL;


class GROUP_TOOL : public TOOL_INTERACTIVE
{
public:
    GROUP_TOOL();

    void Reset( RESET_REASON aReason ) override;

    /// @copydoc TOOL_BASE::Init()
    bool Init() override;

    virtual int GroupProperties( const TOOL_EVENT& aEvent );

    /**
     * Invoke the picker tool to select a new member of the group.
     */
    virtual int PickNewMember( const TOOL_EVENT& aEvent ) = 0;

    ///< Group selected items.
    virtual int Group( const TOOL_EVENT& aEvent ) = 0;

    ///< Ungroup selected items.
    virtual int Ungroup( const TOOL_EVENT& aEvent );

    ///< Add selection to group.
    virtual int AddToGroup( const TOOL_EVENT& aEvent );

    ///< Remove selection from group.
    virtual int RemoveFromGroup( const TOOL_EVENT& aEvent );

    ///< Restrict selection to only member of the group.
    virtual int EnterGroup( const TOOL_EVENT& aEvent );

    ///< Leave the current group (deselect its members and select the group as a whole).
    virtual int LeaveGroup( const TOOL_EVENT& aEvent );

protected:
    ///< Set up handlers for various events.
    void setTransitions() override;

    ///< Get the correctly casted group type from the item.
    /// Works around our lack of working dynamic_cast.
    virtual EDA_GROUP* getGroupFromItem( EDA_ITEM* ) = 0;

    ///< Subclasses implement to provide correct *_COMMIT object type
    virtual std::shared_ptr<COMMIT> createCommit() = 0;

    EDA_DRAW_FRAME*          m_frame = nullptr;
    DIALOG_GROUP_PROPERTIES* m_propertiesDialog = nullptr;
    SELECTION_TOOL*          m_selectionTool = nullptr;
    std::shared_ptr<COMMIT>  m_commit;
};
