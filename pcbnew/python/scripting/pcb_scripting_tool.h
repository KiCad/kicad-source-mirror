/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef PCBNEW_PCB_SCRIPTING_TOOL_H_
#define PCBNEW_PCB_SCRIPTING_TOOL_H_


#include <tools/pcb_tool_base.h>

class ACTION_MENU;

/**
 * Tool relating to pads and pad settings.
 */
class SCRIPTING_TOOL : public PCB_TOOL_BASE
{
public:
    SCRIPTING_TOOL();
    ~SCRIPTING_TOOL();

    ///< React to model/view changes
    void Reset( RESET_REASON aReason ) override;

    ///< Basic initialization
    bool Init() override;

    static void ReloadPlugins();
    static void ShowPluginFolder();

private:

    ///< Reload Python plugins and reset toolbar (if in pcbnew)
    int reloadPlugins( const TOOL_EVENT& aEvent );

    ///< Call LoadPlugins method of the scripting module with appropriate paths
    ///< Must be called under PyLOCK
    static void callLoadPlugins();

    ///< Open the user's plugin folder in the system browser
    int showPluginFolder( const TOOL_EVENT& aEvent );

    ///< Bind handlers to corresponding TOOL_ACTIONs.
    void setTransitions() override;

};


#endif /* PCBNEW_PCB_SCRIPTING_TOOL_H_ */
