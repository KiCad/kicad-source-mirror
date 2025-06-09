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
#include "widgets/resettable_panel.h"
#include <wx/string.h>
#include <wx/checkbox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/stattext.h>
#include <wx/statline.h>
#include <wx/spinctrl.h>
#include <wx/gbsizer.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_GIT_REPOS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_GIT_REPOS_BASE : public RESETTABLE_PANEL
{
	private:

	protected:
		wxCheckBox* m_enableGit;
		wxBoxSizer* m_gitSizer;
		wxStaticText* m_staticText6;
		wxStaticLine* m_staticline2;
		wxStaticText* m_updateLabel;
		wxSpinCtrl* m_updateInterval;
		wxStaticText* m_staticText7;
		wxStaticText* m_staticText12;
		wxStaticLine* m_staticline31;
		wxCheckBox* m_cbDefault;
		wxStaticText* m_authorLabel;
		wxTextCtrl* m_author;
		wxStaticText* m_authorEmailLabel;
		wxTextCtrl* m_authorEmail;

		// Virtual event handlers, override them in your derived class
		virtual void onEnableGitClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void onDefaultClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_GIT_REPOS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_GIT_REPOS_BASE();

};

