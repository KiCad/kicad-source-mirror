///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __dialog_bodygraphictext_properties_base__
#define __dialog_bodygraphictext_properties_base__

#include <wx/intl.h>

#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/radiobox.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class Dialog_BodyGraphicText_Properties_base
///////////////////////////////////////////////////////////////////////////////
class Dialog_BodyGraphicText_Properties_base : public wxDialog 
{
	private:
	
	protected:
		wxStaticText* m_staticText1;
		wxTextCtrl* m_TextValue;
		wxCheckBox* m_CommonUnit;
		wxCheckBox* m_CommonConvert;
		wxCheckBox* m_Orient;
		wxStaticText* m_TextSizeText;
		wxTextCtrl* m_TextSize;
		wxRadioBox* m_TextShapeOpt;
		wxButton* m_buttonOK;
		wxButton* m_buttonCANCEL;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnInitDialog( wxInitDialogEvent& event ){ event.Skip(); }
		virtual void OnOkClick( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		Dialog_BodyGraphicText_Properties_base( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Graphic text properties:"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 360,180 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~Dialog_BodyGraphicText_Properties_base();
	
};

#endif //__dialog_bodygraphictext_properties_base__
