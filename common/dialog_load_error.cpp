#include "dialog_load_error.h"

DIALOG_LOAD_ERROR::DIALOG_LOAD_ERROR( wxWindow* parent )
:
DIALOG_LOAD_ERROR_BASE( parent )
{

}

void DIALOG_LOAD_ERROR::OnOkClick( wxCommandEvent& event )
{
	Destroy();
}


void DIALOG_LOAD_ERROR::ListClear(void)
{
	TextCtrlList->Clear();
}

void DIALOG_LOAD_ERROR::ListSet(wxString *list)
{
	wxString list_value = *list;
	TextCtrlList->SetValue(list_value);
}

void DIALOG_LOAD_ERROR::MessageSet(wxString *message)
{
	wxString message_value = *message;
	StaticTextMessage->SetLabel(message_value);
}

