///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  5 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_PNS_DIFF_PAIR_DIMENSIONS_BASE_H__
#define __DIALOG_PNS_DIFF_PAIR_DIMENSIONS_BASE_H__

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
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/statline.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_PNS_DIFF_PAIR_DIMENSIONS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_PNS_DIFF_PAIR_DIMENSIONS_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxStaticText* m_traceWidthLabel;
		wxTextCtrl* m_traceWidthText;
		wxStaticText* m_traceWidthUnit;
		wxStaticText* m_traceGapLabel;
		wxTextCtrl* m_traceGapText;
		wxStaticText* m_traceGapUnit;
		wxStaticText* m_viaGapLabel;
		wxTextCtrl* m_viaGapText;
		wxStaticText* m_viaGapUnit;
		wxCheckBox* m_viaTraceGapEqual;
		wxStaticLine* m_staticline1;
		wxStdDialogButtonSizer* m_stdButtons;
		wxButton* m_stdButtonsOK;
		wxButton* m_stdButtonsCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnViaTraceGapEqualCheck( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOkClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_PNS_DIFF_PAIR_DIMENSIONS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Differential Pair Dimensions"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 400,216 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_PNS_DIFF_PAIR_DIMENSIONS_BASE();
	
};

#endif //__DIALOG_PNS_DIFF_PAIR_DIMENSIONS_BASE_H__
