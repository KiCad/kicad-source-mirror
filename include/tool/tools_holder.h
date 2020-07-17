/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef  TOOL_HOLDER_H
#define  TOOL_HOLDER_H

#include <vector>
#include <fctsys.h>
#include <common.h>
#include <tool/tool_action.h>


class TOOL_MANAGER;
class TOOL_DISPATCHER;
class ACTIONS;


/*
 * Class TOOLS_HOLDER
 * A mix-in class which allows its owner to hold a set of tools from the tool framework.
 *
 * This is just the framework; the owner is responsible for registering individual tools,
 * creating the dispatcher, etc.
 */
class TOOLS_HOLDER
{
protected:
    TOOL_MANAGER*     m_toolManager;
    ACTIONS*          m_actions;
    TOOL_DISPATCHER*  m_toolDispatcher;

    std::vector<std::string> m_toolStack;   // Stack of user-level "tools".  This is NOT a
                                            // stack of TOOL instances, because somewhat
                                            // confusingly most TOOLs implement more than one
                                            // user-level tool.  A user-level tool actually
                                            // equates to an ACTION handler, so this stack
                                            // stores ACTION names.

    bool              m_immediateActions;   // Preference for immediate actions.  If false,
                                            // the first invocation of a hotkey will just
                                            // select the relevant tool rather than executing
                                            // the tool's action.
    bool              m_dragSelects;        // Prefer selection to dragging.
    bool              m_moveWarpsCursor;    // cursor is warped to move/drag origin

public:
    TOOLS_HOLDER();

    virtual ~TOOLS_HOLDER() { }

    /**
     * Return the MVC controller.
     */
    TOOL_MANAGER* GetToolManager() const { return m_toolManager; }

    /**
     * NB: the definition of "tool" is different at the user level.  The implementation uses
     * a single TOOL_BASE derived class to implement several user "tools", such as rectangle
     * and circle, or wire and bus.  So each user-level tool is actually a TOOL_ACTION.
     */
    virtual void PushTool( const std::string& actionName );
    virtual void PopTool( const std::string& actionName );

    bool ToolStackIsEmpty() { return m_toolStack.empty(); }

    std::string CurrentToolName() const;
    bool IsCurrentTool( const TOOL_ACTION& aAction ) const;

    virtual void DisplayToolMsg( const wxString& msg ) {};

    /**
     * Indicates that hotkeys should perform an immediate action even if another tool is
     * currently active.  If false, the first hotkey should select the relevant tool.
     */
    bool GetDoImmediateActions() const { return m_immediateActions; }

    /**
     * Indicates that a drag should draw a selection rectangle, even when started over an
     * item.
     */
    bool GetDragSelects() const { return m_dragSelects; }

    /**
     * Indicates that a move operation should warp the mouse pointer to the origin of the
     * move object.  This improves snapping, but some users are alergic to mouse warping.
     */
    bool GetMoveWarpsCursor() const { return m_moveWarpsCursor; }

    /**
     * Notification event that some of the common (suite-wide) settings have changed.
     * Update hotkeys, preferences, etc.
     */
    virtual void CommonSettingsChanged( bool aEnvVarsChanged, bool aTextVarsChanged );

    /**
     * Canvas access.
     */
    virtual wxWindow* GetToolCanvas() const = 0;
    virtual void RefreshCanvas() { }

    virtual wxString ConfigBaseName() { return wxEmptyString; }
};

#endif  // TOOL_HOLDER_H
