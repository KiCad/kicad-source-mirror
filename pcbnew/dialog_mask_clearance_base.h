///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __dialog_mask_clearance_base__
#define __dialog_mask_clearance_base__

#include <wx/intl.h>

#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_PADS_MASK_CLEARANCE_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_PADS_MASK_CLEARANCE_BASE : public wxDialog 
{
	DECLARE_EVENT_TABLE()
	private:
		
		// Private event handlers
		void _wxFB_OnButtonCancelClick( wxCommandEvent& event ){ OnButtonCancelClick( event ); }
		void _wxFB_OnButtonOkClick( wxCommandEvent& event ){ OnButtonOkClick( event ); }
		
	
	protected:
		wxStaticText* m_MaskClearanceTitle;
		wxTextCtrl* m_OptMaskMargin;
		wxStdDialogButtonSizer* m_sdbButtonsSizer;
		wxButton* m_sdbButtonsSizerOK;
		wxButton* m_sdbButtonsSizerCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnButtonCancelClick( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnButtonOkClick( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		DIALOG_PADS_MASK_CLEARANCE_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Pads Mask Clearance"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 256,117 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_PADS_MASK_CLEARANCE_BASE();
	
};

#endif //__dialog_mask_clearance_base__
