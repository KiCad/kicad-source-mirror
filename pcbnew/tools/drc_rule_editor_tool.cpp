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

#include <pcb_edit_frame.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_tool_base.h>
#include <tools/zone_filler_tool.h>
#include <tools/pcb_selection_tool.h>
#include <tools/drc_rule_editor_tool.h>
#include <kiface_base.h>
#include <board_commit.h>
#include <board_design_settings.h>
#include <progress_reporter.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <macros.h>
#include <drc/rule_editor/dialog_drc_rule_editor.h>


DRC_RULE_EDITOR_TOOL::DRC_RULE_EDITOR_TOOL() :
        PCB_TOOL_BASE( "pcbnew.DRETool" ), m_editFrame( nullptr ), m_pcb( nullptr ),
        m_drcRuleEditorDlg( nullptr )
{
}


DRC_RULE_EDITOR_TOOL::~DRC_RULE_EDITOR_TOOL()
{
}


void DRC_RULE_EDITOR_TOOL::Reset( RESET_REASON aReason )
{
    m_editFrame = getEditFrame<PCB_EDIT_FRAME>();

    if( m_pcb != m_editFrame->GetBoard() )
    {
        if( m_drcRuleEditorDlg )
            DestroyDRCRuleEditorDialog();

        m_pcb = m_editFrame->GetBoard();
        m_drcEngine = m_pcb->GetDesignSettings().m_DRCEngine;
    }
}


bool DRC_RULE_EDITOR_TOOL::IsDRCRuleEditorDialogShown()
{
    if( m_drcRuleEditorDlg )
        return m_drcRuleEditorDlg->IsShownOnScreen();

    return false;
}


void DRC_RULE_EDITOR_TOOL::updatePointers()
{
    // update my pointers, m_editFrame is the only unchangeable one
    m_pcb = m_editFrame->GetBoard();

    if( m_drcRuleEditorDlg )
        m_drcRuleEditorDlg->UpdateData();
}


void DRC_RULE_EDITOR_TOOL::ShowDRCRuleEditorDialog( wxWindow* aParent )
{
    bool show_dlg_modal = true;

    // the dialog needs a parent frame. if it is not specified, this is the PCB editor frame
    // specified in DRC_RULE_EDITOR_TOOL class.
    if( !aParent )
    {
        // if any parent is specified, the dialog is modal.
        // if this is the default PCB editor frame, it is not modal
        show_dlg_modal = false;
        aParent = m_editFrame;
    }

    Activate();
    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear );

    if( !m_drcRuleEditorDlg )
    {
        m_drcRuleEditorDlg = new DIALOG_DRC_RULE_EDITOR( m_editFrame, aParent );
        updatePointers();

        if( show_dlg_modal )
            m_drcRuleEditorDlg->ShowModal();
        else
            m_drcRuleEditorDlg->Show( true );
    }
    else // The dialog is just not visible (because the user has double clicked on an error item)
    {
        updatePointers();
        m_drcRuleEditorDlg->Show( true );
    }
}


int DRC_RULE_EDITOR_TOOL::ShowDRCRuleEditorDialog( const TOOL_EVENT& aEvent )
{
    ShowDRCRuleEditorDialog( nullptr );
    return 0;
}


void DRC_RULE_EDITOR_TOOL::DestroyDRCRuleEditorDialog()
{
    if( m_drcRuleEditorDlg )
    {
        m_drcRuleEditorDlg->Destroy();
        m_drcRuleEditorDlg = nullptr;
    }
}


void DRC_RULE_EDITOR_TOOL::setTransitions()
{
    Go( &DRC_RULE_EDITOR_TOOL::ShowDRCRuleEditorDialog, PCB_ACTIONS::drcRuleEditor.MakeEvent() );
}