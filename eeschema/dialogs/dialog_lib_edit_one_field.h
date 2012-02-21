
#ifndef _DIALOG_LIB_EDIT_ONE_FIELD_H_
#define _DIALOG_LIB_EDIT_ONE_FIELD_H_


#include <dialog_lib_edit_text_base.h>


class LIB_EDIT_FRAME;
class LIB_FIELD;


class DIALOG_LIB_EDIT_ONE_FIELD : public DIALOG_LIB_EDIT_TEXT_BASE
{
private:
    LIB_EDIT_FRAME* m_parent;
    LIB_FIELD* m_field;

public:
    DIALOG_LIB_EDIT_ONE_FIELD( LIB_EDIT_FRAME* aParent, const wxString& aTitle, LIB_FIELD* aField );
    ~DIALOG_LIB_EDIT_ONE_FIELD() {};
    void TransfertDataToField();
    wxString GetTextField();

private:
    void initDlg( );
    void OnOkClick( wxCommandEvent& aEvent );
    void OnCancelClick( wxCommandEvent& aEvent );
};


#endif    // _DIALOG_LIB_EDIT_ONE_FIELD_H_
