///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  5 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_DISPLAY_OPTIONS_BASE_H__
#define __DIALOG_DISPLAY_OPTIONS_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class DIALOG_SHIM;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/checkbox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/statline.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

#define ID_PADFILL_OPT 1000

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_FOOTPRINTS_DISPLAY_OPTIONS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_FOOTPRINTS_DISPLAY_OPTIONS_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxCheckBox* m_EdgesDisplayOption;
		wxCheckBox* m_TextDisplayOption;
		wxCheckBox* m_ShowPadSketch;
		wxCheckBox* m_ShowPadNum;
		wxCheckBox* m_IsZoomNoCenter;
		wxCheckBox* m_IsMiddleButtonPan;
		wxCheckBox* m_IsMiddleButtonPanLimited;
		wxStaticLine* m_staticline1;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Apply;
		wxButton* m_sdbSizer1Cancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnMiddleBtnPanEnbl( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnApplyClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOkClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_FOOTPRINTS_DISPLAY_OPTIONS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Display Options"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 425,206 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_FOOTPRINTS_DISPLAY_OPTIONS_BASE();
	
};

#endif //__DIALOG_DISPLAY_OPTIONS_BASE_H__
