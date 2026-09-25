/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <commit.h>

class PICKED_ITEMS_LIST;
class TOOL_MANAGER;
class SCH_EDIT_FRAME;
class SCH_BASE_FRAME;
class SCH_ITEM;
class SCH_SCREEN;
class EDA_DRAW_FRAME;
class TOOL_BASE;

template<class T>
class SCH_TOOL_BASE;

#define SKIP_UNDO          0x0001
#define APPEND_UNDO        0x0002
#define SKIP_SET_DIRTY     0x0004
#define SKIP_CONNECTIVITY  0x0008
// With SKIP_UNDO (implicit headless), transfer ownership of removed items to the commit.
#define DELETE_REMOVED_ITEMS 0x0010

/**
 * Commit for the schematic and symbol editors.
 *
 * Outside the symbol editor, Stage() bumps the connectivity revision of the staged item's screen,
 * and the connectivity facade stops serving that screen until the next recalculation.  A commit
 * that staged an item must therefore end in Push() or Revert().  A caller that drops such a commit
 * on purpose must request the recalculation itself.
 */
class SCH_COMMIT : public COMMIT
{
public:
    SCH_COMMIT( TOOL_MANAGER* aToolMgr );
    SCH_COMMIT( EDA_DRAW_FRAME* aFrame );
    SCH_COMMIT( SCH_TOOL_BASE<SCH_BASE_FRAME>* aFrame );

    virtual ~SCH_COMMIT();

    void Push( const wxString& aMessage = wxEmptyString, int aCommitFlags = 0 ) override;

    void Revert() override;
    void RevertToCheckpoint( int aCheckpoint ) override;

    COMMIT& Stage( EDA_ITEM *aItem, CHANGE_TYPE aChangeType, BASE_SCREEN *aScreen = nullptr,
                   RECURSE_MODE aRecurse = RECURSE_MODE::NO_RECURSE ) override;
    COMMIT& Stage( std::vector<EDA_ITEM*> &container, CHANGE_TYPE aChangeType,
                   BASE_SCREEN *aScreen = nullptr ) override;

    EDA_ITEM* ResolveItem( KIID& aID ) override;

    /** Retain the pre-edit state of an item already removed from its screen by cleanup. */
    void RemovedForCleanup( SCH_ITEM* aItem, SCH_SCREEN* aScreen );

protected:
    EDA_ITEM* undoLevelItem( EDA_ITEM* aItem ) const override;

    EDA_ITEM* makeImage( EDA_ITEM* aItem ) const override;

private:
    void pushLibEdit(  const wxString& aMessage, int aCommitFlags );
    void pushSchEdit(  const wxString& aMessage, int aCommitFlags );

    void revertLibEdit();

private:
    TOOL_MANAGER*  m_toolMgr;
    bool           m_isLibEditor;
};
