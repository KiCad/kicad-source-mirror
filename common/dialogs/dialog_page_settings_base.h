///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Nov  6 2013)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_PAGE_SETTINGS_BASE_H__
#define __DIALOG_PAGE_SETTINGS_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class DIALOG_SHIM;

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
#include <wx/sizer.h>
#include <wx/valtext.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/statbmp.h>
#include <wx/button.h>
#include <wx/datectrl.h>
#include <wx/dateevt.h>
#include <wx/checkbox.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

#define ID_CHOICE_PAGE_ORIENTATION 1000
#define ID_TEXTCTRL_USER_PAGE_SIZE_Y 1001
#define ID_TEXTCTRL_USER_PAGE_SIZE_X 1002
#define ID_TEXTCTRL_DATE 1003
#define ID_BTN_APPLY_DATE 1004
#define ID_PICKER_DATE 1005
#define ID_CHECKBOX_DATE 1006
#define ID_TEXTCTRL_REVISION 1007
#define ID_CHECKBOX_REVISION 1008
#define ID_TEXTCTRL_TITLE 1009
#define ID_TEXTCTRL_COMPANY 1010
#define ID_CHECKBOX_COMPANY 1011
#define ID_TEXTCTRL_COMMENT1 1012
#define ID_CHECKBOX_COMMENT1 1013
#define ID_TEXTCTRL_COMMENT2 1014
#define ID_CHECKBOX_COMMENT2 1015
#define ID_TEXTCTRL_COMMENT3 1016
#define ID_CHECKBOX_COMMENT3 1017
#define ID_TEXTCTRL_COMMENT4 1018
#define ID_CHECKBOX_COMMENT4 1019

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
		wxStaticText* m_staticTextHeight;
		wxTextCtrl* m_TextUserSizeY;
		wxStaticText* m_staticTextWidth;
		wxTextCtrl* m_TextUserSizeX;
		wxStaticText* m_staticTextPreview;
		wxStaticBitmap* m_PageLayoutExampleBitmap;
		wxStaticLine* m_staticline1;
		wxStaticText* m_staticTexttbprm;
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
		wxStaticText* m_staticText13;
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
		wxStaticText* m_staticTextfilename;
		wxTextCtrl* m_textCtrlFilePicker;
		wxButton* m_buttonBrowse;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnPaperSizeChoice( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPageOrientationChoice( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnUserPageSizeYTextUpdated( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnUserPageSizeXTextUpdated( wxCommandEvent& event ) { event.Skip(); }
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
		virtual void OnWksFileSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOkClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_PAGES_SETTINGS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Page Settings"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 748,470 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_PAGES_SETTINGS_BASE();
	
};

#endif //__DIALOG_PAGE_SETTINGS_BASE_H__
