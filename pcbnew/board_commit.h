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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#pragma once

#include <commit.h>

class BOARD_ITEM;
class PCB_SHAPE;
class ZONE;
class BOARD;
class PICKED_ITEMS_LIST;
class PCB_TOOL_BASE;
class TOOL_MANAGER;
class EDA_DRAW_FRAME;
class TOOL_BASE;

#define SKIP_UNDO          0x0001
#define APPEND_UNDO        0x0002
#define SKIP_SET_DIRTY     0x0004
#define SKIP_CONNECTIVITY  0x0008
#define ZONE_FILL_OP       0x0010
#define SKIP_TEARDROPS     0x0020
#define SKIP_ENTERED_GROUP 0x0040

class BOARD_COMMIT : public COMMIT
{
public:
    BOARD_COMMIT( EDA_DRAW_FRAME* aFrame );
    BOARD_COMMIT( TOOL_BASE* aTool );
    BOARD_COMMIT( TOOL_MANAGER* aMgr );
    BOARD_COMMIT( TOOL_MANAGER* aMgr, bool aIsBoardEditor, bool aIsFootprintEditor );

    virtual ~BOARD_COMMIT() {}

    BOARD* GetBoard() const;

    virtual void Push( const wxString& aMessage = wxEmptyString, int aCommitFlags = 0 ) override;

    virtual void Revert() override;
    COMMIT&      Stage( EDA_ITEM* aItem, CHANGE_TYPE aChangeType,
                        BASE_SCREEN* aScreen = nullptr,
                        RECURSE_MODE aRecurse = RECURSE_MODE::NO_RECURSE ) override;
    COMMIT&      Stage( std::vector<EDA_ITEM*>& container, CHANGE_TYPE aChangeType,
                        BASE_SCREEN* aScreen = nullptr ) override;
    COMMIT&      Stage( const PICKED_ITEMS_LIST& aItems,
                        UNDO_REDO aModFlag = UNDO_REDO::UNSPECIFIED,
                        BASE_SCREEN* aScreen = nullptr ) override;

    static EDA_ITEM* MakeImage( EDA_ITEM* aItem );

private:
    EDA_ITEM* undoLevelItem( EDA_ITEM* aItem ) const override;

    EDA_ITEM* makeImage( EDA_ITEM* aItem ) const override;

    void propagateDamage( BOARD_ITEM* aItem, std::vector<ZONE*>* aStaleZones,
                          std::vector<BOX2I>& aStaleRuleAreas );

private:
    TOOL_MANAGER*  m_toolMgr;
    bool           m_isBoardEditor;
    bool           m_isFootprintEditor;
};
