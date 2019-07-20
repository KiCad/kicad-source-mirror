///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_SCHEMATIC_FIND_BASE_H__
#define __DIALOG_SCHEMATIC_FIND_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/combobox.h>
#include <wx/radiobut.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/gbsizer.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_SCH_FIND_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_SCH_FIND_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxStaticText* m_staticText1;
		wxComboBox* m_comboFind;
		wxStaticText* m_staticReplace;
		wxComboBox* m_comboReplace;
		wxStaticText* m_staticDirection;
		wxRadioButton* m_radioForward;
		wxRadioButton* m_radioBackward;
		wxCheckBox* m_checkMatchCase;
		wxCheckBox* m_checkWholeWord;
		wxCheckBox* m_checkWildcardMatch;
		wxCheckBox* m_checkAllFields;
		wxCheckBox* m_checkAllPins;
		wxCheckBox* m_checkCurrentSheetOnly;
		wxCheckBox* m_checkReplaceReferences;
		wxButton* m_buttonFind;
		wxButton* m_buttonReplace;
		wxButton* m_buttonReplaceAll;
		wxButton* m_buttonCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnSearchForText( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTextEnter( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnUpdateDrcUI( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void OnOptions( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnFind( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnReplace( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnUpdateReplaceUI( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void OnUpdateReplaceAllUI( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void OnCancel( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_SCH_FIND_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Find"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_SCH_FIND_BASE();
	
};

#endif //__DIALOG_SCHEMATIC_FIND_BASE_H__
