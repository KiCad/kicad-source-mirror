#ifndef __dialog_general_options_h
#define __dialog_general_options_h

#include <dialog_general_options_BoardEditor_base.h>

class DIALOG_GENERALOPTIONS : public DIALOG_GENERALOPTIONS_BOARDEDITOR_BASE
{
private:
    BOARD* m_Board;

    void init();

public:
    DIALOG_GENERALOPTIONS( PCB_EDIT_FRAME* parent );
    ~DIALOG_GENERALOPTIONS() {};
    void OnOkClick( wxCommandEvent& event );
    void OnCancelClick( wxCommandEvent& event );

    PCB_EDIT_FRAME* GetParent() const { return (PCB_EDIT_FRAME*) wxDialog::GetParent(); }

private:
    void OnMiddleBtnPanEnbl( wxCommandEvent& event )
    {
        m_OptMiddleButtonPanLimited->Enable( m_MiddleButtonPANOpt->GetValue() );
    }
};


#endif	// __dialog_general_options_h
