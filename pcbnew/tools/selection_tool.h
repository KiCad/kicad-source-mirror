/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
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

#ifndef __SELECTION_TOOL_H
#define __SELECTION_TOOL_H

#include <set>
#include <boost/shared_ptr.hpp>

#include <math/vector2d.h>
#include <tool/tool_interactive.h>

class SELECTION_AREA;
class BOARD_ITEM;
class GENERAL_COLLECTOR;


/**
 * Class SELECTION_TOOL
 *
 * Our sample selection tool: currently supports:
 * - pick single objects (click LMB)
 * - add objects to existing selection (Shift+LMB)
 * - draw selection box (drag LMB)
 *
 * WORK IN PROGRESS. CONSIDER AS A DEMO!
 */

class SELECTION_TOOL : public TOOL_INTERACTIVE
{
public:
    SELECTION_TOOL();
    ~SELECTION_TOOL();

    void Reset();
    int Main( TOOL_EVENT& aEvent );
    const std::set<BOARD_ITEM*>& GetSelection() const
    {
        return m_selectedItems;
    }

private:
    void selectSingle( const VECTOR2I& aWhere );
    void selectMultiple();
    void handleHighlight( const VECTOR2D& aP );
    BOARD_ITEM* disambiguationMenu( GENERAL_COLLECTOR* aItems );
    BOARD_ITEM* pickSmallestComponent( GENERAL_COLLECTOR* aCollector );
    void toggleSelection( BOARD_ITEM* aItem );
    void clearSelection();

    std::set<BOARD_ITEM*> m_selectedItems;
    SELECTION_AREA* m_selArea;
    boost::shared_ptr<CONTEXT_MENU> m_menu;
    bool m_additive;
};

#endif
