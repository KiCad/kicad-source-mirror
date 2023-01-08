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
class STD_BITMAP_BUTTON;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/statline.h>
#include <wx/choice.h>
#include <wx/textctrl.h>
#include <wx/valtext.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/statbmp.h>
#include <wx/bmpbuttn.h>
#include <wx/button.h>
#include <wx/datectrl.h>
#include <wx/dateevt.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_PAGES_SETTINGS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_PAGES_SETTINGS_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_staticTextPaper;
		wxStaticLine* m_staticline2;
		wxStaticText* m_staticTextSize;
		wxChoice* m_paperSizeComboBox;
		wxStaticText* m_staticTextOrient;
		wxChoice* m_orientationComboBox;
		wxStaticText* m_staticTextCustSize;
		wxStaticText* m_userSizeYLabel;
		wxTextCtrl* m_userSizeYCtrl;
		wxStaticText* m_userSizeYUnits;
		wxStaticText* m_userSizeXLabel;
		wxTextCtrl* m_userSizeXCtrl;
		wxStaticText* m_userSizeXUnits;
		wxCheckBox* m_PaperExport;
		wxStaticText* m_staticTextPreview;
		wxStaticLine* m_staticline31;
		wxStaticBitmap* m_PageLayoutExampleBitmap;
		wxStaticText* m_staticTextDrawingSheet;
		wxStaticLine* m_staticline4;
		wxStaticText* m_staticText30;
		wxTextCtrl* m_textCtrlFilePicker;
		STD_BITMAP_BUTTON* m_browseButton;
		wxStaticText* m_staticTextTitleBlock;
		wxStaticLine* m_staticline3;
		wxStaticText* m_TextSheetCount;
		wxStaticText* m_TextSheetNumber;
		wxStaticText* m_staticTextDate;
		wxTextCtrl* m_TextDate;
		wxButton* m_ApplyDate;
		wxDatePickerCtrl* m_PickDate;
		wxCheckBox* m_DateExport;
		wxStaticText* m_staticTextRev;
		wxTextCtrl* m_TextRevision;
		wxCheckBox* m_RevisionExport;
		wxStaticText* m_staticTextTitle;
		wxTextCtrl* m_TextTitle;
		wxCheckBox* m_TitleExport;
		wxStaticText* m_staticTextCompany;
		wxTextCtrl* m_TextCompany;
		wxCheckBox* m_CompanyExport;
		wxStaticText* m_staticTextComment1;
		wxTextCtrl* m_TextComment1;
		wxCheckBox* m_Comment1Export;
		wxStaticText* m_staticTextComment2;
		wxTextCtrl* m_TextComment2;
		wxCheckBox* m_Comment2Export;
		wxStaticText* m_staticTextComment3;
		wxTextCtrl* m_TextComment3;
		wxCheckBox* m_Comment3Export;
		wxStaticText* m_staticTextComment4;
		wxTextCtrl* m_TextComment4;
		wxCheckBox* m_Comment4Export;
		wxStaticText* m_staticTextComment5;
		wxTextCtrl* m_TextComment5;
		wxCheckBox* m_Comment5Export;
		wxStaticText* m_staticTextComment6;
		wxTextCtrl* m_TextComment6;
		wxCheckBox* m_Comment6Export;
		wxStaticText* m_staticTextComment7;
		wxTextCtrl* m_TextComment7;
		wxCheckBox* m_Comment7Export;
		wxStaticText* m_staticTextComment8;
		wxTextCtrl* m_TextComment8;
		wxCheckBox* m_Comment8Export;
		wxStaticText* m_staticTextComment9;
		wxTextCtrl* m_TextComment9;
		wxCheckBox* m_Comment9Export;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnPaperSizeChoice( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPageOrientationChoice( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnUserPageSizeYTextUpdated( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnUserPageSizeXTextUpdated( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnWksFileSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDateTextUpdated( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDateApplyClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRevisionTextUpdated( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTitleTextUpdated( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCheckboxTitleClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCompanyTextUpdated( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnComment1TextUpdated( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnComment2TextUpdated( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnComment3TextUpdated( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnComment4TextUpdated( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnComment5TextUpdated( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnComment6TextUpdated( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnComment7TextUpdated( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnComment8TextUpdated( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnComment9TextUpdated( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOkClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_PAGES_SETTINGS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Page Settings"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_PAGES_SETTINGS_BASE();

};

