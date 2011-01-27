///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Sep  8 2010)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __dialog_eeschema_options_base__
#define __dialog_eeschema_options_base__

#include <wx/intl.h>

#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/choice.h>
#include <wx/spinctrl.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/panel.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/textctrl.h>
#include <wx/notebook.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_EESCHEMA_OPTIONS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_EESCHEMA_OPTIONS_BASE : public wxDialog 
{
	DECLARE_EVENT_TABLE()
	private:
		
		// Private event handlers
		void _wxFB_OnChooseUnits( wxCommandEvent& event ){ OnChooseUnits( event ); }
		
	
	protected:
		wxNotebook* m_notebook1;
		wxPanel* m_panel1;
		wxStaticText* m_staticText2;
		wxChoice* m_choiceUnits;
		
		wxStaticText* m_staticText3;
		wxChoice* m_choiceGridSize;
		wxStaticText* m_staticGridUnits;
		wxStaticText* m_staticText5;
		wxSpinCtrl* m_spinLineWidth;
		wxStaticText* m_staticLineWidthUnits;
		wxStaticText* m_staticText7;
		wxSpinCtrl* m_spinTextSize;
		wxStaticText* m_staticTextSizeUnits;
		wxStaticText* m_staticText9;
		wxSpinCtrl* m_spinRepeatHorizontal;
		wxStaticText* m_staticRepeatXUnits;
		wxStaticText* m_staticText12;
		wxSpinCtrl* m_spinRepeatVertical;
		wxStaticText* m_staticRepeatYUnits;
		wxStaticText* m_staticText16;
		wxSpinCtrl* m_spinRepeatLabel;
		
		wxCheckBox* m_checkShowGrid;
		wxCheckBox* m_checkShowHiddenPins;
		wxCheckBox* m_checkAutoPan;
		wxCheckBox* m_checkHVOrientation;
		wxCheckBox* m_checkPageLimits;
		
		wxPanel* m_panel2;
		wxStaticText* m_staticText211;
		wxStaticText* m_staticText15;
		wxTextCtrl* m_fieldName1;
		wxStaticText* m_staticText161;
		wxTextCtrl* m_fieldName2;
		wxStaticText* m_staticText17;
		wxTextCtrl* m_fieldName3;
		wxStaticText* m_staticText18;
		wxTextCtrl* m_fieldName4;
		wxStaticText* m_staticText19;
		wxTextCtrl* m_fieldName5;
		wxStaticText* m_staticText20;
		wxTextCtrl* m_fieldName6;
		wxStaticText* m_staticText21;
		wxTextCtrl* m_fieldName7;
		wxStaticText* m_staticText22;
		wxTextCtrl* m_fieldName8;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnChooseUnits( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_EESCHEMA_OPTIONS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Schematic Editor Options"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_EESCHEMA_OPTIONS_BASE();
	
};

#endif //__dialog_eeschema_options_base__
