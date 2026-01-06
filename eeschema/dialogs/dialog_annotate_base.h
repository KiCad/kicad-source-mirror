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
class WX_HTML_REPORT_PANEL;
class WX_INFOBAR;

#include "dialog_shim.h"
#include <wx/infobar.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/radiobut.h>
#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/statbmp.h>
#include <wx/textctrl.h>
#include <wx/gbsizer.h>
#include <wx/panel.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

#define ID_SORT_BY_X_POSITION 1000
#define ID_SORT_BY_Y_POSITION 1001
#define wxID_FIRST_FREE 1002
#define wxID_SHEET_X_100 1003
#define wxID_SHEET_X_1000 1004
#define ID_CLEAR_ANNOTATION_CMP 1005

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_ANNOTATE_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_ANNOTATE_BASE : public DIALOG_SHIM
{
	private:

	protected:
		WX_INFOBAR* m_infoBar;
		wxRadioButton* m_rbScope_Schematic;
		wxRadioButton* m_rbScope_Sheet;
		wxRadioButton* m_rbScope_Selection;
		wxCheckBox* m_checkRecursive;
		wxRadioButton* m_rbSortBy_X_Position;
		wxStaticBitmap* annotate_down_right_bitmap;
		wxRadioButton* m_rbSortBy_Y_Position;
		wxStaticBitmap* annotate_right_down_bitmap;
		wxRadioButton* m_rbKeep_Annotations;
		wxRadioButton* m_rbReset_Annotations;
		wxCheckBox* m_checkRegroupUnits;
		wxRadioButton* m_rbFirstFree;
		wxTextCtrl* m_textNumberAfter;
		wxRadioButton* m_rbSheetX100;
		wxRadioButton* m_rbSheetX1000;
		WX_HTML_REPORT_PANEL* m_MessageWindow;
		wxButton* m_btnClear;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnOptionChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnClearAnnotationClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCloseClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnAnnotateClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_ANNOTATE_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Annotate Schematic"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_ANNOTATE_BASE();

};

