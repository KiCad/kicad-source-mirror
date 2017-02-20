/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2016 CERN
 * Copyright (C) 2016 KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef __ACTIONS_H
#define __ACTIONS_H

#include <tool/tool_action.h>
#include <boost/optional.hpp>

class TOOL_EVENT;
class TOOL_MANAGER;

/**
 * Class ACTIONS
 *
 * Gathers all the actions that are shared by tools. The instance of a subclass of
 * ACTIONS is created inside of ACTION_MANAGER object that registers the actions.
 */
class ACTIONS
{
public:

    virtual ~ACTIONS() {};

    // View controls
    static TOOL_ACTION zoomIn;
    static TOOL_ACTION zoomOut;
    static TOOL_ACTION zoomInCenter;
    static TOOL_ACTION zoomOutCenter;
    static TOOL_ACTION zoomCenter;
    static TOOL_ACTION zoomFitScreen;
    static TOOL_ACTION zoomPreset;

    // Grid control
    static TOOL_ACTION gridFast1;
    static TOOL_ACTION gridFast2;
    static TOOL_ACTION gridNext;
    static TOOL_ACTION gridPrev;
    static TOOL_ACTION gridSetOrigin;
    static TOOL_ACTION gridResetOrigin;
    static TOOL_ACTION gridPreset;

    /**
     * Function TranslateLegacyId()
     * Translates legacy tool ids to the corresponding TOOL_ACTION name.
     * @param aId is legacy tool id to be translated.
     * @return std::string is name of the corresponding TOOL_ACTION. It may be empty, if there is
     * no corresponding TOOL_ACTION.
     */
    virtual boost::optional<TOOL_EVENT> TranslateLegacyId( int aId ) = 0;

    ///> Registers all valid tools for an application with the tool manager
    virtual void RegisterAllTools( TOOL_MANAGER* aToolManager ) = 0;

    ///> Cursor control event types
    enum CURSOR_EVENT_TYPE { CURSOR_UP, CURSOR_DOWN, CURSOR_LEFT, CURSOR_RIGHT,
                             CURSOR_CLICK, CURSOR_DBL_CLICK, CURSOR_FAST_MOVE = 0x8000 };

    ///> Remove event modifier flags
    enum class REMOVE_FLAGS { NORMAL = 0x00, ALT = 0x01 };
};

#endif
