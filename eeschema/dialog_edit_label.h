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

protected:
    // these are protected so that the static ShowModally() gets used.
    DialogLabelEditor( WinEDA_SchematicFrame* parent, SCH_TEXT * CurrentText);
    ~DialogLabelEditor(){};


public:

    /**
     * Function ShowModally
     * is a static function that constructs and then displays one of these dialogs.
     * @param parent
     * @param CurrentText is one of several classes derived from SCH_TEXT
     * @return int - the result Dialog::ShowModal()
     */
    static int ShowModally(  WinEDA_SchematicFrame* parent, SCH_TEXT * CurrentText );

private:
    void init( );
	void onEnterKey( wxCommandEvent& event );
    void OnButtonOKClick( wxCommandEvent& event );
    void OnButtonCANCEL_Click( wxCommandEvent& event );
    void TextPropertiesAccept( wxCommandEvent& event );
};


#endif    // _DIALOG_EDIT_LABEL_H_
