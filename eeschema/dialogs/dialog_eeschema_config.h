
#ifndef _DIALOG_EESCHEMA_CONFIG_H_
#define _DIALOG_EESCHEMA_CONFIG_H_


#include "dialog_eeschema_config_fbp.h"


class SCH_EDIT_FRAME;
class EDA_DRAW_FRAME;


class DIALOG_EESCHEMA_CONFIG : public DIALOG_EESCHEMA_CONFIG_FBP
{
private:
    SCH_EDIT_FRAME* m_Parent;
    bool m_LibListChanged;
    bool m_LibPathChanged;
    wxString m_UserLibDirBufferImg;      // Copy of original g_UserLibDirBuffer

private:

    // event handlers, overiding the fbp handlers
    void Init();
    void OnCloseWindow( wxCloseEvent& event );
    void OnRemoveLibClick( wxCommandEvent& event );
    void OnAddOrInsertLibClick( wxCommandEvent& event );
    void OnAddOrInsertPath( wxCommandEvent& event );
	void OnOkClick( wxCommandEvent& event );
	void OnCancelClick( wxCommandEvent& event );
    void OnRemoveUserPath( wxCommandEvent& event );
	void OnButtonUpClick( wxCommandEvent& event );
	void OnButtonDownClick( wxCommandEvent& event );


public:
    DIALOG_EESCHEMA_CONFIG( SCH_EDIT_FRAME* parent, EDA_DRAW_FRAME* activeWindow );
    ~DIALOG_EESCHEMA_CONFIG() {};
};


#endif    // _DIALOG_EESCHEMA_CONFIG_H_
