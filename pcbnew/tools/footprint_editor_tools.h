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

#ifndef MODULE_EDITOR_TOOLS_H
#define MODULE_EDITOR_TOOLS_H

#include <tools/pcb_tool.h>

namespace KIGFX
{
    class VIEW;
    class VIEW_CONTROLS;
}
class BOARD;
class PCB_EDIT_FRAME;

/**
 * Class MODULE_EDITOR_TOOLS
 *
 * Module editor specific tools.
 */
class MODULE_EDITOR_TOOLS : public PCB_TOOL
{
public:
    MODULE_EDITOR_TOOLS();
    ~MODULE_EDITOR_TOOLS();

    /// @copydoc TOOL_INTERACTIVE::Reset()
    void Reset( RESET_REASON aReason ) override;

    /**
     * Function PlacePad()
     * Places a pad in module editor.
     */
    int PlacePad( const TOOL_EVENT& aEvent );

    /**
     * Function EnumeratePads()
     * Tool for quick pad enumeration.
     */
    int EnumeratePads( const TOOL_EVENT& aEvent );

    /**
     * Function CreateArray
     *
     * Creates an array of objects using settings from a dialog
     */
    int CreateArray( TOOL_EVENT& aEvent );

    /**
     * Function CreatePadFromShapes()
     *
     * Creates a custom-shaped pad from a set of selected graphical shapes
     */
    int CreatePadFromShapes( const TOOL_EVENT& aEvent );

    /**
     * Function ExplodePadToShapes()
     *
     * Breaks apart a complex-shaped part into a set of graphical shapes
     */
    int ExplodePadToShapes( const TOOL_EVENT& aEvent );


    ///> Sets up handlers for various events.
    void setTransitions() override;

};

#endif
