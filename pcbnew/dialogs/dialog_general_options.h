#ifndef __dialog_general_options_h
#define __dialog_general_options_h

#include "dialog_general_options_BoardEditor_base.h"

class Dialog_GeneralOptions : public DialogGeneralOptionsBoardEditor_base
{
private:
    BOARD* m_Board;

    void init();

public:
    Dialog_GeneralOptions( PCB_EDIT_FRAME* parent );
    ~Dialog_GeneralOptions() {};
    void OnOkClick( wxCommandEvent& event );
    void OnCancelClick( wxCommandEvent& event );

    PCB_EDIT_FRAME* GetParent() { return (PCB_EDIT_FRAME*) wxDialog::GetParent(); }
};


#endif	// __dialog_general_options_h
