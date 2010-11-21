///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Sep  8 2010)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __dialog_cleaning_options_base__
#define __dialog_cleaning_options_base__

#include <wx/intl.h>

#include <wx/string.h>
#include <wx/checkbox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_CLEANING_OPTIONS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_CLEANING_OPTIONS_BASE : public wxDialog 
{
	private:
	
	protected:
		wxCheckBox* m_cleanViasOpt;
		wxCheckBox* m_mergeSegmOpt;
		wxCheckBox* m_deleteUnconnectedOpt;
		wxCheckBox* m_reconnectToPadsOpt;
		wxStaticLine* m_staticline;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnCloseWindow( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOKClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		bool cleanVias; 
		bool mergeSegments; 
		bool deleteUnconnectedSegm; 
		bool connectToPads; 
		
		DIALOG_CLEANING_OPTIONS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Cleaning options"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 243,181 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_CLEANING_OPTIONS_BASE();
	
};

#endif //__dialog_cleaning_options_base__
