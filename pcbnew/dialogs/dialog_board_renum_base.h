///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 21 2019)
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
#include <wx/choice.h>
#include <wx/textctrl.h>
#include <wx/valgen.h>
#include <wx/checkbox.h>
#include <wx/radiobut.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_BOARD_RENUM_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_BOARD_RENUM_BASE : public wxDialog
{
	private:

	protected:
		wxStaticText* m_FrontSortDirText;
		wxChoice* m_SortDir;
		wxStaticText* m_Dummy;
		wxStaticText* m_staticText15;
		wxStaticText* m_FrontRefDesStartText;
		wxTextCtrl* m_FrontRefDesStart;
		wxStaticText* m_BottomRefDesStartText;
		wxTextCtrl* m_BackRefDesStart;
		wxStaticText* m_FrontPrefixText;
		wxTextCtrl* m_FrontPrefix;
		wxStaticText* m_BackPrefixText;
		wxTextCtrl* m_BackPrefix;
		wxCheckBox* m_RemoveFrontPrefix;
		wxStaticText* m_staticText31;
		wxCheckBox* m_RemoveBackPrefix;
		wxStaticText* m_staticText35;
		wxStaticText* m_SortGridText;
		wxTextCtrl* m_SortGrid;
		wxRadioButton* m_SortOnModules;
		wxRadioButton* m_SortOnRefDes;
		wxStaticText* m_ExcludeListText;
		wxTextCtrl* m_ExcludeList;
		wxCheckBox* m_WriteChangeFile;
		wxCheckBox* m_WriteLogFile;
		wxTextCtrl* m_MessageWindow;
		wxButton* m_RenumberButton;
		wxStaticText* m_staticText38;
		wxStaticText* m_staticText381;
		wxButton* m_ExitButton;

		// Virtual event handlers, overide them in your derived class
		virtual void OnRenumberClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OKDone( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_BOARD_RENUM_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Geographic Reannotaion "), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE );
		~DIALOG_BOARD_RENUM_BASE();

};

