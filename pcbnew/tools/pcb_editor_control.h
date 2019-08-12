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

#ifndef PCB_EDITOR_CONTROL_H
#define PCB_EDITOR_CONTROL_H

#include <tools/pcb_tool_base.h>
#include <tool/tool_menu.h>

namespace KIGFX {
    class ORIGIN_VIEWITEM;
}

class PCB_EDIT_FRAME;

/**
 * Class PCB_EDITOR_CONTROL
 *
 * Handles actions specific to the board editor in pcbnew.
 */
class PCB_EDITOR_CONTROL : public PCB_TOOL_BASE
{
public:
    PCB_EDITOR_CONTROL();
    ~PCB_EDITOR_CONTROL();

    /// @copydoc TOOL_INTERACTIVE::Reset()
    void Reset( RESET_REASON aReason ) override;

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init() override;

    int New( const TOOL_EVENT& aEvent );
    int Open( const TOOL_EVENT& aEvent );
    int Save( const TOOL_EVENT& aEvent );
    int SaveAs( const TOOL_EVENT& aEvent );
    int SaveCopyAs( const TOOL_EVENT& aEvent );
    int PageSettings( const TOOL_EVENT& aEvent );
    int Plot( const TOOL_EVENT& aEvent );
    
    int BoardSetup( const TOOL_EVENT& aEvent );
    int ImportNetlist( const TOOL_EVENT& aEvent );
    int ImportSpecctraSession( const TOOL_EVENT& aEvent );
    int ExportSpecctraDSN( const TOOL_EVENT& aEvent );
    int GenerateDrillFiles( const TOOL_EVENT& aEvent );
    int GeneratePosFile( const TOOL_EVENT& aEvent );
    int GenerateFabFiles( const TOOL_EVENT& aEvent );

    int UpdatePCBFromSchematic( const TOOL_EVENT& aEvent );
    int ShowEeschema( const TOOL_EVENT& aEvent );
    int ToggleLayersManager( const TOOL_EVENT& aEvent );
    int ToggleMicrowaveToolbar( const TOOL_EVENT& aEvent );
    int TogglePythonConsole( const TOOL_EVENT& aEvent );

    // Track & via size control
    int TrackWidthInc( const TOOL_EVENT& aEvent );
    int TrackWidthDec( const TOOL_EVENT& aEvent );
    int ViaSizeInc( const TOOL_EVENT& aEvent );
    int ViaSizeDec( const TOOL_EVENT& aEvent );

    // Zone actions
    int ZoneMerge( const TOOL_EVENT& aEvent );

    ///> Duplicates a zone onto a layer (prompts for new layer)
    int ZoneDuplicate( const TOOL_EVENT& aEvent );

    /**
     * Function PlaceTarget()
     * Allows user to place a layer alignment target.
     */
    int PlaceTarget( const TOOL_EVENT& aEvent );

    /**
     * Function PlaceModule()
     * Displays a dialog to select a module to be added and allows the user to set its position.
     */
    int PlaceModule( const TOOL_EVENT& aEvent );

    ///> Toggles 'lock' property for selected items.
    int ToggleLockSelected( const TOOL_EVENT& aEvent );

    ///> Locks selected items.
    int LockSelected( const TOOL_EVENT& aEvent );

    ///> Unlocks selected items.
    int UnlockSelected( const TOOL_EVENT& aEvent );

    ///> Runs the drill origin tool for setting the origin for drill and pick-and-place files.
    int DrillOrigin( const TOOL_EVENT& aEvent );

    ///> Low-level access (below undo) to setting the drill origin
    static void DoSetDrillOrigin( KIGFX::VIEW* aView, PCB_BASE_FRAME* aFrame,
                                  BOARD_ITEM* aItem, const VECTOR2D& aPoint );

    int FlipPcbView( const TOOL_EVENT& aEvent );

private:
    ///> How to modify a property for selected items.
    enum MODIFY_MODE { ON, OFF, TOGGLE };

    int modifyLockSelected( MODIFY_MODE aMode );

    ///> Sets up handlers for various events.
    void setTransitions() override;

private:
    PCB_EDIT_FRAME* m_frame;     ///> Pointer to the currently used edit frame.

    std::unique_ptr<KIGFX::ORIGIN_VIEWITEM> m_placeOrigin;    ///> Place & drill origin marker

    static const int WIDTH_STEP; ///> How does line width change after one -/+ key press.
};

#endif
