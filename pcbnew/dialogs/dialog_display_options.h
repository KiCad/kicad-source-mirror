/**
 * @file pcbnew/dialogs/dialog_display_options.h
 */
#include <dialog_display_options_base.h>

class DIALOG_DISPLAY_OPTIONS : public DIALOG_DISPLAY_OPTIONS_BASE
{
private:
   PCB_EDIT_FRAME* m_Parent;

   void init();

public:
   DIALOG_DISPLAY_OPTIONS( PCB_EDIT_FRAME* parent );
   ~DIALOG_DISPLAY_OPTIONS( ) { };
   void OnOkClick( wxCommandEvent& event );
   void OnCancelClick( wxCommandEvent& event );
};

