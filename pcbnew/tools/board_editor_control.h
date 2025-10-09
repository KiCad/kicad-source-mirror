/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2020 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
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

#ifndef BOARD_EDITOR_CONTROL_H
#define BOARD_EDITOR_CONTROL_H

#include <tools/pcb_tool_base.h>
#include <tool/tool_menu.h>

namespace KIGFX {
    class ORIGIN_VIEWITEM;
}

class PCB_EDIT_FRAME;

/**
 * Handle actions specific to the board editor in PcbNew.
 */
class BOARD_EDITOR_CONTROL : public PCB_TOOL_BASE
{
public:
    BOARD_EDITOR_CONTROL();
    ~BOARD_EDITOR_CONTROL();

    /// @copydoc TOOL_INTERACTIVE::Reset()
    void Reset( RESET_REASON aReason ) override;

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init() override;

    int New( const TOOL_EVENT& aEvent );
    int Open( const TOOL_EVENT& aEvent );
    int Save( const TOOL_EVENT& aEvent );
    int SaveAs( const TOOL_EVENT& aEvent );
    int SaveCopy( const TOOL_EVENT& aEvent );
    int Revert( const TOOL_EVENT& aEvent );
    int RescueAutosave( const TOOL_EVENT& aEvent );
    int OpenNonKicadBoard( const TOOL_EVENT& aEvent );
    int ExportFootprints( const TOOL_EVENT& aEvent );
    int PageSettings( const TOOL_EVENT& aEvent );
    int Plot( const TOOL_EVENT& aEvent );

    int Search( const TOOL_EVENT& aEvent );
    int Find( const TOOL_EVENT& aEvent );
    int FindNext( const TOOL_EVENT& aEvent );

    int BoardSetup( const TOOL_EVENT& aEvent );
    int ImportNetlist( const TOOL_EVENT& aEvent );
    int ImportSpecctraSession( const TOOL_EVENT& aEvent );
    int ExportSpecctraDSN( const TOOL_EVENT& aEvent );
    int ExportNetlist( const TOOL_EVENT& aEvent );
    int GenerateDrillFiles( const TOOL_EVENT& aEvent );
    int GeneratePosFile( const TOOL_EVENT& aEvent );
    int GenerateGerbers( const TOOL_EVENT& aEvent );
    int ExportGenCAD( const TOOL_EVENT& aEvent );
    int ExportVRML( const TOOL_EVENT& aEvent );
    int ExportIDF( const TOOL_EVENT& aEvent );
    int ExportSTEP( const TOOL_EVENT& aEvent );
    int ExportCmpFile( const TOOL_EVENT& aEvent );
    int ExportHyperlynx( const TOOL_EVENT& aEvent );
    int GenBOMFileFromBoard( const TOOL_EVENT& aEvent );
    int GenFootprintsReport( const TOOL_EVENT& aEvent );
    int GenD356File( const TOOL_EVENT& aEvent );
    int GenIPC2581File( const TOOL_EVENT& aEvent );
    int GenerateODBPPFiles( const TOOL_EVENT& aEvent );
    int RepairBoard( const TOOL_EVENT& aEvent );

    int UpdatePCBFromSchematic( const TOOL_EVENT& aEvent );
    int UpdateSchematicFromPCB( const TOOL_EVENT& aEvent );
    int ShowEeschema( const TOOL_EVENT& aEvent );
    int ToggleLayersManager( const TOOL_EVENT& aEvent );
    int ToggleProperties( const TOOL_EVENT& aEvent );
    int ToggleNetInspector( const TOOL_EVENT& aEvent );
    int ToggleSearch( const TOOL_EVENT& aEvent );
    int TogglePythonConsole( const TOOL_EVENT& aEvent );
    int ToggleLibraryTree( const TOOL_EVENT& aEvent );

    // Track & via size control
    int TrackWidthInc( const TOOL_EVENT& aEvent );
    int TrackWidthDec( const TOOL_EVENT& aEvent );
    int ViaSizeInc( const TOOL_EVENT& aEvent );
    int ViaSizeDec( const TOOL_EVENT& aEvent );
    int AutoTrackWidth( const TOOL_EVENT& aEvent );

    // Zone actions
    int ZoneMerge( const TOOL_EVENT& aEvent );

    ///< Duplicate a zone onto a layer (prompts for new layer)
    int ZoneDuplicate( const TOOL_EVENT& aEvent );

    int EditFpInFpEditor( const TOOL_EVENT& aEvent );

    ///< Notify Eeschema about selected items.
    int CrossProbeToSch( const TOOL_EVENT& aEvent );

    ///< Equivalent to the above, but initiated by the user.
    int ExplicitCrossProbeToSch( const TOOL_EVENT& aEvent );

    ///< Assign a netclass to a labelled net.
    int AssignNetclass( const TOOL_EVENT& aEvent );

    /**
     * Display a dialog to select a footprint to be added and allows the user to set its position.
     */
    int PlaceFootprint( const TOOL_EVENT& aEvent );

    /**
     * Re-entrancy checker for above.
     */
    bool PlacingFootprint() const { return m_placingFootprint; }

    ///< Toggle 'lock' property for selected items.
    int ToggleLockSelected( const TOOL_EVENT& aEvent );

    ///< Lock selected items.
    int LockSelected( const TOOL_EVENT& aEvent );

    ///< Unlock selected items.
    int UnlockSelected( const TOOL_EVENT& aEvent );

    ///< Run the drill origin tool for setting the origin for drill and pick-and-place files.
    int DrillOrigin( const TOOL_EVENT& aEvent );

    ///< Low-level access (below undo) to setting the drill origin.
    static void DoSetDrillOrigin( KIGFX::VIEW* aView, PCB_BASE_FRAME* aFrame,
                                  EDA_ITEM* aItem, const VECTOR2D& aPoint );

    // Line-mode handlers
    int ChangeLineMode( const TOOL_EVENT& aEvent );
    int OnAngleSnapModeChanged( const TOOL_EVENT& aEvent );

private:
    ///< How to modify a property for selected items.
    enum MODIFY_MODE { ON, OFF, TOGGLE };

    int modifyLockSelected( MODIFY_MODE aMode );

    ///< Set up handlers for various events.
    void setTransitions() override;

    void doCrossProbePcbToSch( const TOOL_EVENT& aEvent, bool aForce );

private:
    PCB_EDIT_FRAME*  m_frame;
    bool             m_inPlaceFootprint;      // Re-entrancy guard for tool.
    bool             m_placingFootprint;      // Re-entrancy guard for placement loop.

    std::unique_ptr<KIGFX::ORIGIN_VIEWITEM> m_placeOrigin;
};

#endif
