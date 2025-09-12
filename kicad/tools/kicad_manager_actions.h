/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef KICAD_MANAGER_ACTIONS_H
#define KICAD_MANAGER_ACTIONS_H

#include <tool/tool_action.h>
#include <tool/actions.h>

class KICAD_MANAGER_ACTIONS : public ACTIONS
{
public:
    static TOOL_ACTION newProject;
    static TOOL_ACTION newFromRepository;
    static TOOL_ACTION newJobsetFile;
    static TOOL_ACTION openDemoProject;
    static TOOL_ACTION openProject;
    static TOOL_ACTION openJobsetFile;
    static TOOL_ACTION closeProject;
    static TOOL_ACTION loadProject;
    static TOOL_ACTION viewDroppedGerbers;

    static TOOL_ACTION editSchematic;
    static TOOL_ACTION editSymbols;
    static TOOL_ACTION editPCB;
    static TOOL_ACTION editFootprints;
    static TOOL_ACTION viewGerbers;
    static TOOL_ACTION convertImage;
    static TOOL_ACTION showCalculator;
    static TOOL_ACTION editDrawingSheet;
    static TOOL_ACTION openTextEditor;
    static TOOL_ACTION showPluginManager;

    static TOOL_ACTION editOtherSch;
    static TOOL_ACTION editOtherPCB;

    static TOOL_ACTION archiveProject;
    static TOOL_ACTION unarchiveProject;
    static TOOL_ACTION openProjectDirectory;
    static TOOL_ACTION restoreLocalHistory;
    static TOOL_ACTION showLocalHistory;
};


#endif
