/////////////////////////////////////////////////////////////////////////////

// Name:        dialog_display_options.h
// Author:      jean-pierre Charras
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////

#ifndef _DIALOG_CVPCB_CONFIG_H_
#define _DIALOG_CVPCB_CONFIG_H_

#include <dialog_cvpcb_config_fbp.h>

class DIALOG_CVPCB_CONFIG : public DIALOG_CVPCB_CONFIG_FBP
{
private:
    CVPCB_MAINFRAME* m_Parent;
    wxConfigBase*    m_Config;
    wxString         m_UserLibDirBufferImg;

    bool m_LibListChanged;
    bool m_LibPathChanged;

private:
    void Init();

    // Virtual event handlers
    void OnCloseWindow( wxCloseEvent& event );
    void OnOkClick( wxCommandEvent& event );
    void OnCancelClick( wxCommandEvent& event );
    void OnAddOrInsertLibClick( wxCommandEvent& event );
    void OnRemoveLibClick( wxCommandEvent& event );
    void OnBrowseModDocFile( wxCommandEvent& event );
    void OnAddOrInsertPath( wxCommandEvent& event );
    void OnRemoveUserPath( wxCommandEvent& event );
    void OnButtonUpClick( wxCommandEvent& event );
    void OnButtonDownClick( wxCommandEvent& event );


public:
    DIALOG_CVPCB_CONFIG( CVPCB_MAINFRAME* parent );
    ~DIALOG_CVPCB_CONFIG() {};
};

#endif

// _DIALOG_CVPCB_CONFIG_H_
