/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#include <tool/action_menu.h>

class EDA_DRAW_FRAME;
struct WINDOW_SETTINGS;

class GRID_MENU : public ACTION_MENU
{
public:
    GRID_MENU( EDA_DRAW_FRAME* aParent );

    void UpdateTitle() override;

    static void BuildChoiceList( wxArrayString* aGridsList, WINDOW_SETTINGS* aCfg, EDA_DRAW_FRAME* aParent );

private:
    ACTION_MENU* create() const override
    {
        return new GRID_MENU( m_parent );
    }

    OPT_TOOL_EVENT eventHandler( const wxMenuEvent& aEvent ) override;
    void update() override;

private:
    EDA_DRAW_FRAME* m_parent;
};
