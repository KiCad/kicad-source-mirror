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

#ifndef ZOOM_MENU_H
#define ZOOM_MENU_H

#include <tool/action_menu.h>

class EDA_DRAW_FRAME;

class ZOOM_MENU : public ACTION_MENU
{
public:
    ZOOM_MENU( EDA_DRAW_FRAME* aParent );

    void UpdateTitle() override;

private:
    ACTION_MENU* create() const override
    {
        return new ZOOM_MENU( m_parent );
    }

    OPT_TOOL_EVENT eventHandler( const wxMenuEvent& aEvent ) override;
    void update() override;

    EDA_DRAW_FRAME* m_parent;
};

#endif /* ZOOM_MENU_H */
