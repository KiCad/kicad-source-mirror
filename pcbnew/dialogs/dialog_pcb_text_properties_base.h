///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Sep  8 2010)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __dialog_pcb_text_properties_base__
#define __dialog_pcb_text_properties_base__

#include <wx/intl.h>

#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/radiobox.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_PCB_TEXT_PROPERTIES_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_PCB_TEXT_PROPERTIES_BASE : public wxDialog 
{
	private:
	
	protected:
		wxStaticText* m_TextLabel;
		wxTextCtrl* m_TextContentCtrl;
		wxStaticText* m_SizeXLabel;
		wxTextCtrl* m_SizeXCtrl;
		wxStaticText* m_SizeYLabel;
		wxTextCtrl* m_SizeYCtrl;
		wxStaticText* m_ThicknessLabel;
		wxTextCtrl* m_ThicknessCtrl;
		wxStaticText* m_PositionXLabel;
		wxTextCtrl* m_PositionXCtrl;
		wxStaticText* m_PositionYLabel;
		wxTextCtrl* m_PositionYCtrl;
		wxStaticText* m_LayerLabel;
		wxChoice* m_LayerSelectionCtrl;
		wxRadioBox* m_OrientationCtrl;
		wxRadioBox* m_StyleCtrl;
		wxRadioBox* m_DisplayCtrl;
		wxStdDialogButtonSizer* m_StandardSizer;
		wxButton* m_StandardSizerOK;
		wxButton* m_StandardSizerCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOkClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_PCB_TEXT_PROPERTIES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Text item properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 433,465 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER|wxSYSTEM_MENU );
		~DIALOG_PCB_TEXT_PROPERTIES_BASE();
	
};

#endif //__dialog_pcb_text_properties_base__
