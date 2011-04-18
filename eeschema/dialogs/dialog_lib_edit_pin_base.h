///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Nov 17 2010)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __dialog_lib_edit_pin_base__
#define __dialog_lib_edit_pin_base__

#include <wx/intl.h>

class wxBitmapComboBox;

#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/combobox.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/statline.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_LIB_EDIT_PIN_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_LIB_EDIT_PIN_BASE : public wxDialog 
{
	DECLARE_EVENT_TABLE()
	private:
		
		// Private event handlers
		void _wxFB_OnCBpartSelection( wxCommandEvent& event ){ OnCBpartSelection( event ); }
		
	
	protected:
		enum
		{
			ID_M_TEXTPINNAME = 1000,
			ID_M_TEXTPINNAMETEXTSIZE,
			ID_M_STATICTEXTPADNAME,
			ID_M_TEXTPADNAME,
			ID_M_TEXTPADNAMETEXTSIZE,
			ID_M_STATICTEXTPINLEN,
		};
		
		wxStaticText* m_staticTextPinName;
		wxTextCtrl* m_textPinName;
		
		wxStaticText* m_staticTextNameSize;
		wxTextCtrl* m_textPinNameTextSize;
		wxStaticText* m_staticNameTextSizeUnits;
		wxStaticText* m_staticTextPadName;
		wxTextCtrl* m_textPadName;
		
		wxStaticText* m_staticTextPadNameSize;
		wxTextCtrl* m_textPadNameTextSize;
		wxStaticText* m_staticNumberTextSizeUnits;
		wxStaticText* m_staticTextOrient;
		wxBitmapComboBox* m_choiceOrientation;
		
		wxStaticText* m_staticTextPinLen;
		wxTextCtrl* m_textLength;
		wxStaticText* m_staticLengthUnits;
		wxStaticText* m_staticTextEType;
		wxBitmapComboBox* m_choiceElectricalType;
		
		
		
		
		wxStaticText* m_staticTextGstyle;
		wxBitmapComboBox* m_choiceStyle;
		
		
		
		wxCheckBox* m_checkApplyToAllParts;
		wxCheckBox* m_checkApplyToAllConversions;
		wxCheckBox* m_checkShow;
		
		wxStaticLine* m_staticline1;
		wxStdDialogButtonSizer* m_sdbSizerButtons;
		wxButton* m_sdbSizerButtonsOK;
		wxButton* m_sdbSizerButtonsCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnCBpartSelection( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_LIB_EDIT_PIN_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Pin Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 487,344 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_LIB_EDIT_PIN_BASE();
	
};

#endif //__dialog_lib_edit_pin_base__
