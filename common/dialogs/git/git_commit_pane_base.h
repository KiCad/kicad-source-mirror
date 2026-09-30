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
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <widgets/file_checklist_ctrl.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/sizer.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class GIT_COMMIT_PANE_BASE
///////////////////////////////////////////////////////////////////////////////
class GIT_COMMIT_PANE_BASE : public wxPanel
{
private:
protected:
    wxStaticText*        m_emptyLabel;
    FILE_CHECKLIST_CTRL* m_checklist;
    wxTextCtrl*          m_message;
    wxCheckBox*          m_amendCheckbox;
    wxButton*            m_commitOnlyButton;
    wxButton*            m_commitButton;
    wxButton*            m_pullButton;
    wxButton*            m_advancedButton;

public:
    GIT_COMMIT_PANE_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition,
                          const wxSize& size = wxSize( 500, 300 ), long style = wxTAB_TRAVERSAL,
                          const wxString& name = wxEmptyString );

    ~GIT_COMMIT_PANE_BASE();
};
