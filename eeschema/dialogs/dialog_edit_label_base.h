///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Mar  9 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_EDIT_LABEL_BASE_H__
#define __DIALOG_EDIT_LABEL_BASE_H__

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
#include <wx/textctrl.h>
#include <wx/valtext.h>
#include <wx/sizer.h>
#include <wx/radiobox.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_LABEL_EDITOR_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_LABEL_EDITOR_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		enum
		{
			wxID_VALUESINGLE = 1000,
			wxID_VALUEMULTI,
			wxID_SIZE
		};
		
		wxFlexGridSizer* m_textControlSizer;
		wxStaticText* m_staticText1;
		wxTextCtrl* m_textLabelSingleLine;
		wxTextCtrl* m_textLabelMultiLine;
		wxStaticText* m_SizeTitle;
		wxTextCtrl* m_TextSize;
		wxStaticText* m_staticSizeUnits;
		wxRadioBox* m_TextOrient;
		wxRadioBox* m_TextStyle;
		wxRadioBox* m_TextShape;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnEnterKey( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOkClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		wxString m_labelText; 
		
		DIALOG_LABEL_EDITOR_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Text Editor"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_LABEL_EDITOR_BASE();
	
};

#endif //__DIALOG_EDIT_LABEL_BASE_H__
