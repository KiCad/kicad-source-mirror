/////////////////////////////////////////////////////////////////////////////
// Name:        dialog_freeroute_exchange.h
/////////////////////////////////////////////////////////////////////////////


#ifndef _DIALOG_FREEROUTE_EXCHANGE_H_
#define _DIALOG_FREEROUTE_EXCHANGE_H_

#include <dialog_freeroute_exchange_base.h>

///////////////////////////////////////////////////////////////////////////////
// Class DIALOG_FREEROUTE derived from wxFormBuilder class DIALOG_FREEROUTE_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_FREEROUTE : public DIALOG_FREEROUTE_BASE
{
private:
    PCB_EDIT_FRAME* m_Parent;
    bool m_FreeRouteSetupChanged;
    bool m_freeRouterIsLocal;

private:
    // Virtual event handlers
    void OnOKButtonClick( wxCommandEvent& event );
    void OnExportButtonClick( wxCommandEvent& event );
    void OnLaunchButtonClick( wxCommandEvent& event );
    void OnImportButtonClick( wxCommandEvent& event );
    void OnVisitButtonClick( wxCommandEvent& event );
    void OnHelpButtonClick( wxCommandEvent& event );
    void OnCancelButtonClick( wxCommandEvent& event );
    void OnTextEditFrUrlUpdated( wxCommandEvent& event );
    void MyInit ( );
    wxString CmdRunFreeRouterLocal();

public:
    DIALOG_FREEROUTE( PCB_EDIT_FRAME* parent );
    ~DIALOG_FREEROUTE() {};

};

#endif

// _DIALOG_FREEROUTE_EXCHANGE_H_
