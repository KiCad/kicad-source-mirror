///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  6 2011)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __dialog_page_settings_base__
#define __dialog_page_settings_base__

#include <wx/intl.h>

#include <wx/string.h>
#include <wx/radiobox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/statbox.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

#define ID_TEXTCTRL_USER_PAGE_SIZE_X 1000
#define ID_TEXTCTRL_USER_PAGE_SIZE_Y 1001
#define ID_TEXTCTRL_REVISION 1002
#define ID_CHECKBOX_REVISION 1003
#define ID_TEXTCTRL_TITLE 1004
#define ID_TEXTCTRL_COMPANY 1005
#define ID_CHECKBOX_COMPANY 1006
#define ID_TEXTCTRL_COMMENT1 1007
#define ID_CHECKBOX_COMMENT1 1008
#define ID_TEXTCTRL_COMMENT2 1009
#define ID_CHECKBOX_COMMENT2 1010
#define ID_TEXTCTRL_COMMENT3 1011
#define ID_CHECKBOX_COMMENT3 1012
#define ID_TEXTCTRL_COMMENT4 1013
#define ID_CHECKBOX_COMMENT4 1014

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_PAGES_SETTINGS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_PAGES_SETTINGS_BASE : public wxDialog 
{
	private:
	
	protected:
		wxRadioBox* m_PageSizeBox;
		
		wxStaticText* UserPageSizeX;
		wxTextCtrl* m_TextUserSizeX;
		wxStaticText* UserPageSizeY;
		wxTextCtrl* m_TextUserSizeY;
		
		
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
		virtual void OnTextctrlUserPageSizeXTextUpdated( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTextctrlUserPageSizeYTextUpdated( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCheckboxTitleClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOkClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_PAGES_SETTINGS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Page Settings"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 439,497 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_PAGES_SETTINGS_BASE();
	
};

#endif //__dialog_page_settings_base__
