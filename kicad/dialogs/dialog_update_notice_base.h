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
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/statbmp.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_UPDATE_NOTICE_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_UPDATE_NOTICE_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticBitmap* m_icon;
		wxStaticText* m_messageLine1;
		wxStaticText* m_messageLine2;
		wxButton* m_skipBtn;
		wxButton* m_btnRemind;
		wxButton* m_btnDetailsPage;
		wxButton* m_btnDownloadPage;

		// Virtual event handlers, override them in your derived class
		virtual void OnSkipThisVersionClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnBtnRemindMeClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnBtnDetailsPageClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnBtnDownloadsPageClicked( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_UPDATE_NOTICE_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Update Available"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_UPDATE_NOTICE_BASE();

};

