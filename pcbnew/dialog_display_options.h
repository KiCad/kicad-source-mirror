/**
 * @file dialog_display_options.h
 */
#include "dialog_display_options_base.h"

class Dialog_Display_Options : public DialogDisplayOptions_base
{
private:
   WinEDA_BasePcbFrame* m_Parent;

   void init();

public:
   Dialog_Display_Options( WinEDA_BasePcbFrame* parent );
   Dialog_Display_Options(WinEDA_BasePcbFrame*, wxWindow*);
   ~Dialog_Display_Options( ) { };
   void OnOkClick( wxCommandEvent& event );
   void OnCancelClick( wxCommandEvent& event );
};

