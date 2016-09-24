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
class PCB_TOOL;
class PCB_BASE_FRAME;
class TOOL_MANAGER;

class BOARD_COMMIT : public COMMIT
{
public:
    BOARD_COMMIT( PCB_TOOL* aTool );
    BOARD_COMMIT( PCB_BASE_FRAME* aFrame );
    virtual ~BOARD_COMMIT();

    virtual void Push( const wxString& aMessage ) override;
    virtual void Revert() override;

private:
    TOOL_MANAGER* m_toolMgr;
    bool m_editModules;
    virtual EDA_ITEM* parentObject( EDA_ITEM* aItem ) const override;
};

#endif
