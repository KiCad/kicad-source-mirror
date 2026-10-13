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
#include <wx/choice.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/sizer.h>
#include <widgets/wx_treebook.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_GIT_ADVANCED_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_GIT_ADVANCED_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxBoxSizer* m_topBar;
		wxStaticText* m_repoName;
		wxChoice* m_branchChoice;
		wxButton* m_fetchButton;
		wxButton* m_pullButton;
		wxButton* m_pushButton;
		WX_TREEBOOK* m_book;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerCancel;

	public:

		DIALOG_GIT_ADVANCED_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Advanced Git"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_GIT_ADVANCED_BASE();

};

