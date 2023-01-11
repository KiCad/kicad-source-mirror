///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
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
#include <wx/radiobut.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/statbmp.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>
#include <wx/gbsizer.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

#define ID_SORT_BY_X_POSITION 1000
#define ID_SORT_BY_Y_POSITION 1001
#define wxID_FIRST_FREE 1002
#define wxID_SHEET_X_100 1003
#define wxID_SHEET_X_1000 1004

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_EESCHEMA_ANNOTATION_OPTIONS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_EESCHEMA_ANNOTATION_OPTIONS_BASE : public RESETTABLE_PANEL
{
	private:

	protected:
		wxCheckBox* m_checkAutoAnnotate;
		wxStaticText* m_orderLabel;
		wxStaticLine* m_staticline2;
		wxRadioButton* m_rbSortBy_X_Position;
		wxStaticBitmap* annotate_down_right_bitmap;
		wxRadioButton* m_rbSortBy_Y_Position;
		wxStaticBitmap* annotate_right_down_bitmap;
		wxStaticText* m_numberingLabel;
		wxStaticLine* m_staticline3;
		wxRadioButton* m_rbFirstFree;
		wxTextCtrl* m_textNumberAfter;
		wxRadioButton* m_rbSheetX100;
		wxRadioButton* m_rbSheetX1000;

		// Virtual event handlers, override them in your derived class
		virtual void OnOptionChanged( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_EESCHEMA_ANNOTATION_OPTIONS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_EESCHEMA_ANNOTATION_OPTIONS_BASE();

};

