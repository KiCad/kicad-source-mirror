/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
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

#ifndef MODULE_TOOLS_H
#define MODULE_TOOLS_H

#include <tool/tool_interactive.h>

namespace KIGFX
{
    class VIEW;
    class VIEW_CONTROLS;
}
class BOARD;
class PCB_EDIT_FRAME;

/**
 * Class MODULE_TOOLS
 *
 * Module editor specific tools.
 */
class MODULE_TOOLS : public TOOL_INTERACTIVE
{
public:
    MODULE_TOOLS();

    /// @copydoc TOOL_INTERACTIVE::Reset()
    void Reset( RESET_REASON aReason );

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init();

    /**
     * Function PlacePad()
     * Places a pad in module editor.
     */
    int PlacePad( TOOL_EVENT& aEvent );

    /**
     * Function EnumeratePads()
     * Tool for quick pad enumeration.
     */
    int EnumeratePads( TOOL_EVENT& aEvent );

    /**
     * Function CopyItems()
     *
     * Copies selected items to the clipboard. Works only in "edit modules" mode.
     */
    int CopyItems( TOOL_EVENT& aEvent );

    /**
     * Function PastePad()
     *
     * Pastes items from the clipboard. Works only in "edit modules" mode.
     */
    int PasteItems( TOOL_EVENT& aEvent );

private:
    ///> Sets up handlers for various events.
    void setTransitions();

    KIGFX::VIEW* m_view;
    KIGFX::VIEW_CONTROLS* m_controls;
    BOARD* m_board;
    PCB_EDIT_FRAME* m_frame;
};

#endif
