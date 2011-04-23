///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Nov 17 2010)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __dialog_layers_select_to_pcb_base__
#define __dialog_layers_select_to_pcb_base__

#include <wx/intl.h>

#include <wx/sizer.h>
#include <wx/gdicmn.h>
#include <wx/statline.h>
#include <wx/string.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/statbox.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class LAYERS_TABLE_DIALOG_BASE
///////////////////////////////////////////////////////////////////////////////
class LAYERS_TABLE_DIALOG_BASE : public wxDialog 
{
	DECLARE_EVENT_TABLE()
	private:
		
		// Private event handlers
		void _wxFB_OnCancelClick( wxCommandEvent& event ){ OnCancelClick( event ); }
		void _wxFB_OnOkClick( wxCommandEvent& event ){ OnOkClick( event ); }
		
	
	protected:
		enum
		{
			ID_M_STATICLINESEP = 1000,
		};
		
		wxStaticBoxSizer* sbSizerLayersTable;
		wxFlexGridSizer* m_flexLeftColumnBoxSizer;
		wxStaticLine* m_staticlineSep;
		wxFlexGridSizer* m_flexRightColumnBoxSizer;
		wxButton* m_buttonStore;
		wxButton* m_buttonRetrieve;
		wxStaticLine* m_staticline1;
		wxStdDialogButtonSizer* m_sdbSizerButtons;
		wxButton* m_sdbSizerButtonsOK;
		wxButton* m_sdbSizerButtonsCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOkClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		LAYERS_TABLE_DIALOG_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Layer selection:"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 400,286 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~LAYERS_TABLE_DIALOG_BASE();
	
};

#endif //__dialog_layers_select_to_pcb_base__
