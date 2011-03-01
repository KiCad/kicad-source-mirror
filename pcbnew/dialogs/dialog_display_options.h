/**
 * @file pcbnew/dialogs/dialog_display_options.h
 */
#include "dialog_display_options_base.h"

class Dialog_Display_Options : public DialogDisplayOptions_base
{
private:
   PCB_EDIT_FRAME* m_Parent;

   void init();

public:
   Dialog_Display_Options( PCB_EDIT_FRAME* parent );
   ~Dialog_Display_Options( ) { };
   void OnOkClick( wxCommandEvent& event );
   void OnCancelClick( wxCommandEvent& event );
};

