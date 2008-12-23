///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __dialog_display_options_base__
#define __dialog_display_options_base__

#include <wx/intl.h>

#include <wx/string.h>
#include <wx/radiobox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/checkbox.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DialogDisplayOptions_base
///////////////////////////////////////////////////////////////////////////////
class DialogDisplayOptions_base : public wxDialog 
{
	private:
	
	protected:
		enum
		{
			wxID_DISPLAY_TRACK = 1000,
			ID_SHOW_CLEARANCE,
			ID_VIAS_HOLES,
			ID_EDGES_MODULES,
			ID_TEXT_MODULES,
			ID_PADS_SHAPES,
		};
		
		wxRadioBox* m_OptDisplayTracks;
		wxRadioBox* m_OptDisplayTracksClearance;
		wxRadioBox* m_OptDisplayViaHole;
		wxRadioBox* m_OptDisplayModEdges;
		wxRadioBox* m_OptDisplayModTexts;
		wxRadioBox* m_OptDisplayPads;
		wxCheckBox* m_OptDisplayPadClearence;
		wxCheckBox* m_OptDisplayPadNumber;
		wxCheckBox* m_OptDisplayPadNoConn;
		wxRadioBox* m_OptDisplayDrawings;
		wxRadioBox* m_Show_Page_Limits;
		
		wxButton* m_buttonOK;
		wxButton* m_buttonCANCEL;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnInitDialog( wxInitDialogEvent& event ){ event.Skip(); }
		virtual void OnOkClick( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		DialogDisplayOptions_base( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 559,303 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DialogDisplayOptions_base();
	
};

#endif //__dialog_display_options_base__
