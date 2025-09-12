/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#pragma once

#include <tools/pcb_tool_base.h>

class ACTION_MENU;
class PCB_SHAPE;


class PAD_TOOL : public PCB_TOOL_BASE
{
public:
    PAD_TOOL();
    ~PAD_TOOL() = default;

    ///< React to model/view changes
    void Reset( RESET_REASON aReason ) override;

    ///< Basic initialization
    bool Init() override;

    /**
     * Tool for quick pad enumeration.
     */
    int EnumeratePads( const TOOL_EVENT& aEvent );

    int PadTable( const TOOL_EVENT& aEvent );

    /**
     * Place a pad in footprint editor.
     */
    int PlacePad( const TOOL_EVENT& aEvent );

    /**
     * Enter/exit WYSIWYG pad shape editing.
     */
    int EditPad( const TOOL_EVENT& aEvent );

    int OnUndoRedo( const TOOL_EVENT& aEvent );

    bool InPadEditMode() { return m_editPad != niluuid; }
    void ExitPadEditMode();

    wxString GetLastPadNumber() const { return m_lastPadNumber; }
    void SetLastPadNumber( const wxString& aPadNumber ) { m_lastPadNumber = aPadNumber; }

    /**
     * Recombine an exploded pad (or one produced with overlapping polygons in an older version).
     * @param aPad the pad to run the recombination algorithm on
     * @param aIsDryRun if true the list will be generated but no changes will be made
     * @return a list of PCB_SHAPEs that will be combined
     */
    std::vector<PCB_SHAPE*> RecombinePad( PAD* aPad, bool aIsDryRun );

private:
    ///< Bind handlers to corresponding TOOL_ACTIONs.
    void setTransitions() override;

    ///< Apply pad settings from board design settings to a pad.
    int pastePadProperties( const TOOL_EVENT& aEvent );

    ///< Copy pad settings from a pad to the board design settings.
    int copyPadSettings( const TOOL_EVENT& aEvent );

    ///< Push pad settings from a pad to other pads on board or footprint.
    int pushPadSettings( const TOOL_EVENT& aEvent );

    void explodePad( PAD* aPad, PCB_LAYER_ID* aLayer, BOARD_COMMIT& aCommit );

    void enterPadEditMode();

private:
    wxString           m_lastPadNumber;

    HIGH_CONTRAST_MODE m_previousHighContrastMode;
    KIID               m_editPad;
};
