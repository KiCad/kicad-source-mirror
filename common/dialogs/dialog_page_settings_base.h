///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Feb  9 2012)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_PAGE_SETTINGS_BASE_H__
#define __DIALOG_PAGE_SETTINGS_BASE_H__

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
#include <wx/valtext.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/statbmp.h>
#include <wx/checkbox.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

#define ID_CHICE_PAGE_SIZE 1000
#define ID_CHOICE_PAGE_ORIENTATION 1001
#define ID_TEXTCTRL_USER_PAGE_SIZE_X 1002
#define ID_TEXTCTRL_USER_PAGE_SIZE_Y 1003
#define ID_PAGE_LAYOUT_EXAMPLE_SIZER 1004
#define ID_TEXTCTRL_REVISION 1005
#define ID_CHECKBOX_REVISION 1006
#define ID_TEXTCTRL_TITLE 1007
#define ID_TEXTCTRL_COMPANY 1008
#define ID_CHECKBOX_COMPANY 1009
#define ID_TEXTCTRL_COMMENT1 1010
#define ID_CHECKBOX_COMMENT1 1011
#define ID_TEXTCTRL_COMMENT2 1012
#define ID_CHECKBOX_COMMENT2 1013
#define ID_TEXTCTRL_COMMENT3 1014
#define ID_CHECKBOX_COMMENT3 1015
#define ID_TEXTCTRL_COMMENT4 1016
#define ID_CHECKBOX_COMMENT4 1017

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_PAGES_SETTINGS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_PAGES_SETTINGS_BASE : public wxDialog 
{
	private:
	
	protected:
		wxStaticText* m_staticText5;
		wxChoice* m_paperSizeComboBox;
		wxStaticText* m_staticText6;
		wxChoice* m_orientationComboBox;
		wxTextCtrl* m_TextUserSizeX;
		wxTextCtrl* m_TextUserSizeY;
		wxStaticBitmap* m_PageLayoutExampleBitmap;
		wxStaticText* m_TextSheetCount;
		wxStaticText* m_TextSheetNumber;
		wxTextCtrl* m_TextRevision;
		wxCheckBox* m_RevisionExport;
		wxTextCtrl* m_TextTitle;
		wxCheckBox* m_TitleExport;
		wxTextCtrl* m_TextCompany;
		wxCheckBox* m_CompanyExport;
		wxTextCtrl* m_TextComment1;
		wxCheckBox* m_Comment1Export;
		wxTextCtrl* m_TextComment2;
		wxCheckBox* m_Comment2Export;
		wxTextCtrl* m_TextComment3;
		wxCheckBox* m_Comment3Export;
		wxTextCtrl* m_TextComment4;
		wxCheckBox* m_Comment4Export;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnCloseWindow( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnPaperSizeChoice( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPageOrientationChoice( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnUserPageSizeXTextUpdated( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnUserPageSizeYTextUpdated( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRevisionTextUpdated( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTitleTextUpdated( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCheckboxTitleClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCompanyTextUpdated( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnComment1TextUpdated( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnComment2TextUpdated( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnComment3TextUpdated( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnComment4TextUpdated( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOkClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_PAGES_SETTINGS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Page Settings"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 748,495 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_PAGES_SETTINGS_BASE();
	
};

#endif //__DIALOG_PAGE_SETTINGS_BASE_H__
