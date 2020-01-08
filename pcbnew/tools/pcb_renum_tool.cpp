#include <string>
#include <wx/wx.h>
#include <wx/filedlg.h>

#include <dialogs/DIALOG_BOARD_RENUM_base.h>
#include <dialogs/DIALOG_BOARD_RENUM.h>
#include <tools/pcb_renum_tool.h>

#ifndef  __linux__      //Include Windows functions
#include <windows.h>
#endif

PCB_RENUM_TOOL::PCB_RENUM_TOOL() :
        PCB_TOOL_BASE("pcbnew.RenumTool"), m_frame(nullptr) {
}

int LoadPCBFile(struct KiCadFile &PCBFile) {
    return (0);
}

bool PCB_RENUM_TOOL::Init() {
    return true;
}

void PCB_RENUM_TOOL::Reset(RESET_REASON aReason) {
    m_frame = getEditFrame<PCB_EDIT_FRAME>();
}

int PCB_RENUM_TOOL::ShowRenumDialog(const TOOL_EVENT& aEvent) {
    DIALOG_BOARD_RENUM dialog(m_frame);
    dialog.ShowModal();
    return 0;
}

void PCB_RENUM_TOOL::setTransitions() {
    Go(&PCB_RENUM_TOOL::ShowRenumDialog, PCB_ACTIONS::boardRenum.MakeEvent());
}

