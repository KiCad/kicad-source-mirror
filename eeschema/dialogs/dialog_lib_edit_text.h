
#ifndef _DIALOG_LIB_EDIT_TEXT_H_
#define _DIALOG_LIB_EDIT_TEXT_H_


#include "dialog_lib_edit_text_base.h"


class WinEDA_LibeditFrame;
class LIB_TEXT;


class DIALOG_LIB_EDIT_TEXT : public DIALOG_LIB_EDIT_TEXT_BASE
{
private:
    WinEDA_LibeditFrame* m_Parent;
    LIB_TEXT* m_GraphicText;

public:
    DIALOG_LIB_EDIT_TEXT( WinEDA_LibeditFrame* aParent, LIB_TEXT* aText );
    ~DIALOG_LIB_EDIT_TEXT() {};

private:
    void InitDialog( );
    void OnOkClick( wxCommandEvent& aEvent );
    void OnCancelClick( wxCommandEvent& aEvent );
};


#endif    // _DIALOG_LIB_EDIT_TEXT_H_
