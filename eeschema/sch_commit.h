/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * Copyright (C) 2016-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef SCHEMATIC_COMMIT_H
#define SCHEMATIC_COMMIT_H

#include <commit.h>

class PICKED_ITEMS_LIST;
class TOOL_MANAGER;
class SCH_EDIT_FRAME;
class EDA_DRAW_FRAME;
class TOOL_BASE;

template<class T>
class EE_TOOL_BASE;

#define SKIP_UNDO          0x0001
#define APPEND_UNDO        0x0002
#define SKIP_SET_DIRTY     0x0004
#define SKIP_CONNECTIVITY  0x0008

class SCH_COMMIT : public COMMIT
{
public:
    SCH_COMMIT( TOOL_MANAGER* aToolMgr );
    SCH_COMMIT( EDA_DRAW_FRAME* aFrame );
    SCH_COMMIT( EE_TOOL_BASE<SCH_BASE_FRAME>* aFrame );

    virtual ~SCH_COMMIT();

    virtual void Push( const wxString& aMessage = wxT( "A commit" ),
                       int aCommitFlags = 0 ) override;

    virtual void Revert() override;
    COMMIT& Stage( EDA_ITEM *aItem, CHANGE_TYPE aChangeType,
                   BASE_SCREEN *aScreen = nullptr ) override;
    COMMIT& Stage( std::vector<EDA_ITEM*> &container, CHANGE_TYPE aChangeType,
                   BASE_SCREEN *aScreen = nullptr ) override;
    COMMIT& Stage( const PICKED_ITEMS_LIST &aItems, UNDO_REDO aModFlag = UNDO_REDO::UNSPECIFIED,
                   BASE_SCREEN *aScreen = nullptr ) override;

private:
    EDA_ITEM* parentObject( EDA_ITEM* aItem ) const override;

    EDA_ITEM* makeImage( EDA_ITEM* aItem ) const override;

    void pushLibEdit(  const wxString& aMessage, int aCommitFlags );
    void pushSchEdit(  const wxString& aMessage, int aCommitFlags );

    void revertLibEdit();

private:
    TOOL_MANAGER*  m_toolMgr;
    bool           m_isLibEditor;
};

#endif
