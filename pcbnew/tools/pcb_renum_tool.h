#ifndef PCBRENUMTOOL_H_
#define PCBRENUMTOOL_H_

#include    <dialogs/dialog_board_renum.h>
#include    <pcb_edit_frame.h>
#include    <tools/pcb_actions.h>
#include    <tools/pcb_tool_base.h>

class PCB_RENUM_TOOL: public wxEvtHandler, public PCB_TOOL_BASE {
public:
    PCB_RENUM_TOOL();

    bool Init() override;
    void Reset(RESET_REASON aReason) override;
    int ShowRenumDialog(const TOOL_EVENT& aEvent);
//
private:
    void setTransitions() override; //> Bind handlers to corresponding TOOL_ACTIONs

private:
    PCB_EDIT_FRAME* m_frame;    // Pointer to the currently used edit frame.
};

#endif /* PCBRENUMTOOL_H_ */
