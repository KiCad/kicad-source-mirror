///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun 30 2011)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE_H__
#define __DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include <wx/listctrl.h>
#include <wx/string.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/radiobox.h>
#include <wx/checkbox.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE : public wxDialog 
{
	private:
	
	protected:
		wxListCtrl* fieldListCtrl;
		wxButton* addFieldButton;
		wxButton* deleteFieldButton;
		wxButton* moveUpButton;
		wxRadioBox* m_FieldHJustifyCtrl;
		wxRadioBox* m_FieldVJustifyCtrl;
		wxCheckBox* showCheckBox;
		wxCheckBox* rotateCheckBox;
		wxRadioBox* m_StyleRadioBox;
		wxStaticText* fieldNameLabel;
		wxTextCtrl* fieldNameTextCtrl;
		wxStaticText* fieldValueLabel;
		wxTextCtrl* fieldValueTextCtrl;
		wxStaticText* textSizeLabel;
		wxTextCtrl* textSizeTextCtrl;
		wxStaticText* posXLabel;
		wxTextCtrl* posXTextCtrl;
		wxStaticText* posYLabel;
		wxTextCtrl* posYTextCtrl;
		wxStdDialogButtonSizer* stdDialogButtonSizer;
		wxButton* stdDialogButtonSizerOK;
		wxButton* stdDialogButtonSizerCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnInitDialog( wxInitDialogEvent& event ) { event.Skip(); }
		virtual void OnListItemDeselected( wxListEvent& event ) { event.Skip(); }
		virtual void OnListItemSelected( wxListEvent& event ) { event.Skip(); }
		virtual void addFieldButtonHandler( wxCommandEvent& event ) { event.Skip(); }
		virtual void deleteFieldButtonHandler( wxCommandEvent& event ) { event.Skip(); }
		virtual void moveUpButtonHandler( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancelButtonClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOKButtonClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Fields Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 615,550 ), long style = wxCAPTION|wxCLOSE_BOX|wxDEFAULT_DIALOG_STYLE|wxMAXIMIZE_BOX|wxMINIMIZE_BOX|wxRESIZE_BORDER|wxSYSTEM_MENU ); 
		~DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE();
	
};

#endif //__DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE_H__
