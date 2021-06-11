/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020-2021 KiCad Developers, see change_log.txt for contributors.
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

#ifndef DRC_TOOL_H
#define DRC_TOOL_H

#include <board_commit.h>
#include <board.h>
#include <pcb_marker.h>
#include <geometry/seg.h>
#include <geometry/shape_poly_set.h>
#include <memory>
#include <vector>
#include <tools/pcb_tool_base.h>


class PCB_EDIT_FRAME;
class DIALOG_DRC;
class DRC_ITEM;
class WX_PROGRESS_REPORTER;
class DRC_ENGINE;


class DRC_TOOL : public PCB_TOOL_BASE
{
public:
    DRC_TOOL();
    ~DRC_TOOL();

    /// @copydoc TOOL_INTERACTIVE::Reset()
    void Reset( RESET_REASON aReason ) override;

    /**
     * Open a dialog and prompts the user, then if a test run button is
     * clicked, runs the test(s) and creates the MARKERS.  The dialog is only
     * created if it is not already in existence.
     *
     * @param aParent is the parent window for wxWidgets. Usually the PCB editor frame
     * but can be another dialog
     * if aParent == NULL (default), the parent will be the PCB editor frame
     * and the dialog will be not modal (just float on parent
     * if aParent is specified, the dialog will be modal.
     * The modal mode is mandatory if the dialog is created from another dialog, not
     * from the PCB editor frame
     */
    void ShowDRCDialog( wxWindow* aParent );

    int ShowDRCDialog( const TOOL_EVENT& aEvent );

    /**
     * Check to see if the DRC_TOOL dialog is currently shown
     *
     * @return true if the dialog is shown
     */
    bool IsDRCDialogShown();

    /**
     * Check to see if the DRC engine is running the tests
     *
     * @return true if the DRC engine is running the tests
     */
    bool IsDRCRunning() const { return m_drcRunning; }

    /**
     * Close and free the DRC dialog.
     */
    void DestroyDRCDialog();

    std::shared_ptr<DRC_ENGINE> GetDRCEngine() { return m_drcEngine; }

    /**
     * Run the DRC tests.
     */
    void RunTests( PROGRESS_REPORTER* aProgressReporter, bool aRefillZones,
                   bool aReportAllTrackErrors, bool aTestFootprints );

    int PrevMarker( const TOOL_EVENT& aEvent );
    int NextMarker( const TOOL_EVENT& aEvent );
    int ExcludeMarker( const TOOL_EVENT& aEvent );

private:
    ///< Set up handlers for various events.
    void setTransitions() override;

    /**
     * Update needed pointers from the one pointer which is known not to change.
     */
    void updatePointers();

    EDA_UNITS userUnits() const { return m_editFrame->GetUserUnits(); }

    PCB_EDIT_FRAME*  m_editFrame;        // The pcb frame editor which owns the board
    BOARD*           m_pcb;
    DIALOG_DRC*      m_drcDialog;
    bool             m_drcRunning;

    std::shared_ptr<DRC_ENGINE>            m_drcEngine;

    std::vector<std::shared_ptr<DRC_ITEM>> m_unconnected;      // list of unconnected pads
    std::vector<std::shared_ptr<DRC_ITEM>> m_footprints;       // list of footprint warnings
};


#endif  // DRC_TOOL_H
