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
class WX_HTML_REPORT_PANEL;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/statline.h>
#include <wx/radiobut.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/statbmp.h>
#include <wx/sizer.h>
#include <wx/choice.h>
#include <wx/checkbox.h>
#include <wx/textctrl.h>
#include <wx/valgen.h>
#include <wx/gbsizer.h>
#include <wx/panel.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_BOARD_REANNOTATE_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_BOARD_REANNOTATE_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticLine* m_staticline1;
		wxRadioButton* m_Down_Right;
		wxStaticBitmap* reannotate_down_right_bitmap;
		wxRadioButton* m_Right_Down;
		wxStaticBitmap* reannotate_right_down_bitmap;
		wxRadioButton* m_Down_Left;
		wxStaticBitmap* reannotate_down_left_bitmap;
		wxRadioButton* m_Left_Down;
		wxStaticBitmap* reannotate_left_down_bitmap;
		wxRadioButton* m_Up_Right;
		wxStaticBitmap* reannotate_up_right_bitmap;
		wxRadioButton* m_Right_Up;
		wxStaticBitmap* reannotate_right_up_bitmap;
		wxRadioButton* m_Up_Left;
		wxStaticBitmap* reannotate_up_left_bitmap;
		wxRadioButton* m_Left_Up;
		wxStaticBitmap* reannotate_left_up_bitmap;
		wxStaticText* m_staticText9;
		wxChoice* m_locationChoice;
		wxStaticText* m_SortGridText;
		wxChoice* m_GridChoice;
		wxStaticLine* m_staticline2;
		wxRadioButton* m_AnnotateAll;
		wxRadioButton* m_AnnotateFront;
		wxRadioButton* m_AnnotateBack;
		wxRadioButton* m_AnnotateSelection;
		wxCheckBox* m_ExcludeLocked;
		wxStaticText* m_ExcludeListText;
		wxTextCtrl* m_ExcludeList;
		wxStaticText* m_staticText10;
		wxStaticLine* m_staticline3;
		wxStaticText* m_FrontRefDesStartText;
		wxTextCtrl* m_FrontRefDesStart;
		wxStaticText* m_BottomRefDesStartText;
		wxTextCtrl* m_BackRefDesStart;
		wxStaticText* m_FrontPrefixText;
		wxTextCtrl* m_FrontPrefix;
		wxStaticText* m_BackPrefixText;
		wxTextCtrl* m_BackPrefix;
		wxCheckBox* m_RemoveFrontPrefix;
		wxCheckBox* m_RemoveBackPrefix;
		WX_HTML_REPORT_PANEL* m_MessageWindow;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void FilterFrontPrefix( wxCommandEvent& event ) { event.Skip(); }
		virtual void FilterBackPrefix( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCloseClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnApplyClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_BOARD_REANNOTATE_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Geographical Reannotation"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_BOARD_REANNOTATE_BASE();

};

