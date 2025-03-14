///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a-dirty)
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
#include <wx/statline.h>
#include <wx/textctrl.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/filepicker.h>
#include <wx/panel.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_GIT_REPOSITORY_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_GIT_REPOSITORY_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxBoxSizer* bSizerMain;
		wxStaticText* m_staticText1;
		wxStaticLine* m_staticline1;
		wxStaticText* m_staticText3;
		wxTextCtrl* m_txtName;
		wxStaticText* m_staticText4;
		wxTextCtrl* m_txtURL;
		wxStaticText* m_staticText9;
		wxChoice* m_ConnType;
		wxPanel* m_panelAuth;
		wxStaticText* m_staticText2;
		wxStaticLine* m_staticline2;
		wxCheckBox* m_cbCustom;
		wxFilePickerCtrl* m_fpSSHKey;
		wxStaticText* m_staticText11;
		wxTextCtrl* m_txtUsername;
		wxStaticText* m_labelPass1;
		wxTextCtrl* m_txtPassword;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;
		wxButton* m_sdbSizerHelp;

		// Virtual event handlers, override them in your derived class
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnUpdateUI( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void OnLocationExit( wxFocusEvent& event ) { event.Skip(); }
		virtual void OnSelectConnType( wxCommandEvent& event ) { event.Skip(); }
		virtual void onCbCustom( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnFileUpdated( wxFileDirPickerEvent& event ) { event.Skip(); }
		virtual void OnTestClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOKClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_GIT_REPOSITORY_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Git Repository"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxCAPTION|wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_GIT_REPOSITORY_BASE();

};

