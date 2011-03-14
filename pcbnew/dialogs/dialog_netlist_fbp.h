///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Sep  8 2010)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __dialog_netlist_fbp__
#define __dialog_netlist_fbp__

#include <wx/intl.h>

#include <wx/string.h>
#include <wx/radiobox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/statline.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_NETLIST_FBP
///////////////////////////////////////////////////////////////////////////////
class DIALOG_NETLIST_FBP : public wxDialog 
{
	private:
	
	protected:
		enum
		{
			ID_OPEN_NELIST = 1000,
			ID_READ_NETLIST_FILE,
			ID_TEST_NETLIST,
			ID_COMPILE_RATSNEST,
		};
		
		wxRadioBox* m_Select_By_Timestamp;
		wxRadioBox* m_ChangeExistingFootprintCtrl;
		wxRadioBox* m_DeleteBadTracks;
		wxRadioBox* m_RemoveExtraFootprintsCtrl;
		wxButton* m_button1;
		wxButton* m_button2;
		wxButton* m_button3;
		wxButton* m_button4;
		wxButton* m_button5;
		wxStaticLine* m_staticline1;
		wxStaticText* m_staticTextNetfilename;
		wxTextCtrl* m_NetlistFilenameCtrl;
		wxStaticText* m_staticText1;
		wxTextCtrl* m_MessageWindow;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnOpenNelistClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnReadNetlistFileClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTestFootprintsClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCompileRatsnestClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_NETLIST_FBP( wxWindow* parent, wxWindowID id = wxID_CANCEL, const wxString& title = _("Netlist"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 519,431 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_NETLIST_FBP();
	
};

#endif //__dialog_netlist_fbp__
