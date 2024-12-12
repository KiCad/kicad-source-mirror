/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DRC_RULE_EDITOR_TOOL_H
#define DRC_RULE_EDITOR_TOOL_H

#include <board_commit.h>
#include <board.h>
#include <memory>
#include <vector>
#include <tools/pcb_tool_base.h>


class PCB_EDIT_FRAME;
class DIALOG_DRC_RULE_EDITOR;
class WX_PROGRESS_REPORTER;
class DRC_ENGINE;


class DRC_RULE_EDITOR_TOOL : public PCB_TOOL_BASE
{
public:
    DRC_RULE_EDITOR_TOOL();
    ~DRC_RULE_EDITOR_TOOL();

    /// @copydoc TOOL_INTERACTIVE::Reset()
    void Reset( RESET_REASON aReason ) override;

    /**
     * Opens the DRC Rule Editor dialog.  The dialog is only created if it is not already in existence.
     *
     * @param aParent is the parent window for modal invocations.  If nullptr, the parent will
     * be the PCB_EDIT_FRAME and the dialog will be modeless.
     */
    void ShowDRCRuleEditorDialog( wxWindow* aParent );

    int ShowDRCRuleEditorDialog( const TOOL_EVENT& aEvent );

    /**
     * Check to see if the DRC_TOOL dialog is currently shown
     */
    bool IsDRCRuleEditorDialogShown();

    /**
     * Close and free the DRC dialog.
     */
    void DestroyDRCRuleEditorDialog();

    std::shared_ptr<DRC_ENGINE> GetDRCEngine() { return m_drcEngine; }

private:
    ///< Set up handlers for various events.
    void setTransitions() override;

    /**
     * Update needed pointers from the one pointer which is known not to change.
     */
    void updatePointers();

    EDA_UNITS userUnits() const { return m_editFrame->GetUserUnits(); }

private:
    PCB_EDIT_FRAME*             m_editFrame;
    BOARD*                      m_pcb;
    DIALOG_DRC_RULE_EDITOR*     m_drcRuleEditorDlg;
    std::shared_ptr<DRC_ENGINE> m_drcEngine;
};


#endif // DRC_RULE_EDITOR_TOOL_H
