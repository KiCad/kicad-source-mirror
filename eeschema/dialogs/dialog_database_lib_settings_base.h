///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b3)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class WX_INFOBAR;

#include "dialog_shim.h"
#include <wx/infobar.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/radiobut.h>
#include <wx/textctrl.h>
#include <wx/stattext.h>
#include <wx/gbsizer.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/statbox.h>
#include <wx/spinctrl.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_DATABASE_LIB_SETTINGS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_DATABASE_LIB_SETTINGS_BASE : public DIALOG_SHIM
{
	private:

	protected:
		WX_INFOBAR* m_infoBar;
		wxRadioButton* m_rbDSN;
		wxTextCtrl* m_txtDSN;
		wxStaticText* m_staticText2;
		wxTextCtrl* m_txtUser;
		wxStaticText* m_staticText3;
		wxTextCtrl* m_txtPassword;
		wxRadioButton* m_rbConnectionString;
		wxTextCtrl* m_txtConnectionString;
		wxButton* m_btnTest;
		wxButton* m_btnReloadConfig;
		wxStaticText* m_staticText5;
		wxSpinCtrl* m_spinCacheSize;
		wxStaticText* m_staticText6;
		wxSpinCtrl* m_spinCacheTimeout;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnDSNSelected( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnConnectionStringSelected( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnBtnTest( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnBtnReloadConfig( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCloseClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnApplyClick( wxCommandEvent& event ) { event.Skip(); }


	public:
		wxStaticText* m_stConnectionTestStatus;
		wxStaticText* m_stLibrariesStatus;

		DIALOG_DATABASE_LIB_SETTINGS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Database Library Settings"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 500,600 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_DATABASE_LIB_SETTINGS_BASE();

};

