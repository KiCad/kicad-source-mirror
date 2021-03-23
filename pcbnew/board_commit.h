/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#ifndef __BOARD_COMMIT_H
#define __BOARD_COMMIT_H

#include <commit.h>

class BOARD_ITEM;
class PICKED_ITEMS_LIST;
class PCB_TOOL_BASE;
class TOOL_MANAGER;
class EDA_DRAW_FRAME;
class TOOL_BASE;

class BOARD_COMMIT : public COMMIT
{
public:
    BOARD_COMMIT( EDA_DRAW_FRAME* aFrame );
    BOARD_COMMIT( PCB_TOOL_BASE *aTool );

    virtual ~BOARD_COMMIT();

    virtual void Push( const wxString& aMessage = wxT( "A commit" ),
                       bool aCreateUndoEntry = true, bool aSetDirtyBit = true ) override;

    virtual void Revert() override;
    COMMIT&      Stage( EDA_ITEM* aItem, CHANGE_TYPE aChangeType ) override;
    COMMIT&      Stage( std::vector<EDA_ITEM*>& container, CHANGE_TYPE aChangeType ) override;
    COMMIT&      Stage(
                 const PICKED_ITEMS_LIST& aItems, UNDO_REDO aModFlag = UNDO_REDO::UNSPECIFIED ) override;

    /**
     * Sets a flag that will cause Push() to resolve net conflicts on track/via clusters instead
     * of the default behavior which is to skip updating track/via clusters that have conflicts.
     * This is used in the netlist updater to update any clusters that were changed due to pad nets
     * changing, but should not be used for other changes as you typically don't want to change
     * track/via nets due to temporary conflicts created by board editing operations.
     * @param aResolve is true if this commit should resolve conflicting track/via net assignments
    */
    void SetResolveNetConflicts( bool aResolve = true ) { m_resolveNetConflicts = aResolve; }

private:
    virtual EDA_ITEM* parentObject( EDA_ITEM* aItem ) const override;

private:
    TOOL_MANAGER* m_toolMgr;
    bool          m_isFootprintEditor;
    bool          m_resolveNetConflicts;
};

#endif
