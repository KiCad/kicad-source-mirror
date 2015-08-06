///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Mar 13 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_CLEANING_OPTIONS_BASE_H__
#define __DIALOG_CLEANING_OPTIONS_BASE_H__

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
#include <wx/statline.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_CLEANING_OPTIONS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_CLEANING_OPTIONS_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxCheckBox* m_cleanViasOpt;
		wxCheckBox* m_mergeSegmOpt;
		wxCheckBox* m_deleteUnconnectedOpt;
		wxStaticLine* m_staticline;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOKClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_CLEANING_OPTIONS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Cleaning Options"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 257,185 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_CLEANING_OPTIONS_BASE();
	
};

#endif //__DIALOG_CLEANING_OPTIONS_BASE_H__
