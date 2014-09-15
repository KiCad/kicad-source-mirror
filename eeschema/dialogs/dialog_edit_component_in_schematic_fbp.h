///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Nov  6 2013)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_FBP_H__
#define __DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_FBP_H__

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
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/radiobox.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/button.h>
#include <wx/statbox.h>
#include <wx/listctrl.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_FBP
///////////////////////////////////////////////////////////////////////////////
class DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_FBP : public DIALOG_SHIM
{
	private:
	
	protected:
		wxStaticText* m_staticTextUnit;
		wxChoice* unitChoice;
		wxStaticText* unitsInterchageableText;
		wxStaticText* unitsInterchageableLabel;
		wxRadioBox* orientationRadioBox;
		wxRadioBox* mirrorRadioBox;
		wxStaticText* m_staticTextChipname;
		wxTextCtrl* chipnameTextCtrl;
		wxCheckBox* convertCheckBox;
		wxButton* defaultsButton;
		wxStaticText* m_staticTextTimeStamp;
		wxTextCtrl* m_textCtrlTimeStamp;
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
		wxButton* m_show_datasheet_button;
		wxStaticText* textSizeLabel;
		wxTextCtrl* textSizeTextCtrl;
		wxStaticText* m_staticTextUnitSize;
		wxStaticText* posXLabel;
		wxTextCtrl* posXTextCtrl;
		wxStaticText* m_staticTextUnitPosX;
		wxStaticText* posYLabel;
		wxTextCtrl* posYTextCtrl;
		wxStaticText* m_staticTextUnitPosY;
		wxStdDialogButtonSizer* stdDialogButtonSizer;
		wxButton* stdDialogButtonSizerOK;
		wxButton* stdDialogButtonSizerCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnCloseDialog( wxCloseEvent& event ) { event.Skip(); }
		virtual void SetInitCmp( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnListItemDeselected( wxListEvent& event ) { event.Skip(); }
		virtual void OnListItemSelected( wxListEvent& event ) { event.Skip(); }
		virtual void addFieldButtonHandler( wxCommandEvent& event ) { event.Skip(); }
		virtual void deleteFieldButtonHandler( wxCommandEvent& event ) { event.Skip(); }
		virtual void moveUpButtonHandler( wxCommandEvent& event ) { event.Skip(); }
		virtual void showButtonHandler( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancelButtonClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOKButtonClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_FBP( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Component Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxCAPTION|wxCLOSE_BOX|wxDEFAULT_DIALOG_STYLE|wxMAXIMIZE_BOX|wxMINIMIZE_BOX|wxRESIZE_BORDER|wxSYSTEM_MENU ); 
		~DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_FBP();
	
};

#endif //__DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_FBP_H__
