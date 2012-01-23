/////////////////////////////////////////////////////////////////////////////
// Name:        dialog_edit_label.h
// Author:      jean-pierre Charras
// Modified by:
// Licence: GPL
/////////////////////////////////////////////////////////////////////////////

#ifndef _DIALOG_EDIT_LABEL_H_
#define _DIALOG_EDIT_LABEL_H_

#include <dialog_edit_label_base.h>


class SCH_EDIT_FRAME;
class SCH_TEXT;


class DialogLabelEditor : public DialogLabelEditor_Base
{
private:
    SCH_EDIT_FRAME* m_Parent;
    SCH_TEXT*       m_CurrentText;
    wxTextCtrl*     m_textLabel;

public:
    DialogLabelEditor( SCH_EDIT_FRAME* parent, SCH_TEXT* aTextItem );
    ~DialogLabelEditor(){};


public:

private:
    void InitDialog( );
    virtual void OnEnterKey( wxCommandEvent& aEvent );
    virtual void OnOkClick( wxCommandEvent& aEvent );
    virtual void OnCancelClick( wxCommandEvent& aEvent );
    void TextPropertiesAccept( wxCommandEvent& aEvent );
};


#endif    // _DIALOG_EDIT_LABEL_H_
