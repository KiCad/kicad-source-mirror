
#ifndef _DIALOG_LIB_EDIT_TEXT_H_
#define _DIALOG_LIB_EDIT_TEXT_H_


#include "dialog_lib_edit_text_base.h"


class LIB_EDIT_FRAME;
class LIB_TEXT;


class DIALOG_LIB_EDIT_TEXT : public DIALOG_LIB_EDIT_TEXT_BASE
{
private:
    LIB_EDIT_FRAME* m_Parent;
    LIB_TEXT* m_GraphicText;

public:
    DIALOG_LIB_EDIT_TEXT( LIB_EDIT_FRAME* aParent, LIB_TEXT* aText );
    ~DIALOG_LIB_EDIT_TEXT() {};

private:
    void initDlg( );
    void OnOkClick( wxCommandEvent& aEvent );
    void OnCancelClick( wxCommandEvent& aEvent );
};


#endif    // _DIALOG_LIB_EDIT_TEXT_H_
