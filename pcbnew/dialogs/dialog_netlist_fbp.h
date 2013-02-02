///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct  8 2012)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_NETLIST_FBP_H__
#define __DIALOG_NETLIST_FBP_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class DIALOG_SHIM;

#include "dialog_shim.h"
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
class DIALOG_NETLIST_FBP : public DIALOG_SHIM
{
	private:
	
	protected:
		enum
		{
			ID_OPEN_NELIST = 1000,
			ID_READ_NETLIST_FILE,
			ID_TEST_NETLIST,
			ID_COMPILE_RATSNEST
		};
		
		wxRadioBox* m_Select_By_Timestamp;
		wxRadioBox* m_cmpNameSourceOpt;
		wxRadioBox* m_ChangeExistingFootprintCtrl;
		wxRadioBox* m_DeleteBadTracks;
		wxRadioBox* m_RemoveExtraFootprintsCtrl;
		wxButton* m_buttonBrowse;
		wxButton* m_buttonRead;
		wxButton* m_buttonFPTest;
		wxButton* m_buttonRebild;
		wxButton* m_buttonClose;
		wxStaticLine* m_staticline1;
		wxStaticText* m_staticTextNetfilename;
		wxTextCtrl* m_NetlistFilenameCtrl;
		wxStaticText* m_staticText1;
		wxTextCtrl* m_MessageWindow;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnOpenNetlistClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnReadNetlistFileClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTestFootprintsClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCompileRatsnestClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_NETLIST_FBP( wxWindow* parent, wxWindowID id = wxID_CANCEL, const wxString& title = _("Netlist"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 519,431 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_NETLIST_FBP();
	
};

#endif //__DIALOG_NETLIST_FBP_H__
