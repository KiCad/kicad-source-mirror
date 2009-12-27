/////////////////////////////////////////////////////////////////////////////
// Name:        dialog_edit_label.h
// Author:      jean-pierre Charras
// Modified by:
// Licence: GPL
/////////////////////////////////////////////////////////////////////////////

#ifndef _DIALOG_EDIT_LABEL_H_
#define _DIALOG_EDIT_LABEL_H_

#include "dialog_edit_label_base.h"

class DialogLabelEditor : public DialogLabelEditor_Base
{
private:
    WinEDA_SchematicFrame * m_Parent;
    SCH_TEXT * m_CurrentText;
    wxTextCtrl* m_TextLabel;

public:
    DialogLabelEditor( WinEDA_SchematicFrame* parent, SCH_TEXT * CurrentText);
    ~DialogLabelEditor(){};


public:

private:
    void InitDialog( );
  	void onEnterKey( wxCommandEvent& event );
    void OnButtonOKClick( wxCommandEvent& event );
    void OnButtonCANCEL_Click( wxCommandEvent& event );
    void TextPropertiesAccept( wxCommandEvent& event );
};


#endif    // _DIALOG_EDIT_LABEL_H_
