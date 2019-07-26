///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_FIND_BASE_H__
#define __DIALOG_FIND_BASE_H__

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
#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_FIND_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_FIND_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxStaticText* m_staticText1;
		wxComboBox* m_SearchCombo;
		wxCheckBox* m_matchCase;
		wxCheckBox* m_matchWords;
		wxCheckBox* m_wildcards;
		wxButton* m_button1;
		wxButton* m_button2;
		wxButton* m_button3;
		
		// Virtual event handlers, overide them in your derived class
		virtual void onClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnInitDialog( wxInitDialogEvent& event ) { event.Skip(); }
		virtual void OnTextEnter( wxCommandEvent& event ) { event.Skip(); }
		virtual void onButtonFindItemClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void onButtonFindMarkerClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void onButtonCloseClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_FIND_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Find"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_FIND_BASE();
	
};

#endif //__DIALOG_FIND_BASE_H__
