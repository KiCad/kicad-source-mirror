/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2016 CERN
 * Copyright (C) 2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef CVPCB_ACTIONS_H
#define CVPCB_ACTIONS_H

#include <tool/tool_action.h>
#include <tool/actions.h>
#include <core/optional.h>

class TOOL_EVENT;
class TOOL_MANAGER;

/**
 * Class CVPCB_ACTIONS
 *
 * Gathers all the actions that are shared by tools. The instance of CVPCB_ACTIONS is created
 * inside of ACTION_MANAGER object that registers the actions.
 */
class CVPCB_ACTIONS : public ACTIONS
{
public:
    // Selection Tool
    /// Activation of the selection tool
    static TOOL_ACTION selectionActivate;

    /// Tool selection
    static TOOL_ACTION no_selectionTool;
    static TOOL_ACTION measureTool;

    // Miscellaneous
    static TOOL_ACTION zoomTool;
    static TOOL_ACTION resetCoords;
    static TOOL_ACTION switchCursor;
    static TOOL_ACTION switchUnits;
    static TOOL_ACTION showHelp;
    static TOOL_ACTION toBeDone;


    ///> @copydoc COMMON_ACTIONS::TranslateLegacyId()
    virtual OPT<TOOL_EVENT> TranslateLegacyId( int aId ) override;

    ///> @copydoc COMMON_ACTIONS::RegisterAllTools()
    virtual void RegisterAllTools( TOOL_MANAGER* aToolManager ) override;
};

#endif
