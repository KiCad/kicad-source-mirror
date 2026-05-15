///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/radiobox.h>
#include <wx/textctrl.h>
#include <wx/filepicker.h>
#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_GIT_CREDENTIALS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_GIT_CREDENTIALS_BASE : public DIALOG_SHIM
{
private:
protected:
    wxStaticText*           m_promptLabel;
    wxStaticText*           m_urlLabel;
    wxRadioBox*             m_authChoice;
    wxStaticText*           m_userLabel;
    wxTextCtrl*             m_userCtrl;
    wxStaticText*           m_passLabel;
    wxTextCtrl*             m_passCtrl;
    wxStaticText*           m_keyLabel;
    wxFilePickerCtrl*       m_keyPicker;
    wxCheckBox*             m_saveCheck;
    wxStdDialogButtonSizer* m_sdbSizer1;
    wxButton*               m_sdbSizer1OK;
    wxButton*               m_sdbSizer1Cancel;

    // Virtual event handlers, override them in your derived class
    virtual void OnConnTypeChanged( wxCommandEvent& event ) { event.Skip(); }


public:
    DIALOG_GIT_CREDENTIALS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY,
                                 const wxString& title = _( "Git Authentication Required" ),
                                 const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                                 long style = wxDEFAULT_DIALOG_STYLE );

    ~DIALOG_GIT_CREDENTIALS_BASE();
};
